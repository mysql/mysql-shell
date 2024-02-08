/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_VALIDATIONS_H_
#define MODULES_ADMINAPI_COMMON_VALIDATIONS_H_

#include <memory>
#include <string>

#include "modules/adminapi/common/common.h"
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
 * @param skip_check_tables_pk true to skip checking tables without PKs
 *
 * Throws an exception if checks fail.
 */
void ensure_gr_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance, bool full = true,
    bool skip_check_tables_pk = false);

void ensure_ar_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance);

/**
 * Validates the permissions of the user running the operation.
 *
 * NOTE: Even if a clusterAdmin account is meant to be created, it won't be if
 * the current user is missing privileges, so the check must always be done.
 *
 * @param instance the target instance to verify used user privileges.
 * @param purpose the purpose / context with which this method is being called.
 */
void ensure_user_privileges(
    const mysqlshdk::mysql::IInstance &instance,
    Cluster_type purpose = Cluster_type::GROUP_REPLICATION);

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

/**
 * Ensure CA Certificate options are set on the target instance
 *
 * This function verifies if the CA Certificate options --ssl-ca or --ssl-capath
 * are set when VERIFY_CA or VERIFY_IDENTIFY are used as memberSslMode
 *
 * @param instance target instance to perform the verification
 * @param ssl_mode SslMode used in
 * createCluster()/addInstance()/rejoinInstance()
 */
void ensure_certificates_set(const mysqlshdk::mysql::IInstance &instance,
                             Cluster_ssl_mode ssl_mode);

/**
 * Ensure CA Certificate options are set on the target instance
 *
 * This function verifies if the CA Certificate options --ssl-ca or --ssl-capath
 * are set when CERT_ISSUER* or CERT_SUBJECT* are used as memberAuthType
 *
 * @param instance target instance to perform the verification
 * @param member_auth member auth mode
 */
void ensure_certificates_set(const mysqlshdk::mysql::IInstance &instance,
                             Replication_auth_type auth_type);

/**
 * Check if an upgrade of the protocol is possible
 *
 * @param group_instance session to an instance member of the group
 * @param skip_server_uuid skip that server when checking
 */
void check_protocol_upgrade_possible(
    const mysqlshdk::mysql::IInstance &group_instance,
    const std::string &skip_server_uuid = "");
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_VALIDATIONS_H_
