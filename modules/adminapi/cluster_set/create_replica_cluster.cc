/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "adminapi/common/async_topology.h"
#include "adminapi/common/instance_validations.h"
#include "adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/cluster/create_cluster_set.h"
#include "modules/adminapi/cluster_set/create_replica_cluster.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/instance_monitoring.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/provision.h"
#include "mysql/clone.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
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

Create_replica_cluster::~Create_replica_cluster() {}

shcore::Option_pack_ref<mysqlsh::dba::Create_cluster_options>
Create_replica_cluster::prepare_create_cluster_options() {
  shcore::Option_pack_ref<mysqlsh::dba::Create_cluster_options> options;

  options->dry_run = m_options.dry_run;
  options->replication_allowed_host = m_options.replication_allowed_host;

  options->gr_options.ssl_mode = m_options.gr_options.ssl_mode;
  options->gr_options.ip_allowlist = m_options.gr_options.ip_allowlist;
  options->gr_options.ip_allowlist_option_name =
      m_options.gr_options.ip_allowlist_option_name;
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

  Create_cluster_clone_options no_clone_options;

  options->clone_options = no_clone_options;

  return options;
}

void Create_replica_cluster::ensure_compatible_clone_donor(
    const std::string &instance_def) {
  /*
   * A donor is compatible if:
   *
   *   - It's an ONLINE member of the Primary Cluster
   *   - It has the same version of the recipient
   *   - It has the same operating system as the recipient
   */

  auto console = current_console();

  const auto target =
      Scoped_instance(m_cluster_set->connect_target_instance(instance_def));

  // Check if the target belongs to the PRIMARY Cluster (MD)
  std::string target_address = target->get_canonical_address();
  try {
    m_cluster_set->get_metadata_storage()->get_instance_by_address(
        target_address);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
      throw shcore::Exception("Instance " + target_address +
                                  " does not belong to the Primary Cluster",
                              SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);
    }
    throw;
  }

  auto target_status = mysqlshdk::gr::get_member_state(*target);

  if (target_status != mysqlshdk::gr::Member_state::ONLINE) {
    throw shcore::Exception(
        "Instance " + target_address +
            " is not an ONLINE member of the Primary Cluster.",
        SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE);
  }

  // Check if the instance has the same version of the recipient
  if (target->get_version() != m_target_instance->get_version()) {
    throw shcore::Exception("Instance " + target_address +
                                " cannot be a donor because it has a different "
                                "version than the recipient.",
                            SHERR_DBA_CLONE_DIFF_VERSION);
  }

  // Check if the instance has the same OS the recipient
  if (target->get_version_compile_os() !=
      m_target_instance->get_version_compile_os()) {
    throw shcore::Exception("Instance " + target_address +
                                " cannot be a donor because it has a different "
                                "Operating System than the recipient.",
                            SHERR_DBA_CLONE_DIFF_OS);
  }

  // Check if the instance is running on the same platform of the recipient
  if (target->get_version_compile_machine() !=
      m_target_instance->get_version_compile_machine()) {
    throw shcore::Exception(
        "Instance " + target_address +
            " cannot be a donor because it is running on a different "
            "platform than the recipient.",
        SHERR_DBA_CLONE_DIFF_PLATFORM);
  }
}

