/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/utils/debug.h"

namespace mysqlsh {
namespace dba {

namespace {
void throw_incompatible_md_version(const mysqlshdk::utils::Version &installed,
                                   const mysqlshdk::utils::Version &required) {
  auto msg = shcore::str_format(
      "Incompatible Metadata version. This operation is disallowed because the "
      "installed Metadata version '%s' is lower than the required version, "
      "'%s'. Upgrade the Metadata to remove this restriction. See \\? "
      "dba.<<<upgradeMetadata>>> for additional details.",
      installed.get_base().c_str(), required.get_base().c_str());
  msg = shcore::str_subvars(
      msg,
      [style = shcore::current_naming_style()](std::string_view var) {
        return shcore::get_member_name(var, style);
      },
      "<<<", ">>>");

  mysqlsh::current_console()->print_error(msg);

  throw shcore::Exception("Metadata version is not compatible",
                          SHERR_DBA_METADATA_UNSUPPORTED);
}

void verify_compatibility(metadata::Compatibility compatibility,
                          const MetadataStorage &md) {
  switch (compatibility) {
    case metadata::Compatibility::COMPATIBLE:
      return;

    case metadata::Compatibility::COMPATIBLE_WARN_UPGRADE: {
      auto version_installed = md.installed_version().get_base();
      auto version_current =
          mysqlsh::dba::metadata::current_version().get_base();

      auto msg = shcore::str_format(
          "The installed metadata version '%s' is lower than the version "
          "supported by Shell, version '%s'. It is recommended to upgrade the "
          "Metadata. See \\? dba.<<<upgradeMetadata>>> for additional details.",
          version_installed.c_str(), version_current.c_str());
      msg = shcore::str_subvars(
          msg,
          [style = shcore::current_naming_style()](std::string_view var) {
            return shcore::get_member_name(var, style);
          },
          "<<<", ">>>");

      mysqlsh::current_console()->print_warning(msg);
      return;
    }

    case metadata::Compatibility::INCOMPATIBLE_UPGRADING:
      mysqlsh::current_console()->print_error(
          "The metadata is being upgraded. Wait until the upgrade process "
          "completes and then retry the operation.");

      throw shcore::Exception("Metadata is being upgraded",
                              SHERR_DBA_METADATA_INVALID);

    case metadata::Compatibility::INCOMPATIBLE_FAILED_UPGRADE: {
      auto msg = shcore::str_subvars(
          metadata::kFailedUpgradeError,
          [style = shcore::current_naming_style()](std::string_view var) {
            return shcore::get_member_name(var, style);
          },
          "<<<", ">>>");

      mysqlsh::current_console()->print_error(msg);

      throw shcore::Exception("Metadata upgrade failed",
                              SHERR_DBA_METADATA_INVALID);
    }

    default:
      break;
  }

  throw std::invalid_argument(
      shcore::str_format("Invalid value for MD compatibility: %d",
                         static_cast<int>(compatibility)));
}

void check_clusters_availability(Cluster_availability availability,
                                 std::string &msg_out) {
  switch (availability) {
    case Cluster_availability::ONLINE:
      break;
    case Cluster_availability::ONLINE_NO_PRIMARY:
      msg_out += ": Could not connect to PRIMARY of Primary Cluster";
      break;

    case Cluster_availability::OFFLINE:
      msg_out += ": All members of Primary Cluster are OFFLINE";
      break;

    case Cluster_availability::SOME_UNREACHABLE:
      msg_out +=
          ": All reachable members of Primary Cluster are OFFLINE, but "
          "there are some unreachable members that could be ONLINE";
      break;

    case Cluster_availability::NO_QUORUM:
      msg_out +=
          ": Could not connect to an ONLINE member of Primary Cluster "
          "within quorum";
      break;

    case Cluster_availability::UNREACHABLE:
      msg_out += ": Could not connect to any ONLINE member of Primary Cluster";
      break;
  }
}

}  // namespace

// The AdminAPI maximum supported MySQL Server version
const mysqlshdk::utils::Version
    Precondition_checker::k_max_adminapi_server_version(
        mysqlshdk::utils::k_shell_version.get_major(),
        mysqlshdk::utils::k_shell_version.get_minor(), 9999);

// The AdminAPI minimum supported MySQL Server version
const mysqlshdk::utils::Version
    Precondition_checker::k_min_adminapi_server_version(8, 0, 0);

// The AdminAPI deprecated version (currently disabled)
const mysqlshdk::utils::Version
    Precondition_checker::k_deprecated_adminapi_server_version{};

const mysqlshdk::utils::Version Precondition_checker::k_min_cs_version(8, 0,
                                                                       27);
const mysqlshdk::utils::Version Precondition_checker::k_min_rr_version(8, 0,
                                                                       23);

Command_conditions::Builder Command_conditions::Builder::gen_dba(
    std::string_view name) {
  Builder builder;
  builder.m_conds.name = shcore::str_format(
      "Dba.%.*s", static_cast<int>(name.size()), name.data());

  builder.min_mysql_version(
      Precondition_checker::k_min_adminapi_server_version);

  return builder;
}

Command_conditions::Builder Command_conditions::Builder::gen_cluster(
    std::string_view name) {
  Builder builder;
  builder.m_conds.name = shcore::str_format(
      "Cluster.%.*s", static_cast<int>(name.size()), name.data());

  builder.min_mysql_version(
      Precondition_checker::k_min_adminapi_server_version);

  return builder;
}

Command_conditions::Builder Command_conditions::Builder::gen_replicaset(
    std::string_view name) {
  Builder builder;
  builder.m_conds.name = shcore::str_format(
      "ReplicaSet.%.*s", static_cast<int>(name.size()), name.data());

  builder.min_mysql_version(
      Precondition_checker::k_min_adminapi_server_version);
  builder.target_instance(TargetType::AsyncReplicaSet);

  return builder;
}

Command_conditions::Builder Command_conditions::Builder::gen_clusterset(
    std::string_view name) {
  Builder builder;
  builder.m_conds.name = shcore::str_format(
      "ClusterSet.%.*s", static_cast<int>(name.size()), name.data());

  builder.min_mysql_version(Precondition_checker::k_min_cs_version);
  builder.target_instance(TargetType::InnoDBClusterSet);

  return builder;
}

Command_conditions::Builder &Command_conditions::Builder::min_mysql_version(
    mysqlshdk::utils::Version version) {
  m_conds.min_mysql_version = std::move(version);
  return *this;
}

Command_conditions::Builder &Command_conditions::Builder::quorum_state(
    ReplicationQuorum::States state) {
  m_conds.cluster_status = state;
  return *this;
}

Command_conditions::Builder &Command_conditions::Builder::cluster_global_status(
    Cluster_global_status state) {
  m_conds.cluster_global_state = Cluster_global_status_mask(state);
  return *this;
}

Command_conditions::Builder &
Command_conditions::Builder::cluster_global_status_any_ok() {
  m_conds.cluster_global_state = Cluster_global_status::OK;
  m_conds.cluster_global_state |= Cluster_global_status::OK_NOT_REPLICATING;
  m_conds.cluster_global_state |= Cluster_global_status::OK_NOT_CONSISTENT;
  m_conds.cluster_global_state |= Cluster_global_status::OK_MISCONFIGURED;
  return *this;
}

Command_conditions::Builder &
Command_conditions::Builder::cluster_global_status_add_not_ok() {
  m_conds.cluster_global_state |= Cluster_global_status::NOT_OK;
  return *this;
}

Command_conditions::Builder &
Command_conditions::Builder::cluster_global_status_add_invalidated() {
  m_conds.cluster_global_state |= Cluster_global_status::INVALIDATED;
  return *this;
}

Command_conditions::Builder &Command_conditions::Builder::primary_required() {
  m_conds.primary_required = true;
  m_conds.m_primary_set = true;
  return *this;
}

Command_conditions::Builder &
Command_conditions::Builder::primary_not_required() {
  m_conds.primary_required = false;
  m_conds.m_primary_set = true;
  return *this;
}

Command_conditions::Builder &Command_conditions::Builder::allowed_on_fence() {
  m_conds.allowed_on_fenced = true;
  return *this;
}

Command_conditions Command_conditions::Builder::build() {
  // must specify primary check
  assert(m_conds.m_primary_set);
  if (!m_conds.m_primary_set)
    throw std::logic_error("Primary condition check must be specified");

  return std::exchange(m_conds, {});
}

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
  }
  if (!session->is_open()) {
    throw shcore::Exception::runtime_error(
        "The session was closed. An open session is required to perform "
        "this operation");
  }

