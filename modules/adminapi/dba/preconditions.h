/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_DBA_PRECONDITIONS_H_
#define MODULES_ADMINAPI_DBA_PRECONDITIONS_H_

#include <memory>
#include <string>
#include "mysqlshdk/libs/db/session.h"

namespace mysqlsh {
namespace dba {

enum GRInstanceType {
  Standalone = 1 << 0,
  GroupReplication = 1 << 1,
  InnoDBCluster = 1 << 2,
  StandaloneWithMetadata = 1 << 3,
  Unknown = 1 << 4
};

enum class SlaveReplicationState { New, Recoverable, Diverged, Irrecoverable };

struct NewInstanceInfo {
  std::string member_id;
  std::string host;
  int port;
};

struct MissingInstanceInfo {
  std::string id;
  std::string label;
  std::string host;
};

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
};  // namespace ManagedInstance

namespace ReplicationQuorum {
enum State {
  Normal = 1 << 0,
  Quorumless = 1 << 1,
  Dead = 1 << 2,
  Any = Normal | Quorumless | Dead
};
}

struct Cluster_check_info {
  // The state of the cluster from the quorum point of view
  ReplicationQuorum::State quorum;

  // The UIUD of the master instance
  std::string master;

  // The UUID of the instance from which the data was consulted
  std::string source;

  // The configuration type of the instance from which the data was consulted
  GRInstanceType source_type;

  // The state of the instance from which the data was consulted
  ManagedInstance::State source_state;
};

void validate_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session);

Cluster_check_info get_cluster_check_info(
    const std::shared_ptr<mysqlshdk::db::ISession> &group_session);

void check_preconditions(const std::string &function_name,
                         const Cluster_check_info &info);

Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<mysqlshdk::db::ISession> &group_session);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_PRECONDITIONS_H_
