/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_COMMON_SQL_H_
#define MODULES_ADMINAPI_COMMON_SQL_H_

#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "scripting/common.h"

#define PASSWORD_LENGTH 32

namespace mysqlsh {
namespace dba {
GRInstanceType get_gr_instance_type(
    std::shared_ptr<mysqlshdk::db::ISession> connection);
void get_port_and_datadir(std::shared_ptr<mysqlshdk::db::ISession> connection,
                          int *port, std::string *datadir);

Cluster_check_info get_replication_group_state(
    const mysqlshdk::mysql::IInstance &connection, GRInstanceType source_type);

std::vector<std::string> get_peer_seeds(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &instance_host);
std::vector<std::pair<std::string, int>> get_open_sessions(
    std::shared_ptr<mysqlshdk::db::ISession> connection);

Instance_definition query_instance_info(
    const mysqlshdk::mysql::IInstance &instance);
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_SQL_H_
