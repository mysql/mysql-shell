/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_COMPATIBILITY_H_
#define MODULES_UTIL_DUMP_COMPATIBILITY_H_

#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace compatibility {

extern const std::set<std::string> k_mysqlaas_allowed_privileges;

extern const std::unordered_set<std::string> k_mds_users_with_system_user_priv;

struct Deferred_statements {
  struct Index_info {
    std::vector<std::string> fulltext;
    std::vector<std::string> spatial;
    std::vector<std::string> regular;
    bool has_virtual_columns = false;

    bool empty() const {
      return regular.empty() && fulltext.empty() && spatial.empty();
    }

    std::size_t size() const {
      return regular.size() + fulltext.size() + spatial.size();
    }
  };

  std::string rewritten;
  Index_info index_info;
  std::vector<std::string> foreign_keys;
  std::string secondary_engine;

  bool has_alters() const {
    return !index_info.empty() || !foreign_keys.empty();
  }

  bool empty() const { return !has_alters() && secondary_engine.empty(); }
};

/// Checks grant statement for presence of privileges, returns found ones.
std::vector<std::string> check_privileges(
    const std::string &grant, std::string *out_rewritten_grant = nullptr,
    const std::set<std::string> &privileges = k_mysqlaas_allowed_privileges);

// Rewrite GRANT or REVOKE statement stripping filtered privileges
std::string filter_grant_or_revoke(
    const std::string &stmt,
    const std::function<bool(bool is_revoke, const std::string &priv_type,
                             const std::string &column_list,
                             const std::string &object_type,
                             const std::string &priv_level)> &filter);

/// Return mysql schema table if it is an object of a grant statement
std::string_view is_grant_on_object_from_mysql_schema(const std::string &grant);

bool check_create_table_for_data_index_dir_option(
    const std::string &create_table, std::string *rewritten = nullptr);

bool check_create_table_for_encryption_option(const std::string &create_table,
                                              std::string *rewritten = nullptr);

std::string check_create_table_for_engine_option(
    const std::string &create_table, std::string *rewritten = nullptr,
    const std::string &target = "InnoDB");

bool check_create_table_for_tablespace_option(
    const std::string &create_table, std::string *rewritten = nullptr,
    const std::vector<std::string> &whitelist = {"innodb_"});

bool check_create_table_for_fixed_row_format(const std::string &create_table,
                                             std::string *rewritten = nullptr);

std::vector<std::string> check_statement_for_charset_option(
    const std::string &statement, std::string *rewritten = nullptr,
    const std::vector<std::string> &whitelist = {"utf8mb4"});

std::string check_statement_for_definer_clause(
    const std::string &statement, std::string *rewritten = nullptr);

bool check_statement_for_sqlsecurity_clause(const std::string &statement,
                                            std::string *rewritten = nullptr);

Deferred_statements check_create_table_for_indexes(
    const std::string &statement, const std::string &table_name,
    bool fulltext_only);

std::set<std::string> check_create_user_for_authentication_plugins(
    std::string_view create_user);

bool check_create_user_for_empty_password(const std::string &create_user);

std::string convert_create_user_to_create_role(const std::string &create_user);

void add_pk_to_create_table(const std::string &statement,
                            std::string *rewritten);

bool add_pk_to_create_table_if_missing(const std::string &statement,
                                       std::string *rewritten,
                                       bool ignore_pke = true);

/**
 * Converts the given 5.6 GRANT statement to a corresponding CREATE USER
 * statement, the GRANT statement is converted to a version supported by 8.0
 * (and 5.7) server.
 *
 * The returned CREATE USER statement will always contain the IDENTIFIED WITH
 * clause. If it's missing in the input GRANT statement, the authentication
 * plugin supplied as a parameter is going to be used.
 *
 * @param grant 5.6 GRANT statement to be converted
 * @param authentication_plugin authentication plugin to use if it's missing in
 *        the given GRANT statement
 * @param[out] rewritten the input GRANT statement cleared of the clauses
 *             which are not supported by the 8.0 server
 *
 * @returns CREATE USER statement which corresponds to the input GRANT statement
 */
