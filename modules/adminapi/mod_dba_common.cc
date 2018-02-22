/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/mod_dba_common.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/mysql/user_privileges.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_file.h"
// #include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "modules/adminapi/mod_dba_sql.h"

namespace mysqlsh {
namespace dba {

namespace ReplicaSetStatus {
std::string describe(Status state) {
  std::string ret_val;

  switch (state) {
    case OK:
      ret_val = "OK";
      break;
    case OK_PARTIAL:
      ret_val = "OK_PARTIAL";
      break;
    case OK_NO_TOLERANCE:
      ret_val = "OK_NO_TOLERANCE";
      break;
    case NO_QUORUM:
      ret_val = "NO_QUORUM";
      break;
    case UNKNOWN:
      ret_val = "UNKNOWN";
      break;
  }
  return ret_val;
}
};  // namespace ReplicaSetStatus

std::string get_mysqlprovision_error_string(
    const shcore::Value::Array_type_ref &errors) {
  if (!errors)
    return "mysqlprovision error";
  std::vector<std::string> str_errors;

  if (errors) {
    for (auto error : *errors) {
      auto data = error.as_map();
      auto error_type = data->get_string("type");
      auto error_text = data->get_string("msg");

      str_errors.push_back(error_type + ": " + error_text);
    }
  } else {
    str_errors.push_back("Unexpected error calling mysqlprovision, "
      "see the shell log for more details");
  }

  return shcore::str_join(str_errors, "\n");
}

const char *kMemberSSLModeAuto = "AUTO";
const char *kMemberSSLModeRequired = "REQUIRED";
const char *kMemberSSLModeDisabled = "DISABLED";
const std::set<std::string> kMemberSSLModeValues = {kMemberSSLModeAuto,
                                                    kMemberSSLModeDisabled,
                                                    kMemberSSLModeRequired};

void validate_ssl_instance_options(const shcore::Value::Map_type_ref &options) {
  // Validate use of SSL options for the cluster instance and issue an
  // exception if invalid.
  shcore::Argument_map opt_map(*options);
  if (opt_map.has_key("adoptFromGR")) {
    bool adopt_from_gr = opt_map.bool_at("adoptFromGR");
    if (adopt_from_gr && (opt_map.has_key("memberSslMode")))
      throw shcore::Exception::argument_error(
          "Cannot use memberSslMode option if adoptFromGR is set to true.");
  }

  if (opt_map.has_key("memberSslMode")) {
    std::string ssl_mode = opt_map.string_at("memberSslMode");
    ssl_mode = shcore::str_upper(ssl_mode);
    if (kMemberSSLModeValues.count(ssl_mode) == 0) {
      std::string valid_values = shcore::str_join(kMemberSSLModeValues, ",");
      throw shcore::Exception::argument_error(
          "Invalid value for memberSslMode option. "
              "Supported values: " + valid_values + ".");
    }
  }
}

void validate_ip_whitelist_option(const shcore::Value::Map_type_ref &options) {
  // Validate the value of the ipWhitelist option an issue an exception
  // if invalid.
  shcore::Argument_map opt_map(*options);
  // Just a very simple validation is enough since the GCS layer on top of
  // which group replication runs has proper validation for the values provided
  // to the ipWhitelist option.
  if (opt_map.has_key("ipWhitelist")) {
    std::string ip_whitelist = options->get_string("ipWhitelist");
    ip_whitelist = shcore::str_strip(ip_whitelist);
    if (ip_whitelist.empty())
      throw shcore::Exception::argument_error(
          "Invalid value for ipWhitelist, string value cannot be empty.");
  }
}

/**
 * Validate the value specified for the localAddress option.
 *
 * @param options Map type value with containing the specified options.
 * @throw ArgumentError if the value is empty or no host and port is specified
 *        (i.e., value is ":").
 */
void validate_local_address_option(const shcore::Value::Map_type_ref &options) {
  // Minimal validation is performed here, the rest is already currently
  // handled at the mysqlprovision level (including the logic to automatically
  // set the host and port when not specified).
  shcore::Argument_map opt_map(*options);
  if (opt_map.has_key("localAddress")) {
    std::string local_address = opt_map.string_at("localAddress");
    local_address = shcore::str_strip(local_address);
    if (local_address.empty())
      throw shcore::Exception::argument_error(
          "Invalid value for localAddress, string value cannot be empty.");
    if (local_address.compare(":") == 0)
      throw shcore::Exception::argument_error(
          "Invalid value for localAddress. If ':' is specified then at least a "
          "non-empty host or port must be specified: '<host>:<port>' or "
          "'<host>:' or ':<port>'.");
  }
}

/**
 * Validate the value specified for the groupSeeds option.
 *
 * @param options Map type value with containing the specified options.
 * @throw ArgumentError if the value is empty.
 */
void validate_group_seeds_option(const shcore::Value::Map_type_ref &options) {
  // Minimal validation is performed here the rest is already currently
  // handled at the mysqlprovision level (including the logic to automatically
  // set the group seeds when not specified)
  shcore::Argument_map opt_map(*options);
  if (opt_map.has_key("groupSeeds")) {
    std::string group_seeds = opt_map.string_at("groupSeeds");
    group_seeds = shcore::str_strip(group_seeds);
    if (group_seeds.empty())
      throw shcore::Exception::argument_error(
          "Invalid value for groupSeeds, string value cannot be empty.");
  }
}

/**
 * Validate the value specified for the groupName option.
 *
 * @param options Map type value with containing the specified options.
 * @throw ArgumentError if the value is empty.
 */
void validate_group_name_option(const shcore::Value::Map_type_ref &options) {
  // Minimal validation is performed here, the rest is already currently
  // handled at the mysqlprovision level (including the logic to automatically
  // set the group name when not specified)
  shcore::Argument_map opt_map(*options);
  if (opt_map.has_key("groupName")) {
    std::string group_name = opt_map.string_at("groupName");
    group_name = shcore::str_strip(group_name);
    if (group_name.empty())
      throw shcore::Exception::argument_error(
          "Invalid value for groupName, string value cannot be empty.");
  }
}

/*
 * Check the existence of replication filters that do not exclude the
 * metadata from being replicated.
 * Raise an exception if invalid replication filters are found.
 */
void validate_replication_filters(
    std::shared_ptr<mysqlshdk::db::ISession> session) {
  shcore::Value status = get_master_status(session);
  auto status_map = status.as_map();

  std::string binlog_do_db = status_map->get_string("BINLOG_DO_DB");

  if (!binlog_do_db.empty() &&
      binlog_do_db.find("mysql_innodb_cluster_metadata") == std::string::npos)
    throw shcore::Exception::runtime_error(
        "Invalid 'binlog-do-db' settings, metadata cannot be excluded. Remove "
            "binlog filters or include the 'mysql_innodb_cluster_metadata' "
            "database in the 'binlog-do-db' option.");

  std::string binlog_ignore_db = status_map->get_string("BINLOG_IGNORE_DB");

  if (binlog_ignore_db.find("mysql_innodb_cluster_metadata") !=
      std::string::npos)
    throw shcore::Exception::runtime_error(
        "Invalid 'binlog-ignore-db' settings, metadata cannot be excluded. "
            "Remove binlog filters or the 'mysql_innodb_cluster_metadata' "
            "database from the 'binlog-ignore-db' option.");
}


/** Count how many accounts there are with the given username, excluding
  localhost

  This allows us to know whether there's a wildcarded account that the user may
  have created previously which we can use for management or whether the user
  has created multiple accounts, which would mean that they must know what
  they're doing and also that we can't tell which of these accounts should be
  validated.

  @return # of wildcarded accounts, # of other accounts
  */
std::pair<int, int> find_cluster_admin_accounts(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &admin_user, std::vector<std::string> *out_hosts) {

  shcore::sqlstring query;
  // look up the hosts the account is allowed from
  query = shcore::sqlstring(
      "SELECT DISTINCT grantee"
      " FROM information_schema.user_privileges"
      " WHERE grantee like ?", 0) <<
      ((shcore::sqlstring("?", 0) << admin_user).str()+"@%");

  int local = 0;
  int w = 0;
  int nw = 0;
  auto result = session->query(query);
  if (result) {
    auto row = result->fetch_one();
    while (row) {
      std::string account = row->get_string(0);
      std::string user, host;
      shcore::split_account(account, &user, &host, true);
      assert(user == admin_user);

      if (out_hosts)
        out_hosts->push_back(host);
      // ignore localhost accounts
      if (host == "localhost" || host == "127.0.0.1") {
        local++;
      } else {
        if (host.find('%') != std::string::npos) {
          w++;
        } else {
          nw++;
        }
      }
      row = result->fetch_one();
    }
  }
  log_info("%s user has %i accounts with wildcard and %i without",
           admin_user.c_str(), w, nw + local);

  return std::make_pair(w, nw);
}

namespace {

/*
 * Validate if the user specified for being the cluster admin has all of the
 * requirements:
 * 1) has a host wildcard other than 'localhost' or '127.0.0.1' or the one
 *    specified
 * 2) has the necessary privileges assigned
 */

// Global privs needed for managing cluster instances
const std::set<std::string> k_global_privileges{
  "RELOAD", "SHUTDOWN", "PROCESS", "FILE",
  "SUPER", "REPLICATION SLAVE", "REPLICATION CLIENT",
  "CREATE USER"
};

// Schema privileges needed on the metadata schema
const std::set<std::string> k_metadata_schema_privileges{
  "ALTER", "ALTER ROUTINE", "CREATE",
  "CREATE ROUTINE", "CREATE TEMPORARY TABLES",
  "CREATE VIEW", "DELETE", "DROP",
  "EVENT", "EXECUTE", "INDEX", "INSERT", "LOCK TABLES",
  "REFERENCES", "SELECT", "SHOW VIEW", "TRIGGER", "UPDATE"
};

// Schema privileges needed on the mysql schema
const std::set<std::string> k_mysql_schema_privileges{
  "SELECT", "INSERT", "UPDATE", "DELETE"
};

// list of (schema, [privilege]) pairs, with the required privileges on each
// schema
const std::map<std::string, std::set<std::string>> k_schema_grants {
  {"mysql_innodb_cluster_metadata", k_metadata_schema_privileges},
  {"sys", {"SELECT"}},
  {"mysql", k_mysql_schema_privileges }  // need for mysql.plugin,
                                         // mysql.user others
};

// List of (schema, (table, [privilege])) triples, with the required privileges
// on each schema.table.
// Our code uses some additional performance_schema system variable tables
// (global_status, session_status, variables_info), but they do not require a
// specific privilege (enabled by default).
// See: https://dev.mysql.com/doc/refman/8.0/en/performance-schema-system-variable-tables.html
const std::map<std::string, std::map<std::string, std::set<std::string>>>
    k_table_grants {
  {
    "performance_schema", {
      {"replication_applier_configuration", {"SELECT"}},
      {"replication_applier_status", {"SELECT"}},
      {"replication_applier_status_by_coordinator", {"SELECT"}},
      {"replication_applier_status_by_worker", {"SELECT"}},
      {"replication_connection_configuration", {"SELECT"}},
      {"replication_connection_status", {"SELECT"}},
      {"replication_group_member_stats", {"SELECT"}},
      {"replication_group_members", {"SELECT"}},
      {"threads", {"SELECT"}},  // used in code
    }
  }
};

}  // namespace

/** Check that the provided account has privileges to manage a cluster.
 */
bool validate_cluster_admin_user_privileges(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &admin_user, const std::string &admin_host,
    std::string *validation_error) {
  bool is_valid = true;

  auto append_error_message = [validation_error, &is_valid](
                                  const std::string &info,
                                  const std::set<std::string> &privileges) {
    is_valid = false;

    *validation_error +=
        "Missing " + info + ": " + shcore::str_join(privileges, ", ") + ". ";
  };

  log_info("Validating account %s@%s...", admin_user.c_str(),
           admin_host.c_str());

  mysqlshdk::mysql::User_privileges user_privileges{session, admin_user,
                                                    admin_host};

  // Check global privileges.
  auto global = user_privileges.validate(k_global_privileges);

  if (global.has_missing_privileges()) {
    auto missing = global.get_missing_privileges();

    if (!global.has_grant_option()) {
      missing.insert("GRANT OPTION");
    }

    append_error_message("global privileges", missing);
  }

  for (const auto &schema_grants : k_schema_grants) {
    auto schema =
        user_privileges.validate(schema_grants.second, schema_grants.first);

    if (schema.has_missing_privileges()) {
      append_error_message("privileges on schema '" + schema_grants.first + "'",
                           schema.get_missing_privileges());
    }
  }

  for (const auto &schema_grants : k_table_grants) {
    for (const auto &table_grants : schema_grants.second) {
      auto table = user_privileges.validate(table_grants.second,
                                            schema_grants.first,
                                            table_grants.first);

      if (table.has_missing_privileges()) {
        append_error_message("privileges on table '" + schema_grants.first +
                             "." + table_grants.first + "'",
                             table.get_missing_privileges());
      }
    }
  }

  return is_valid;
}

namespace {

std::string create_grant(const std::string &username,
                         const std::set<std::string> &privileges,
                         const std::string &schema = "*",
                         const std::string &object = "*") {
  return "GRANT " + shcore::str_join(privileges, ", ") + " ON " + schema +
         "." + object + " TO " + username + " WITH GRANT OPTION";
}

std::vector<std::string> create_grants(const std::string &username) {
  std::vector<std::string> grants;

  // global privileges
  grants.emplace_back(create_grant(username, k_global_privileges));

  // privileges for schemas
  for (const auto &schema : k_schema_grants)
    grants.emplace_back(create_grant(username, schema.second, schema.first));

  // privileges for tables
  for (const auto &schema : k_table_grants)
    for (const auto &table : schema.second)
      grants.emplace_back(create_grant(username, table.second, schema.first,
                                       table.first));

  return grants;
}

}  // namespace

void create_cluster_admin_user(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &username, const std::string &password) {
#ifndef NDEBUG
  shcore::split_account(username, nullptr, nullptr);
#endif

  // We're not checking if the current user has enough privileges to create a
  // cluster admin here, because such check should be performed before calling
  // this function.
  // If the current user doesn't have the right privileges to create a cluster
  // admin, one of the SQL statements below will throw.

  session->execute("SET sql_log_bin = 0");
  session->execute("START TRANSACTION");
  try {
    log_info("Creating account %s", username.c_str());
    // Create the user
    {
      auto query = shcore::sqlstring(("CREATE USER " + username +
                                      " IDENTIFIED BY ?").c_str(), 0);
      query << password;
      query.done();
      session->execute(query);
    }

    // Give the grants
    for (const auto &grant : create_grants(username))
      session->execute(grant);
    session->execute("COMMIT");
    session->execute("SET sql_log_bin = 1");
  } catch (...) {
    session->execute("ROLLBACK");
    session->execute("SET sql_log_bin = 1");
    throw;
  }
}

/**
 * Determines the value to use on the cluster for the SSL mode
 * based on:
 * - The instance having SSL available
 * - The instance having require_secure_transport ON
 * - The memberSslMode option given by the user
 *
 * Returns:
 *   REQUIRED if SSL is supported and member_ssl_mode is either AUTO or REQUIRED
 *   DISABLED if SSL is not supported and member_ssl_mode is either unspecified
 *            DISABLED or AUTO.
 *
 * Error:
 *   - If SSL is supported, require_secure_transport is ON and member_ssl_mode
 *     is DISABLED
 *   - If SSL mode is not supported and member_ssl_mode is REQUIRED
 */
std::string resolve_cluster_ssl_mode(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &member_ssl_mode) {
  std::string ret_val;
  std::string have_ssl;

  get_server_variable(session, "global.have_ssl",
                      have_ssl);

  // The instance supports SSL
  if (!shcore::str_casecmp(have_ssl.c_str(), "YES")) {
    // memberSslMode is DISABLED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "DISABLED")) {
      int require_secure_transport = 0;
      get_server_variable(session,
                          "GLOBAL.require_secure_transport",
                          require_secure_transport);

      if (require_secure_transport) {
        throw shcore::Exception::argument_error("The instance "
          "'" + session->uri() + "' requires secure connections, to create the "
          "cluster either turn off require_secure_transport or use the "
          "memberSslMode option with 'REQUIRED' value.");
      }

      ret_val = dba::kMemberSSLModeDisabled;
    } else {
      // memberSslMode is either AUTO or REQUIRED
      ret_val = dba::kMemberSSLModeRequired;
    }

  // The instance does not support SSL
  } else {
    // memberSslMode is REQUIRED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "REQUIRED")) {
      throw shcore::Exception::argument_error("The instance "
        "'" + session->uri() + "' does not have SSL enabled, to create the "
        "cluster either use an instance with SSL enabled, remove the "
        "memberSslMode option or use it with any of 'AUTO' or 'DISABLED'.");
    }

