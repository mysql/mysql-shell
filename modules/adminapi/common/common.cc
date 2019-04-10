/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/common.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/user_privileges.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {

namespace {
constexpr const char kSandboxDatadir[] = "sandboxdata";
constexpr uint16_t k_max_port = 65535;
}  // namespace

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
  if (!errors) return "mysqlprovision error";
  std::vector<std::string> str_errors;

  if (errors) {
    for (auto error : *errors) {
      auto data = error.as_map();
      auto error_type = data->get_string("type");
      auto error_text = data->get_string("msg");

      str_errors.push_back(error_type + ": " + error_text);
    }
  } else {
    str_errors.push_back(
        "Unexpected error calling mysqlprovision, "
        "see the shell log for more details");
  }

  return shcore::str_join(str_errors, "\n");
}

std::vector<std::string> convert_ipwhitelist_to_netmask(
    const std::vector<std::string> &ip_whitelist) {
  std::vector<std::string> ret;

  for (const std::string &value : ip_whitelist) {
    // Strip any blank chars from the ip_whitelist value and
    // Translate CIDR to netmask notation
    ret.push_back(
        mysqlshdk::utils::Net::cidr_to_netmask(shcore::str_strip(value)));
  }

  return ret;
}

std::vector<std::string> convert_ipwhitelist_to_netmask(
    const std::string &ip_whitelist) {
  return convert_ipwhitelist_to_netmask(
      shcore::str_split(shcore::str_strip(ip_whitelist), ",", -1));
}

bool is_group_replication_option_supported(
    const mysqlshdk::utils::Version &version, const std::string &option,
    const std::map<std::string, Option_availability> &options_map) {
  assert(options_map.find(option) != options_map.end());
  Option_availability opt_avail = options_map.at(option);
  if (version.get_major() == 8) {
    // 8.0 server
    // if only the 5.7 version was provided to the Option_availability struct,
    // then assume variable will also be supported in 8.0. This check is enough
    // since the default Version constructor sets the major version to 0.
    return version >= opt_avail.support_in_80;
  } else if (version.get_major() == 5 && version.get_minor() == 7) {
    // 5.7 server
    if (opt_avail.support_in_57.get_major() == 0) {
      // if the default constructor for version was used, no 5.7 version was
      // used on the Option_availability struct, meaning 5.7 is not supported
      return false;
    } else {
      return version >= opt_avail.support_in_57;
    }
  } else {
    throw std::runtime_error(
        "Unexpected version found for GR option support check: '" +
        version.get_full() + "'.");
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
              " WHERE grantee like ?",
              0)
          << ((shcore::sqlstring("?", 0) << admin_user).str() + "@%");

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

      if (out_hosts) out_hosts->push_back(host);
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
const std::set<std::string> k_global_privileges{"RELOAD",
                                                "SHUTDOWN",
                                                "PROCESS",
                                                "FILE",
                                                "SUPER",
                                                "REPLICATION SLAVE",
                                                "REPLICATION CLIENT",
                                                "CREATE USER"};

// Schema privileges needed on the metadata schema
const std::set<std::string> k_metadata_schema_privileges{
    "ALTER",
    "ALTER ROUTINE",
    "CREATE",
    "CREATE ROUTINE",
    "CREATE TEMPORARY TABLES",
    "CREATE VIEW",
    "DELETE",
    "DROP",
    "EVENT",
    "EXECUTE",
    "INDEX",
    "INSERT",
    "LOCK TABLES",
    "REFERENCES",
    "SELECT",
    "SHOW VIEW",
    "TRIGGER",
    "UPDATE"};

// Schema privileges needed on the mysql schema
const std::set<std::string> k_mysql_schema_privileges{"SELECT", "INSERT",
                                                      "UPDATE", "DELETE"};

// list of (schema, [privilege]) pairs, with the required privileges on each
// schema
const std::map<std::string, std::set<std::string>> k_schema_grants{
    {"mysql_innodb_cluster_metadata", k_metadata_schema_privileges},
    {"sys", {"SELECT"}},
    {"mysql", k_mysql_schema_privileges}  // need for mysql.plugin,
                                          // mysql.user others
};

// List of (schema, (table, [privilege])) triples, with the required privileges
// on each schema.table.
// Our code uses some additional performance_schema system variable tables
// (global_status, session_status, variables_info), but they do not require a
// specific privilege (enabled by default).
// See:
// https://dev.mysql.com/doc/refman/8.0/en/performance-schema-system-variable-tables.html
const std::map<std::string, std::map<std::string, std::set<std::string>>>
    k_table_grants{
        {"performance_schema",
         {
             {"replication_applier_configuration", {"SELECT"}},
             {"replication_applier_status", {"SELECT"}},
             {"replication_applier_status_by_coordinator", {"SELECT"}},
             {"replication_applier_status_by_worker", {"SELECT"}},
             {"replication_connection_configuration", {"SELECT"}},
             {"replication_connection_status", {"SELECT"}},
             {"replication_group_member_stats", {"SELECT"}},
             {"replication_group_members", {"SELECT"}},
             {"threads", {"SELECT"}},  // used in code
         }}};

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
        "\nMissing " + info + ": " + shcore::str_join(privileges, ", ") + ".";
  };

  log_info("Validating account %s@%s...", admin_user.c_str(),
           admin_host.c_str());

  mysqlshdk::mysql::User_privileges user_privileges{session, admin_user,
                                                    admin_host};

  // Check global privileges.
  auto global = user_privileges.validate(k_global_privileges);

  if (global.has_missing_privileges() || !global.has_grant_option()) {
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
      auto table = user_privileges.validate(
          table_grants.second, schema_grants.first, table_grants.first);

      if (table.has_missing_privileges()) {
        append_error_message("privileges on table '" + schema_grants.first +
                                 "." + table_grants.first + "'",
                             table.get_missing_privileges());
      }
    }
  }

  *validation_error = "The account " +
                      shcore::make_account(admin_user, admin_host) +
                      " is missing privileges required to manage an "
                      "InnoDB cluster:" +
                      *validation_error;

  return is_valid;
}

