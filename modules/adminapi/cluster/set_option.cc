/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/set_option.h"
#include "adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Set_option::Set_option(Cluster_impl *cluster, const std::string &option,
                       const std::string &value)
    : m_cluster(cluster), m_option(option), m_value_str(value) {
  assert(cluster);
}

Set_option::Set_option(Cluster_impl *cluster, const std::string &option,
                       int64_t value)
    : m_cluster(cluster), m_option(option), m_value_int(value) {
  assert(cluster);
}

Set_option::Set_option(Cluster_impl *cluster, const std::string &option,
                       bool value)
    : m_cluster(cluster), m_option(option), m_value_bool(value) {
  assert(cluster);
}

Set_option::~Set_option() {}

void Set_option::ensure_option_valid() {
  /* - Validate if the option is valid, being the accepted values:
   *     - clusterName
   *     - disableClone
   *     - exitStateAction
   *     - memberWeight
   *     - failoverConsistency
   *     - consistency
   *     - expelTimeout
   *     - replicationAllowedHost
   */
  if (k_global_cluster_supported_options.count(m_option) == 0 &&
      m_option != kClusterName && m_option != kDisableClone &&
      m_option != kReplicationAllowedHost) {
    throw shcore::Exception::argument_error("Option '" + m_option +
                                            "' not supported.");
  }

  // Verify the deprecation of failoverConsistency
  if (m_option == kFailoverConsistency) {
    auto console = mysqlsh::current_console();
    console->print_warning(
        "The failoverConsistency option is deprecated. "
        "Please use the consistency option instead.");
  } else if (m_option == kClusterName) {
    // Validate if the clusterName value is valid
    if (!m_value_int.is_null() || !m_value_bool.is_null()) {
      throw shcore::Exception::argument_error(
          "Invalid value for 'clusterName': Argument #2 is expected to be a "
          "string.");
    } else {
      mysqlsh::dba::validate_cluster_name(*m_value_str,
                                          Cluster_type::GROUP_REPLICATION);
    }
  } else if (m_option == kDisableClone) {
    // Validate if the disableClone value is valid
    // Ensure disableClone is a boolean or integer value
    if (!m_value_str.is_null()) {
      throw shcore::Exception::type_error(
          "Invalid value for 'disableClone': Argument #2 is expected to be a "
          "boolean.");
    }
  } else if (m_option == kReplicationAllowedHost) {
    if (m_value_str.is_null() || m_value_str->empty()) {
      throw shcore::Exception::argument_error(
          "Invalid value for 'replicationAllowedHost': Argument #2 is expected "
          "to be a string.");
    }
  }
}

void Set_option::check_disable_clone_support() {
  auto console = mysqlsh::current_console();

  log_debug("Checking of disableClone is not supported by all cluster members");

  // Get all cluster instances
  auto members = mysqlshdk::gr::get_members(*m_cluster->get_cluster_server());

  size_t bad_count = 0;

  // Check if all instances have a version that supports clone
  for (const auto &member : members) {
    if (member.version.empty() ||
        !is_option_supported(
            mysqlshdk::utils::Version(member.version), m_option,
            {{kDisableClone,
              {"",
               mysqlshdk::mysql::k_mysql_clone_plugin_initial_version,
               {}}}})) {
      bad_count++;
    }
  }

  if (bad_count > 0) {
    throw shcore::Exception::runtime_error(
        "Option 'disableClone' not supported on Cluster.");
  }
}

