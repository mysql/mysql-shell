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

#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/include/shellcore/console.h"
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
    mysqlshdk::utils::nullable<bool> restart)
    : Configure_instance(target_instance, mycnf_path, output_mycnf_path,
                         cluster_admin, cluster_admin_password, clear_read_only,
                         interactive, restart) {}

Configure_local_instance::~Configure_local_instance() {}

/*
 * Validates the parameter and performs other validations regarding
 * the command execution
 */
void Configure_local_instance::prepare() {
  // Check if we are dealing with a sandbox instance
  std::string cnf_path;
  m_is_sandbox = is_sandbox(*m_target_instance, &cnf_path);
  // if instance is sandbox and the mycnf path is empty, fill it.
  if (m_is_sandbox && m_mycnf_path.empty()) {
    m_mycnf_path = std::move(cnf_path);
    m_is_cnf_from_sandbox = true;
  }

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
    auto console = mysqlsh::current_console();

    console->println("The instance '" +
                     m_target_instance->get_connection_options().as_uri(
                         mysqlshdk::db::uri::formats::only_transport()) +
                     "' belongs to an InnoDB cluster.");
    if (m_target_instance->get_version() >=
        mysqlshdk::utils::Version(8, 0, 5)) {
      console->print_info(
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
    } else {
      // Set the internal configuration object properly according to the given
      // command options (to write configuration to the option file.
      prepare_config_object();
    }
  } else {
    // Do the rest of preparations same way as Configure_instance
    Configure_instance::prepare();
  }
}

/*
 * Executes the API command.
 */
shcore::Value Configure_local_instance::execute() {
  shcore::Value ret_val;

  // Execute the configure local instance operation
  if (m_instance_type == GRInstanceType::InnoDBCluster) {
    if (m_target_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 5))
      return {};
    auto console = mysqlsh::current_console();
    console->println("Persisting the cluster settings...");

    // Print warning if no group_seed value is empty (not set).
    if ((*m_cfg->get_string("group_replication_group_seeds")).empty()) {
      std::string instance_address =
          m_target_instance->get_connection_options().as_uri(
              mysqlshdk::db::uri::formats::only_transport());

      console->print_warning(
          "The 'group_replication_group_seeds' is not defined on instance '" +
          instance_address +
          "'. This option is mandatory to allow the server to automatically "
          "rejoin the cluster after reboot. Please manually update its value "
          "on "
          "the option file.");
    }

    mysqlsh::dba::persist_gr_configurations(*m_target_instance, m_cfg.get());

    mysqlsh::current_console()->print_info(
        "The instance '" + m_target_instance->descr() +
        "' was configured for use in an InnoDB cluster.");

    console->println();
    console->println(
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
