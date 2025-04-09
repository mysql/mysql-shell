/*
 * Copyright (c) 2016, 2025, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/base_cluster_impl.h"

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/instance_monitoring.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/common/routing_guideline_impl.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/setup_account.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mutable_result.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/mysql/version_compatibility.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/version.h"
#include "scripting/types.h"
#include "utils/logger.h"
#include "utils/utils_string.h"

namespace mysqlsh::dba {

// # of seconds to wait until clone starts
constexpr std::chrono::seconds k_clone_start_timeout{30};

namespace {
/**
 * List with the supported build-in tags for setOption and setInstanceOption
 */
const built_in_tags_map_t k_supported_set_option_tags{
    {"_hidden", shcore::Value_type::Bool},
    {"_disconnect_existing_sessions_when_hidden", shcore::Value_type::Bool}};
}  // namespace

Base_cluster_impl::Base_cluster_impl(
    std::string cluster_name, std::shared_ptr<Instance> group_server,
    std::shared_ptr<MetadataStorage> metadata_storage)
    : m_cluster_name{std::move(cluster_name)},
      m_cluster_server{std::move(group_server)},
      m_metadata_storage{std::move(metadata_storage)} {
  if (m_cluster_server) {
    m_cluster_server->retain();

    m_admin_credentials.get(m_cluster_server->get_connection_options());
  }
}

Base_cluster_impl::~Base_cluster_impl() { disconnect_internal(); }

void Base_cluster_impl::disconnect() { disconnect_internal(); }

void Base_cluster_impl::target_server_invalidated() {
  if (m_cluster_server && m_primary_master) {
    m_cluster_server->release();
    m_cluster_server = m_primary_master;
    m_cluster_server->retain();
  } else {
    // find some other server to connect to?
  }
}

bool Base_cluster_impl::check_valid() const {
  if (!get_cluster_server() || !get_cluster_server()->get_session() ||
      !get_cluster_server()->get_session()->is_open() ||
      !get_metadata_storage()) {
    return false;
  }
  // Check if the server is still reachable even though the session is open
  try {
    get_cluster_server()->execute("select 1");
  } catch (const shcore::Error &e) {
    if (mysqlshdk::db::is_mysql_client_error(e.code())) {
      return false;
    } else {
      throw;
    }
  }
  return true;
}

void Base_cluster_impl::check_preconditions(const Command_conditions &conds) {
  log_debug("Checking '%s' preconditions.", conds.name.c_str());

  // Makes sure the metadata state is re-loaded on each API call
  m_metadata_storage->invalidate_cached();

  // Makes sure the primary master is reset before acquiring it on each API call
  m_primary_master.reset();

  bool primary_available = false;
  try {
    current_ipool()->set_metadata(get_metadata_storage());

    primary_available =
        (acquire_primary(conds.primary_required, true) != nullptr);
    log_debug("Acquired primary: %d", primary_available);

  } catch (const shcore::Exception &e) {
    switch (e.code()) {
      case SHERR_DBA_GROUP_HAS_NO_QUORUM:
        log_debug(
            "Cluster has no quorum and cannot process write transactions: %s",
            e.what());
        break;
      case SHERR_DBA_GROUP_MEMBER_NOT_ONLINE:
      case SHERR_DBA_GROUP_UNREACHABLE:
      case SHERR_DBA_GROUP_OFFLINE:
        log_debug("No PRIMARY member available: %s", e.what());
        break;
      default:
        if (shcore::str_beginswith(
                e.what(), "Failed to execute query on Metadata server"))
          log_debug("Unable to query Metadata schema: %s", e.what());
        else if (mysqlshdk::db::is_mysql_client_error(e.code()))
          log_warning("Connection error acquiring primary: %s",
                      e.format().c_str());
        else
          throw;
    }
  }

  check_function_preconditions(conds, get_metadata_storage(),
                               get_cluster_server(), primary_available);
}

bool Base_cluster_impl::get_gtid_set_is_complete() const {
  shcore::Value flag;
  if (get_metadata_storage()->query_cluster_attribute(
          get_id(), k_cluster_attribute_assume_gtid_set_complete, &flag))
    return flag.as_bool();
  return false;
}

void Base_cluster_impl::sync_transactions(
    const mysqlshdk::mysql::IInstance &target_instance,
    const std::string &channel_name, int timeout, bool only_received,
    bool silent) const {
  DBUG_EXECUTE_IF("dba_sync_transactions_timeout", {
    throw shcore::Exception(
        "Timeout reached waiting for all received transactions "
        "to be applied on instance '" +
            target_instance.descr() + "' (debug)",
        SHERR_DBA_GTID_SYNC_TIMEOUT);
  });

  if (only_received) {
    std::string gtid_set =
        mysqlshdk::mysql::get_received_gtid_set(target_instance, channel_name);

    bool sync_res = wait_for_gtid_set_safe(target_instance, gtid_set,
                                           channel_name, timeout, true, silent);

    if (!sync_res) {
      throw shcore::Exception(
          "Timeout reached waiting for all received transactions "
          " to be applied on instance '" +
              target_instance.descr() + "'",
          SHERR_DBA_GTID_SYNC_TIMEOUT);
    }
  } else {
    auto master = get_primary_master();

    std::string gtid_set = mysqlshdk::mysql::get_executed_gtid_set(*master);

    bool sync_res = wait_for_gtid_set_safe(target_instance, gtid_set,
                                           channel_name, timeout, true, silent);

    if (!sync_res) {
      throw shcore::Exception(
          "Timeout reached waiting for transactions from " + master->descr() +
              " to be applied on instance '" + target_instance.descr() + "'",
          SHERR_DBA_GTID_SYNC_TIMEOUT);
    }
  }

  if (!silent) {
    current_console()->print_info();
    current_console()->print_info();
  }
}

void Base_cluster_impl::sync_transactions(
    const mysqlshdk::mysql::IInstance &target_instance,
    Instance_type instance_type, int timeout, bool only_received,
    bool silent) const {
  switch (instance_type) {
    case Instance_type::GROUP_MEMBER: {
      sync_transactions(target_instance, mysqlshdk::gr::k_gr_applier_channel,
                        timeout, only_received, silent);
      break;
    }
    case Instance_type::ASYNC_MEMBER: {
      sync_transactions(target_instance, k_replicaset_channel_name, timeout,
                        only_received, silent);
      break;
    }
    case Instance_type::READ_REPLICA: {
      sync_transactions(target_instance, k_read_replica_async_channel_name,
                        timeout, only_received, silent);
      break;
    }
    case Instance_type::NONE:
      break;
  }
}

