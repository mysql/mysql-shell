/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/enumset.h"

namespace mysqlsh {
namespace dba {

class Instance;
class MetadataStorage;

struct NewInstanceInfo {
  std::string member_id;
  std::string host;
  int port;
  std::string version;
};

struct MissingInstanceInfo {
  std::string id;
  std::string label;
  std::string endpoint;
};

// TODO(rennox): This should be renamed to simply InstanceType as it is
// no longer exclusive to GR.
namespace GRInstanceType {
enum Type {
  Standalone = 1 << 0,
  GroupReplication = 1 << 1,
  InnoDBCluster = 1 << 2,
  StandaloneWithMetadata = 1 << 3,
  StandaloneInMetadata = 1 << 4,
  AsyncReplicaSet = 1 << 5,
  Unknown = 1 << 6
};
}

namespace ManagedInstance {
enum State {
  OnlineRW = 1 << 0,
  OnlineRO = 1 << 1,
  Recovering = 1 << 2,
  Unreachable = 1 << 3,
  Offline = 1 << 4,
  Error = 1 << 5,
  Missing = 1 << 6,
  Any =
      OnlineRO | OnlineRW | Recovering | Unreachable | Offline | Error | Missing
};

std::string describe(State state);
}  // namespace ManagedInstance

namespace ReplicationQuorum {
enum class States {
  All_online = 1 << 0,
  Normal = 1 << 1,
  Quorumless = 1 << 2,
  Dead = 1 << 3,
};

using State = mysqlshdk::utils::Enum_set<States, States::Dead>;

}  // namespace ReplicationQuorum

struct Cluster_check_info {
  // Server version from the instance from which the data was consulted
  mysqlshdk::utils::Version source_version;

  // The state of the cluster from the quorum point of view
  // Supports multiple states i.e. Normal | All_online
  ReplicationQuorum::State quorum;

  // The configuration type of the instance from which the data was consulted
  GRInstanceType::Type source_type;

  // The state of the instance from which the data was consulted
  ManagedInstance::State source_state;
};

void validate_session(const std::shared_ptr<mysqlshdk::db::ISession> &session);

void check_preconditions(const std::string &function_name,
                         const Cluster_check_info &info);

Cluster_check_info get_cluster_check_info(
    const std::shared_ptr<Instance> &group_server);

Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<Instance> &group_server);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_PRECONDITIONS_H_
