/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/version.h"

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
 * @param  instance target instance for the validation. Must be authenticated
 *         with an account with SELECT access to all schemas.
 * @return true if no issues found.
 */
bool validate_schemas(const mysqlshdk::mysql::IInstance &instance) {
  bool ok = true;
  std::string k_gr_compliance_skip_schemas =
      "('mysql', 'sys', 'performance_schema', 'information_schema')";
  std::string k_gr_compliance_skip_engines = "('InnoDB', 'MEMORY')";

  auto console = mysqlsh::current_console();

  {
    std::shared_ptr<mysqlshdk::db::IResult> result = instance.query(
        "SELECT table_schema, table_name, engine "
        " FROM information_schema.tables "
        " WHERE engine NOT IN " +
        k_gr_compliance_skip_engines + " AND table_schema NOT IN " +
        k_gr_compliance_skip_schemas);

    auto row = result->fetch_one();
    if (row) {
      console->print_error(
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
    std::shared_ptr<mysqlshdk::db::IResult> result = instance.query(
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
      console->print_error(
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
    console->print_info(
        "If you can't change the tables structure to include an extra visible "
        "key to be used as PRIMARY KEY, you can make use of the INVISIBLE "
        "COLUMN feature available since 8.0.23: "
        "https://dev.mysql.com/doc/refman/8.0/en/invisible-columns.html");
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
 * The host address will be printed to the console along with an informational
 * message.
 *
 * If the target host name resolves to a loopback address (127.*) an error
 * will be printed and the function returns false. Unless the target is a
 * sandbox, detected by checking that the name of the datadir is sandboxdata.
 *
 * If an IPv6 address is used and the instance version is < 8.0.14
 * throw an exception.
 *
 * @param  instance target instance with cached global sysvars
 * @param  verbose  if 1 additional informational messages are shown, if 2
 * further explanations are provided
 * @throw  an exception if the host name configuration is not suitable
 */
void validate_host_address(const mysqlshdk::mysql::IInstance &instance,
                           int verbose) {
  auto console = mysqlsh::current_console();

  bool report_host_set;
  std::string hostname;
  std::string report_host;

  auto result = instance.query("SELECT @@hostname, @@report_host");
  auto row = result->fetch_one_or_throw();
  hostname = row->get_string(0);
  report_host = row->get_string(1, "");
  report_host_set = !row->is_null(1);

  if (report_host_set) {
    log_debug("Target has report_host=%s", report_host.c_str());
    log_debug("Target has hostname=%s", hostname.c_str());
    hostname = report_host;
  } else {
    log_debug("Target has report_host=NULL");
    log_debug("Target has hostname=%s", hostname.c_str());
  }

  if (report_host_set && report_host.empty()) {
    console->print_error("Invalid 'report_host' value for instance '" +
                         instance.get_connection_options().uri_endpoint() +
                         "'. The value cannot be empty if defined.");
    // NOTE: The value for report_host can be set to an empty string which is
    // invalid. If defined the report_host value should not be an empty
    // string, otherwise it is used by replication as an empty string "".
    throw std::runtime_error(
        "The value for variable 'report_host' cannot be empty.");
  }

  if (verbose > 0) {
    console->println();
    console->print_info(
        "This instance reports its own address as " +
        mysqlshdk::textui::bold(instance.get_canonical_address()));
    if (!report_host_set && verbose > 1) {
      console->print_info(
          "Clients and other cluster members will communicate with it through "
          "this address by default. "
          "If this is not correct, the report_host MySQL "
          "system variable should be changed.");
    }
  }

  // Validate the IP of the hostname used for GR.
  try {
    std::string seed_ip =
        mysqlshdk::utils::Net::resolve_hostname_ipv4(hostname);
    const bool is_loopback = mysqlshdk::utils::Net::is_loopback(hostname);
    if (is_loopback && seed_ip != "127.0.0.1") {
      // Loopback IPv4 addresses except for 127.0.0.1 are not supported by GCS
      // leading to errors.
      console->print_error(shcore::str_format(
          "Cannot use host '%s' for instance '%s' because it resolves to an IP "
          "address (%s) that does not "
          "match a real network interface, thus it is not supported. Change "
          "your system settings and/or "
          "set the MySQL server 'report_host' variable to a hostname that "
          "resolves to a supported IP address.",
          hostname.c_str(), instance.descr().c_str(), seed_ip.c_str()));

      throw std::runtime_error(shcore::str_format(
          "Invalid host/IP '%s' resolves to '%s' which is not "
          "supported.",
          hostname.c_str(), seed_ip.c_str()));
    }
  } catch (const mysqlshdk::utils::net_error &error) {
    // if it is an IPv6 address and the instance version does not support IPv6
    // throw an error
    // Do not try to resolve hostnames. Even if we can resolve them, there is
    // no guarantee that name resolution is the same across cluster instances
    // and the instance where we are running the ngshell
    bool is_ipv6 = mysqlshdk::utils::Net::is_ipv6(hostname);
    bool supports_ipv6 =
        instance.get_version() >= mysqlshdk::utils::Version(8, 0, 14);
    if (is_ipv6 && !supports_ipv6) {
      // Using an IPv6 address and the instance does not support it.
      console->print_error(shcore::str_format(
          "Cannot use host '%s' for instance '%s' "
          "because it is an IPv6 address which is only supported by "
          "Group Replication from MySQL version >= 8.0.14. Set the MySQL "
          "server 'report_host' "
          "variable to an IPv4 address or hostname that resolves an IPv4 "
          "address.",
          hostname.c_str(), instance.descr().c_str()));
      throw std::runtime_error(shcore::str_format(
          "Unsupported IP address '%s'. IPv6 is only "
          "supported by Group Replication on MySQL version >= 8.0.14.",
          hostname.c_str()));
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
 * @param  config               pointer to config handler
 * @param  cluster_type         target cluster/replicaset type
 * @param  can_persist          true if instance has support to persist
 *                              variables, false if has but it is disabled and
 *                              null if it is not supported.
 * @param  restart_needed[out]  true if instance needs to be restarted
 * @param  mycnf_change_needed[out]  true if my.cnf has to be updated
 * @param  sysvar_change_needed[out] true if sysvars have to be updated
 * @param  ret_val[out]         assigned to check results
 * @return                      [description]
 */
std::vector<mysqlshdk::mysql::Invalid_config> validate_configuration(
    mysqlshdk::mysql::IInstance *instance, const std::string &mycnf_path,
    mysqlshdk::config::Config *const config, Cluster_type cluster_type,
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
  std::vector<mysqlshdk::mysql::Invalid_config> invalid_cfs_vec =
      check_instance_config(*instance, *config, cluster_type);

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
    for (mysqlshdk::mysql::Invalid_config &cfg : invalid_cfs_vec) {
      shcore::Value::Map_type_ref error(new shcore::Value::Map_type());
      std::string action;
      if (!log_bin_wrong && cfg.var_name == "log_bin" &&
          cfg.types.is_set(mysqlshdk::mysql::Config_type::CONFIG)) {
        log_bin_wrong = true;
      }

      if (cfg.types.is_set(mysqlshdk::mysql::Config_type::CONFIG) &&
          cfg.types.is_set(mysqlshdk::mysql::Config_type::SERVER)) {
        *mycnf_change_needed = true;
        if (cfg.restart) {
          *restart_needed = true;
          action = "config_update+restart";
        } else {
          action = "server_update+config_update";
          *sysvar_change_needed = true;
        }
      } else if (cfg.types.is_set(mysqlshdk::mysql::Config_type::CONFIG)) {
        *mycnf_change_needed = true;
        action = "config_update";
      } else if (cfg.types.is_set(mysqlshdk::mysql::Config_type::SERVER)) {
        *sysvar_change_needed = true;
        if (cfg.restart) {
          *restart_needed = true;
          action = "server_update+restart";
        } else {
          action = "server_update";
        }
      } else if (cfg.types.is_set(
                     mysqlshdk::mysql::Config_type::RESTART_ONLY)) {
        *restart_needed = true;
        action = "restart";
      } else if (cfg.types.empty()) {
        throw std::runtime_error("Unexpected change type");
      }

      (*error)["option"] = shcore::Value(cfg.var_name);
      (*error)["current"] = shcore::Value(cfg.current_val);
      (*error)["required"] = shcore::Value(cfg.required_val);
      (*error)["action"] = shcore::Value(action);
      if (!cfg.persisted_val.is_null()) {
        (*error)["persisted"] = shcore::Value(*cfg.persisted_val);
      }
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

TargetType::Type ensure_instance_not_belong_to_cluster(
    const mysqlshdk::mysql::IInstance &instance,
    const std::shared_ptr<Instance> &cluster_instance,
    bool *out_already_member) {
  TargetType::Type type = mysqlsh::dba::get_gr_instance_type(instance);

  if (out_already_member) *out_already_member = false;

  if (type != TargetType::Standalone &&
      type != TargetType::StandaloneWithMetadata &&
      type != TargetType::StandaloneInMetadata) {
    // Retrieves the new instance UUID
    std::string uuid = instance.get_uuid();

    // Verifies if this UUID is part of the current replication group
    std::vector<mysqlshdk::gr::Member> members(
        mysqlshdk::gr::get_members(*cluster_instance));

    if (std::find_if(members.begin(), members.end(),
                     [&uuid](const mysqlshdk::gr::Member &member) {
                       return member.uuid == uuid;
                     }) != members.end()) {
      if (type == TargetType::InnoDBCluster ||
          type == TargetType::InnoDBClusterSet) {
        log_debug("Instance '%s' already managed by InnoDB cluster",
                  instance.descr().c_str());
        if (out_already_member) {
          // don't throw exception if out_already_member is given
          *out_already_member = true;
        } else {
          throw shcore::Exception(
              "The instance '" + instance.descr() +
                  "' is already part of this InnoDB cluster",
              SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER);
        }
      } else {
        if (out_already_member) {
          *out_already_member = true;
        } else {
          current_console()->print_error(
              "Instance '" + instance.descr() +
              "' is part of the Group Replication group but is not in the "
              "metadata. Please use <Cluster>.rescan() to update the "
              "metadata.");
          throw shcore::Exception(
              "Metadata inconsistent: " + instance.descr() +
                  " is a group member but is not in the metadata.",
              SHERR_DBA_ASYNC_MEMBER_INCONSISTENT);
        }
      }
    } else {
      if (type == TargetType::InnoDBCluster ||
          type == TargetType::InnoDBClusterSet) {
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
  return type;
}

void ensure_instance_not_belong_to_metadata(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &address_in_metadata,
    const mysqlsh::dba::Cluster_impl &cluster) {
  auto console = mysqlsh::current_console();

  // Check if the instance exists on the cluster
  log_debug("Checking if the instance belongs to the cluster");
  Instance_metadata instance_md;
  try {
    instance_md = cluster.get_metadata_storage()->get_instance_by_address(
        address_in_metadata);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
      return;
    }
    throw;
  }

  if (instance_md.cluster_id == cluster.get_id()) {
    // Check if instance is running auto-rejoin
    bool is_rejoining = mysqlshdk::gr::is_running_gr_auto_rejoin(instance);

    std::string err_msg = "The instance '" + instance.descr() +
                          "' already belongs to the cluster: '" +
                          cluster.get_name() + "'";
    if (is_rejoining) {
      err_msg += " and is currently trying to auto-rejoin.";
    } else {
      err_msg += ".";
    }
    throw shcore::Exception::runtime_error(err_msg);
  }
}

/**
 * Validate if asynchronous replication is configured on the target instance.
 *
 * This function validates if replication is configured on the target
 * instance and depending on the type of check, prints a warning message,
 * or an error message and terminates with an exception.
 *
 * @param instance The instance to validate.
 * @param type     The type of check (Check_type)
 */
void validate_async_channels(const mysqlshdk::mysql::IInstance &instance,
                             Check_type type) {
  log_debug(
      "Checking if instance '%s' has asynchronous (source-replica) "
      "replication configured.",
      instance.descr().c_str());

  if (mysqlshdk::mysql::is_async_replication_configured(instance)) {
    auto console = mysqlsh::current_console();
    std::string error_msg = "";

    switch (type) {
      case Check_type::CHECK:
        error_msg += "The";
        break;
      case Check_type::CREATE:
        error_msg += "Cannot create cluster on";
        break;
      case Check_type::BOOTSTRAP:
        error_msg += "Cannot bootstrap cluster on";
        break;
      case Check_type::JOIN:
        error_msg += "Cannot join";
        break;
      case Check_type::REJOIN:
        error_msg += "Cannot rejoin";
        break;
    }

    error_msg +=
        " instance '" + instance.descr() + "' " +
        std::string((type == Check_type::JOIN || type == Check_type::REJOIN)
                        ? "to the cluster "
                        : "") +
        std::string(type == Check_type::CHECK
                        ? "cannot be added to an InnoDB cluster "
                        : "") +
        "because it has asynchronous (source-replica) "
        "replication channel(s) configured. "
        "MySQL InnoDB Cluster does not support manually configured channels "
        "as they are not managed using the AdminAPI (e.g. when PRIMARY moves "
        "to another member) which may cause cause replication to break or "
        "even create split-brain scenarios (data loss).";

    if (type == Check_type::CREATE) {
      error_msg +=
          " Use the 'force' option to skip this validation on a temporary "
          "scenario (e.g. migrating from a replication topology to InnoDB "
          "Cluster).";
    }

    if (type == Check_type::CHECK) {
      console->print_warning(error_msg);
    } else {
      console->print_error(error_msg);

      throw shcore::Exception::runtime_error(
          "The instance '" + instance.descr() +
          "' has asynchronous replication configured.");
    }
  }
}

}  // namespace checks
}  // namespace dba
}  // namespace mysqlsh
