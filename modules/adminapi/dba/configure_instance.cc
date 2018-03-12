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

#include "modules/adminapi/dba/configure_instance.h"
#include "modules/adminapi/dba/validations.h"
#include "modules/adminapi/instance_validations.h"
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

Configure_instance::Configure_instance(
    mysqlshdk::mysql::IInstance *target_instance, const std::string &mycnf_path,
    const std::string &output_mycnf_path, const std::string &cluster_admin,
    const mysqlshdk::utils::nullable<std::string> &cluster_admin_password,
    mysqlshdk::utils::nullable<bool> clear_read_only, const bool interactive,
    mysqlshdk::utils::nullable<bool> restart,
    std::shared_ptr<ProvisioningInterface> provisioning_interface,
    std::shared_ptr<mysqlsh::IConsole> console_handler)
    : m_mycnf_path(mycnf_path),
      m_output_mycnf_path(output_mycnf_path),
      m_cluster_admin(cluster_admin),
      m_cluster_admin_password(cluster_admin_password),
      m_interactive(interactive),
      m_clear_read_only(clear_read_only),
      m_restart(restart),
      m_target_instance(target_instance),
      m_provisioning_interface(provisioning_interface),
      m_console(console_handler) {
  assert(target_instance);
  assert(console_handler);
  assert(provisioning_interface);

  m_local_target = mysqlshdk::utils::Net::is_local_address(
      m_target_instance->get_connection_options().get_host());

  // Set the current user/host
  m_target_instance->get_current_user(&m_current_user, &m_current_host);
}

Configure_instance::~Configure_instance() {}

/*
 * Validates the .cnf file path. If interactive is enabled and the file path
 * is not provided, it attempts to resolve it
 *
 * When this function exits, the following will be true:
 * - if return value is false, then the config file cannot be updated
 * - if return value is true, then:
 *  - m_mycnf_path contains an input config file
 *  - either m_mycnf_path can be written to or m_output_mycnf_path will contain
 *    a path that can be used to write the config file
 */
bool Configure_instance::check_config_path_for_update() {
  if (!m_local_target) {
    // Check if the instance is not local first, if so we can throw an error
    // already
    m_console->print_warning(
        "Cannot update configuration file for a remote target instance.");
    return false;
  }

  // If the cnf file path is empty attempt to resolve it
  if (m_mycnf_path.empty()) {
    if (is_sandbox(*m_target_instance, &m_mycnf_path)) {
      m_console->println("Sandbox MySQL configuration file at: " +
                         m_mycnf_path);
    } else {
      if (m_interactive)
        m_mycnf_path = prompt_cnf_path(*m_target_instance, m_console);
    }

    // If interactive is disabled or the attempt to resolve the cnf file
    // path wasn't successful throw an error
    if (m_mycnf_path.empty()) {
      m_console->print_error(
          "The path to the MySQL configuration file is required to verify and "
          "fix InnoDB cluster related options.");
      m_console->println("Please provide its path with the mycnfPath option.");
      return false;
    }
  }

  bool prompt_output_path = false;
  // If outputMycnfPath wasn't used, we must validate mycnfPath for its
  // existence and write permissions
  if (m_output_mycnf_path.empty()) {
    // Check if the configuration file does not exist
    if (!shcore::file_exists(m_mycnf_path)) {
      m_console->print_warning(m_mycnf_path + " does not exist.");
      if (m_interactive) {
        // Ask the user if accepts that a new file is created in the same path
        // as myCnfPath
        if (m_console->confirm("Do you want to create a new MySQL "
                               "configuration file at that path?",
                               Prompt_answer::YES) == Prompt_answer::NO) {
          return false;
        }
      } else {
        m_console->println(m_mycnf_path + " will be created.");
      }
    }
    // If path exists (or if it's OK to create it), check if it's also writable
    try {
      shcore::check_file_writable_or_throw(m_mycnf_path);
    } catch (std::exception &e) {
      if (m_interactive) {
        m_console->print_warning("mycnfPath is not writable: " + m_mycnf_path +
                                 ": " + e.what());
        m_console->println(
            "The required configuration changes may be written to a "
            "different file, which you can copy over to its proper location.");

        prompt_output_path = true;
      } else {
        m_console->print_error("mycnfPath is not writable: " + m_mycnf_path +
                               ": " + e.what());
        m_console->println(
            "The outputMycnfPath option may be used to have "
            "the updated configuration file written to a different path.");
        return false;
      }
    }
  }

  // Now ensure that the output config file we have (if we have one) is writable
  if (!m_output_mycnf_path.empty() || prompt_output_path) {
    for (;;) {
      if (prompt_output_path) {
        assert(m_interactive);
        // Prompt the user for an alternative filepath and suggest the user to
        // sudo cp into the right place
        if (!m_console->prompt("Output path for updated configuration file: ",
                               &m_output_mycnf_path))
          return false;
      }

      try {
        shcore::check_file_writable_or_throw(m_output_mycnf_path);
        break;
      } catch (std::exception &e) {
        // If invalid option is given, we error out
        m_console->print_error("outputMycnfPath is not writable: " +
                               m_output_mycnf_path + ": " + e.what());
        // If prompt_output_path is false, it means m_output_mycnf_path was
        // given by the user via option, so we fail directly
        if (!prompt_output_path) return false;
      }
    }
  }
  return true;
}