void Base_cluster_impl::create_clone_recovery_user_nobinlog(
    mysqlshdk::mysql::IInstance *target_instance,
    const mysqlshdk::mysql::Auth_options &donor_account,
    const std::string &account_host, const std::string &account_cert_issuer,
    const std::string &account_cert_subject, bool dry_run) {
  log_info("Creating clone recovery user %s@%s at %s%s.",
           donor_account.user.c_str(), account_host.c_str(),
           target_instance->descr().c_str(), dry_run ? " (dryRun)" : "");

  if (dry_run) return;

  try {
    mysqlshdk::mysql::Suppress_binary_log nobinlog(*target_instance);

    // Create recovery user for clone equal to the donor user

    mysqlshdk::mysql::IInstance::Create_user_options options;
    options.password = donor_account.password;
    options.cert_issuer = account_cert_issuer;
    options.cert_subject = account_cert_subject;
    options.grants.push_back({"CLONE_ADMIN, EXECUTE", "*.*", false});
    options.grants.push_back({"SELECT", "performance_schema.*", false});

    mysqlshdk::mysql::create_user(*target_instance, donor_account.user,
                                  {account_host}, options);
  } catch (const shcore::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}

void Base_cluster_impl::handle_clone_provisioning(
    const std::shared_ptr<mysqlsh::dba::Instance> &recipient,
    const std::shared_ptr<mysqlsh::dba::Instance> &donor,
    const Async_replication_options &ar_options,
    const std::string &repl_account_host, const std::string &cert_issuer,
    const std::string &cert_subject,
    const Recovery_progress_style &progress_style, int sync_timeout,
    bool dry_run) {
  auto console = current_console();

  // When clone is used, always stop the channels using automatic-failover
  // first regardless of its status. This is required to avoid a blocking
  // scenario on which the replication channel is attempting to connect to the
  // source and failing while clone is trying to drop data. Clone is blocked
  // until the channel gives up and stops.
  {
    try {
      stop_channel(*recipient, k_read_replica_async_channel_name, true,
                   dry_run);
    } catch (std::exception &e) {
      mysqlsh::current_console()->print_error(
          shcore::str_format("Failed to stop replication channel '%s' on '%s', "
                             "please retry the operation: '%s'",
                             k_read_replica_async_channel_name,
                             recipient->descr().c_str(), e.what()));
      throw shcore::Exception("Failed to stop replication channel.",
                              SHERR_DBA_FAILED_TO_STOP_CHANNEL);
    }

    try {
      stop_channel(*recipient, k_clusterset_async_channel_name, true, dry_run);
    } catch (std::exception &e) {
      mysqlsh::current_console()->print_error(
          shcore::str_format("Failed to stop replication channel '%s' on '%s', "
                             "please retry the operation: '%s'",
                             k_clusterset_async_channel_name,
                             recipient->descr().c_str(), e.what()));
      throw shcore::Exception("Failed to stop replication channel.",
                              SHERR_DBA_FAILED_TO_STOP_CHANNEL);
    }
  }

  /*
   * Clone handling:
   *
   * 1.  Install the Clone plugin on the donor and source
   * 2.  Set the donor to the source: SET GLOBAL clone_valid_donor_list
   *     = "donor_host:donor_port";
   * 3.  Create or update a recovery account with the required
   *     privileges for replicaSets management + clone usage (BACKUP_ADMIN)
   * 4.  Ensure the donor's recovery account has the clone usage required
   *     privileges: BACKUP_ADMIN
   * 5.  Grant the CLONE_ADMIN privilege to the recovery account
   *     at the recipient
   * 6.  Create the SQL clone command based on the above
   * 7.  Execute the clone command
   */

  // Install the clone plugin on the recipient and source
  log_info("Installing the clone plugin on source '%s'%s.",
           donor.get()->get_canonical_address().c_str(),
           dry_run ? " (dryRun)" : "");

  if (!dry_run) {
    mysqlshdk::mysql::install_clone_plugin(*donor, nullptr);
  }

  log_info("Installing the clone plugin on recipient '%s'%s.",
           recipient->get_canonical_address().c_str(),
           dry_run ? " (dryRun)" : "");

  if (!dry_run) {
    mysqlshdk::mysql::install_clone_plugin(*recipient, nullptr);
  }

  std::string source_address = donor->get_canonical_address();

  // Set the donor to the selected donor on the recipient
  log_debug("Setting clone_valid_donor_list to '%s', on recipient '%s'%s",
            source_address.c_str(), recipient->get_canonical_address().c_str(),
            dry_run ? " (dryRun)" : "");

  // Create or update a recovery account with the required privileges for
  // read-replica management + clone usage (BACKUP_ADMIN) on the recipient

  if (!dry_run) {
    recipient->set_sysvar("clone_valid_donor_list", source_address);

    // Check if super_read_only is enabled. If so it must be disabled to create
    // the account
    if (recipient->get_sysvar_bool("super_read_only", false)) {
      recipient->set_sysvar("super_read_only", false);
    }
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
  create_clone_recovery_user_nobinlog(
      recipient.get(), *ar_options.repl_credentials, repl_account_host,
      cert_issuer, cert_subject, dry_run);

  if (!dry_run) {
    // Ensure the donor's recovery account has the clone usage required
    // privileges: BACKUP_ADMIN
    get_primary_master()->executef("GRANT BACKUP_ADMIN ON *.* TO ?@?",
                                   ar_options.repl_credentials->user,
                                   repl_account_host);

    // If the donor instance is processing transactions, it may have the
    // clone-user handling (create + grant) still in the backlog waiting to be
    // applied. For that reason, we must wait for it to be in sync with the
    // primary before starting the clone itself
    std::string primary_address = get_primary_master()->get_canonical_address();

    if (!mysqlshdk::utils::are_endpoints_equal(primary_address,
                                               source_address)) {
      console->print_info(
          "* Waiting for the donor to synchronize with PRIMARY...");

      try {
        // Sync the donor with the primary
        sync_transactions(*donor,
                          get_type() == Cluster_type::ASYNC_REPLICATION
                              ? ""
                              : mysqlshdk::gr::k_gr_applier_channel,
                          sync_timeout);
      } catch (const shcore::Exception &e) {
        if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
          console->print_error(
              "The donor instance failed to synchronize its transaction set "
              "with the PRIMARY.");
        }
        throw;
      } catch (const cancel_sync &) {
        // Throw it up
        throw;
      }
      console->print_info();
    }

    // Create a new connection to the recipient and run clone asynchronously
    std::string instance_def =
        recipient->get_connection_options().uri_endpoint();
    const auto recipient_clone =
        Scoped_instance(connect_target_instance(instance_def));

    mysqlshdk::db::Connection_options clone_donor_opts(source_address);

    // we need a point in time as close as possible, but still earlier than
    // when recovery starts to monitor the recovery phase. The timestamp
    // resolution is timestamp(3) irrespective of platform
    std::string begin_time =
        recipient->queryf_one_string(0, "", "SELECT NOW(3)");

    std::exception_ptr error_ptr;

    auto clone_thread = spawn_scoped_thread([&recipient_clone, clone_donor_opts,
                                             ar_options, &error_ptr] {
      mysqlsh::thread_init();

      bool enabled_ssl{false};
      switch (ar_options.auth_type) {
        case Replication_auth_type::CERT_ISSUER:
        case Replication_auth_type::CERT_SUBJECT:
        case Replication_auth_type::CERT_ISSUER_PASSWORD:
        case Replication_auth_type::CERT_SUBJECT_PASSWORD:
          enabled_ssl = true;
          break;
        default:
          break;
      }

      try {
        mysqlshdk::mysql::do_clone(recipient_clone, clone_donor_opts,
                                   *ar_options.repl_credentials, enabled_ssl);
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

    shcore::Scoped_callback join([&clone_thread]() {
      if (clone_thread.joinable()) {
        clone_thread.join();
      }
    });

    try {
      auto post_clone_auth = recipient->get_connection_options();
      post_clone_auth.set_login_options_from(donor->get_connection_options());

      monitor_standalone_clone_instance(
          recipient->get_connection_options(), post_clone_auth, begin_time,
          progress_style, k_clone_start_timeout.count(),
          current_shell_options()->get().dba_restart_wait_timeout);

      // When clone is used, the target instance will restart and all
      // connections are closed so we need to test if the connection to the
      // target instance and MD are closed and re-open if necessary
      recipient->reconnect_if_needed("Target");
      m_metadata_storage->get_md_server()->reconnect_if_needed("Metadata");

      // Remove the BACKUP_ADMIN grant from the recovery account
      get_primary_master()->executef("REVOKE BACKUP_ADMIN ON *.* FROM ?@?",
                                     ar_options.repl_credentials->user,
                                     repl_account_host);
    } catch (const stop_monitoring &) {
      console->print_info();
      console->print_note("Recovery process canceled. Reverting changes...");

      // Cancel the clone query
      mysqlshdk::mysql::cancel_clone(*recipient);

      log_info("Clone canceled.");

      log_debug("Waiting for clone thread...");
      // wait for the clone thread to finish
      clone_thread.join();
      log_debug("Clone thread joined");

      // When clone is canceled, the target instance will restart and all
      // connections are closed so we need to wait for the server to start-up
      // and re-establish the session. Also we need to test if the connection
      // to the target instance and MD are closed and re-open if necessary
      *recipient = *wait_server_startup(
          recipient->get_connection_options(),
          mysqlshdk::mysql::k_server_recovery_restart_timeout,
          Recovery_progress_style::NONE);

      recipient->reconnect_if_needed("Target");
      m_metadata_storage->get_md_server()->reconnect_if_needed("Metadata");

      // Remove the BACKUP_ADMIN grant from the recovery account
      get_primary_master()->executef("REVOKE BACKUP_ADMIN ON *.* FROM ?@?",
                                     ar_options.repl_credentials->user,
                                     repl_account_host);
      cleanup_clone_recovery(recipient.get(), *ar_options.repl_credentials,
                             repl_account_host);

      // Thrown the exception cancel_sync up
      throw cancel_sync();
    } catch (const restart_timeout &) {
      console->print_warning(
          "Clone process appears to have finished and tried to restart the "
          "MySQL server, but it has not yet started back up.");

      console->print_info();
      console->print_info(
          "Please make sure the MySQL server at '" + recipient->descr() +
          "' is properly restarted. The operation will be reverted, but you may"
          " retry adding the instance after restarting it. ");

      throw shcore::Exception("Timeout waiting for server to restart",
                              SHERR_DBA_SERVER_RESTART_TIMEOUT);
    } catch (const shcore::Error &e) {
      throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
    }
  }
}

void Base_cluster_impl::check_replication_version_compatibility(
    const std::string &source_descr, const std::string &replica_descr,
    const mysqlshdk::utils::Version &source_version,
    const mysqlshdk::utils::Version &replica_version, bool is_potential,
    bool log_only, shcore::Array_t *instance_errors) {
  const char *potential_str = is_potential ? "potential " : "";

  enum class Message_type { ERROR, WARNING, INFO };

  auto format_message = [&](Message_type type, const std::string &msg) {
    // If instance_errors, add the info to it and return right away
    if (instance_errors) {
      switch (type) {
        case Message_type::ERROR:
          (*instance_errors)->push_back(shcore::Value("ERROR: " + msg));
          break;
        case Message_type::WARNING:
          (*instance_errors)->push_back(shcore::Value("WARNING: " + msg));
          break;
        case Message_type::INFO:
          log_debug("%s", msg.c_str());
          break;
      }
      return;
    }

    switch (type) {
      case Message_type::ERROR: {
        if (log_only) {
          log_info(
              "%s NOTE: Check ignored due to 'dba.versionCompatibilityChecks' "
              "being disabled.",
              msg.c_str());
        } else {
          mysqlsh::current_console()->print_error(msg);
          throw shcore::Exception("Incompatible replication source version",
                                  SHERR_DBA_REPLICATION_INCOMPATIBLE_SOURCE);
        }
        break;
      }
      case Message_type::WARNING: {
        if (log_only) {
          log_info(
              "%s NOTE: Check ignored due to 'dba.versionCompatibilityChecks' "
              "being disabled.",
              msg.c_str());
        } else {
          mysqlsh::current_console()->print_warning(msg);
          mysqlsh::current_console()->print_info();
        }
        break;
      }
      case Message_type::INFO: {
        log_debug("%s", msg.c_str());
        break;
      }
    }
  };

  auto compatibility = mysqlshdk::mysql::verify_compatible_replication_versions(
      source_version, replica_version);

  switch (compatibility) {
    case mysqlshdk::mysql::Replication_version_compatibility::INCOMPATIBLE: {
      std::string error_message = shcore::str_format(
          "The %sreplication source ('%s') is running version %s, which is "
          "incompatible with the target replica's ('%s') version %s. If you "
          "understand the limitations and potential issues, use the "
          "'dba.versionCompatibilityChecks' option to bypass this check.",
          potential_str, source_descr.c_str(),
          source_version.get_full().c_str(), replica_descr.c_str(),
          replica_version.get_full().c_str());
      format_message(Message_type::ERROR, error_message);
      break;
    }
    case mysqlshdk::mysql::Replication_version_compatibility::DOWNGRADE_ONLY: {
      std::string warning_message = shcore::str_format(
          "The %sreplication source ('%s') is running version %s, which "
          "has limited compatibility with the target instance's ('%s') "
          "version %s. This setup should only be used for rollback purposes, "
          "where new functionality from version %s is not yet utilized. It "
          "is not suitable for regular continuous production deployment.",
          potential_str, source_descr.c_str(),
          source_version.get_full().c_str(), replica_descr.c_str(),
          replica_version.get_full().c_str(),
          source_version.get_full().c_str());
      format_message(Message_type::WARNING, warning_message);
      break;
    }
    case mysqlshdk::mysql::Replication_version_compatibility::COMPATIBLE: {
      std::string info_message = shcore::str_format(
          "The %sreplication source's ('%s') version (%s) is compatible "
          "with the replica instance's ('%s') version (%s).",
          potential_str, source_descr.c_str(),
          source_version.get_full().c_str(), replica_descr.c_str(),
          replica_version.get_full().c_str());
      format_message(Message_type::INFO, info_message);
      break;
    }
  }
}

void Base_cluster_impl::check_compatible_replication_sources(
    const mysqlshdk::mysql::IInstance &source,
    const mysqlshdk::mysql::IInstance &replica,
    const std::list<std::shared_ptr<mysqlshdk::mysql::IInstance>>
        *potential_sources,
    bool potential) {
  using mysqlshdk::mysql::Replication_version_compatibility;

  // If dba.versionCompatibilityChecks is disabled, all checks are done but only
  // logged.
  bool log_only =
      !current_shell_options()->get().dba_version_compatibility_checks;

  auto check = [&](const mysqlshdk::mysql::IInstance &src, bool is_potential) {
    auto source_version = src.get_version();
    auto replica_version = replica.get_version();

    check_replication_version_compatibility(src.descr(), replica.descr(),
                                            source_version, replica_version,
                                            is_potential, log_only, nullptr);
  };

  if (potential_sources && !potential_sources->empty()) {
    for (const auto &potential_source : *potential_sources) {
      check(*potential_source,
            potential_source->get_uuid() != source.get_uuid());
    }
  } else {
    check(source, potential);
  }
}

void Base_cluster_impl::add_incompatible_replication_source_issues(
    const mysqlshdk::mysql::IInstance &source,
    const mysqlshdk::mysql::IInstance &replica, bool potential,
    shcore::Array_t &instance_errors) {
  auto src_endpoint = source.descr();
  auto repl_endpoint = replica.descr();
  auto src_version = source.get_version();
  auto rpl_version = replica.get_version();

  check_replication_version_compatibility(source.descr(), replica.descr(),
                                          src_version, rpl_version, potential,
                                          false, &instance_errors);
}

void Base_cluster_impl::check_compatible_clone_donor(
    const mysqlshdk::mysql::IInstance &donor,
    const mysqlshdk::mysql::IInstance &recipient) {
  /*
   * A donor is compatible if:
   *
   *   - Supports clone
   *   - The versions of the donor and recipient are compatible
   *   - It has the same operating system as the recipient
   */

  // Check if the instance supports clone
  auto donor_version = donor.get_version();
  if (!supports_mysql_clone(donor_version)) {
    throw shcore::Exception(
        shcore::str_format("Instance '%s' does not support MySQL Clone.",
                           donor.get_canonical_address().c_str()),
        SHERR_DBA_CLONE_NO_SUPPORT);
  }

  // Check version compatibility of donor / recipient
  {
    DBUG_EXECUTE_IF("dba_clone_version_check_fail",
                    { donor_version = mysqlshdk::utils::Version(8, 0, 17); });

    auto recipient_version = recipient.get_version();

    if (!mysqlshdk::mysql::verify_compatible_clone_versions(
            donor_version, recipient_version)) {
      throw shcore::Exception(
          shcore::str_format(
              "Instance '%s' cannot be a donor because its "
              "version (%s) isn't compatible with the recipient's (%s).",
              donor.get_canonical_address().c_str(),
              donor_version.get_full().c_str(),
              recipient_version.get_full().c_str()),
          SHERR_DBA_CLONE_DIFF_VERSION);
    }
  }

  // Check if the instance has the same OS the recipient
  {
    auto donor_version_compile_os = donor.get_version_compile_os();
    auto recipient_version_compile_os = recipient.get_version_compile_os();

    if (donor_version_compile_os != recipient_version_compile_os) {
      throw shcore::Exception(
          shcore::str_format(
              "Instance '%s' cannot be a donor because it has a different "
              "Operating System (%s) than the recipient (%s).",
              donor.get_canonical_address().c_str(),
              donor_version_compile_os.c_str(),
              recipient_version_compile_os.c_str()),
          SHERR_DBA_CLONE_DIFF_OS);
    }
  }

  // Check if the instance is running on the same platform of the recipient
  {
    auto donor_version_compile_machine = donor.get_version_compile_machine();
    auto recipient_version_compile_machine =
        recipient.get_version_compile_machine();

    if (donor_version_compile_machine != recipient_version_compile_machine) {
      throw shcore::Exception(
          shcore::str_format(
              "Instance '%s' cannot be a donor because it is running on a "
              "different platform (%s) than the recipient (%s).",
              donor.get_canonical_address().c_str(),
              donor_version_compile_machine.c_str(),
              recipient_version_compile_machine.c_str()),
          SHERR_DBA_CLONE_DIFF_PLATFORM);
    }
  }
}

Clone_availability Base_cluster_impl::check_clone_availablity(
    const mysqlshdk::mysql::IInstance &donor_instance,
    const mysqlshdk::mysql::IInstance &target_instance) const {
  try {
    check_compatible_clone_donor(donor_instance, target_instance);
  } catch (shcore::Exception &e) {
    current_console()->print_info();
    current_console()->print_warning(
        shcore::str_format("Clone-based recovery not available: %s", e.what()));

    return Clone_availability::Unavailable;
  }

  return Clone_availability::Available;
}

void Base_cluster_impl::set_target_server(
    const std::shared_ptr<Instance> &instance) {
  disconnect();

  m_cluster_server = instance;
  m_cluster_server->retain();

  m_metadata_storage = std::make_shared<MetadataStorage>(m_cluster_server);
}

std::shared_ptr<Instance> Base_cluster_impl::connect_target_instance(
    const std::string &instance_def, bool print_error,
    bool allow_account_override) {
  return connect_target_instance(
      mysqlshdk::db::Connection_options(instance_def), print_error,
      allow_account_override);
}

std::shared_ptr<Instance> Base_cluster_impl::connect_target_instance(
    const mysqlshdk::db::Connection_options &instance_def, bool print_error,
    bool allow_account_override) {
  assert(m_cluster_server);

  // Once an instance is part of the cluster, it must accept the same
  // credentials used in the cluster object. But while it's being added, it
  // can either accept the cluster credentials or have its own, with the
  // assumption that once it joins, the common credentials will get replicated
  // to it and all future operations will start using that account. So, the
  // following scenarios are possible:
  // * host:port  user/pwd taken from cluster session and must exist at target
  // * icuser@host:port  pwd taken from cluster session and must exist at
  // target
  // * icuser:pwd@host:port  pwd MUST match the session one
  // * user@host:port  pwd prompted, no extra checks
  // * user:pwd@host:port  no extra checks
  auto ipool = current_ipool();

  const auto &main_opts = m_cluster_server->get_connection_options();

  // default to copying credentials and all other connection params from the
  // main session
  mysqlshdk::db::Connection_options connect_opts(main_opts);

  // overwrite address related options
  connect_opts.clear_host();
  connect_opts.clear_port();
  connect_opts.clear_socket();
  connect_opts.set_scheme(main_opts.get_scheme());
  if (instance_def.has_socket()) {
    connect_opts.set_socket(instance_def.get_socket());
    connect_opts.set_host("localhost");
  } else {
    if (instance_def.has_host()) connect_opts.set_host(instance_def.get_host());
    if (instance_def.has_port()) connect_opts.set_port(instance_def.get_port());
  }
  connect_opts.clear_transport_type();
  if (instance_def.has_transport_type())
    connect_opts.set_transport_type(instance_def.get_transport_type());

  // is user has specified the connect-timeout connection option, use that
  // value explicitly
  if (instance_def.has_value(mysqlshdk::db::kConnectTimeout)) {
    connect_opts.set(mysqlshdk::db::kConnectTimeout,
                     instance_def.get_connect_timeout());
  }

  if (allow_account_override) {
    if (instance_def.has_user()) {
      if (instance_def.get_user() != connect_opts.get_user()) {
        // override all credentials with what was given
        connect_opts.set_login_options_from(instance_def);
      } else {
        // if username matches, then password must also be the same
        if (instance_def.has_password() &&
            (!connect_opts.has_password() ||
             instance_def.get_password() != connect_opts.get_password())) {
          current_console()->print_error(
              "Password for user " + instance_def.get_user() + " at " +
              instance_def.uri_endpoint() +
              " must be the same as in the rest of the cluster.");
          throw std::invalid_argument("Invalid target instance specification");
        }
      }
    }
  } else {
    // if override is not allowed, then any credentials given must match the
    // cluster session
    if (instance_def.has_user()) {
      bool mismatch = false;

      if (instance_def.get_user() != connect_opts.get_user()) mismatch = true;

      if (instance_def.has_password() &&
          (!connect_opts.has_password() ||
           instance_def.get_password() != connect_opts.get_password()))
        mismatch = true;

      if (mismatch) {
        current_console()->print_error(
            "Target instance must be given as host:port. Credentials will be "
            "taken from the main session and, if given, must match them (" +
            instance_def.uri_endpoint() + ")");
        throw std::invalid_argument("Invalid target instance specification");
      }
    }
  }

  if (instance_def.has_scheme() &&
      instance_def.get_scheme() != main_opts.get_scheme()) {
    // different scheme means it's an X protocol URI
    const auto error = make_unsupported_protocol_error();
    const auto endpoint = Connection_options(instance_def).uri_endpoint();
    detail::report_connection_error(error, endpoint);
    throw shcore::Exception::runtime_error(
        detail::connection_error_msg(error, endpoint));
  }

  try {
    try {
      return ipool->connect_unchecked(connect_opts);
    } catch (const shcore::Error &err) {
      if (err.code() == ER_ACCESS_DENIED_ERROR) {
        if (!allow_account_override) {
          mysqlsh::current_console()->print_error(
              "The administrative account credentials for " +
              Connection_options(instance_def).uri_endpoint() +
              " do not match the cluster's administrative account. The "
              "cluster administrative account user name and password must be "
              "the same on all instances that belong to it.");
          print_error = false;
        } else {
          // if user overrode the account, then just bubble up the error
          if (connect_opts.get_user() != main_opts.get_user()) {
            // no-op
          } else {
            mysqlsh::current_console()->print_error(
                err.format() + ": Credentials for user " +
                connect_opts.get_user() + " at " + connect_opts.uri_endpoint() +
                " must be the same as in the rest of the cluster.");
            print_error = false;
          }
        }
      }
      throw;
    }
  }
  CATCH_REPORT_AND_THROW_CONNECTION_ERROR_PRINT(
      Connection_options(instance_def).uri_endpoint(), print_error);
}

shcore::Value Base_cluster_impl::list_routers(bool only_upgrade_required) {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["routers"] = router_list(get_metadata_storage().get(), get_id(),
                                   only_upgrade_required);

  return shcore::Value(std::move(dict));
}

/**
 * Helper method that has common code for setup_router_account and
 * setup_admin_account methods
 * @param user the user part of the account
 * @param host The host part of the account
 * @param interactive the value of the interactive flag
 * @param update the value of the update flag
 * @param requireSubject certificate subject to require
 * @param requireIssuer certificate issuer to require
 * @param dry_run the value of the dry_run flag
 * @param password the password for the account
 * @param type The type of account to create, Admin or Router
 */
void Base_cluster_impl::setup_account_common(
    const std::string &username, const std::string &host,
    const Setup_account_options &options, const Setup_account_type &type) {
  // NOTE: GR by design guarantees that the primary instance is always the one
  // with the lowest instance version. A similar (although not explicit)
  // guarantee exists on Semi-sync replication, replication from newer master
  // to older slaves might not be possible but is generally not supported.
  // See: https://dev.mysql.com/doc/refman/en/replication-compatibility.html
  //
  // By using the primary instance to validate
  // the list of privileges / build the list of grants to be given to the
  // new/existing user we are sure that privileges that appeared in recent
  // versions (like REPLICATION_APPLIER) are only checked/granted in case all
  // cluster members support it. This ensures that there is no issue when the
  // DDL statements are replicated throughout the cluster since they won't
  // contain unsupported grants.

  // The pool is initialized with the metadata using the current session
  MetadataStorage metadata(get_cluster_server());

  const auto primary_instance = get_primary_master();
  shcore::on_leave_scope finally_primary([this]() { release_primary(); });

  // get the metadata version to build an accurate list of grants
  mysqlshdk::utils::Version metadata_version;
  if (!metadata.check_version(&metadata_version)) {
    throw std::logic_error("Internal error, metadata not found.");
  }

  {
    std::vector<std::string> grant_list;
    // get list of grants
    switch (type) {
      case Setup_account_type::ROUTER:
        grant_list = create_router_grants(shcore::make_account(username, host),
                                          metadata_version);
        break;
      case Setup_account_type::ADMIN:
        grant_list = create_admin_grants(shcore::make_account(username, host),
                                         primary_instance->get_version());
        break;
    }

    Setup_account op_setup(username, host, options, grant_list,
                           *primary_instance, get_type());
    // Always execute finish when leaving "try catch".
    shcore::on_leave_scope finally([&op_setup]() { op_setup.finish(); });

    // Prepare the setup_account execution
    op_setup.prepare();
    // Execute the setup_account command.
    op_setup.execute();
  }
}

void Base_cluster_impl::setup_admin_account(
    const std::string &username, const std::string &host,
    const Setup_account_options &options) {
  setup_account_common(username, host, options, Setup_account_type::ADMIN);
}

void Base_cluster_impl::setup_router_account(
    const std::string &username, const std::string &host,
    const Setup_account_options &options) {
  setup_account_common(username, host, options, Setup_account_type::ROUTER);
}

void Base_cluster_impl::remove_router_metadata(const std::string &router) {
  {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds = Command_conditions::Builder::gen_cluster("removeRouterMetadata")
                    .target_instance(TargetType::InnoDBCluster,
                                     TargetType::InnoDBClusterSet)
                    .primary_required()
                    .cluster_global_status(Cluster_global_status::OK)
                    .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds =
            Command_conditions::Builder::gen_replicaset("removeRouterMetadata")
                .primary_required()
                .build();
        break;
      default:
        throw std::logic_error(
            "Command context is incorrect for \"removeRouterMetadata\".");
    }

    check_preconditions(conds);
  }

  auto c_lock = get_lock_shared();

  MetadataStorage::Transaction trx(get_metadata_storage());

  if (!get_metadata_storage()->remove_router(router)) {
    throw shcore::Exception::argument_error("Invalid router instance '" +
                                            router + "'");
  }

  trx.commit();
}

void Base_cluster_impl::set_instance_tag(const std::string &instance_def,
                                         const std::string &option,
                                         const shcore::Value &value) {
  // Connect to the target Instance and check if it belongs to the
  // cluster/replicaSet
  const auto target_instance =
      Scoped_instance(connect_target_instance(instance_def));
  const auto target_uuid = target_instance->get_uuid();
  const auto is_instance_on_md = get_metadata_storage()->is_instance_on_cluster(
      get_id(), target_instance->get_canonical_address());

  if (!is_instance_on_md) {
    std::string err_msg =
        "The instance '" + Connection_options(instance_def).uri_endpoint() +
        "' does not belong to the " + api_class(get_type()) + ".";
    throw shcore::Exception::runtime_error(err_msg);
  }

  get_metadata_storage()->set_instance_tag(target_uuid, option, value);
}

void Base_cluster_impl::set_cluster_tag(const std::string &option,
                                        const shcore::Value &value) {
  get_metadata_storage()->set_cluster_tag(get_id(), option, value);
}

void Base_cluster_impl::set_router_tag(const std::string &router,
                                       const std::string &option,
                                       const shcore::Value &value) {
  get_metadata_storage()->set_router_tag(get_type(), get_id(), router, option,
                                         value);
}

void Base_cluster_impl::set_instance_option(const std::string &instance_def,
                                            const std::string &option,
                                            const shcore::Value &value) {
  {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds = Command_conditions::Builder::gen_cluster("setInstanceOption")
                    .target_instance(TargetType::InnoDBCluster,
                                     TargetType::InnoDBClusterSet)
                    .quorum_state(ReplicationQuorum::States::Normal)
                    .primary_required()
                    .cluster_global_status_any_ok()
                    .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds = Command_conditions::Builder::gen_replicaset("setInstanceOption")
                    .primary_required()
                    .build();
        break;
      default:
        throw std::logic_error(
            "Command context is incorrect for \"setInstanceOption\".");
    }

    check_preconditions(conds);
  }

  std::string opt_namespace, opt;
  shcore::Value val;
  std::tie(opt_namespace, opt, val) =
      validate_set_option_namespace(option, value, k_supported_set_option_tags);
  if (opt_namespace.empty()) {
    // no namespace was provided
    _set_instance_option(instance_def, option, val);
  } else {
    set_instance_tag(instance_def, opt, val);
  }
}

void Base_cluster_impl::set_option(const std::string &option,
                                   const shcore::Value &value) {
  std::string opt_namespace, opt;
  shcore::Value val;
  std::tie(opt_namespace, opt, val) =
      validate_set_option_namespace(option, value, k_supported_set_option_tags);

  if (!opt_namespace.empty() && get_type() == Cluster_type::GROUP_REPLICATION) {
    auto conds = Command_conditions::Builder::gen_cluster("setOption")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .quorum_state(ReplicationQuorum::States::Normal)
                     .primary_required()
                     .cluster_global_status_any_ok()
                     .build();

    check_preconditions(conds);

  } else if (opt_namespace.empty() &&
             shcore::str_beginswith(opt, "clusterSetReplication") &&
             ((get_type() == Cluster_type::GROUP_REPLICATION) ||
              (get_type() == Cluster_type::REPLICATED_CLUSTER))) {
    auto conds = Command_conditions::Builder::gen_cluster("setOption")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet,
                                      TargetType::InnoDBClusterSetOffline)
                     .primary_required()
                     .cluster_global_status_any_ok()
                     .cluster_global_status_add_not_ok()
                     .build();

    check_preconditions(conds);
  } else {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds = Command_conditions::Builder::gen_cluster("setOption")
                    .target_instance(TargetType::InnoDBCluster,
                                     TargetType::InnoDBClusterSet)
                    .quorum_state(ReplicationQuorum::States::All_online)
                    .primary_required()
                    .cluster_global_status_any_ok()
                    .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds = Command_conditions::Builder::gen_replicaset("setOption")
                    .primary_required()
                    .build();
        break;
      case Cluster_type::REPLICATED_CLUSTER:
        conds = Command_conditions::Builder::gen_clusterset("setOption")
                    .quorum_state(ReplicationQuorum::States::All_online)
                    .primary_required()
                    .cluster_global_status_any_ok()
                    .build();
        break;
      default:
        throw std::logic_error(
            "Command context is incorrect for \"setOption\".");
    }

    check_preconditions(conds);
  }

  // make sure metadata session is using the primary
  acquire_primary();

  shcore::on_leave_scope finally_primary([this]() { release_primary(); });

  if (opt_namespace.empty()) {
    // no namespace was provided
    _set_option(option, val);
  } else {
    set_cluster_tag(opt, val);
  }
}

