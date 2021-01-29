/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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
#include <mysqld_error.h>
#include <map>

#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/global_topology_check.h"
#include "modules/adminapi/common/instance_pool.h"
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
      case MDS::MINOR_LOWER:
      case MDS::PATCH_LOWER:
        return "The installed metadata version %s is lower than the version "
               "required by Shell which is version %s. It is recommended to "
               "upgrade the metadata. See \\? dba.<<<upgradeMetadata>>> for "
               "additional details.";
      case MDS::FAILED_UPGRADE:
        return metadata::kFailedUpgradeError;
      case MDS::UPGRADING:
        return "The metadata is being upgraded. Wait until the upgrade process "
               "completes and then retry the operation.";
      default:
        break;
    }
  } else if (function_name == "Dba.createCluster" ||
             function_name == "Dba.createReplicaSet") {
    switch (state) {
      case MDS::MAJOR_HIGHER:
        return "Operation not allowed. The installed metadata version %s is "
               "higher than the supported by the Shell which is version %s. "
               "Please use the latest version of the Shell.";
      case MDS::MAJOR_LOWER:
        return "Operation not allowed. The installed metadata version %s is "
               "lower than the version required by Shell which is version %s. "
               "Upgrade the metadata to execute this operation. See \\? "
               "dba.<<<upgradeMetadata>>> for additional details.";
      default:
        break;
    }
  } else if (function_name == "Dba.getCluster" ||
             function_name == "Dba.getReplicaSet") {
    Cluster_type type = function_name == "Dba.getCluster"
                            ? Cluster_type::GROUP_REPLICATION
                            : Cluster_type::ASYNC_REPLICATION;

    switch (state) {
      case MDS::MAJOR_HIGHER:
        return "No " + thing(type) +
               " change operations can be executed because the installed "
               "metadata version %s is higher than the supported by the Shell "
               "which is version %s. Please use the latest version of the "
               "Shell.";
      case MDS::MAJOR_LOWER:
        return "No " + thing(type) +
               " change operations can be executed because the installed "
               "metadata version %s is lower than the version required by "
               "Shell which is version %s. Upgrade the metadata to remove this "
               "restriction. See \\? dba.<<<upgradeMetadata>>> for additional "
               "details.";
      default:
        break;
    }
  } else if (function_name == "Dba.rebootClusterFromCompleteOutage") {
    switch (state) {
      case MDS::MAJOR_HIGHER:
        return "Operation not allowed. No " +
               thing(Cluster_type::GROUP_REPLICATION) +
               " change operations can be executed because the installed "
               "metadata version %s is higher than the supported by the Shell "
               "which is version %s. Please use the latest version of the "
               "Shell.";
      case MDS::MAJOR_LOWER:
        return "The " + thing(Cluster_type::GROUP_REPLICATION) +
               " will be rebooted as configured on the metadata, however, no "
               "change operations can be executed because the installed "
               "metadata version %s is lower than the version required by "
               "Shell which is version %s. Upgrade the metadata to remove this "
               "restriction. See \\? dba.<<<upgradeMetadata>>> for additional "
               "details.";
      default:
        break;
    }
  } else if (function_name == "Cluster" || function_name == "ReplicaSet") {
    Cluster_type type = function_name == "Cluster"
                            ? Cluster_type::GROUP_REPLICATION
                            : Cluster_type::ASYNC_REPLICATION;

    switch (state) {
      case MDS::MAJOR_HIGHER:
        return "Operation not allowed. No " + thing(type) +
               " change operations can be executed because the installed "
               "metadata version %s is higher than the supported by the Shell "
               "which is version %s. Please use the latest version of the "
               "Shell.";
      case MDS::MAJOR_LOWER:
        return "Operation not allowed. No " + thing(type) +
               " change operations can be executed because the installed "
               "metadata version %s is lower than the version required by "
               "Shell "
               "which is version %s. Upgrade the metadata to remove this "
               "restriction. See \\? dba.<<<upgradeMetadata>>> for additional "
               "details.";
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
}  // namespace

// The replicaset functions do not use quorum
ReplicationQuorum::State na_quorum;
const std::map<std::string, Function_availability>
    Precondition_checker::s_preconditions = {
        // The Dba functions
        {"Dba.createCluster",
         {k_min_gr_version,
          TargetType::Standalone | TargetType::StandaloneWithMetadata |
              TargetType::GroupReplication,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR},
           {metadata::kCompatibleLower, MDS_actions::NOTE},
           {metadata::States(metadata::State::FAILED_SETUP),
            MDS_actions::NONE}}}},
        {"Dba.getCluster",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAny}},
        {"Dba.dropMetadataSchema",
         {k_min_adminapi_server_version,
          TargetType::StandaloneWithMetadata |
              TargetType::StandaloneInMetadata | TargetType::InnoDBCluster |
              TargetType::AsyncReplicaSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {}}},
        {"Dba.rebootClusterFromCompleteOutage",
         {k_min_gr_version,
          TargetType::StandaloneInMetadata | TargetType::InnoDBCluster |
              TargetType::InnoDBClusterSet |
              TargetType::InnoDBClusterSetOffline,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO |
              ManagedInstance::State::Offline,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::States(metadata::State::MAJOR_HIGHER),
            MDS_actions::RAISE_ERROR},
           {metadata::States(metadata::State::MAJOR_LOWER), MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAny}},
        {"Dba.configureLocalInstance",
         {k_min_gr_version,
          TargetType::Standalone | TargetType::StandaloneWithMetadata |
              TargetType::StandaloneInMetadata | TargetType::InnoDBCluster |
              TargetType::InnoDBClusterSet |
              TargetType::InnoDBClusterSetOffline | TargetType::Unknown |
              TargetType::GroupReplication,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {},
          kClusterGlobalStateAny}},
        {"Dba.checkInstanceConfiguration",
         {k_min_gr_version,
          TargetType::Standalone | TargetType::StandaloneWithMetadata |
              TargetType::StandaloneInMetadata | TargetType::InnoDBCluster |
              TargetType::InnoDBClusterSet |
              TargetType::InnoDBClusterSetOffline |
              TargetType::GroupReplication | TargetType::Unknown,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {},
          kClusterGlobalStateAny}},
        {"Dba.configureInstance",
         {k_min_gr_version,
          TargetType::Standalone | TargetType::StandaloneWithMetadata |
              TargetType::StandaloneInMetadata | TargetType::GroupReplication |
              TargetType::InnoDBCluster | TargetType::InnoDBClusterSet |
              TargetType::InnoDBClusterSetOffline,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {},
          kClusterGlobalStateAny}},

        {"Dba.upgradeMetadata",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::AsyncReplicaSet |
              TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::All_online),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeInProgress, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAny}},

        {"Dba.configureReplicaSetInstance",
         {k_min_ar_version,
          TargetType::Standalone | TargetType::StandaloneWithMetadata |
              TargetType::StandaloneInMetadata | TargetType::AsyncReplicaSet |
              TargetType::Unknown,
          na_quorum,
          ManagedInstance::State::Any,
          {}}},
        {"Dba.createReplicaSet",
         {k_min_ar_version,
          TargetType::Standalone | TargetType::StandaloneWithMetadata,
          na_quorum,
          ManagedInstance::State::Any,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR},
           {metadata::kCompatibleLower, MDS_actions::NOTE}}}},
        {"Dba.getReplicaSet",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}}}},
        {"Dba.getClusterSet",
         {k_min_cs_version,
          TargetType::InnoDBClusterSet | TargetType::InnoDBClusterSetOffline,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAny}},

        // GR Cluster functions
        {"Cluster.addInstance",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}},
          Cluster_global_status_mask(Cluster_global_status::OK)}},
        {"Cluster.removeInstance",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}},
          Cluster_global_status_mask(Cluster_global_status::OK)}},
        {"Cluster.rejoinInstance",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}},
          Cluster_global_status_mask(Cluster_global_status::OK)}},
        {"Cluster.describe",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAny}},
        {"Cluster.status",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAny}},
        {"Cluster.resetRecoveryAccountsPassword",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAnyOk}},
        {"Cluster.options",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAny}},
        {"Cluster.dissolve",
         {k_min_gr_version,
          TargetType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"Cluster.checkInstanceState",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAny}},
        {"Cluster.rescan",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}},
          Cluster_global_status_mask(Cluster_global_status::OK)}},
        {"Cluster.forceQuorumUsingPartitionOf",
         {k_min_gr_version,
          TargetType::GroupReplication | TargetType::InnoDBCluster |
              TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::States(metadata::State::MAJOR_HIGHER),
            MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAny}},
        {"Cluster.switchToSinglePrimaryMode",
         {k_min_gr_version,
          TargetType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::All_online),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"Cluster.switchToMultiPrimaryMode",
         {k_min_gr_version,
          TargetType::InnoDBCluster,
          ReplicationQuorum::State(ReplicationQuorum::States::All_online),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"Cluster.setPrimaryInstance",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::All_online),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAnyOk}},
        {"Cluster.setOption",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::All_online),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAnyOk}},
        {"Cluster.setInstanceOption",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAnyOk}},
        {"Cluster.listRouters",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}},
          kClusterGlobalStateAny}},
        {"Cluster.removeRouterMetadata",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}},
          Cluster_global_status_mask(Cluster_global_status::OK)}},
        {"Cluster.setupAdminAccount",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}},
          Cluster_global_status_mask(Cluster_global_status::OK)}},
        {"Cluster.setupRouterAccount",
         {k_min_gr_version,
          TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}},
          Cluster_global_status_mask(Cluster_global_status::OK)}},
        {"Cluster.getClusterSet",
         {k_min_cs_version,
          TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAny}},
        {"Cluster.createClusterSet",
         {k_min_cs_version,
          TargetType::InnoDBCluster,
          // TODO(anyone): All of the preconditions quorum related checks are
          // done using ReplicationQuorum::State which should eventually be
          // replaced by Cluster_status, i.e. to satisfy WL12805-FR2.1
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}}}},

        // ClusterSet Functions
        {"ClusterSet.createReplicaCluster",
         {k_min_cs_version,
          TargetType::InnoDBClusterSet,
          ReplicationQuorum::State(ReplicationQuorum::States::Normal),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          Cluster_global_status_mask(Cluster_global_status::OK)}},
        {"ClusterSet.removeCluster",
         {k_min_cs_version,
          TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAnyOk}},
        {"ClusterSet.rejoinCluster",
         {k_min_cs_version,
          TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAnyOk}},
        {"ClusterSet.setPrimaryCluster",
         {k_min_cs_version,
          TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAnyOk}},
        {"ClusterSet.forcePrimaryCluster",
         {k_min_cs_version,
          TargetType::InnoDBClusterSet,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAnyOk}},
        {"ClusterSet.status",
         {k_min_cs_version,
          TargetType::InnoDBClusterSet | TargetType::InnoDBClusterSetOffline,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAny}},
        {"ClusterSet.describe",
         {k_min_cs_version,
          TargetType::InnoDBClusterSet | TargetType::InnoDBClusterSetOffline,
          ReplicationQuorum::State::any(),
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR},
           {metadata::kIncompatible, MDS_actions::WARN},
           {metadata::kCompatibleLower, MDS_actions::NOTE}},
          kClusterGlobalStateAny}},

        // ReplicaSet Functions
        {"ReplicaSet.addInstance",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.rejoinInstance",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.removeInstance",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.describe",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.status",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.dissolve",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::OnlineRW,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.checkInstanceState",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {}}},
        {"ReplicaSet.setPrimaryInstance",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::Any,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.forcePrimaryInstance",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::Any,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.listRouters",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.removeRouterMetadata",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.setupAdminAccount",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.setupRouterAccount",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.setOption",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.setInstanceOption",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
          {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}}},
        {"ReplicaSet.options",
         {k_min_ar_version,
          TargetType::AsyncReplicaSet,
          na_quorum,
          ManagedInstance::State::Any,
          {{metadata::kUpgradeStates, MDS_actions::RAISE_ERROR}}}},
};

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
void Precondition_checker::check_session() const {
  auto session = m_metadata->get_md_server()->get_session();

  // A classic session is required to perform any of the AdminAPI operations
  if (!session) {
    throw shcore::Exception::runtime_error(
        "An open session is required to perform this operation");
  } else if (!session->is_open()) {
    throw shcore::Exception::runtime_error(
        "The session was closed. An open session is required to perform "
        "this operation");
  }

  // Validate if the server version is supported by the AdminAPI
  mysqlshdk::utils::Version server_version = session->get_server_version();

  if (server_version >= k_max_adminapi_server_version ||
      server_version < k_min_adminapi_server_version) {
    throw shcore::Exception::runtime_error(
        "Unsupported server version: AdminAPI operations require MySQL "
        "server versions 5.7 or 8.0");
  }
}

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

