/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_GTID_VALIDATIONS_H_
#define MODULES_ADMINAPI_COMMON_GTID_VALIDATIONS_H_

#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlsh {
namespace dba {

enum class Member_op_action { ADD_INSTANCE, REJOIN_INSTANCE };

/**
 * Validate/sets the instance recovery method
 *
 * This function validates the instance recovery method when used in the
 * 'recoveryMethod' option or sets the right recovery method when
 * 'recoveryMethod' is 'auto' (default).
 *
 * The validations apply for both addInstance() and rejoinInstance() commands of
 * both InnoDB cluster and ReplicaSet
 *
 * @param cluster_type Cluster_type object that indicates the type of cluster
 * @param op_action Member_op_action object that indicated the type of action
 * (add/rejoin)
 * @param donor_instance Instance object that points to the cluster session
 * instance
 * @param target_instance Instance object that points to the target instance
 * @param check_recoverable Function pointer to the function that determines
 * whether an instance can recover from the cluster or not
 * @param check_replica_gtid_state
 * @param opt_recovery_method Member_recovery_method that points to the recovery
 * method set in 'recoveryMethod'
 * @param gtid_set_is_complete boolean value indicating if the GTID-set was
 * marked as complete in the cluster
 * @param interactive boolean indicating if interactive mode is used
 * @param clone_disabled boolean indicating if clone was disabled on the cluster
 * (or not supported)
 *
 * @return a Member_recovery_method object with the recovery method
 * validated/set
 */
Member_recovery_method validate_instance_recovery(
    Cluster_type cluster_type, Member_op_action op_action,
    mysqlshdk::mysql::IInstance *donor_instance,
    mysqlshdk::mysql::IInstance *target_instance,
    const std::function<bool(mysqlshdk::mysql::IInstance *)> &check_recoverable,
    Member_recovery_method opt_recovery_method, bool gtid_set_is_complete,
    bool interactive, bool clone_disabled = false);

mysqlshdk::mysql::Replica_gtid_state check_replica_group_gtid_state(
    const mysqlshdk::mysql::IInstance &source,
    const mysqlshdk::mysql::IInstance &replica, std::string *out_missing_gtids,
    std::string *out_errant_gtids);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_GTID_VALIDATIONS_H_
