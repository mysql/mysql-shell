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
#ifndef MODULES_ADMINAPI_COMMON_ASYNC_TOPOLOGY_H_
#define MODULES_ADMINAPI_COMMON_ASYNC_TOPOLOGY_H_

#include "modules/adminapi/common/async_replication_options.h"

#include <list>
#include <string>

#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

// AR topology change functions.
// Assumes validations, metadata updates and account management are handled
// outside.

struct Scoped_instance;

/**
 * Adds a prepared instance as a replica of the given primary.
 *
 * repl_options - AR options, including repl credentials
 */
void async_add_replica(mysqlshdk::mysql::IInstance *primary,
                       mysqlshdk::mysql::IInstance *target,
                       const Async_replication_options &repl_options,
                       bool dry_run);

/**
 * Rejoins a replica to replicaset.
 *
 * More specifically, set the replicaset primary (in case it has changed)
 * and restart replication.
 *
 * NOTE: It is assumed that the replication user already exists (created when
 * adding the instance the first time), otherwise the instance needs to be
 * removed and added again.
 *
 * @param primary Instance object for the replicaset PRIMARY.
 * @param target Instance object for the target replica to rejoin.
 * @param rpl_options Async_replication_options object with the replication
 *                    user credentials to use to rejoin the target instance.
 */
void async_rejoin_replica(mysqlshdk::mysql::IInstance *primary,
                          mysqlshdk::mysql::IInstance *target,
                          const Async_replication_options &rpl_options);

/**
 * Removes a replica from the replicaset.
 */
void async_remove_replica(mysqlshdk::mysql::IInstance *target, bool dry_run);

/**
 * Promotes a secondary to primary.
 */
void async_set_primary(mysqlshdk::mysql::IInstance *current_primary,
                       mysqlshdk::mysql::IInstance *promoted,
                       const Async_replication_options &repl_options,
                       shcore::Scoped_callback_list *undo_list, bool dry_run);

/**
 * Promotes a secondary to primary, when the primary is unavailable.
 */
void async_force_primary(mysqlshdk::mysql::IInstance *promoted,
                         const Async_replication_options &repl_options,
                         bool dry_run);

/**
 * Revert the change performed by async_force_primary().
 *
 * @param promoted Instance that was going to be promoted.
 * @param dry_run bool indicate if dry run is being performed.
 */
void undo_async_force_primary(mysqlshdk::mysql::IInstance *promoted,
                              bool dry_run);

/**
 * Change the primary of one or more secondary instances.
 *
 * Reuses the same credentials as currently in use.
 */
void async_change_primary(mysqlshdk::mysql::IInstance *primary,
                          const std::list<Scoped_instance> &secondaries,
                          const Async_replication_options &repl_options,
                          mysqlshdk::mysql::IInstance *old_primary,
                          shcore::Scoped_callback_list *undo_list,
                          bool dry_run);

void wait_apply_retrieved_trx(mysqlshdk::mysql::IInstance *instance,
                              int timeout_sec);

void wait_all_apply_retrieved_trx(
    std::list<Scoped_instance> *out_instances, int timeout_sec,
    bool invalidate_error_instances,
    std::vector<Instance_metadata> *out_instances_md,
    std::list<Instance_id> *out_invalidate_ids);

void fence_instance(mysqlshdk::mysql::IInstance *instance);

void unfence_instance(mysqlshdk::mysql::IInstance *instance);

void reset_channel(mysqlshdk::mysql::IInstance *instance,
                   bool reset_credentials, bool dry_run);

void stop_channel(mysqlshdk::mysql::IInstance *instance,
                  const std::string &channel_name = "", bool dry_run = false);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_ASYNC_TOPOLOGY_H_
