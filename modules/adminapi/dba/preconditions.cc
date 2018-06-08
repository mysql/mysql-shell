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

#include "modules/adminapi/dba/preconditions.h"
#include <map>

#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "mysqlshdk/libs/db/utils_error.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlsh {
namespace dba {

namespace {
// The AdminAPI maximum supported MySQL Server version
const mysqlshdk::utils::Version k_max_adminapi_server_version =
    mysqlshdk::utils::Version("8.1");

// The AdminAPI minimum supported MySQL Server version
const mysqlshdk::utils::Version k_min_adminapi_server_version =
    mysqlshdk::utils::Version("5.7");

// Note that this structure may be initialized using initializer
// lists, so the order of the fields is very important
struct FunctionAvailability {
  int instance_config_state;
  int cluster_status;
  int instance_status;
};

// TODO(alfredo) - this map should be turned into a compile-time lookup table
const std::map<std::string, FunctionAvailability>
    AdminAPI_function_availability = {
        // The Dba functions
        {"Dba.createCluster",
         {GRInstanceType::Standalone | GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::GroupReplication,
          ReplicationQuorum::State::Any, ManagedInstance::State::Any}},
        {"Dba.getCluster",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Any,
          ManagedInstance::State::Any}},
        {"Dba.dropMetadataSchema",
         {GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::Normal, ManagedInstance::State::OnlineRW}},
        {"Dba.rebootClusterFromCompleteOutage",
         {GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::Any,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO}},
        {"Dba.configureLocalInstance",
         {GRInstanceType::Standalone | GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::InnoDBCluster | GRInstanceType::Unknown,
          ReplicationQuorum::State::Any, ManagedInstance::State::Any}},
        {"Dba.checkInstanceConfiguration",
         {GRInstanceType::Standalone | GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::Unknown,
          ReplicationQuorum::State::Any, ManagedInstance::State::Any}},
        {"Dba.configureInstance",
         {GRInstanceType::Standalone | GRInstanceType::StandaloneWithMetadata,
          ReplicationQuorum::State::Any, ManagedInstance::State::Any}},

        // The Replicaset/Cluster functions
        {"Cluster.addInstance",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal,
          ManagedInstance::State::OnlineRW}},
        {"Cluster.removeInstance",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal,
          ManagedInstance::State::OnlineRW}},
        {"Cluster.rejoinInstance",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO}},
        {"Cluster.describe",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Any,
          ManagedInstance::State::Any}},
        {"Cluster.status",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Any,
          ManagedInstance::State::Any}},
        {"Cluster.dissolve",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal,
          ManagedInstance::State::OnlineRW}},
        {"Cluster.checkInstanceState",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO}},
        {"Cluster.rescan",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal,
          ManagedInstance::State::OnlineRW}},
        {"ReplicaSet.status",
         {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Any,
          ManagedInstance::State::Any}},
        {"Cluster.forceQuorumUsingPartitionOf",
         {GRInstanceType::GroupReplication | GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::Any,
          ManagedInstance::State::OnlineRW |
              ManagedInstance::State::OnlineRO}}};

/**
 * Validates the session used to manage the group.
 *
 * Checks if the given session is valid for use with AdminAPI.
 *
 * @param group_session A group session to validate.
 *
 * @throws shcore::Exception::runtime_error if session is not valid.
 */
void validate_group_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &group_session) {
  // Validate if the server version is open and supported by the AdminAPI
  validate_session(group_session);

  validate_connection_options(group_session->get_connection_options(),
                              shcore::Exception::runtime_error);

  if (mysqlshdk::gr::is_group_replication_delayed_starting(
          mysqlshdk::mysql::Instance(group_session)))
    throw shcore::Exception::runtime_error(
        "Cannot perform operation while group replication is "
        "starting up");
}

}  // namespace

namespace ManagedInstance {
std::string describe(State state) {
  std::string ret_val;
  switch (state) {
    case OnlineRW:
      ret_val = "Read/Write";
      break;
    case OnlineRO:
      ret_val = "Read Only";
      break;
    case Recovering:
      ret_val = "Recovering";
      break;
    case Unreachable:
      ret_val = "Unreachable";
      break;
    case Offline:
      ret_val = "Offline";
      break;
    case Error:
      ret_val = "Error";
      break;
    case Missing:
      ret_val = "(Missing)";
      break;
    case Any:
      assert(0);  // FIXME(anyone) avoid using enum as a bitmask
      break;
  }
  return ret_val;
}
};  // namespace ManagedInstance

/**
 * Validates the session for AdminAPI operations.
 *
 * Checks if the given session exists and is open and if the session version is
 * valid for use with AdminAPI.
 *
 * @param group_session A session to validate.
 *
 * @throws shcore::Exception::runtime_error if session is not valid.
 */
