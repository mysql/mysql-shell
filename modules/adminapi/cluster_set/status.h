/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_SET_STATUS_H_
#define MODULES_ADMINAPI_CLUSTER_SET_STATUS_H_

#include <map>
#include <string>
#include <vector>

#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "mysqlshdk/libs/mysql/gtid_utils.h"

namespace mysqlsh {
namespace dba {
namespace clusterset {

shcore::Dictionary_t cluster_set_status(Cluster_set_impl *cluster_set,
                                        uint64_t extended);

shcore::Dictionary_t cluster_set_describe(Cluster_set_impl *cluster_set);

}  // namespace clusterset
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_SET_STATUS_H_
