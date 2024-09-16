/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/server_features.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "utils/version.h"

namespace mysqlsh {
namespace dba {

/** Count how many accounts there are with the given username, excluding
  localhost

  This allows us to know whether there's a wildcarded account or an account with
  netmask that the user may have created previously which we can use for
  management or whether the user has created multiple accounts, which would mean
  that they must know what they're doing and also that we can't tell which of
  these accounts should be validated.

  @return # of wildcarded accounts + netmask accounts,  # of other accounts
  */
std::pair<int, int> find_cluster_admin_accounts(
    const mysqlshdk::mysql::IInstance &instance, const std::string &admin_user,
    std::vector<std::string> *out_hosts) {
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
  auto result = instance.query(query);
  if (result) {
    auto row = result->fetch_one();
    while (row) {
      std::string account = row->get_string(0);
      std::string user, host;
      shcore::split_account(account, &user, &host,
                            shcore::Account::Auto_quote::HOST);
      assert(user == admin_user);

      if (out_hosts) out_hosts->push_back(host);
      // ignore localhost accounts
      if (host == "localhost" || host == "127.0.0.1") {
        local++;
      } else {
        if (host.find('%') != std::string::npos) {
          w++;
        } else {
          // If we find the '/' check if the thing to the left
          // is an ipv4, if it is, we assume we are using a netmask
          auto pos = host.find('/');
          if (pos != std::string::npos &&
              mysqlshdk::utils::Net::is_ipv4(host.substr(0, pos))) {
            w++;
          } else {
            nw++;
          }
        }
      }
      row = result->fetch_one();
    }
  }
  log_info("%s user has %i accounts with wildcard or netmask and %i without",
           admin_user.c_str(), w, nw + local);

  return std::make_pair(w, nw);
}

namespace {
using mysqlshdk::utils::Version;

/*
 * Validate if the user specified for being the cluster admin has all of the
 * requirements:
 * 1) has a host wildcard other than 'localhost' or '127.0.0.1' or the one
 *    specified
 * 2) has the necessary privileges assigned
 */

struct Privilege_versions {
  std::string name;  // Privilege name
  Version minimum;   // Minimum version supported
  Version maximum;   // Maximum version supported
};

// Global privs needed for managing cluster instances
// BUG#29743910: clusterAdmin needs SELECT on *.* for tables compliance check.
// BUG#30339460: SYSTEM_VARIABLES_ADMIN and PERSIST_RO_VARIABLES_ADMIN
//               privileges needed to change Global system variables.
const std::vector<Privilege_versions> k_admin_global_privileges{
    {"RELOAD", Version(), Version()},
    {"SHUTDOWN", Version(), Version()},
    {"PROCESS", Version(), Version()},
    {"FILE", Version(), Version()},
    {"SELECT", Version(), Version()},
    {"REPLICATION SLAVE", Version(), Version()},
    {"REPLICATION CLIENT", Version(), Version()},
    {"CREATE USER", Version(), Version()},
    {"SYSTEM_VARIABLES_ADMIN", Version(8, 0, 0), Version()},
    {"PERSIST_RO_VARIABLES_ADMIN", Version(8, 0, 0), Version()},
    {"REPLICATION_APPLIER", Version(8, 0, 18), Version()},
    {"EXECUTE", k_mysql_clone_plugin_initial_version, Version()},
    {"CLONE_ADMIN", k_mysql_clone_plugin_initial_version, Version()},
    {"REPLICATION_SLAVE_ADMIN", Version(8, 0, 0), Version()},
    {"GROUP_REPLICATION_ADMIN", Version(8, 0, 0), Version()},
    {"ROLE_ADMIN", Version(8, 0, 0), Version()},
    {"CONNECTION_ADMIN", Version(8, 0, 0), Version()}};

// Schema privileges needed on the metadata schema
// NOTE: SELECT *.* required (BUG#29743910), thus no longer needed here.
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
    "SHOW VIEW",
    "TRIGGER",
    "UPDATE"};

// Schema privileges needed on the mysql schema
// NOTE: SELECT *.* required (BUG#29743910), thus no longer needed here.
const std::set<std::string> k_mysql_schema_privileges{"INSERT", "UPDATE",
                                                      "DELETE"};

// list of (schema, [privilege]) pairs, with the required privileges on each
// schema
// NOTE: SELECT *.* required (BUG#29743910), thus no longer needed for 'sys'.
const std::map<std::string, std::set<std::string>> k_admin_schema_grants{
    {"mysql_innodb_cluster_metadata", k_metadata_schema_privileges},
    {"mysql_innodb_cluster_metadata_bkp", k_metadata_schema_privileges},
    {"mysql_innodb_cluster_metadata_previous", k_metadata_schema_privileges},
    {"mysql", k_mysql_schema_privileges}  // need for mysql.plugin,
                                          // mysql.user others
};

// List of (schema, (table, [privilege])) triples, with the required privileges
// on each schema.table.
// Our code uses some additional performance_schema system variable tables
// (global_status, session_status, variables_info), but they do not require a
// specific privilege (enabled by default).
// See:
// https://dev.mysql.com/doc/refman/en/performance-schema-system-variable-tables.html
const std::map<std::string, std::map<std::string, std::set<std::string>>>
    k_admin_table_grants{};

// NOTE: SELECT *.* required (BUG#29743910), thus the following SELECT
//       privileges on specific tables are no longer required:
//    k_table_grants{
//        {"performance_schema",
//         {
//             {"replication_applier_configuration", {"SELECT"}},
//             {"replication_applier_status", {"SELECT"}},
//             {"replication_applier_status_by_coordinator", {"SELECT"}},
//             {"replication_applier_status_by_worker", {"SELECT"}},
//             {"replication_connection_configuration", {"SELECT"}},
//             {"replication_connection_status", {"SELECT"}},
//             {"replication_group_member_stats", {"SELECT"}},
//             {"replication_group_members", {"SELECT"}},
//             {"threads", {"SELECT"}},  // used in code
//         }}};

std::set<std::string> global_admin_privileges_for_server_version(
    const Version &version) {
  std::set<std::string> privs;

  for (const auto &priv : k_admin_global_privileges) {
    if ((!priv.minimum && !priv.maximum) ||
        ((!priv.minimum || version >= priv.minimum) &&
         (!priv.maximum || version <= priv.maximum))) {
      privs.insert(priv.name);
    }
  }
  return privs;
}

const std::map<std::string, std::set<std::string>> k_router_schema_grants{
    {"mysql_innodb_cluster_metadata", {"SELECT", "EXECUTE"}},
};

const std::map<std::string, std::map<std::string, std::set<std::string>>>
    k_router_table_grants{{"performance_schema",
                           {{"replication_group_members", {"SELECT"}},
                            {"replication_group_member_stats", {"SELECT"}},
                            {"global_variables", {"SELECT"}}}},
                          {"mysql_innodb_cluster_metadata",
                           {
                               {"routers", {"INSERT", "UPDATE", "DELETE"}},
                               {"v2_routers", {"INSERT", "UPDATE", "DELETE"}},
                               {"clusters", {"UPDATE"}},
                               {"clustersets", {"UPDATE"}},
                               {"v2_gr_clusters", {"UPDATE"}},
                               {"v2_ar_clusters", {"UPDATE"}},
                               {"v2_cs_clustersets", {"UPDATE"}},
                           }}};

std::string create_grant(const std::string &username,
                         const std::set<std::string> &privileges,
                         bool with_grant_option,
                         const std::string &schema = "*",
                         const std::string &object = "*") {
  std::string grant = "GRANT " + shcore::str_join(privileges, ", ") + " ON " +
                      schema + "." + object + " TO " + username;

  if (with_grant_option) grant += " WITH GRANT OPTION";

  return grant;
}

}  // namespace

/** Check that the provided account has privileges to manage a cluster.
 */
bool validate_cluster_admin_user_privileges(
    const mysqlshdk::mysql::IInstance &instance, const std::string &admin_user,
    const std::string &admin_host, Cluster_type purpose,
    std::string *validation_error) {
  bool is_valid = true;

  std::string account = shcore::make_account(admin_user, admin_host);

  auto append_error_message = [account, validation_error, &is_valid](
                                  const std::string &info,
                                  const std::set<std::string> &privileges) {
    is_valid = false;
    *validation_error += "\nGRANT " + shcore::str_join(privileges, ", ") +
                         " ON " + info + " TO " + account +
                         " WITH GRANT OPTION;";
  };

  log_info("Validating account %s@%s...", admin_user.c_str(),
           admin_host.c_str());

  mysqlshdk::mysql::User_privileges user_privileges{instance, admin_user,
                                                    admin_host};

  // Check global privileges.
  const auto global_privileges =
      global_admin_privileges_for_server_version(instance.get_version());

  auto global = user_privileges.validate(global_privileges);

  if (global.has_missing_privileges()) {
    append_error_message("*.*", global.missing_privileges());
  } else if (!global.has_grant_option()) {
    append_error_message("*.*", global_privileges);
  }

  for (const auto &schema_grants : k_admin_schema_grants) {
    auto schema =
        user_privileges.validate(schema_grants.second, schema_grants.first);

    if (schema.has_missing_privileges()) {
      append_error_message(schema_grants.first + ".*",
                           schema.missing_privileges());
    } else if (!schema.has_grant_option()) {
      append_error_message(schema_grants.first + ".*", schema_grants.second);
    }
  }

  for (const auto &schema_grants : k_admin_table_grants) {
    for (const auto &table_grants : schema_grants.second) {
      auto table = user_privileges.validate(
          table_grants.second, schema_grants.first, table_grants.first);

      if (table.has_missing_privileges()) {
        append_error_message(schema_grants.first + "." + table_grants.first,
                             table.missing_privileges());
      } else if (!table.has_grant_option()) {
        append_error_message(schema_grants.first + "." + table_grants.first,
                             table_grants.second);
      }
    }
  }

  *validation_error = shcore::str_format(
      "The account %s is missing privileges required to manage an %s: %s",
      account.c_str(),
      to_display_string(purpose, Display_form::THING_FULL).c_str(),
      validation_error->c_str());

  return is_valid;
}

std::vector<std::string> create_admin_grants(
    const std::string &username,
    const mysqlshdk::utils::Version &instance_version) {
  std::vector<std::string> grants;

  // global privileges
  const auto global_privileges =
      global_admin_privileges_for_server_version(instance_version);
  grants.emplace_back(create_grant(username, global_privileges, true));

  // privileges for schemas
  for (const auto &schema : k_admin_schema_grants) {
    grants.emplace_back(
        create_grant(username, schema.second, true, schema.first));
  }

  // privileges for tables
  for (const auto &schema : k_admin_table_grants) {
    for (const auto &table : schema.second) {
      grants.emplace_back(create_grant(username, table.second, true,
                                       schema.first, table.first));
    }
  }

  return grants;
}

std::vector<std::string> create_router_grants(
    const std::string &username,
    const mysqlshdk::utils::Version &metadata_version) {
  std::vector<std::string> grants;

  // privileges for schemas
  // No need to remove schemas that might not exist on older metadata versions
  // because MySQL enables you to grant priavileges on databases that do not
  // exist. See https://dev.mysql.com/doc/refman/en/grant.html
  for (const auto &schema : k_router_schema_grants) {
    grants.emplace_back(
        create_grant(username, schema.second, false, schema.first));
  }

  // The same would apply to a non existing table if we were granting the
  // CREATE privilege which we are not, so we must remove the non existing
  // tables on older metadata version from the list of tables
  std::map<std::string, std::map<std::string, std::set<std::string>>>
      router_table_grants = k_router_table_grants;
  // For old metadata versions, there is no v2_routers, v2_gr_clusters,
  // v2_ar_clusters, and v2_cs_clustersets tables, so drop them from
  // table_grants
  if (metadata_version <= mysqlshdk::utils::Version(2, 0, 0)) {
    router_table_grants["mysql_innodb_cluster_metadata"].erase("v2_routers");
    router_table_grants["mysql_innodb_cluster_metadata"].erase(
        "v2_gr_clusters");
    router_table_grants["mysql_innodb_cluster_metadata"].erase(
        "v2_ar_clusters");
    router_table_grants["mysql_innodb_cluster_metadata"].erase(
        "v2_cs_clustersets");
    router_table_grants["mysql_innodb_cluster_metadata"].erase("clustersets");
    router_table_grants["mysql_innodb_cluster_metadata"].erase("clusters");
  }

  // privileges for tables
  for (const auto &schema : router_table_grants) {
    for (const auto &table : schema.second) {
      grants.emplace_back(create_grant(username, table.second, false,
                                       schema.first, table.first));
    }
  }

  return grants;
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
    const std::string &host, bool interactive, Cluster_type purpose) {
  int n_wildcard_accounts, n_non_wildcard_accounts;
  std::vector<std::string> hosts;

  std::tie(n_wildcard_accounts, n_non_wildcard_accounts) =
      mysqlsh::dba::find_cluster_admin_accounts(instance, user, &hosts);

  // there are multiple accounts defined with the same username
  // we assume that the user knows what they're doing and skip the rest
  if (n_wildcard_accounts + n_non_wildcard_accounts > 1) {
    log_info("%i accounts named %s found. Skipping remaining checks",
             n_wildcard_accounts + n_non_wildcard_accounts, user.c_str());
    return true;
  } else {
    auto console = mysqlsh::current_console();

    if (hosts.size() == 1 && n_wildcard_accounts == 0) {
      console->print_info();
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
      // there is only one account with a wildcard or netmask, so validate it
      if (n_wildcard_accounts == 1 && hiter != hosts.end()) {
        std::string whost = *hiter;
        // don't need to check privs if this is the account in use, since
        // that will already be validated before
        if (whost != host) {
          std::string error_msg;
          if (mysqlsh::dba::validate_cluster_admin_user_privileges(
                  instance, user, whost, purpose, &error_msg)) {
            log_info(
                "Account %s@%s has required privileges for cluster management",
                user.c_str(), whost.c_str());
            // account accepted
            return true;
          } else {
            console->print_error(error_msg);
            if (!interactive) {
              auto msg = shcore::str_format(
                  "The account %s is missing privileges required to manage an "
                  "%s.",
                  shcore::make_account(user, whost).c_str(),
                  to_display_string(purpose, Display_form::THING_FULL).c_str());

              throw shcore::Exception::runtime_error(msg);
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

/*
 * Prompts new account password and prompts for confirmation of the password
 *
 * @return a string with the password introduced by the user
 */
std::string prompt_new_account_password() {
  std::string password1;
  std::string password2;

  auto console = mysqlsh::current_console();
  shcore::Prompt_result res;

  for (;;) {
    res = console->prompt_password("Password for new account: ", &password1);
    if (shcore::Prompt_result::Ok == res) {
      res = console->prompt_password("Confirm password: ", &password2);
      if (shcore::Prompt_result::Ok == res) {
        if (password1 != password2) {
          console->print_info("Passwords don't match, please try again.");
          continue;
        }
      }
    }
    break;
  }
  if (res == shcore::Prompt_result::Ok) {
    return password1;
  } else {
    throw shcore::cancelled("Cancelled");
  }
}

/** Validates a given account name and splits it into username and host.
 * This function splits a given account name in the format <username>@<host>
 * into username and hostname. The hostname part is optional and if not provided
 * defaults to '%'.
 *
 * @param account The account to split into username and host.
 * @return pair<std::string, std::string> returns the pair username, hostname
 * @throw argument_error if username is empty or if the the account name
 * has invalid syntax.
 */
std::pair<std::string, std::string> validate_account_name(
    const std::string &account) {
  // split user into user/host
  std::string username, host;
  try {
    shcore::split_account(account, &username, &host,
                          shcore::Account::Auto_quote::HOST);
  } catch (const std::exception &err) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid user syntax: %s", err.what()));
  }
  if (shcore::str_strip(username).empty()) {
    throw shcore::Exception::argument_error(
        "Invalid user syntax: User name must not be empty.");
  }
  if (host.empty()) {
    host = "%";
  }
  return std::make_pair(username, host);
}

}  // namespace dba
}  // namespace mysqlsh