Replication_auth_type Base_cluster_impl::query_cluster_auth_type() const {
  if (shcore::Value value; get_metadata_storage()->query_cluster_attribute(
          get_id(), k_cluster_attribute_member_auth_type, &value)) {
    return to_replication_auth_type(value.as_string());
  }

  return Replication_auth_type::PASSWORD;
}

std::string Base_cluster_impl::query_cluster_auth_cert_issuer() const {
  if (shcore::Value value;
      get_metadata_storage()->query_cluster_attribute(
          get_id(), k_cluster_attribute_cert_issuer, &value) &&
      (value.get_type() == shcore::String))
    return value.as_string();

  return {};
}

std::string Base_cluster_impl::query_cluster_instance_auth_cert_subject(
    std::string_view instance_uuid) const {
  if (shcore::Value value;
      get_metadata_storage()->query_instance_attribute(
          instance_uuid, k_instance_attribute_cert_subject, &value) &&
      (value.get_type() == shcore::String))
    return value.as_string();

  return {};
}

std::tuple<std::string, std::string, shcore::Value>
Base_cluster_impl::validate_set_option_namespace(
    const std::string &option, const shcore::Value &value,
    const built_in_tags_map_t &built_in_tags) const {
  // Check if we're using namespaces
  auto tokens = shcore::str_split(option, ":", 1);
  if (tokens.size() != 2)
    return std::make_tuple("", option, value);  // not using namespaces

  // colon cannot be first char or last char, we don't support empty
  // namespaces nor empty namespace options
  const std::string &opt_namespace = tokens[0];
  const std::string &opt = tokens[1];
  if (opt_namespace.empty() || opt.empty()) {
    throw shcore::Exception::argument_error("'" + option +
                                            "' is not a valid identifier.");
  }

  // option is of type namespace:option
  // For now since we don't allow namespaces other than 'tag', throw an
  // error if the namespace is not a tag
  if (opt_namespace != "tag") {
    throw shcore::Exception::argument_error("Namespace '" + opt_namespace +
                                            "' not supported.");
  }

  // tag namespace.
  if (!shcore::is_valid_identifier(opt)) {
    throw shcore::Exception::argument_error("'" + opt +
                                            "' is not a valid tag identifier.");
  }

  // Even if it is a valid identifier, if it starts with _ we need to make
  // sure it is one of the allowed built-in tags
  shcore::Value val = value;
  if (opt[0] == '_') {
    built_in_tags_map_t::const_iterator c_it = built_in_tags.find(opt);
    if (c_it == built_in_tags.cend()) {
      throw shcore::Exception::argument_error("'" + opt +
                                              "' is not a valid built-in tag.");
    }
    // If the type of the value is not Null, check that it can be
    // converted to the expected type
    if (value.get_type() != shcore::Value_type::Null) {
      try {
        switch (c_it->second) {
          case shcore::Value_type::Bool:
            val = shcore::Value(value.as_bool());
            break;
          default:
            // so far, built-in tags are only expected to be booleans.
            // Any other type should throw an exception. This exception
            // will be caught on the catch clause, and re-created with a
            // more informative message.
            throw shcore::Exception::type_error(
                "Unsupported built-in tag type.");
        }
      } catch (const shcore::Exception &) {
        throw shcore::Exception::type_error(shcore::str_format(
            "Built-in tag '%s' is expected to be of type %s, but is %s",
            c_it->first.c_str(), type_name(c_it->second).c_str(),
            type_name(value.get_type()).c_str()));
      }
    }
  }

  return std::make_tuple(opt_namespace, opt, std::move(val));
}

