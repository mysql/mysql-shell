/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mysqlsh {
namespace compatibility {

/// Checks grant statement for presence of privileges, returns found ones.
std::vector<std::string> check_privileges(
    const std::string grant, std::string *rewritten_grant = nullptr,
    const std::set<std::string> &privileges = {"SUPER", "FILE", "RELOAD",
                                               "BINLOG_ADMIN"});

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

}  // namespace compatibility
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_COMPATIBILITY_H_
