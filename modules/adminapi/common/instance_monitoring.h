/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_INSTANCE_MONITORING_H_
#define MODULES_ADMINAPI_COMMON_INSTANCE_MONITORING_H_

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"

namespace mysqlsh {
namespace dba {

constexpr const int k_server_restart_poll_interval_ms = 1000;

class stop_wait {};

/**
 * Wait for the target MySQL instance to start
 *
 * @param instance_def target instance connection options
 * @param timeout maximum value to wait for the startup, in seconds
 * @param progress_style progress style: Recovery_progress_style
 *
 * returns shared_ptr holding a mysqlsh::dba::Instance object with an open
 * session to the target instance
 */
std::shared_ptr<mysqlsh::dba::Instance> wait_server_startup(
    const mysqlshdk::db::Connection_options &instance_def, int timeout,
    Recovery_progress_style progress_style);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_INSTANCE_MONITORING_H_