namespace {

std::string create_grant(const std::string &username,
                         const std::set<std::string> &privileges,
                         const std::string &schema = "*",
                         const std::string &object = "*") {
  return "GRANT " + shcore::str_join(privileges, ", ") + " ON " + schema + "." +
         object + " TO " + username + " WITH GRANT OPTION";
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
      grants.emplace_back(
          create_grant(username, table.second, schema.first, table.first));

  return grants;
}

}  // namespace

void create_cluster_admin_user(std::shared_ptr<mysqlshdk::db::ISession> session,
                               const std::string &username,
                               const std::string &password) {
  std::string user, host;
  shcore::split_account(username, &user, &host);

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
    session->executef("CREATE USER ?@? IDENTIFIED BY ?", user, host, password);

    // Give the grants
    for (const auto &grant : create_grants(username)) session->execute(grant);
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

  get_server_variable(session, "global.have_ssl", &have_ssl);

  // The instance supports SSL
  if (!shcore::str_casecmp(have_ssl.c_str(), "YES")) {
    // memberSslMode is DISABLED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "DISABLED")) {
      int require_secure_transport = 0;
      get_server_variable(session, "GLOBAL.require_secure_transport",
                          &require_secure_transport);

      if (require_secure_transport) {
        throw shcore::Exception::argument_error(
            "The instance "
            "'" +
            session->uri() +
            "' requires secure connections, to create the "
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
      throw shcore::Exception::argument_error(
          "The instance "
          "'" +
          session->uri() +
          "' does not have SSL enabled, to create the "
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
  get_server_variable(psession, "global.group_replication_ssl_mode",
                      &gr_ssl_mode);

  // The cluster REQUIRES SSL
  if (!shcore::str_casecmp(gr_ssl_mode.c_str(), "REQUIRED")) {
    // memberSslMode is DISABLED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "DISABLED"))
      throw shcore::Exception::runtime_error(
          "The cluster has SSL (encryption) enabled. "
          "To add the instance '" +
          session->uri() +
          "' to the cluster either "
          "disable SSL on the cluster, remove the memberSslMode option or use "
          "it with any of 'AUTO' or 'REQUIRED'.");

    // Now checks if SSL is actually supported on the instance
    std::string have_ssl;

    get_server_variable(session, "global.have_ssl", &have_ssl);

    // SSL is not supported on the instance
    if (shcore::str_casecmp(have_ssl.c_str(), "YES")) {
      throw shcore::Exception::runtime_error(
          "Instance '" + session->uri() +
          "' does not support SSL and cannot "
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
          "To add the instance '" +
          session->uri() +
          "' to the cluster either "
          "enable SSL on the cluster, remove the memberSslMode option or use "
          "it with any of 'AUTO' or 'DISABLED'.");

    // If the instance session requires SSL connections, then it can's
    // Join a cluster with SSL disabled
    int secure_transport_required = 0;
    get_server_variable(session, "global.require_secure_transport",
                        &secure_transport_required);
    if (secure_transport_required) {
      throw shcore::Exception::runtime_error(
          "The instance '" + session->uri() +
          "' is configured to require a "
          "secure transport but the cluster has SSL disabled. To add the "
          "instance to the cluster, either turn OFF the "
          "require_secure_transport option on the instance or enable SSL on "
          "the cluster.");
    }

    ret_val = dba::kMemberSSLModeDisabled;
  } else {
    // Only GR SSL mode "REQUIRED" and "DISABLED" are currently supported.
    throw shcore::Exception::runtime_error(
        "Unsupported Group Replication SSL Mode for the cluster: '" +
        gr_ssl_mode +
        "'. If the cluster was created using "
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
  shcore::sqlstring query(
      "SELECT mysql_server_uuid FROM "
      "mysql_innodb_cluster_metadata.instances "
      "WHERE replicaset_id = ?",
      0);
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

  // Get the metadata session version to determine how do we obtain each member
  // version
  auto version = metadata->get_session()->get_server_version();

  std::vector<NewInstanceInfo> ret;
  for (auto i : new_members) {
    shcore::sqlstring query;

    // from 8.0.3, member_version is available
    // NOTE: the ORDER BY is required to avoid results from being listed in a
    //       non-deterministic way (resulting in test issues for some
    //       platforms in direct mode).
    //       MEMBER_ID is used assuming that UUIDs of version 1 are always used
    //       but if this changes the different columns need to be used to sort
    //       the results.
    if (version >= mysqlshdk::utils::Version(8, 0, 3)) {
      query = shcore::sqlstring(
          "SELECT MEMBER_ID, MEMBER_HOST, MEMBER_PORT, MEMBER_VERSION "
          "FROM performance_schema.replication_group_members "
          "WHERE MEMBER_ID = ? "
          "ORDER BY MEMBER_ID",
          0);
    } else {
      query = shcore::sqlstring(
          "SELECT MEMBER_ID, MEMBER_HOST, MEMBER_PORT "
          "FROM performance_schema.replication_group_members "
          "WHERE MEMBER_ID = ? "
          "ORDER BY MEMBER_ID",
          0);
    }

    query << i;
    query.done();

    auto result = metadata->execute_sql(query);
    auto row = result->fetch_one();

    NewInstanceInfo info;
    info.member_id = row->get_string(0);
    info.host = row->get_string(1);
    info.port = row->get_int(2);

    if (version >= mysqlshdk::utils::Version(8, 0, 3)) {
      info.version = row->get_string(3);
    }

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
        "SELECT mysql_server_uuid, instance_name, "
        "JSON_UNQUOTE(JSON_EXTRACT(addresses, '$.mysqlClassic')) AS host "
        "FROM mysql_innodb_cluster_metadata.instances "
        "WHERE mysql_server_uuid = ?",
        0);
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
  if (!name.empty()) {
    bool valid = false;

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
 * start with an alphanumeric or '_' character and can only contain
 * alphanumerics, Period ".", Underscore "_", Hyphen "-" or Colon ":"
 * characters.
 * @param name of the cluster
 * @throws shcore::Exception if the name is not valid.
 */
void SHCORE_PUBLIC validate_label(const std::string &label) {
  if (!label.empty()) {
    bool valid = false;

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
    std::shared_ptr<mysqlshdk::db::ISession> session) {
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
  if (gr_group_name == "NULL") return true;

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
bool validate_super_read_only(std::shared_ptr<mysqlshdk::db::ISession> session,
                              bool clear_read_only) {
  int super_read_only = 0;
  // TODO(alfredo) - there are 4 slightly different copies of this snippet...
  get_server_variable(session, "super_read_only", &super_read_only, false);

  if (super_read_only) {
    auto console = mysqlsh::current_console();

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

      console->print_error(mysqlsh::fit_screen(
          "The MySQL instance at '" + session_address +
          "' currently has "
          "the super_read_only system variable set to protect it from "
          "inadvertent updates from applications. You must first unset it to "
          "be "
          "able to perform any changes to this instance.\n"
          "For more information see: https://dev.mysql.com/doc/refman/en/"
          "server-system-variables.html#sysvar_super_read_only."));

      // Get the list of open session to the instance
      std::vector<std::pair<std::string, int>> open_sessions;
      open_sessions = get_open_sessions(session);

      if (!open_sessions.empty()) {
        console->println(
            "If you unset super_read_only you should consider closing the "
            "following: ");

        for (auto &value : open_sessions) {
          std::string account = value.first;
          int open_sessions = value.second;

          console->println(std::to_string(open_sessions) +
                           " open session(s) of '" + account + "'. ");
        }
      }

      throw shcore::Exception::runtime_error("Server in SUPER_READ_ONLY mode");
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
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id) {
  std::string instance_uuid;
  // get server_uuid from the instance that we're trying to rejoin
  get_server_variable(instance_session, "server_uuid", &instance_uuid);
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
 * Checks if the target instance is a sandbox
 *
 * @param instance Object which represents the target instance
 * @param optional string to store the value of the cnfPath in case the instance
 * is a sandbox
 *
 * @return a boolean value which is true if the target instance is a sandbox or
 * false if is not.
 */
bool is_sandbox(const mysqlshdk::mysql::IInstance &instance,
                std::string *cnfPath) {
  int port = 0;
  std::string datadir;

  mysqlsh::dba::get_port_and_datadir(instance.get_session(), &port, &datadir);
  std::string path_separator = datadir.substr(datadir.size() - 1);
  auto path_elements = shcore::split_string(datadir, path_separator);

  // Removes the empty element at the end
  if (!datadir.empty() && (datadir[datadir.size() - 1] == '/' ||
                           datadir[datadir.size() - 1] == '\\')) {
    datadir.pop_back();
  }

  // Sandbox deployment structure is: <root_path>/<port>/sandboxdata
  // So we validate such structure to determine it is a sandbox
  if (datadir.length() < strlen(kSandboxDatadir) ||
      datadir.compare(datadir.length() - strlen(kSandboxDatadir),
                      strlen(kSandboxDatadir), kSandboxDatadir) != 0) {
    // Not a sandbox, we can immediately return false
    return false;
  } else {
    // Check if the my.cnf file is present
    if (path_elements[path_elements.size() - 3] == std::to_string(port)) {
      path_elements[path_elements.size() - 2] = "my.cnf";
      std::string tmpPath = shcore::str_join(path_elements, path_separator);

      // Remove the trailing path_separator
      if (tmpPath.back() == path_separator[0]) tmpPath.pop_back();

      if (shcore::is_file(tmpPath)) {
        if (cnfPath) {
          *cnfPath = tmpPath;
        }
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
}

// AdminAPI interactive handling specific methods

/*
 * Resolves the .cnf file path
 *
 * @param instance Instance object which represents the target instance
 *
 * @return the target instance resolved .cnf file path
 */
std::string prompt_cnf_path(const mysqlshdk::mysql::IInstance &instance) {
  // Path is not given, let's try to autodetect it
  std::string cnfPath;
  // Try to locate the .cnf file path
  // based on the OS and the default my.cnf path as configured
  // by our official MySQL packages

  // Detect the OS
  auto console = mysqlsh::current_console();

  console->println();
  console->println("Detecting the configuration file...");
  std::vector<std::string> config_paths, default_paths;

  if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 0)) {
    // use the performance schema table to try and find the existing
    // configuration files
    auto session = instance.get_session();
    auto result = session->query(
        "select DISTINCT VARIABLE_PATH from performance_schema.variables_info "
        "WHERE VARIABLE_PATH <> ''");
    auto row = result->fetch_one();
    while (row) {
      config_paths.emplace_back(row->get_string(0));
      row = result->fetch_one();
    }
  }
  // if no config files were found, try to look in the default paths
  if (config_paths.empty()) {
    shcore::OperatingSystem os = shcore::get_os_type();
    auto default_paths = mysqlshdk::config::get_default_config_paths(os);
  }
  // Iterate the config_paths found in the instance checking if the user wants
  // to modify any of them
  for (const auto &value : config_paths) {
    // Prompt the user to validate if shall use it or not
    console->println("Found configuration file being used by instance '" +
                     instance.get_connection_options().uri_endpoint() +
                     "' at location: " + value);

    if (console->confirm("Do you want to modify this file?") ==
        Prompt_answer::YES) {
      cnfPath = value;
      break;
    }
  }
  if (cnfPath.empty()) {
    // Iterate the default_paths to check if the files exist and if so,
    // set cnfPath
    for (const auto &value : default_paths) {
      if (shcore::is_file(value)) {
        // Prompt the user to validate if shall use it or not
        console->println("Found configuration file at standard location: " +
                         value);

        if (console->confirm("Do you want to modify this file?") ==
            Prompt_answer::YES) {
          cnfPath = value;
          break;
        }
      }
    }
  }

  if (cnfPath.empty()) {
    console->println("Default file not found at the standard locations.");

    bool done = false;
    std::string tmpPath;

    while (!done && console->prompt("Please specify the path to the MySQL "
                                    "configuration file: ",
                                    &tmpPath)) {
      if (tmpPath.empty()) {
        done = true;
      } else {
        if (shcore::is_file(tmpPath)) {
          cnfPath = tmpPath;
          done = true;
        } else {
          console->println(
              "The given path to the MySQL configuration file is invalid.");
          console->println();
        }
      }
    }
  }

  return cnfPath;
}

/*
 * Prompts new account password and prompts for confirmation of the password
 *
 * @return a string with the password introduced by the user
 */
std::string prompt_new_account_password() {
  std::string password1;
  std::string password2;

  auto console = mysqlsh::current_console();

  for (;;) {
    if (shcore::Prompt_result::Ok ==
        console->prompt_password("Password for new account: ", &password1)) {
      if (shcore::Prompt_result::Ok ==
          console->prompt_password("Confirm password: ", &password2)) {
        if (password1 != password2) {
          console->println("Passwords don't match, please try again.");
          continue;
        }
      }
    }
    break;
  }
  return password1;
}

/*
 * Generates a prompt with several options
 *
 * @param options a vector of strings containing the optional selections
 * @param defopt the default option
 *
 * @return an integer representing the chosen option
 */
int prompt_menu(const std::vector<std::string> &options, int defopt) {
  int i = 0;
  auto console = mysqlsh::current_console();
  for (const auto &opt : options) {
    console->println(std::to_string(++i) + ") " + opt);
  }
  for (;;) {
    std::string result;
    if (defopt > 0) {
      console->println();
      if (!console->prompt(
              "Please select an option [" + std::to_string(defopt) + "]: ",
              &result))
        return 0;
    } else {
      console->println();
      if (!console->prompt("Please select an option: ", &result)) return 0;
    }
    // Note that menu options start at 1, not 0 since that's what users will
    // input
    if (result.empty() && defopt > 0) return defopt;
    std::stringstream ss(result);
    ss >> i;
    if (i <= 0 || i > static_cast<int>(options.size())) continue;
    return i;
  }
}

/*
 * Check cluster admin restrictions
 *
 * @param instance Instance object which represents the target instance
 * @param user the username
 * @param host the hostname
 * @param interactive boolean indicating if interactive mode is used.
 *
 * @return a boolean value indicating whether the account has enough privileges
 * or not
 */
bool check_admin_account_access_restrictions(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::string &host, bool interactive) {
  int n_wildcard_accounts, n_non_wildcard_accounts;
  std::vector<std::string> hosts;

  auto session = instance.get_session();
  std::tie(n_wildcard_accounts, n_non_wildcard_accounts) =
      mysqlsh::dba::find_cluster_admin_accounts(session, user, &hosts);

  // there are multiple accounts defined with the same username
  // we assume that the user knows what they're doing and skip the rest
  if (n_wildcard_accounts + n_non_wildcard_accounts > 1) {
    log_info("%i accounts named %s found. Skipping remaining checks",
             n_wildcard_accounts + n_non_wildcard_accounts, user.c_str());
    return true;
  } else {
    auto console = mysqlsh::current_console();

    if (hosts.size() == 1 && n_wildcard_accounts == 0) {
      console->println();
      std::string err_msg =
          "User '" + user + "' can only connect from '" + hosts[0] + "'.";
      std::string msg =
          "New account(s) with proper source address specification to allow "
          "remote connection from all instances must be created to manage the "
          "cluster.";
      if (!interactive) {
        console->print_error(msg);
        throw std::runtime_error(err_msg);
      } else {
        console->print_error(err_msg + " " + msg);
      }
      return false;
    } else {
      auto hiter =
          std::find_if(hosts.begin(), hosts.end(), [](const std::string &a) {
            return a.find('%') != std::string::npos;
          });
      // there is only one account with a wildcard, so validate it
      if (n_wildcard_accounts == 1 && hiter != hosts.end()) {
        std::string whost = *hiter;
        // don't need to check privs if this is the account in use, since
        // that will already be validated before
        if (whost != host) {
          std::string error_msg;
          if (mysqlsh::dba::validate_cluster_admin_user_privileges(
                  session, user, whost, &error_msg)) {
            log_info(
                "Account %s@%s has required privileges for cluster management",
                user.c_str(), whost.c_str());
            // account accepted
            return true;
          } else {
            console->print_error(error_msg);
            if (!interactive) {
              throw shcore::Exception::runtime_error(
                  "The account " + shcore::make_account(user, whost) +
                  " is missing privileges required to manage an InnoDB "
                  "cluster.");
            }
            return false;
          }
        }
      }
    }
    // accept other cases by default
    return true;
  }
}

namespace {
bool prompt_full_account(std::string *out_account) {
  bool cancelled = false;
  auto console = mysqlsh::current_console();
  for (;;) {
    std::string create_user;

    if (console->prompt("Account Name: ", &create_user) &&
        !create_user.empty()) {
      try {
        // normalize the account name
        if (std::count(create_user.begin(), create_user.end(), '@') <= 1 &&
            create_user.find(' ') == std::string::npos &&
            create_user.find('\'') == std::string::npos &&
            create_user.find('"') == std::string::npos &&
            create_user.find('\\') == std::string::npos) {
          auto p = create_user.find('@');
          if (p == std::string::npos)
            create_user = shcore::make_account(create_user, "%");
          else
            create_user = shcore::make_account(create_user.substr(0, p),
                                               create_user.substr(p + 1));
        }
        // validate
        shcore::split_account(create_user, nullptr, nullptr, false);

        if (out_account) *out_account = create_user;
        break;
      } catch (std::runtime_error &) {
        console->println(
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
  return !mysqlsh::current_console()->prompt("Account Host: ", out_host) ||
         out_host->empty();
}

}  // namespace

/*
 * Prompts options to create a valid admin account
 *
 * @param user the username
 * @param host the hostname
 * @param out_create_account the resolved account to be created
 *
 * @return a boolean value indicating whether the account has enough privileges
 * (or was resolved), or not.
 */
bool prompt_create_usable_admin_account(const std::string &user,
                                        const std::string &host,
                                        std::string *out_create_account) {
  assert(out_create_account);
  int result;
  result = prompt_menu({"Create remotely usable account for '" + user +
                            "' with same grants and password",
                        "Create a new admin account for InnoDB "
                        "cluster with minimal required grants",
                        "Ignore and continue", "Cancel"},
                       1);
  bool cancelled;
  auto console = mysqlsh::current_console();

  switch (result) {
    case 1:
      console->println(
          "Please provide a source address filter for the account (e.g: "
          "192.168.% or % etc) or leave empty and press Enter to cancel.");
      cancelled = prompt_account_host(out_create_account);
      if (!cancelled) {
        *out_create_account = shcore::make_account(user, *out_create_account);
      }
      break;
    case 2:
      console->println(
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
      console->println("Canceling...");
      return false;
  }

  if (cancelled) {
    console->println("Canceling...");
    return false;
  }
  return true;
}

/*
 * Check if super_read_only is enable and prompts the user to act on it:
 * disable it or not.
 *
 * @param instance Instance object which represents the target instance
 * @param throw_on_error boolean value to indicate if an exception shall be
 * thrown or nor in case of an error
 *
 * @return a boolean value indicating the result of the operation: canceled or
 * not
 */
bool prompt_super_read_only(const mysqlshdk::mysql::IInstance &instance,
                            bool throw_on_error) {
  auto session = instance.get_session();
  auto options_session = session->get_connection_options();
  auto active_session_address =
      options_session.as_uri(mysqlshdk::db::uri::formats::only_transport());

  // Get the status of super_read_only in order to verify if we need to
  // prompt the user to disable it
  mysqlshdk::utils::nullable<bool> super_read_only = instance.get_sysvar_bool(
      "super_read_only", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // If super_read_only is not null and enabled
  if (*super_read_only) {
    auto console = mysqlsh::current_console();
    console->print_info(mysqlsh::fit_screen(
        "The MySQL instance at '" + active_session_address +
        "' currently has the super_read_only system variable set to "
        "protect it from inadvertent updates from applications. "
        "You must first unset it to be able to perform any changes "
        "to this instance.\n"
        "For more information see: https://dev.mysql.com/doc/refman/"
        "en/server-system-variables.html#sysvar_super_read_only."));
    console->println();

    // Get the list of open session to the instance
    std::vector<std::pair<std::string, int>> open_sessions;
    open_sessions = mysqlsh::dba::get_open_sessions(session);

    if (!open_sessions.empty()) {
      console->print_note(
          "There are open sessions to '" + active_session_address +
          "'.\n"
          "You may want to kill these sessions to prevent them from "
          "performing unexpected updates: \n");

      for (auto &value : open_sessions) {
        std::string account = value.first;
        int open_sessions = value.second;

        console->println("" + std::to_string(open_sessions) +
                         " open session(s) of "
                         "'" +
                         account + "'. \n");
      }
    }

    if (console->confirm("Do you want to disable super_read_only and continue?",
                         Prompt_answer::NO) == Prompt_answer::NO) {
      console->println();

      if (throw_on_error)
        throw shcore::cancelled("Cancelled");
      else
        console->println("Cancelled");

      return false;
    } else {
      return true;
    }
  }
  return true;
}

/*
 * Prints a table with configurable contents
 *
 * @param column_names the names of the table columns
 * @param column_labels the labels for the columns
 * @param documents a Value::Array with the table contents
 */
void dump_table(const std::vector<std::string> &column_names,
                const std::vector<std::string> &column_labels,
                shcore::Value::Array_type_ref documents) {
  std::vector<uint64_t> max_lengths;

  size_t field_count = column_names.size();

  // Updates the max_length array with the maximum length between column name,
  // min column length and column max length
  for (size_t field_index = 0; field_index < field_count; field_index++) {
    max_lengths.push_back(0);
    max_lengths[field_index] = std::max<uint64_t>(
        max_lengths[field_index], column_labels[field_index].size());
  }

  // Now updates the length with the real column data lengths
  if (documents) {
    for (auto map : *documents) {
      auto document = map.as_map();
      for (size_t field_index = 0; field_index < field_count; field_index++)
        max_lengths[field_index] = std::max<uint64_t>(
            max_lengths[field_index],
            document->get_string(column_names[field_index]).size());
    }
  }

  //-----------

  size_t index = 0;
  std::vector<std::string> formats(field_count, "%-");

  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for (index = 0; index < field_count; index++) {
    // Creates the format string to print each field
    formats[index].append(std::to_string(max_lengths[index]));
    if (index == field_count - 1)
      formats[index].append("s |");
    else
      formats[index].append("s | ");

    std::string field_separator(max_lengths[index] + 2, '-');
    field_separator.append("+");
    separator.append(field_separator);
  }
  separator.append("\n");

  // Prints the initial separator line and the column headers
  auto console = mysqlsh::current_console();
  console->print(separator);
  console->print("| ");

  for (index = 0; index < field_count; index++) {
    std::string data = shcore::str_format(formats[index].c_str(),
                                          column_labels[index].c_str());
    console->print(data.c_str());

    // Once the header is printed, updates the numeric fields formats
    // so they are right aligned
    // if (numerics[index])
    //  formats[index] = formats[index].replace(1, 1, "");
  }
  console->println();
  console->print(separator);

  if (documents) {
    // Now prints the records
    for (auto map : *documents) {
      auto document = map.as_map();

      console->print("| ");

      for (size_t field_index = 0; field_index < field_count; field_index++) {
        std::string raw_value = document->get_string(column_names[field_index]);
        std::string data =
            shcore::str_format(formats[field_index].c_str(), raw_value.c_str());

        console->print(data.c_str());
      }
      console->println();
    }
  }

  console->println(separator);
}

/*
 * Gets the configure instance action returned from MP
 *
 * @param opt_map map of the "config_errors" JSON dictionary retured by MP:
 * result->get_array("config_errors");
 *
 * @return an enum object of ConfigureInstanceAction with the action to be taken
 */
ConfigureInstanceAction get_configure_instance_action(
    const shcore::Value::Map_type &opt_map) {
  std::string action = opt_map.get_string("action");
  ConfigureInstanceAction ret_val;

  if (action == "server_update+config_update")
    ret_val = ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_DYNAMIC;
  else if (action == "config_update+restart")
    ret_val = ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_STATIC;
  else if (action == "config_update")
    ret_val = ConfigureInstanceAction::UPDATE_CONFIG;
  else if (action == "server_update")
    ret_val = ConfigureInstanceAction::UPDATE_SERVER_DYNAMIC;
  else if (action == "restart")
    ret_val = ConfigureInstanceAction::UPDATE_SERVER_STATIC;
  else
    ret_val = ConfigureInstanceAction::UNDEFINED;

  return ret_val;
}

/*
 * Presents the required configuration changes and prompts the user for
 * confirmation
 *
 * @param result the return value of the MP operation
 * @param print_note a boolean value indicating if the column 'note' should be
 * presented or not
 */
void print_validation_results(const shcore::Value::Map_type_ref &result,
                              bool print_note) {
  auto console = mysqlsh::current_console();

  if (result->has_key("config_errors")) {
    console->print_note("Some configuration options need to be fixed:");

    auto config_errors = result->get_array("config_errors");

    if (print_note) {
      for (auto option : *config_errors) {
        auto opt_map = option.as_map();
        ConfigureInstanceAction action =
            get_configure_instance_action(*opt_map);
        std::string note;
        switch (action) {
          case ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_DYNAMIC: {
            note = "Update the server variable and the config file";
            break;
          }
          case ConfigureInstanceAction::UPDATE_SERVER_AND_CONFIG_STATIC: {
            note = "Update the config file and restart the server";
            break;
          }
          case ConfigureInstanceAction::UPDATE_CONFIG: {
            note = "Update the config file";
            break;
          }
          case ConfigureInstanceAction::UPDATE_SERVER_DYNAMIC: {
            note = "Update the server variable";
            break;
          }
          case ConfigureInstanceAction::UPDATE_SERVER_STATIC: {
            note = "Update read-only variable and restart the server";
            break;
          }
          default: {
            note = "Undefined action";
            break;
          }
        }
        (*opt_map)["note"] = shcore::Value(note);
      }

      dump_table({"option", "current", "required", "note"},
                 {"Variable", "Current Value", "Required Value", "Note"},
                 config_errors);
    } else {
      dump_table({"option", "current", "required"},
                 {"Variable", "Current Value", "Required Value"},
                 config_errors);
    }

    for (auto option : *config_errors) {
      auto opt_map = option.as_map();
      opt_map->erase("note");
    }
  }
}

std::string get_canonical_instance_address(
    std::shared_ptr<mysqlshdk::db::ISession> session) {
  auto result = session->query("SELECT coalesce(@@report_host, @@hostname)");
  if (auto row = result->fetch_one()) {
    return row->get_string(0);
  }
  throw std::logic_error("Unable to query instance address");
}

void validate_connection_options(
    const Connection_options &options,
    std::function<shcore::Exception(const std::string &)> factory) {
  auto throw_exception = [&options, &factory](const std::string &error) {
    throw factory(
        "Connection '" +
        options.as_uri(mysqlshdk::db::uri::formats::user_transport()) +
        "' is not valid: " + error + ".");
  };

  if (options.has_transport_type() &&
      options.get_transport_type() != mysqlshdk::db::Tcp) {
    // TODO(ak) this restriction should be lifted and sockets allowed
    throw_exception(
        "a MySQL session through TCP/IP is required to perform "
        "this operation");
  }

  if (options.has_host()) {
    // host has to be an IPv4 address or resolve to an IPv4 address
    const std::string &host = options.get_host();

    if (mysqlshdk::utils::Net::is_ipv6(host))
      throw_exception(
          host +
          " is an IPv6 address, which is not supported by "
          "the Group Replication. Please ensure an IPv4 address is used when "
          "setting up an InnoDB cluster");

    try {
      mysqlshdk::utils::Net::resolve_hostname_ipv4(host);
    } catch (mysqlshdk::utils::net_error &error) {
      throw_exception("unable to resolve the IPv4 address");
    }
  }
}

std::string get_report_host_address(
    const mysqlshdk::db::Connection_options &cnx_opts,
    const mysqlshdk::db::Connection_options &group_cnx_opts) {
  // Set login credentials to connect to instance if needed.
  // NOTE: It is assumed that the same login credentials can be used to
  //       connect to all cluster instances.
  auto target_cnx_opts = cnx_opts;
  if (!target_cnx_opts.has_user()) {
    target_cnx_opts.set_login_options_from(group_cnx_opts);
  }

  std::string instance_address =
      target_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());

  // Connect to instance to try to get report_host value.
  std::string md_address;
  log_debug("Connecting to instance '%s' to determine report host.",
            instance_address.c_str());
  try {
    std::shared_ptr<mysqlshdk::db::ISession> session =
        mysqlshdk::db::mysql::Session::create();
    session->connect(target_cnx_opts);
    mysqlshdk::mysql::Instance target_instance(session);
    log_debug("Successfully connected to instance");

    // Get the instance report host value.
    md_address = mysqlshdk::mysql::get_report_host(target_instance) + ":" +
                 std::to_string(target_cnx_opts.get_port());

    session->close();
    log_debug("Closed connection to the instance '%s'.",
              instance_address.c_str());
  } catch (std::exception &err) {
    log_debug("Failed to connect to instance '%s': %s",
              instance_address.c_str(), err.what());
  }

  return (!md_address.empty()) ? md_address : instance_address;
}

std::unique_ptr<mysqlshdk::config::Config> create_server_config(
    mysqlshdk::mysql::IInstance *instance,
    const std::string &srv_cfg_handler_name, bool silent) {
  auto cfg = shcore::make_unique<mysqlshdk::config::Config>();

  // Get the capabilities to use set persist by the server.
  mysqlshdk::utils::nullable<bool> can_set_persist =
      instance->is_set_persist_supported();

  // Add server configuration handler depending on SET PERSIST support.
  cfg->add_handler(
      srv_cfg_handler_name,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          shcore::make_unique<mysqlshdk::config::Config_server_handler>(
              instance, (!can_set_persist.is_null() && *can_set_persist)
                            ? mysqlshdk::mysql::Var_qualifier::PERSIST
                            : mysqlshdk::mysql::Var_qualifier::GLOBAL)));

  if (!silent) {
    if (can_set_persist.is_null()) {
      auto console = mysqlsh::current_console();

      std::string warn_msg =
          "Instance '" + instance->descr() +
          "' cannot persist Group Replication configuration since MySQL "
          "version " +
          instance->get_version().get_base() +
          " does not support the SET PERSIST command (MySQL version >= 8.0.11 "
          "required). Please use the <Dba>.<<<configureLocalInstance>>>"
          "() command locally to persist the changes.";
      console->print_warning(warn_msg);
    } else if (*can_set_persist == false) {
      auto console = mysqlsh::current_console();

      std::string warn_msg =
          "Instance '" + instance->descr() +
          "' will not load the persisted cluster configuration upon reboot "
          "since "
          "'persisted-globals-load' is set to 'OFF'. Please use the <Dba>."
          "<<<configureLocalInstance>>>() command locally to persist the "
          "changes "
          "or set 'persisted-globals-load' to 'ON' on the configuration file.";
      console->print_warning(warn_msg);
    }
  }

  return cfg;
}

void add_config_file_handler(mysqlshdk::config::Config *cfg,
                             const std::string handler_name,
                             const std::string &mycnf_path,
                             const std::string &output_mycnf_path) {
  // Add configuration handle to update option file (if provided).
  if (mycnf_path.empty() && output_mycnf_path.empty()) {
    throw std::logic_error("No option file path was provided.");
  }
  if (output_mycnf_path.empty()) {
    // Read and update mycnf.
    cfg->add_handler(handler_name,
                     std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                         new mysqlshdk::config::Config_file_handler(
                             mycnf_path, mycnf_path)));
  } else if (mycnf_path.empty()) {
    // Update output_mycnf (creating it if needed).
    cfg->add_handler(
        handler_name,
        std::unique_ptr<mysqlshdk::config::IConfig_handler>(
            new mysqlshdk::config::Config_file_handler(output_mycnf_path)));
  } else {
    // Read from mycnf but update output_mycnf (creating it if needed).
    cfg->add_handler(handler_name,
                     std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                         new mysqlshdk::config::Config_file_handler(
                             mycnf_path, output_mycnf_path)));
  }
}

std::string resolve_gr_local_address(
    const mysqlshdk::utils::nullable<std::string> &local_address,
    const std::string &report_host, int port) {
  assert(!report_host.empty());  // First we need to get the report host.

  std::string local_host, str_local_port;
  int local_port;

  if (local_address.is_null() || local_address->empty()) {
    // No local address specified, use instance host and port * 10 + 1.
    local_host = report_host;
    local_port = port * 10 + 1;
  } else if (local_address->front() == '[' && local_address->back() == ']') {
    // It is an IPV6 address (no port specified), since it is surrounded by [].
    // NOTE: Must handle this situation first to avoid splitting IPv6 addresses
    //      in the middle next.
    local_host = *local_address;
    local_port = port * 10 + 1;
  } else {
    // Parse the given address host:port (both parts are optional).
    // Note: Get the last ':' in case a IPv6 address is used, e.g. [::1]:123.
    size_t pos = local_address->find_last_of(":");
    if (pos != std::string::npos) {
      // Local address with ':' separating host and port.
      local_host = local_address->substr(0, pos);
      str_local_port = local_address->substr(pos + 1, local_address->length());

      // No host part, use instance report host.
      if (local_host.empty()) {
        local_host = report_host;
      }

      // No port part, then use instance port * 10 +1.
      if (str_local_port.empty()) {
        local_port = port * 10 + 1;
      } else {
        // Convert port string value to int
        try {
          local_port = std::stoi(str_local_port);
        } catch (const std::exception &) {
          // Error if the port cannot be converted to an integer (not valid).
          throw shcore::Exception::argument_error(
              "Invalid port '" + str_local_port +
              "' for localAddress option. The port must be an integer between "
              "1 and 65535.");
        }

        // The specified port must have a valid range.
        if (local_port <= 0 || local_port > k_max_port) {
          throw shcore::Exception::argument_error(
              "Invalid port '" + str_local_port +
              "' for localAddress option. The port must be an integer between "
              "1 and 65535.");
        }
      }
    } else {
      // No separator found ':'.
      // First, if the value only has digits assume it is a port.
      if (std::all_of(local_address->cbegin(), local_address->cend(),
                      ::isdigit)) {
        local_port = std::stoi(*local_address);
        local_host = report_host;

        // The specified port must have a valid range.
        if (local_port <= 0 || local_port > k_max_port) {
          throw shcore::Exception::argument_error(
              "Invalid port '" + str_local_port +
              "' for localAddress option. The port must be an integer between "
              "1 and 65535.");
        }
      } else {
        // Otherwise, assume only the host part was provided.
        local_host = *local_address;
        local_port = port * 10 + 1;
      }
    }
  }

  // Check the port value, if out of range then issue an error.
  // NOTE: This error may only be issued if the port was automatically assigned
  //       based on instance port (when specified by the user an error is
  //       raised before reach this part of the code).
  if (local_port <= 0 || local_port > k_max_port) {
    throw shcore::Exception::argument_error(
        "Invalid port '" + std::to_string(local_port) +
        "' used by default for localAddress option. The port must be an "
        "integer between 1 and 65535.");
  }

  // Verify if the port is already in use.
  bool port_busy = false;
  try {
    port_busy =
        mysqlshdk::utils::Net::is_port_listening(local_host, local_port);
  } catch (...) {
    // Ignore any error while checking if the port is busy, let GR try later and
    // possibly fail (e.g., if a wrong host is used).
  }
  if (port_busy) {
    throw std::runtime_error(
        "The port '" + std::to_string(local_port) +
        "' for localAddress option is already in use. Specify an "
        "available port to be used with localAddress option or free port '" +
        std::to_string(local_port) + "'.");
  }

  // Return the final local address value to use for GR.
  return local_host + ":" + std::to_string(local_port);
}

}  // namespace dba
}  // namespace mysqlsh
