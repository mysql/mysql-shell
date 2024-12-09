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

#include "modules/adminapi/cluster/rejoin_replica_instance.h"

#include <cassert>
#include <string>

#include "modules/adminapi/cluster/api_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh::dba::cluster {

bool Rejoin_replica_instance::check_rejoinable() {
  // Check if the instance belongs to the Metadata and is a Read-Replica (by
  // uuid)
  if (!m_cluster_impl->is_instance_cluster_member(*m_target_instance)) {
    mysqlsh::current_console()->print_error(
        "The instance '" + m_target_read_replica_address +
        "' does not belong to the Cluster '" + m_cluster_impl->get_name() +
        "'.");
    mysqlsh::current_console()->print_info();

    throw shcore::Exception("Instance does not belong to the Cluster",
                            SHERR_DBA_READ_REPLICA_DOES_NOT_BELONG_TO_CLUSTER);
  }

  // Check if it is a Read-Replica
  if (!m_cluster_impl->is_read_replica(*m_target_instance)) {
    throw shcore::Exception("Instance '" + m_target_read_replica_address +
                                " is not a Read-Replica of the Cluster '" +
                                m_cluster_impl->get_name() + "'.",
                            SHERR_DBA_READ_REPLICA_NOT_READ_REPLICA);
  }

  // Check if the replicationSources are not empty
  m_replication_sources = m_cluster_impl->get_read_replica_replication_sources(
      m_target_instance->get_uuid());

  if (!m_replication_sources.source_type.has_value() ||
      (*m_replication_sources.source_type == Source_type::CUSTOM &&
       m_replication_sources.replication_sources.empty())) {
    mysqlsh::current_console()->print_error(
        "Unable to rejoin Read-Replica '" + m_target_read_replica_address +
        "' to the Cluster: There are no available replication sources. Use "
        "Cluster.setInstanceOption() with the option 'replicationSources' to "
        "update the instance's sources.");

    throw shcore::Exception("No replication sources configured",
                            SHERR_DBA_READ_REPLICA_EMPTY_SOURCE_LIST);
  }

  if (*m_replication_sources.source_type == Source_type::CUSTOM) {
    assert(!m_replication_sources.replication_sources.empty());
    // Check if the replicationSources are reachable and ONLINE cluster members
    m_cluster_impl->validate_replication_sources(
        m_replication_sources.replication_sources,
        m_target_read_replica_address, m_target_instance->get_uuid(), true);
  }

  auto status = mysqlshdk::mysql::get_read_replica_status(*m_target_instance);

  bool read_replica_misconfigured = false;
  bool update_replication_sources = false;

  if (!m_target_instance->is_read_only(true)) {
    read_replica_misconfigured = true;
  }

  Managed_async_channel channel_config;

  // Get the channel configuration
  if (get_managed_connection_failover_configuration(*m_target_instance,
                                                    &channel_config)) {
    if (!channel_config.automatic_failover) {
      read_replica_misconfigured = true;
    } else if (m_replication_sources.source_type == Source_type::CUSTOM) {
      // Check if the configured replication sources are different than the
      // ones in the metadata
      update_replication_sources =
          channel_config.sources != m_replication_sources.replication_sources;
    } else {
      update_replication_sources =
          (*m_replication_sources.source_type == Source_type::PRIMARY &&
           channel_config.primary_weight != k_read_replica_higher_weight) ||
          (*m_replication_sources.source_type == Source_type::SECONDARY &&
           channel_config.secondary_weight != k_read_replica_higher_weight);
    }
  }

  if (status == mysqlshdk::mysql::Read_replica_status::ONLINE &&
      !update_replication_sources && !read_replica_misconfigured) {
    mysqlsh::current_console()->print_note(
        m_target_instance->descr() +
        " is already an active (ONLINE) Read-Replica of the Cluster '" +
        m_cluster_impl->get_name() + "'.");
    return false;
  }

  return true;
}

Member_recovery_method Rejoin_replica_instance::validate_instance_recovery() {
  auto check_recoverable =
      [this](const mysqlshdk::mysql::IInstance &tgt_instance) {
        // Get the gtid state in regards to the donor
        mysqlshdk::mysql::Replica_gtid_state state =
            check_replica_group_gtid_state(*m_donor_instance, tgt_instance,
                                           nullptr, nullptr);

        return (state != mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE);
      };

  const auto gtid_set_is_complete = m_cluster_impl->get_gtid_set_is_complete();

  return mysqlsh::dba::validate_instance_recovery(
      Cluster_type::GROUP_REPLICATION, Member_op_action::ADD_INSTANCE,
      *m_donor_instance, *m_target_instance, check_recoverable,
      m_options.clone_options.recovery_method.value_or(
          Member_recovery_method::AUTO),
      gtid_set_is_complete, current_shell_options()->get().wizards,
      m_cluster_impl->check_clone_availablity(*m_donor_instance,
                                              *m_target_instance));
}

