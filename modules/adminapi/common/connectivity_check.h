/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_CONNECTIVITY_CHECK_H_
#define MODULES_ADMINAPI_COMMON_CONNECTIVITY_CHECK_H_

#include <string>

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlsh {
namespace dba {

void test_self_connection(const mysqlshdk::mysql::IInstance &instance,
                          std::string_view local_address,
                          Cluster_ssl_mode ssl_mode,
                          Replication_auth_type member_auth_type,
                          std::string_view cert_issuer,
                          std::string_view cert_subject,
                          std::string_view comm_stack);

void test_peer_connection(
    const mysqlshdk::mysql::IInstance &from_instance,
    std::string_view from_local_address, std::string_view from_cert_subject,
    const mysqlshdk::mysql::IInstance &to_instance,
    std::string_view to_local_address, std::string_view to_cert_subject,
    Cluster_ssl_mode ssl_mode, Replication_auth_type member_auth,
    std::string_view cert_issuer, std::string_view comm_stack,
    bool skip_self_check = false);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_CONNECTIVITY_CHECK_H_
