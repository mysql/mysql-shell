/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/instance_validations.h"
#include <memory>
#include <string>
#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/textui/textui.h"

// Naming convention for validations:
//
// - check_ for generic check functions, which should not print anything and
// instead should return the result of its check so that the caller can take
// action on it.
//
// - validate_ for validation functions that print messages for issues, but
// not contextual messages. They should return the general result of the
// validation (e.g. pass/fail) for the caller to decide what to do.
//
// - ensure_ performs a validation, prints out errors, contextual info
// and throws an exception on validation error. They should usually be wrappers
// around a validate_ or check_ functions.
//
// checks and validations are mostly generic code, ensure is AdminAPI specific
// and thus should be in mod_dba_common.cc (or whatever it's split into)

namespace mysqlsh {
namespace dba {
namespace checks {

/**
 * Perform validation of schemas for compatibility issues with group
 * replication. If any issues are found, they're printed to the console.
 *
 * Checks performed are:
 * - GR compatible storage engines only (InnoDB and MEMORY)
 * - all tables must have a PK or a UNIQUE NOT NULL key
 *
 * @param  session session for the schema. Must be authenticated with an account
 *          with SELECT access to all schemas.
 * @param  console console object to send output to
 * @return         true if no issues found.
 */
bool validate_schemas(std::shared_ptr<mysqlshdk::db::ISession> session,
                      std::shared_ptr<IConsole> console) {
  bool ok = true;
  std::string k_gr_compliance_skip_schemas =
      "('mysql', 'sys', 'performance_schema', 'information_schema')";
  std::string k_gr_compliance_skip_engines = "('InnoDB', 'MEMORY')";

  {
    std::shared_ptr<mysqlshdk::db::IResult> result = session->query(
        "SELECT table_schema, table_name, engine "
        " FROM information_schema.tables "
        " WHERE engine NOT IN " +
        k_gr_compliance_skip_engines + " AND table_schema NOT IN " +
        k_gr_compliance_skip_schemas);

    auto row = result->fetch_one();
    if (row) {
      console->print_warning(
          "The following tables use a storage engine that are not supported by "
          "Group Replication:");
      ok = false;
      std::string tables;
      while (row) {
        if (!tables.empty()) tables.append(", ");
        tables.append(row->get_string(0))
            .append(".")
            .append(row->get_string(1));
        row = result->fetch_one();
      }
      tables.append("\n");
      console->println(tables);
    }
  }
  {
    std::shared_ptr<mysqlshdk::db::IResult> result = session->query(
        "SELECT t.table_schema, t.table_name "
        "FROM information_schema.tables t "
        "    LEFT JOIN (SELECT table_schema, table_name "
        "               FROM information_schema.statistics "
        "               GROUP BY table_schema, table_name, index_name "
        "               HAVING SUM(CASE "
        "                   WHEN non_unique = 0 AND nullable != 'YES' "
        "                   THEN 1 ELSE 0 END) = COUNT(*) "
        "              ) puks "
        "    ON t.table_schema = puks.table_schema "
        "        AND t.table_name = puks.table_name "
        "WHERE puks.table_name IS NULL "
        "    AND t.table_type = 'BASE TABLE' "
        "    AND t.table_schema NOT IN " +
        k_gr_compliance_skip_schemas);

    auto row = result->fetch_one();
    if (row) {
      console->print_warning(
          "The following tables do not have a Primary Key or equivalent "
          "column: ");
      ok = false;
      std::string tables;
      while (row) {
        if (!tables.empty()) tables.append(", ");
        tables.append(row->get_string(0))
            .append(".")
            .append(row->get_string(1));
        row = result->fetch_one();
      }
      tables.append("\n");
      console->println(tables);
    }
  }
  if (!ok) {
    console->print_info(
        "Group Replication requires tables to use InnoDB and "
        "have a PRIMARY KEY or PRIMARY KEY Equivalent (non-null "
        "unique key). Tables that do not follow these "
        "requirements will be readable but not updateable "
        "when used with Group Replication. "
        "If your applications make updates (INSERT, UPDATE or "
        "DELETE) to these tables, ensure they use the InnoDB "
        "storage engine and have a PRIMARY KEY or PRIMARY KEY "
        "Equivalent.");
  }
  return ok;
}

/**
 * Validate if a supported InnoDB page size value is used.
 *
 * Currently, innodb_page_size > 4k is required due to the size of some
 * required information in the Metadata.
 *
 * @param instance target instance to check.
 * @param console console to send output to.
 */
void validate_innodb_page_size(mysqlshdk::mysql::IInstance *instance,
                               std::shared_ptr<IConsole> console) {
  log_info("Validating InnoDB page size of instance '%s'.",
           instance->descr().c_str());
  std::string page_size = instance->get_sysvar_string(
      "innodb_page_size", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  int page_size_value = std::stoul(page_size);
  if (page_size_value <= 4096) {
    console->print_error(
        "Instance '" + instance->descr() + "' is using a non-supported InnoDB "
        "page size (innodb_page_size=" + page_size + "). Only instances with "
        "innodb_page_size greater than 4k (4096) can be used with InnoDB "
        "Cluster.");
    throw shcore::Exception::runtime_error(
        "Unsupported innodb_page_size value: " + page_size);
  }
}

/**
 * Validate that the target instances host is correctly configured in MySQL.
 * The check will be based on the @@hostname and @@report_host sysvars,
 * where @@report_host takes precedence if it's not NULL.
 *
 * The host address will be printed to te console along with an informational
 * message.
 *
 * If the target host name resolves to a loopback address (127.*) and error
 * will be printed and the function returns false. Unless the target is a
 * sandbox, detected by checking that the name of the datadir is sandboxdata.
 *
 * @param  instance target instance with cached global sysvars
 * @param  verbose  if true, additional informational messages are shown
 * @param  console  console to send output to
 * @return          true if the host name configuration is suitable
 */
bool validate_host_address(mysqlshdk::mysql::IInstance *instance,
                           bool verbose,
                           std::shared_ptr<IConsole> console) {
  // Sanity check for the instance address
  bool report_host_set = false;
  std::string address = instance->get_cached_global_sysvar("report_host");
  if (!address.empty()) {
    log_debug("Target has report_host=%s", address.c_str());
    report_host_set = true;
  } else {
    log_debug("Target has report_host=NULL");
  }
  if (address.empty()) {
    address = *instance->get_cached_global_sysvar("hostname");
  }
  log_debug("Target has hostname=%s",
            instance->get_cached_global_sysvar("hostname")->c_str());

  console->println();
  console->print_info("This instance reports its own address as " +
                      mysqlshdk::textui::bold(address));
  if (!report_host_set && verbose) {
    console->println(
        "Clients and other cluster members will communicate with it through "
        "this address by default. "
        "If this is not correct, the report_host MySQL "
        "system variable should be changed.");
  }

  if (!is_sandbox(*instance, nullptr)) {
    if (mysqlshdk::utils::Net::is_loopback(address)) {
      console->print_error(address + " resolves to a loopback address.");
      console->println(
          "Because that address will be used by other cluster members to "
          "connect to it, it must resolve to an externally reachable address.");
      console->println(
          "This address was determined through the report_host or hostname "
          "MySQL system variable.");
      return false;
    }
  }

  return true;
}

// Internal function. To be rewritten together with mysqlprovision
void check_required_actions(const shcore::Dictionary_t &result, bool *restart,
                            bool *dynamic_sysvar_change,
                            bool *config_file_change) {
  // If there are only configuration errors, a simple restart may solve them
  *restart = false;
  *dynamic_sysvar_change = false;
  *config_file_change = false;

  if (result->has_key("config_errors")) {
    auto config_errors = result->get_array("config_errors");

    // We gotta review item by item to determine if a simple restart is
    // enough
    for (auto config_error : *config_errors) {
      auto error_map = config_error.as_map();
      ConfigureInstanceAction action =
          get_configure_instance_action(*error_map);

      if (action == ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_STATIC ||
          action == ConfigureInstanceAction::UPDATE_SERVER_STATIC) {
        *restart = true;
      }

      if (action == ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_DYNAMIC ||
          action == ConfigureInstanceAction::UPDATE_SERVER_DYNAMIC ||
          action == ConfigureInstanceAction::UPDATE_SERVER_STATIC) {
        *dynamic_sysvar_change = true;
      }

      if (action == ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_DYNAMIC ||
          action == ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_STATIC ||
          action == ConfigureInstanceAction::UPDATE_CONFIG) {
        *config_file_change = true;
      }
    }
  }
}

/**
 * Validate configuration of the target MySQL server instance.
 *
 * Configuration settings are checked through sysvars, but if mycnf_path is
 * given the config file will also be checked. If any changes are required,
 * they will be sent to the console. Also returns flags indicating the types
 * of changes that are required, if any.
 *
 * @param  instance             target instance
 * @param  mycnf_path           optional config file path, for local instances
 * @param  console              output for users sent here
 * @param  mp                   mp object reference
 * @param  restart_needed[out]  true if instance needs to be restarted
 * @param  mycnf_change_needed[out]  true if my.cnf has to be updated
 * @param  sysvar_change_needed[out] true if sysvars have to be updated
 * @param  fatal_errors[out]    true on errors
 * @param  ret_val[out]         assigned to check results
 * @return                      [description]
 */
bool validate_configuration(mysqlshdk::mysql::IInstance *instance,
                            const std::string &mycnf_path,
                            std::shared_ptr<IConsole> console,
                            std::shared_ptr<ProvisioningInterface> mp,
                            bool *restart_needed, bool *mycnf_change_needed,
                            bool *sysvar_change_needed, bool *fatal_errors,
                            shcore::Value *ret_val) {
  // Check supported innodb_page_size (must be > 4k). See: BUG#27329079
  validate_innodb_page_size(instance, console);

  log_info("Validating configuration of %s (mycnf = %s)",
           instance->descr().c_str(), mycnf_path.c_str());
  // Perform check with no update
  shcore::Value check_result;
  check_result = mp->exec_check_ret_handler(
      instance->get_connection_options(), mycnf_path, "", false, false);

  if (ret_val)
    *ret_val = check_result;

  log_debug("Check command returned: %s", check_result.descr().c_str());

  *restart_needed = false;
  *mycnf_change_needed = false;
  *sysvar_change_needed = false;
  *fatal_errors = false;

  if (check_result.as_map()->get_string("status") != "ok") {
    check_required_actions(check_result.as_map(), restart_needed,
                           sysvar_change_needed, mycnf_change_needed);
    auto config_errors = check_result.as_map()->get_array("config_errors");

    console->println();
    print_validation_results(check_result.as_map(), console, true);

    // If there are fatal errors, abort immediately
    if (!check_result.as_map()->get_array("errors")->empty()) {
      *fatal_errors = true;
      return false;
    }

    if (config_errors && !config_errors->empty()) {
      for (auto config_error : *config_errors) {
        auto error_map = config_error.as_map();

        std::string option = error_map->get_string("option");
        // The my_cnf file path must be provided and the instance be local
        if (option == "log_bin") {
          console->print_note(
              "The following variable needs to be changed, but cannot be "
              "done dynamically: 'log_bin'");
          *mycnf_change_needed = true;
          break;
        }
      }
    }

    return false;
  } else {
    return true;
  }
}

}  // namespace checks
}  // namespace dba
}  // namespace mysqlsh
