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

#include "modules/adminapi/common/preconditions.h"
#include <map>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/db/utils_error.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {

namespace {
// Holds a relation of messages for a given metadata state.
using MDS = metadata::State;

std::string lookup_message(const std::string &function_name, MDS state) {
  if (function_name == "*") {
    switch (state) {
      case MDS::MAJOR_HIGHER:
        return "The installed metadata version %s is higher than the supported "
               "by the Shell which is version %s. It is recommended to use a "
               "Shell version that supports this metadata.";
      case MDS::MAJOR_LOWER:
        return "The installed metadata version %s is lower than the supported "
               "by the Shell which is version %s. It is recommended to upgrade "
               "the metadata. See \\? dba.<<<upgradeMetadata>>> for "
               "additional details.";
      case MDS::FAILED_UPGRADE:
        return metadata::kFailedUpgradeError;
      case MDS::UPGRADING:
        return "The metadata is being upgraded. Wait until the upgrade process "
               "completes and then retry the operation.";
      default:
        break;
    }
  } else if (function_name == "Dba.createCluster") {
    switch (state) {
      case MDS::MAJOR_HIGHER:
        return "Operation not allowed. The installed metadata version %s is "
               "higher than the supported by the Shell which is version %s. "
               "Use a Shell version that supports this metadata version "
               "execute this operation.";
      case MDS::MAJOR_LOWER:
        return "Operation not allowed. The installed metadata version %s is "
               "lower than the supported by the Shell which is version %s. "
               "Upgrade the metadata to execute this operation. See \\? "
               "dba.<<<upgradeMetadata>>> for additional details.";
      default:
        break;
    }
  } else if (function_name == "Dba.getCluster") {
    switch (state) {
      case MDS::MAJOR_HIGHER:
        return "The cluster is in READ ONLY mode because the installed "
               "metadata version %s is higher than the supported by the Shell "
               "which is version %s. Use a Shell version that supports this "
               "metadata version to remove this restriction.";
      case MDS::MAJOR_LOWER:
        return "The cluster is in READ ONLY mode because the installed "
               "metadata version %s is lower than the supported by the Shell "
               "which is version %s. Upgrade the metadata to remove this "
               "restriction. See \\? dba.<<<upgradeMetadata>>> for "
               "additional details.";
      default:
        break;
    }
  } else if (function_name == "Cluster") {
    switch (state) {
      case MDS::MAJOR_HIGHER:
        return "Operation not allowed. The cluster is in READ ONLY mode "
               "because the installed metadata version %s is higher than the "
               "supported by the Shell which is version %s. Use a Shell "
               "version that supports this metadata version to remove this "
               "restriction.";
      case MDS::MAJOR_LOWER:
        return "Operation not allowed. The cluster is in READ ONLY mode "
               "because the installed metadata version %s is lower than the "
               "supported by the Shell which is version %s. Upgrade the "
               "metadata to remove this restriction. See \\? "
               "dba.<<<upgradeMetadata>>> for additional details.";
      default:
        break;
    }
  }

  // If the message for the specific function is not found, then it tries
  // getting the message for the class. If the message for the class is not
  // found, then it tries getting the generic message.
  if (function_name != "*") {
    auto tokens = shcore::str_split(function_name, ".");
    if (tokens.size() == 2) {
      return lookup_message(tokens[0], state);
    } else {
      return lookup_message("*", state);
    }
  }
  return "";
}

enum class MDS_actions { NONE, WARN, ERROR };

// The AdminAPI maximum supported MySQL Server version
const mysqlshdk::utils::Version k_max_adminapi_server_version =
    mysqlshdk::utils::Version("8.1");

// The AdminAPI minimum supported MySQL Server version
const mysqlshdk::utils::Version k_min_adminapi_server_version =
    mysqlshdk::utils::Version("5.7");

struct Metadata_validations {
  metadata::States state;
  MDS_actions action;
};

// Note that this structure may be initialized using initializer
// lists, so the order of the fields is very important
struct FunctionAvailability {
  int instance_config_state;
  ReplicationQuorum::State cluster_status;
  int instance_status;
  std::vector<Metadata_validations> metadata_validations;
};

// TODO(alfredo) - this map should be turned into a compile-time lookup
// table
const std::map<std::string, FunctionAvailability>
    AdminAPI_function_availability = {
        // The Dba functions
        {"Dba.createCluster",
         {GRInstanceType::Standalone | GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::GroupReplication,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Dba.getCluster",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::ERROR},
           {metadata::kIncompatibleVersion, MDS_actions::WARN}}}},
        {"Dba.dropMetadataSchema",
         {GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::StandaloneInMetadata |
              GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW,
          {}}},
        {"Dba.rebootClusterFromCompleteOutage",
         {GRInstanceType::StandaloneInMetadata | GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Dba.configureLocalInstance",
         {GRInstanceType::Standalone | GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::StandaloneInMetadata |
              GRInstanceType::InnoDBCluster | GRInstanceType::Unknown,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {}}},
        {"Dba.checkInstanceConfiguration",
         {GRInstanceType::Standalone | GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::Unknown,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {}}},
        {"Dba.configureInstance",
         {GRInstanceType::Standalone | GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::StandaloneInMetadata,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {}}},

        {"Dba.upgradeMetadata",
         {GRInstanceType::InnoDBCluster | GRInstanceType::StandaloneInMetadata |
              GRInstanceType::StandaloneWithMetadata |
              GRInstanceType::GroupReplication,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeInProgress, MDS_actions::ERROR}}}},

        // The Replicaset/Cluster functions
        {"Cluster.addInstance",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.removeInstance",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.rejoinInstance",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.describe",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::ERROR}}}},
        {"Cluster.status",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::ERROR}}}},
        {"Cluster.resetRecoveryAccountsPassword",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kUpgradeStates, MDS_actions::ERROR}}}},
        {"Cluster.options",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::ERROR}}}},
        {"Cluster.dissolve",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.checkInstanceState",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {}}},
        {"Cluster.rescan",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.forceQuorumUsingPartitionOf",
         {GRInstanceType::GroupReplication | GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.switchToSinglePrimaryMode",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::All_online),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.switchToMultiPrimaryMode",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::All_online),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.setPrimaryInstance",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::All_online),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.setOption",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::All_online),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.setInstanceOption",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kLockWriteOperations, MDS_actions::ERROR}}}},
        {"Cluster.listRouters",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {}}},
        {"Cluster.removeRouterMetadata",
         {GRInstanceType::InnoDBCluster,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {}}}};

