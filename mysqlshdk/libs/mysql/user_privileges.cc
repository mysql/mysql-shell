/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/mysql/user_privileges.h"

#include <mysqld_error.h>
#include <algorithm>
#include <iterator>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace mysql {

namespace {

/**
 * List of all privileges, equivalent to "ALL [PRIVILEGES]".
 */
const std::set<std::string> k_all_privileges = {"ALTER",
                                                "ALTER ROUTINE",
                                                "CREATE",
                                                "CREATE ROLE",
                                                "CREATE ROUTINE",
                                                "CREATE TABLESPACE",
                                                "CREATE TEMPORARY TABLES",
                                                "CREATE USER",
                                                "CREATE VIEW",
                                                "DELETE",
                                                "DROP",
                                                "DROP ROLE",
                                                "EVENT",
                                                "EXECUTE",
                                                "FILE",
                                                "INDEX",
                                                "INSERT",
                                                "LOCK TABLES",
                                                "PROCESS",
                                                "REFERENCES",
                                                "RELOAD",
                                                "REPLICATION CLIENT",
                                                "REPLICATION SLAVE",
                                                "SELECT",
                                                "SHOW DATABASES",
                                                "SHOW VIEW",
                                                "SHUTDOWN",
                                                "SUPER",
                                                "TRIGGER",
                                                "UPDATE",
                                                "APPLICATION_PASSWORD_ADMIN",
                                                "AUDIT_ADMIN",
                                                "BACKUP_ADMIN",
                                                "BINLOG_ADMIN",
                                                "BINLOG_ENCRYPTION_ADMIN",
                                                "CLONE_ADMIN",
                                                "CONNECTION_ADMIN",
                                                "ENCRYPTION_KEY_ADMIN",
                                                "FIREWALL_ADMIN",
                                                "FIREWALL_USER",
                                                "GROUP_REPLICATION_ADMIN",
                                                "INNODB_REDO_LOG_ARCHIVE",
                                                "NDB_STORED_USER",
                                                "PERSIST_RO_VARIABLES_ADMIN",
                                                "REPLICATION_APPLIER",
                                                "REPLICATION_SLAVE_ADMIN",
                                                "RESOURCE_GROUP_ADMIN",
                                                "RESOURCE_GROUP_USER",
                                                "ROLE_ADMIN",
                                                "SESSION_VARIABLES_ADMIN",
                                                "SET_USER_ID",
                                                "SYSTEM_USER",
                                                "SYSTEM_VARIABLES_ADMIN",
                                                "TABLE_ENCRYPTION_ADMIN",
                                                "VERSION_TOKEN_ADMIN",
                                                "XA_RECOVER_ADMIN"};

const std::set<std::string> k_all_privileges_57 = {"ALTER",
                                                   "ALTER ROUTINE",
                                                   "CREATE",
                                                   "CREATE ROUTINE",
                                                   "CREATE TABLESPACE",
                                                   "CREATE TEMPORARY TABLES",
                                                   "CREATE USER",
                                                   "CREATE VIEW",
                                                   "DELETE",
                                                   "DROP",
                                                   "EVENT",
                                                   "EXECUTE",
                                                   "FILE",
                                                   "INDEX",
                                                   "INSERT",
                                                   "LOCK TABLES",
                                                   "PROCESS",
                                                   "REFERENCES",
                                                   "RELOAD",
                                                   "REPLICATION CLIENT",
                                                   "REPLICATION SLAVE",
                                                   "SELECT",
                                                   "SHOW DATABASES",
                                                   "SHOW VIEW",
                                                   "SHUTDOWN",
                                                   "SUPER",
                                                   "TRIGGER",
                                                   "UPDATE"};

/**
 * Checks if set contains only valid privileges.
 *
 * Converts all privileges to uppercase. Replaces string "ALL" or
 * "ALL PRIVILEGES" with a list of all privileges.
 *
 * @param privileges A set of privileges to validate.
 * @param all_privileges A set with all the individual privileges (included by
 *                       ALL or ALL PRIVILEGES).
 *
 * @return A set of valid privileges.
 *
 * @throw std::runtime_error if set contains an invalid privilege.
 * @throw std::runtime_error if set contains more than one privilege and "ALL"
 *                           is specified.
 */
std::set<std::string> validate_privileges(
    const std::set<std::string> &privileges,
    const std::set<std::string> &all_privileges) {
  std::set<std::string> result;
  std::transform(privileges.begin(), privileges.end(),
                 std::inserter(result, result.begin()), shcore::str_upper);

  std::set<std::string> difference;
  std::set_difference(result.begin(), result.end(), all_privileges.begin(),
                      all_privileges.end(),
                      std::inserter(difference, difference.begin()));

  if (!difference.empty()) {
    bool has_all = (difference.erase("ALL") != 0 ||
                    difference.erase("ALL PRIVILEGES") != 0);

    if (!difference.empty()) {
      throw std::runtime_error{"Invalid privilege in the privileges list: " +
                               shcore::str_join(difference, ", ") + "."};
    }

    if (has_all) {
      if (privileges.size() > 1) {
        throw std::runtime_error{
            "Invalid list of privileges. 'ALL [PRIVILEGES]' already includes "
            "all the other privileges and it must be specified alone."};
      }

      result = all_privileges;
    }
  }

  return result;
}

/**
 * Checks if string represents a grantable privilege.
 *
 * @param grant A string to be checked.
 *
 * @return True if string represents a grantable privilege.
 */
bool is_grantable(const std::string &grant) {
  return grant.compare("YES") == 0;
}

/**
 * Get all the child roles (recursively) for the specified role.
 *
 * @param role_edges Map with the information of all the defined role edges.
 * @param out_child_roles Output set with all the resulting child roles.
 * @param role String with the target role to obtain all its children.
 */
void get_child_roles(
    const std::unordered_map<std::string, std::set<std::string>> &role_edges,
    std::set<std::string> *out_child_roles, const std::string &role) {
  // Get direct children (iterator) from the target role/user.
  auto children_iter = role_edges.find(role);
  if (children_iter == role_edges.end()) {
    // No child role found, exit.
    return;
  }

  for (const std::string &child_role : children_iter->second) {
    // Add child to resulting set and recursively get its children if not
    // already in the result role set.
    if (out_child_roles->find(child_role) == out_child_roles->end()) {
      out_child_roles->emplace(child_role);
      get_child_roles(role_edges, out_child_roles, child_role);
    }
  }
}

}  // namespace