void validate_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  // A classic session is required to perform any of the AdminAPI operations
  if (!session) {
    throw shcore::Exception::runtime_error(
        "The shell must be connected to a member of the InnoDB cluster being "
        "managed");
  } else if (!session->is_open()) {
    throw shcore::Exception::runtime_error(
        "The session was closed. An open session is required to perform "
        "this operation");
  }

  // Validate if the server version is supported by the AdminAPI
  mysqlshdk::utils::Version server_version = session->get_server_version();

  if (server_version >= k_max_adminapi_server_version ||
      server_version < k_min_adminapi_server_version)
    throw shcore::Exception::runtime_error(
        "Unsupported server version: AdminAPI operations require MySQL server "
        "versions 5.7 or 8.0");
}

Cluster_check_info get_cluster_check_info(
    const std::shared_ptr<mysqlshdk::db::ISession> &group_session) {
  validate_group_session(group_session);

  Cluster_check_info state;
  // TODO(ak) make get_gr_instance_type() check the metadata of the MD server
  // instead
  // Retrieves the instance configuration type from the perspective of the
  // active session
  try {
    state.source_type = get_gr_instance_type(group_session);
  } catch (shcore::Exception &e) {
    if (mysqlshdk::db::is_server_connection_error(e.code())) {
      throw;
    } else {
      log_warning("Error detecting GR instance: %s", e.what());
      state.source_type = GRInstanceType::Unknown;
    }
  }

  // If it is a GR instance, validates the instance state
  if (state.source_type == GRInstanceType::GroupReplication ||
      state.source_type == GRInstanceType::InnoDBCluster) {
    // Retrieves the instance cluster statues from the perspective of the
    // active session (The Metadata Session)
    state = get_replication_group_state(group_session, state.source_type);
  } else {
    state.quorum = ReplicationQuorum::Normal;
    state.source_state = ManagedInstance::Offline;
  }

  return state;
}

void check_preconditions(const std::string &function_name,
                         const Cluster_check_info &state) {
  // Retrieves the availability configuration for the given function
  assert(AdminAPI_function_availability.find(function_name) !=
         AdminAPI_function_availability.end());
  FunctionAvailability availability =
      AdminAPI_function_availability.at(function_name);
  std::string error;

  // Retrieves the instance configuration type from the perspective of the
  // active session
  // Validates availability based on the configuration state
  if (state.source_type & availability.instance_config_state) {
    // If it is a GR instance, validates the instance state
    if (state.source_type == GRInstanceType::GroupReplication ||
        state.source_type == GRInstanceType::InnoDBCluster) {
      // Validates availability based on the instance status
      if (state.source_state & availability.instance_status) {
        // Finally validates availability based on the Cluster quorum
        if ((state.quorum & availability.cluster_status) == 0) {
          switch (state.quorum) {
            case ReplicationQuorum::Quorumless:
              error = "There is no quorum to perform the operation";
              break;
            case ReplicationQuorum::Dead:
              error =
                  "Unable to perform the operation on a dead InnoDB "
                  "cluster";
              break;
            default:
              // no-op
              break;
          }
        }
      } else {
        error = "This function is not available through a session";
        switch (state.source_state) {
          case ManagedInstance::OnlineRO:
            error += " to a read only instance";
            break;
          case ManagedInstance::Offline:
            error += " to an offline instance";
            break;
          case ManagedInstance::Error:
            error += " to an instance in error state";
            break;
          case ManagedInstance::Recovering:
            error += " to a recovering instance";
            break;
          case ManagedInstance::Unreachable:
            error += " to an unreachable instance";
            break;
          default:
            // no-op
            break;
        }
      }
    }
  } else {
    error = "This function is not available through a session";
    switch (state.source_type) {
      case GRInstanceType::Unknown:
        error =
            "Unable to detect target instance state. Please check account "
            "privileges.";
        break;
      case GRInstanceType::Standalone:
        error += " to a standalone instance";
        break;
      case GRInstanceType::StandaloneWithMetadata:
        error +=
            " to a standalone instance (metadata exists, but GR is not active)";
        break;
      case GRInstanceType::GroupReplication:
        error +=
            " to an instance belonging to an unmanaged replication "
            "group";
        break;
      case GRInstanceType::InnoDBCluster:
        error += " to an instance already in an InnoDB cluster";
        break;
      default:
        // no-op
        break;
    }
  }

  if (!error.empty()) throw shcore::Exception::runtime_error(error);
}

Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<mysqlshdk::db::ISession> &group_session) {
  assert(function_name.find('.') != std::string::npos);

  if (!group_session)
    throw shcore::Exception::runtime_error(
        "An open session is required to perform this operation.");

  Cluster_check_info info = get_cluster_check_info(group_session);

  check_preconditions(function_name, info);

  return info;
}

}  // namespace dba
}  // namespace mysqlsh
