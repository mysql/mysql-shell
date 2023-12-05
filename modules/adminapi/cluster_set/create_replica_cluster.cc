/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster_set/create_replica_cluster.h"

#include <memory>
#include <vector>

#include "adminapi/common/async_topology.h"
#include "adminapi/common/instance_validations.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/connectivity_check.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/server_features.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "shellcore/console.h"

namespace mysqlsh {
namespace dba {
namespace clusterset {

Create_replica_cluster::Create_replica_cluster(
    Cluster_set_impl *cluster_set, mysqlsh::dba::Instance *primary_instance,
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
    const std::string &cluster_name, Recovery_progress_style progress_style,
    const Create_replica_cluster_options &options)
    : m_cluster_set(cluster_set),
      m_target_instance(target_instance),
      m_primary_instance(primary_instance),
      m_cluster_name(cluster_name),
      m_progress_style(progress_style),
      m_options(options) {
  assert(cluster_set);
}

Create_replica_cluster::~Create_replica_cluster() = default;

shcore::Option_pack_ref<mysqlsh::dba::Create_cluster_options>
Create_replica_cluster::prepare_create_cluster_options() {
  shcore::Option_pack_ref<mysqlsh::dba::Create_cluster_options> options;

  options->dry_run = m_options.dry_run;
  options->replication_allowed_host = m_options.replication_allowed_host;

  options->gr_options.ssl_mode = m_options.gr_options.ssl_mode;
  options->gr_options.ip_allowlist = m_options.gr_options.ip_allowlist;
  options->gr_options.local_address = m_options.gr_options.local_address;
  options->gr_options.exit_state_action =
      m_options.gr_options.exit_state_action;
  options->gr_options.member_weight = m_options.gr_options.member_weight;
  options->gr_options.consistency = m_options.gr_options.consistency;
  options->gr_options.expel_timeout = m_options.gr_options.expel_timeout;
  options->gr_options.auto_rejoin_tries =
      m_options.gr_options.auto_rejoin_tries;
  options->gr_options.manual_start_on_boot =
      m_options.gr_options.manual_start_on_boot;
  options->gr_options.communication_stack =
      m_options.gr_options.communication_stack;
  options->gr_options.paxos_single_leader =
      m_options.gr_options.paxos_single_leader;

  Create_cluster_clone_options no_clone_options;

  options->clone_options = no_clone_options;

  // Set the maximum value for Replica Clusters by default
  // Required to avoid a failure in the ClusterSet replication channel due to
  // transaction sizes in the Replica side being bigger than on the source.
  options->gr_options.transaction_size_limit = static_cast<int64_t>(0);

  // recovery auth options
  options->member_auth_options.member_auth_type =
      m_cluster_set->query_clusterset_auth_type();
  options->member_auth_options.cert_issuer =
      m_cluster_set->query_clusterset_auth_cert_issuer();
  options->member_auth_options.cert_subject = m_options.cert_subject;

  return options;
}

std::shared_ptr<Cluster_impl> Create_replica_cluster::create_cluster_object(
    const mysqlshdk::mysql::Auth_options &repl_credentials,
    const std::string &repl_account_host) {
  std::shared_ptr<Cluster_impl> cluster;

  log_info("Creating Cluster object on %s", m_target_instance->descr().c_str());
  if (!m_options.dry_run) {
    cluster = m_op_create_cluster->execute().as_object<Cluster>()->impl();
    m_op_create_cluster->finish();
  }

  {
    auto auth_type = m_cluster_set->query_clusterset_auth_type();

    current_console()->print_info(
        shcore::str_format("Cluster \"memberAuthType\" is set to '%s' "
                           "(inherited from the ClusterSet).",
                           to_string(auth_type).c_str()));
  }

  // record previously created async replication user for this cluster, now
  // that we have metadata for it
  log_info("Recording metadata for the newly created replication user %s",
           repl_credentials.user.c_str());

  if (m_options.dry_run) return cluster;

  m_cluster_set->record_cluster_replication_user(
      cluster.get(), repl_credentials, repl_account_host);

  // New clusters inherit these attributes

  if (m_cluster_set->get_primary_cluster()->get_gtid_set_is_complete()) {
    m_cluster_set->get_metadata_storage()->update_cluster_attribute(
        cluster->get_id(), k_cluster_attribute_assume_gtid_set_complete,
        shcore::Value::True());
  }

  // Store the value of group_replication_transaction_size_limit inherited from
  // the Primary Cluster
  m_cluster_set->get_metadata_storage()->update_cluster_attribute(
      cluster->get_id(), k_cluster_attribute_transaction_size_limit,
      shcore::Value(get_transaction_size_limit(
          *m_cluster_set->get_primary_cluster()->get_cluster_server())));

  return cluster;
}

void Create_replica_cluster::recreate_recovery_account(
    const std::shared_ptr<Cluster_impl> cluster,
    const std::string &auth_cert_subject, std::string *recovery_user,
    std::string *recovery_host) {
  std::string rec_user, host;

  // Get the recovery account
  std::vector<std::string> recovery_user_hosts;
  std::tie(rec_user, recovery_user_hosts, std::ignore) =
      cluster->get_replication_user(*m_target_instance);

  // Recovery account is coming from the Metadata
  host = recovery_user_hosts.front();

  // Ensure the replication user is deleted if exists before creating it
  // again
  m_primary_instance->drop_user(rec_user, host);

  log_debug("Re-creating temporary recovery user '%s'@'%s' at instance '%s'.",
            rec_user.c_str(), host.c_str(),
            m_primary_instance->descr().c_str());

  std::tie(rec_user, host) =
      cluster->recreate_replication_user(m_target_instance, auth_cert_subject);

  if (recovery_user) *recovery_user = rec_user;
  if (recovery_host) *recovery_host = host;
}

void Create_replica_cluster::prepare() {
  auto console = mysqlsh::current_console();

  // The target instance must comply with the following requirements to become a
  // REPLICA Cluster
  //
  //  - Must be running MySQL >= 8.0.27.
  //  - Must comply with the requirements for InnoDB Cluster.
  //  - The cluster name must be unique within the ClusterSet
  //  - Must not have any pre-existing replication channels configured.
  //  - Must not belong to an InnoDB Cluster or ReplicaSet, i.e. must be a
  //  standalone instance.
  //  - Its `server_id` and `server_uuid` must be unique in the ClusterSet,
  //  including among OFFLINE or unreachable members.
  //  - Must have been pre-configured with the clusterAdmin account used to
  //  create the PRIMARY Cluster.
  //  - Must be able to connect to all members of the PRIMARY cluster and
  //  vice-versa.

  console->print_info(shcore::str_format(
      "Setting up replica '%s' of cluster '%s' at instance '%s'.\n",
      m_cluster_name.c_str(),
      m_cluster_set->get_primary_cluster()->get_name().c_str(),
      m_target_instance->descr().c_str()));

  // Check if the instance is running MySQL >= 8.0.27
  auto instance_version = m_target_instance->get_version();

  if (instance_version < Precondition_checker::k_min_cs_version) {
    console->print_info("The target instance is running MySQL version " +
                        instance_version.get_full() + ", but " +
                        Precondition_checker::k_min_cs_version.get_full() +
                        " is the minimum required for InnoDB ClusterSet.");
    throw shcore::Exception("Unsupported MySQL version",
                            SHERR_DBA_BADARG_VERSION_NOT_SUPPORTED);
  }

  // Check if the cluster name is unique within the ClusterSet
  // NOTE: The cluster name is case insensitive. This is enforced in the
  // Metadata schema by making cluster_name an UNIQUE KEY
  auto clusters = m_cluster_set->get_clusters();

  // Do case-insensitive comparison for the Cluster name to error out right away
  auto it = std::find_if(clusters.begin(), clusters.end(),
                         [&](const Cluster_set_member_metadata &c) {
                           return shcore::str_lower(c.cluster.cluster_name) ==
                                  shcore::str_lower(m_cluster_name);
                         });

  if (it != clusters.end()) {
    console->print_error(
        shcore::str_format("A Cluster named '%s' already exists in the "
                           "ClusterSet. Please use a different name.",
                           m_cluster_name.c_str()));

    throw shcore::Exception("Cluster name already in use.",
                            SHERR_DBA_CLUSTER_NAME_ALREADY_IN_USE);
  }

  // Check if the instance already belongs to an InnoDB Cluster, either ONLINE
  // or OFFLINE
  {
    TargetType::Type instance_type;

    if (!validate_instance_standalone(*m_target_instance, &instance_type)) {
      mysqlsh::current_console()->print_error(
          "The instance '" + m_target_instance->descr() +
          "' is already part of an " + to_string(instance_type) +
          ". A new Replica Cluster must be created on a standalone instance.");

      throw shcore::Exception(
          "Target instance already part of an " + to_string(instance_type),
          SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER);
    }
  }

  // Check if there are unknown replication channels running at the instance
  log_debug(
      "Checking if instance '%s' has asynchronous (source-replica) "
      "replication configured.",
      m_target_instance->get_canonical_address().c_str());

  if (mysqlsh::dba::checks::check_illegal_async_channels(*m_target_instance,
                                                         {})) {
    console->print_error(
        "The target instance '" + m_target_instance->get_canonical_address() +
        "' has asynchronous (source-replica) "
        "replication channel(s) configured which is not supported in "
        "InnoDB ClusterSet.");

    throw shcore::Exception("Unsupported active replication channel.",
                            SHERR_DBA_CLUSTER_UNSUPPORTED_REPLICATION_CHANNEL);
  }

  // Store the current value of super_read_only
  bool super_read_only =
      m_target_instance->get_sysvar_bool("super_read_only", false);

  // Get the ClusterSet configured SSL mode
  m_ssl_mode = m_cluster_set->query_clusterset_ssl_mode();
  if (m_ssl_mode == Cluster_ssl_mode::NONE) m_ssl_mode = Cluster_ssl_mode::AUTO;

  resolve_ssl_mode_option(kClusterSetReplicationSslMode, "Replica Cluster",
                          *m_target_instance, &m_ssl_mode);

  // recovery auth checks
  auto auth_type = m_cluster_set->query_clusterset_auth_type();
  log_debug("ClusterSet recovery auth: %s", to_string(auth_type).c_str());

  // check if member auth request mode is supported
  validate_instance_member_auth_type(*m_target_instance, true, m_ssl_mode,
                                     "memberSslMode", auth_type);

  // check if certSubject was correctly supplied
  validate_instance_member_auth_options("cluster", true, auth_type,
                                        m_options.cert_subject);

  try {
    shcore::Option_pack_ref<mysqlsh::dba::Create_cluster_options> options =
        prepare_create_cluster_options();

    m_op_create_cluster = std::make_unique<Create_cluster>(
        m_cluster_set->get_metadata_storage(), m_target_instance,
        m_cluster_set->get_primary_master(), m_cluster_name, *options);
    // Validations for InnoDB Cluster compliance are done now. It includes:
    //   - clusterName validation.
    //   - Must not belong to an InnoDB Cluster or ReplicaSet, i.e. must be a
    //     standalone instance.
    m_op_create_cluster->prepare();
  } catch (...) {
    // catch any exception that is thrown, restore super read-only-mode if it
    // was enabled and re-throw the caught exception.
    if (super_read_only && !m_options.dry_run) {
      log_debug(
          "Restoring super_read_only mode on instance '%s' to 'ON' since it "
          "was enabled before createReplicaCluster operation started.",
          m_target_instance->descr().c_str());
      m_target_instance->set_sysvar("super_read_only", true);
    }

    throw;
  }

  // Check if the instance `server_id` and `server_uuid` are unique in the
  // ClusterSet, including among OFFLINE or unreachable members.
  m_cluster_set->get_primary_cluster()->validate_server_ids(*m_target_instance);

  // Validate the clone options
  m_options.clone_options.check_option_values(m_target_instance->get_version());

  console->print_info();
  console->print_info("* Checking transaction state of the instance...");

  m_options.clone_options.recovery_method =
      m_cluster_set->validate_instance_recovery(
          Member_op_action::ADD_INSTANCE, *m_target_instance,
          m_options.clone_options.recovery_method.value_or(
              Member_recovery_method::AUTO),
          m_cluster_set->get_primary_cluster()->get_gtid_set_is_complete(),
          current_shell_options()->get().wizards);

  // check connectivity between this and the primary of the clusterset
  if (current_shell_options()->get().dba_connectivity_checks) {
    console->print_info(
        "* Checking connectivity and SSL configuration to PRIMARY Cluster...");

    auto cert_issuer = m_cluster_set->query_clusterset_auth_cert_issuer();
    auto primary_cluster = m_cluster_set->get_primary_cluster();
    auto primary = primary_cluster->get_primary_master();

    test_peer_connection(
        *m_target_instance, m_options.gr_options.local_address.value_or(""),
        m_options.cert_subject, *primary,
        primary->get_sysvar_string("group_replication_local_address")
            .value_or(""),
        primary_cluster->query_cluster_instance_auth_cert_subject(
            primary->get_uuid()),
        m_ssl_mode, auth_type, cert_issuer,
        m_op_create_cluster->get_communication_stack(), true);
  }

  console->print_info();

  DBUG_EXECUTE_IF("dba_abort_create_replica_cluster",
                  { throw std::logic_error("debug"); });
}

shcore::Value Create_replica_cluster::execute() {
  bool sync_transactions_revert = false;
  shcore::Scoped_callback_list undo_list;
  Async_replication_options ar_options;

  log_info("Creating replica cluster on '%s'",
           m_target_instance->descr().c_str());

  try {
    // Do not send the target_instance back to the pool
    m_target_instance->steal();

    std::string repl_account_host;

    // Prepare the replication options
    {
      auto primary_cluster = m_cluster_set->get_primary_cluster();

      ar_options = m_cluster_set->get_clusterset_default_replication_options();

      // "merge" user supplied options into the current ones
      m_options.merge_ar_options(&ar_options);

      // Enable auto-failover
      ar_options.auto_failover = true;

      // Create recovery account
      {
        auto auth_type = primary_cluster->query_cluster_auth_type();
        auto auth_cert_issuer =
            primary_cluster->query_cluster_auth_cert_issuer();

        std::tie(ar_options.repl_credentials, repl_account_host) =
            m_cluster_set->create_cluster_replication_user(
                m_target_instance.get(), "", auth_type, auth_cert_issuer,
                m_options.cert_subject, m_options.dry_run);
      }

      if (!m_options.dry_run) {
        undo_list.push_front([=, this, &sync_transactions_revert]() {
          log_info("Revert: Dropping replication account '%s'",
                   ar_options.repl_credentials->user.c_str());
          m_cluster_set->get_primary_master()->drop_user(
              ar_options.repl_credentials->user, "%", true);

          if (!sync_transactions_revert) return;

          // cannot let an exception proceed, otherwise it will cancel the
          // undo_list rollback
          try {
            m_cluster_set->sync_transactions(*m_target_instance.get(),
                                             {k_clusterset_async_channel_name},
                                             m_options.timeout);
          } catch (const shcore::Exception &e) {
            log_info("Error syncing transactions on '%s': %s",
                     m_target_instance->descr().c_str(), e.what());
          }
        });
      }
    }

    // Handle clone provisioning
    if (*m_options.clone_options.recovery_method ==
        Member_recovery_method::CLONE) {
      // Pick a donor if the option is not used. By default, the donor must be
      // the PRIMARY member of the PRIMARY Cluster.
      std::string donor;
      if (m_options.clone_options.clone_donor.has_value()) {
        donor = *m_options.clone_options.clone_donor;
      } else {
        // Pick the primary instance of the primary cluster as donor
        donor = m_primary_instance->get_canonical_address();
      }

      const auto donor_instance =
          Scoped_instance(m_cluster_set->connect_target_instance(donor));

      // Ensure the donor is valid:
      //   - It's an ONLINE member of the Primary Cluster
      //   - It has the same version of the recipient
      //   - It has the same operating system as the recipient
      m_cluster_set->get_primary_cluster()->ensure_compatible_clone_donor(
          donor_instance, *m_target_instance);

      m_cluster_set->handle_clone_provisioning(
          m_target_instance, donor_instance, ar_options, repl_account_host,
          m_cluster_set->query_clusterset_auth_cert_issuer(),
          m_options.cert_subject, m_progress_style, m_options.timeout,
          m_options.dry_run);
    }

    // Set debug trap to test reversion of replication user creation
    DBUG_EXECUTE_IF("dba_create_replica_cluster_fail_pre_create_cluster",
                    { throw std::logic_error("debug"); });

    // Create a new InnoDB Cluster and update its Metadata
    auto cluster =
        create_cluster_object(*ar_options.repl_credentials, repl_account_host);

    undo_list.push_front([=, this]() {
      log_info("Revert: Dropping Cluster '%s' from Metadata",
               cluster->get_name().c_str());
      MetadataStorage::Transaction trx(m_cluster_set->get_metadata_storage());
      m_cluster_set->get_metadata_storage()->drop_cluster(cluster->get_name());
      trx.commit();
    });

    // store per-cluster replication channel options (cluster can be null
    // because of dry run)
    if (cluster) {
      auto md = cluster->get_metadata_storage();

      auto update_attrib = [&md, &cluster](const auto &option,
                                           std::string_view name) {
        if (!option.has_value()) return;
        md->update_cluster_attribute(cluster->get_id(), name,
                                     shcore::Value(option.value()));
      };

      update_attrib(m_options.ar_options.connect_retry,
                    k_cluster_attribute_repl_connect_retry);
      update_attrib(m_options.ar_options.retry_count,
                    k_cluster_attribute_repl_retry_count);
      update_attrib(m_options.ar_options.heartbeat_period,
                    k_cluster_attribute_repl_heartbeat_period);
      update_attrib(m_options.ar_options.compression_algos,
                    k_cluster_attribute_repl_compression_algorithms);
      update_attrib(m_options.ar_options.zstd_compression_level,
                    k_cluster_attribute_repl_zstd_compression_level);
      update_attrib(m_options.ar_options.bind, k_cluster_attribute_repl_bind);
      update_attrib(m_options.ar_options.network_namespace,
                    k_cluster_attribute_repl_network_namespace);
    }

    // NOTE: This should be the very last thing to be reverted to ensure changes
    // are replicated
    undo_list.push_back([this]() {
      log_info("Revert: Stopping Group Replication on '%s'",
               m_target_instance->descr().c_str());

      try {
        // Stop Group Replication and reset GR variables
        mysqlsh::dba::leave_cluster(*m_target_instance, true);
      } catch (const std::exception &err) {
        mysqlsh::current_console()->print_error("Failed to revert changes: " +
                                                std::string(err.what()));
        throw;
      }
    });

    // Set debug trap to test reversion of the creation of the InnoDB
    // Cluster
    DBUG_EXECUTE_IF("dba_create_replica_cluster_fail_post_create_cluster",
                    { throw std::logic_error("debug"); });

    // Setup the target instance as an async replica of the global primary
    auto console = mysqlsh::current_console();
    console->print_info(
        "* Configuring ClusterSet managed replication channel...");

    DBUG_EXECUTE_IF("dba_create_replica_cluster_source_delay", {
      // Simulate high load on the Primary Cluster (lag) by
      // introducing delay in the replication channel
      ar_options.delay = 5;
    });

    // We can't leave the ClusterSet repl channel in an invalid state (e.g.: due
    // to incorrect repl options supplied by the user)
    undo_list.push_back([&]() {
      log_info("Revert: resetting async repl channel '%s' on '%s'",
               k_clusterset_async_channel_name,
               m_target_instance->descr().c_str());

      stop_channel(*m_target_instance.get(), k_clusterset_async_channel_name,
                   true, m_options.dry_run);

      reset_channel(*m_target_instance.get(), k_clusterset_async_channel_name,
                    true, m_options.dry_run);

      log_info("Revert: reconciling internally generated GTIDs");

      if (!m_options.dry_run)
        m_cluster_set->reconcile_view_change_gtids(m_target_instance.get());
    });

    auto repl_source = m_cluster_set->get_primary_master();

    m_cluster_set->update_replica(m_target_instance.get(), repl_source.get(),
                                  ar_options, true, false, m_options.dry_run);

    // Channel is up, enable transaction sync on revert
    sync_transactions_revert = true;

    // NOTE: This should be done last to allow changes to be replicated
    undo_list.push_back([this]() {
      log_info("Revert: Removing and resetting ClusterSet settings");
      m_cluster_set->remove_replica(m_target_instance.get(), m_options.dry_run);
    });

    // If the communication stack is 'MySQL' the recovery account must be
    // recreated because the account was created with the binary log disabled
    // so any other member joining the cluster afterward won't have the account
    // and in the event of a primary failover of a clusterSet
    // switchover/failover Group replication will fail to start when using this
    // communication stack.
    //
    // The account was created with the binary log disabled because when using
    // the communicationStack 'MySQL' the recovery channel must be set-up before
    // starting GR because GR requires that the recovery channel is configured
    // in order to obtain the credentials information. The account had to be
    // created with the binary log disabled otherwise an errant transaction
    // would be created in the Replica Cluster
    if (!m_options.dry_run &&
        get_communication_stack(*cluster->get_cluster_server()) ==
            kCommunicationStackMySQL) {
      std::string repl_account_user;

      recreate_recovery_account(cluster, m_options.cert_subject,
                                &repl_account_user, &repl_account_host);

      undo_list.push_front([=, this]() {
        log_info("Revert: Dropping replication user '%s'",
                 repl_account_user.c_str());
        m_cluster_set->get_primary_master()->drop_user(repl_account_user,
                                                       repl_account_host, true);
      });
    }

    try {
      console->print_info();
      console->print_info("* Waiting for instance '" +
                          m_target_instance->descr() +
                          "' to synchronize with PRIMARY Cluster...");
      // Sync and check whether the slave started OK
      if (!m_options.dry_run) {
        m_cluster_set->sync_transactions(*m_target_instance.get(),
                                         {k_clusterset_async_channel_name},
                                         m_options.timeout);
      }
    } catch (const shcore::Exception &e) {
      if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
        console->print_info(
            "The instance failed to synchronize its transaction set "
            "with the PRIMARY Cluster. You may increase or disable the "
            "transaction sync timeout with the option 'timeout'");
      }
      throw;
    } catch (const cancel_sync &) {
      // Throw it up
      throw;
    }

    // set the replica cluster instance as super_read_only and enable
    // skip_replica_start
    log_info("Enabling super_read_only and skip_replica_start on %s",
             m_target_instance->descr().c_str());
    if (!m_options.dry_run) {
      bool super_read_only =
          m_target_instance->get_sysvar_bool("super_read_only", false);
      bool skip_replica_start =
          m_target_instance->get_sysvar_bool("skip_replica_start", false);

      if (!super_read_only) {
        m_target_instance->set_sysvar("super_read_only", true);
      }

      m_target_instance->set_sysvar(
          "skip_replica_start", true,
          mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);

      undo_list.push_front([=, this]() {
        if (skip_replica_start) {
          log_info("revert: Clearing skip_replica_start");
          m_target_instance->set_sysvar(
              "skip_replica_start", false,
              mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);
        }
      });
    }

    // Set debug trap to test reversion of the ClusterSet setting set-up
    DBUG_EXECUTE_IF("dba_create_replica_cluster_fail_setup_cs_settings",
                    { throw std::logic_error("debug"); });

    // update clusterset metadata
    console->print_info("* Updating topology");

    if (!m_options.dry_run) {
      auto metadata = m_cluster_set->get_metadata_storage();

      Cluster_set_member_metadata cluster_md;
      cluster_md.cluster.cluster_id = cluster->get_id();
      cluster_md.cluster_set_id = m_cluster_set->get_id();
      cluster_md.master_cluster_id =
          m_cluster_set->get_primary_cluster()->get_id();
      cluster_md.primary_cluster = false;

      log_info("Updating ClusterSet metadata");
      MetadataStorage::Transaction trx(metadata);
      metadata->record_cluster_set_member_added(cluster_md);
      trx.commit();
    }

    undo_list.cancel();

    log_debug("ClusterSet metadata updates done");

    // Sync with the primary Cluster to ensure the Metadata changes are
    // propagated to the new Replica Cluster before returning, otherwise, any
    // attempt to get this Cluster's object from a connection to one of its
    // members may result in the object having the wrong view of the topology.
    // i.e. not understanding its part of a ClusterSet
    console->print_info();
    console->print_info(
        "* Waiting for the Cluster to synchronize with the PRIMARY Cluster...");
    if (!m_options.dry_run) {
      m_cluster_set->sync_transactions(*m_target_instance.get(),
                                       {k_clusterset_async_channel_name},
                                       m_options.timeout);
    }

    console->print_info();
    console->print_info("Replica Cluster '" + m_cluster_name +
                        "' successfully created on ClusterSet '" +
                        m_cluster_set->get_name() + "'.");
    console->print_info();

    if (m_options.dry_run) {
      console->print_info();
      console->print_info("dryRun finished.");
      console->print_info();

      return shcore::Value::Null();
    } else {
      return shcore::Value(std::static_pointer_cast<shcore::Object_bridge>(
          std::make_shared<Cluster>(cluster)));
    }
  } catch (...) {
    auto console = mysqlsh::current_console();

    console->print_error("Error creating Replica Cluster: " +
                         format_active_exception());

    console->print_note("Reverting changes...");

    undo_list.call();

    console->print_info();
    console->print_info("Changes successfully reverted.");

    throw;
  }
}

}  // namespace clusterset
}  // namespace dba
}  // namespace mysqlsh
