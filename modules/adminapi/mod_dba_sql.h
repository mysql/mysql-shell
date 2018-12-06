/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_MOD_DBA_SQL_H_
#define MODULES_ADMINAPI_MOD_DBA_SQL_H_

#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/mod_dba_common.h"
#include "scripting/common.h"

#define PASSWORD_LENGTH 32

namespace mysqlsh {
namespace dba {
GRInstanceType get_gr_instance_type(
    std::shared_ptr<mysqlshdk::db::ISession> connection);
void get_port_and_datadir(std::shared_ptr<mysqlshdk::db::ISession> connection,
                          int &port, std::string &datadir);
SlaveReplicationState get_slave_replication_state(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &slave_executed);
Cluster_check_info get_replication_group_state(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    GRInstanceType source_type);
ManagedInstance::State SHCORE_PUBLIC
get_instance_state(std::shared_ptr<mysqlshdk::db::ISession> connection,
                   const std::string &address);
bool is_server_on_replication_group(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &uuid);
std::string get_plugin_status(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    std::string plugin_name);
bool SHCORE_PUBLIC get_server_variable(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &name, std::string &value, bool throw_on_error = true);
bool SHCORE_PUBLIC get_server_variable(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &name, int &value, bool throw_on_error = true);
void set_global_variable(std::shared_ptr<mysqlshdk::db::ISession> connection,
                         const std::string &name, const std::string &value);
bool get_status_variable(std::shared_ptr<mysqlshdk::db::ISession> connection,
                         const std::string &name, std::string &value,
                         bool throw_on_error = true);
bool is_gtid_subset(std::shared_ptr<mysqlshdk::db::ISession> connection,
                    const std::string &subset, const std::string &set);
shcore::Value get_master_status(
    std::shared_ptr<mysqlshdk::db::ISession> connection);
std::vector<std::string> get_peer_seeds(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &instance_host);
std::vector<std::pair<std::string, int>> get_open_sessions(
    std::shared_ptr<mysqlshdk::db::ISession> connection);
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_SQL_H_
