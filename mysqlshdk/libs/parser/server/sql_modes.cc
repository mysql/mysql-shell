/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/parser/server/sql_modes.h"

const std::unordered_set<std::string_view> k_sql_modes_57 = {
    "ALLOW_INVALID_DATES",
    "ANSI_QUOTES",
    "ERROR_FOR_DIVISION_BY_ZERO",
    "HIGH_NOT_PRECEDENCE",
    "IGNORE_SPACE",
    "NO_AUTO_CREATE_USER",
    "NO_AUTO_VALUE_ON_ZERO",
    "NO_BACKSLASH_ESCAPES",
    "NO_DIR_IN_CREATE",
    "NO_ENGINE_SUBSTITUTION",
    "NO_FIELD_OPTIONS",
    "NO_KEY_OPTIONS",
    "NO_TABLE_OPTIONS",
    "NO_UNSIGNED_SUBTRACTION",
    "NO_ZERO_DATE",
    "NO_ZERO_IN_DATE",
    "ONLY_FULL_GROUP_BY",
    "PAD_CHAR_TO_FULL_LENGTH",
    "PIPES_AS_CONCAT",
    "REAL_AS_FLOAT",
    "STRICT_ALL_TABLES",
    "STRICT_TRANS_TABLES",
    "ANSI",
    "DB2",
    "MAXDB",
    "MSSQL",
    "MYSQL323",
    "MYSQL40",
    "ORACLE",
    "POSTGRESQL",
    "TRADITIONAL",
};

const std::string_view k_default_sql_mode_57 =
    "ONLY_FULL_GROUP_BY,"
    "STRICT_TRANS_TABLES,"
    "NO_ZERO_IN_DATE,"
    "NO_ZERO_DATE,"
    "ERROR_FOR_DIVISION_BY_ZERO,"
    "NO_AUTO_CREATE_USER,"
    "NO_ENGINE_SUBSTITUTION";

const std::unordered_set<std::string_view> k_sql_modes_80 = {
    "ALLOW_INVALID_DATES",
    "ANSI_QUOTES",
    "ERROR_FOR_DIVISION_BY_ZERO",
    "HIGH_NOT_PRECEDENCE",
    "IGNORE_SPACE",
    "NO_AUTO_VALUE_ON_ZERO",
    "NO_BACKSLASH_ESCAPES",
    "NO_DIR_IN_CREATE",
    "NO_ENGINE_SUBSTITUTION",
    "NO_UNSIGNED_SUBTRACTION",
    "NO_ZERO_DATE",
    "NO_ZERO_IN_DATE",
    "ONLY_FULL_GROUP_BY",
    "PAD_CHAR_TO_FULL_LENGTH",
    "PIPES_AS_CONCAT",
    "REAL_AS_FLOAT",
    "STRICT_ALL_TABLES",
    "STRICT_TRANS_TABLES",
    "TIME_TRUNCATE_FRACTIONAL",
    "ANSI",
    "TRADITIONAL",
};

const std::string_view k_default_sql_mode_80 =
    "ONLY_FULL_GROUP_BY,"
    "STRICT_TRANS_TABLES,"
    "NO_ZERO_IN_DATE,"
    "NO_ZERO_DATE,"
    "ERROR_FOR_DIVISION_BY_ZERO,"
    "NO_ENGINE_SUBSTITUTION";