/*
 * Validates the value of 'persisted_globals_load'.
 * NOTE: the validation is only performed for versions >= 8.0.5
 */
bool Configure_instance::check_persisted_globals_load() {
  // Check if the instance version is >= 8.0.5
  assert(m_target_instance->get_version() >=
         mysqlshdk::utils::Version(8, 0, 5));

  // Check the value of persisted_globals_load
  mysqlshdk::utils::nullable<bool> persist_global =
      m_target_instance->get_cached_global_sysvar_as_bool(
          "persisted_globals_load");

  if (!*persist_global) {
    m_console->print_note("persisted_globals_load option is OFF");
    m_console->println(
        "Remote configuration of the instance is not possible because "
        "options changed with SET PERSIST will not be loaded, unless "
        "'persisted_globals_load' is set to ON.");

    return false;
  }
  return true;
}

/*
 * Checks that the account in use isn't just a root@localhost account that
 * would make it unable to access any other members of the cluster. If the
 * account does not pass the check and interactive is enabled, ask the user
 * if wants to create the same account in '%' or to create a new account.
 */
void Configure_instance::check_create_admin_user() {
  if (m_cluster_admin.empty()) {
    m_cluster_admin_password = "";

    // Check that the account in use isn't too restricted (like localhost only)
    if (!check_admin_account_access_restrictions(
            *m_target_instance, m_current_user, m_current_host, m_console)) {
      // If interaction is enabled use the console_handler admin-user
      // handling function
      if (m_interactive) {
        m_console->println();
        if (!prompt_create_usable_admin_account(m_current_user, m_current_host,
                                                &m_cluster_admin, m_console)) {
          throw shcore::cancelled("Cancelled");
        }
      }
    }
  }

  // If the clusterAdmin account option is used (or given interactively),
  // validate that account instead.
  if (!m_cluster_admin.empty()) {
    std::string admin_user, admin_user_host;
    shcore::split_account(m_cluster_admin, &admin_user, &admin_user_host,
                          false);
    // Check if the clusterAdmin account exists
    bool cluster_admin_user_exists =
        m_target_instance->user_exists(admin_user, admin_user_host);

    // Set the hostname if empty
    if (admin_user_host.empty()) {
      admin_user_host = "%";
      std::string full = shcore::make_account(admin_user, admin_user_host);
      m_console->print_note("Assuming full account name " + full + " for " +
                            m_cluster_admin);
      m_cluster_admin = full;
    }

    if (cluster_admin_user_exists) {
      std::string error_info;
      // cluster admin account exists, so we will validate its privileges
      // and log a warning to inform that the user won't be created

      if (!validate_cluster_admin_user_privileges(
              m_target_instance->get_session(), admin_user, admin_user_host,
              &error_info)) {
        m_console->print_warning(
            "User " + m_cluster_admin +
            " already exists and will not be created. However, it is missing "
            "privileges.");
        m_console->println(error_info);

        throw shcore::Exception::runtime_error(
            "The account " +
            shcore::make_account(m_current_user, m_current_host) +
            " is missing privileges required to manage an InnoDB cluster.");
      } else {
        m_console->print_info("User " + m_cluster_admin +
                              " already exists and will not be created.");
      }
      m_create_cluster_admin = false;
    } else {
      // If interaction is enabled, clusterAdmin is used but
      // clusterAdminPassword isn't provided, prompt the user for it,
      // unless the account name is the same as the current
      if (m_current_user == admin_user) {
        m_cluster_admin_password = m_target_instance->get_session()
                                       ->get_connection_options()
                                       .get_password();
      } else if (m_cluster_admin_password.is_null()) {
        if (m_interactive) {
          m_cluster_admin_password = prompt_new_account_password(m_console);
        } else {
          throw shcore::Exception::runtime_error(
              "password for clusterAdmin required");
        }
      }
    }
  } else {
    m_create_cluster_admin = false;
  }
}

