/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/adminapi/cluster/cluster_set_option.h"
#include "modules/adminapi/cluster/replicaset/set_option.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {
Cluster_set_option::Cluster_set_option(Cluster_impl *cluster,
                                       const std::string &option,
                                       const std::string &value)
    : m_cluster(cluster), m_option(option), m_value_str(value) {
  assert(cluster);
}

Cluster_set_option::Cluster_set_option(Cluster_impl *cluster,
                                       const std::string &option, int64_t value)
    : m_cluster(cluster), m_option(option), m_value_int(value) {
  assert(cluster);
}

Cluster_set_option::Cluster_set_option(Cluster_impl *cluster,
                                       const std::string &option, bool value)
    : m_cluster(cluster), m_option(option), m_value_bool(value) {
  assert(cluster);
}

Cluster_set_option::~Cluster_set_option() {}

void Cluster_set_option::ensure_option_valid() {
  // - Validate if the option is valid, being the accepted values:
  //   - clusterName
  //   - disableClone
  if (m_option == kClusterName) {
    if (!m_value_int.is_null() || !m_value_bool.is_null()) {
      throw shcore::Exception::argument_error(
          "Invalid value for 'clusterName': Argument #2 is expected to be a "
          "string.");
    } else {
      mysqlsh::dba::validate_cluster_name(*m_value_str,
                                          Cluster_type::GROUP_REPLICATION);
    }
  } else if (m_option == kDisableClone) {
    // Ensure forceClone is a boolean or integer value
    if (!m_value_str.is_null()) {
      throw shcore::Exception::argument_error(
          "Invalid value for 'disableClone': Argument #2 is expected to be a "
          "boolean.");
    }
  }
}

void Cluster_set_option::check_disable_clone_support() {
  auto console = mysqlsh::current_console();

  log_debug("Checking of disableClone is not supported by all cluster members");

  // Get all cluster instances
  auto members = mysqlshdk::gr::get_members(*m_cluster->get_target_server());

  size_t bad_count = 0;

  // Check if all instances have a version that supports clone
  for (const auto &member : members) {
    if (member.version.empty() ||
        !is_option_supported(mysqlshdk::utils::Version(member.version),
                             m_option, k_global_cluster_supported_options)) {
      bad_count++;
    }
  }

  if (bad_count > 0) {
    throw shcore::Exception::runtime_error(
        "Option 'disableClone' not supported on Cluster.");
  }
}

void Cluster_set_option::update_disable_clone_option(bool disable_clone) {
  size_t count, cluster_size;

  // Get the cluster size (instances)
  cluster_size =
      m_cluster->get_metadata_storage()->get_cluster_size(m_cluster->get_id());

  // Enable/Disable the clone plugin on the target cluster
  count = m_cluster->setup_clone_plugin(!disable_clone);

  // Update disableClone value in the metadata only if we succeeded to
  // update the plugin status in at least one of the members
  if (count != cluster_size) {
    m_cluster->get_metadata_storage()->update_cluster_attribute(
        m_cluster->get_id(), k_cluster_attribute_disable_clone,
        disable_clone ? shcore::Value::True() : shcore::Value::False());
  } else {
    // If we failed to update the value in all cluster members, thrown an
    // exception
    throw shcore::Exception::runtime_error(
        "Failed to set the value of '" + m_option +
        "' in the Cluster: Unable to update the clone plugin status in all "
        "cluster members.");
  }
}

void Cluster_set_option::prepare() {
  // Check if the option is a ReplicaSet option and create the
  // Replicaset Set_option object to handle it. If the option it not a
  // ReplicaSet option then verify if it is a Cluster option and thrown an
  // error in case is not
  if (k_global_replicaset_supported_options.count(m_option) != 0) {
    std::shared_ptr<GRReplicaSet> default_rs =
        m_cluster->get_default_replicaset();

    if (!m_value_str.is_null()) {
      m_replicaset_set_option =
          std::make_unique<Set_option>(*default_rs, m_option, *m_value_str);
    } else {
      m_replicaset_set_option =
          std::make_unique<Set_option>(*default_rs, m_option, *m_value_int);
    }

    // If m_replicaset_set_option is set it means that the option is a
    // ReplicaSet option thus must be validated by Replicaset_set_option
    m_replicaset_set_option->prepare();
  } else if (k_global_cluster_supported_options.count(m_option) != 0) {
    // The option is a Cluster option, validate if the option is valid
    ensure_option_valid();

    // Verify user privileges to execute operation;
    ensure_user_privileges(*m_cluster->get_target_server());
  } else {
    throw shcore::Exception::argument_error("Option '" + m_option +
                                            "' not supported.");
  }
}

shcore::Value Cluster_set_option::execute() {
  auto console = mysqlsh::current_console();

  // Check if m_replicaset_set_option is set, meaning that the option is a
  // ReplicaSet option thus the operation must be executed by the
  // Replicaset_set_option object
  if (m_replicaset_set_option) {
    // Execute Replicaset_set_option operations.
    m_replicaset_set_option->execute();
  } else {
    std::string current_full_cluster_name = m_cluster->get_name();
    std::string current_cluster_name = m_cluster->cluster_name();

    if (m_option == kClusterName) {
      auto md = m_cluster->get_metadata_storage();

      console->print_info("Setting the value of '" + m_option + "' to '" +
                          *m_value_str + "' in the Cluster ...");
      console->print_info();

      std::string domain_name;
      std::string cluster_name;
      parse_fully_qualified_cluster_name(*m_value_str, &domain_name, nullptr,
                                         &cluster_name);

      m_cluster->set_cluster_name(cluster_name);

      try {
        md->update_cluster_name(m_cluster->get_id(), m_cluster->cluster_name());
      } catch (...) {
        // revert changes
        m_cluster->set_cluster_name(current_cluster_name);
        throw;
      }

      console->print_info("Successfully set the value of '" + m_option +
                          "' to '" + *m_value_str + "' in the Cluster: '" +
                          current_full_cluster_name + "'.");
    } else if (m_option == kDisableClone) {
      // Ensure forceClone is a boolean or integer value
      std::string m_value_printable =
          ((!m_value_bool.is_null() && *m_value_bool == true) ||
           (!m_value_int.is_null() && *m_value_int >= 1))
              ? "true"
              : "false";

      console->print_info("Setting the value of '" + m_option + "' to '" +
                          m_value_printable + "' in the Cluster ...");
      console->print_info();

      check_disable_clone_support();

      if (!m_value_int.is_null()) {
        if (*m_value_int >= 1) {
          update_disable_clone_option(true);
        } else {
          update_disable_clone_option(false);
        }
      } else if (!m_value_bool.is_null()) {
        update_disable_clone_option(*m_value_bool);
      }

      console->print_info("Successfully set the value of '" + m_option +
                          "' to '" + m_value_printable + "' in the Cluster: '" +
                          current_cluster_name + "'.");
    }
  }

  return shcore::Value();
}

void Cluster_set_option::rollback() {
  if (m_replicaset_set_option) {
    m_replicaset_set_option->rollback();
  }
}

void Cluster_set_option::finish() {
  if (m_replicaset_set_option) {
    m_replicaset_set_option->finish();
  }

  // Reset all auxiliary (temporary) data used for the operation execution.
  m_option.clear();
}

}  // namespace dba
}  // namespace mysqlsh
