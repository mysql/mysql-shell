/*
 * Copyright (c) 2023, 2025, Oracle and/or its affiliates.
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
    "mysql_option_tracker_persister",
    "mysql_rest_service_admin",
    "mysql_rest_service_data_provider",
    "mysql_rest_service_dev",
    "mysql_rest_service_meta_provider",
    "mysql_rest_service_schema_admin",
    "mysql_rest_service_user",
    "mysql_task_admin",
    "mysql_task_user",
    "ociadmin",
    "ocidbm",
    "ocimonitor",
    "ocirpl",
    "oracle-cloud-agent",
    "rrhhuser",
};

constexpr inline std::array k_excluded_schemas = {
    "information_schema", "mysql", "ndbinfo", "performance_schema", "sys",
};

constexpr inline std::array k_mhs_excluded_schemas = {
    // stores the audit plugin's configuration
    "mysql_audit",
    // stores the autopilot's information
    "mysql_autopilot",
    // stores the firewall's configuration
    "mysql_firewall",
    // stores the options information
    "mysql_option",
    // stores the MRS metadata
    "mysql_rest_service_metadata",
    // MRS related
    "mysql_tasks",
};

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COMMON_DUMP_CONSTANTS_H_
