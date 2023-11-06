/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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
#include <functional>
#include <iterator>
#include <vector>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
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
 * Creates privilege level for the given table in form `schema`.`table`.
 *
 * @param schema schema name
 * @param table table name
 */
std::string privilege_level(const std::string &schema,
                            const std::string &table) {
  const auto quote = [](const std::string &name) {
    return User_privileges::k_wildcard == name ? name
                                               : shcore::quote_identifier(name);
  };

  return quote(schema) + "." + quote(table);
}

/**
 * Checks if `schema`.`table` correspond to a database-level privilege.
 *
 * @param schema schema name
 * @param table table name
 */
bool is_database_privilege_level(const std::string &schema,
                                 const std::string &table) {
  return User_privileges::k_wildcard != schema &&
         User_privileges::k_wildcard == table;
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

    read_partial_revokes(instance);
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

  std::function<std::set<std::string> &(const std::string &)> get_target;

  const auto use = [&get_target](Privileges *target) {
    get_target = [target](const std::string &level) -> std::set<std::string> & {
      return (*target)[level];
    };
  };

  const auto use_wildcard = [&get_target](Wildcard_privileges *target) {
    get_target = [target](const std::string &level) -> std::set<std::string> & {
      const auto range = target->equal_range(level);

      for (auto curr = range.first; curr != range.second; ++curr) {
        if (level == curr->first) {
          return curr->second;
        }
      }

      return target->emplace(level, std::set<std::string>{})->second;
    };
  };

  const auto type = it.next_token();
  bool grant = true;
  bool grant_all = false;

  if (shcore::str_caseeq(type, "GRANT")) {
    use(&m_privileges.regular);

    if (shcore::str_ibeginswith(statement, k_grant_all)) {
      grant_all = true;
      it.set_position(::strlen(k_grant_all));
    }
  } else if (shcore::str_caseeq(type, "REVOKE")) {
    use(&m_revoked_privileges);

    // REVOKE statements can appear if partial_revokes is enabled, partial
    // revokes apply at the schema level only (column_list or object_type) does
    // not appear in the statement
    grant = false;
  } else {
    throw std::logic_error("Unsupported grant statement: " + std::string{type});
  }

  std::set<std::string> privileges;

  if (!grant_all) {
    bool all_privileges_done = false;
    bool privilege_done = false;

    do {
      bool add_privilege = true;
      std::string privilege{it.next_token()};

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
          privilege += ' ';
          privilege += token;
        } else if (add_privilege) {
          privileges.emplace(std::exchange(privilege, {}));
        }
      } while (!privilege_done && !all_privileges_done);
    } while (!all_privileges_done);
  }

  if (1 == privileges.size() &&
      shcore::str_caseeq(*privileges.begin(), "PROXY")) {
    // GRANT PROXY ON user_or_role TO user_or_role ...
    // PROXY privilege is not handled
    return;
  }

  // we're now past ON, next is [object_type] priv_level
  std::string privilege_level{it.next_token()};

  if (shcore::str_caseeq(privilege_level, "FUNCTION", "PROCEDURE")) {
    // we're not interested in such privileges
    return;
  } else if (shcore::str_caseeq(privilege_level, "TABLE")) {
    privilege_level = it.next_token();
  }

  // get the privilege level
  std::string schema;
  std::string table;
  shcore::split_priv_level(privilege_level, &schema, &table);

  const auto uses_wildcards = is_wildcard_privilege_level(schema, table);

  if (uses_wildcards) {
    // this cannot be a revoked privilege, as partial_revokes has to be disabled
    use_wildcard(&m_privileges.wildcard);
    // we're storing wildcard privileges using schema name only
    privilege_level = schema;
  }

  if (grant_all) {
    // check what kind of privilege level is it, choose which privileges are
    // granted
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
      if (uses_wildcards) {
        use_wildcard(&m_grantable_privileges.wildcard);
      } else {
        use(&m_grantable_privileges.regular);
      }
      break;
    }
  }

  // AS user does not appear in the output of SHOW GRANTS

  assert(get_target);

  if (!privileges.empty()) {
    auto &current_privileges = get_target(privilege_level);

    std::move(privileges.begin(), privileges.end(),
              std::inserter(current_privileges, current_privileges.begin()));
  }
}

