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

#ifndef MODULES_ADMINAPI_COMMON_ACCOUNTS_H_
#define MODULES_ADMINAPI_COMMON_ACCOUNTS_H_

#include "modules/adminapi/common/cluster_types.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlsh {
namespace dba {

std::pair<int, int> find_cluster_admin_accounts(
    const mysqlshdk::mysql::IInstance &instance, const std::string &admin_user,
    std::vector<std::string> *out_hosts);

bool validate_cluster_admin_user_privileges(
    const mysqlshdk::mysql::IInstance &instance, const std::string &admin_user,
    const std::string &admin_host, Cluster_type purpose,
    std::string *validation_error);

std::vector<std::string> create_admin_grants(
    const std::string &username,
    const mysqlshdk::utils::Version &instance_version);

std::vector<std::string> create_router_grants(
    const std::string &username,
    const mysqlshdk::utils::Version &metadata_version);

bool check_admin_account_access_restrictions(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::string &host, bool interactive, Cluster_type purpose);

std::string prompt_new_account_password();

std::pair<std::string, std::string> validate_account_name(
    const std::string &account);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_ACCOUNTS_H_
