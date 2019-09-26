/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_COMMON_ASYNC_UTILS_H_
#define MODULES_ADMINAPI_COMMON_ASYNC_UTILS_H_

#include "modules/adminapi/common/instance_pool.h"

namespace mysqlsh {
namespace dba {

/** Executes a FTWRL on all instances in the replicaset, after a GTID sync.
 *
 * Locks are held in sessions that are owned by the object. Both locks and
 * sessions are released at destruction.
 */
class Global_locks {
 public:
  void acquire(const std::list<Scoped_instance> &instances,
               const std::string &master_uuid, uint32_t gtid_sync_timeout,
               bool dry_run);
  ~Global_locks();

 private:
  Scoped_instance m_master;
  std::list<Scoped_instance> m_slaves;

  void sync_and_lock_all(const std::list<Scoped_instance> &instances,
                         const std::string &master_uuid,
                         uint32_t gtid_sync_timeout, bool dry_run);
};

}  // namespace  dba
}  // namespace  mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_ASYNC_UTILS_H_
