/*
 * Copyright (c) 2016, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_SQL_H_
#define MODULES_ADMINAPI_COMMON_SQL_H_

#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"

namespace mysqlsh {
namespace dba {

TargetType::Type get_gr_instance_type(
    const mysqlshdk::mysql::IInstance &instance);

void get_port_and_datadir(const mysqlshdk::mysql::IInstance &instance,
                          int *port, std::string *datadir);

Cluster_check_info get_replication_group_state(
    const mysqlshdk::mysql::IInstance &connection,
    TargetType::Type source_type);

std::vector<std::pair<std::string, int>> get_open_sessions(
    const mysqlshdk::mysql::IInstance &instance);

Instance_metadata query_instance_info(
    const mysqlshdk::mysql::IInstance &instance, bool ignore_gr_endpoint,
    Instance_type instance_type = Instance_type::GROUP_MEMBER);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_SQL_H_