shcore::Value Base_cluster_impl::get_cluster_tags() const {
  shcore::Dictionary_t res = shcore::make_dict();

  assert(get_type() != Cluster_type::REPLICATED_CLUSTER);

  auto map_to_array = [](const shcore::Dictionary_t &tags) -> shcore::Array_t {
    shcore::Array_t result = shcore::make_array();
    if (tags) {
      for (const auto &tag : *tags) {
        result->emplace_back(shcore::Value(shcore::make_dict(
            "option", shcore::Value(tag.first), "value", tag.second)));
      }
    }
    return result;
  };

  Cluster_metadata cluster_md;

  get_metadata_storage()->get_cluster(get_id(), &cluster_md);

  // Fill cluster tags
  (*res)["global"] = shcore::Value(map_to_array(cluster_md.tags));

  // Fill the cluster instance tags
  std::vector<Instance_metadata> instance_defs =
      get_metadata_storage()->get_all_instances(get_id(), true);

  // get the list of tags each instance
  for (const auto &instance_def : instance_defs) {
    (*res)[instance_def.label] = shcore::Value(map_to_array(instance_def.tags));
  }
  return shcore::Value(res);
}

void Base_cluster_impl::disconnect_internal() {
  if (m_cluster_server) {
    m_cluster_server->release();
    m_cluster_server.reset();
  }

  if (m_primary_master) {
    m_primary_master->release();
    m_primary_master.reset();
  }

  if (m_metadata_storage) {
    m_metadata_storage.reset();
  }
}

