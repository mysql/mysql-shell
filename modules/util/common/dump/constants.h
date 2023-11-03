/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_COMMON_DUMP_CONSTANTS_H_
#define MODULES_UTIL_COMMON_DUMP_CONSTANTS_H_

#include <array>

namespace mysqlsh {
namespace dump {
namespace common {

constexpr inline std::array k_excluded_users = {
    "mysql.infoschema",
    "mysql.session",
    "mysql.sys",
};

constexpr inline std::array k_mhs_excluded_users = {
    "administrator",
    "ociadmin",
    "ocimonitor",
    "ocirpl",
};

constexpr inline std::array k_excluded_schemas = {
    "information_schema", "mysql", "ndbinfo", "performance_schema", "sys",
};

constexpr inline std::array k_mhs_excluded_schemas = {
    // stores the audit plugin's configuration
    "mysql_audit",
    // stores the firewall's configuration
    "mysql_firewall",
};

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COMMON_DUMP_CONSTANTS_H_