void Create_replica_cluster::handle_clone(
    const Async_replication_options &ar_options,
    const std::string &repl_account_host, bool dry_run) {
  // TODO(alfredo) merge this with the handle_clone in Cluster addinst?
  auto console = current_console();
  /*
   * Clone handling:
   *
   * 1.  Pick a valid donor (unless cloneDonor is set). By default, the donor
   *     must be an ONLINE SECONDARY member. If not available, then must be the
   *     PRIMARY.
   * 2.  Install the Clone plugin on the donor and recipient (if not installed
   *     already)
   * 3.  Set the donor to the selected donor: SET GLOBAL clone_valid_donor_list
   *     = "donor_host:donor_port";
   * 4.  Create or update a recovery account with the required
   *     privileges for replicaSets management + clone usage (BACKUP_ADMIN)
   * 5.  Ensure the donor's recovery account has the clone usage required
   *     privileges: BACKUP_ADMIN
   * 6.  Grant the CLONE_ADMIN privilege to the recovery account
   *     at the recipient
   * 7.  Create the SQL clone command based on the above
   * 8.  Execute the clone command
   */

  // Ensure the cloneDonor is valid
  std::string donor;
  if (!m_options.clone_options.clone_donor.is_null()) {
    ensure_compatible_clone_donor(
        m_options.clone_options.clone_donor.get_safe());
    donor = m_options.clone_options.clone_donor.get_safe();
  } else {
    // Pick the primary instance of the primary cluster as donor
    donor = m_primary_instance->get_canonical_address();
  }

  // Install the clone plugin on the recipient and donor
  // If cloneDonor was set, establish a connection to it. Otherwise, the primary
  // instance will be used as donor
  // auto donor_instance = m_primary_instance;
  const auto donor_instance =
      Scoped_instance(m_cluster_set->connect_target_instance(donor));

  log_info("Installing the clone plugin on donor '%s'%s.",
           donor_instance->get_canonical_address().c_str(),
           dry_run ? " (dryRun)" : "");

  if (!dry_run) {
    mysqlshdk::mysql::install_clone_plugin(*donor_instance, nullptr);
  }

  log_info("Installing the clone plugin on recipient '%s'%s.",
           m_target_instance->get_canonical_address().c_str(),
           dry_run ? " (dryRun)" : "");

  if (!dry_run) {
    mysqlshdk::mysql::install_clone_plugin(*m_target_instance, nullptr);
  }

  // Set the donor to the selected donor on the recipient
  if (!dry_run) {
    m_target_instance->set_sysvar("clone_valid_donor_list", donor);
  }

  // Create or update a recovery account with the required privileges for
  // replicaSets management + clone usage (BACKUP_ADMIN) on the recipient

  // Check if super_read_only is enabled. If so it must be disabled to create
  // the account
  if (!dry_run &&
      m_target_instance->get_sysvar_bool("super_read_only").get_safe()) {
    m_target_instance->set_sysvar("super_read_only", false);
  }

  // Clone requires a user in both donor and recipient:
  //
  // On the donor, the clone user requires the BACKUP_ADMIN privilege for
  // accessing and transferring data from the donor, and for blocking DDL
  // during the cloning operation.
  //
  // On the recipient, the clone user requires the CLONE_ADMIN privilege for
  // replacing recipient data, blocking DDL during the cloning operation, and
  // automatically restarting the server. The CLONE_ADMIN privilege includes
  // BACKUP_ADMIN and SHUTDOWN privileges implicitly.
  //
  // For that reason, we create a user in the recipient with the same username
  // and password as the replication user created in the donor.
  create_clone_recovery_user_nobinlog(m_target_instance.get(),
                                      *ar_options.repl_credentials,
                                      repl_account_host, m_options.dry_run);

  if (!dry_run) {
    // Ensure the donor's recovery account has the clone usage required
    // privileges: BACKUP_ADMIN
    m_primary_instance->executef("GRANT BACKUP_ADMIN ON *.* TO ?@?",
                                 ar_options.repl_credentials->user,
                                 repl_account_host);

    // If the donor instance is processing transactions, it may have the
    // clone-user handling (create + grant) still in the backlog waiting to be
    // applied. For that reason, we must wait for it to be in sync with the
    // primary before starting the clone itself (BUG#30628746)
    std::string primary_address = m_primary_instance->get_canonical_address();

    std::string donor_address = donor_instance->get_canonical_address();

    if (primary_address != donor_address) {
      console->print_info(
          "* Waiting for the donor to synchronize with PRIMARY...");
      if (!dry_run) {
        try {
          // Sync the donor with the primary
          m_cluster_set->sync_transactions(*donor_instance, "",
                                           m_options.timeout);
        } catch (const shcore::Exception &e) {
          if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
            console->print_error(
                "The donor instance failed to synchronize its transaction set "
                "with the PRIMARY. You may increase or disable the transaction "
                "sync timeout with the option 'timeout'");
          }
          throw;
        } catch (const cancel_sync &) {
          // Throw it up
          throw;
        }
      }
      console->print_info();
    }

    // Create a new connection to the recipient and run clone asynchronously
    std::string instance_def =
        m_target_instance->get_connection_options().uri_endpoint();

    const auto recipient_clone =
        Scoped_instance(m_cluster_set->connect_target_instance(instance_def));

    mysqlshdk::db::Connection_options clone_donor_opts(donor);

    // we need a point in time as close as possible, but still earlier than
    // when recovery starts to monitor the recovery phase. The timestamp
    // resolution is timestamp(3) irrespective of platform
    std::string begin_time =
        m_target_instance->queryf_one_string(0, "", "SELECT NOW(3)");

    std::exception_ptr error_ptr;

    auto clone_thread = spawn_scoped_thread(
        [&recipient_clone, clone_donor_opts, ar_options, &error_ptr] {
          mysqlsh::thread_init();

          try {
            mysqlshdk::mysql::do_clone(recipient_clone, clone_donor_opts,
                                       *ar_options.repl_credentials);
          } catch (const shcore::Error &err) {
            // Clone canceled
            if (err.code() == ER_QUERY_INTERRUPTED) {
              log_info("Clone canceled: %s", err.format().c_str());
            } else {
              log_info("Error cloning from instance '%s': %s",
                       clone_donor_opts.uri_endpoint().c_str(),
                       err.format().c_str());
              error_ptr = std::current_exception();
            }
          } catch (const std::exception &err) {
            log_info("Error cloning from instance '%s': %s",
                     clone_donor_opts.uri_endpoint().c_str(), err.what());
            error_ptr = std::current_exception();
          }

          mysqlsh::thread_end();
        });

    shcore::Scoped_callback join([&clone_thread, error_ptr]() {
      if (clone_thread.joinable()) clone_thread.join();
    });

    try {
      auto post_clone_auth = m_target_instance->get_connection_options();
      post_clone_auth.set_login_options_from(
          donor_instance->get_connection_options());

      monitor_standalone_clone_instance(
          m_target_instance->get_connection_options(), post_clone_auth,
          begin_time, m_progress_style, k_clone_start_timeout,
          current_shell_options()->get().dba_restart_wait_timeout);

      // When clone is used, the target instance will restart and all
      // connections are closed so we need to test if the connection to the
      // target instance and MD are closed and re-open if necessary
      refresh_target_connections(m_target_instance.get());
      refresh_target_connections(
          m_cluster_set->get_metadata_storage()->get_md_server().get());

      // Remove the BACKUP_ADMIN grant from the recovery account
      m_primary_instance->executef("REVOKE BACKUP_ADMIN ON *.* FROM ?@?",
                                   ar_options.repl_credentials->user,
                                   repl_account_host);
    } catch (const stop_monitoring &) {
      console->print_info();
      console->print_note("Recovery process canceled. Reverting changes...");

      // Cancel the clone query
      mysqlshdk::mysql::cancel_clone(*m_target_instance);

      log_info("Clone canceled.");

      log_debug("Waiting for clone thread...");
      // wait for the clone thread to finish
      clone_thread.join();
      log_debug("Clone thread joined");

      // When clone is canceled, the target instance will restart and all
      // connections are closed so we need to wait for the server to start-up
      // and re-establish the session. Also we need to test if the connection
      // to the target instance and MD are closed and re-open if necessary
      *m_target_instance = *wait_server_startup(
          m_target_instance->get_connection_options(),
          mysqlshdk::mysql::k_server_recovery_restart_timeout,
          Recovery_progress_style::NOWAIT);

      refresh_target_connections(m_target_instance.get());

      // Remove the BACKUP_ADMIN grant from the recovery account
      m_primary_instance->executef("REVOKE BACKUP_ADMIN ON *.* FROM ?",
                                   ar_options.repl_credentials->user);

      cleanup_clone_recovery(m_target_instance.get(),
                             *ar_options.repl_credentials, repl_account_host);

      // Thrown the exception cancel_sync up
      throw cancel_sync();
    } catch (const restart_timeout &) {
      console->print_warning(
          "Clone process appears to have finished and tried to restart the "
          "MySQL server, but it has not yet started back up.");

      console->print_info();
      console->print_info(
          "Please make sure the MySQL server at '" +
          m_target_instance->descr() +
          "' is properly restarted. The operation will be reverted, but you may"
          " retry adding the instance after restarting it. ");

      throw shcore::Exception("Timeout waiting for server to restart",
                              SHERR_DBA_SERVER_RESTART_TIMEOUT);
    } catch (const shcore::Error &e) {
      throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
    }
  }
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

  // record previously created async replication user for this cluster, now
  // that we have metadata for it
  log_info("Recording metadata for the newly created replication user %s",
           repl_credentials.user.c_str());

  if (!m_options.dry_run) {
    m_cluster_set->record_cluster_replication_user(
        cluster.get(), repl_credentials, repl_account_host);
  }

  // New clusters inherit this attribute
  if (m_cluster_set->get_primary_cluster()->get_gtid_set_is_complete() &&
      !m_options.dry_run) {
    m_cluster_set->get_metadata_storage()->update_cluster_attribute(
        cluster->get_id(), k_cluster_attribute_assume_gtid_set_complete,
        shcore::Value::True());
  }

  return cluster;
}

