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

#include "modules/adminapi/cluster/rejoin_instance.h"
#include <stdexcept>
#include "adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/provision.h"
#include "mysqlshdk/libs/mysql/clone.h"

namespace mysqlsh::dba::cluster {

bool Rejoin_instance::check_rejoinable() {
  // Check if the instance is part of the Metadata
  auto status = validate_instance_rejoinable(
      *m_target_instance, m_cluster_impl->get_metadata_storage(),
      m_cluster_impl->get_id(), &m_uuid_mismatch);

  switch (status) {
    case Instance_rejoinability::NOT_MEMBER: {
      throw shcore::Exception::runtime_error(
          "The instance '" + m_target_instance->descr() + "' " +
          "does not belong to the cluster: '" + m_cluster_impl->get_name() +
          "'.");
    }

    case mysqlsh::dba::Instance_rejoinability::READ_REPLICA: {
      throw std::logic_error("Unexpected instance_type: READ_REPLICA");
    }

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

void Rejoin_instance::do_run() {
  // Check if the instance can be rejoined
  if (!check_rejoinable()) {
    return;
  }

  // Validations and variables initialization
  prepare(checks::Check_type::REJOIN, &m_is_switching_comm_stack);

  // Execute the rejoin
  auto console = current_console();

  // Set a Config object for the target instance (required to configure GR).
  std::unique_ptr<mysqlshdk::config::Config> cfg = create_server_config(
      m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);

  console->print_info(shcore::str_format(
      "Rejoining instance '%s' to cluster '%s'...",
      m_target_instance->descr().c_str(), m_cluster_impl->get_name().c_str()));
  console->print_info();

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  if (!m_options.dry_run) {
    mysqlshdk::gr::install_group_replication_plugin(*m_target_instance,
                                                    nullptr);
  }

  // Ensure GR is not auto-rejoining, but after the GR plugin is installed
  if (m_is_autorejoining && !m_options.dry_run) {
    cluster_topology_executor_ops::ensure_not_auto_rejoining(m_target_instance);
  }

  // Validate group_replication_gtid_assignment_block_size. Its value must be
  // the same on the instance as it is on the cluster but can only be checked
  // after the GR plugin is installed. This check is also done on the rejoin
  // operation which covers the rejoin and rebootCluster operations.
  m_cluster_impl->validate_variable_compatibility(
      *m_target_instance, "group_replication_gtid_assignment_block_size");

  // Clone handling
  bool clone_supported =
      mysqlshdk::mysql::is_clone_available(*m_target_instance);
  int64_t restore_clone_threshold = 0;
  bool restore_recovery_accounts = false;
  bool owns_repl_user = false;

  if (clone_supported && !m_options.dry_run) {
    restore_clone_threshold = m_cluster_impl->prepare_clone_recovery(
        *m_target_instance,
        *m_clone_opts.recovery_method == Member_recovery_method::CLONE);
  }

  // enable skip_slave_start if the cluster belongs to a ClusterSet, and
  // configure the managed replication channel if the cluster is a
  // replica.
  if (!m_ignore_cluster_set && m_cluster_impl->is_cluster_set_member() &&
      !m_options.dry_run) {
    m_cluster_impl->configure_cluster_set_member(m_target_instance);
  }

  // (Re-)join the instance to the cluster (setting up GR properly).
  // NOTE: the join_cluster() function expects the number of members in
  //       the cluster excluding the joining node, thus cluster_count must
  //       exclude the rejoining node (cluster size - 1) since it already
  //       belongs to the metadata (BUG#30174191).
  uint64_t cluster_count =
      m_cluster_impl->get_metadata_storage()->get_cluster_size(
          m_cluster_impl->get_id()) -
      1;

  const auto post_failure_actions = [this](bool owns_rpl_user,
                                           int64_t restore_clone_thrsh,
                                           bool restore_rec_accounts) {
    assert(restore_clone_thrsh >= -1);

    bool credentials_exist = m_gr_opts.recovery_credentials &&
                             !m_gr_opts.recovery_credentials->user.empty();

    if (owns_rpl_user && credentials_exist) {
      log_debug("Dropping recovery user '%s'@'%s' at instance '%s'.",
                m_gr_opts.recovery_credentials->user.c_str(),
                m_account_host.c_str(), m_primary_instance->descr().c_str());
      m_primary_instance->drop_user(m_gr_opts.recovery_credentials->user,
                                    m_account_host, true);
    }

    if (restore_rec_accounts) {
      m_cluster_impl->restore_recovery_account_all_members();
    }

    // Check if group_replication_clone_threshold must be restored.
    // This would only be needed if the clone failed and the server didn't
    // restart.
    if (restore_clone_thrsh != 0) {
      try {
        // TODO(miguel): 'start group_replication' returns before reading
        // the threshold value so we can have a race condition. We should
        // wait until the instance is 'RECOVERING'
        log_debug(
            "Restoring value of group_replication_clone_threshold to: %s.",
            std::to_string(restore_clone_thrsh).c_str());

        // If -1 we must restore it's default, otherwise we restore the
        // initial value
        if (restore_clone_thrsh == -1) {
          m_target_instance->set_sysvar_default(
              "group_replication_clone_threshold");
        } else {
          m_target_instance->set_sysvar("group_replication_clone_threshold",
                                        restore_clone_thrsh);
        }
      } catch (const shcore::Error &e) {
        log_info(
            "Could not restore value of group_replication_clone_threshold: "
            "%s. Not a fatal error.",
            e.what());
      }
    }
  };

  try {
    // we need a point in time as close as possible, but still earlier than
    // when recovery starts to monitor the recovery phase. The timestamp
    // resolution is timestamp(3) irrespective of platform
    std::string join_begin_time =
        m_target_instance->queryf_one_string(0, "", "SELECT NOW(3)");

    bool recovery_certificates{false};
    switch (m_cluster_impl->query_cluster_auth_type()) {
      case mysqlsh::dba::Replication_auth_type::CERT_ISSUER:
      case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT:
      case mysqlsh::dba::Replication_auth_type::CERT_ISSUER_PASSWORD:
      case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT_PASSWORD:
        recovery_certificates = true;
        break;
      default:
        break;
    }

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
      owns_repl_user = std::invoke([this](void) {
        // Creates the replication user ONLY if not already given.
        // NOTE: User always created at the seed instance.
        if (m_gr_opts.recovery_credentials &&
            !m_gr_opts.recovery_credentials->user.empty()) {
          return false;
        }

        std::tie(m_gr_opts.recovery_credentials, m_account_host) =
            m_cluster_impl->create_replication_user(
                m_target_instance.get(), m_auth_cert_subject,
                Replication_account_type::GROUP_REPLICATION, false, {}, true,
                m_options.dry_run);

        return true;
      });

      // Re-create the account in the instance but with binlog disabled before
      // starting GR, otherwise we'd create errant transaction.
      m_cluster_impl->create_local_replication_user(
          m_target_instance, m_auth_cert_subject, m_gr_opts,
          !recovery_certificates, m_options.dry_run);

      // Set the allowlist to 'AUTOMATIC' to ensure no older values are used
      m_gr_opts.ip_allowlist = "AUTOMATIC";

      // At this point, all Cluster members got the recovery credentials
      // changed to be able to use the account created for the joining
      // member so in case of failure, we must restore all
      restore_recovery_accounts = true;

      // Sync again with the primary Cluster to ensure the replication account
      // that was just re-created was already replicated to the Replica Cluster,
      // otherwise, GR won't be able to authenticate using the account and join
      // the instance to the group.
      if (!m_ignore_cluster_set && m_cluster_impl->is_cluster_set_member() &&
          !m_options.dry_run) {
        console->print_info();
        console->print_info(
            "* Waiting for the Cluster to synchronize with the PRIMARY "
            "Cluster...");
        m_cluster_impl->sync_transactions(*m_primary_instance,
                                          k_clusterset_async_channel_name, 0);
      }
    }

    if (!m_options.dry_run) {
      mysqlsh::dba::join_cluster(*m_target_instance, *m_primary_instance,
                                 m_gr_opts, recovery_certificates,
                                 cluster_count, cfg.get());

      // Wait until recovery done. Will throw an exception if recovery fails.
      m_cluster_impl->wait_instance_recovery(*m_target_instance,
                                             join_begin_time,
                                             m_options.get_recovery_progress());
    }

    if (m_comm_stack == kCommunicationStackMySQL && !m_options.dry_run) {
      // We cannot call restore_recovery_account_all_members() without knowing
      // whether recovery has started already, otherwise, the credentials for
      // the replication channel won't match since they were already reset in
      // the Cluster. So we must wait for recovery to start before restoring
      // the recovery accounts.
      m_cluster_impl->restore_recovery_account_all_members();
    }

    // Update group_seeds in other members
    if (!m_options.dry_run) {
      auto cluster_size =
          m_cluster_impl->get_metadata_storage()->get_cluster_size(
              m_cluster_impl->get_id()) -
          1;  // expects the number of members in the cluster excluding the
              // joining node

      m_cluster_impl->update_group_peers(
          *m_target_instance, m_gr_opts, cluster_size,
          m_target_instance->get_canonical_address(), true);
    }

    if (m_uuid_mismatch && !m_options.dry_run) {
      m_cluster_impl->update_metadata_for_instance(*m_target_instance);
    }

    // Verification step to ensure the server_id is an attribute on all the
    // instances of the cluster
    if (!m_options.dry_run) {
      m_cluster_impl->ensure_metadata_has_server_id(*m_target_instance);
    }

    console->print_info("The instance '" + m_target_instance->descr() +
                        "' was successfully rejoined to the cluster.");
    console->print_info();

    if (m_options.dry_run) {
      console->print_info("dryRun finished.");
      console->print_info();
    }

  } catch (const std::exception &e) {
    log_info("Failed to rejoin instance: '%s': %s",
             m_target_instance->descr().c_str(), e.what());

    post_failure_actions(owns_repl_user, restore_clone_threshold,
                         restore_recovery_accounts);

    throw;
  }
}

}  // namespace mysqlsh::dba::cluster