Precondition_checker::Precondition_checker(
    const std::shared_ptr<MetadataStorage> &metadata,
    const std::shared_ptr<Instance> &group_server)
    : m_metadata(metadata), m_group_server(group_server) {}

Cluster_check_info Precondition_checker::check_preconditions(
    const std::string &function_name,
    const Function_availability *custom_func_avail) {
  Function_availability availability;
  // If a Function_availability is not provided, look it up based on the
  // function name
  if (!custom_func_avail) {
    // Retrieves the availability configuration for the given function
    assert(s_preconditions.find(function_name) != s_preconditions.end());
    availability = s_preconditions.at(function_name);
  } else {
    // If a function availability is provided use it
    availability = *custom_func_avail;
  }

  check_session();

  MDS mds = check_metadata_state_actions(function_name);
  Cluster_check_info state;

  // bypass the checks if MD setup failed and it's allowed (we're recovering)
  if (mds != MDS::FAILED_SETUP) {
    state = get_cluster_check_info(*m_metadata, m_group_server.get(), false);

    // Check minimum version for the specific function
    if (availability.min_version > state.source_version) {
      throw shcore::Exception::runtime_error(
          "Unsupported server version: This AdminAPI operation requires MySQL "
          "version " +
          availability.min_version.get_full() + " or newer, but target is " +
          state.source_version.get_full());
    }

    // Validates availability based on the configuration state
    check_instance_configuration_preconditions(
        state.source_type, availability.instance_config_state);

    // If it is a GR instance, validates the instance state
    if (state.source_type == TargetType::GroupReplication ||
        state.source_type == TargetType::InnoDBCluster ||
        state.source_type == TargetType::InnoDBClusterSet ||
        state.source_type == TargetType::InnoDBClusterSetOffline ||
        state.source_type == TargetType::AsyncReplicaSet) {
      // Validates availability based on the instance status
      check_managed_instance_status_preconditions(state.source_state,
                                                  availability.instance_status);

      // Finally validates availability based on the Cluster quorum for IDC
      // operations
      if (state.source_type != TargetType::AsyncReplicaSet) {
        check_quorum_state_preconditions(state.quorum,
                                         availability.cluster_status);
      }

      // Validate other ClusterSet preconditions
      if (state.source_type == TargetType::InnoDBClusterSet ||
          state.source_type == TargetType::InnoDBClusterSetOffline) {
        check_cluster_set_preconditions(availability.cluster_set_state);
      }
    }
  } else {
    state = get_cluster_check_info(*m_metadata, m_group_server.get(), true);
  }

  return state;
}