  // Validate if the server version is supported by the AdminAPI
  mysqlshdk::utils::Version server_version = session->get_server_version();

  if (server_version < Precondition_checker::k_min_adminapi_server_version) {
    throw shcore::Exception::runtime_error(
        "Unsupported server version: AdminAPI operations in this version of "
        "MySQL Shell are supported on MySQL Server " +
        Precondition_checker::k_min_adminapi_server_version.get_short() +
        " and above");
  }

  if (server_version > Precondition_checker::k_max_adminapi_server_version) {
    throw shcore::Exception::runtime_error(
        "Unsupported server version: AdminAPI operations in this version of "
        "MySQL Shell support MySQL Server up to version " +
        Precondition_checker::k_max_adminapi_server_version.get_short());
  }

  // Print a warning for deprecated version
  if (Precondition_checker::k_deprecated_adminapi_server_version &&
      (server_version <=
       Precondition_checker::k_deprecated_adminapi_server_version)) {
    mysqlsh::current_console()->print_warning(
        "Support for AdminAPI operations in MySQL version " +
        Precondition_checker::k_deprecated_adminapi_server_version.get_short() +
        " is deprecated and will be removed in a future release of MySQL "
        "Shell");
  }
}

namespace ManagedInstance {
std::string describe(State state) {
  switch (state) {
    case Unknown:
      return "Unknown";
    case OnlineRW:
      return "Read/Write";
    case OnlineRO:
      return "Read Only";
    case Recovering:
      return "Recovering";
    case Unreachable:
      return "Unreachable";
    case Offline:
      return "Offline";
    case Error:
      return "Error";
    case Missing:
      return "(Missing)";
    case Any:
      assert(0);  // FIXME(anyone) avoid using enum as a bitmask
      break;
  }

  return {};
}
}  // namespace ManagedInstance

