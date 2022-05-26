/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_INSTANCE_VALIDATIONS_H_
#define MODULES_ADMINAPI_COMMON_INSTANCE_VALIDATIONS_H_

#include <memory>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/repl_config.h"

namespace mysqlsh {
namespace dba {
namespace checks {

void validate_host_address(const mysqlshdk::mysql::IInstance &instance,
                           int verbose);

bool validate_schemas(const mysqlshdk::mysql::IInstance &instance,
                      bool skip_check_tables_pk);

void validate_innodb_page_size(mysqlshdk::mysql::IInstance *instance);

std::vector<mysqlshdk::mysql::Invalid_config> validate_configuration(
    mysqlshdk::mysql::IInstance *instance, const std::string &mycnf_path,
    mysqlshdk::config::Config *const config, Cluster_type cluster_type,
    const mysqlshdk::utils::nullable<bool> &can_persist, bool *restart_needed,
    bool *mycnf_change_needed, bool *sysvar_change_needed,
    shcore::Value *ret_val = nullptr);

/**
 * Validate that the target instance has the performance_schema enabled
 *
 * @param  instance target instance to validate
 */
void validate_performance_schema_enabled(
    const mysqlshdk::mysql::IInstance &instance);

void ensure_instance_not_belong_to_cluster(
    const std::shared_ptr<Instance> &instance,
    const std::shared_ptr<Instance> &cluster_instance,
    const std::string &cluster_id);

void ensure_instance_not_belong_to_metadata(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &address_in_metadata,
    const mysqlsh::dba::Cluster_impl &cluster);

enum class Check_type { CHECK, CREATE, BOOTSTRAP, JOIN, REJOIN };

size_t check_illegal_async_channels(
    const mysqlshdk::mysql::IInstance &instance,
    const std::unordered_set<std::string> &allowed_channels_);

void validate_async_channels(
    const mysqlshdk::mysql::IInstance &instance,
    const std::unordered_set<std::string> &allowed_channels, Check_type type);

}  // namespace checks
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_INSTANCE_VALIDATIONS_H_