std::set<std::string> User_privileges::get_privileges_at_level(
    const All_privileges &privileges, const std::string &schema,
    const std::string &table) const {
  std::set<std::string> user_privileges;

  const auto copy = [&user_privileges, &privileges](const std::string &s,
                                                    const std::string &t) {
    const auto level = privilege_level(s, t);
    const auto it = privileges.regular.find(level);
    const auto insert = [&user_privileges](const auto &container) {
      std::copy(container.begin(), container.end(),
                std::inserter(user_privileges, user_privileges.begin()));
    };

    if (it != privileges.regular.end()) {
      insert(it->second);
    } else if (is_database_privilege_level(s, t)) {
      for (const auto &wild : privileges.wildcard) {
        if (shcore::match_sql_wild(s, wild.first)) {
          insert(wild.second);
          break;
        }
      }
    }
  };

  copy(k_wildcard, k_wildcard);

  if (k_wildcard != schema) {
    copy(schema, k_wildcard);

    if (k_wildcard != table) {
      copy(schema, table);
    }
  }

  return user_privileges;
}

std::set<std::string> User_privileges::get_missing_privileges(
    const std::set<std::string> &required_privileges, const std::string &schema,
    const std::string &table, bool *are_grantable) const {
  assert(are_grantable);

  auto grantable_privileges =
      get_privileges_at_level(m_grantable_privileges, schema, table);
  auto privileges = get_privileges_at_level(m_privileges, schema, table);

  if (k_wildcard != schema) {
    // partial revokes apply at the schema level only
    const auto it =
        m_revoked_privileges.find(privilege_level(schema, k_wildcard));

    if (it != m_revoked_privileges.end()) {
      for (const auto &revoked : it->second) {
        grantable_privileges.erase(revoked);
        privileges.erase(revoked);
      }
    }
  }

  const auto get_missing = [&required_privileges](const auto &actual) {
    std::set<std::string> missing;
    std::set_difference(required_privileges.begin(), required_privileges.end(),
                        actual.begin(), actual.end(),
                        std::inserter(missing, missing.begin()));
    return missing;
  };

  *are_grantable = get_missing(grantable_privileges).empty();

  privileges.merge(grantable_privileges);
  return get_missing(privileges);
}

bool User_privileges::is_wildcard_privilege_level(
    const std::string &schema, const std::string &table) const {
  // wildcard match is used only if partial_revokes is disabled and a
  // schema name in a database-level privilege contains a SQL wildcard character
  return !m_partial_revokes && is_database_privilege_level(schema, table) &&
         shcore::has_sql_wildcard(schema);
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
    std::string account{it.next_token()};
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
  // Roles are not supported in MySQL 5.7
  if (instance.get_version() < Version(8, 0, 0)) {
    return;
  }

  // Get value of system variable that indicates if all roles are active.
  bool all_roles_active = false;

  if (const auto active = instance.get_sysvar_bool(
          "activate_all_roles_on_login", Var_qualifier::GLOBAL);
      active.has_value()) {
    all_roles_active = *active;
  } else {
    // Roles are not supported in this instance
    return;
  }

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

void User_privileges::read_partial_revokes(
    const mysqlshdk::mysql::IInstance &instance) {
  // Partial revokes were added in 8.0.16
  if (instance.get_version() < Version(8, 0, 16)) {
    m_partial_revokes = false;
  } else {
    m_partial_revokes =
        instance.get_sysvar_bool("partial_revokes").value_or(false);
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
    m_missing_privileges = privileges.get_missing_privileges(
        uppercase_privileges, schema, table, &m_has_grant_option);
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
