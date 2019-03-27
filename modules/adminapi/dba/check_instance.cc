/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/dba/check_instance.h"

#include <memory>

#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

// Constructor receives command parameters, ref to ProvisioningInterface and
// ref to IConsole. It should also perform basic validation of
// parameters if there's any need.
Check_instance::Check_instance(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const std::string &verify_mycnf_path,
    std::shared_ptr<ProvisioningInterface> provisioning_interface, bool silent)
    : m_instance_cnx_opts(instance_cnx_opts),
      m_mycnf_path(verify_mycnf_path),
      m_provisioning_interface(provisioning_interface),
      m_silent(silent) {
  assert(provisioning_interface);
}

Check_instance::~Check_instance() { delete m_target_instance; }

void Check_instance::check_instance_address() {
  // Sanity check for the instance address
  if (is_sandbox(*m_target_instance, nullptr)) {
    // bug#26393614
    auto console = mysqlsh::current_console();
    console->print_note("Instance detected as a sandbox.");
    console->println(
        "Please note that sandbox instances are only suitable for deploying "
        "test clusters for use within the same host.");
  }
  checks::validate_host_address(m_target_instance, !m_silent);
}

bool Check_instance::check_schema_compatibility() {
  auto console = mysqlsh::current_console();
  if (!m_silent) {
    console->println();
    console->println(
        "Checking whether existing tables comply with Group Replication "
        "requirements...");
  }
  if (checks::validate_schemas(m_target_instance->get_session())) {
    if (!m_silent) console->print_info("No incompatible tables detected");
    return true;
  }
  return false;
}

void Check_instance::check_running_async_repl() {
  auto console = mysqlsh::current_console();

  log_debug(
      "Checking if instance '%s' is running asynchronous (master-slave) "
      "replication.",
      m_target_instance->descr().c_str());

  if (mysqlshdk::mysql::is_async_replication_running(*m_target_instance)) {
    console->print_warning(
        "The instance '" + m_target_instance->descr() +
        "' cannot be added to an InnoDB cluster because it has the "
        "asynchronous (master-slave) replication configured and running. "
        "To add to it a cluster please stop the slave threads by executing "
        "the query: 'STOP SLAVE;'");
  }
}

bool Check_instance::check_configuration() {
  auto console = mysqlsh::current_console();
  if (!m_silent) {
    console->println();
    console->println("Checking instance configuration...");

    bool local_target = mysqlshdk::utils::Net::is_local_address(
        m_target_instance->get_connection_options().get_host());
    if (!m_mycnf_path.empty()) {
      if (!local_target) {
        throw shcore::Exception::argument_error(
            "mycnfPath or verifyMyCnf not allowed for remote instances");
      } else {
        console->println("Configuration file " + m_mycnf_path +
                         " will also be checked.");
      }
    }
  } else {
    log_debug("Checking instance configuration...");
  }
  // Perform check with no update
  bool restart;
  bool config_file_change;
  bool dynamic_sysvar_change;
  if (!checks::validate_configuration(
           m_target_instance, m_mycnf_path, m_cfg.get(), m_can_set_persist,
           &restart, &config_file_change, &dynamic_sysvar_change, &m_ret_val)
           .empty()) {
    if (config_file_change || dynamic_sysvar_change) {
      console->print_note(
          "Please use the dba.configureInstance() command to repair these "
          "issues.");
    } else {
      console->print_note("Please restart the MySQL server and try again.");
    }
    return false;
  }

  if (!m_silent)
    console->print_info(
        "Instance configuration is compatible with InnoDB cluster");
  return true;
}

/*
 * Validates the parameter and performs other validations regarding
 * the command execution
 */