struct User_privileges::Mapped_row {
  Mapped_row() : schema{k_wildcard}, table{k_wildcard} {}
  std::string schema;
  std::string table;
};

User_privileges::User_privileges(const mysqlshdk::mysql::IInstance &instance,
                                 const std::string &user,
                                 const std::string &host) {
  m_account = shcore::make_account(user, host);

  read_global_privileges(instance, m_account);

  // Set list of individual privileges included by ALL.
  set_all_privileges(instance);

  if (m_privileges[m_account][k_wildcard][k_wildcard].empty()) {
    // User does not exist (all users have at least one privilege: USAGE)
    return;
  }

  m_user_exists = true;

  read_schema_privileges(instance, m_account);
  read_table_privileges(instance, m_account);

  // Use of roles is only supported starting from MySQL 8.0.0.
  if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 0)) {
    // Read user roles.
    try {
      log_debug("Reading roles information for user %s", m_account.c_str());
      read_user_roles(instance);
    } catch (const mysqlshdk::db::Error &err) {
      log_debug("%s", err.format().c_str());
      if (err.code() == ER_TABLEACCESS_DENIED_ERROR) {
        auto console = mysqlsh::current_console();
        console->print_error("Unable to check privileges for user " +
                             m_account +
                             ". User requires SELECT privilege on mysql.* to "
                             "obtain information about all roles.");
        throw std::runtime_error{"Unable to get roles information."};
      } else {
        throw;
      }
    }

    // Read privileges for all user roles.
    for (const std::string &user_role : m_roles) {
      log_debug("Reading privileges for role/user %s", user_role.c_str());
      read_global_privileges(instance, user_role);
      read_schema_privileges(instance, user_role);
      read_table_privileges(instance, user_role);
    }
  }
}

const char User_privileges::k_wildcard[] = "*";

bool User_privileges::user_exists() const { return m_user_exists; }

