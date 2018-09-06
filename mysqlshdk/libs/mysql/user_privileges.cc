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

#include "mysqlshdk/libs/mysql/user_privileges.h"

#include <algorithm>
#include <iterator>

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
 *
 * @return A set of valid privileges.
 *
 * @throw std::runtime_error if set contains an invalid privilege.
 * @throw std::runtime_error if set contains more than one privilege and "ALL"
 *                           is specified.
 */
std::set<std::string> validate_privileges(
    const std::set<std::string> &privileges) {
  std::set<std::string> result;
  std::transform(privileges.begin(), privileges.end(),
                 std::inserter(result, result.begin()), shcore::str_upper);

  std::set<std::string> difference;
  std::set_difference(result.begin(), result.end(), k_all_privileges.begin(),
                      k_all_privileges.end(),
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

      result = k_all_privileges;
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

}  // namespace

struct User_privileges::Mapped_row {
  Mapped_row() : schema{k_wildcard}, table{k_wildcard} {}
  std::string schema;
  std::string table;
};

User_privileges::User_privileges(const std::shared_ptr<db::ISession> &session,
                                 const std::string &user,
                                 const std::string &host) {
  m_account = shcore::make_account(user, host);

  read_global_privileges(session);

  if (m_privileges[k_wildcard][k_wildcard].empty()) {
    // User does not exist (all users have at least one privilege: USAGE)
    return;
  }

  m_user_exists = true;

  read_schema_privileges(session);
  read_table_privileges(session);
}

const char User_privileges::k_wildcard[] = "*";

bool User_privileges::user_exists() const { return m_user_exists; }

bool User_privileges::has_grant_option(const std::string &schema,
                                       const std::string &table) const {
  if (user_exists()) {
    bool grant = m_grants.at(k_wildcard).at(k_wildcard);

    if (!grant && schema != k_wildcard) {
      auto schema_grants = m_grants.find(schema);

      if (schema_grants != m_grants.end()) {
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

    return grant;
  } else {
    return false;
  }
}

std::set<std::string> User_privileges::get_missing_privileges(
    const std::set<std::string> &required_privileges, const std::string &schema,
    const std::string &table) const {
  auto privileges = validate_privileges(required_privileges);
  std::set<std::string> missing_privileges;

  if (user_exists()) {
    auto user_privileges = m_privileges.at(k_wildcard).at(k_wildcard);

    if (schema != k_wildcard) {
      auto schema_privileges = m_privileges.find(schema);

      if (schema_privileges != m_privileges.end()) {
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

    std::set_difference(
        privileges.begin(), privileges.end(), user_privileges.begin(),
        user_privileges.end(),
        std::inserter(missing_privileges, missing_privileges.begin()));
  } else {
    // If user does not exist, then all privileges are also missing.
    missing_privileges = privileges;
  }

  if (required_privileges.size() == 1 &&
      missing_privileges.size() == k_all_privileges.size()) {
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
    const std::shared_ptr<db::IResult> &result, Row_mapper map_row) {
  Mapped_row mapped_row;
  std::string schema;
  std::string table;
  std::set<std::string> *privileges = nullptr;
  bool *grants = nullptr;

  auto select_table = [&]() {
    schema = mapped_row.schema;
    table = mapped_row.table;
    privileges = &m_privileges[schema][table];
    grants = &m_grants[schema][table];
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

void User_privileges::read_global_privileges(
    const std::shared_ptr<db::ISession> &session) {
  const char *const query =
      "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
      "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
      "WHERE GRANTEE = ?";

  read_privileges(session, query, [](const db::IRow *const, Mapped_row *) {});
}

void User_privileges::read_schema_privileges(
    const std::shared_ptr<db::ISession> &session) {
  const char *const query =
      "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
      "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
      "WHERE GRANTEE = ? ORDER BY TABLE_SCHEMA";

  read_privileges(session, query,
                  [](const db::IRow *const row, Mapped_row *result) {
                    result->schema = row->get_string(2);
                  });
}

void User_privileges::read_table_privileges(
    const std::shared_ptr<db::ISession> &session) {
  const char *const query =
      "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
      "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
      "WHERE GRANTEE = ? ORDER BY TABLE_SCHEMA, TABLE_NAME";

  read_privileges(session, query,
                  [](const db::IRow *const row, Mapped_row *result) {
                    result->schema = row->get_string(2);
                    result->table = row->get_string(3);
                  });
}

void User_privileges::read_privileges(
    const std::shared_ptr<db::ISession> &session, const char *const query,
    Row_mapper map_row) {
  shcore::sqlstring sql_query = shcore::sqlstring(query, 0);

  sql_query << m_account;
  sql_query.done();

  parse_privileges(session->query(sql_query), map_row);
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