void Rejoin_replica_instance::validate_replication_channels() {
  // Check if there are unknown replication channels running at the instance
  log_debug(
      "Checking if instance '%s' has asynchronous (source-replica) "
      "replication configured.",
      m_target_instance->get_canonical_address().c_str());

  if (mysqlsh::dba::checks::check_illegal_async_channels(
          *m_target_instance, {k_read_replica_async_channel_name})) {
    mysqlsh::current_console()->print_error(
        "Unable to rejoin '" + m_target_instance->descr() +
        "' to the Cluster: unexpected asynchronous (source-replica) "
        "replication channel(s) found.");
    throw shcore::Exception("Unsupported active replication channel.",
                            SHERR_DBA_CLUSTER_UNSUPPORTED_REPLICATION_CHANNEL);
  }
}

std::shared_ptr<Instance>
Rejoin_replica_instance::get_default_donor_instance() {
  // When replicationSources is set to 'primary' or 'secondary' the main source
  // is the primary member of the cluster
  if (m_replication_sources.source_type == Source_type::PRIMARY ||
      m_replication_sources.source_type == Source_type::SECONDARY) {
    return m_cluster_impl->get_cluster_server();
  }

  // When replicationSources is set to a list of instances the main source
  // must be the first member of the list (the one with highest weight)
  assert(!m_replication_sources.replication_sources.empty());
  auto source_str =
      m_replication_sources.replication_sources.begin()->to_string();

  try {
    return m_cluster_impl->get_session_to_cluster_instance(source_str);
  } catch (const shcore::Exception &) {
    throw shcore::Exception(
        "Source is not reachable",
        SHERR_DBA_READ_REPLICA_INVALID_SOURCE_LIST_UNREACHABLE);
  }
}

void Rejoin_replica_instance::prepare() {
  // Validate the Clone options.
  m_options.clone_options.check_option_values(m_target_instance->get_version());

  // Get the main donor and source instance
  // By default, the instance is:
  //
  //   - The primary member when 'replicationSources' is set to "primary" or
  //   "secondary"
  //   - The first member of the 'replicationSources' list
  m_donor_instance = get_default_donor_instance();

  // If the cloneDonor option us used set it as the donor instance
  if (*m_options.clone_options.recovery_method ==
      Member_recovery_method::CLONE) {
    if (m_options.clone_options.clone_donor.has_value()) {
      const std::string &donor = *m_options.clone_options.clone_donor;

      m_donor_instance =
          Scoped_instance(m_cluster_impl->connect_target_instance(donor));
    }
  }

  // get the cert subject to use (we're in a rejoin, so it should be there, but
  // check it anyway)
  m_auth_cert_subject =
      m_cluster_impl->query_cluster_instance_auth_cert_subject(
          *m_target_instance);

  switch (auto auth_type = m_cluster_impl->query_cluster_auth_type();
          auth_type) {
    case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT:
    case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT_PASSWORD:
      if (m_auth_cert_subject.empty()) {
        current_console()->print_error(shcore::str_format(
            "The Cluster's SSL mode is set to '%s' but the instance being "
            "rejoined doesn't have the 'certSubject' option set. Please remove "
            "the instance with Cluster.<<<removeInstance>>>() and then add it "
            "back using Cluster.<<<addReplicaInstance>>>() with the "
            "appropriate authentication options.",
            to_string(auth_type).c_str()));

        throw shcore::Exception(
            shcore::str_format(
                "The Cluster's SSL mode is set to '%s' but the 'certSubject' "
                "option for the instance isn't valid.",
                to_string(auth_type).c_str()),
            SHERR_DBA_MISSING_CERT_OPTION);
      }
      break;
    default:
      break;
  }

  // Validate instance configuration and state
  // Check if the instance is running MySQL >= 8.0.23
  cluster_topology_executor_ops::validate_read_replica_version(
      m_target_instance->get_version(),
      m_cluster_impl->get_lowest_instance_version());

  // Ensure no pre-existing replication channels are configured
  validate_replication_channels();

  current_console()->print_info(
      "* Checking transaction state of the instance...");
  m_options.clone_options.recovery_method = validate_instance_recovery();

  // Check if the donor is valid
  {
    m_cluster_impl->ensure_donor_is_valid(*m_donor_instance);

    if (*m_options.clone_options.recovery_method ==
        Member_recovery_method::CLONE) {
      m_cluster_impl->check_compatible_clone_donor(*m_donor_instance,
                                                   *m_target_instance);
    }
  }
}