/*
 * Creates the clusterAdmin account.
 *
 * - Create the clusterAdmin account.
 */
void Configure_instance::create_admin_user() {
  if (m_create_cluster_admin) {
    std::string admin_user, admin_user_host;
    shcore::split_account(m_cluster_admin, &admin_user, &admin_user_host,
                          false);
    try {
      assert(!m_current_user.empty());

      if (admin_user == m_current_user) {
        log_info("Cloning current user account %s@%s as %s",
                 m_current_user.c_str(), m_current_host.c_str(),
                 m_cluster_admin.c_str());
        mysqlshdk::mysql::clone_user(m_target_instance->get_session(),
                                     m_current_user, m_current_host, admin_user,
                                     admin_user_host, m_cluster_admin_password);
      } else {
        log_info("Creating new cluster admin account %s",
                 m_cluster_admin.c_str());
        create_cluster_admin_user(m_target_instance->get_session(),
                                  m_cluster_admin, m_cluster_admin_password);
      }
    } catch (shcore::Exception &err) {
      std::string error_msg = "Error creating clusterAdmin account: '" +
                              m_cluster_admin + "', with error: " + err.what();
      throw shcore::Exception::runtime_error(error_msg);
    }

    m_console->print_note("Cluster admin user " + m_cluster_admin +
                          " created.");
  }
}

/*
 * Presents the required configuration changes and prompts the user for
 * confirmation (if interactive is enabled).
 *
 * Returns true if any configuration changes are required.
 */
bool Configure_instance::check_configuration_updates(
    bool *restart, bool *dynamic_sysvar_change, bool *config_file_change) {
  // Perform check with no update

  // If mycnfPath was used, validate if the file exists and if doesn't do not
  // use it in the check call
  std::string cnf_path = m_mycnf_path;
  if (!m_mycnf_path.empty() && !shcore::file_exists(cnf_path)) {
    m_console->print_warning("MySQL configuration file " + cnf_path +
                             " does not exist. Skipping file verification.");
    cnf_path.clear();
  }

  bool fatal_errors;
  if (!checks::validate_configuration(
          m_target_instance, cnf_path, m_console, m_provisioning_interface,
          restart, config_file_change, dynamic_sysvar_change, &fatal_errors)) {
    // If there are fatal errors, abort immediately
    if (fatal_errors) {
      throw shcore::Exception::runtime_error("errors found");
    }

    // FR1.1 - if any of the variables needed to be changed cannot be remotely
    // persisted, an error must be issued to indicate the user to run the
    // command locally in the target instance.
    // The only variable that cannot be persisted remotely is 'log_bin'
    // Fortunately, in 8.0 log_bin is ON by default, so this should be rare

    if (*config_file_change && m_can_set_persist && m_mycnf_path.empty()) {
      m_console->print_warning(
          "Some changes that require access to the MySQL configuration file "
          "need to be made. Please perform this operation in the same host as "
          "the target instance and use the mycnfPath option to update it.");
    }

    if (*restart && !*config_file_change && !*dynamic_sysvar_change) {
      m_console->print_note(
          "MySQL server restart is required for configuration to take effect.");
    }
    return false;
  } else {
    m_console->println();
    if (*restart) {
      m_console->print_note(
          "The instance '" + m_target_instance->descr() +
          "' needs to be restarted for configuration changes to take effect.");
    } else {
      m_console->print_note("The instance '" + m_target_instance->descr() +
                            "' is valid for InnoDB cluster usage.");
    }
    return !*restart;
  }
}

