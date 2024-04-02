/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/remove_replica_instance.h"

#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/dba_errors.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "scripting/types.h"

namespace mysqlsh::dba::cluster {

void Remove_replica_instance::do_run() {
  auto console = mysqlsh::current_console();

  m_target_read_replica_address = m_target_instance->get_canonical_address();

  // Check if the read-replica belongs to this Cluster first
  if (!m_cluster_impl->is_instance_cluster_member(*m_target_instance)) {
    mysqlsh::current_console()->print_error(
        "The instance '" + m_target_read_replica_address +
        "' does not belong to the Cluster '" + m_cluster_impl->get_name() +
        "'.");
    mysqlsh::current_console()->print_info();

    throw shcore::Exception("Instance does not belong to the Cluster",
                            SHERR_DBA_READ_REPLICA_DOES_NOT_BELONG_TO_CLUSTER);
  }

  console->print_info("Removing Read-Replica '" +
                      m_target_read_replica_address + "' from the Cluster '" +
                      m_cluster_impl->get_name() + "'.");
  console->print_info();

  // Check the replication channel status first
  mysqlshdk::mysql::Replication_channel channel;
  bool skip_sync = false;

  if (!mysqlshdk::mysql::get_channel_status(
          *m_target_instance, mysqlsh::dba::k_read_replica_async_channel_name,
          &channel)) {
    if (!m_options.force.value_or(false)) {
      console->print_error(
          "The Read-Replica Replication channel could not be found. Use the "
          "'force' option to ignore this check.");

      throw shcore::Exception("Read-Replica Replication channel does not exist",
                              SHERR_DBA_REPLICATION_OFF);
    } else {
      skip_sync = true;
      console->print_warning(
          "Ignoring non-existing Read-Replica Replication channel because "
          "of 'force' option");
    }
  } else {
    if (channel.status() != mysqlshdk::mysql::Replication_channel::Status::ON) {
      if (!m_options.force.value_or(false)) {
        console->print_error(
            "The Read-Replica Replication channel has an invalid state: '" +
            to_string(channel.status()) +
            "'. Use the 'force' option to ignore this check.");

        throw shcore::Exception(
            "Read-Replica Replication channel not in expected state",
            SHERR_DBA_REPLICATION_INVALID);
      } else {
        skip_sync = true;
        console->print_warning(
            "The Read-Replica Replication channel has an invalid state: '" +
            to_string(channel.status()) +
            "'. Ignoring because of 'force' option");
      }
    }
  }

  // Drop the replication user first since we need to query it from the
  // Metadata
  {
    auto sql_undo = m_repl_account_mng.record_undo([this](auto &mng) {
      mng.drop_read_replica_replication_user(*m_target_instance,
                                             m_options.dry_run);
    });

    m_undo_tracker.add(
        "Restore Read-Replica's replication account", std::move(sql_undo),
        [this]() {
          return m_cluster_impl->get_metadata_storage()->get_md_server();
        });
  }

  // Remove the Read-Replica from the Metadata and wait for the transactions
  // to sync
  log_debug("Removing read-replica '%s' from the Metadata",
            m_target_read_replica_address.c_str());
  if (!m_options.dry_run) {
    auto remove_read_replica_trx_undo = Transaction_undo::create();

    MetadataStorage::Transaction trx(m_cluster_impl->get_metadata_storage());

    m_cluster_impl->get_metadata_storage()->remove_instance(
        m_target_read_replica_address, remove_read_replica_trx_undo.get());

    // Only commit transactions once everything is done
    trx.commit();

    m_undo_tracker.add(
        "Adding back Read-Replica's metadata",
        Sql_undo_list(std::move(remove_read_replica_trx_undo)), [this]() {
          return m_cluster_impl->get_metadata_storage()->get_md_server();
        });
  }

  // Sync transactions after the Metadata updates to ensure the instance has
  // received them all
  if (!skip_sync) {
    try {
      console->print_info(
          "* Waiting for the Read-Replica to synchronize with the Cluster...");
      m_cluster_impl->sync_transactions(
          *m_target_instance, Instance_type::READ_REPLICA, m_options.timeout);
    } catch (const shcore::Exception &e) {
      if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
        console->print_error(
            "The instance '" + m_target_read_replica_address +
            "' failed to synchronize its transaction set with the Cluster. "
            "There might be too many transactions to apply or some replication "
            "error. You may increase the transaction sync timeout with the "
            "option 'timeout' or use the 'force' option to ignore the timeout, "
            "however, it might leave the instance in an inconsistent state and "
            "can lead to errors if you want to reuse it.");

        throw;
      } else if (m_options.force.value_or(false)) {
        console->print_warning(
            "Transaction sync failed but ignored because of 'force' "
            "option, the instance might have been left in an inconsistent "
            "state "
            "that can lead to errors if it is reused: " +
            e.format());
      } else {
        console->print_error(
            "Transaction sync failed. Use the 'force' option to remove "
            "anyway.");
        throw;
      }
    }
  }

  // Remove the managed channel and stop replication
  console->print_info(
      "* Stopping and deleting the Read-Replica managed replication "
      "channel...");

  remove_channel(*m_target_instance, k_read_replica_async_channel_name,
                 m_options.dry_run);

  m_undo_tracker.add_back(
      "Adding back Read-Replica replication channel", [this]() {
        std::string repl_user;
        std::string repl_user_host;

        std::tie(repl_user, repl_user_host) =
            m_cluster_impl->get_metadata_storage()
                ->get_read_replica_repl_account(m_target_instance->get_uuid());

        Async_replication_options ar_options;

        // TODO(RR): Add SSL support
        ar_options.ssl_mode = Cluster_ssl_mode::DISABLED;

        // Set CONNECTION_RETRY_INTERVAL and CONNECTION_RETRY_COUNT
        ar_options.connect_retry = k_read_replica_master_connect_retry;
        ar_options.retry_count = k_read_replica_master_retry_count;

        // Enable SOURCE_CONNECTION_AUTO_FAILOVER
        ar_options.auto_failover = true;

        auto auth_cert_subject =
            m_cluster_impl->query_cluster_instance_auth_cert_subject(
                *m_target_instance);

        auto auth_host = m_repl_account_mng.create_replication_user(
            *m_target_instance, auth_cert_subject,
            Replication_account::Account_type::READ_REPLICA, false, {}, true,
            m_options.dry_run);

        ar_options.repl_credentials = std::move(auth_host.auth);
        repl_user_host = std::move(auth_host.host);

        Replication_sources replication_sources_opts =
            m_cluster_impl->get_read_replica_replication_sources(
                m_target_instance->get_uuid());

        m_cluster_impl->setup_read_replica(m_target_instance.get(), ar_options,
                                           replication_sources_opts, false,
                                           m_options.dry_run);
      });

  try {
    log_info("Disabling automatic failover management at %s",
             m_target_instance->descr().c_str());

    reset_managed_connection_failover(*m_target_instance, m_options.dry_run);
  } catch (...) {
    log_error("Error disabling automatic failover at %s: %s",
              m_target_instance->descr().c_str(),
              format_active_exception().c_str());
    throw;
  }

  console->print_info();
  console->print_info("Read-Replica '" + m_target_instance->descr() +
                      "' successfully removed from the Cluster '" +
                      m_cluster_impl->get_name() + "'.");
  console->print_info();

  if (m_options.dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

void Remove_replica_instance::do_undo() { m_undo_tracker.execute(); }

}  // namespace mysqlsh::dba::cluster
