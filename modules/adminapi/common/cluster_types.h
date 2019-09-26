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

#ifndef MODULES_ADMINAPI_COMMON_CLUSTER_TYPES_H_
#define MODULES_ADMINAPI_COMMON_CLUSTER_TYPES_H_

#include <string>

namespace mysqlsh {
namespace dba {

using Cluster_id = std::string;
using Instance_id = uint32_t;

enum class Global_topology_type {
  NONE,                // not a replicated/async cluster
  SINGLE_PRIMARY_TREE  // 1 master/active, 1 or more slaves
};

std::string to_string(Global_topology_type type);
Global_topology_type to_topology_type(const std::string &s);

enum class Cluster_type { NONE, GROUP_REPLICATION, ASYNC_REPLICATION };

std::string to_string(Cluster_type type);

enum class Display_form {
  THING,         // cluster, replicaset
  A_THING,       // a cluster, a replicaset
  THINGS,        // clusters, replicasets
  THING_FULL,    // InnoDB Cluster, InnoDB ReplicaSet
  A_THING_FULL,  // an InnoDB Cluster, a ReplicaSet
  THINGS_FULL    // InnoDB Clusters, ReplicaSets
};
std::string to_display_string(Cluster_type type, Display_form form);

inline std::string thing(Cluster_type type) {
  return to_display_string(type, Display_form::THING);
}

inline std::string a_thing(Cluster_type type) {
  return to_display_string(type, Display_form::A_THING);
}

inline std::string things(Cluster_type type) {
  return to_display_string(type, Display_form::THINGS);
}

}  // namespace dba
}  // namespace mysqlsh

#endif  //  MODULES_ADMINAPI_COMMON_CLUSTER_TYPES_H_