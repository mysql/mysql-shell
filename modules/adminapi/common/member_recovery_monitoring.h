/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_MEMBER_RECOVERY_MONITORING_H_
#define MODULES_ADMINAPI_COMMON_MEMBER_RECOVERY_MONITORING_H_

#include <string>

#include "modules/adminapi/common/clone_progress.h"
#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlsh {
namespace dba {

constexpr const int k_recovery_status_poll_interval_ms = 1000;
constexpr const int k_clone_status_poll_interval_ms = 500;

class stop_monitoring {};
class restart_timeout {};

mysqlshdk::gr::Group_member_recovery_status wait_recovery_start(
    const mysqlshdk::db::Connection_options &instance_def,
    const std::string &begin_time, int timeout_sec);

std::shared_ptr<mysqlsh::dba::Instance> wait_clone_start(
    const mysqlshdk::db::Connection_options &instance_def,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    const std::string &begin_time, int timeout_sec);

void monitor_distributed_recovery(const mysqlshdk::mysql::IInstance &instance,
                                  Recovery_progress_style /*progress_style*/);

void monitor_standalone_clone_instance(
    const mysqlshdk::db::Connection_options &instance_def,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    const std::string &begin_time, Recovery_progress_style progress_style,
    int startup_timeout_sec, int restart_timeout_sec);

void monitor_post_clone_gr_recovery_status(
    mysqlsh::dba::Instance *instance,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    const std::string &begin_time, Recovery_progress_style progress_style,
    int startup_timeout_sec);

void monitor_gr_recovery_status(
    const mysqlshdk::db::Connection_options &instance_def,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    const std::string &begin_time, Recovery_progress_style progress_style,
    int startup_timeout_sec, int restart_timeout_sec);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_MEMBER_RECOVERY_MONITORING_H_
