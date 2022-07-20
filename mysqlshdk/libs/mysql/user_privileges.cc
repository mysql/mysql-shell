/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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
#include <vector>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace mysql {

namespace {

using mysqlshdk::utils::Version;

/**
 * List of all schema privileges, equivalent to: GRANT ALL ON `schema`.*.
 */
const std::set<std::string> k_all_schema_privileges = {
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
    "UPDATE",
};

/**
 * List of all table privileges, equivalent to: GRANT ALL ON `schema`.`table`.
 */
const std::set<std::string> k_all_table_privileges = {
    "ALTER",  "CREATE",     "CREATE VIEW", "DELETE",    "DROP",    "INDEX",
    "INSERT", "REFERENCES", "SELECT",      "SHOW VIEW", "TRIGGER", "UPDATE",
};

/**
 * Beginning of a GRANT ALL statement
 */
const char k_grant_all[] = "GRANT ALL PRIVILEGES ON ";

/**
 * Checks if set contains only valid privileges.
 *
 * Converts all privileges to uppercase.
 *
 * @param privileges A set of privileges to validate.
 * @param all_privileges A set with all the individual privileges.
 *
 * @return A set of valid privileges.
 *
 * @throw std::runtime_error if set contains an invalid privilege.
 */
std::set<std::string> validate_privileges(
    const std::set<std::string> &privileges,
    const std::set<std::string> &all_privileges) {
  std::set<std::string> result;
  std::transform(privileges.begin(), privileges.end(),
                 std::inserter(result, result.begin()),
                 [](const auto &s) { return shcore::str_upper(s); });

  std::set<std::string> difference;
  std::set_difference(result.begin(), result.end(), all_privileges.begin(),
                      all_privileges.end(),
                      std::inserter(difference, difference.begin()));

  if (!difference.empty()) {
    throw std::runtime_error{"Invalid privilege in the privileges list: " +
                             shcore::str_join(difference, ", ") + "."};
  }

  return result;
}

/**
 * Creates privilege level for the given schema in form `schema`.*.
 *
 * @param schema schema name
 */
std::string privilege_level(const std::string &schema) {
  return shcore::quote_identifier(schema) + ".*";
}

/**
 * Creates privilege level for the given table in form `schema`.`table`.
 *
 * @param schema schema name
 * @param table table name
 */
std::string privilege_level(const std::string &schema,
                            const std::string &table) {
  return shcore::quote_identifier(schema) + "." +
         shcore::quote_identifier(table);
}

}  // namespace

const char User_privileges::k_wildcard[] = "*";

User_privileges::User_privileges(const mysqlshdk::mysql::IInstance &instance,
                                 const std::string &user,
                                 const std::string &host,
                                 bool allow_skip_grants_user)
    : m_user(user), m_host(host) {
  m_account = shcore::make_account(m_user, m_host);
  const bool is_skip_grants_user =
      ("'skip-grants user'@'skip-grants host'" == m_account) &&
      allow_skip_grants_user;
  m_user_exists = is_skip_grants_user ? true : check_if_user_exists(instance);

  // Set list of individual privileges included by ALL.
  set_all_privileges(instance);

  if (!m_user_exists) {
    // user does not exist, nothing to do
    return;
  }

  if (is_skip_grants_user) {
    // if this is a skip-grants users, account has all privileges without GRANT
    // OPTION (as GRANT statements cannot be executed anyway)
    parse_grant(
        "GRANT ALL PRIVILEGES ON *.* TO 'skip-grants user'@'skip-grants host'");
  } else {
    // Read user roles, if they are not supported, method will simply exit
    try {
      log_debug("Reading roles information for user %s", m_account.c_str());
      read_user_roles(instance);
    } catch (const mysqlshdk::db::Error &err) {
      log_debug("Failed to read user roles: %s", err.format().c_str());

      if (err.code() == ER_TABLEACCESS_DENIED_ERROR) {
        const auto console = mysqlsh::current_console();

        console->print_error("Unable to check privileges for user " +
                             m_account +
                             ". User requires SELECT privilege on mysql.* to "
                             "obtain information about all roles.");

        throw std::runtime_error{"Unable to get roles information."};
      } else {
        throw;
      }
    }

    parse_user_grants(instance);
  }
}

bool User_privileges::user_exists() const { return m_user_exists; }

bool User_privileges::check_if_user_exists(
    const mysqlshdk::mysql::IInstance &instance) const {
  // all users have at least one privilege: USAGE
  const auto result = instance.queryf(
      "SELECT PRIVILEGE_TYPE FROM INFORMATION_SCHEMA.USER_PRIVILEGES WHERE "
      "GRANTEE=? LIMIT 1",
      m_account);
  return nullptr != result->fetch_one();
}

