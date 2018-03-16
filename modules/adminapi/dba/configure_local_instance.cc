/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <algorithm>
#include <string>
#include <vector>

#include "modules/adminapi/dba/configure_local_instance.h"
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

Configure_local_instance::Configure_local_instance(
    mysqlshdk::mysql::IInstance *target_instance, const std::string &mycnf_path,
    const std::string &output_mycnf_path, const std::string &cluster_admin,
    const mysqlshdk::utils::nullable<std::string> &cluster_admin_password,
    mysqlshdk::utils::nullable<bool> clear_read_only, const bool interactive,
    mysqlshdk::utils::nullable<bool> restart,
    std::shared_ptr<ProvisioningInterface> provisioning_interface,
    std::shared_ptr<mysqlsh::IConsole> console_handler)
    : Configure_instance(target_instance, mycnf_path, output_mycnf_path,
                         cluster_admin, cluster_admin_password, clear_read_only,
                         interactive, restart, provisioning_interface,
                         console_handler) {
  assert(console_handler);
  assert(provisioning_interface);
}

Configure_local_instance::~Configure_local_instance() {}

/*
 * Validates the parameter and performs other validations regarding
 * the command execution
 */
void Configure_local_instance::prepare() {
  // if (m_target_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 5))
  // {
  //   m_console->print_warning("Deprecated function for target server version."
  //     "This function is meant for MySQL server versions 8.0.4 or older only.
  //     " "Please use configureInstance() instead.");
  //   m_console->println();
  // }

  m_instance_type = get_gr_instance_type(m_target_instance->get_session());

  // configure_local_instance() has 2 different operation modes:
  // - configuring instance before it joins a cluster
  // - persisting config options (like gr_group_seeds_list) after instance
  // joins a cluster
  //
  // The 1st use is replaced with configureInstance() while the 2nd should
  // be replaced with a specialized function.
  // This function should be deprecated.

  // Parameters validation for the case we only need to persist GR options
  if (m_instance_type == GRInstanceType::InnoDBCluster) {
    m_console->println("The instance '" +
                     m_target_instance->get_connection_options().as_uri(
                         mysqlshdk::db::uri::formats::only_transport()) +
                     "' belongs to an InnoDB cluster.");
    if (m_target_instance->get_version() >=
        mysqlshdk::utils::Version(8, 0, 5)) {
      m_console->print_info(
          "Calling this function on a cluster member is only required for "
          "MySQL versions 8.0.4 or earlier.");
      return;
    }

    // Validate if the instance is local or not
    if (!m_target_instance->get_connection_options().has_host()) {
      throw shcore::Exception::runtime_error(
          "This function requires a TCP connection, host is missing.");
    } else {
      if (!mysqlshdk::utils::Net::is_local_address(
              m_target_instance->get_connection_options().get_host())) {
        throw shcore::Exception::runtime_error(
            "This function only works with local instances");
      }
    }

    if (!check_config_path_for_update()) {
      throw shcore::Exception::runtime_error(
          "Unable to update MySQL configuration file.");
    }
  } else {
    // Do the rest of preparations same way as Configure_instance
    Configure_instance::prepare();
  }
}

/*
 * Updates the .cnf configuration file with the current in-memory InnoDD
 * Cluster settings.
 */
void Configure_local_instance::update_mycnf() {
  // Update the group_seeds if the instance already belongs to an InnoDB
  // Cluster
  auto seeds =
      get_peer_seeds(m_target_instance->get_session(),
                     m_target_instance->get_connection_options().as_uri(
                         mysqlshdk::db::uri::formats::only_transport()));

  auto peer_seeds = shcore::str_join(seeds, ",");
  set_global_variable(m_target_instance->get_session(),
                      "group_replication_group_seeds", peer_seeds);

  perform_configuration_updates(false);
}

/*
 * Executes the API command.
 */
shcore::Value Configure_local_instance::execute() {
  // TODO(someone): as soon as MP is fully rewritten to C++,we must use pure
  // C++ types instead of shcore types. The function return type should be
  // changed to std::map<std::string, std::string>
  shcore::Value ret_val;

  // Execute the configure local instance operation
  if (m_instance_type == GRInstanceType::InnoDBCluster) {
    if (m_target_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 5))
      return {};
    m_console->println("Persisting the cluster settings...");

    update_mycnf();

    m_console->println();
    m_console->println(
        "The instance cluster settings were successfully persisted.");

    return shcore::Value();
  } else {
    return Configure_instance::execute();
  }
}

void Configure_local_instance::rollback() { Configure_instance::rollback(); }

void Configure_local_instance::finish() { Configure_instance::finish(); }

}  // namespace dba
}  // namespace mysqlsh
