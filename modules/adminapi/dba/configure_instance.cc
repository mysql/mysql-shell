/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#include "modules/adminapi/dba/configure_instance.h"

#include <algorithm>
#include <string>
#include <vector>

#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/parallel_applier_options.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/mod_utils.h"
#include "my_loglevel.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

Configure_instance::Configure_instance(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
    const Configure_instance_options &options, TargetType::Type instance_type)
    : m_target_instance(target_instance),
      m_options(options),
      m_instance_type(instance_type) {}

Configure_instance::~Configure_instance() {}

/*
 * Validates the .cnf file path. If interactive is enabled and the file path
 * is not provided, it attempts to resolve it
 *
 * When this function exits, the following will be true:
 * - if return value is false, then the config file cannot be updated
 * - if return value is true, then:
 *  - m_options.mycnf_path contains an input config file
 *  - either m_options.mycnf_path can be written to or
 * m_options.output_mycnf_path will contain a path that can be used to write the
 * config file
 */
bool Configure_instance::check_config_path_for_update() {
  auto console = mysqlsh::current_console();

  if (!m_local_target) {
    // Check if the instance is not local first, if so we can throw an error
    // already
    console->print_warning(
        "Cannot update configuration file for a remote target instance.");
    return false;
  }

  // If the cnf file path is empty attempt to resolve it
  if (m_options.mycnf_path.empty()) {
    if (m_options.interactive())
      m_options.mycnf_path = prompt_cnf_path(*m_target_instance);

    // If interactive is disabled or the attempt to resolve the cnf file
    // path wasn't successful throw an error
    if (m_options.mycnf_path.empty()) {
      console->print_error(
          "The path to the MySQL configuration file is required to verify and "
          "fix InnoDB cluster related options.");
      console->print_info("Please provide its path with the mycnfPath option.");
      return false;
    }
  } else {
    if (m_is_cnf_from_sandbox)
      console->print_info("Sandbox MySQL configuration file at: " +
                          m_options.mycnf_path);
  }

  bool prompt_output_path = false;
  // If outputMycnfPath wasn't used, we must validate mycnfPath for its
  // existence and write permissions
  if (m_options.output_mycnf_path.empty()) {
    // Check if the configuration file does not exist
    if (!shcore::is_file(m_options.mycnf_path)) {
      console->print_warning(m_options.mycnf_path + " does not exist.");
      if (m_options.interactive()) {
        // Ask the user if accepts that a new file is created in the same path
        // as myCnfPath
        if (console->confirm("Do you want to create a new MySQL "
                             "configuration file at that path?",
                             Prompt_answer::YES) == Prompt_answer::NO) {
          return false;
        }
      } else {
        console->print_info(m_options.mycnf_path + " will be created.");
      }
    }
    // If path exists (or if it's OK to create it), check if it's also writable
    try {
      shcore::check_file_writable_or_throw(m_options.mycnf_path);
    } catch (const std::exception &e) {
      if (m_options.interactive()) {
        console->print_warning("mycnfPath is not writable: " +
                               m_options.mycnf_path + ": " + e.what());
        console->print_info(
            "The required configuration changes may be written to a "
            "different file, which you can copy over to its proper location.");

        prompt_output_path = true;
      } else {
        console->print_error("mycnfPath is not writable: " +
                             m_options.mycnf_path + ": " + e.what());
        console->print_info(
            "The outputMycnfPath option may be used to have "
            "the updated configuration file written to a different path.");
        return false;
      }
    }
  }

  // Now ensure that the output config file we have (if we have one) is writable
  if (!m_options.output_mycnf_path.empty() || prompt_output_path) {
    for (;;) {
      if (prompt_output_path) {
        assert(m_options.interactive());
        // Prompt the user for an alternative filepath and suggest the user to
        // sudo cp into the right place
        if (console->prompt("Output path for updated configuration file: ",
                            &m_options.output_mycnf_path) !=
            shcore::Prompt_result::Ok)
          return false;
      }

      try {
        shcore::check_file_writable_or_throw(m_options.output_mycnf_path);
        break;
      } catch (const std::exception &e) {
        // If invalid option is given, we error out
        console->print_error("outputMycnfPath is not writable: " +
                             m_options.output_mycnf_path + ": " + e.what());
        // If prompt_output_path is false, it means m_options.output_mycnf_path
        // was given by the user via option, so we fail directly
        if (!prompt_output_path) return false;
      }
    }
    // NOTE: The output_cnf_path is properly updated by the caller on the
    // config_file_handler (nothing else needed here).
  }
  return true;
}