    // memberSslMode is either not defined, DISABLED or AUTO
    ret_val = dba::kMemberSSLModeDisabled;
  }

  return ret_val;
}

/**
 * Determines the value to use on the instance for the SSL mode
 * configured for the cluster, based on:
 * - The instance having SSL available
 * - The instance having require_secure_transport ON
 * - The cluster SSL configuration
 * - The memberSslMode option given by the user
 *
 * Returns:
 *   REQUIRED if SSL is supported and member_ssl_mode is either AUTO or REQUIRED
 *            and the cluster uses SSL
 *   DISABLED if SSL is not supported and member_ssl_mode is either unspecified
 *            DISABLED or AUTO and the cluster has SSL disabled.
 *
 * Error:
 *   - Cluster has SSL enabled and member_ssl_mode is DISABLED or not specified
 *   - Cluster has SSL enabled and SSL is not supported on the instance
 *   - Cluster has SSL disabled and member_ssl_mode is REQUIRED
 *   - Cluster has SSL disabled and the instance has require_secure_transport ON
 */
std::string resolve_instance_ssl_mode(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    std::shared_ptr<mysqlshdk::db::ISession> psession,
    const std::string &member_ssl_mode) {
  std::string ret_val;
  std::string gr_ssl_mode;

  // Checks how memberSslMode was configured on the cluster
  get_server_variable(psession,
                      "global.group_replication_ssl_mode",
                      gr_ssl_mode);

  // The cluster REQUIRES SSL
  if (!shcore::str_casecmp(gr_ssl_mode.c_str(), "REQUIRED")) {
    // memberSslMode is DISABLED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "DISABLED"))
      throw shcore::Exception::runtime_error(
          "The cluster has SSL (encryption) enabled. "
          "To add the instance '" + session->uri() + "' to the cluster either "
          "disable SSL on the cluster, remove the memberSslMode option or use "
          "it with any of 'AUTO' or 'REQUIRED'.");

    // Now checks if SSL is actually supported on the instance
    std::string have_ssl;

    get_server_variable(session, "global.have_ssl",
                        have_ssl);

    // SSL is not supported on the instance
    if (shcore::str_casecmp(have_ssl.c_str(), "YES")) {
      throw shcore::Exception::runtime_error(
          "Instance '" + session->uri() + "' does not support SSL and cannot "
          "join a cluster with SSL (encryption) enabled. "
          "Enable SSL support on the instance and try again, otherwise it can "
          "only be added to a cluster with SSL disabled.");
    }

    // memberSslMode is either AUTO or REQUIRED
    ret_val = dba::kMemberSSLModeRequired;

  // The cluster has SSL DISABLED
  } else if (!shcore::str_casecmp(gr_ssl_mode.c_str(), "DISABLED")) {

    // memberSslMode is REQUIRED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "REQUIRED"))
      throw shcore::Exception::runtime_error(
          "The cluster has SSL (encryption) disabled. "
          "To add the instance '" + session->uri() + "' to the cluster either "
          "enable SSL on the cluster, remove the memberSslMode option or use "
          "it with any of 'AUTO' or 'DISABLED'.");

    // If the instance session requires SSL connections, then it can's
    // Join a cluster with SSL disabled
    int secure_transport_required = 0;
    get_server_variable(session,
                        "global.require_secure_transport",
                        secure_transport_required);
    if (secure_transport_required) {
      throw shcore::Exception::runtime_error(
          "The instance '" + session->uri() + "' is configured to require a "
          "secure transport but the cluster has SSL disabled. To add the "
          "instance to the cluster, either turn OFF the "
          "require_secure_transport option on the instance or enable SSL on "
          "the cluster.");
    }

    ret_val = dba::kMemberSSLModeDisabled;
  } else {
    // Only GR SSL mode "REQUIRED" and "DISABLED" are currently supported.
    throw shcore::Exception::runtime_error(
        "Unsupported Group Replication SSL Mode for the cluster: '"
            + gr_ssl_mode + "'. If the cluster was created using "
            "adoptFromGR:true make sure the group_replication_ssl_mode "
            "variable is set with a supported value (DISABLED or REQUIRED) "
            "for all cluster members.");
  }

  return ret_val;
}

