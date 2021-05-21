/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_COMPATIBILITY_H_
#define MODULES_UTIL_DUMP_COMPATIBILITY_H_

#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mysqlsh {
namespace compatibility {

extern const std::set<std::string> k_mysqlaas_allowed_privileges;

extern const std::set<std::string> k_mysqlaas_allowed_authentication_plugins;

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

std::vector<std::string> check_statement_for_charset_option(
    const std::string &statement, std::string *rewritten = nullptr,
    const std::vector<std::string> &whitelist = {"utf8mb4"});

std::string check_statement_for_definer_clause(
    const std::string &statement, std::string *rewritten = nullptr);

bool check_statement_for_sqlsecurity_clause(const std::string &statement,
                                            std::string *rewritten = nullptr);

std::vector<std::string> check_create_table_for_indexes(
    const std::string &statement, bool fulltext_only,
    std::string *rewritten = nullptr, bool return_alter_table = false);

std::string check_create_user_for_authentication_plugin(
    const std::string &create_user,
    const std::set<std::string> &plugins =
        k_mysqlaas_allowed_authentication_plugins);

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

}  // namespace compatibility
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_COMPATIBILITY_H_
