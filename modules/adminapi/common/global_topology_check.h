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

#ifndef MODULES_ADMINAPI_COMMON_GLOBAL_TOPOLOGY_CHECK_H_
#define MODULES_ADMINAPI_COMMON_GLOBAL_TOPOLOGY_CHECK_H_

#include <memory>
#include <string>
#include "modules/adminapi/common/global_topology.h"

namespace mysqlsh {
namespace dba {

void validate_node_status(const topology::Node *node);

void validate_global_topology_active_cluster_available(
    const topology::Global_topology &topology);

void validate_global_topology_consistent(
    const topology::Global_topology &topology);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_GLOBAL_TOPOLOGY_CHECK_H_
