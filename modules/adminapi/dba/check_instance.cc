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

#include <memory>

#include "modules/adminapi/dba/check_instance.h"
#include "modules/adminapi/dba/validations.h"
#include "modules/adminapi/instance_validations.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

// Constructor receives command parameters, ref to ProvisioningInterface and
// ref to IConsole. It should also perform basic validation of
// parameters if there's any need.
Check_instance::Check_instance(
    mysqlshdk::mysql::IInstance *target_instance,
    const std::string &verify_mycnf_path,
    std::shared_ptr<ProvisioningInterface> provisioning_interface,
    std::shared_ptr<mysqlsh::IConsole> console_handler, bool silent)
    : m_target_instance(target_instance),
      m_provisioning_interface(provisioning_interface),
      m_console(console_handler),
      m_mycnf_path(verify_mycnf_path),
      m_silent(silent) {
  assert(console_handler);
  assert(provisioning_interface);
}

Check_instance::~Check_instance() {}

bool Check_instance::check_instance_address() {
  // Sanity check for the instance address
  if (is_sandbox(*m_target_instance, nullptr)) {
    // bug#26393614
    m_console->print_note("Instance detected as a sandbox.");
    m_console->println(
        "Please note that sandbox instances are only suitable for deploying "
        "test clusters for use within the same host.");
  }
  return checks::validate_host_address(m_target_instance, !m_silent, m_console);
}

bool Check_instance::check_schema_compatibility() {
  if (!m_silent) {
    m_console->println();
    m_console->println(
        "Checking whether existing tables comply with Group Replication "
        "requirements...");
  }
  if (checks::validate_schemas(m_target_instance->get_session(), m_console)) {
    if (!m_silent) m_console->print_info("No incompatible tables detected");
    return true;
  }
  return false;
}

bool Check_instance::check_configuration() {
  if (!m_silent) {
    m_console->println();
    m_console->println("Checking instance configuration...");

    bool local_target = mysqlshdk::utils::Net::is_local_address(
        m_target_instance->get_connection_options().get_host());
    if (m_mycnf_path.empty()) {
      if (m_target_instance->get_version() <
          mysqlshdk::utils::Version(8, 0, 5)) {
        m_console->print_note(
            "Note: verifyMyCnf option was not given so only dynamic "
            "configuration "
            "will be verified.");
      }
    } else {
      if (!local_target) {
        throw shcore::Exception::argument_error(
            "mycnfPath or verifyMyCnf not allowed for remote instances");
      } else {
        m_console->println("Configuration file " + m_mycnf_path +
                           " will also be checked.");
      }
    }
  } else {
    log_debug("Checking instance configuration...");
  }
  // Perform check with no update
  bool fatal_errors;
  bool restart;
  bool config_file_change;
  bool dynamic_sysvar_change;

  if (!checks::validate_configuration(
          m_target_instance, m_mycnf_path, m_console, m_provisioning_interface,
          &restart, &config_file_change, &dynamic_sysvar_change, &fatal_errors,
          &m_ret_val)) {
    // If there are fatal errors, abort immediately
    if (fatal_errors) {
      m_console->print_note("Please fix issues and try again.");
      return false;
    }

    if (config_file_change || dynamic_sysvar_change) {
      m_console->print_note(
          "Please use the dba.configureInstance() command to repair these "
          "issues.");
    } else {
      m_console->print_note("Please restart the MySQL server and try again.");
    }
    return false;
  }

  if (!m_silent)
    m_console->print_note(
        "Instance configuration is compatible with InnoDB cluster");
  return true;
}

/*
 * Validates the parameter and performs other validations regarding
 * the command execution
 */
void Check_instance::prepare() {
  std::string target = m_target_instance->descr();

  if (!m_silent) {
    bool local_target = mysqlshdk::utils::Net::is_local_address(
        m_target_instance->get_connection_options().get_host());
    if (!local_target) {
      m_console->print_info("Validating MySQL instance at " +
                            m_target_instance->descr() +
                            " for use in an InnoDB cluster...");
    } else {
      m_console->print_info(
          "Validating local MySQL instance listening at port " +
          std::to_string(m_target_instance->get_session()
                             ->get_connection_options()
                             .get_port()) +
          " for use in an InnoDB cluster...");
    }
  }

  m_is_valid = true;
  bool bad_schema = false;

  try {
    ensure_user_privileges(*m_target_instance, m_console);

    if (!check_instance_address()) m_is_valid = false;

    if (!check_schema_compatibility()) {
      bad_schema = true;
    }

    if (!check_configuration()) m_is_valid = false;
  } catch (...) {
    m_is_valid = false;
    throw;
  }

  if (m_is_valid && !m_silent) {
    m_console->println();
    m_console->print_note("The instance '" + target +
                          "' is valid for InnoDB cluster usage.");
    if (bad_schema) {
      m_console->print_warning(
          "Some non-fatal issues were detected in some of the existing "
          "tables.");
      m_console->println(
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
  m_console->println();
  return m_ret_val;
}

void Check_instance::rollback() {
  // nothing to rollback
}

void Check_instance::finish() {
  // nothing to finish
}

}  // namespace dba
}  // namespace mysqlsh
