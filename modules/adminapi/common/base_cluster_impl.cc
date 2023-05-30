/*
 * Copyright (c) 2016, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/base_cluster_impl.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/instance_monitoring.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/setup_account.h"
#include "modules/adminapi/common/sql.h"
#include "modules/mod_shell.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace dba {

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

std::string Base_cluster_impl::make_replication_user_name(
    uint32_t server_id, std::string_view user_prefix, bool server_id_hexa) {
  if (server_id_hexa)
    return shcore::str_format("%.*s%x", static_cast<int>(user_prefix.length()),
                              user_prefix.data(), server_id);

  return shcore::str_format("%.*s%u", static_cast<int>(user_prefix.length()),
                            user_prefix.data(), server_id);
}

Base_cluster_impl::Base_cluster_impl(
    const std::string &cluster_name, std::shared_ptr<Instance> group_server,
    std::shared_ptr<MetadataStorage> metadata_storage)
    : m_cluster_name(cluster_name),
      m_cluster_server(group_server),
      m_metadata_storage(metadata_storage) {
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

void Base_cluster_impl::check_preconditions(
    const std::string &function_name,
    Function_availability *custom_func_avail) {
  log_debug("Checking '%s' preconditions.", function_name.c_str());
  bool primary_available = false;

  // Makes sure the metadata state is re-loaded on each API call
  m_metadata_storage->invalidate_cached();

  // Makes sure the primary master is reset before acquiring it on each API call
  m_primary_master.reset();

  bool primary_required = false;
  if (custom_func_avail) {
    primary_required = custom_func_avail->primary_required;
  } else {
    primary_required = Precondition_checker::get_function_preconditions(
                           api_class(get_type()) + "." + function_name)
                           .primary_required;
  }

  try {
    current_ipool()->set_metadata(get_metadata_storage());

    primary_available = (acquire_primary(primary_required, true) != nullptr);
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

  check_function_preconditions(api_class(get_type()) + "." + function_name,
                               get_metadata_storage(), get_cluster_server(),
                               primary_available, custom_func_avail);
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
    const std::string &channel_name, int timeout, bool only_received) const {
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
                                           channel_name, timeout, true);

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
                                           channel_name, timeout, true);

    if (!sync_res) {
      throw shcore::Exception(
          "Timeout reached waiting for transactions from " + master->descr() +
              " to be applied on instance '" + target_instance.descr() + "'",
          SHERR_DBA_GTID_SYNC_TIMEOUT);
    }
  }

  current_console()->print_info();
  current_console()->print_info();
}

void Base_cluster_impl::sync_transactions(
    const mysqlshdk::mysql::IInstance &target_instance,
    Instance_type instance_type, int timeout, bool only_received) const {
  switch (instance_type) {
    case Instance_type::GROUP_MEMBER: {
      sync_transactions(target_instance, mysqlshdk::gr::k_gr_applier_channel,
                        timeout, only_received);
      break;
    }
    case Instance_type::ASYNC_MEMBER: {
      sync_transactions(target_instance, k_replicaset_channel_name, timeout,
                        only_received);
      break;
    }
    case Instance_type::READ_REPLICA: {
      sync_transactions(target_instance, k_read_replica_async_channel_name,
                        timeout, only_received);
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
    mysqlshdk::mysql::Suppress_binary_log nobinlog(target_instance);

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
          Recovery_progress_style::NOWAIT);

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

void Base_cluster_impl::ensure_compatible_clone_donor(
    const mysqlshdk::mysql::IInstance &donor,
    const mysqlshdk::mysql::IInstance &recipient) {
  /*
   * A donor is compatible if:
   *
   *   - Supports clone
   *   - It has the same version of the recipient
   *   - It has the same operating system as the recipient
   */
  std::string target_address = donor.get_canonical_address();

  // Check if the instance support clone
  if (!supports_mysql_clone(donor.get_version())) {
    throw shcore::Exception(
        "Instance " + target_address + " does not support MySQL Clone.",
        SHERR_DBA_CLONE_NO_SUPPORT);
  }

  auto donor_version = donor.get_version();
  auto recipient_version = recipient.get_version();

  DBUG_EXECUTE_IF("dba_clone_version_check_fail",
                  { donor_version = mysqlshdk::utils::Version(8, 0, 17); });

  // Check if the instance has the same version of the recipient
  if (donor_version != recipient_version) {
    throw shcore::Exception("Instance " + target_address +
                                " cannot be a donor because it has a different "
                                "version (" +
                                donor_version.get_full() +
                                ") than the recipient (" +
                                recipient_version.get_full() + ").",
                            SHERR_DBA_CLONE_DIFF_VERSION);
  }

  // Check if the instance has the same OS the recipient
  auto donor_version_compile_os = donor.get_version_compile_os();
  auto recipient_version_compile_os = recipient.get_version_compile_os();

  if (donor_version_compile_os != recipient_version_compile_os) {
    throw shcore::Exception("Instance " + target_address +
                                " cannot be a donor because it has a different "
                                "Operating System (" +
                                donor_version_compile_os +
                                ") than the recipient (" +
                                recipient_version_compile_os + ").",
                            SHERR_DBA_CLONE_DIFF_OS);
  }

  // Check if the instance is running on the same platform of the recipient
  auto donor_version_compile_machine = donor.get_version_compile_machine();
  auto recipient_version_compile_machine =
      recipient.get_version_compile_machine();

  if (donor_version_compile_machine != recipient_version_compile_machine) {
    throw shcore::Exception(
        "Instance " + target_address +
            " cannot be a donor because it is running on a different "
            "platform (" +
            donor_version_compile_machine + ") than the recipient (" +
            recipient_version_compile_machine + ").",
        SHERR_DBA_CLONE_DIFF_PLATFORM);
  }
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
  connect_opts.clear_scheme();
  connect_opts.clear_host();
  connect_opts.clear_port();
  connect_opts.clear_socket();
  connect_opts.set_scheme(main_opts.get_scheme());
  if (instance_def.has_host()) connect_opts.set_host(instance_def.get_host());
  if (instance_def.has_port()) connect_opts.set_port(instance_def.get_port());
  if (instance_def.has_socket())
    connect_opts.set_socket(instance_def.get_socket());

  // is user has specified the connect-timeout connection option, use that
  // value explicitly
  if (instance_def.has_value(mysqlshdk::db::kConnectTimeout)) {
    if (connect_opts.has_value(mysqlshdk::db::kConnectTimeout)) {
      connect_opts.remove(mysqlshdk::db::kConnectTimeout);
    }

    connect_opts.set(mysqlshdk::db::kConnectTimeout,
                     instance_def.get(mysqlshdk::db::kConnectTimeout));
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
  auto metadata = std::make_shared<MetadataStorage>(get_cluster_server());

  const auto primary_instance = get_primary_master();
  shcore::on_leave_scope finally_primary([this]() { release_primary(); });

  // get the metadata version to build an accurate list of grants
  mysqlshdk::utils::Version metadata_version;
  if (!metadata->check_version(&metadata_version)) {
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

void Base_cluster_impl::remove_router_metadata(const std::string &router,
                                               bool lock_metadata) {
  check_preconditions("removeRouterMetadata");

  auto c_lock = get_lock_shared();

  /*
   * The metadata lock is currently only used in ReplicaSet, and only until it's
   * removed (deprecated because we use DB transactions). At that time, all
   * metadata locks will be removed, along with this parameter.
   */
  if (!get_metadata_storage()->remove_router(router, lock_metadata)) {
    throw shcore::Exception::argument_error("Invalid router instance '" +
                                            router + "'");
  }
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
  check_preconditions("setInstanceOption");

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
  Function_availability custom_func_avail = {
      Precondition_checker::k_min_gr_version,
      TargetType::InnoDBCluster | TargetType::InnoDBClusterSet,
      ReplicationQuorum::State(ReplicationQuorum::States::Normal),
      {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}},
      true,
      Precondition_checker::kClusterGlobalStateAnyOk};

  std::string opt_namespace, opt;
  shcore::Value val;
  std::tie(opt_namespace, opt, val) =
      validate_set_option_namespace(option, value, k_supported_set_option_tags);

  Function_availability *custom_precondition{nullptr};
  if (!opt_namespace.empty() && get_type() == Cluster_type::GROUP_REPLICATION) {
    custom_precondition = &custom_func_avail;
  }
  check_preconditions("setOption", custom_precondition);

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
      (value.type == shcore::String))
    return value.as_string();

  return {};
}

std::string Base_cluster_impl::query_cluster_instance_auth_cert_subject(
    std::string_view instance_uuid) const {
  if (shcore::Value value;
      get_metadata_storage()->query_instance_attribute(
          instance_uuid, k_instance_attribute_cert_subject, &value) &&
      (value.type == shcore::String))
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
    if (value.type != shcore::Value_type::Null) {
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
            type_name(value.type).c_str()));
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

void Base_cluster_impl::set_routing_option(const std::string &router,
                                           const std::string &option,
                                           const shcore::Value &value) {
  // Preconditions the same to sister function
  check_preconditions("setRoutingOption");

  std::string opt_namespace, opt;
  shcore::Value val;
  std::tie(opt_namespace, opt, val) =
      validate_set_option_namespace(option, value, {});

  if (!opt_namespace.empty()) {
    set_router_tag(router, opt, val);
    return;
  }

  auto value_fixed = validate_router_option(*this, opt, val);

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
}

shcore::Dictionary_t Base_cluster_impl::routing_options(
    const std::string &router) {
  check_preconditions("routingOptions");

  return router_options(get_metadata_storage().get(), get_type(), get_id(),
                        router);
}

}  // namespace dba
}  // namespace mysqlsh