bool User_privileges::has_grant_option(const std::string &schema,
                                       const std::string &table) const {
  if (user_exists()) {
    bool grant = false;

    // Check grantable for user and roles.
    for (const auto &kv : m_grants) {
      auto role_grant = kv.second;
      grant |= role_grant.at(k_wildcard).at(k_wildcard);

      if (!grant && schema != k_wildcard) {
        auto schema_grants = role_grant.find(schema);

        if (schema_grants != role_grant.end()) {
          auto table_grants = schema_grants->second.find(k_wildcard);

          if (table_grants != schema_grants->second.end()) {
            grant |= table_grants->second;
          }

          if (!grant && table != k_wildcard) {
            table_grants = schema_grants->second.find(table);

            if (table_grants != schema_grants->second.end()) {
              grant |= table_grants->second;
            }
          }
        }
      }
    }

    return grant;
  } else {
    return false;
  }
}

std::set<std::string> User_privileges::get_missing_privileges(
    const std::set<std::string> &required_privileges, const std::string &schema,
    const std::string &table) const {
  auto privileges = validate_privileges(required_privileges, m_all_privileges);
  std::set<std::string> missing_privileges;

  if (user_exists()) {
    std::set<std::string> user_privileges;

    // Get privileges from user and associated roles.
    for (const auto &kv : m_privileges) {
      // Get wildcard privileges.
      auto &role_privileges = kv.second;
      auto &wildcard_privileges = role_privileges.at(k_wildcard).at(k_wildcard);
      user_privileges.insert(wildcard_privileges.begin(),
                             wildcard_privileges.end());

      // Get schema/table privileges.
      if (schema != k_wildcard) {
        auto schema_privileges = role_privileges.find(schema);

        if (schema_privileges != role_privileges.end()) {
          auto table_privileges = schema_privileges->second.find(k_wildcard);

          if (table_privileges != schema_privileges->second.end()) {
            user_privileges.insert(table_privileges->second.begin(),
                                   table_privileges->second.end());
          }

          if (table != k_wildcard) {
            table_privileges = schema_privileges->second.find(table);

            if (table_privileges != schema_privileges->second.end()) {
              user_privileges.insert(table_privileges->second.begin(),
                                     table_privileges->second.end());
            }
          }
        }
      }
    }

    std::set_difference(
        privileges.begin(), privileges.end(), user_privileges.begin(),
        user_privileges.end(),
        std::inserter(missing_privileges, missing_privileges.begin()));
  } else {
    // If user does not exist, then all privileges are also missing.
    missing_privileges = std::move(privileges);
  }

  if (required_privileges.size() == 1 &&
      missing_privileges.size() == m_all_privileges.size()) {
    // Set missing to 'ALL [PRIVILEGES]'
    return {shcore::str_upper(*required_privileges.begin())};
  } else {
    return missing_privileges;
  }
}

User_privileges_result User_privileges::validate(
    const std::set<std::string> &required_privileges, const std::string &schema,
    const std::string &table) const {
  return User_privileges_result(*this, required_privileges, schema, table);
}

void User_privileges::parse_privileges(
    const std::shared_ptr<db::IResult> &result, Row_mapper map_row,
    const std::string &user_role) {
  Mapped_row mapped_row;
  std::string schema;
  std::string table;
  std::set<std::string> *privileges = nullptr;
  bool *grants = nullptr;

  auto select_table = [&]() {
    schema = mapped_row.schema;
    table = mapped_row.table;
    privileges = &m_privileges[user_role][schema][table];
    grants = &m_grants[user_role][schema][table];
  };

  select_table();

  while (const auto row = result->fetch_one()) {
    map_row(row, &mapped_row);

    if (schema != mapped_row.schema || table != mapped_row.table) {
      select_table();
    }

    privileges->emplace(row->get_string(0));
    *grants = is_grantable(row->get_string(1));
  }
}

std::set<std::string> User_privileges::get_mandatory_roles(
    const mysqlshdk::mysql::IInstance &instance) const {
  // Get value of the system variable with the mandatory roles.
  std::string query{"SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'"};
  auto result = instance.query(query);
  auto row = result->fetch_one();
  std::string str_roles = row->get_string(1);

  // Return an empty set if no mandatory roles are defined.
  if (str_roles.empty()) {
    return std::set<std::string>{};
  }

  // Convert the string value for 'mandatory_roles to a set of strings, each
  // role with the format '<user>'@'<host>'.
  std::set<std::string> mandatory_roles;

  std::vector<std::string> roles =
      shcore::str_split(shcore::str_strip(str_roles), ",", -1);

  for (const std::string &role : roles) {
    // Split each role account to handle backticks and the optional host part.
    std::string user, host;
    shcore::split_account(role, &user, &host);

    // If the host part is not defined then % must be used by default.
    if (host.empty()) {
      host = '%';
    }

    // Insert role in the resulting set with the desired format.
    mandatory_roles.insert(shcore::make_account(user, host));
  }

  return mandatory_roles;
}

