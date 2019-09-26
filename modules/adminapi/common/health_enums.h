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

#ifndef MODULES_ADMINAPI_COMMON_HEALTH_ENUMS_H_
#define MODULES_ADMINAPI_COMMON_HEALTH_ENUMS_H_

// This file contains enums that describe the state of instances, groups,
// replication channels individually and also of the status of these components
// when combined in various topologies.
//
// Some enums are meant to provide a high-level view of the status of health
// of the cluster to the end user.
//
// Others are meant to be used internally to guide diagnostics and for the
// derivation of less specific status values.
//
// Most of the the complexity here is because there's a gap between the
// observable state of the cluster and the actual intended configuration of the
// cluster. We can't put everything about the intended configuration in the
// metadata because the metadata will not always be updatable or reliable.
// For example, we can't put the active cluster in the metadata because
// we won't be able to update the metadata when the active cluster is dead.
// The observable state of a cluster is also not always reliable, since an
// unreachable cluster could be running fine, even if we can't connect to it.
//
// The only way to close the gap is by having an external entity that can
// resolve ambiguities, either a human admin at the shell prompt or some
// service/component.
//

#include <string>
#include <vector>

#undef ERROR  // for windows

namespace mysqlsh {
namespace dba {

// Instance specific status (relative to its group), in order of
// seriousness.
enum class Instance_status {
  INVALID,        // The instance is not part of the cluster
  UNREACHABLE,    // Can't connect to the instance
  MISSING,        // Not in a quorum and no GR either
  OUT_OF_QUORUM,  // Instance is not part of a group quorum
  RECOVERING,     // Everything OK with the instance but it's RECOVERING
  OK              // Everything OK with the instance and is ONLINE
};

std::string to_string(Instance_status status);
#if 0
namespace channel {
// Channel specific health status (in isolation)
enum class Status {
  NONE,              // Channel does not exist
  STOPPED,           // Channel exists but is stopped
  APPLIER_ERROR,     // applier (or coordinator) error
  CONNECTION_ERROR,  // connection thread error
  RUNNING            // Started and OK
};

}  // namespace channel

// Replicaset specific health status, from more serious to less
// Derived from Instance_health and the GR group status
enum class Group_status {
  UNREACHABLE,  // none of the members in the group are reachable from the shell
  INVALIDATED,  // cluster was forced out of the global topology
  UNAVAILABLE,  // none of the members in the group are ONLINE
  NO_QUORUM,    // group is reachable but has no quorum
  SPLIT_BRAIN,  // more than 1 partition of the group has quorum
  OK            // group topology is OK and can tolerate at least 1 failure
};

enum class Group_health {
  UNREACHABLE,  // none of the members in the group are reachable from the shell
  INVALIDATED,  // cluster was forced out of the global topology
  UNAVAILABLE,  // none of the members in the group are ONLINE
  NO_QUORUM,    // group is reachable but has no quorum
  SPLIT_BRAIN,  // more than 1 partition of the group has quorum
  OK_PARTIAL,   // group is OK but some members are not ONLINE
  OK_NO_TOLERANCE,  // group topology is OK but can't handle failures
  OK                // group topology is OK and can tolerate at least 1 failure
};

std::string to_string(Group_status status);
std::string to_string(Group_health health);

namespace replicaset {

// The status of the replicaset channels relative to it's master
enum class Slave_health {
  UNAVAILABLE,       // The replicaset has no quorum or is unreachable
  TOO_MANY_SLAVES,   // >1 running slaves found
  MISSING_SLAVE,     // slave expected, but there is none
  UNEXPECTED_SLAVE,  // no slave expected, but there is one or more running
  SLAVE_ERROR,       // channel has some replication error
  SLAVE_STOPPED,     // required slave is configured but not running
  WRONG_SLAVE,       // slave is running from the wrong instance
  INVALID_MASTER,    // master instance is not OK/RECOVERING
  OK                 // all OK and no unexpected slaves
};

}  // namespace replicaset

// ----------------------------------------------------------------------------
// The status enums below are very high level statuses. These are the statuses
// that matter at a business level. It should be possible to summarize
// everything else with these.

// Individual, low-level issues will not be reflected in these enums unless
// they have a global effect.

// General integrity status of the global topology
// Safe global changes are only allowed if OK or WARNING
enum class Health_status {
  OK = 0,    // no errors that affect the global topology
  WARNING,   // non-fatal errors in the topology
  ERROR,     // one or more errors in the topology
  CRITICAL,  // critical errors in the topology (inconsistencies possible)
  FATAL      // no available replicasets
};

// Worse is bigger
inline bool operator>(Health_status a, Health_status b) {
  return static_cast<int>(a) > static_cast<int>(b);
}

inline bool operator>=(Health_status a, Health_status b) {
  return static_cast<int>(a) >= static_cast<int>(b);
}

inline bool operator<(Health_status a, Health_status b) {
  return static_cast<int>(a) < static_cast<int>(b);
}

inline bool operator<=(Health_status a, Health_status b) {
  return static_cast<int>(a) <= static_cast<int>(b);
}
#endif

// Availability status of the cluster as a whole, regardless of local issues.
// Global topology changes are only possible if AVAILABLE_PARTIAL or better.
enum class Global_availability_status {
  CATASTROPHIC,       // All groups unavailable, no (other) instances visible
  UNAVAILABLE,        // No available groups visible, but restore possible
  AVAILABLE_PARTIAL,  // At least one active group visible
  AVAILABLE           // All groups available
};

std::string to_string(Global_availability_status status);
std::string explain(Global_availability_status status);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_HEALTH_ENUMS_H_