bool Base_cluster_impl::is_cluster_clusterset_member() const {
  if (Cluster_impl *cluster =
          dynamic_cast<Cluster_impl *>(const_cast<Base_cluster_impl *>(this))) {
    return cluster->is_cluster_set_member();
  }

  return false;
}

void Base_cluster_impl::set_routing_option(const std::string &router,
                                           const std::string &option,
                                           const shcore::Value &value) {
  {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds = Command_conditions::Builder::gen_cluster("setRoutingOption")
                    .target_instance(TargetType::InnoDBCluster,
                                     TargetType::InnoDBClusterSet)
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
                    .primary_required()
                    .cluster_global_status_any_ok()
                    .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds = Command_conditions::Builder::gen_replicaset("setRoutingOption")
                    .primary_required()
                    .build();
        break;
      case Cluster_type::REPLICATED_CLUSTER:
        conds = Command_conditions::Builder::gen_clusterset("setRoutingOption")
                    .primary_required()
                    .cluster_global_status_any_ok()
                    .build();
        break;
      default:
        throw std::logic_error(
            "Command context is incorrect for \"setRoutingOption\".");
    }

    check_preconditions(conds);
  }

  std::string opt_namespace, opt;
  shcore::Value val;
  std::tie(opt_namespace, opt, val) =
      validate_set_option_namespace(option, value, {});

  if (!opt_namespace.empty()) {
    set_router_tag(router, opt, val);
    return;
  }

  auto value_fixed = validate_router_option(*this, opt, val);

  // Handling for routing guidelines
  bool global_routing_guideline =
      opt == k_router_option_routing_guideline && router.empty() &&
      value_fixed.get_type() == shcore::Value_type::String;

  if (value_fixed.get_type() != shcore::Value_type::Null) {
    if (global_routing_guideline) {
      // Check if the guideline is already active
      if (get_active_routing_guideline() == value.get_string()) {
        mysqlsh::current_console()->print_info(shcore::str_format(
            "Routing Guideline '%s' is already active in the topology.",
            value.get_string().c_str()));
        return;
      }
    }

    // Check if all routers support the guideline
    if (opt == k_router_option_routing_guideline) {
      Routing_guideline_metadata rg =
          get_metadata_storage()->get_routing_guideline(get_type(), get_id(),
                                                        value.get_string());

      mysqlshdk::utils::Version guideline_version;
      auto guideline = shcore::Value::parse(rg.guideline).as_map();

      if (guideline->has_key("version")) {
        try {
          guideline_version =
              mysqlshdk::utils::Version(guideline->get_string("version"));
        } catch (...) {
          throw shcore::Exception(
              shcore::str_format("Invalid routing guideline version '%s'",
                                 guideline->at("version").descr().c_str()),
              SHERR_DBA_ROUTING_GUIDELINE_INVALID_VERSION);
        }
      }

      auto throw_error = [&](const std::vector<std::string> &routers) {
        std::string router_list = shcore::str_join(routers, ", ");
        throw shcore::Exception(
            shcore::str_format(
                "The following Routers in the %s do not support the "
                "Routing Guideline and must be upgraded: %s",
                to_display_string(get_type(), Display_form::THING).c_str(),
                router_list.c_str()),
            SHERR_DBA_ROUTER_UNSUPPORTED_FEATURE);
      };

      std::vector<std::string> unsupported_routers;

      for (const auto &router_md :
           get_metadata_storage()->get_routers(get_id())) {
        std::string router_label = shcore::str_format(
            "%s::%s", router_md.hostname.c_str(), router_md.name.c_str());

        // If not setting globally, we just need to check the target router
        if (!global_routing_guideline && router_label != router) continue;

        // Check if the router's supported guidelines version is available
        if (!router_md.supported_routing_guidelines_version.has_value()) {
          unsupported_routers.push_back(router_label);
          continue;
        }

        // Check if the guideline is supported by the router:
        //  - Guideline_version < Router_supported_version
        //  - Router_supported_version.major - Guideline_version.major <= 1
        auto router_version = mysqlshdk::utils::Version(
            *router_md.supported_routing_guidelines_version);
        if ((guideline_version > router_version) ||
            (router_version.get_major() - guideline_version.get_major() > 1)) {
          unsupported_routers.push_back(router_label);
        }
      }

      if (!unsupported_routers.empty()) {
        throw_error(unsupported_routers);
      }
    }
  }

  // If 'read_only_targets' or 'use_replica_primary_as_rw' are being changed and
  // there's an active routing guideline, warn users about it
  if (std::string active_rg = get_active_routing_guideline();
      !active_rg.empty() &&
      (opt == k_router_option_read_only_targets ||
       opt == k_router_option_use_replica_primary_as_rw)) {
    mysqlsh::current_console()->print_warning(shcore::str_format(
        "The '%s' setting has no effect because the "
        "Routing Guideline '%s' is currently active in the topology. Routing "
        "Guidelines take precedence over this configuration.",
        opt.c_str(), active_rg.c_str()));
  }

  auto msg = shcore::str_format("Routing option '%s' successfully updated",
                                opt.c_str());
  if (router.empty()) {
    get_metadata_storage()->set_global_routing_option(get_type(), get_id(), opt,
                                                      value_fixed);
    msg += ".";
  } else {
    get_metadata_storage()->set_routing_option(get_type(), router, get_id(),
                                               opt, value_fixed);
    msg += shcore::str_format(" in router '%s'.", router.c_str());
  }

  mysqlsh::current_console()->print_info(msg);

  if (global_routing_guideline) {
    mysqlsh::current_console()->print_info(
        shcore::str_format("Routing Guideline '%s' has been enabled and is now "
                           "the active guideline for the topology.",
                           val.get_string().c_str()));
  }
}

