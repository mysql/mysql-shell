/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh::dba {

// Constructor receives command parameters, ref to ProvisioningInterface and
// ref to IConsole. It should also perform basic validation of
// parameters if there's any need.
Check_instance::Check_instance(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const std::string &verify_mycnf_path, bool silent,
    bool skip_check_tables_pk)
    : m_instance_cnx_opts(instance_cnx_opts),
      m_mycnf_path(verify_mycnf_path),
      m_silent(silent),
      m_skip_check_tables_pk{skip_check_tables_pk} {}

void Check_instance::check_instance_address() {
  // Sanity check for the instance address
  if (is_sandbox(*m_target_instance, nullptr)) {
    // bug#26393614
    auto console = mysqlsh::current_console();
    console->print_note("Instance detected as a sandbox.");
    console->print_info(
        "Please note that sandbox instances are only suitable for deploying "
        "test clusters for use within the same host.");
  }
  checks::validate_host_address(*m_target_instance, m_silent ? 1 : 2);
}

bool Check_instance::check_schema_compatibility() {
  auto console = mysqlsh::current_console();
  if (!m_silent) {
    console->print_info();
    console->print_info(
        "Checking whether existing tables comply with Group Replication "
        "requirements...");
  }
  if (checks::validate_schemas(*m_target_instance, m_skip_check_tables_pk)) {
    if (!m_silent) console->print_info("No incompatible tables detected");
    return true;
  }
  return false;
}

void Check_instance::check_clone_plugin_status() {
  if (!supports_mysql_clone(m_target_instance->get_version())) return;

  log_debug("Checking if instance '%s' has the clone plugin installed",
            m_target_instance->descr().c_str());

  std::optional<std::string> plugin_state =
      m_target_instance->get_plugin_status(
          mysqlshdk::mysql::k_mysql_clone_plugin_name);

  // Check if the plugin_state is "DISABLED"
  if (plugin_state.has_value() && (plugin_state == "DISABLED")) {
    mysqlsh::current_console()->print_warning(
        "The instance '" + m_target_instance->descr() +
        "' has the Clone plugin disabled. For optimal InnoDB cluster usage, "
        "consider enabling it.");
  }
}

bool Check_instance::check_configuration() {
  bool bad_schema = check_schema_compatibility();

  auto console = mysqlsh::current_console();
  if (!m_silent) {
    console->print_info();
    console->print_info("Checking instance configuration...");

    bool local_target = mysqlshdk::utils::Net::is_local_address(
        m_target_instance->get_connection_options().get_host());
    if (!m_mycnf_path.empty()) {
      if (!local_target) {
        throw shcore::Exception::argument_error(
            "mycnfPath or verifyMyCnf not allowed for remote instances");
      } else {
        console->print_info("Configuration file " + m_mycnf_path +
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
           m_target_instance.get(), m_mycnf_path, m_cfg.get(),
           Cluster_type::GROUP_REPLICATION, m_can_set_persist, &restart,
           &config_file_change, &dynamic_sysvar_change, &m_ret_val)
           .empty()) {
    if (config_file_change || dynamic_sysvar_change) {
      console->print_note(
          "Please use the dba.<<<configureInstance>>>() command to repair "
          "these issues.");
    } else {
      console->print_note("Please restart the MySQL server and try again.");
    }
    return false;
  }

  if (!bad_schema) {
    // if we're here it means validate_configuration found no errors and the
    // only problems are related to schema, we need to update the status and
    // still return false as we can't continue anyway.
    auto m = m_ret_val.as_map();
    (*m)["status"] = shcore::Value("error");
    return false;
  }

  if (!m_silent) {
    console->print_info(
        "Instance configuration is compatible with InnoDB cluster");
  }

  return true;
}

void Check_instance::prepare_config_object() {
  bool use_cfg_handler = false;
  // if the configuration file was provided and exists, we add it to the
  // config object.
  if (!m_mycnf_path.empty()) {
    if (shcore::is_file(m_mycnf_path)) {
      use_cfg_handler = true;
    } else {
      mysqlsh::current_console()->print_error(
          "Configuration file " + m_mycnf_path +
          " doesn't exist. The verification of the file will be skipped.");

      // Clear it up so we don't print any odd log message
      m_mycnf_path = "";
    }
  }
  // Add server configuration handler depending on SET PERSIST support.
  // NOTE: Add server handler first to set it has the default handler.
  m_cfg = mysqlsh::dba::create_server_config(
      m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler,
      true);

  // Add configuration handle to update option file (if provided) and not to
  // be skipped
  if (use_cfg_handler) {
    mysqlsh::dba::add_config_file_handler(
        m_cfg.get(), mysqlshdk::config::k_dft_cfg_file_handler,
        m_target_instance->get_uuid(), m_mycnf_path, m_mycnf_path);
  }
}

shcore::Value Check_instance::do_run() {
  auto console = mysqlsh::current_console();

  // Establish a session to the target instance
  m_target_instance = Instance::connect(m_instance_cnx_opts);

  std::string target = m_target_instance->descr();

  if (!m_silent) {
    bool local_target =
        !m_target_instance->get_connection_options().has_port() ||
        mysqlshdk::utils::Net::is_local_address(
            m_target_instance->get_connection_options().get_host());
    if (!local_target) {
      console->print_info("Validating MySQL instance at " +
                          m_target_instance->descr() +
                          " for use in an InnoDB Cluster...");
    } else {
      console->print_info(
          "Validating local MySQL instance listening at port " +
          std::to_string(m_target_instance->get_canonical_port()) +
          " for use in an InnoDB Cluster...");
    }
  }

  // Get the capabilities to use set persist by the server.
  m_can_set_persist = m_target_instance->is_set_persist_supported();

  // Set the internal configuration object properly according to the given
  // command options (to read configuration from the server and/or option
  // file (e.g., my.cnf).
  prepare_config_object();

  m_is_valid = true;

  try {
    ensure_user_privileges(*m_target_instance);

    // Check that the account in use isn't too restricted (like localhost only)
    std::string current_user, current_host;
    m_target_instance->get_current_user(&current_user, &current_host);
    if (!check_admin_account_access_restrictions(
            *m_target_instance, current_user, current_host, false,
            Cluster_type::GROUP_REPLICATION)) {
      m_is_valid = false;
    }

    check_instance_address();

    // Check if async replication is running
    //
    // NOTE: This check shall be only executed when the flag silent is
    // disabled. Otherwise, if async replication is running,
    // addInstance/rejoinInstance will print a WARNING and will also error out.
    // This happens because addInstance/rejoinInstance do a call
    // to ensure_instance_configuration_valid() which on the background creates
    // a Check_instance object with the silent flag enabled and executes it.
    if (!m_silent) {
      validate_async_channels(*m_target_instance, {},
                              checks::Check_type::CHECK);
    }

    // Check if the target instance has the clone plugin installed
    // NOTE: only if the instance is >= 8.0.17
    check_clone_plugin_status();

    if (!check_configuration()) m_is_valid = false;
  } catch (...) {
    m_is_valid = false;
    throw;
  }
  if (m_is_valid && !m_silent) {
    console->print_info();
    console->print_info("The instance '" + target +
                        "' is valid for InnoDB Cluster usage.");
  }

  mysqlsh::current_console()->print_info();

  return m_ret_val;
}
}  // namespace mysqlsh::dba
