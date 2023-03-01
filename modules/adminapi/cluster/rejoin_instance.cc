/*
 * Copyright (c) 2022, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/cluster_topology_executor.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/validations.h"

namespace mysqlsh::dba::cluster {

namespace {
constexpr std::chrono::seconds k_recovery_start_timeout{30};
}

bool Rejoin_instance::check_rejoinable() {
  // Check if the instance is part of the Metadata
  auto status = validate_instance_rejoinable(
      *m_target_instance, m_cluster_impl->get_metadata_storage(),
      m_cluster_impl->get_id(), &m_uuid_mismatch);

  switch (status) {
    case Instance_rejoinability::NOT_MEMBER:
      throw shcore::Exception::runtime_error(
          "The instance '" + m_target_instance->descr() + "' " +
          "does not belong to the cluster: '" + m_cluster_impl->get_name() +
          "'.");

    case Instance_rejoinability::ONLINE: {
      if (m_within_reboot_cluster) {
        current_console()->print_note(m_target_instance->descr() +
                                      " already rejoined the cluster '" +
                                      m_cluster_impl->get_name() + "'.");
      } else {
        current_console()->print_note(
            m_target_instance->descr() +
            " is already an active (ONLINE) member of cluster '" +
            m_cluster_impl->get_name() + "'.");
      }
      return false;
    }

    case Instance_rejoinability::RECOVERING: {
      if (m_within_reboot_cluster) {
        current_console()->print_note(
            m_target_instance->descr() + " already rejoined the cluster '" +
            m_cluster_impl->get_name() + "' and is recovering.");
        return false;
      } else {
        current_console()->print_note(
            m_target_instance->descr() +
            " is already an active (RECOVERING) member of cluster '" +
            m_cluster_impl->get_name() + "'.");
        return false;
      }
    }

    case Instance_rejoinability::REJOINABLE:
      break;
  }

  std::string nice_error =
      "The instance '" + m_target_instance->descr() +
      "' may belong to a different cluster as the one registered "
      "in the Metadata since the value of "
      "'group_replication_group_name' does not match the one "
      "registered in the cluster's Metadata: possible split-brain "
      "scenario. Please remove the instance from the cluster.";

  try {
    if (!validate_cluster_group_name(*m_target_instance,
                                     m_cluster_impl->get_group_name())) {
      throw shcore::Exception::runtime_error(nice_error);
    }
  } catch (const shcore::Error &e) {
    // If the GR plugin is not installed, we can get this error.
    // In that case, we install the GR plugin and retry.
    if (e.code() == ER_UNKNOWN_SYSTEM_VARIABLE) {
      log_info("%s: installing GR plugin (%s)",
               m_target_instance->descr().c_str(), e.format().c_str());
      mysqlshdk::gr::install_group_replication_plugin(*m_target_instance,
                                                      nullptr);

      if (!validate_cluster_group_name(*m_target_instance,
                                       m_cluster_impl->get_group_name())) {
        throw shcore::Exception::runtime_error(nice_error);
      }
    } else {
      throw;
    }
  }

  return true;
}

void Rejoin_instance::resolve_local_address(
    Group_replication_options *gr_options,
    const Group_replication_options &user_gr_options) {
  auto hostname = m_target_instance->get_canonical_hostname();

  // During a reboot from complete outage the command does not change the
  // communication stack unless set via switchCommunicationStack
  auto communication_stack =
      get_communication_stack(*m_cluster_impl->get_cluster_server());

  gr_options->local_address = mysqlsh::dba::resolve_gr_local_address(
      user_gr_options.local_address.value_or("").empty()
          ? gr_options->local_address
          : user_gr_options.local_address,
      communication_stack, hostname, *m_target_instance->get_sysvar_int("port"),
      false, true);

  // Validate that the local_address value we want to use as well as the
  // local address values in use on the cluster are compatible with the
  // version of the instance being added to the cluster.
  cluster_topology_executor_ops::validate_local_address_ip_compatibility(
      m_target_instance, gr_options->local_address.value_or(""),
      gr_options->group_seeds.value_or(""),
      m_cluster_impl->get_lowest_instance_version());
}

void Rejoin_instance::check_and_resolve_instance_configuration() {
  Group_replication_options user_options(m_options.gr_options);

  // Check instance configuration and state, like dba.checkInstance
  // But don't do it if it was already done by the caller
  m_cluster_impl->check_instance_configuration(
      m_target_instance, false, checks::Check_type::REJOIN, true);

  // Resolve the SSL Mode to use to configure the instance.
  if (m_primary_instance) {
    resolve_instance_ssl_mode_option(*m_target_instance, *m_primary_instance,
                                     &m_options.gr_options.ssl_mode);
    log_info("SSL mode used to configure the instance: '%s'",
             to_string(m_options.gr_options.ssl_mode).c_str());
  }

  // Read actual GR configurations to preserve them when rejoining the
  // instance.
  m_options.gr_options.read_option_values(*m_target_instance,
                                          m_is_switching_comm_stack);

  // If this is not seed instance, then we should try to read the
  // failoverConsistency and expelTimeout values from a a cluster member.
  m_cluster_impl->query_group_wide_option_values(
      m_target_instance.get(), &m_options.gr_options.consistency,
      &m_options.gr_options.expel_timeout);

  // Compute group_seeds using local_address from each active cluster member
  {
    auto group_seeds = [](const std::map<std::string, std::string> &seeds,
                          const std::string &skip_uuid = "") {
      std::string ret;
      for (const auto &i : seeds) {
        if (i.first != skip_uuid) {
          if (!ret.empty()) ret.append(",");
          ret.append(i.second);
        }
      }
      return ret;
    };

    m_options.gr_options.group_seeds =
        group_seeds(m_cluster_impl->get_cluster_group_seeds(),
                    m_target_instance->get_uuid());
  }

  // Resolve and validate GR local address.
  // NOTE: Must be done only after getting the report_host used by GR and for
  //       the metadata;
  resolve_local_address(&m_options.gr_options, user_options);
}

bool Rejoin_instance::create_replication_user() {
  // Creates the replication user ONLY if not already given.
  // NOTE: User always created at the seed instance.
  if (!m_options.gr_options.recovery_credentials ||
      m_options.gr_options.recovery_credentials->user.empty()) {
    std::tie(m_options.gr_options.recovery_credentials, m_account_host) =
        m_cluster_impl->create_replication_user(m_target_instance.get());
    return true;
  }
  return false;
}

void Rejoin_instance::do_run() {
  // Validations and variables initialization
  {
    m_primary_instance = m_cluster_impl->get_cluster_server();

    // Validate the GR options.
    // Note: If the user provides no group_seeds value, it is automatically
    // assigned a value with the local_address values of the existing cluster
    // members and those local_address values are already validated on the
    // validate_local_address_ip_compatibility method, so we only need to
    // validate the group_seeds value provided by the user.
    m_options.gr_options.check_option_values(
        m_target_instance->get_version(),
        m_target_instance->get_canonical_port());

    m_options.gr_options.manual_start_on_boot =
        m_cluster_impl->get_manual_start_on_boot_option();

    // Validate the Clone options.
    // TODO(miguel): add support for recoveryMethod
    /*m_options.clone_options.check_option_values(
        m_target_instance->get_version());*/

    // Validate the options used
    cluster_topology_executor_ops::validate_add_rejoin_options(
        m_options.gr_options, get_communication_stack(*m_primary_instance));

    m_is_autorejoining =
        cluster_topology_executor_ops::is_member_auto_rejoining(
            m_target_instance);

    if (!check_rejoinable()) {
      return;
    }

    // Verify whether the instance supports the communication stack in use in
    // the Cluster and set it in the options handler
    m_comm_stack = get_communication_stack(*m_primary_instance);

    cluster_topology_executor_ops::check_comm_stack_support(
        m_target_instance, &m_options.gr_options, m_comm_stack);

    // Check if the Cluster's communication stack has switched
    if (!m_is_switching_comm_stack) {
      std::string persisted_comm_stack =
          m_target_instance
              ->get_sysvar_string("group_replication_communication_stack")
              .value_or(kCommunicationStackXCom);

      if (persisted_comm_stack != m_comm_stack) {
        m_is_switching_comm_stack = true;
      }
    }

    // Check for various instance specific configuration issues
    check_and_resolve_instance_configuration();

    // Check GTID consistency to determine if the instance can safely rejoin
    // the cluster BUG#29953812: ADD_INSTANCE() PICKY ABOUT GTID_EXECUTED,
    // REJOIN_INSTANCE() NOT: DATA NOT COPIED
    m_cluster_impl->validate_rejoin_gtid_consistency(*m_target_instance);

    // Set the transaction size limit, to ensure no older values are used
    m_options.gr_options.transaction_size_limit =
        get_transaction_size_limit(*m_primary_instance);

    // Set the paxos_single_leader option
    // Verify whether the primary supports it, meaning it's in use. Checking if
    // the target supports it is not valid since the target might be 8.0 and
    // the primary 5.7
    if (supports_paxos_single_leader(m_primary_instance->get_version())) {
      m_options.gr_options.paxos_single_leader =
          get_paxos_single_leader_enabled(*m_primary_instance).value_or(false);
    }
  }

  // Execute the rejoin
  auto console = current_console();

  // Set a Config object for the target instance (required to configure GR).
  std::unique_ptr<mysqlshdk::config::Config> cfg = create_server_config(
      m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);

  console->print_info("Rejoining instance '" + m_target_instance->descr() +
                      "' to cluster '" + m_cluster_impl->get_name() + "'...");
  console->print_info();

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  mysqlshdk::gr::install_group_replication_plugin(*m_target_instance, nullptr);

  // Ensure GR is not auto-rejoining, but after the GR plugin is installed
  if (m_is_autorejoining)
    cluster_topology_executor_ops::ensure_not_auto_rejoining(m_target_instance);

  // Validate group_replication_gtid_assignment_block_size. Its value must be
  // the same on the instance as it is on the cluster but can only be checked
  // after the GR plugin is installed. This check is also done on the rejoin
  // operation which covers the rejoin and rebootCluster operations.
  m_cluster_impl->validate_variable_compatibility(
      *m_target_instance, "group_replication_gtid_assignment_block_size");

  // enable skip_slave_start if the cluster belongs to a ClusterSet, and
  // configure the managed replication channel if the cluster is a
  // replica.
  if (!m_ignore_cluster_set && m_cluster_impl->is_cluster_set_member()) {
    m_cluster_impl->configure_cluster_set_member(m_target_instance);
  }

  // TODO(alfredo) - when clone support is added to rejoin, join() can
  // probably be simplified into create repl user + rejoin() + update metadata

  // (Re-)join the instance to the cluster (setting up GR properly).
  // NOTE: the join_cluster() function expects the number of members in
  //       the cluster excluding the joining node, thus cluster_count must
  //       exclude the rejoining node (cluster size - 1) since it already
  //       belongs to the metadata (BUG#30174191).
  std::optional<uint64_t> cluster_count =
      m_cluster_impl->get_metadata_storage()->get_cluster_size(
          m_cluster_impl->get_id()) -
      1;

  // we need a point in time as close as possible, but still earlier than
  // when recovery starts to monitor the recovery phase. The timestamp
  // resolution is timestamp(3) irrespective of platform
  std::string join_begin_time =
      m_target_instance->queryf_one_string(0, "", "SELECT NOW(3)");

  // If using 'MySQL' communication stack, the account must already
  // exist in the rejoining member since authentication is based on MySQL
  // credentials so the account must exist in both ends before GR
  // starts. However, we cannot assure the account credentials haven't changed
  // in the cluster resulting in a mismatch. That'd be the case if
  // cluster.resetRecoveryAccounts() was used while the instance was offline.
  // For that reason, we simply re-create the account to ensure it will rejoin
  // the cluster.
  if (m_comm_stack == kCommunicationStackMySQL) {
    console->print_info("Re-creating recovery account...");
    // Re-create the account in the cluster
    create_replication_user();

    // Re-create the account in the instance but with binlog disabled before
    // starting GR, otherwise we'd create errant transaction.
    m_cluster_impl->create_local_replication_user(m_target_instance,
                                                  m_options.gr_options);

    // Set the allowlist to 'AUTOMATIC' to ensure no older values are used
    m_options.gr_options.ip_allowlist = "AUTOMATIC";

    // Sync again with the primary Cluster to ensure the replication account
    // that was just re-created was already replicated to the Replica Cluster,
    // otherwise, GR won't be able to authenticate using the account and join
    // the instance to the group.
    console->print_info();
    console->print_info(
        "* Waiting for the Cluster to synchronize with the PRIMARY "
        "Cluster...");
    m_cluster_impl->sync_transactions(*m_primary_instance,
                                      k_clusterset_async_channel_name, 0);
  }

  mysqlsh::dba::join_cluster(*m_target_instance, *m_primary_instance,
                             m_options.gr_options, cluster_count, cfg.get());

  if (m_comm_stack == kCommunicationStackMySQL) {
    // We cannot call restore_recovery_account_all_members() without knowing
    // whether recovery has started already, otherwise, the credentials for
    // the replication channel won't match since they were already reset in
    // the Cluster. So we must wait for recovery to start before restoring the
    // recovery accounts.
    //
    // TODO(anyone): whenever clone support is added to rejoinInstance() the
    // call to wait_recovery_start can be removed since wait_recovery() will
    // be used, regardless of the communication stack.
    wait_recovery_start(m_target_instance->get_connection_options(),
                        join_begin_time, k_recovery_start_timeout.count());

    m_cluster_impl->restore_recovery_account_all_members();
  }

  // Update group_seeds in other members
  {
    auto cluster_size =
        m_cluster_impl->get_metadata_storage()->get_cluster_size(
            m_cluster_impl->get_id()) -
        1;  // expects the number of members in the cluster excluding the
            // joining node

    m_cluster_impl->update_group_peers(
        *m_target_instance, m_options.gr_options, cluster_size,
        m_target_instance->get_canonical_address(), true);
  }

  if (m_uuid_mismatch) {
    m_cluster_impl->update_metadata_for_instance(*m_target_instance);
  }

  console->print_info("The instance '" + m_target_instance->descr() +
                      "' was successfully rejoined to the cluster.");
  console->print_info();
}

}  // namespace mysqlsh::dba::cluster
