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

#include "modules/adminapi/common/instance_validations.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_net.h"

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
 * @return         true if no issues found.
 */
bool validate_schemas(std::shared_ptr<mysqlshdk::db::ISession> session) {
  bool ok = true;
  std::string k_gr_compliance_skip_schemas =
      "('mysql', 'sys', 'performance_schema', 'information_schema')";
  std::string k_gr_compliance_skip_engines = "('InnoDB', 'MEMORY')";

  auto console = mysqlsh::current_console();

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
 */
void validate_innodb_page_size(mysqlshdk::mysql::IInstance *instance) {
  log_info("Validating InnoDB page size of instance '%s'.",
           instance->descr().c_str());
  std::string page_size = *instance->get_sysvar_string(
      "innodb_page_size", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  auto console = mysqlsh::current_console();

  int page_size_value = std::stoul(page_size);
  if (page_size_value <= 4096) {
    console->print_error(
        "Instance '" + instance->descr() +
        "' is using a non-supported InnoDB "
        "page size (innodb_page_size=" +
        page_size +
        "). Only instances with "
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
 * @throw  an exception if the host name configuration is not suitable
 */
void validate_host_address(mysqlshdk::mysql::IInstance *instance,
                           bool verbose) {
  auto console = mysqlsh::current_console();

  bool report_host_set;
  std::string address;
  try {
    address = mysqlshdk::mysql::get_report_host(*instance, &report_host_set);
    if (report_host_set) {
      log_debug("Target has report_host=%s", address.c_str());
    } else {
      log_debug("Target has report_host=NULL");
      log_debug("Target has hostname=%s", address.c_str());
    }
  } catch (const std::exception &err) {
    console->print_error("Invalid 'report_host' value for instance '" +
                         instance->descr() +
                         "'. The value cannot be empty if defined.");
    throw;
  }

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

  // Validate the IP of the hostname used for GR.
  // IP address '127.0.1.1' is not supported by GCS leading to errors.
  // NOTE: This IP is set by default in Debian platforms.
  std::string seed_ip = mysqlshdk::utils::Net::resolve_hostname_ipv4(address);
  if (seed_ip == "127.0.1.1") {
    console->print_error(
        "Cannot use host '" + address + "' for instance '" + instance->descr() +
        "' because it resolves to an IP address (127.0.1.1) that does not "
        "match a real network interface, thus it is not supported by the Group "
        "Replication communication layer. Change your system settings and/or "
        "set the MySQL server 'report_host' variable to a hostname that "
        "resolves to a supported IP address.");

    throw std::runtime_error("Invalid host/IP '" + address +
                             "' resolves to '127.0.1.1' which is not "
                             "supported by Group Replication.");
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
 * @param  config               pointer to config handler
 * @param  can_persist          true if instance has support to persist
 *                              variables, false if has but it is disabled and
 *                              null if it is not supported.
 * @param  restart_needed[out]  true if instance needs to be restarted
 * @param  mycnf_change_needed[out]  true if my.cnf has to be updated
 * @param  sysvar_change_needed[out] true if sysvars have to be updated
 * @param  ret_val[out]         assigned to check results
 * @return                      [description]
 */

std::vector<mysqlshdk::gr::Invalid_config> validate_configuration(
    mysqlshdk::mysql::IInstance *instance, const std::string &mycnf_path,
    mysqlshdk::config::Config *const config,
    const mysqlshdk::utils::nullable<bool> &can_persist, bool *restart_needed,
    bool *mycnf_change_needed, bool *sysvar_change_needed,
    shcore::Value *ret_val) {
  // Check supported innodb_page_size (must be > 4k). See: BUG#27329079
  validate_innodb_page_size(instance);

  // Check if performance_schema is enabled
  // BUG#25867733
  validate_performance_schema_enabled(*instance);

  log_info("Validating configuration of %s (mycnf = %s)",
           instance->descr().c_str(), mycnf_path.c_str());
  // Perform check with no update
  std::vector<mysqlshdk::gr::Invalid_config> invalid_cfs_vec =
      check_instance_config(*instance, *config);

  // Sort invalid cfs_vec by the name of the variable
  std::sort(invalid_cfs_vec.begin(), invalid_cfs_vec.end());

  // Build the map to be used by the print_validation results
  shcore::Value::Map_type_ref map_val(new shcore::Value::Map_type());
  shcore::Value check_result = shcore::Value(map_val);
  *restart_needed = false;
  *mycnf_change_needed = false;
  *sysvar_change_needed = false;
  bool log_bin_wrong = false;

  if (invalid_cfs_vec.empty()) {
    (*map_val)["status"] = shcore::Value("ok");
  } else {
    (*map_val)["status"] = shcore::Value("error");
    shcore::Value::Array_type_ref config_errors(
        new shcore::Value::Array_type());
    for (mysqlshdk::gr::Invalid_config &cfg : invalid_cfs_vec) {
      shcore::Value::Map_type_ref error(new shcore::Value::Map_type());
      std::string action;
      if (!log_bin_wrong && cfg.var_name == "log_bin" &&
          cfg.types.is_set(mysqlshdk::gr::Config_type::CONFIG)) {
        log_bin_wrong = true;
      }

      if (cfg.types.is_set(mysqlshdk::gr::Config_type::CONFIG) &&
          cfg.types.is_set(mysqlshdk::gr::Config_type::SERVER)) {
        *mycnf_change_needed = true;
        if (cfg.restart) {
          *restart_needed = true;
          action = "config_update+restart";
        } else {
          action = "server_update+config_update";
          *sysvar_change_needed = true;
        }
      } else if (cfg.types.is_set(mysqlshdk::gr::Config_type::CONFIG)) {
        *mycnf_change_needed = true;
        action = "config_update";
      } else if (cfg.types.is_set(mysqlshdk::gr::Config_type::SERVER)) {
        if (cfg.restart) {
          *restart_needed = true;
          action = "restart";
        } else {
          action = "server_update";
          *sysvar_change_needed = true;
        }
      } else if (cfg.types.empty()) {
        throw std::runtime_error("Unexpected change type");
      }

      (*error)["option"] = shcore::Value(cfg.var_name);
      (*error)["current"] = shcore::Value(cfg.current_val);
      (*error)["required"] = shcore::Value(cfg.required_val);
      (*error)["action"] = shcore::Value(action);
      config_errors->push_back(shcore::Value(error));
    }
    (*map_val)["config_errors"] = shcore::Value(config_errors);
  }
  if (ret_val) *ret_val = check_result;

  log_debug("Check command returned: %s", check_result.descr().c_str());

  if (!invalid_cfs_vec.empty()) {
    auto console = mysqlsh::current_console();

    console->println();
    print_validation_results(check_result.as_map(), true);
    if (*restart_needed) {
      // if we have to change some read only variables, print a message to the
      // user.
      std::string base_msg =
          "Some variables need to be changed, but cannot be done dynamically "
          "on the server";
      if (log_bin_wrong && *mycnf_change_needed) {
        base_msg += ": an option file is required";
      } else {
        if (*mycnf_change_needed) {
          if (can_persist.is_null()) {
            // 5.7 server, set persist is not supported
            base_msg += ": an option file is required";
          } else if (!*can_persist) {
            // 8.0 server with set persist support disabled
            base_msg +=
                ": set persist support is disabled. Enable it or provide an "
                "option file";
          }
        }
      }
      base_msg += ".";
      console->print_info(base_msg);
    }
  }
  return invalid_cfs_vec;
}

void validate_performance_schema_enabled(
    const mysqlshdk::mysql::IInstance &instance) {
  log_info("Checking if performance_schema is enabled on instance '%s'.",
           instance.descr().c_str());
  mysqlshdk::utils::nullable<bool> ps_enabled = instance.get_sysvar_bool(
      "performance_schema", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  auto console = mysqlsh::current_console();

  if (ps_enabled.is_null() || !*ps_enabled) {
    console->print_error("Instance '" + instance.descr() +
                         "' has the performance_schema "
                         "disabled (performance_schema=OFF). Instances must "
                         "have the performance_schema enabled to for InnoDB "
                         "Cluster usage.");
    throw shcore::Exception::runtime_error(
        "performance_schema disabled on target instance.");
  }
}

void ensure_instance_not_belong_to_replicaset(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlsh::dba::ReplicaSet &replicaset) {
  GRInstanceType type =
      mysqlsh::dba::get_gr_instance_type(instance.get_session());

  if (type != GRInstanceType::Standalone &&
      type != GRInstanceType::StandaloneWithMetadata) {
    // Retrieves the new instance UUID
    mysqlshdk::utils::nullable<std::string> uuid = instance.get_sysvar_string(
        "server_uuid", mysqlshdk::mysql::Var_qualifier::GLOBAL);

    // Verifies if the instance is part of the cluster replication group
    auto cluster_session = replicaset.get_cluster()->get_group_session();

    // Verifies if this UUID is part of the current replication group
    if (is_server_on_replication_group(cluster_session, *uuid)) {
      if (type == GRInstanceType::InnoDBCluster) {
        log_debug("Instance '%s' already managed by InnoDB cluster",
                  instance.descr().c_str());
        throw shcore::Exception::runtime_error(
            "The instance '" + instance.descr() +
            "' is already part of this InnoDB cluster");
      } else {
        current_console()->print_error(
            "Instance '" + instance.descr() +
            "' is part of the Group Replication group but is not in the "
            "metadata. Please use <Cluster>.rescan() to update the metadata.");
        throw shcore::Exception::runtime_error("Metadata inconsistent");
      }
    } else {
      if (type == GRInstanceType::InnoDBCluster) {
        // Check if instance is running auto-rejoin and warn user.
        if (mysqlshdk::gr::is_running_gr_auto_rejoin(instance)) {
          throw shcore::Exception::runtime_error(
              "The instance '" + instance.descr() +
              "' is currently attempting to rejoin the cluster. Use <cluster>."
              "rejoinInstance() if you want to to override the auto-rejoin "
              "process.");
        } else {
          throw shcore::Exception::runtime_error(
              "The instance '" + instance.descr() +
              "' is already part of another InnoDB cluster");
        }
      } else {
        throw shcore::Exception::runtime_error(
            "The instance '" + instance.descr() +
            "' is already part of another Replication Group");
      }
    }
  }
}

void ensure_instance_not_belong_to_metadata(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &address_in_metadata,
    const mysqlsh::dba::ReplicaSet &replicaset) {
  auto console = mysqlsh::current_console();

  // Check if the instance exists on the ReplicaSet
  log_debug("Checking if the instance belongs to the replicaset");
  bool is_instance_on_md =
      replicaset.get_cluster()
          ->get_metadata_storage()
          ->is_instance_on_replicaset(replicaset.get_id(), address_in_metadata);

  if (is_instance_on_md) {
    // Check if instance is running auto-rejoin
    bool is_rejoining = mysqlshdk::gr::is_running_gr_auto_rejoin(instance);

    std::string err_msg = "The instance '" + instance.descr() +
                          "' already belongs to the ReplicaSet: '" +
                          replicaset.get_name() + "'";
    if (is_rejoining)
      err_msg += " and is currently trying to auto-rejoin.";
    else
      err_msg += ".";
    throw shcore::Exception::runtime_error(err_msg);
  }
}

}  // namespace checks
}  // namespace dba
}  // namespace mysqlsh