shcore::Dictionary_t Base_cluster_impl::router_options(
    const shcore::Option_pack_ref<Router_options_options> &options) {
  {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds = Command_conditions::Builder::gen_cluster("routerOptions")
                    .target_instance(TargetType::InnoDBCluster,
                                     TargetType::InnoDBClusterSet)
                    .primary_required()
                    .allowed_on_fence()
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_1_0)
                    .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds = Command_conditions::Builder::gen_replicaset("routerOptions")
                    .primary_required()
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_1_0)
                    .build();
        break;

      case Cluster_type::REPLICATED_CLUSTER:
        conds = Command_conditions::Builder::gen_clusterset("routerOptions")
                    .primary_not_required()
                    .allowed_on_fence()
                    .build();
        break;

      default:
        throw std::logic_error(shcore::str_format(
            "Command context is incorrect for \"routerOptions\"."));
    }

    check_preconditions(conds);
  }

  return mysqlsh::dba::router_options(get_metadata_storage().get(), get_type(),
                                      get_id(), get_name(), *options);
}

std::shared_ptr<Routing_guideline_impl>
Base_cluster_impl::create_routing_guideline(
    std::shared_ptr<Base_cluster_impl> self, const std::string &name,
    shcore::Dictionary_t json,
    const shcore::Option_pack_ref<Create_routing_guideline_options> &options) {
  {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds =
            Command_conditions::Builder::gen_cluster("createRoutingGuideline")
                .target_instance(TargetType::InnoDBCluster,
                                 TargetType::InnoDBClusterSet)
                .quorum_state(ReplicationQuorum::States::Normal)
                .compatibility_check(
                    metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                .primary_required()
                .cluster_global_status_any_ok()
                .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds = Command_conditions::Builder::gen_replicaset(
                    "createRoutingGuideline")
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_required()
                    .build();
        break;
      case Cluster_type::REPLICATED_CLUSTER:
        conds = Command_conditions::Builder::gen_clusterset(
                    "createRoutingGuideline")
                    .quorum_state(ReplicationQuorum::States::All_online)
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_required()
                    .cluster_global_status_any_ok()
                    .build();
        break;
      default:
        throw std::logic_error(
            "Command context is incorrect for \"createRoutingGuideline\".");
    }

    check_preconditions(conds);

    if (is_cluster_clusterset_member()) {
      current_console()->print_error(shcore::str_format(
          "Cluster '%s' is a member of a ClusterSet, use "
          "<ClusterSet>.<<<createRoutingGuideline>>>() instead.",
          get_name().c_str()));
      throw shcore::Exception::runtime_error(
          "Function not available for ClusterSet members");
    }
  }

  return Routing_guideline_impl::create(self, name, json, options);
}