std::string convert_grant_to_create_user(
    const std::string &grant, const std::string &authentication_plugin,
    std::string *rewritten);

std::string strip_default_role(const std::string &create_user,
                               std::string *rewritten);

/**
 * Checks if the given SQL statement contains sensitive information (i.e.
 * passwords).
 *
 * Only the syntax of SQL statement is analysed, data is not checked.
 *
 * @param statement Statement to be checked.
 *
 * @returns true if statement contains sensitive information
 */
bool contains_sensitive_information(const std::string &statement);

/**
 * Hides all sensitive information in the given statement.
 *
 * Supported statements:
 *  - SET PASSWORD FOR
 *
 * Supported clauses:
 *  - PASSWORD.* = 'string'
 *  - PASSWORD.*('string')
 *  - .*PASSWORD = 'string'
 *  - .*PASSWORD('string')
 *  - IDENTIFIED BY 'string' [REPLACE 'string']
 *  - IDENTIFIED BY PASSWORD 'string'
 *  - IDENTIFIED BY RANDOM PASSWORD REPLACE 'string'
 *  - IDENTIFIED WITH plugin BY 'string' [REPLACE 'string']
 *  - IDENTIFIED WITH plugin BY RANDOM PASSWORD REPLACE 'string'
 *  - IDENTIFIED WITH plugin AS 'string'
 *  - IDENTIFIED {VIA|WITH} plugin {USING|AS} 'string' [OR ...]
 *  - IDENTIFIED {VIA|WITH} plugin {USING|AS} PASSWORD('string') [OR ...]
 *
 * @param statement Statement to be processed.
 * @param replacement Text to be used as a replacement.
 * @param[out] rewritten Replaced strings (quoted).
 *
 * @returns modified statement
 */
std::string hide_sensitive_information(const std::string &statement,
                                       std::string_view replacement,
                                       std::vector<std::string_view> *replaced);

struct Privilege_level_info {
  enum class Level {
    GLOBAL,
    SCHEMA,
    TABLE,
    ROUTINE,
    ROLE,
  };

  bool grant = true;
  Level level = Level::GLOBAL;
  std::string schema;
  std::string object;
  std::set<std::string> privileges;
  std::string account;
  bool with_grant = false;
};

/**
 * Parses the grant/revoke statement and extracts information about privileges.
 *
 * @param statement Statement to be checked.
 * @param[out] info Extracted information.
 *
 * @returns true if information was extracted
 * @throws std::runtime_error if statement is malformed
 */
bool parse_grant_statement(const std::string &statement,
                           Privilege_level_info *info);

/**
 * Constructs a grant/revoke statement using provided information about
 * privileges.
 *
 * NOTE: Given that parse_grant_statement() does not extract all the
 *       information, currently only schema-level privileges are supported.
 *
 * @param info Information about privileges.
 *
 * @returns Corresponding grant/revoke statement.
 */
std::string to_grant_statement(const Privilege_level_info &info);

/**
 * Checks if server with the given version supports SET_ANY_DEFINER privilege.
 *
 * @param v Version to be checked.
 *
 * @returns true If server supports this privilege.
 */
bool supports_set_any_definer_privilege(const mysqlshdk::utils::Version &v);

/**
 * Replaces first occurrence of a keyword (case insensitive comparison) with the
 * given value.
 *
 * @param stmt Statement to be checked.
 * @param keyword Keyword to be replaced (cannot contain spaces).
 * @param value Value to be replaced with.
 * @param[out] result Statement after replacement.
 *
 * @returns true If keyword was found and replaced.
 */
bool replace_keyword(std::string_view stmt, std::string_view keyword,
                     std::string_view value, std::string *result = nullptr);

}  // namespace compatibility
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_COMPATIBILITY_H_