void Create_replica_cluster::recreate_recovery_account(
    const std::shared_ptr<Cluster_impl> cluster, std::string *recovery_user,
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
      cluster->recreate_replication_user(m_target_instance);

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

  console->print_info("Setting up replica '" + m_cluster_name +
                      "' of cluster '" +
                      m_cluster_set->get_primary_cluster()->get_name() +
                      "' at instance '" + m_target_instance->descr() + "'.\n");

  // Check if the instance is running MySQL >= 8.0.27
  auto instance_version = m_target_instance->get_version();

  if (instance_version < k_min_cs_version) {
    console->print_info("The target instance is running MySQL version " +
                        instance_version.get_full() + ", but " +
                        k_min_cs_version.get_full() +
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
                         [&](const Cluster_metadata &c) {
                           return shcore::str_lower(c.cluster_name) ==
                                  shcore::str_lower(m_cluster_name);
                         });

  if (it != clusters.end()) {
    console->print_error(
        "A Cluster named '" + m_cluster_name +
        "' already exists in the ClusterSet. Please use a different name.");

    throw shcore::Exception("Cluster name already in use.",
                            SHERR_DBA_CLUSTER_NAME_ALREADY_IN_USE);
  }

  // Check if the instance already belongs to an InnoDB Cluster, either ONLINE
  // or OFFLINE
  {
    std::string managed_type;
    auto md_storage = std::make_unique<MetadataStorage>(m_target_instance);
    auto state = get_cluster_check_info(*md_storage, nullptr, true);

    switch (state.source_type) {
      case TargetType::AsyncReplicaSet:
        managed_type = "an InnoDB ReplicaSet";
        break;
      case TargetType::Standalone:
      case TargetType::StandaloneWithMetadata:
        break;
      case TargetType::StandaloneInMetadata:
      case TargetType::InnoDBCluster:
      case TargetType::InnoDBClusterSet:
      case TargetType::InnoDBClusterSetOffline:
        managed_type = "an InnoDB Cluster";
        break;
      case TargetType::GroupReplication:
        managed_type = "a Group Replication group";
        break;
      case TargetType::Unknown:
        assert(0);
        throw std::logic_error("Unexpected state in " +
                               m_target_instance->descr());
    }

    if (!managed_type.empty()) {
      console->print_error(
          "The instance '" + m_target_instance->descr() +
          "' is already part of " + managed_type +
          ". A new Replica Cluster must be created on a standalone instance.");

      throw shcore::Exception("Target instance already part of " + managed_type,
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
      m_target_instance->get_sysvar_bool("super_read_only").get_safe();

  try {
    shcore::Option_pack_ref<mysqlsh::dba::Create_cluster_options> options =
        prepare_create_cluster_options();

    m_op_create_cluster = std::make_unique<Create_cluster>(
        m_cluster_set->get_metadata_storage(), m_target_instance,
        m_cluster_name, *options);
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
  std::string target_instance_uuid = m_target_instance->get_uuid();
  int64_t target_instance_id = m_target_instance->get_id();

  for (const auto &cluster : clusters) {
    auto cluster_members =
        m_cluster_set->get_metadata_storage()->get_all_instances(
            cluster.cluster_id);

    for (const auto &member : cluster_members) {
      if (member.uuid == target_instance_uuid) {
        console->print_error(
            "The target instance '" +
            m_target_instance->get_canonical_address() +
            "' has a 'server_uuid' already being used by a member "
            "of the ClusterSet.");
        throw shcore::Exception("Invalid server_uuid.",
                                SHERR_DBA_INVALID_SERVER_UUID);
      }

      if (member.server_id == target_instance_id) {
        console->print_error(
            "The target instance '" +
            m_target_instance->get_canonical_address() +
            "' has a 'server_id' already being used by a member "
            "of the ClusterSet.");
        throw shcore::Exception("Invalid server_id.",
                                SHERR_DBA_INVALID_SERVER_ID);
      }
    }
  }

  // Validate the clone options
  m_options.clone_options.check_option_values(m_target_instance->get_version());

  console->print_info();
  console->print_info("* Checking transaction state of the instance...");

  m_options.clone_options.recovery_method =
      m_cluster_set->validate_instance_recovery(
          Member_op_action::ADD_INSTANCE, m_target_instance.get(),
          m_options.clone_options.recovery_method.get_safe(),
          m_cluster_set->get_primary_cluster()->get_gtid_set_is_complete(),
          m_options.interactive());

  // Get the ClusterSet configured SSL mode
  shcore::Value ssl_mode;
  m_cluster_set->get_metadata_storage()->query_cluster_set_attribute(
      m_cluster_set->get_id(), k_cluster_set_attribute_ssl_mode, &ssl_mode);

  std::string ssl_mode_str = ssl_mode.as_string();
  m_ssl_mode = to_cluster_ssl_mode(ssl_mode_str);

  // Validate if the target instance supports SSL
  std::string have_ssl = *m_target_instance->get_sysvar_string("have_ssl");

  // The instance does not support SSL
  if (shcore::str_casecmp(have_ssl.c_str(), "YES") != 0 &&
      m_ssl_mode == Cluster_ssl_mode::REQUIRED) {
    console->print_error(
        "The ClusterSet's clusterSetReplicationSslMode is 'REQUIRED', "
        "however, the target instance does not support SSL. Please enable SSL "
        "support on the target instance.");

    throw shcore::Exception("Unsupported SSL Mode.",
                            SHERR_DBA_CLUSTER_UNSUPPORTED_SSL_MODE);
  }

  DBUG_EXECUTE_IF("dba_abort_create_replica_cluster",
                  { throw std::logic_error("debug"); });
}

shcore::Value Create_replica_cluster::execute() {
  auto console = mysqlsh::current_console();
  shcore::Value ret_val;
  shcore::Scoped_callback_list undo_list;
  Async_replication_options ar_options;

  try {
    // Do not send the target_instance back to the pool
    m_target_instance->steal();

    std::string repl_account_host;

    // Prepare the replication options
    {
      ar_options = m_cluster_set->get_clusterset_replication_options();

      // Enable auto-failover
      ar_options.auto_failover = true;

      // Create recovery account
      std::tie(ar_options.repl_credentials, repl_account_host) =
          m_cluster_set->create_cluster_replication_user(
              m_target_instance.get(), "", m_options.dry_run);

      if (!m_options.dry_run) {
        undo_list.push_front([=]() {
          log_info("Revert: Dropping replication account '%s'",
                   ar_options.repl_credentials->user.c_str());
          m_cluster_set->get_primary_master()->drop_user(
              ar_options.repl_credentials->user, "%", true);
        });
      }
    }

    // Handle clone provisioning
    if (m_options.clone_options.recovery_method ==
        Member_recovery_method::CLONE) {
      try {
        // Do and monitor the clone
        handle_clone(ar_options, repl_account_host, m_options.dry_run);

        // When clone is used, the target instance will restart and all
        // connections are closed so we need to test if the connection to the
        // target instance and MD are closed and re-open if necessary
        refresh_target_connections(m_target_instance.get());
      } catch (const cancel_sync &) {
        // Throw a real exception to be formatted in the outer catch.
        // cancel_sync is an empty std::exception
        throw shcore::Exception("Clone based recovery canceled.",
                                SHERR_DBA_CLONE_CANCELED);
      }
    }

    // Set debug trap to test reversion of replication user creation
    DBUG_EXECUTE_IF("dba_create_replica_cluster_fail_pre_create_cluster",
                    { throw std::logic_error("debug"); });

    // Create a new InnoDB Cluster and update its Metadata
    auto repl_user = *ar_options.repl_credentials;
    auto cluster = create_cluster_object(repl_user, repl_account_host);

    undo_list.push_front([=]() {
      log_info("Revert: Dropping Cluster '%s' from Metadata",
               cluster->get_name().c_str());
      m_cluster_set->get_metadata_storage()->drop_cluster(cluster->get_name());
    });

    // NOTE: This should be the very last thing to be reverted to ensure changes
    // are replicated
    undo_list.push_back([=]() {
      log_info("Revert: Stopping Group Replication on '%s'",
               m_target_instance->descr().c_str());

      try {
        // Stop Group Replication and reset GR variables
        mysqlsh::dba::leave_cluster(*m_target_instance, true);
      } catch (const std::exception &err) {
        console->print_error("Failed to revert changes: " +
                             std::string(err.what()));
        throw;
      }
    });

    // Set debug trap to test reversion of the creation of the InnoDB
    // Cluster
    DBUG_EXECUTE_IF("dba_create_replica_cluster_fail_post_create_cluster",
                    { throw std::logic_error("debug"); });

    // Setup the target instance as an async replica of the global primary
    console->print_info(
        "* Configuring ClusterSet managed replication channel...");

    m_cluster_set->update_replica(m_target_instance.get(), m_primary_instance,
                                  ar_options, true, m_options.dry_run);

    // NOTE: This should be done last to allow changes to be replicated
    undo_list.push_back([=]() {
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
        cluster->get_communication_stack() == kCommunicationStackMySQL) {
      std::string repl_account_user;

      recreate_recovery_account(cluster, &repl_account_user,
                                &repl_account_host);

      undo_list.push_front([&]() {
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
          m_target_instance->get_sysvar_bool("super_read_only").get_safe();
      bool skip_replica_start =
          m_target_instance->get_sysvar_bool("skip_replica_start").get_safe();

      if (!super_read_only) {
        m_target_instance->set_sysvar("super_read_only", true);
      }

      m_target_instance->set_sysvar(
          "skip_replica_start", true,
          mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);

      undo_list.push_front([=]() {
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
      metadata->record_cluster_set_member_added(cluster_md);
    }

    undo_list.cancel();

    log_debug("ClusterSet metadata updates done");

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