/*
 * Parses and handles the MP check operation result in order to print
 * human-readable descriptive results
 *
 * @param result The result object of the MP check operation
 */
void Configure_instance::handle_mp_op_result(
    const shcore::Dictionary_t &result) {
  // Handle the results of the configure instance operation
  if (result->get_string("status") == "ok") {
    m_console->print_info("The instance '" + m_target_instance->descr() +
                          "' was configured for use in an InnoDB cluster.");
  } else {
    auto errors = result->get_array("errors");

    if (!errors->empty()) {
      for (const shcore::Value &err : *errors) {
        m_console->print_error(err.as_string());
      }
      throw shcore::Exception::runtime_error(
          "Error updating MySQL configuration");
    }

    m_console->print_info("The instance '" + m_target_instance->descr() +
                          "' was configured for cluster usage.");

    bool restart = false;
    bool dynamic_sysvar_change = false;
    bool config_file_change = false;
    checks::check_required_actions(result, &restart, &dynamic_sysvar_change,
                                   &config_file_change);
    m_needs_restart = restart;
  }
}

void Configure_instance::ensure_instance_address_usable() {
  // Sanity check for the instance address
  if (is_sandbox(*m_target_instance, nullptr)) {
    m_console->print_info("Instance detected as a sandbox.");
    m_console->println(
        "Please note that sandbox instances are only suitable for deploying "
        "test clusters for use within the same host.");
  }
  if (!checks::validate_host_address(m_target_instance, true, m_console)) {
    throw shcore::Exception::runtime_error("Invalid address detected");
  }
}

void Configure_instance::ensure_configuration_change_possible(
    bool needs_mycnf_change) {
  // Check whether we have everything needed to make config changes
  if (m_can_set_persist) {
    // (FR2/FR3) If the instance has a version >= 8.0.5, verify the status of
    // persisted_globals_load. If its value is set to 'OFF' and the instance
    // is remote, issue an error
    // This should only be checked if we actually need to change configs
    if (!check_persisted_globals_load()) {
      // if persisted globals won't be loaded, then SET PERSIST is unusable
      // which means our only hope is editing my.cnf locally...
      m_can_set_persist = false;
    }
  }

  // If we need to make config changes and either some of these changes
  // can only be done in the my.cnf (e.g. log_bin) or we can't use SET PERSIST
  // then ensure that we can update my.cnf
  // NOTE: if mycnfPath was used in the cmd call, we also use it
  if (needs_mycnf_change || !m_can_set_persist || !m_mycnf_path.empty() ||
      !m_output_mycnf_path.empty()) {
    // (FR3/FR4) If the instance has a version < 8.0.5 the configuration file
    // path is mandatory in case the instance is local. If the instance is
    // remote issue an error
    if (!check_config_path_for_update()) {
      m_console->print_error("Unable to change MySQL configuration.");
      m_console->println(
          "MySQL server configuration needs to be "
          "updated, but neither remote nor local configuration is possible.");
      if (m_target_instance->get_version() < mysqlshdk::utils::Version(8, 0, 5))
        m_console->println(
            "Please run this command locally, in the same host as the MySQL "
            "server being configured, and pass the path to its configuration "
            "file through the mycnfPath option.");
      throw shcore::Exception::runtime_error("Unable to update configuration");
    }
    m_can_update_mycnf = true;
  }
}