namespace {
bool prompt_full_account(std::string *out_account) {
  bool cancelled = false;
  auto console = mysqlsh::current_console();
  for (;;) {
    std::string create_user;

    if (console->prompt("Account Name: ", &create_user) ==
            shcore::Prompt_result::Ok &&
        !create_user.empty()) {
      try {
        // normalize the account name
        if (std::count(create_user.begin(), create_user.end(), '@') <= 1 &&
            create_user.find(' ') == std::string::npos &&
            create_user.find('\'') == std::string::npos &&
            create_user.find('"') == std::string::npos &&
            create_user.find('\\') == std::string::npos) {
          auto p = create_user.find('@');
          if (p == std::string::npos) {
            create_user = shcore::make_account(create_user, "%");
          } else {
            create_user = shcore::make_account(create_user.substr(0, p),
                                               create_user.substr(p + 1));
          }
        }
        // validate
        shcore::split_account(create_user, nullptr, nullptr, false);

        if (out_account) *out_account = create_user;
        break;
      } catch (const std::runtime_error &) {
        console->print_info(
            "`" + create_user +
            "` is not a valid account name. Must be user[@host] or "
            "'user'[@'host']");
      }
    } else {
      cancelled = true;
      break;
    }
  }
  return cancelled;
}

bool prompt_account_host(std::string *out_host) {
  auto result = mysqlsh::current_console()->prompt("Account Host: ", out_host);
  return result != shcore::Prompt_result::Ok || out_host->empty();
}

/*
 * Prompts options to create a valid admin account
 *
 * @param user the username
 * @param host the hostname
 * @param cluster_type
 * @param out_create_account the resolved account to be created
 *
 * @return a boolean value indicating whether the account has enough privileges
 * (or was resolved), or not.
 */
bool prompt_create_usable_admin_account(const std::string &user,
                                        const std::string & /* host */,
                                        Cluster_type cluster_type,
                                        std::string *out_create_account) {
  assert(out_create_account);
  int result;
  result = prompt_menu(
      {"Create remotely usable account for '" + user +
           "' with same grants and password",
       "Create a new admin account for " +
           to_display_string(cluster_type, Display_form::THING_FULL) +
           " with minimal required grants",
       "Ignore and continue", "Cancel"},
      1);
  bool cancelled;
  auto console = mysqlsh::current_console();

  switch (result) {
    case 1:
      console->print_info(
          "Please provide a source address filter for the account (e.g: "
          "192.168.% or % etc) or leave empty and press Enter to cancel.");
      cancelled = prompt_account_host(out_create_account);
      if (!cancelled) {
        *out_create_account = shcore::make_account(user, *out_create_account);
      }
      break;
    case 2:
      console->print_info(
          "Please provide an account name (e.g: icroot@%) "
          "to have it created with the necessary\n"
          "privileges or leave empty and press Enter to cancel.");
      cancelled = prompt_full_account(out_create_account);
      break;
    case 3:
      *out_create_account = "";
      return true;
    case 4:
    default:
      console->print_info("Canceling...");
      return false;
  }

  if (cancelled) {
    console->print_info("Canceling...");
    return false;
  }
  return true;
}
}  // namespace