/**
 * Validates the session used to manage the group.
 *
 * Checks if the given session is valid for use with AdminAPI.
 *
 * @param instance A group session to validate.
 *
 * @throws shcore::Exception::runtime_error if session is not valid.
 */
void validate_group_session(const mysqlsh::dba::Instance &instance) {
  // Validate if the server version is open and supported by the AdminAPI
  validate_session(instance.get_session());

  validate_connection_options(instance.get_connection_options(),
                              shcore::Exception::runtime_error);
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
}  // namespace ManagedInstance

/**
 * Validates the session for AdminAPI operations.
 *
 * Checks if the given session exists and is open and if the session version
 * is valid for use with AdminAPI.
 *
 * @param group_session A session to validate.
 *
 * @throws shcore::Exception::runtime_error if session is not valid.
 */
void validate_session(const std::shared_ptr<mysqlshdk::db::ISession> &session) {
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
        "Unsupported server version: AdminAPI operations require MySQL "
        "server "
        "versions 5.7 or 8.0");
}

Cluster_check_info get_cluster_check_info(
    const std::shared_ptr<Instance> &group_server) {
  validate_group_session(*group_server);

  Cluster_check_info state;
  // Retrieves the instance configuration type from the perspective of the
  // active session
  try {
    state.source_type = get_gr_instance_type(*group_server);
  } catch (const shcore::Exception &e) {
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
    state = get_replication_group_state(*group_server, state.source_type);

    // On IDC we want to also determine whether the quorum is just Normal or
    // if All the instances are ONLINE
    if (state.source_type == GRInstanceType::InnoDBCluster &&
        state.quorum == ReplicationQuorum::States::Normal) {
      try {
        // If the number of members that belong to the same replicaset in the
        // instances table is the same as the number of ONLINE members in
        // replication_group_members then ALL the members in the cluster are
        // ONLINE
        auto result(group_server->query(
            "SELECT (SELECT COUNT(*) "
            "FROM mysql_innodb_cluster_metadata.instances "
            "WHERE replicaset_id = (SELECT replicaset_id "
            "FROM mysql_innodb_cluster_metadata.instances "
            "WHERE mysql_server_uuid=@@server_uuid)) = (SELECT count(*) "
            "FROM performance_schema.replication_group_members "
            "WHERE member_state = 'ONLINE') as all_online"));

        const mysqlshdk::db::IRow *row = result->fetch_one();
        if (row->get_int(0, 0)) {
          state.quorum |= ReplicationQuorum::States::All_online;
        }
      } catch (const mysqlshdk::db::Error &e) {
        log_error(
            "Error while verifying all members in InnoDB Cluster are ONLINE: "
            "%s",
            e.what());
        throw;
      }
    }
  } else {
    state.quorum = ReplicationQuorum::States::Normal;
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
        if (!state.quorum.matches_any(availability.cluster_status)) {
          if (state.quorum.is_set(ReplicationQuorum::States::Normal)) {
            if (availability.cluster_status.is_set(
                    ReplicationQuorum::States::All_online)) {
              error =
                  "This operation requires all the cluster members to be "
                  "ONLINE";
            } else {
              error = "Unable to perform this operation";
            }
          } else if (state.quorum.is_set(
                         ReplicationQuorum::States::Quorumless)) {
            error = "There is no quorum to perform the operation";
          } else if (state.quorum.is_set(ReplicationQuorum::States::Dead)) {
            error =
                "Unable to perform the operation on a dead InnoDB "
                "cluster";
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
            " to a standalone instance (metadata exists, instance does not "
            "belong to that metadata, and GR is not active)";
        break;
      case GRInstanceType::StandaloneInMetadata:
        error +=
            " to a standalone instance (metadata exists, instance belongs to "
            "that metadata, but GR is not active)";
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

/**
 * This function verifies the installed metadata vs the supported metadata.
 * The action to be taken depends on the metadata options configured for each
 * function:
 *
 * If the metadata state is Compatible no action will be taken.
 *
 * If the metadata is not Compatible, but any of the other states the action
 * depends on the configuration for each function as follows:
 *
 * If no specific actions are configured for the function:
 * - Read_compatible will be treated as Incompatible
 * - If metadata is Incompatible an error will be shown indicating whether:
 *   - The metadata Should be updated
 *   - The Shell should be updated
 *
 * If specific actions are configured they will be executed as configured.
 *
 * Every function requiring specific action handling should have it registered
 * on the AdminAPI_function_availability.
 */
void check_metadata_preconditions(
    const std::string &function_name,
    const std::shared_ptr<Instance> &group_server) {
  mysqlshdk::utils::Version installed;

  // It is assumed that if metadata is Compatible, there should be no
  // restrictions on any function
  assert(AdminAPI_function_availability.find(function_name) !=
         AdminAPI_function_availability.end());
  FunctionAvailability availability =
      AdminAPI_function_availability.at(function_name);

  // Metadata validation is done only on the functions that require it
  if (!availability.metadata_validations.empty()) {
    MDS compatibility =
        metadata::version_compatibility(group_server, &installed);

    if (compatibility != MDS::EQUAL) {
      for (const auto &validation : availability.metadata_validations) {
        if (validation.state.is_set(compatibility)) {
          // Gets the right message for the function on this state
          std::string msg = lookup_message(function_name, compatibility);

          // If the message is empty then no action is required, falls back to
          // existing precondition checks.
          if (!msg.empty()) {
            auto pre_formatted = shcore::str_format(
                msg.c_str(), installed.get_base().c_str(),
                mysqlsh::dba::metadata::current_version().get_base().c_str());

            if (validation.action == MDS_actions::WARN) {
              auto console = mysqlsh::current_console();
              console->print_warning(pre_formatted);
            } else if (validation.action == MDS_actions::ERROR) {
              throw std::runtime_error(shcore::str_subvars(
                  pre_formatted,
                  [](const std::string &var) {
                    return shcore::get_member_name(
                        var, shcore::current_naming_style());
                  },
                  "<<<", ">>>"));
            }
          }
        }
      }
    }
  }
}

Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<Instance> &group_server) {
  assert(function_name.find('.') != std::string::npos);

  if (!group_server || !group_server->get_session() ||
      !group_server->get_session()->is_open())
    throw shcore::Exception::runtime_error(
        "An open session is required to perform this operation.");

  // Performs metadata state validations before anything else
  check_metadata_preconditions(function_name, group_server);

  Cluster_check_info info = get_cluster_check_info(group_server);

  check_preconditions(function_name, info);

  return info;
}

}  // namespace dba
}  // namespace mysqlsh
