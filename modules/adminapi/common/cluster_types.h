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

#ifndef MODULES_ADMINAPI_COMMON_CLUSTER_TYPES_H_
#define MODULES_ADMINAPI_COMMON_CLUSTER_TYPES_H_

#include <string>

#include "mysqlshdk/libs/utils/enumset.h"

namespace mysqlsh {
namespace dba {

using Cluster_set_id = std::string;
using Cluster_id = std::string;
using Instance_id = uint32_t;

enum class Global_topology_type {
  NONE,                // not a replicated/async cluster
  SINGLE_PRIMARY_TREE  // 1 master/active, 1 or more slaves
};

std::string to_string(Global_topology_type type);
Global_topology_type to_topology_type(const std::string &s);

enum class Cluster_set_global_status {
  HEALTHY,    // Primary Cluster has ClusterSet-wise Status OK and ALL Replica
              // Clusters have ClusterSet-wise Status OK
  AVAILABLE,  // Primary Cluster has ClusterSet-wise Status OK and 1 or more
              // Replica Cluster have any ClusterSet-wise status that is not OK
  UNAVAILABLE,  // Primary Cluster has ClusterSet-wise Status UNAVAILABLE
  UNKNOWN       // Primary Cluster has ClusterSet-wise Status UNKNOWN
};

std::string to_string(Cluster_set_global_status type);

enum class Cluster_type {
  NONE,
  GROUP_REPLICATION,  // InnoDB Cluster
  ASYNC_REPLICATION,  // InnoDB ReplicaSet
  REPLICATED_CLUSTER  // InnoDB ClusterSet
};

std::string to_string(Cluster_type type);

enum class Cluster_status {
  OK,                       // All members ONLINE, >=3 members
  OK_PARTIAL,               // >=3 members ONLINE, some other members down
  OK_NO_TOLERANCE,          // All members ONLINE, <3 members
  OK_NO_TOLERANCE_PARTIAL,  // <3 members ONLINE, some other members down
  NO_QUORUM,                // Group has no quorum
  OFFLINE,                  // No members ONLINE (but all reachable)
  INVALIDATED,              // Part of a ClusterSet but is invalidated
  ERROR,                    // Group Replication Error
  UNKNOWN                   // No members ONLINE (some or all unreachable)
};

std::string to_string(Cluster_status state);
Cluster_status to_cluster_status(const std::string &s);

enum class Cluster_global_status {
  OK,  // If it's a Primary, it must have any of the OK_* status. If a Replica,
       // any of the OK_* status plus the Replication Channel status must be OK
  OK_NOT_REPLICATING,  // Replica Cluster with any of the OK_* status and
                       // Replication Channel status STOPPED or ERROR
  OK_NOT_CONSISTENT,   // Replica Cluster with any of the OK_* status and
                       // Replication Channel status OK or ERROR + diagnostic
                       // INCONSISTENT
  OK_MISCONFIGURED,    // Replica Cluster with any of the OK_* status and
                       // Replication Channel status OK or ERROR + diagnostic
                       // MISCONFIGURED
  NOT_OK,              // Primary Cluster has status NO_QUORUM or OFFLINE, or a
                       // Replica Cluster with status NO_QUORUM or OFFLINE
  INVALIDATED,         // The cluster was invalidated by a failover.
  UNKNOWN              // If it's a Primary Cluster with status UNKNOWN.
};

std::string to_string(Cluster_global_status status);

enum class Cluster_channel_status {
  OK,       // Replication channel up and running
  STOPPED,  // Replication channel stopped gracefully. Either both IO and SQL
            // threads or just one of them.
  ERROR,    // Replication channel stopped due to a replication error (e.g.
            // conflicting GTID-set)
  MISCONFIGURED,  // Channel exists but is replicating from the wrong place
  MISSING,        // Channel doesn't exist
  UNKNOWN  // Shell cannot connect to the Replica Cluster to obtain information
           // about the replication channel and others
};

std::string to_string(Cluster_channel_status status);

enum class Display_form {
  THING,         // cluster, replicaset
  A_THING,       // a cluster, a replicaset
  THINGS,        // clusters, replicasets
  THING_FULL,    // InnoDB Cluster, InnoDB ReplicaSet
  A_THING_FULL,  // an InnoDB Cluster, a ReplicaSet
  THINGS_FULL,   // InnoDB Clusters, ReplicaSets
  API_CLASS,     // Cluster, ReplicaSet
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

inline std::string api_class(Cluster_type type) {
  return to_display_string(type, Display_form::API_CLASS);
}

}  // namespace dba
}  // namespace mysqlsh

#endif  //  MODULES_ADMINAPI_COMMON_CLUSTER_TYPES_H_