void Set_option::update_disable_clone_option(bool disable_clone) {
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

void Set_option::connect_all_members() {
  // Get cluster session to use the same authentication credentials for all
  // cluster instances.
  Connection_options cluster_cnx_opt =
      m_cluster->get_cluster_server()->get_connection_options();

  auto console = mysqlsh::current_console();

  // Check if all instances have the ONLINE state
  for (const auto &instance_def : m_cluster->get_instances_with_state()) {
    // Instance is ONLINE, initialize and populate the internal cluster
    // instances list

    // Establish a session to the instance
    // Set login credentials to connect to instance.
    // NOTE: It is assumed that the same login credentials can be used to
    // connect to all cluster instances.
    Connection_options instance_cnx_opts =
        shcore::get_connection_options(instance_def.first.endpoint, false);
    instance_cnx_opts.set_login_options_from(cluster_cnx_opt);

    log_debug("Connecting to instance '%s'.",
              instance_def.first.endpoint.c_str());

    // Establish a session to the instance
    try {
      // Add the instance to instances internal list
      m_cluster_instances.emplace_back(Instance::connect(instance_cnx_opts));
    } catch (const std::exception &err) {
      log_debug("Failed to connect to instance: %s", err.what());

      console->print_error(
          "Unable to connect to instance '" + instance_def.first.endpoint +
          "'. Please, verify connection credentials and make sure the "
          "instance is available.");

      throw shcore::Exception::runtime_error(err.what());
    }
  }
}

void Set_option::ensure_option_supported_all_members_cluster() {
  auto console = mysqlsh::current_console();

  log_debug("Checking if all members of the cluster support the option '%s'",
            m_option.c_str());

  for (const auto &instance : m_cluster_instances) {
    std::string instance_address = instance->descr();

    // Verify if the instance version is supported
    bool is_supported = is_option_supported(instance->get_version(), m_option,
                                            k_global_cluster_supported_options);

    if (!is_supported) {
      console->print_error(
          "The instance '" + instance_address + "' has the version " +
          instance->get_version().get_full() +
          " which does not support the option '" + m_option + "'.");

      throw shcore::Exception::runtime_error(
          "One or more instances of the cluster have a version that does not "
          "support this operation.");
    }
  }
}

void Set_option::prepare() {
  // Verify user privileges to execute operation;
  ensure_user_privileges(*m_cluster->get_cluster_server());

  // Validate if the option is valid
  ensure_option_valid();

  // Verify if all cluster members support the option
  // NOTE: clusterName and disableClone do not require this validation
  if (m_option != kClusterName && m_option != kDisableClone &&
      m_option != kReplicationAllowedHost) {
    ensure_option_supported_all_members_cluster();

    // Get the Cluster Config Object
    m_cfg = m_cluster->create_config_object();

    if (m_option == kAutoRejoinTries && !m_value_int.is_null() &&
        *m_value_int != 0) {
      auto console = mysqlsh::current_console();
      std::string warn_msg =
          "Each cluster member will only proceed according to its "
          "exitStateAction if auto-rejoin fails (i.e. all retry attempts are "
          "exhausted).";
      console->print_warning(warn_msg);
      console->print_info();
    }
  }

  // If disableClone check if the target cluster supports it
  if (m_option == kDisableClone) {
    check_disable_clone_support();
  }
}

shcore::Value Set_option::execute() {
  auto console = mysqlsh::current_console();

  if (m_option != kClusterName && m_option != kDisableClone &&
      m_option != kReplicationAllowedHost) {
    // Update the option values in all Cluster members:
    std::string option_gr_variable =
        k_global_cluster_supported_options.at(m_option).option_variable;

    console->print_info(
        "Setting the value of '" + m_option + "' to '" +
        (m_value_str.is_null() ? std::to_string(*m_value_int) : *m_value_str) +
        "' in all cluster members ...");
    console->print_info();

    if (!m_value_str.is_null()) {
      m_cfg->set(option_gr_variable, m_value_str);
    } else {
      m_cfg->set(option_gr_variable, m_value_int);
    }

    m_cfg->apply();

    console->print_info("Successfully set the value of '" + m_option +
                        "' to '" +
                        ((m_value_str.is_null() ? std::to_string(*m_value_int)
                                                : *m_value_str)) +
                        "' in the '" + m_cluster->get_name() + "' cluster.");

    return shcore::Value();
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
    } else if (m_option == kReplicationAllowedHost) {
      m_cluster->update_replication_allowed_host(*m_value_str);

      m_cluster->get_metadata_storage()->update_cluster_attribute(
          m_cluster->get_id(), k_cluster_attribute_replication_allowed_host,
          shcore::Value(*m_value_str));

      current_console()->print_info(shcore::str_format(
          "Internally managed GR recovery accounts updated for Cluster '%s'",
          m_cluster->get_name().c_str()));
    }
  }

  return shcore::Value();
}

void Set_option::rollback() {}

void Set_option::finish() {
  // Close all sessions to cluster instances.
  for (const auto &instance : m_cluster_instances) {
    instance->close_session();
  }

  // Reset all auxiliary (temporary) data used for the operation execution.
  m_option.clear();
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