void User_privileges::read_user_roles(
    const mysqlshdk::mysql::IInstance &instance) {
  // Get user and host parts from the user account.
  std::string user, host;
  shcore::split_account(m_account, &user, &host);

  // Get value of system variable that indicates if all roles are active.
  std::string query{"SHOW GLOBAL VARIABLES LIKE 'activate_all_roles_on_login'"};
  auto result = instance.query(query);
  auto row = result->fetch_one();
  bool all_roles_active = (row->get_string(1) == "ON");

  std::set<std::string> active_roles;
  if (!all_roles_active) {
    // Get active roles for the user account (specified with SET DEFAULT ROLE).
    // NOTE: activate_all_roles_on_login as precedence over default roles.
    shcore::sqlstring sql_query = shcore::sqlstring(
        "SELECT default_role_user, default_role_host "
        "FROM mysql.default_roles WHERE user = ? AND host = ?",
        0);
    sql_query << user;
    sql_query << host;
    sql_query.done();
    result = instance.query(sql_query);
    row = result->fetch_one();

    // If there are no active roles then exit.
    if (!row) {
      return;
    }

    // Get all active roles
    while (row) {
      std::string role_user = row->get_string(0);
      std::string role_host = row->get_string(1);
      active_roles.emplace(shcore::make_account(role_user, role_host));
      row = result->fetch_one();
    }
  }

  // Get all roles to obtain hierarchy and dependency between roles.
  std::unordered_map<std::string, std::set<std::string>> all_roles;
  query =
      "SELECT from_user, from_host, to_user, to_host "
      "FROM mysql.role_edges";
  result = instance.query(query);
  row = result->fetch_one();
  while (row) {
    std::string from_role =
        shcore::make_account(row->get_string(0), row->get_string(1));
    std::string to_role =
        shcore::make_account(row->get_string(2), row->get_string(3));

    // Each role/user can have multiple roles/users associated to it.
    auto it = all_roles.find(to_role);
    if (it == all_roles.end()) {
      // New role/user association.
      all_roles.emplace(to_role, std::set<std::string>{from_role});
    } else {
      // Add another role/user association to an existing one.
      it->second.emplace(from_role);
    }
    row = result->fetch_one();
  }

  // Get mandatory roles and add them to the user.
  std::set<std::string> mandatory_roles = get_mandatory_roles(instance);
  if (!mandatory_roles.empty()) {
    auto it = all_roles.find(m_account);
    if (it == all_roles.end()) {
      // No roles defined for the user, add the mandatory roles
      all_roles.emplace(m_account, std::set<std::string>{mandatory_roles});
    } else {
      // Already exist roles defined for user, add the mandatory roles.
      it->second.insert(mandatory_roles.begin(), mandatory_roles.end());
    }
  }

  // 'activate_all_roles_on_login' is ON then all roles associated to the user
  // are active (if he has any).
  if (all_roles_active) {
    auto it = all_roles.find(m_account);
    if (it != all_roles.end()) {
      active_roles.insert(it->second.begin(), it->second.end());
    }
  }

  // Initial result list contains: user account + all (active) associated roles.
  // NOTE: user account is added because it can be granted to a role it has.
  m_roles.emplace(m_account);
  m_roles.insert(active_roles.begin(), active_roles.end());

  // Get remaining child roles for all active roles and add them to the result
  // role list.
  // NOTE: Role dependencies ("sub-roles") are automatically active (behaviour
  // confirmed with the server team).
  for (const std::string &active_role : active_roles) {
    get_child_roles(all_roles, &m_roles, active_role);
  }

  // Remove the user account to set assigned user/roles (excluding itself).
  m_roles.erase(m_account);
}