std::string Base_cluster_impl::get_router_option(
    const std::string &option) const {
  auto md = get_metadata_storage();
  auto highest_router_version =
      md->get_highest_bootstrapped_router_version(get_type(), get_id());

  auto default_global_options = md->get_default_router_options(
      get_type(), get_id(), highest_router_version);

  auto default_global_options_parsed =
      Router_configuration_schema(default_global_options);

  std::string value;

  if (!default_global_options_parsed.get_option_value(option, value)) {
    return "";
  }

  return value;
}

std::string Base_cluster_impl::get_active_routing_guideline() const {
  return get_router_option(k_router_option_routing_guideline);
}

namespace {
Read_only_targets from_string(const std::string &str) {
  std::string lowered_str = shcore::str_lower(str);

  if (lowered_str == k_router_option_read_only_targets_secondaries) {
    return Read_only_targets::SECONDARIES;
  } else if (lowered_str == k_router_option_read_only_targets_read_replicas) {
    return Read_only_targets::READ_REPLICAS;
  } else if (lowered_str == k_router_option_read_only_targets_all) {
    return Read_only_targets::ALL;
  } else if (lowered_str.empty()) {
    return Read_only_targets::NONE;
  } else {
    throw std::invalid_argument("Invalid value for Read_only_targets: " + str);
  }
}
}  // namespace

Read_only_targets Base_cluster_impl::get_read_only_targets_option() const {
  std::string read_only_targets =
      get_router_option(k_router_option_read_only_targets);

  return from_string(read_only_targets);
}

bool Base_cluster_impl::has_guideline(const std::string &name) const {
  return get_metadata_storage()->check_routing_guideline_exists(get_type(),
                                                                get_id(), name);
}

const std::vector<Routing_guideline_metadata>
Base_cluster_impl::get_routing_guidelines() const {
  return get_metadata_storage()->get_routing_guidelines(get_type(), get_id());
}

void Base_cluster_impl::set_global_routing_option(const std::string &option,
                                                  const shcore::Value &value) {
  get_metadata_storage()->set_global_routing_option(get_type(), get_id(),
                                                    option, value);
}

void Base_cluster_impl::store_routing_guideline(
    const std::shared_ptr<Routing_guideline_impl> &guideline) {
  auto console = mysqlsh::current_console();
  bool first_rg_or_no_active = false;

  std::string active_rg = get_active_routing_guideline();

  if (get_routing_guidelines().empty() || active_rg.empty()) {
    first_rg_or_no_active = true;
  }

  // Save to the metadata
  guideline->save_guideline();

  // First guideline created or no active guidelines
  if (first_rg_or_no_active) {
    console->print_info();
    console->print_note(shcore::str_format(
        "Routing guideline '%s' won't be made active by default. To activate "
        "this guideline, please use .<<<setRoutingOption()>>> with the option "
        "'guideline'.",
        guideline->get_name().c_str()));
  } else if (!active_rg.empty()) {
    // Print a message indicating which Guideline is the active one, if any
    console->print_info();
    console->print_note(shcore::str_format(
        "Routing Guideline '%s' is the guideline currently active in the '%s'.",
        active_rg.c_str(), api_class(get_type()).c_str()));
  }
}