Precondition_checker::Precondition_checker(
    const std::shared_ptr<MetadataStorage> &metadata,
    const std::shared_ptr<Instance> &group_server, bool primary_available)
    : m_metadata(metadata),
      m_group_server(group_server),
      m_primary_available(primary_available) {}

Cluster_check_info Precondition_checker::check_preconditions(
    const Command_conditions &conds) {
  check_session();

  // bypass the checks if MD setup failed and it's allowed (we're recovering)
  if (check_metadata_state_actions(conds) == metadata::State::FAILED_SETUP)
    return get_cluster_check_info(*m_metadata, m_group_server.get(), true);

  Cluster_check_info state =
      get_cluster_check_info(*m_metadata, m_group_server.get(), false);

  // Check minimum version for the specific function
  if (conds.min_mysql_version > state.source_version) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Unsupported server version: This AdminAPI operation requires MySQL "
        "version %s or newer, but target is %s",
        conds.min_mysql_version.get_full().c_str(),
        state.source_version.get_full().c_str()));
  }

  // Validates availability based on the allowed target instance type.
  check_instance_configuration_preconditions(state.source_type,
                                             conds.instance_config_state);

  // If it is a GR instance, validates the instance state
  if (state.source_type == TargetType::GroupReplication ||
      state.source_type == TargetType::InnoDBCluster ||
      state.source_type == TargetType::InnoDBClusterSet ||
      state.source_type == TargetType::InnoDBClusterSetOffline ||
      state.source_type == TargetType::AsyncReplicaSet) {
    // Validates availability based on the the primary availability
    check_managed_instance_status_preconditions(state.source_type, state.quorum,
                                                conds.primary_required);

    // Validates availability based on the Cluster quorum status (InnoDB
    // Cluster/ClusterSet)
    if (state.source_type != TargetType::AsyncReplicaSet) {
      check_quorum_state_preconditions(state.quorum, conds.cluster_status);
    }

    // Validate ClusterSet preconditions
    if (state.source_type == TargetType::InnoDBClusterSet ||
        state.source_type == TargetType::InnoDBClusterSetOffline) {
      check_cluster_set_preconditions(conds.cluster_global_state);
    }

    // Validates command availability based on whether it is allowed or not
    // while fenced.
    if (state.source_type == TargetType::InnoDBClusterSet) {
      check_cluster_fenced_preconditions(conds.allowed_on_fenced);
    }
  }

  return state;
}