void User_privileges::parse_user_grants(
    const mysqlshdk::mysql::IInstance &instance) {
  std::string query = "SHOW GRANTS FOR " + m_account;

  if (!m_roles.empty()) {
    query += " USING " + shcore::str_join(m_roles, ",");
  }

  const auto result = instance.query(query);

  while (const auto row = result->fetch_one()) {
    parse_grant(row->get_string(0));
  }
}

void User_privileges::parse_grant(const std::string &statement) {
  utils::SQL_iterator it(statement, 0, false);

  std::set<std::string> privileges;
  std::string privilege_level;
  Privileges *target = nullptr;

  const auto type = it.next_token();
  bool grant = true;
  bool grant_all = false;

  if (shcore::str_caseeq(type, "GRANT")) {
    target = &m_privileges;

    if (shcore::str_ibeginswith(statement, k_grant_all)) {
      grant_all = true;
      it.set_position(::strlen(k_grant_all));
    }
  } else if (shcore::str_caseeq(type, "REVOKE")) {
    target = &m_revoked_privileges;

    // REVOKE statements can appear if partial_revokes is enabled, partial
    // revokes apply at the schema level only (column_list or object_type) does
    // not appear in the statement
    grant = false;
  } else {
    throw std::logic_error("Unsupported grant statement: " + type);
  }

  if (!grant_all) {
    bool all_privileges_done = false;
    bool privilege_done = false;

    do {
      bool add_privilege = true;
      auto privilege = it.next_token();

      do {
        auto token = it.next_token();

        if (shcore::str_caseeq(token, grant ? "TO" : "FROM")) {
          // this is GRANT role, we're not interested in these
          return;
        }

        if (shcore::str_caseeq(token, "(")) {
          // column-level privilege, we're ignoring these
          add_privilege = false;

          // move past column_list
          do {
            token = it.next_token();
          } while (!shcore::str_caseeq(token, ")"));

          // read next token (either , or ON)
          token = it.next_token();
        }

        privilege_done = shcore::str_caseeq(token, ",");
        all_privileges_done = shcore::str_caseeq(token, "ON");

        if (!privilege_done && !all_privileges_done) {
          privilege += " " + token;
        } else if (add_privilege) {
          privileges.emplace(std::exchange(privilege, {}));
        }
      } while (!privilege_done && !all_privileges_done);
    } while (!all_privileges_done);
  }

  // we're now past ON, next is [object_type] priv_level
  privilege_level = it.next_token();

  if (shcore::str_caseeq(privilege_level, "FUNCTION", "PROCEDURE")) {
    // we're not interested in such privileges
    return;
  } else if (shcore::str_caseeq(privilege_level, "TABLE")) {
    privilege_level = it.next_token();
  }

  if (grant_all) {
    // check what kind of privilege level is it, choose which privileges are
    // granted
    std::string schema;
    std::string table;

    shcore::split_priv_level(privilege_level, &schema, &table);

    if (k_wildcard == schema) {
      privileges = m_all_privileges;
    } else {
      if (k_wildcard == table) {
        privileges = k_all_schema_privileges;
      } else {
        privileges = k_all_table_privileges;
      }
    }
  }

  // check if GRANT OPTION is here
  while (it.valid()) {
    if (shcore::str_caseeq(it.next_token(), "GRANT") &&
        shcore::str_caseeq(it.next_token(), "OPTION")) {
      target = &m_grantable_privileges;
      break;
    }
  }

  // AS user does not appear in the output of SHOW GRANTS

  assert(target);

  if (!privileges.empty()) {
    auto &current_privileges = (*target)[privilege_level];

    std::move(privileges.begin(), privileges.end(),
              std::inserter(current_privileges, current_privileges.begin()));
  }
}

std::set<std::string> User_privileges::get_privileges_at_level(
    const Privileges &privileges, const std::string &schema,
    const std::string &table) const {
  std::set<std::string> user_privileges;

  const auto copy = [&user_privileges, &privileges](const std::string &level) {
    const auto it = privileges.find(level);

    if (it != privileges.end()) {
      std::copy(it->second.begin(), it->second.end(),
                std::inserter(user_privileges, user_privileges.begin()));
    }
  };

  copy("*.*");

  if (k_wildcard != schema) {
    copy(privilege_level(schema));

    if (k_wildcard != table) {
      copy(privilege_level(schema, table));
    }
  }

  return user_privileges;
}