/*
 * Validates the parameter and performs other validations regarding
 * the command execution
 */
void Configure_instance::prepare() {
  if (!m_local_target) {
    m_console->print_info("Configuring MySQL instance at " +
                          m_target_instance->descr() +
                          " for use in an InnoDB cluster...");
  } else {
    m_console->print_info(
        "Configuring local MySQL instance listening at port " +
        std::to_string(m_target_instance->get_session()
                           ->get_connection_options()
                           .get_port()) +
        " for use in an InnoDB cluster...");
  }

  // Check capabilities based on target server and user options
  m_can_set_persist = false;
  // NOTE: RESTART is only available in 8.0.4, but the command is only
  // support in 8.0.5 or higher
  m_can_restart = false;
  if (m_target_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 5)) {
    m_can_set_persist = true;
    m_can_restart = true;
    if (is_sandbox(*m_target_instance)) m_can_restart = false;
  }
  // this is checked again later, after prompting user
  m_can_update_mycnf = !m_mycnf_path.empty() && m_local_target;

  // Ensure the user has privs to do all these checks
  ensure_user_privileges(*m_target_instance, m_console);

  ensure_instance_address_usable();

  // Validate the admin_account privileges:
  //   - If interactive is enabled, use the interaction handler function to
  //     validate the user and prompt the user accordingly to resolve any issue
  //   - By default, the current user account is the admin_account.
  //   - If the clusterAdmin option was used, check if that account exists
  //     and if so check its privileges. Otherwise, fall-back to the current
  //     user account
  check_create_admin_user();

  // If interactive is enabled we have to ask the user if accepts
  // the changes to be done. For such purpose, we must call
  // checkInstanceConfiguration to obtain the list of issues to present
  // to the user before proceeding to the configuration itself
  // Also, in order to verify if there's any variable that cannot be remotely
  // changed we use the output from checkInstanceConfiguration
  bool needs_sysvar_change = false;
  m_needs_configuration_update = false;
  if (!check_configuration_updates(&m_needs_restart, &needs_sysvar_change,
                                   &m_needs_update_mycnf)) {
    if (needs_sysvar_change || m_needs_update_mycnf)
      m_needs_configuration_update = true;
  }

  if (m_needs_configuration_update) {
    ensure_configuration_change_possible(m_needs_update_mycnf);
  }

  // If there is anything to be done, ask the user for confirmation
  if (m_interactive) {
    if (m_needs_configuration_update) {
      // If the user does not accept the changes terminate the operation
      if (m_console->confirm(
              "Do you want to perform the required configuration changes?",
              Prompt_answer::NONE) == Prompt_answer::NO) {
        throw shcore::cancelled("Cancelled");
      }
    }

    if (m_can_restart && m_needs_restart && m_restart.is_null()) {
      if (m_needs_configuration_update) {
        if (m_console->confirm(
                "Do you want to restart the instance after configuring it?",
                Prompt_answer::NONE) == Prompt_answer::YES) {
          m_restart = true;
        } else {
          m_restart = false;
        }
      } else {
        if (m_console->confirm("Do you want to restart the instance now?",
                               Prompt_answer::NONE) == Prompt_answer::YES) {
          m_restart = true;
        } else {
          m_restart = false;
        }
      }
    }
    // Verify the need to disable super-read-only
    // due to the need to create the clusterAdmin account (and others)
    // Handle clear_read_only interaction
    if (m_clear_read_only.is_null() && m_interactive &&
        m_create_cluster_admin) {
      m_console->println();
      if (prompt_super_read_only(*m_target_instance, m_console, true))
        m_clear_read_only = true;
    }
  }
}

