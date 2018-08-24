/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MODULES_ADMINAPI_DBA_PROVISION_H_
#define MODULES_ADMINAPI_DBA_PROVISION_H_

#include <memory>
#include <string>

#include "modules/adminapi/mod_dba_replicaset.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlsh {
namespace dba {

/**
 * Remove the specified instance from its current replicaset (using Group
 * Replication).
 *
 * In more detail, the following steps are executed:
 * - Stop Group Replication;
 * - Reset/disable necessary Group Replication variables on removed instance;
 *
 * NOTE: Metedata is NOT changed by this function, it only remove the instance
 *       from the MySQL Group Replication (replicaset) and update any necessary
 *       variables on the instance itself.
 *
 * @param instance target Instance object to remove from the replicaset.
 */
void leave_replicaset(const mysqlshdk::mysql::Instance &instance);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_PROVISION_H_