void Rejoin_replica_instance::do_run() {
  m_target_read_replica_address = m_target_instance->get_canonical_address();

  // Check if the instance can be rejoined
  if (!check_rejoinable()) return;

  auto console = mysqlsh::current_console();
  console->print_info("Rejoining Read-Replica '" + m_target_instance->descr() +
                      "' to Cluster '" + m_cluster_impl->get_name() + "'...");
  console->print_info();

  // Validations and variables initialization
  prepare();

  // Create the replication user
  Async_replication_options ar_options;
  std::string repl_account_host;

  std::tie(ar_options, repl_account_host) =
      m_cluster_impl->create_read_replica_replication_user(
          m_target_instance.get(), m_auth_cert_subject, m_options.timeout,
          m_options.dry_run);

  m_undo_tracker.add("Dropping replication account", [=, this]() {
    log_info("Dropping replication account '%s'",
             ar_options.repl_credentials->user.c_str());
    m_cluster_impl->drop_read_replica_replication_user(m_target_instance.get(),
                                                       m_options.dry_run);
  });

  if (*m_options.clone_options.recovery_method ==
      Member_recovery_method::CLONE) {
    m_cluster_impl->handle_clone_provisioning(
        m_target_instance, m_donor_instance, ar_options, repl_account_host,
        m_cluster_impl->query_cluster_auth_cert_issuer(), m_auth_cert_subject,
        m_options.get_recovery_progress(), m_options.timeout,
        m_options.dry_run);

    // Clone will copy all tables, including the replication settings stored
    // in mysql.slave_master_info. MySQL will start replication by default
    // if the replication setting are not empty, so in a fast system or if
    // --skip-slave-start is not enabled replication will start and the
    // slave threads will be up-and-running before we issue the new CHANGE
    // MASTER. This will result in an error: MySQL Error 3081 (HY000): (...)
    // This operation cannot be performed with running replication threads;
    // run STOP SLAVE FOR CHANNEL '' first
    //
    // To avoid this situation, we must stop the slave and reset the
    // replication channels being the only possibility the ClusterSet
    // channel if the donor belongs to a Replica Cluster
    remove_channel(*m_target_instance, k_clusterset_async_channel_name,
                   m_options.dry_run);
  }

  // Rejoin the read-replica
  Replication_sources replication_sources_opts =
      m_cluster_impl->get_read_replica_replication_sources(
          m_target_instance->get_uuid());

  m_cluster_impl->setup_read_replica(m_target_instance.get(), ar_options,
                                     replication_sources_opts, true,
                                     m_options.dry_run);

  // Remove the channel last, to ensure the revert updates are
  // propagated
  m_undo_tracker.add("Removing Read-Replica replication channel", [this]() {
    remove_channel(*m_target_instance, k_read_replica_async_channel_name,
                   m_options.dry_run);
    reset_managed_connection_failover(*m_target_instance, m_options.dry_run);
  });

  // Update metadata
  auto rejoin_read_replica_trx_undo = Transaction_undo::create();

  MetadataStorage::Transaction trx(m_cluster_impl->get_metadata_storage());

  m_cluster_impl->get_metadata_storage()->update_read_replica_repl_account(
      m_target_instance->get_uuid(), ar_options.repl_credentials->user,
      repl_account_host, rejoin_read_replica_trx_undo.get());

  trx.commit();
  log_debug("rejoinInstance() metadata updates done");

  m_undo_tracker.add(
      "Removing Read-Replica's metadata",
      Sql_undo_list(std::move(rejoin_read_replica_trx_undo)), [this]() {
        return m_cluster_impl->get_metadata_storage()->get_md_server();
      });

  // Synchronize with source
  try {
    console->print_info();
    console->print_info("* Waiting for Read-Replica '" +
                        m_target_instance->descr() +
                        "' to synchronize with Cluster...");

    if (!m_options.dry_run) {
      m_cluster_impl->sync_transactions(*m_target_instance.get(),
                                        Instance_type::READ_REPLICA,
                                        m_options.timeout);
    }
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
      console->print_warning(
          "The Read-Replica failed to synchronize its transaction set "
          "with the Cluster. You may increase or disable the "
          "transaction sync timeout with the option 'timeout'");
    }
    throw;
  } catch (const cancel_sync &) {
    // Throw it up
    throw;
  }

  // Disable offline_mode if enabled
  if (m_target_instance->get_sysvar_bool("offline_mode", false)) {
    mysqlsh::current_console()->print_note(
        shcore::str_format("Disabling 'offline_mode' on '%s'",
                           m_target_instance->descr().c_str()));
    m_target_instance->set_sysvar("offline_mode", false);
  }

  console->print_info();
  console->print_info("Read-Replica '" + m_target_instance->descr() +
                      "' successfully rejoined to the Cluster '" +
                      m_cluster_impl->get_name() + "'.");
  console->print_info();

  if (m_options.dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

void Rejoin_replica_instance::do_undo() { m_undo_tracker.execute(); }

}  // namespace mysqlsh::dba::cluster