void Precondition_checker::check_instance_configuration_preconditions(
    mysqlsh::dba::TargetType::Type instance_type, int allowed_types) const {
  // If instance type is on the allowed types, we are good to go
  if (instance_type & allowed_types) return;

  // The original failure detecting the instance type gets logged.
  if (instance_type == TargetType::Unknown) {
    auto instance =
        m_group_server ? m_group_server : m_metadata->get_md_server();
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Unable to detect state for instance '%s'. Please see the shell log "
        "for more details.",
        instance->get_canonical_address().c_str()));
  }

  int code = 0;
  std::string error("This function is not available through a session");

  switch (instance_type) {
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
    case TargetType::AsyncReplication:
      error +=
          " to an instance belonging to an unmanaged asynchronous replication "
          "topology";
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
    default:
      assert(!"Invalid target instance type");
      break;
  }

  if (code != 0) throw shcore::Exception(error, code);
  throw shcore::Exception::runtime_error(error);
}

void Precondition_checker::check_managed_instance_status_preconditions(
    mysqlsh::dba::TargetType::Type instance_type,
    mysqlsh::dba::ReplicationQuorum::State instance_quorum_state,
    bool primary_req) const {
  if (!primary_req || m_primary_available) return;

  // If there's no primary, check also the quorum state to determine if this
  // is a quorum loss scenario
  if (instance_quorum_state.is_set(ReplicationQuorum::States::Quorumless)) {
    throw shcore::Exception("There is no quorum to perform the operation",
                            SHERR_DBA_GROUP_HAS_NO_QUORUM);
  }

  std::string error;
  switch (instance_type) {
    case TargetType::InnoDBClusterSet:
    case TargetType::InnoDBClusterSetOffline:
      error =
          "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY "
          "instance isn't available. Operation is not possible when in that "
          "state";
      break;
    default:
      error = "This operation requires a PRIMARY instance available";
      break;
  }

  auto global_state = get_cluster_global_state();
  check_clusters_availability(global_state.second, error);
  error.append(".");

  throw shcore::Exception::runtime_error(error);
}

void Precondition_checker::check_quorum_state_preconditions(
    mysqlsh::dba::ReplicationQuorum::State instance_quorum_state,
    mysqlsh::dba::ReplicationQuorum::State allowed_states) const {
  // ignore filter
  if (allowed_states.empty()) return;

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

  if (code != 0) throw shcore::Exception(error, code);
  throw shcore::Exception::runtime_error(error);
}

std::pair<Cluster_global_status, Cluster_availability>
Precondition_checker::get_cluster_global_state() const {
  Cluster_metadata cluster_md;
  Cluster_set_metadata cset_md;

  if (m_metadata->get_cluster_for_server_uuid(m_group_server->get_uuid(),
                                              &cluster_md) &&
      !cluster_md.cluster_set_id.empty() &&
      m_metadata->get_cluster_set(cluster_md.cluster_set_id, false, &cset_md,
                                  nullptr)) {
    Cluster_impl cluster(cluster_md, m_group_server, m_metadata,
                         Cluster_availability::ONLINE);

    shcore::on_leave_scope finally_primary(
        [&cluster]() { cluster.release_primary(); });

    auto clusterset = cluster.get_cluster_set_object();

    auto global_status = clusterset->get_cluster_global_status(&cluster);

    // Check Primary Cluster status
    clusterset->connect_primary();
    auto primary_cluster = clusterset->get_primary_cluster();

    auto primary_cluster_availability = primary_cluster->cluster_availability();

    return {global_status, primary_cluster_availability};
  }

  throw std::logic_error("Cluster metadata not found");
}