void Precondition_checker::check_instance_configuration_preconditions(
    mysqlsh::dba::TargetType::Type instance_type, int allowed_types) const {
  // If instance type is on the allowed types, we are good to go
  if (instance_type & allowed_types) return;

  std::string error;
  int code = 0;

  error = "This function is not available through a session";
  switch (instance_type) {
    case TargetType::Unknown:
      // The original failure detecting the instance type gets logged.
      error =
          "Unable to detect target instance state. Please see the shell log "
          "for more details.";
      break;
    case TargetType::Standalone:
      error += " to a standalone instance";
      code = SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED;
      break;
    case TargetType::StandaloneWithMetadata:
      // TODO(rennox): This validation was there, but doesn't sound right to be
      // comparing against the allowed types rather than to the actual instance
      // type
      if (allowed_types & TargetType::AsyncReplicaSet) {
        error +=
            " to a standalone instance (metadata exists, instance does not "
            "belong to that metadata)";
      } else {
        error +=
            " to a standalone instance (metadata exists, instance does not "
            "belong to that metadata, and GR is not active)";
      }
      break;
    case TargetType::StandaloneInMetadata:
      // TODO(rennox): This validation was there, but doesn't sound right to be
      // comparing against the allowed types rather than to the actual instance
      // type
      if (allowed_types & TargetType::AsyncReplicaSet) {
        error +=
            " to a standalone instance (metadata exists, instance belongs to "
            "that metadata)";
      } else {
        error +=
            " to a standalone instance (metadata exists, instance belongs to "
            "that metadata, but GR is not active)";
      }
      code = SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE;
      break;
    case TargetType::GroupReplication:
      error += " to an instance belonging to an unmanaged replication group";
      break;
    case TargetType::InnoDBCluster:
      if (((allowed_types | TargetType::InnoDBClusterSet |
            TargetType::InnoDBClusterSetOffline) ==
           (TargetType::InnoDBClusterSet |
            TargetType::InnoDBClusterSetOffline))) {
        error +=
            " to an instance that belongs to an InnoDB Cluster that is not a "
            "member of an InnoDB ClusterSet";
        code = SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTERSET;
      } else {
        error += " to an instance already in an InnoDB Cluster";
        code = SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER;
      }
      break;
    case TargetType::AsyncReplicaSet:
      error += " to an instance that is a member of an InnoDB ReplicaSet";
      code = SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_REPLICASET;
      break;
    case TargetType::InnoDBClusterSet:
      error += " to an InnoDB Cluster that belongs to an InnoDB ClusterSet";
      code = SHERR_DBA_CLUSTER_ALREADY_IN_CLUSTERSET;
      break;
    case TargetType::InnoDBClusterSetOffline:
      error +=
          " to an InnoDB Cluster that belongs to an InnoDB ClusterSet but is "
          "not ONLINE";
      code = SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE;
      break;
  }

  if (code != 0) {
    throw shcore::Exception(error, code);
  } else {
    throw shcore::Exception::runtime_error(error);
  }
}

