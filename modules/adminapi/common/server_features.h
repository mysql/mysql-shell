/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_SERVER_FEATURES_H_
#define MODULES_ADMINAPI_COMMON_SERVER_FEATURES_H_

#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh::dba {

// The 1st version where the remote clone plugin became available
inline const mysqlshdk::utils::Version k_mysql_clone_plugin_initial_version(8,
                                                                            0,
                                                                            17);

inline const mysqlshdk::utils::Version
    k_mysql_communication_stack_initial_version(8, 0, 27);

inline const mysqlshdk::utils::Version k_paxos_single_leader_initial_version(
    8, 0, 31);

inline const mysqlshdk::utils::Version k_view_change_uuid_deprecated(8, 3, 0);

inline const mysqlshdk::utils::Version
    k_transaction_writeset_extraction_removed(8, 3, 0);

// Feature getters

std::string get_communication_stack(
    const mysqlshdk::mysql::IInstance &target_instance);

int64_t get_transaction_size_limit(
    const mysqlshdk::mysql::IInstance &target_instance);

std::optional<bool> get_paxos_single_leader_enabled(
    const mysqlshdk::mysql::IInstance &target_instance);

// Feature version checks

bool supports_mysql_communication_stack(
    const mysqlshdk::utils::Version &version);

bool supports_paxos_single_leader(const mysqlshdk::utils::Version &version);

bool supports_mysql_clone(const mysqlshdk::utils::Version &version);

bool supports_repl_channel_compression(
    const mysqlshdk::utils::Version &version);

bool supports_repl_channel_network_namespace(
    const mysqlshdk::utils::Version &version);

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_SERVER_FEATURES_H_
