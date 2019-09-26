/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_COMMON_VALIDATIONS_H_
#define MODULES_ADMINAPI_COMMON_VALIDATIONS_H_

#include <memory>

#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlsh {
namespace dba {

/**
 * Ensure that the configuration of the target instance is valid for GR.
 *
 * Similar to a manual call to dba.checkInstance(), but with fewer checks
 * (e.g. no checks for schema objects).
 *
 * @param target_instance the target to check (sysvars must be cached)
 * @param full if false, executes a reduced set of tests.
 *
 * Throws an exception if checks fail.
 */
void ensure_gr_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance, bool full = true);

void ensure_ar_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance);

/**
 * Validates the permissions of the user running the operation.
 *
 * NOTE: Even if a clusterAdmin account is meant to be created, it won't be if
 * the current user is missing privileges, so the check must always be done.
 *
 * @param instance the target instance to verify used user privileges.
 */
void ensure_user_privileges(const mysqlshdk::mysql::IInstance &instance);

/**
 * Throw exception (or show warning if fatal is false) if instance needs any
 * transactions that were purged.
 */
bool ensure_gtid_sync_possible(const mysqlshdk::mysql::IInstance &master,
                               const mysqlshdk::mysql::IInstance &instance,
                               bool fatal);

/**
 * Throw exception (or show warning if fatal is false) if instance has
 * transactions that don't exist in master.
 */
bool ensure_gtid_no_errants(const mysqlshdk::mysql::IInstance &master,
                            const mysqlshdk::mysql::IInstance &instance,
                            bool fatal);
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_VALIDATIONS_H_
