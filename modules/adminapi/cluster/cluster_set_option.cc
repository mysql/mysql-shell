/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/replicaset/set_option.h"
#include "mysqlshdk/include/shellcore/console.h"
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

Cluster_set_option::~Cluster_set_option() {}

void Cluster_set_option::ensure_option_valid() {
  // - Validate if the option is valid, being the accepted values:
  //   - clusterName
  // In this class only 'clusterName' must be checked.
  if (!m_value_int.is_null()) {
    throw shcore::Exception::argument_error(
        "Invalid value for 'clusterName': Argument #2 is expected to be a "
        "string.");
  } else {
    mysqlsh::dba::validate_cluster_name(*m_value_str);
  }
}

void Cluster_set_option::prepare() {
  // Check if the option is a ReplicaSet option and create the
  // Replicaset Set_option object to handle it. If the option it not a
  // ReplicaSet option then verify if it is a Cluster option and thrown an error
  // in case is not
  if (k_global_replicaset_supported_options.count(m_option) != 0) {
    std::shared_ptr<ReplicaSet> default_rs =
        m_cluster->get_default_replicaset();

    if (!m_value_str.is_null()) {
      m_replicaset_set_option =
          shcore::make_unique<Set_option>(*default_rs, m_option, *m_value_str);
    } else {
      m_replicaset_set_option =
          shcore::make_unique<Set_option>(*default_rs, m_option, *m_value_int);
    }

    // If m_replicaset_set_option is set it means that the option is a
    // ReplicaSet option thus must be validated by Replicaset_set_option
    m_replicaset_set_option->prepare();
  } else if (k_global_cluster_supported_options.count(m_option) != 0) {
    // The option is a Cluster option, validate if the option is valid
    ensure_option_valid();

    // Verify user privileges to execute operation;
    mysqlshdk::mysql::Instance cluster_session(m_cluster->get_group_session());
    ensure_user_privileges(cluster_session);
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
    std::string current_cluster_name = m_cluster->get_name();

    if (m_option == "clusterName") {
      console->print_info("Setting the value of '" + m_option + "' to '" +
                          *m_value_str + "' in the Cluster ...");
      console->println();

      m_cluster->get_metadata_storage()->set_cluster_name(current_cluster_name,
                                                          *m_value_str);
      m_cluster->set_name(*m_value_str);

      console->print_info("Successfully set the value of '" + m_option +
                          "' to '" + *m_value_str + "' in the Cluster: '" +
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