void User_privileges::set_all_privileges(
    const mysqlshdk::mysql::IInstance &instance) {
  // Set list of ALL privileges according to the server version.
  if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 0)) {
    // Set all privileges for 8.0 servers.
    m_all_privileges = k_all_privileges;

    // Remove individual privileges dependent on plugins, which are only listed
    // if the plugin is installed.
    // NOTE: Plugin specific privileges are only granted when using ALL if the
    // plugin is installed (even if disabled) at the time the ALL privilege is
    // granted to the user, otherwise it will not be listed as an individual
    // privilege, even if the plugin is installed later. This means that a new
    // grant ALL will be needed for existing users with "ALL" privileges after
    // installing a new plugin for its plugin specific privileges to be
    // included in the list of individual grants.
    if (instance.get_plugin_status("audit_log").is_null()) {
      m_all_privileges.erase("AUDIT_ADMIN");
    }
    if (instance.get_plugin_status("mysql_firewall").is_null()) {
      m_all_privileges.erase("FIREWALL_ADMIN");
      m_all_privileges.erase("FIREWALL_USER");
    }
    if (instance.get_plugin_status("version_tokens").is_null()) {
      m_all_privileges.erase("VERSION_TOKEN_ADMIN");
    }

    // REPLICATION APPLIER privilege was introduced on MySQL 8.0.18
    if (instance.get_version() < mysqlshdk::utils::Version(8, 0, 18))
      m_all_privileges.erase("REPLICATION_APPLIER");

    // BACKUP_ADMIN privilege was introduced on MySQL 8.0.17
    if (instance.get_version() <
        mysqlshdk::mysql::k_mysql_clone_plugin_initial_version)
      m_all_privileges.erase("BACKUP_ADMIN");

    // CLONE_ADMIN privilege was introduced on MySQL 8.0.17
    if (instance.get_version() <
        mysqlshdk::mysql::k_mysql_clone_plugin_initial_version)
      m_all_privileges.erase("CLONE_ADMIN");

    // Remove individual privileges dependent on engines, which are only listed
    // if the engine is available.
    auto res = instance.query(
        "SELECT engine FROM information_schema.engines "
        "WHERE engine = 'NDBCLUSTER'");
    if (!res->fetch_one()) {
      m_all_privileges.erase("NDB_STORED_USER");
    }

  } else {
    // Set all privileges for 5.7 servers.
    m_all_privileges = k_all_privileges_57;
  }
}

void User_privileges::read_global_privileges(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user_role) {
  const char *const query =
      "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
      "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
      "WHERE GRANTEE = ?";

  read_privileges(
      instance, query, [](const db::IRow *const, Mapped_row *) {}, user_role);
}

void User_privileges::read_schema_privileges(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user_role) {
  const char *const query =
      "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
      "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
      "WHERE GRANTEE = ? ORDER BY TABLE_SCHEMA";

  read_privileges(
      instance, query,
      [](const db::IRow *const row, Mapped_row *result) {
        result->schema = row->get_string(2);
      },
      user_role);
}

void User_privileges::read_table_privileges(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user_role) {
  const char *const query =
      "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
      "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
      "WHERE GRANTEE = ? ORDER BY TABLE_SCHEMA, TABLE_NAME";

  read_privileges(
      instance, query,
      [](const db::IRow *const row, Mapped_row *result) {
        result->schema = row->get_string(2);
        result->table = row->get_string(3);
      },
      user_role);
}

void User_privileges::read_privileges(
    const mysqlshdk::mysql::IInstance &instance, const char *const query,
    Row_mapper map_row, const std::string &user_role) {
  shcore::sqlstring sql_query = shcore::sqlstring(query, 0);

  sql_query << user_role;
  sql_query.done();

  parse_privileges(instance.query(sql_query), map_row, user_role);
}

User_privileges_result::User_privileges_result(
    const User_privileges &privileges,
    const std::set<std::string> &required_privileges, const std::string &schema,
    const std::string &table) {
  m_user_exists = privileges.user_exists();
  m_has_grant_option = privileges.has_grant_option(schema, table);
  m_missing_privileges =
      privileges.get_missing_privileges(required_privileges, schema, table);
}

bool User_privileges_result::user_exists() const { return m_user_exists; }

bool User_privileges_result::has_grant_option() const {
  return m_has_grant_option;
}

bool User_privileges_result::has_missing_privileges() const {
  if (user_exists()) {
    return !m_missing_privileges.empty();
  } else {
    return true;
  }
}

std::set<std::string> User_privileges_result::get_missing_privileges() const {
  return m_missing_privileges;
}

}  // namespace mysql
}  // namespace mysqlshdk
