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

#ifndef MODULES_ADMINAPI_REPLICA_SET_REPLICA_SET_STATUS_H_
#define MODULES_ADMINAPI_REPLICA_SET_REPLICA_SET_STATUS_H_

#include "modules/adminapi/common/global_topology.h"

namespace mysqlsh {
namespace dba {

struct Status_options {
  bool show_members = false;
  int show_details = 0;
};

shcore::Dictionary_t replica_set_status(
    const topology::Server_global_topology &topology,
    const Status_options &opts);

shcore::Dictionary_t channel_status(const topology::Server &server,
                                    const topology::Instance &instance,
                                    const Status_options &opts);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICA_SET_REPLICA_SET_STATUS_H_