Cluster_status Precondition_checker::get_cluster_status() {
  Cluster_metadata cluster_md;
  if (m_metadata->get_cluster_for_server_uuid(m_group_server->get_uuid(),
                                              &cluster_md)) {
    Cluster_impl cluster(cluster_md, m_group_server, m_metadata,
                         Cluster_availability::ONLINE);

    shcore::on_leave_scope finally_primary(
        [&cluster]() { cluster.release_primary(); });

    return cluster.cluster_status();
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
  if (allowed_states == mysqlsh::dba::Cluster_global_status_mask::any()) return;

  // Only queries the global state if not ALL the states are allowed
  auto global_state = get_cluster_global_state();

  if (allowed_states.is_set(global_state.first)) return;

  // ERROR: The function is available on instances in a ClusterSet
  // with specific global state and the instance global state is
  // different
  auto error = shcore::str_format(
      "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state "
      "of %s within the ClusterSet. Operation is not possible when in that "
      "state",
      to_string(global_state.first).c_str());

  // If the global_state is NOT_OK, check the primary cluster availability
  // to provide a finer grain of detail in the error message
  if (global_state.first == Cluster_global_status::NOT_OK)
    check_clusters_availability(global_state.second, error);

  throw shcore::Exception::runtime_error(error);
}

/**
 * Verifies if a function is allowed or not based on:
 *   - If the caller instance belongs to a Cluster or ClusterSet
 *   - The cluster is Fenced to Writes
 *
 * @param allowed_on_fenced indicates if the operation is allowed on a fenced
 * cluster
 */
void Precondition_checker::check_cluster_fenced_preconditions(
    bool allowed_on_fenced) {
  if (allowed_on_fenced) return;

  // Only queries status if not allowed on fenced
  auto cluster_status = get_cluster_status();
  if (cluster_status != Cluster_status::FENCED_WRITES) return;

  // ERROR: The function is available on instances in a Cluster
  // with status FENCED_WRITES
  throw shcore::Exception::runtime_error(shcore::str_format(
      "Unable to perform the operation on an InnoDB Cluster with status %s",
      to_string(cluster_status).c_str()));
}

/**
 * This function verifies the installed metadata vs the supported metadata.
 *
 * If, for the specified conditions, the current MD state is incompatible, this
 * method throws an exception. Otherwise, it returns the MD state.
 *
 * The verification is done following these ordered steps:
 *  - if MD is in FAILED_SETUP, the method doesn't throw and returns (state is
 * dealt with later)
 *  - if MD is in UPGRADING, throws
 *  - if MD is in FAILED_UPGRADE, throws unless the conditions have
 * IGNORE_FAILED_UPGRADE
 *  - if MD is in NONEXISTING, throws unless the conditions have
 * IGNORE_NON_EXISTING
 *  - if MD is in (MAJOR_HIGHER, MINOR_HIGHER or PATCH_HIGHER), throws (shell
 * needs to be upgraded)
 *  - otherwise, the state is converted to metadata::Compatibility and checked
 * against the specified conditions and throws if condition is INCOMPATIBLE_*
 *
 */
metadata::State Precondition_checker::check_metadata_state_actions(
    const Command_conditions &conds) {
  auto state = m_metadata->state();
  DBUG_EXECUTE_IF("md_assume_version_lower",
                  { state = metadata::State::MAJOR_LOWER; });

  log_info("Metadata state: %d", static_cast<int>(state));

  // these states don't need to check against the command's supported state
  // actions
  switch (state) {
    case metadata::State::EQUAL:
    case metadata::State::FAILED_SETUP:
      return state;

    case metadata::State::UPGRADING: {
      verify_compatibility(metadata::Compatibility::INCOMPATIBLE_UPGRADING,
                           *m_metadata);

      // std::unreachable();
      assert(!"std::unreachable");
      break;
    }

    case metadata::State::FAILED_UPGRADE: {
      auto ignore_failed = std::any_of(
          conds.metadata_states.begin(), conds.metadata_states.end(),
          [](const auto &compatibility) {
            return (compatibility ==
                    metadata::Compatibility::IGNORE_FAILED_UPGRADE);
          });
      if (ignore_failed) return state;

      verify_compatibility(metadata::Compatibility::INCOMPATIBLE_FAILED_UPGRADE,
                           *m_metadata);

      // std::unreachable();
      assert(!"std::unreachable");
      break;
    }
    default:
      break;
  }

  // not having a MD available gets special treatment
  if (state == metadata::State::NONEXISTING) {
    // command must support a Compatibility::IGNORE_NON_EXISTING state action
    auto not_required =
        std::any_of(conds.metadata_states.begin(), conds.metadata_states.end(),
                    [](const auto &compatibility) {
                      return (compatibility ==
                              metadata::Compatibility::IGNORE_NON_EXISTING);
                    });
    if (not_required) return state;

    mysqlsh::current_console()->print_error(
        "Command not available on an unmanaged standalone instance.");
    throw shcore::Exception("Metadata Schema not found.",
                            SHERR_DBA_METADATA_NOT_FOUND);
  }

  // check for shell needs to be upgraded
  switch (state) {
    case metadata::State::MAJOR_HIGHER:
    case metadata::State::MINOR_HIGHER:
    case metadata::State::PATCH_HIGHER: {
      auto version_installed = m_metadata->installed_version().get_base();
      auto version_current =
          mysqlsh::dba::metadata::current_version().get_base();

      mysqlsh::current_console()->print_error(shcore::str_format(
          "Incompatible Metadata version. No operations are allowed on a "
          "Metadata version higher than Shell supports ('%s'), the installed "
          "Metadata version '%s' is not supported. Please upgrade MySQL Shell.",
          version_current.c_str(), version_installed.c_str()));

      throw shcore::Exception("Metadata version is not compatible",
                              SHERR_DBA_METADATA_UNSUPPORTED);
    }
    default:
      break;
  }

  // if the command doesn't require any more checks
  if (conds.metadata_states.empty()) return state;

  // convert to compatibility
  metadata::Compatibility compatibility;
  switch (state) {
    case metadata::State::MAJOR_LOWER:
    case metadata::State::MINOR_LOWER:
    case metadata::State::PATCH_LOWER:
      compatibility = metadata::Compatibility::COMPATIBLE_WARN_UPGRADE;
      break;
    case metadata::State::UPGRADING:
      compatibility = metadata::Compatibility::INCOMPATIBLE_UPGRADING;
      break;
    case metadata::State::FAILED_UPGRADE:
      compatibility = metadata::Compatibility::INCOMPATIBLE_FAILED_UPGRADE;
      break;
    default:
      assert(!"Invalid MD state");
      return state;
  }

  // check if incompatibility must trigger an error
  for (const auto &cur_compatibility : conds.metadata_states) {
    // min version has to be checked manually
    switch (cur_compatibility) {
      case metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0:
      case metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_1_0:
      case metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_2_0: {
        auto version_required = mysqlshdk::utils::Version(2, 0, 0);
        if (cur_compatibility ==
            metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_1_0)
          version_required = mysqlshdk::utils::Version(2, 1, 0);
        else if (cur_compatibility ==
                 metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_2_0)
          version_required = mysqlshdk::utils::Version(2, 2, 0);

        auto version_installed = m_metadata->installed_version();
        DBUG_EXECUTE_IF("md_assume_version_1_0_1", {
          version_installed = mysqlshdk::utils::Version(1, 0, 1);
        });
        DBUG_EXECUTE_IF("md_assume_version_2_0_0", {
          version_installed = mysqlshdk::utils::Version(2, 0, 0);
        });
        DBUG_EXECUTE_IF("md_assume_version_2_1_0", {
          version_installed = mysqlshdk::utils::Version(2, 1, 0);
        });

        if (version_installed >= version_required) continue;

        throw_incompatible_md_version(version_installed, version_required);
      }
      default:
        break;
    }

    // reaching this point, we only need to check if the command compatibility
    // matches with the actual one

    if (cur_compatibility != compatibility) continue;

    verify_compatibility(compatibility, *m_metadata);
    return state;
  }

  // reaching this point, there's an incompatibility (either
  // metadata::Compatibility::COMPATIBLE_WARN_UPGRADE,
  // metadata::Compatibility::INCOMPATIBLE_UPGRADING or
  // metadata::Compatibility::INCOMPATIBLE_FAILED_UPGRADE) and the command
  // didn't specify any state that would trigger it. There is, however, a final
  // use case related to Compatibility::COMPATIBLE_WARN_UPGRADE, which is when
  // the command doesn't care about any metadata, but at the same time none can
  // be present.
  //
  // We can detect this if the only state action is
  // Compatibility::IGNORE_NON_EXISTING, which means that we can then throw a
  // similar error as Compatibility::INCOMPATIBLE_MIN_VERSION_* (but with the
  // current version).

  if ((compatibility == metadata::Compatibility::COMPATIBLE_WARN_UPGRADE) &&
      (conds.metadata_states.size() == 1) &&
      (conds.metadata_states.front() ==
       metadata::Compatibility::IGNORE_NON_EXISTING)) {
    throw_incompatible_md_version(m_metadata->installed_version(),
                                  mysqlsh::dba::metadata::current_version());
  }

  return state;
}

Cluster_check_info check_function_preconditions(
    const Command_conditions &conds,
    const std::shared_ptr<MetadataStorage> &metadata,
    const std::shared_ptr<Instance> &group_server, bool primary_available) {
  // Check if an open session is needed
  if (!metadata && (!group_server || !group_server->get_session() ||
                    !group_server->get_session()->is_open())) {
    throw shcore::Exception::runtime_error(
        "An open session is required to perform this operation.");
  }

  Precondition_checker checker(metadata, group_server, primary_available);
  return checker.check_preconditions(conds);
}

}  // namespace dba
}  // namespace mysqlsh