void Check_instance::prepare() {
  auto console = mysqlsh::current_console();

  // Establish a session to the target instance
  {
    std::shared_ptr<mysqlshdk::db::ISession> session;
    session = mysqlshdk::db::mysql::Session::create();
    session->connect(m_instance_cnx_opts);
    m_target_instance = new mysqlshdk::mysql::Instance(session);
    m_target_instance->cache_global_sysvars();
  }

  std::string target = m_target_instance->descr();

  if (!m_silent) {
    bool local_target = mysqlshdk::utils::Net::is_local_address(
        m_target_instance->get_connection_options().get_host());
    if (!local_target) {
      console->print_info("Validating MySQL instance at " +
                          m_target_instance->descr() +
                          " for use in an InnoDB cluster...");
    } else {
      console->print_info("Validating local MySQL instance listening at port " +
                          std::to_string(m_target_instance->get_session()
                                             ->get_connection_options()
                                             .get_port()) +
                          " for use in an InnoDB cluster...");
    }
  }

  // Get the capabilities to use set persist by the server.
  m_can_set_persist = m_target_instance->is_set_persist_supported();

  // Set the internal configuration object properly according to the given
  // command options (to read configuration from the server and/or option
  // file (e.g., my.cnf).
  prepare_config_object();

  m_is_valid = true;
  bool bad_schema = false;

  try {
    ensure_user_privileges(*m_target_instance);

    // Check that the account in use isn't too restricted (like localhost only)
    std::string current_user, current_host;
    m_target_instance->get_current_user(&current_user, &current_host);
    if (!check_admin_account_access_restrictions(
            *m_target_instance, current_user, current_host, false)) {
      m_is_valid = false;
    }

    check_instance_address();

    if (!check_schema_compatibility()) {
      bad_schema = true;
    }

    // Check if async replication is running
    //
    // NOTE: This check shall be only executed when the flag silent is
    // disabled. Otherwise, if async replication is running,
    // addInstance/rejoinInstance will print a WARNING and will also error out.
    // This happens because addInstance/rejoinInstance do a call
    // to ensure_instance_configuration_valid() which on the background creates
    // a Check_instance object with the silent flag enabled and executes it.
    if (!m_silent) {
      check_running_async_repl();
    }

    if (!check_configuration()) m_is_valid = false;
  } catch (...) {
    m_is_valid = false;
    throw;
  }

  if (m_is_valid && !m_silent) {
    console->println();
    console->print_info("The instance '" + target +
                        "' is valid for InnoDB cluster usage.");
    if (bad_schema) {
      console->print_warning(
          "Some non-fatal issues were detected in some of the existing "
          "tables.");
      console->println(
          "You may choose to ignore these issues, although replicated updates "
          "on these tables will not be possible.");
    }
  }
}

/*
 * Executes the API command.
 *
 * This is a purely check command, no changes are made, thus execute()
 * is a no-op.
 */
shcore::Value Check_instance::execute() {
  mysqlsh::current_console()->println();
  return m_ret_val;
}

void Check_instance::rollback() {
  // nothing to rollback
}

void Check_instance::finish() {
  // Close the instance session at the end if available.
  if (m_target_instance) {
    m_target_instance->close_session();
  }
}

void Check_instance::prepare_config_object() {
  m_cfg = shcore::make_unique<mysqlshdk::config::Config>();
  bool use_cfg_handler = false;
  // if the configuration file was provided and exists, we add it to the
  // config object.
  if (!m_mycnf_path.empty() && shcore::is_file(m_mycnf_path)) {
    use_cfg_handler = true;
  }
  // Add server configuration handler depending on SET PERSIST support.
  // NOTE: Add server handler first to set it has the default handler.
  m_cfg->add_handler(
      mysqlshdk::config::k_dft_cfg_server_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          new mysqlshdk::config::Config_server_handler(
              m_target_instance,
              (!m_can_set_persist.is_null() && *m_can_set_persist)
                  ? mysqlshdk::mysql::Var_qualifier::PERSIST
                  : mysqlshdk::mysql::Var_qualifier::GLOBAL)));

  // Add configuration handle to update option file (if provided) and not to
  // be skipped
  if (use_cfg_handler) {
    m_cfg->add_handler(mysqlshdk::config::k_dft_cfg_file_handler,
                       std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                           new mysqlshdk::config::Config_file_handler(
                               m_mycnf_path, m_mycnf_path)));
  }
}

}  // namespace dba
}  // namespace mysqlsh