std::set<std::string> User_privileges::get_missing_privileges(
    const std::set<std::string> &required_privileges, const std::string &schema,
    const std::string &table, bool only_grantable) const {
  std::set<std::string> user_privileges =
      get_privileges_at_level(m_grantable_privileges, schema, table);

  if (!only_grantable) {
    auto privileges = get_privileges_at_level(m_privileges, schema, table);

    std::move(privileges.begin(), privileges.end(),
              std::inserter(user_privileges, user_privileges.begin()));
  }

  if (k_wildcard != schema) {
    // partial revokes apply at the schema level only
    const auto it = m_revoked_privileges.find(privilege_level(schema));

    if (it != m_revoked_privileges.end()) {
      for (const auto &revoked : it->second) {
        user_privileges.erase(revoked);
      }
    }
  }

  std::set<std::string> missing_privileges;

  std::set_difference(
      required_privileges.begin(), required_privileges.end(),
      user_privileges.begin(), user_privileges.end(),
      std::inserter(missing_privileges, missing_privileges.begin()));

  return missing_privileges;
}

User_privileges_result User_privileges::validate(
    const std::set<std::string> &required_privileges, const std::string &schema,
    const std::string &table) const {
  return User_privileges_result(*this, required_privileges, schema, table);
}

std::set<std::string> User_privileges::get_mandatory_roles(
    const mysqlshdk::mysql::IInstance &instance) const {
  // Get value of the system variable with the mandatory roles.
  const auto result =
      instance.query("SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'");
  const auto str_roles = shcore::str_strip(result->fetch_one()->get_string(1));

  // Return an empty set if no mandatory roles are defined.
  if (str_roles.empty()) {
    return {};
  }

  // Convert the string value for 'mandatory_roles to a set of strings, each
  // role with the format '<user>'@'<host>'.
  std::set<std::string> mandatory_roles;
  utils::SQL_iterator it(str_roles, 0, false);

  while (it.valid()) {
    // user name
    auto account = it.next_token();
    // @ or ,
    const auto token = it.next_token();

    if (shcore::str_caseeq(token, "@")) {
      // host name will follow
      account += token;
      account += it.next_token();
      // read , (if it's there)
      it.next_token();
    }  // else it's , or end of roles

    // Split each role account to handle backticks and the optional host part.
    std::string user, host;
    shcore::split_account(account, &user, &host);

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
  // Get value of system variable that indicates if all roles are active.
  bool all_roles_active = false;

  // Roles are not supported in MySQL 5.7
  if (instance.get_version() < Version(8, 0, 0)) {
    return;
  }

  const auto res =
      instance.query("SELECT @@GLOBAL.activate_all_roles_on_login");
  all_roles_active = res->fetch_one_or_throw()->get_int(0) == 1;

  std::string query;

  if (all_roles_active) {
    // if all roles are active, mandatory roles and all the roles user has are
    // activated when user logs in

    // get mandatory roles and add them to the active roles
    std::set<std::string> mandatory_roles = get_mandatory_roles(instance);
    std::move(mandatory_roles.begin(), mandatory_roles.end(),
              std::inserter(m_roles, m_roles.begin()));

    // get all the roles user has
    query =
        "SELECT from_user, from_host FROM mysql.role_edges WHERE to_user=? "
        "AND to_host=?";
  } else {
    // activate_all_roles_on_login is disabled, only default roles are activated
    query =
        "SELECT default_role_user, default_role_host FROM mysql.default_roles "
        "WHERE user=? AND host=?";
  }

  const auto result = instance.queryf(query, m_user, m_host);

  while (const auto row = result->fetch_one()) {
    m_roles.emplace(
        shcore::make_account(row->get_string(0), row->get_string(1)));
  }
}

void User_privileges::set_all_privileges(
    const mysqlshdk::mysql::IInstance &instance) {
  const auto result = instance.query("SHOW PRIVILEGES");

  m_all_privileges.clear();

  while (const auto row = result->fetch_one()) {
    m_all_privileges.emplace(shcore::str_upper(row->get_string(0)));
  }

  // we're not interested in these two privileges, GRANT OPTION is tracked
  // separately, PROXY is not handled
  m_all_privileges.erase("GRANT OPTION");
  m_all_privileges.erase("PROXY");
}

User_privileges_result::User_privileges_result(
    const User_privileges &privileges,
    const std::set<std::string> &required_privileges, const std::string &schema,
    const std::string &table) {
  m_user_exists = privileges.user_exists();

  // Validate if the set contains only valid privileges
  std::set<std::string> uppercase_privileges =
      validate_privileges(required_privileges, privileges.m_all_privileges);

  if (m_user_exists) {
    m_has_grant_option =
        privileges
            .get_missing_privileges(uppercase_privileges, schema, table, true)
            .empty();
    m_missing_privileges = privileges.get_missing_privileges(
        uppercase_privileges, schema, table, false);
  } else {
    m_has_grant_option = false;
    m_missing_privileges = std::move(uppercase_privileges);
  }
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

const std::set<std::string> &User_privileges_result::missing_privileges()
    const {
  return m_missing_privileges;
}

}  // namespace mysql
}  // namespace mysqlshdk
