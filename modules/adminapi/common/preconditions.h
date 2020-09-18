/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_
#define MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_

#include <memory>
#include <string>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlsh {
namespace dba {

class Instance;
class MetadataStorage;

// The AdminAPI maximum supported MySQL Server version
const mysqlshdk::utils::Version k_max_adminapi_server_version =
    mysqlshdk::utils::Version("8.1");

// The AdminAPI minimum supported MySQL Server version
const mysqlshdk::utils::Version k_min_adminapi_server_version =
    mysqlshdk::utils::Version("5.7");

// Specific minimum versions for GR and AR functions
const mysqlshdk::utils::Version k_min_gr_version =
    mysqlshdk::utils::Version("5.7");
const mysqlshdk::utils::Version k_min_ar_version =
    mysqlshdk::utils::Version("8.0");

void validate_session(const std::shared_ptr<mysqlshdk::db::ISession> &session);

void check_preconditions(const std::string &function_name,
                         const Cluster_check_info &info,
                         FunctionAvailability *custom_func_avail = nullptr);

Cluster_check_info get_cluster_check_info(const MetadataStorage &group_server);

// NOTE: BUG#30628479 is applicable to all the API functions and the root cause
// is that several instances of the Metadata class are created during a function
// execution.
// To solve this, all the API functions should create a single instance of the
// Metadata class and pass it down through the chain call. In other words, the
// following function should be deprecated in favor of the version below.
Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<Instance> &group_server,
    FunctionAvailability *custom_func_avail = nullptr);

// All fun
Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<MetadataStorage> &metadata,
    FunctionAvailability *custom_func_avail = nullptr);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_