/*
 * Checks that the account in use isn't just a root@localhost account that
 * would make it unable to access any other members of the cluster. If the
 * account does not pass the check and interactive is enabled, ask the user
 * if wants to create the same account in '%' or to create a new account.
 */
void Configure_instance::check_create_admin_user() {
  auto console = mysqlsh::current_console();

  if (m_options.cluster_admin.empty()) {
    // Check that the account in use isn't too restricted (like localhost only)
    if (!check_admin_account_access_restrictions(*m_target_instance,
                                                 m_current_user, m_current_host,
                                                 m_options.interactive())) {
      // If interaction is enabled use the console_handler admin-user
      // handling function
      if (m_options.interactive()) {
        console->print_info();
        if (!prompt_create_usable_admin_account(m_current_user, m_current_host,
                                                m_options.cluster_type,
                                                &m_options.cluster_admin)) {
          throw shcore::cancelled("Cancelled");
        }
      }
    }
  }

  // If the clusterAdmin account option is used (or given interactively),
  // validate that account instead.
  if (!m_options.cluster_admin.empty()) {
    std::string admin_user, admin_user_host;
    shcore::split_account(m_options.cluster_admin, &admin_user,
                          &admin_user_host, false);
    // Check if the clusterAdmin account exists
    bool cluster_admin_user_exists =
        m_target_instance->user_exists(admin_user, admin_user_host);

    // Set the hostname if empty
    if (admin_user_host.empty()) {
      admin_user_host = "%";
      std::string full = shcore::make_account(admin_user, admin_user_host);
      console->print_info("Assuming full account name " + full + " for " +
                          m_options.cluster_admin);
      m_options.cluster_admin = full;
    }

    if (cluster_admin_user_exists) {
      if (!m_options.cluster_admin_password.is_null()) {
        throw shcore::Exception::argument_error(
            "The " + m_options.cluster_admin +
            " account already exists, clusterAdminPassword is not allowed for "
            "an existing account.");
      }
      std::string error_info;
      // cluster admin account exists, so we will validate its privileges
      // and log a warning to inform that the user won't be created

      if (!validate_cluster_admin_user_privileges(
              *m_target_instance, admin_user, admin_user_host, &error_info)) {
        console->print_warning(
            "User " + m_options.cluster_admin +
            " already exists and will not be created. However, it is missing "
            "privileges.");
        console->print_info(error_info);

        throw shcore::Exception::runtime_error(
            "The account " +
            shcore::make_account(m_current_user, m_current_host) +
            " is missing privileges required to manage an InnoDB cluster.");
      } else {
        console->print_info("User " + m_options.cluster_admin +
                            " already exists and will not be created.");
      }
      m_create_cluster_admin = false;
    } else {
      // If interaction is enabled, clusterAdmin is used but
      // clusterAdminPassword isn't provided, prompt the user for it,
      // unless the account name is the same as the current
      if (m_current_user == admin_user) {
        m_options.cluster_admin_password =
            m_target_instance->get_connection_options().get_password();
      } else if (m_options.cluster_admin_password.is_null()) {
        if (m_options.interactive()) {
          m_options.cluster_admin_password = prompt_new_account_password();
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
    shcore::split_account(m_options.cluster_admin, &admin_user,
                          &admin_user_host, false);
    try {
      assert(!m_current_user.empty());

      if (admin_user == m_current_user) {
        log_info("Cloning current user account %s@%s as %s",
                 m_current_user.c_str(), m_current_host.c_str(),
                 m_options.cluster_admin.c_str());
        mysqlshdk::mysql::Suppress_binary_log nobinlog(m_target_instance.get());
        mysqlshdk::mysql::clone_user(
            *m_target_instance, m_current_user, m_current_host, admin_user,
            admin_user_host, *m_options.cluster_admin_password);
      } else {
        log_info("Creating new cluster admin account %s",
                 m_options.cluster_admin.c_str());
        create_cluster_admin_user(*m_target_instance, m_options.cluster_admin,
                                  *m_options.cluster_admin_password);
      }
    } catch (const shcore::Exception &err) {
      std::string error_msg = "Error creating clusterAdmin account: '" +
                              m_options.cluster_admin +
                              "', with error: " + err.what();
      throw shcore::Exception::runtime_error(error_msg);
    }

    mysqlsh::current_console()->print_info(
        "Cluster admin user " + m_options.cluster_admin + " created.");
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
  auto console = mysqlsh::current_console();

  // Perform check with no update

  // If mycnfPath was used, validate if the file exists and if doesn't do not
  // use it in the check call
  std::string cnf_path = m_options.mycnf_path;
  if (!m_options.mycnf_path.empty() && !shcore::is_file(cnf_path)) {
    console->print_warning("MySQL configuration file " + cnf_path +
                           " does not exist. Skipping file verification.");
    cnf_path.clear();
  }

  m_invalid_cfgs = checks::validate_configuration(
      m_target_instance.get(), cnf_path, m_cfg.get(), m_options.cluster_type,
      m_can_set_persist, restart, config_file_change, dynamic_sysvar_change);
  if (!m_invalid_cfgs.empty()) {
    // If no option file change or dynamic variable changes are required but
    // restart variable was set to true, then it means that only read-only
    // variables were detected as invalid configs on the server. If SET PERSIST
    // is supported then those variables still need to be updated (persisted),
    // otherwise only a restart is required because the value for those
    // variables is correct in the option file.
    // NOTE: validate_configuration() requires changes to the option file for
    //       any invalid server variable, unless it was able to confirm that
    //       the value is correct in the given option file or SET PERSIST is
    //       supported.
    return !(*restart && !*config_file_change && !*dynamic_sysvar_change &&
             (m_can_set_persist.is_null() || !*m_can_set_persist));
  } else {
    console->print_info();
    console->print_info(
        "The instance '" + m_target_instance->descr() +
        "' is valid to be used in " +
        to_display_string(m_options.cluster_type, Display_form::A_THING_FULL) +
        ".");
    return false;
  }
}

void Configure_instance::ensure_instance_address_usable() {
  auto console = mysqlsh::current_console();

  // Sanity check for the instance address
  if (is_sandbox(*m_target_instance, nullptr)) {
    console->print_note("Instance detected as a sandbox.");
    console->print_info(
        "Please note that sandbox instances are only suitable for deploying "
        "test clusters for use within the same host.");
  }
  checks::validate_host_address(*m_target_instance, 2);
}

void Configure_instance::ensure_configuration_change_possible(
    bool needs_mycnf_change) {
  // (FR2/FR3) If the instance has a version >= 8.0.5, verify the status of
  // persisted_globals_load. If its value is set to 'OFF' and the instance
  // is remote, issue an error
  // This should only be checked if we actually need to change configs
  if (!m_can_set_persist.is_null() && !*m_can_set_persist) {
    auto console = mysqlsh::current_console();
    console->print_note("persisted_globals_load option is OFF");
    console->print_info(
        "Remote configuration of the instance is not possible because "
        "options changed with SET PERSIST will not be loaded, unless "
        "'persisted_globals_load' is set to ON.");
  }

  // If we need to make config changes and either some of these changes
  // can only be done in the my.cnf (e.g. log_bin) or we can't use SET PERSIST
  // then ensure that we can update my.cnf
  // NOTE: if mycnfPath was used in the cmd call, we also use it
  if (needs_mycnf_change ||
      (m_can_set_persist.is_null() || !*m_can_set_persist) ||
      !m_options.mycnf_path.empty() || !m_options.output_mycnf_path.empty()) {
    // (FR3/FR4) If the instance has a version < 8.0.11 the configuration file
    // path is mandatory in case the instance is local. If the instance is
    // remote issue an error
    if (!check_config_path_for_update()) {
      auto console = mysqlsh::current_console();

      console->print_error("Unable to change MySQL configuration.");
      console->print_info(
          "MySQL server configuration needs to be "
          "updated, but neither remote nor local configuration is possible.");
      if (m_target_instance->get_version() < mysqlshdk::utils::Version(8, 0, 5))
        console->print_info(
            "Please run this command locally, in the same host as the MySQL "
            "server being configured, and pass the path to its configuration "
            "file through the mycnfPath option.");
      throw shcore::Exception::runtime_error("Unable to update configuration");
    } else {
      // Update the configuration handler if the m_options.mycnf_path or
      // m_options.output_mycnf_path were read during the wizard on
      // check_config_path_for_update. Remove the current config handler if it
      // exists before adding it again.
      if (!m_options.mycnf_path.empty() ||
          !m_options.output_mycnf_path.empty()) {
        if (m_cfg->has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
          m_cfg->remove_handler(mysqlshdk::config::k_dft_cfg_file_handler);
        }
        mysqlsh::dba::add_config_file_handler(
            m_cfg.get(), mysqlshdk::config::k_dft_cfg_file_handler,
            m_target_instance->get_uuid(), m_options.mycnf_path,
            m_options.output_mycnf_path);
      }
    }
  }
}

void Configure_instance::validate_config_path() {
  // If the configuration file was provided but doesn't exist, then issue an
  // error (BUG#27702439).
  if (!m_options.mycnf_path.empty() && !shcore::is_file(m_options.mycnf_path)) {
    auto console = mysqlsh::current_console();
    console->print_error(
        "The specified MySQL option file does not exist. A valid path is "
        "required for the mycnfPath option.");
    console->print_info(
        "Please provide a valid path for the mycnfPath option or leave it "
        "empty if you which to skip the verification of the MySQL option "
        "file.");
    throw std::runtime_error("File '" + m_options.mycnf_path +
                             "' does not exist.");
  }
}

namespace {
void validate_applier_worker_threads_option(
    const mysqlshdk::utils::Version &version) {
  if (version < mysqlshdk::utils::Version(8, 0, 23)) {
    throw shcore::Exception::runtime_error(
        "Option 'applierWorkerThreads' not supported on target server "
        "version: '" +
        version.get_full() + "'");
  }
}
}  // namespace

void Configure_instance::validate_applier_worker_threads() {
  auto target_instance_version = m_target_instance->get_version();
  Parallel_applier_options parallel_applier_options(*m_target_instance);

  if (!m_options.replica_parallel_workers.is_null()) {
    // Validate if the target instance supports applierWorkerThreads

    validate_applier_worker_threads_option(target_instance_version);

    if (*parallel_applier_options.replica_parallel_workers !=
        m_options.replica_parallel_workers.get_safe()) {
      m_set_applier_worker_threads = true;
    }
  } else {
    // If the target instance supports applierWorkerThreads, set the default
    // value of 4
    if (target_instance_version >= mysqlshdk::utils::Version(8, 0, 23)) {
      auto console = mysqlsh::current_console();

      m_options.replica_parallel_workers = kReplicaParallelWorkersDefault;
      m_set_applier_worker_threads = true;

      console->print_info();
      console->print_info(
          "applierWorkerThreads will be set to the default value of 4.");
    }
  }
}

void Configure_instance::prepare_config_object() {
  // Add server configuration handler depending on SET PERSIST support.
  // NOTE: Add server handler first to set it as the default handler.
  m_cfg = mysqlsh::dba::create_server_config(
      m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler,
      true);

  // Add configuration handle to update option file (if provided).
  if (!m_options.mycnf_path.empty() || !m_options.output_mycnf_path.empty()) {
    mysqlsh::dba::add_config_file_handler(
        m_cfg.get(), mysqlshdk::config::k_dft_cfg_file_handler,
        m_target_instance->get_uuid(), m_options.mycnf_path,
        m_options.output_mycnf_path);
  }
}

/*
 * Validates the parameter and performs other validations regarding
 * the command execution
 */
void Configure_instance::prepare() {
  auto console = mysqlsh::current_console();

  // If the instance already belongs to an InnoDB Cluster or ReplicaSet the
  // function can only be used to set a new value for applierWorkerThreads
  if (m_instance_type == TargetType::InnoDBCluster ||
      m_instance_type == TargetType::AsyncReplicaSet) {
    std::string cluster_type =
        m_instance_type == TargetType::InnoDBCluster ? "Cluster" : "ReplicaSet";

    console->print_info("The instance '" + m_target_instance->descr() +
                        "' belongs to an InnoDB " + cluster_type + ".");

    // If the instance already belongs to a cluster/replicaset we must disallow
    // clusterAdmin

    if (!m_options.cluster_admin.empty()) {
      throw shcore::Exception::argument_error(
          "The clusterAdmin option is not allowed for instances already "
          "belonging to an InnoDB " +
          cluster_type + ".");
    }

    m_create_cluster_admin = false;
  }

  if (m_options.cluster_admin.empty() &&
      !m_options.cluster_admin_password.is_null()) {
    throw shcore::Exception::argument_error(
        "The clusterAdminPassword is allowed only if clusterAdmin is "
        "specified.");
  }

  // Check if the target instance is local
  m_local_target = !m_target_instance->get_connection_options().has_host() ||
                   mysqlshdk::utils::Net::is_local_address(
                       m_target_instance->get_connection_options().get_host());

  // Set the current user/host
  m_target_instance->get_current_user(&m_current_user, &m_current_host);

  if (!m_local_target) {
    console->print_info(
        "Configuring MySQL instance at " + m_target_instance->descr() +
        " for use in " +
        to_display_string(m_options.cluster_type, Display_form::A_THING_FULL) +
        "...");
  } else {
    console->print_info(
        "Configuring local MySQL instance listening at port " +
        std::to_string(m_target_instance->get_canonical_port()) +
        " for use in " +
        to_display_string(m_options.cluster_type, Display_form::A_THING_FULL) +
        "...");
  }

  // Check if we are dealing with a sandbox instance
  std::string cnf_path;
  m_is_sandbox = is_sandbox(*m_target_instance, &cnf_path);
  // if instance is sandbox and the mycnf path is empty, fill it.
  if (m_is_sandbox && m_options.mycnf_path.empty()) {
    m_options.mycnf_path = std::move(cnf_path);
    m_is_cnf_from_sandbox = true;
  }

  // Check capabilities based on target server and user options
  // NOTE: RESTART is only available in 8.0.4, but the command is only
  // supported in 8.0.11 or higher
  m_can_restart = false;
  if (m_target_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 11)) {
    m_can_restart = true;
  }

  // Get the capabilities to use set persist by the server.
  m_can_set_persist = m_target_instance->is_set_persist_supported();

  // Ensure the user has privs to do all these checks
  ensure_user_privileges(*m_target_instance);

  // Check lock service UDFs availability (after checking privileges).
  //
  // This has to be done after checking the user privileges otherwise if
  // the user has not enough privileges the check for the locks support will
  // fail
  //
  // Skip the check if the instance is already a member of an InnoDB Cluster or
  // ReplicaSet
  if (m_instance_type != TargetType::InnoDBCluster &&
      m_instance_type != TargetType::AsyncReplicaSet) {
    check_lock_service();
  }

  ensure_instance_address_usable();

  // Validate the admin_account privileges:
  //   - If interactive is enabled, use the interaction handler function to
  //     validate the user and prompt the user accordingly to resolve any issue
  //   - By default, the current user account is the admin_account.
  //   - If the clusterAdmin option was used, check if that account exists
  //     and if so check its privileges. Otherwise, fall-back to the current
  //     user account
  check_create_admin_user();

  // validate the mycnfpath parameter
  validate_config_path();

  // Validate applierWorkerThreads
  validate_applier_worker_threads();

  // Set the internal configuration object properly according to the given
  // command options (to read/write configuration from the server and/or option
  // file (e.g., my.cnf).
  prepare_config_object();

  // If interactive is enabled we have to ask the user if accepts
  // the changes to be done. For such purpose, we must call
  // checkInstanceConfiguration to obtain the list of issues to present
  // to the user before proceeding to the configuration itself
  // Also, in order to verify if there's any variable that cannot be remotely
  // changed we use the output from checkInstanceConfiguration
  bool needs_sysvar_change = false;
  m_needs_configuration_update = check_configuration_updates(
      &m_needs_restart, &needs_sysvar_change, &m_needs_update_mycnf);

  if (m_needs_configuration_update) {
    ensure_configuration_change_possible(m_needs_update_mycnf);
  }

  // If there is anything to be done, ask the user for confirmation
  if (m_options.interactive()) {
    if (m_needs_configuration_update) {
      // If the user does not accept the changes terminate the operation
      if (console->confirm(
              "Do you want to perform the required configuration changes?",
              Prompt_answer::NONE) == Prompt_answer::NO) {
        throw shcore::cancelled("Cancelled");
      }
    }

    if (m_can_restart && m_needs_restart && m_options.restart.is_null()) {
      if (m_needs_configuration_update) {
        if (console->confirm(
                "Do you want to restart the instance after configuring it?",
                Prompt_answer::NONE) == Prompt_answer::YES) {
          m_options.restart = mysqlshdk::utils::nullable<bool>(true);
        } else {
          m_options.restart = mysqlshdk::utils::nullable<bool>(false);
        }
      } else {
        if (console->confirm("Do you want to restart the instance now?",
                             Prompt_answer::NONE) == Prompt_answer::YES) {
          m_options.restart = mysqlshdk::utils::nullable<bool>(true);
        } else {
          m_options.restart = mysqlshdk::utils::nullable<bool>(true);
        }
      }
    }
    // Verify the need to disable super-read-only
    // due to the need to create the clusterAdmin account (and others)
    // Handle clear_read_only interaction
    // TODO(alfredo) - this should be replace with validate_super_read_only(),
    // the separation between prepare() and execute() isn't so important anymore
    if (m_options.clear_read_only.is_null() && m_options.interactive() &&
        m_create_cluster_admin) {
      console->print_info();
      if (prompt_super_read_only(*m_target_instance, true)) {
        m_options.clear_read_only = true;
      }
    }
  }
}

/*
 * Executes the API command.
 */
shcore::Value Configure_instance::execute() {
  auto console = mysqlsh::current_console();
  {
    bool need_restore = false;
    if ((m_install_lock_service_udfs || m_create_cluster_admin)) {
      need_restore = clear_super_read_only();
    }

    shcore::on_leave_scope reset_read_only([this, need_restore]() {
      if (need_restore) restore_super_read_only();
    });

    // Acquire required locks on target instance (check user privileges first).
    // No "write" operation allowed to be executed concurrently on the target
    // instance.
    m_target_instance->get_lock_exclusive();

    // Always release locks at the end, when leaving the function scope.
    auto finally =
        shcore::on_leave_scope([this]() { m_target_instance->release_lock(); });

    // Handle the clusterAdmin account creation
    if (m_create_cluster_admin) {
      create_admin_user();
    }

    // Handle applierWorkerThreads
    if (m_set_applier_worker_threads) {
      m_cfg->set(mysqlshdk::mysql::get_replication_option_keyword(
                     m_target_instance->get_version(), kReplicaParallelWorkers),
                 m_options.replica_parallel_workers);
    }
  }

  if (m_needs_configuration_update) {
    // Execute the configure instance operation
    console->print_info("Configuring instance...");
    m_needs_restart = mysqlsh::dba::configure_instance(
        m_cfg.get(), m_invalid_cfgs, m_target_instance->get_version());
    if (!m_options.output_mycnf_path.empty() &&
        m_options.output_mycnf_path != m_options.mycnf_path &&
        m_needs_configuration_update) {
      console->print_info("The instance '" + m_target_instance->descr() +
                          "' was configured to be used in " +
                          to_display_string(m_options.cluster_type,
                                            Display_form::A_THING_FULL) +
                          " but you must copy " + m_options.output_mycnf_path +
                          " to the MySQL option file path.");
    } else {
      console->print_info("The instance '" + m_target_instance->descr() +
                          "' was configured to be used in " +
                          to_display_string(m_options.cluster_type,
                                            Display_form::A_THING_FULL) +
                          ".");
    }
  } else {
    console->print_info(
        "The instance '" + m_target_instance->descr() +
        "' is already ready to be used in " +
        to_display_string(m_options.cluster_type, Display_form::A_THING_FULL) +
        ".");

    if (m_set_applier_worker_threads) {
      m_cfg->apply();

      if (m_instance_type == TargetType::InnoDBCluster ||
          m_instance_type == TargetType::AsyncReplicaSet) {
        std::string cluster_type = m_instance_type == TargetType::InnoDBCluster
                                       ? "Cluster"
                                       : "ReplicaSet";

        console->print_warning(
            "The changes on the value of " +
            mysqlshdk::mysql::get_replication_option_keyword(
                m_target_instance->get_version(), kReplicaParallelWorkers) +
            " will only take place after the instance leaves and rejoins the " +
            cluster_type + ".");

        m_cfg->apply();

        console->print_info();
        console->print_info(
            "Successfully set the value of " +
            mysqlshdk::mysql::get_replication_option_keyword(
                m_target_instance->get_version(), kReplicaParallelWorkers) +
            ".");
      } else {
        console->print_info();
        console->print_info("Successfully enabled parallel appliers.");
      }
    }
  }

  if (m_needs_restart) {
    // We can restart and user wants to restart
    if (m_can_restart && !m_options.restart.is_null() && *m_options.restart) {
      try {
        console->print_info("Restarting MySQL...");

        DBUG_EXECUTE_IF("dba_abort_instance_restart",
                        { throw shcore::Error("restart aborted (debug)", 0); });

        m_target_instance->query("RESTART");
        console->print_note("MySQL server at " + m_target_instance->descr() +
                            " was restarted.");

      } catch (const shcore::Error &err) {
        log_error("Error executing RESTART: %s", err.format().c_str());
        console->print_error("Remote restart of MySQL server failed: " +
                             err.format());
        console->print_info(
            "Please restart MySQL manually (check "
            "https://dev.mysql.com/doc/refman/en/restart.html for more "
            "details).");
        // Rethrow the exception so that the caller can know that it needs
        // a restart and the requested action (restart) failed.
        throw;
      }
    } else {
      console->print_note(
          "MySQL server needs to be restarted for configuration changes to "
          "take effect.");
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
      *m_target_instance, m_options.clear_read_only, false);
  // If super_read_only was disabled, print the information
  if (super_read_only) {
    mysqlsh::current_console()->print_info(
        "Disabled super_read_only on the instance '" +
        m_target_instance->descr() + "'");
    return true;
  }
  return false;
}

void Configure_instance::restore_super_read_only() {
  // If we disabled super_read_only we must enable it back
  // also confirm that the initial status was 1/ON
  if (m_options.clear_read_only == true) {
    mysqlsh::current_console()->print_info(
        "Enabling super_read_only on the instance '" +
        m_target_instance->descr() + "'");
    m_target_instance->set_sysvar("super_read_only", "ON",
                                  mysqlshdk::mysql::Var_qualifier::GLOBAL);
  }
}

void Configure_instance::check_lock_service() {
  m_install_lock_service_udfs =
      mysqlshdk::mysql::has_lock_service_udfs(*m_target_instance);
}

}  // namespace dba
}  // namespace mysqlsh