void Precondition_checker::check_managed_instance_status_preconditions(
    mysqlsh::dba::ManagedInstance::State instance_state,
    int allowed_states) const {
  // If state is allowed, we are good to go
  if (instance_state & allowed_states) return;
  std::string error = "This function is not available through a session";

  switch (instance_state) {
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

  throw shcore::Exception::runtime_error(error);
}

void Precondition_checker::check_quorum_state_preconditions(
    mysqlsh::dba::ReplicationQuorum::State instance_quorum_state,
    mysqlsh::dba::ReplicationQuorum::State allowed_states) const {
  // If state is allowed, we are good to go
  if (instance_quorum_state.matches_any(allowed_states)) return;

  std::string error;
  int code = 0;
  if (instance_quorum_state.is_set(ReplicationQuorum::States::Normal)) {
    if (allowed_states.is_set(ReplicationQuorum::States::All_online)) {
      error =
          "This operation requires all the cluster members to be "
          "ONLINE";
    } else {
      error = "Unable to perform this operation";
    }
  } else if (instance_quorum_state.is_set(
                 ReplicationQuorum::States::Quorumless)) {
    error = "There is no quorum to perform the operation";
    code = SHERR_DBA_GROUP_HAS_NO_QUORUM;
  } else if (instance_quorum_state.is_set(ReplicationQuorum::States::Dead)) {
    error = "Unable to perform the operation on a dead InnoDB cluster";
  }

  if (code != 0) {
    throw shcore::Exception(error, code);
  } else {
    throw shcore::Exception::runtime_error(error);
  }
}

Cluster_global_status Precondition_checker::get_cluster_global_state() {
  Cluster_metadata cluster_md;
  Cluster_set_metadata cset_md;

  if (m_metadata->get_cluster_for_server_uuid(m_group_server->get_uuid(),
                                              &cluster_md) &&
      !cluster_md.cluster_set_id.empty() &&
      m_metadata->get_cluster_set(cluster_md.cluster_set_id, false, &cset_md,
                                  nullptr)) {
    Cluster_impl cluster(cluster_md, m_group_server, m_metadata,
                         Cluster_availability::ONLINE);
    auto finally_primary =
        shcore::on_leave_scope([&cluster]() { cluster.release_primary(); });

    return cluster.get_cluster_set()->get_cluster_global_status(&cluster);
  }

  throw std::logic_error("Cluster metadata not found");
}

/**
 * Verifies if a function is allowed or not based considering:
 * - If the caller instance belongs to a cluster set
 * - The cluster global status
 * - The cluster global status that allow the function to be executed
 * @param allowed_states indicates the cluster global states that would enable
 * the function.
 */
void Precondition_checker::check_cluster_set_preconditions(
    mysqlsh::dba::Cluster_global_status_mask allowed_states) {
  std::string error;

  if (allowed_states != mysqlsh::dba::Cluster_global_status_mask::any()) {
    // Only queries the global state if not ALL the states are allowed
    Cluster_global_status global_state = get_cluster_global_state();

    if (!allowed_states.is_set(global_state)) {
      // ERROR: The function is available on instances in a ClusterSet
      // with specific global state and the instance global state is
      // different
      error =
          "The InnoDB Cluster is part of an InnoDB ClusterSet and has global "
          "state of " +
          to_string(global_state) +
          " within the ClusterSet. Operation is not possible when in that "
          "state ";

      throw shcore::Exception::runtime_error(error);
    }
  }
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
MDS Precondition_checker::check_metadata_state_actions(
    const std::string &function_name) {
  // It is assumed that if metadata is Compatible, there should be no
  // restrictions on any function
  assert(Precondition_checker::s_preconditions.find(function_name) !=
         Precondition_checker::s_preconditions.end());
  Function_availability availability =
      Precondition_checker::s_preconditions.at(function_name);

  // Metadata validation is done only on the functions that require it
  if (!availability.metadata_state_actions.empty()) {
    MDS compatibility = m_metadata->state();

    if (compatibility != MDS::EQUAL) {
      for (const auto &item : availability.metadata_state_actions) {
        if (item.state.is_set(compatibility)) {
          // Gets the right message for the function on this state
          std::string msg = lookup_message(function_name, compatibility);

          // If the message is empty then no action is required, falls back to
          // existing precondition checks.
          if (!msg.empty()) {
            auto pre_formatted = shcore::str_format(
                msg.c_str(), m_metadata->installed_version().get_base().c_str(),
                mysqlsh::dba::metadata::current_version().get_base().c_str());

            auto console = mysqlsh::current_console();
            if (item.action == MDS_actions::WARN) {
              console->print_warning(pre_formatted);
            } else if (item.action == MDS_actions::NOTE) {
              console->print_note(pre_formatted);
            } else if (item.action == MDS_actions::RAISE_ERROR) {
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
    return compatibility;
  }
  return MDS::EQUAL;
}

Cluster_check_info check_function_preconditions(
    const std::string &function_name,
    const std::shared_ptr<MetadataStorage> &metadata,
    const std::shared_ptr<Instance> &group_server,
    const Function_availability *custom_func_avail) {
  // Check if an open session is needed
  if (!metadata && (!group_server || !group_server->get_session() ||
                    !group_server->get_session()->is_open())) {
    throw shcore::Exception::runtime_error(
        "An open session is required to perform this operation.");
  }

  Precondition_checker checker(metadata, group_server);
  return checker.check_preconditions(function_name, custom_func_avail);
}

}  // namespace dba
}  // namespace mysqlsh