std::shared_ptr<Routing_guideline_impl>
Base_cluster_impl::get_routing_guideline(
    std::shared_ptr<Base_cluster_impl> self, const std::string &name) {
  {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds = Command_conditions::Builder::gen_cluster("getRoutingGuideline")
                    .target_instance(TargetType::InnoDBCluster,
                                     TargetType::InnoDBClusterSet)
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_not_required()
                    .cluster_global_status_any_ok()
                    .cluster_global_status_add_not_ok()
                    .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds =
            Command_conditions::Builder::gen_replicaset("getRoutingGuideline")
                .compatibility_check(
                    metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                .primary_not_required()
                .build();
        break;
      case Cluster_type::REPLICATED_CLUSTER:
        conds =
            Command_conditions::Builder::gen_clusterset("getRoutingGuideline")
                .compatibility_check(
                    metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                .primary_not_required()
                .cluster_global_status_any_ok()
                .cluster_global_status_add_not_ok()
                .build();
        break;
      default:
        throw std::logic_error(
            "Command context is incorrect for \"getRoutingGuideline\".");
    }

    check_preconditions(conds);

    if (is_cluster_clusterset_member()) {
      current_console()->print_error(shcore::str_format(
          "Cluster '%s' is a member of a ClusterSet, use "
          "<ClusterSet>.<<<getRoutingGuideline>>>() instead.",
          get_name().c_str()));
      throw shcore::Exception::runtime_error(
          "Function not available for ClusterSet members");
    }
  }

  if (name.empty()) {
    std::string active_rg = get_active_routing_guideline();

    if (active_rg.empty()) {
      throw std::invalid_argument("No active Routing Guideline");
    }

    return Routing_guideline_impl::load(self, active_rg);
  }

  return Routing_guideline_impl::load(self, name);
}

void Base_cluster_impl::remove_routing_guideline(const std::string &name) {
  {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds =
            Command_conditions::Builder::gen_cluster("removeRoutingGuideline")
                .target_instance(TargetType::InnoDBCluster,
                                 TargetType::InnoDBClusterSet)
                .quorum_state(ReplicationQuorum::States::Normal)
                .compatibility_check(
                    metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                .primary_required()
                .cluster_global_status_any_ok()
                .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds = Command_conditions::Builder::gen_replicaset(
                    "removeRoutingGuideline")
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_required()
                    .build();
        break;
      case Cluster_type::REPLICATED_CLUSTER:
        conds = Command_conditions::Builder::gen_clusterset(
                    "removeRoutingGuideline")
                    .quorum_state(ReplicationQuorum::States::All_online)
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_required()
                    .cluster_global_status_any_ok()
                    .build();
        break;
      default:
        throw std::logic_error(
            "Command context is incorrect for \"removeRoutingGuideline\".");
    }

    check_preconditions(conds);

    if (is_cluster_clusterset_member()) {
      current_console()->print_error(shcore::str_format(
          "Cluster '%s' is a member of a ClusterSet, use "
          "<ClusterSet>.<<<removeRoutingGuideline>>>() instead.",
          get_name().c_str()));
      throw shcore::Exception::runtime_error(
          "Function not available for ClusterSet members");
    }
  }

  auto md = get_metadata_storage();
  MetadataStorage::Transaction trx(md);

  // Get the default router options, i.e. the topology global ones, to check if
  // the guideline is being in use. If not, proceed to check every single router
  // options to check if any is using it
  {
    if (get_active_routing_guideline() == name) {
      current_console()->print_error(shcore::str_format(
          "Routing Guideline '%s' is currently being used by the topology.",
          name.c_str()));
      throw shcore::Exception(
          "Routing Guideline is in use and cannot be deleted",
          SHERR_DBA_ROUTING_GUIDELINE_IN_USE);
    } else if (auto router_options_md =
                   md->get_router_options(get_type(), get_id()).as_map()) {
      for (const auto &[name_router, options] : *router_options_md) {
        auto configuration_map_parsed = Router_configuration_schema(options);

        if (configuration_map_parsed.is_option_set_to_value(
                k_router_option_routing_guideline, name)) {
          current_console()->print_error(shcore::str_format(
              "Routing Guideline '%s' is currently being used by Router '%s'.",
              name.c_str(), name_router.c_str()));
          throw shcore::Exception(
              "Routing Guideline is in use and cannot be deleted",
              SHERR_DBA_ROUTING_GUIDELINE_IN_USE);
        }
      }
    }
  }

  md->delete_routing_guideline(name);
  trx.commit();

  current_console()->print_info(shcore::str_format(
      "Routing Guideline '%s' successfully removed.", name.c_str()));
}

std::unique_ptr<mysqlshdk::db::IResult> Base_cluster_impl::routing_guidelines(
    std::shared_ptr<Base_cluster_impl> self) {
  {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds = Command_conditions::Builder::gen_cluster("routingGuidelines")
                    .target_instance(TargetType::InnoDBCluster,
                                     TargetType::InnoDBClusterSet)
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_not_required()
                    .cluster_global_status_any_ok()
                    .cluster_global_status_add_not_ok()
                    .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds = Command_conditions::Builder::gen_replicaset("routingGuidelines")
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_not_required()
                    .build();
        break;
      case Cluster_type::REPLICATED_CLUSTER:
        conds = Command_conditions::Builder::gen_clusterset("routingGuidelines")
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_not_required()
                    .cluster_global_status_any_ok()
                    .cluster_global_status_add_not_ok()
                    .build();
        break;
      default:
        throw std::logic_error(
            "Command context is incorrect for \"routingGuidelines\".");
    }

    if (is_cluster_clusterset_member()) {
      current_console()->print_error(
          shcore::str_format("Cluster '%s' is a member of a ClusterSet, use "
                             "<ClusterSet>.<<<routingGuidelines>>>() instead.",
                             get_name().c_str()));
      throw shcore::Exception::runtime_error(
          "Function not available for ClusterSet members");
    }

    check_preconditions(conds);
  }

  using mysqlshdk::db::Mutable_result;

  static std::vector<mysqlshdk::db::Column> columns(
      {Mutable_result::make_column("guideline"),
       Mutable_result::make_column("active", mysqlshdk::db::Type::Integer),
       Mutable_result::make_column("routes"),
       Mutable_result::make_column("destinations")});

  auto result = std::make_unique<Mutable_result>(columns);

  for (const auto &guideline :
       get_metadata_storage()->get_routing_guidelines(get_type(), get_id())) {
    try {
      auto po = Routing_guideline_impl::load(self, guideline);

      std::string route_names;
      auto routes = po->get_routes();

      if (routes) {
        for (auto row = routes->fetch_one_named(); row;
             row = routes->fetch_one_named()) {
          route_names.append(row.get_string("name")).append(",");
        }
        if (!route_names.empty()) route_names.pop_back();
      }

      std::string destination_names;
      auto destinations = po->get_destinations();

      if (destinations) {
        for (auto row = destinations->fetch_one_named(); row;
             row = destinations->fetch_one_named()) {
          destination_names.append(row.get_string("destination")).append(",");
        }
        if (!destination_names.empty()) destination_names.pop_back();
      }

      result->append(guideline.name,
                     get_active_routing_guideline() == guideline.name,
                     route_names, destination_names);
    } catch (std::exception &e) {
      auto console = current_console();
      console->print_warning(
          shcore::str_format("Unable to load Routing Guideline '%s':\n %s",
                             guideline.name.c_str(), e.what()));
      console->print_info();
      result->append(guideline.name, false, "<invalid>", "<invalid>");
    }
  }
  return result;
}

std::shared_ptr<Routing_guideline_impl>
Base_cluster_impl::import_routing_guideline(
    std::shared_ptr<Base_cluster_impl> self, const std::string &file_path,
    const shcore::Option_pack_ref<Import_routing_guideline_options> &options) {
  {
    Command_conditions conds;
    switch (get_type()) {
      case Cluster_type::GROUP_REPLICATION:
        conds =
            Command_conditions::Builder::gen_cluster("importRoutingGuideline")
                .target_instance(TargetType::InnoDBCluster,
                                 TargetType::InnoDBClusterSet)
                .quorum_state(ReplicationQuorum::States::Normal)
                .compatibility_check(
                    metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                .primary_required()
                .cluster_global_status_any_ok()
                .build();
        break;
      case Cluster_type::ASYNC_REPLICATION:
        conds = Command_conditions::Builder::gen_replicaset(
                    "importRoutingGuideline")
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_required()
                    .build();
        break;
      case Cluster_type::REPLICATED_CLUSTER:
        conds = Command_conditions::Builder::gen_clusterset(
                    "importRoutingGuideline")
                    .quorum_state(ReplicationQuorum::States::All_online)
                    .compatibility_check(
                        metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_3_0)
                    .primary_required()
                    .cluster_global_status_any_ok()
                    .build();
        break;
      default:
        throw std::logic_error(
            "Command context is incorrect for \"importRoutingGuideline\".");
    }

    check_preconditions(conds);

    if (is_cluster_clusterset_member()) {
      current_console()->print_error(shcore::str_format(
          "Cluster '%s' is a member of a ClusterSet, use "
          "<ClusterSet>.<<<importRoutingGuideline>>>() instead.",
          get_name().c_str()));
      throw shcore::Exception::runtime_error(
          "Function not available for ClusterSet members");
    }
  }

  return Routing_guideline_impl::import(self, file_path, options);
}

std::vector<Router_metadata> Base_cluster_impl::get_routers() const {
  return get_metadata_storage()->get_routers(get_id());
}

}  // namespace mysqlsh::dba