void Configure_instance::perform_configuration_updates(bool use_set_persist) {
  shcore::Value ret_val;

  assert(use_set_persist || m_local_target);

  // If outputMycnfPath was used in the cmd call but myCnfPath not then we
  // must set the value of myCnfPath to outputMycnfPath and clear
  // outputMycnfPath since the behavior expected from MP is the same when only
  // either the outputMycnfPath or mycnfPath are provided (create/override the
  // configuration file) but MP only implemented the case where just the
  // mycnfPath is provided.
  if (m_mycnf_path.empty() && !m_output_mycnf_path.empty()) {
    m_mycnf_path = m_output_mycnf_path;
    m_output_mycnf_path.clear();
  }

  // Call MP
  // Execute the configure operation
  ret_val = m_provisioning_interface->exec_check_ret_handler(
      m_target_instance->get_connection_options(), m_mycnf_path,
      m_output_mycnf_path, true, use_set_persist);

  log_debug("Configuration update op returned: %s", ret_val.descr().c_str());

  handle_mp_op_result(ret_val.as_map());
}

/*
 * Executes the API command.
 */
shcore::Value Configure_instance::execute() {
  // Handle the clusterAdmin account creation
  if (m_create_cluster_admin) {
    bool need_restore = clear_super_read_only();
    shcore::on_leave_scope reset_read_only([this, need_restore]() {
      if (need_restore) restore_super_read_only();
    });

    create_admin_user();
  }

  if (m_needs_configuration_update) {
    // TODO(someone): as soon as MP is fully rewritten to C++,we must use pure
    // C++ types instead of shcore types. The function return type should be
    // changed to std::map<std::string, std::string>

    // Execute the configure instance operation
    m_console->println("Configuring instance...");

    bool use_set_persist = !m_needs_update_mycnf && m_can_set_persist;

    perform_configuration_updates(use_set_persist);
  }

  if (m_needs_restart) {
    // We can restart and user wants to restart
    if (m_can_restart && !m_restart.is_null() && *m_restart) {
      try {
        m_console->println("Restarting MySQL...");
        m_target_instance->get_session()->query("RESTART");
        m_console->print_note("MySQL server at " + m_target_instance->descr() +
                              " was restarted.");
      } catch (const mysqlshdk::db::Error &err) {
        log_error("Error executing RESTART: %s", err.format().c_str());
        m_console->print_error("Remote restart of MySQL server failed: " +
                               err.format());
        m_console->println("Please restart MySQL manually");
      }
    } else {
      if (!m_output_mycnf_path.empty() && m_output_mycnf_path != m_mycnf_path &&
          m_needs_configuration_update) {
        m_console->print_note("You must now copy " + m_output_mycnf_path +
                              " to the MySQL "
                              "configuration file path and restart MySQL for "
                              "changes to take effect.");
      } else {
        m_console->print_note(
            "MySQL server needs to be restarted for configuration changes to "
            "take effect.");
      }
    }
  }
  return shcore::Value();
}

bool Configure_instance::clear_super_read_only() {
  // Check super_read_only
  // NOTE: this is left for last to avoid setting super_read_only to true
  // and right before some execution failure of the command leaving the
  // instance in an incorrect state

  // Handle clear_read_only interaction
  bool super_read_only = validate_super_read_only(
      m_target_instance->get_session(), m_clear_read_only == true, m_console);

  // If super_read_only was disabled, print the information
  if (super_read_only) {
    auto session_address = m_target_instance->get_session()->uri(
        mysqlshdk::db::uri::formats::only_transport());
    m_console->print_info("Disabled super_read_only on the instance '" +
                          session_address + "'");
    return true;
  }
  return false;
}

void Configure_instance::restore_super_read_only() {
  // If we disabled super_read_only we must enable it back
  // also confirm that the initial status was 1/ON
  if (m_clear_read_only == true) {
    auto session_address = m_target_instance->get_session()->uri(
        mysqlshdk::db::uri::formats::only_transport());
    m_console->print_info("Enabling super_read_only on the instance '" +
                          session_address + "'");
    m_target_instance->set_sysvar("super_read_only", "ON",
                                  mysqlshdk::mysql::Var_qualifier::GLOBAL);
  }
}

void Configure_instance::rollback() {}

void Configure_instance::finish() {}

}  // namespace dba
}  // namespace mysqlsh