/**
 * Gets the list of instances belonging to the GR group to which the metadata
 * belongs to
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @return a vector of strings with the list of member id's (member_id) of the
 *         instances belonging to the GR group
 */
std::vector<std::string> get_instances_gr(
    const std::shared_ptr<MetadataStorage> &metadata) {
  // Get the list of instances belonging to the GR group
  std::string query =
      "SELECT member_id FROM performance_schema.replication_group_members";

  auto result = metadata->execute_sql(query);

  std::vector<std::string> ret_val;
  auto row = result->fetch_one();
  while (row) {
    ret_val.push_back(row->get_string(0));
    row = result->fetch_one();
  }

  return ret_val;
}

/**
 * Gets the list of instances registered on the metadata for a specific
 * ReplicaSet
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @param rd_id the ReplicaSet id
 * @return a vector of strings with the list of instances uuid's
 *         (mysql_server_uuid) of the instances registered on the metadata, for
 *         the replicaset with the id rs_id
 */
std::vector<std::string> get_instances_md(
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id) {
  // Get the list of instances registered on the Metadata
  shcore::sqlstring query("SELECT mysql_server_uuid FROM " \
                          "mysql_innodb_cluster_metadata.instances " \
                          "WHERE replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  auto result = metadata->execute_sql(query);
  std::vector<std::string> ret_val;
  auto row = result->fetch_one();

  while (row) {
    ret_val.push_back(row->get_string(0));
    row = result->fetch_one();
  }

  return ret_val;
}

/**
 * Gets the list of instances belonging to the GR group which are not
 * registered on the metadata for a specific ReplicaSet.
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @param rd_id the ReplicaSet id
 * @return a vector of NewInstanceInfo with the list of instances found
 * as belonging to the GR group but not registered on the Metadata
 */
std::vector<NewInstanceInfo> get_newly_discovered_instances(
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id) {
  std::vector<std::string> instances_gr_array, instances_md_array;

  instances_gr_array = get_instances_gr(metadata);

  instances_md_array = get_instances_md(metadata, rs_id);

  // Check the differences between the two lists
  std::vector<std::string> new_members;

  // Sort the arrays in order to be able to use
  // std::set_difference()
  std::sort(instances_gr_array.begin(), instances_gr_array.end());
  std::sort(instances_md_array.begin(), instances_md_array.end());

  // Check if the instances_gr list has more members than the instances_md lists
  // Meaning that an instance was added to the GR group outside of the AdminAPI
  std::set_difference(instances_gr_array.begin(), instances_gr_array.end(),
                      instances_md_array.begin(), instances_md_array.end(),
                      std::inserter(new_members, new_members.begin()));

  std::vector<NewInstanceInfo> ret;
  for (auto i : new_members) {
    shcore::sqlstring query(
        "SELECT MEMBER_ID, MEMBER_HOST, MEMBER_PORT " \
        "FROM performance_schema.replication_group_members " \
        "WHERE MEMBER_ID = ?", 0);
    query << i;
    query.done();

    auto result = metadata->execute_sql(query);
    auto row = result->fetch_one();

    NewInstanceInfo info;
    info.member_id = row->get_string(0);
    info.host = row->get_string(1);
    info.port = row->get_int(2);

    ret.push_back(info);
  }

  return ret;
}

/**
 * Gets the list of instances registered on the Metadata, for a specific
 * ReplicaSet, which do not belong in the GR group.
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @param rd_id the ReplicaSet id
 * @return a vector of MissingInstanceInfo with the list of instances found
 * as registered on the Metadatada but not present in the GR group.
 */
std::vector<MissingInstanceInfo> get_unavailable_instances(
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id) {
  std::vector<std::string> instances_gr_array, instances_md_array;

  instances_gr_array = get_instances_gr(metadata);
  instances_md_array = get_instances_md(metadata, rs_id);

  // Check the differences between the two lists
  std::vector<std::string> removed_members;

  // Sort the arrays in order to be able to use
  // std::set_difference()
  std::sort(instances_gr_array.begin(), instances_gr_array.end());
  std::sort(instances_md_array.begin(), instances_md_array.end());

  // Check if the instances_md list has more members than the instances_gr
  // lists. Meaning that an instance was removed from the GR group outside
  // of the AdminAPI
  std::set_difference(instances_md_array.begin(), instances_md_array.end(),
                      instances_gr_array.begin(), instances_gr_array.end(),
                      std::inserter(removed_members, removed_members.begin()));

  std::vector<MissingInstanceInfo> ret;
  for (auto i : removed_members) {
    shcore::sqlstring query(
        "SELECT mysql_server_uuid, instance_name, " \
        "JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) AS host " \
        "FROM mysql_innodb_cluster_metadata.instances " \
        "WHERE mysql_server_uuid = ?", 0);
    query << i;
    query.done();

    auto result = metadata->execute_sql(query);
    auto row = result->fetch_one();
    MissingInstanceInfo info;
    info.id = row->get_string(0);
    info.label = row->get_string(1);
    info.host = row->get_string(2);
    ret.push_back(info);
  }

  return ret;
}

/**
* Validate the given Cluster name.
* Cluster name must be non-empty and no greater than 40 characters long and
* start with an alphabetic or '_' character and just contain alphanumeric or
* the '_' character.
* @param name of the cluster
* @throws shcore::Exception if the name is not valid.
*/
void SHCORE_PUBLIC validate_cluster_name(const std::string &name) {
  bool valid = false;

  if (!name.empty()) {
    if (name.length() > 40) {
      throw shcore::Exception::argument_error(
        "The Cluster name can not be greater than 40 characters.");
    }

    std::locale locale;

    valid = std::isalpha(name[0], locale) || name[0] == '_';
    if (!valid) {
      throw shcore::Exception::argument_error(
        "The Cluster name can only start with an alphabetic or the '_' "
        "character.");
    }

    size_t index = 1;
    while (valid && index < name.size()) {
      valid = std::isalnum(name[index], locale) || name[index] == '_';
      if (!valid) {
        std::string msg =
          "The Cluster name can only contain alphanumerics or the '_' "
          "character."
          " Invalid character '";
        msg.append(&(name.at(index)), 1);
        msg.append("' found.");
        throw shcore::Exception::argument_error(msg);
      }
      index++;
    }
  } else {
    throw shcore::Exception::argument_error(
      "The Cluster name cannot be empty.");
  }

  return;
}

/**
* Validate the given label name.
* Cluster name must be non-empty and no greater than 256 characters long and
* start with an alphanumeric or '_' character and can only contain alphanumerics,
* Period ".", Underscore "_", Hyphen "-" or Colon ":" characters.
* @param name of the cluster
* @throws shcore::Exception if the name is not valid.
*/
void SHCORE_PUBLIC validate_label(const std::string &label) {
  bool valid = false;

  if (!label.empty()) {
    if (label.length() > 256) {
      throw shcore::Exception::argument_error(
        "The label can not be greater than 256 characters.");
    }

    std::locale locale;

    valid = std::isalnum(label[0], locale) || label[0] == '_';
    if (!valid) {
      throw shcore::Exception::argument_error(
        "The label can only start with an alphanumeric or the '_' "
        "character.");
    }

    size_t index = 1;
    while (valid && index < label.size()) {
      valid = std::isalnum(label[index], locale) || label[index] == '_' ||
        label[index] == '.' || label[index] == '-' || label[index] == ':';
      if (!valid) {
        std::string msg =
          "The label can only contain alphanumerics or the '_', '.', '-', "
          "':' characters. Invalid character '";
        msg.append(&(label.at(index)), 1);
        msg.append("' found.");
        throw shcore::Exception::argument_error(msg);
      }
      index++;
    }
  } else {
    throw shcore::Exception::argument_error("The label can not be empty.");
  }

  return;
}

/*
 * Gets Group Replication Group Name variable: group_replication_group_name
 *
 * @param session object which represents the session to the instance
 * @return string with the value of @@group_replication_group_name
 */
std::string get_gr_replicaset_group_name(
      std::shared_ptr<mysqlshdk::db::ISession>session) {
  std::string group_name;

  std::string query("SELECT @@group_replication_group_name");

  // Any error will bubble up right away
  auto result = session->query(query);
  auto row = result->fetch_one();
  if (row) {
    group_name = row->get_as_string(0);
  }
  return group_name;
}

/**
 * Validates if the current session instance 'group_replication_group_name'
 * differs from the one registered in the corresponding ReplicaSet table of the
 * Metadata
 *
 * @param session session to instance that is supposed to belong to a group
 * @param md_group_name name of the group the member belongs to according to
 *        metadata
 * @return a boolean value indicating whether the replicaSet
 * 'group_replication_group_name' is the same as the one registered in the
 * corresponding replicaset in the Metadata
 */
bool validate_replicaset_group_name(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &md_group_name) {
  std::string gr_group_name = get_gr_replicaset_group_name(session);

  log_info("Group Replication 'group_name' value: %s", gr_group_name.c_str());
  log_info("Metadata 'group_name' value: %s", md_group_name.c_str());

  // If the value is NULL it means the instance does not belong to any GR group
  // so it can be considered valid
  if (gr_group_name == "NULL")
    return true;

  return (gr_group_name == md_group_name);
}

/*
 * Check for the value of super-read-only and any open sessions
 * to the instance and raise an exception if super_read_only is turned on.
 *
 * @param session object which represents the session to the instance
 * @param clear_read_only boolean value used to confirm that
 * super_read_only must be disabled
 *
 * @return a boolean value indicating the status of super_read_only
 */
bool validate_super_read_only(std::shared_ptr<mysqlshdk::db::ISession>session,
                              bool clear_read_only) {
  int super_read_only = 0;

  get_server_variable(session, "super_read_only",
                      super_read_only, false);

  if (super_read_only) {
    if (clear_read_only) {
      auto session_address =
          session->uri(mysqlshdk::db::uri::formats::only_transport());
      log_info("Disabling super_read_only on the instance '%s'",
               session_address.c_str());
      set_global_variable(session, "super_read_only", "OFF");
    } else {
      auto session_options = session->get_connection_options();

      auto session_address =
        session_options.as_uri(mysqlshdk::db::uri::formats::only_transport());

      std::string error_msg;

      error_msg =
        "The MySQL instance at '" + session_address + "' currently has "
        "the super_read_only system variable set to protect it from "
        "inadvertent updates from applications. You must first unset it to be "
        "able to perform any changes to this instance. "
        "For more information see: https://dev.mysql.com/doc/refman/en/"
        "server-system-variables.html#sysvar_super_read_only. ";

      // Get the list of open session to the instance
      std::vector<std::pair<std::string, int>> open_sessions;
      open_sessions = get_open_sessions(session);

      if (!open_sessions.empty()) {
        error_msg +=
          "If you unset super_read_only you should consider closing the "
          "following: ";

        for (auto &value : open_sessions) {
          std::string account = value.first;
          int open_sessions = value.second;

          error_msg +=
            std::to_string(open_sessions) + " open session(s) of '" +
            account + "'. ";
        }
      }
      throw shcore::Exception::runtime_error(error_msg);
    }
  }
  return super_read_only;
}

/*
 * Checks if an instance can be rejoined to a given ReplicaSet.
 *
 * @param session Object which represents the session to the instance
 * @param metadata Metadata object which represents the session to the metadata
 * storage
 * @param rs_id The ReplicaSet id
 *
 * @return a boolean value which is true if the instance can be rejoined to the
 * ReplicaSet and false otherwise.
 */
bool validate_instance_rejoinable(
    std::shared_ptr<mysqlshdk::db::ISession> instance_session,
    const std::shared_ptr<MetadataStorage> &metadata,
    uint64_t rs_id) {
  std::string instance_uuid;
  // get server_uuid from the instance that we're trying to rejoin
  get_server_variable(instance_session, "server_uuid", instance_uuid);
  // get the list of missing instances
  std::vector<MissingInstanceInfo> missing_instances_list =
      get_unavailable_instances(metadata, rs_id);

  // Unless the instance is missing, we cannot add it back to the cluster
  auto it =
      std::find_if(missing_instances_list.begin(), missing_instances_list.end(),
                   [&instance_uuid](MissingInstanceInfo &info) {
                     return instance_uuid == info.id;
                   });
  // if the instance is not on the list of missing instances, then it cannot
  // be rejoined
  return it != std::end(missing_instances_list);
}

/**
 * Validate the hostname IP.
 *
 * A hostname might be resolved to an IP value not supported by the Group
 * Replication Communication Service (GCS), like 127.0.1.1. This function
 * validates if the hostname is resolved to a supported IP address, throwing
 * an exception if not supported.
 *
 * @param hostname string with the hostname to check.
 *
 * @throws std::runtime_error if the hostname is resolved to a non supported
 *         IP address.
 */
void validate_host_ip(const std::string &hostname) {
    std::string seed_ip =  mysqlshdk::utils::resolve_hostname_ipv4(hostname);
  // IP address '127.0.1.1' is not supported by GCS leading to errors.
  // NOTE: This IP is set by default in Debian platforms.
  if (seed_ip == "127.0.1.1")
    throw std::runtime_error(
        "Cannot perform operation using a connection with a hostname that "
        "resolves to '127.0.1.1', since it is not supported by the Group "
        "Replication communication layer. Change your system settings and/or "
        "connect to the instance using a hostname that resolves to a supported "
        "IP address (not 127.0.1.1).");
}
}  // namespace dba
}  // namespace mysqlsh
