/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/replicaset/add_instance.h"

#include <climits>
#include <memory>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "my_dbug.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/textui/textui.h"

namespace mysqlsh {
namespace dba {

// # of seconds to wait until recovery starts
constexpr const int k_recovery_start_timeout = 30;

// # of seconds to wait until server finished restarting during recovery
constexpr const int k_server_recovery_restart_timeout = 60;

Add_instance::Add_instance(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const ReplicaSet &replicaset, const Group_replication_options &gr_options,
    const Clone_options &clone_options,
    const mysqlshdk::utils::nullable<std::string> &instance_label,
    bool interactive, int wait_recovery, const std::string &replication_user,
    const std::string &replication_password, bool overwrite_seed,
    bool skip_instance_check, bool skip_rpl_user)
    : m_instance_cnx_opts(instance_cnx_opts),
      m_replicaset(replicaset),
      m_gr_opts(gr_options),
      m_clone_opts(clone_options),
      m_instance_label(instance_label),
      m_interactive(interactive),
      m_rpl_user(replication_user),
      m_rpl_pwd(replication_password),
      m_seed_instance(overwrite_seed),
      m_skip_instance_check(skip_instance_check),
      m_skip_rpl_user(skip_rpl_user) {
  m_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());

  // Init m_progress_style
  if (wait_recovery == 0)
    m_progress_style = Recovery_progress_style::NOWAIT;
  else if (wait_recovery == 1)
    m_progress_style = Recovery_progress_style::NOINFO;
  else if (wait_recovery == 2)
    m_progress_style = Recovery_progress_style::TEXTUAL;
  else
    m_progress_style = Recovery_progress_style::PROGRESSBAR;
}

Add_instance::Add_instance(
    mysqlshdk::mysql::IInstance *target_instance, const ReplicaSet &replicaset,
    const Group_replication_options &gr_options,
    const Clone_options &clone_options,
    const mysqlshdk::utils::nullable<std::string> &instance_label,
    bool interactive, int wait_recovery, const std::string &replication_user,
    const std::string &replication_password, bool overwrite_seed,
    bool skip_instance_check, bool skip_rpl_user)
    : m_replicaset(replicaset),
      m_gr_opts(gr_options),
      m_clone_opts(clone_options),
      m_instance_label(instance_label),
      m_interactive(interactive),
      m_rpl_user(replication_user),
      m_rpl_pwd(replication_password),
      m_seed_instance(overwrite_seed),
      m_skip_instance_check(skip_instance_check),
      m_skip_rpl_user(skip_rpl_user) {
  assert(target_instance);
  m_reuse_session_for_target_instance = true;
  m_target_instance = target_instance;
  m_instance_cnx_opts = target_instance->get_connection_options();
  m_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());

  // Init m_progress_style
  if (wait_recovery == 0)
    m_progress_style = Recovery_progress_style::NOWAIT;
  else if (wait_recovery == 1)
    m_progress_style = Recovery_progress_style::NOINFO;
  else if (wait_recovery == 2)
    m_progress_style = Recovery_progress_style::TEXTUAL;
  else
    m_progress_style = Recovery_progress_style::PROGRESSBAR;
}

Add_instance::~Add_instance() {
  if (!m_reuse_session_for_target_instance) {
    delete m_target_instance;
  }
}

namespace {
bool check_recoverable_from_any(const ReplicaSet *replicaset,
                                mysqlshdk::mysql::IInstance *target_instance) {
  // Check if the GTIDs were purged from the whole cluster
  bool recoverable = false;

  replicaset->execute_in_members(
      {"'ONLINE'"}, target_instance->get_connection_options(), {},
      [&recoverable, &target_instance](
          const std::shared_ptr<mysqlshdk::db::ISession> &session) {
        mysqlshdk::mysql::Instance instance(session);

        // Get the gtid state in regards to the cluster_session
        mysqlshdk::mysql::Replica_gtid_state state =
            mysqlshdk::mysql::check_replica_gtid_state(
                instance, *target_instance, nullptr, nullptr);

        if (state != mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE) {
          recoverable = true;
          // stop searching as soon as we find a possible donor
          return false;
        }
        return true;
      });

  return recoverable;
}

void check_gtid_consistency_and_recoverability(
    const ReplicaSet *replicaset, mysqlshdk::mysql::IInstance *target_instance,
    bool *out_recovery_possible, bool *out_recovery_safe,
    Member_recovery_method recovery_method) {
  auto console = current_console();

  std::string errant_gtid_set;

  // Get the gtid state in regards to the cluster_session
  mysqlshdk::mysql::Replica_gtid_state state =
      mysqlshdk::mysql::check_replica_gtid_state(
          mysqlshdk::mysql::Instance(
              replicaset->get_cluster()->get_group_session()),
          *target_instance, nullptr, &errant_gtid_set);

  switch (state) {
    case mysqlshdk::mysql::Replica_gtid_state::NEW:
      console->println();
      if (replicaset->get_cluster()->get_gtid_set_is_complete()) {
        console->print_note(mysqlshdk::textui::format_markup_text(
            {"The target instance '" + target_instance->descr() +
             "' has not been pre-provisioned (GTID set is empty), but the "
             "cluster was configured to assume that incremental distributed "
             "state recovery can correctly provision it in this case."},
            80));

        *out_recovery_possible = true;
        *out_recovery_safe = true;
      } else {
        console->print_note(mysqlshdk::textui::format_markup_text(
            {"The target instance '" + target_instance->descr() +
             "' has not been pre-provisioned (GTID set is empty). The Shell is"
             " unable to decide whether incremental distributed state recovery "
             "can correctly provision it."},
            80));

        *out_recovery_possible = true;
        *out_recovery_safe = false;
      }

      if (recovery_method == Member_recovery_method::AUTO) {
        console->print_info(mysqlshdk::textui::format_markup_text(
            {shcore::str_format(
                 "The safest and most convenient way to provision a new "
                 "instance is through automatic clone provisioning, which will "
                 "completely overwrite the state of '%s' with a physical "
                 "snapshot from an existing cluster member. To use this method "
                 "by default, set the 'recoveryMethod' option to 'clone'.",
                 target_instance->descr().c_str()),

             "The incremental distributed state recovery may be safely used if "
             "you are sure all updates ever executed in the cluster were done "
             "with GTIDs enabled, there are no purged transactions and the new "
             "instance contains the same GTID set as the cluster or a subset "
             "of it. To use this method by default, set the 'recoveryMethod' "
             "option to 'incremental'."},
            80));
      }
      console->print_info();
      break;

    case mysqlshdk::mysql::Replica_gtid_state::DIVERGED:
      console->println();
      console->print_warning(mysqlshdk::textui::format_markup_text(
          {"A GTID set check of the MySQL instance at '" +
           target_instance->descr() +
           "' determined that it contains transactions that "
           "do not originate from the cluster, which must be "
           "discarded before it can join the cluster."},
          80));

      console->print_info();
      console->print_info(target_instance->descr() +
                          " has the following errant GTIDs that do not exist "
                          "in the cluster:\n" +
                          errant_gtid_set);
      console->print_info();

      console->print_warning(mysqlshdk::textui::format_markup_text(
          {"Discarding these extra GTID events can either be done manually or "
           "by completely overwriting the state of " +
               target_instance->descr() +
               " with a physical snapshot from an existing cluster member. To "
               "use this method by default, set the 'recoveryMethod' option to "
               "'clone'.",

           "Having extra GTID events is not expected, and it is "
           "recommended to investigate this further and ensure that the data "
           "can be removed prior to choosing the clone recovery method."},
          80));

      *out_recovery_possible = false;
      *out_recovery_safe = false;
      break;

    case mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE:
      if (!check_recoverable_from_any(replicaset, target_instance)) {
        console->print_note(mysqlshdk::textui::format_markup_text(
            {"A GTID set check of the MySQL instance at '" +
             target_instance->descr() +
             "' determined that it is missing transactions that "
             "were purged from all cluster members."},
            80));

        *out_recovery_possible = false;
        *out_recovery_safe = false;
      } else {
        *out_recovery_possible = true;
        *out_recovery_safe = true;
      }
      break;

    case mysqlshdk::mysql::Replica_gtid_state::RECOVERABLE:
    case mysqlshdk::mysql::Replica_gtid_state::IDENTICAL:
      // This is the only case where it's safe to assume that the instance can
      // be safely recovered through binlog.
      *out_recovery_possible = true;
      *out_recovery_safe = true;

      if (recovery_method == Member_recovery_method::AUTO)
        console->print_info(mysqlshdk::textui::format_markup_text(
            {shcore::str_format(
                 "The safest and most convenient way to provision a new "
                 "instance is through automatic clone provisioning, which will "
                 "completely overwrite the state of '%s' with a physical "
                 "snapshot from an existing cluster member. To use this method "
                 "by default, set the 'recoveryMethod' option to 'clone'.",
                 target_instance->descr().c_str()),

             "The incremental distributed state recovery may be safely used if "
             "you are sure all updates ever executed in the cluster were done "
             "with GTIDs enabled, there are no purged transactions and the new "
             "instance contains the same GTID set as the cluster or a subset "
             "of it. To use this method by default, set the 'recoveryMethod' "
             "option to 'incremental'."},
            80));
      console->print_info();
      break;

    default:
      throw std::logic_error("Internal error");
  }
}
}  // namespace

/**
 * Checks what type of recovery is possible and whether clone should be enabled.
 *
 * Input:
 * - recoveryMethod option
 * - GTID state of the target instance, whether it's:
 *    - recoverable
 *    - compatible (no errant transactions)
 *    - safe to assume that it was cloned/copied from the cluster
 * - User prompts, when necessary and interaction allowed
 * - Whether gtidSetComplete was set to true when the cluster was created
 *
 * Output:
 * - exception on fatal conditions or cancellation by user
 */
Member_recovery_method Add_instance::validate_instance_recovery() {
  /*
  IF recoveryMethod = clone THEN
      CONTINUE WITH CLONE
  ELSE IF recoveryMethod = recovery THEN
      IF recovery_possible THEN
          CONTINUE WITHOUT CLONE
      ELSE
          ERROR "Recovery is not possible"
  ELSE
      IF recovery_possible THEN
          IF recovery_safe OR gtidSetComplete THEN
              CONTINUE WITHOUT CLONE
          ELSE
              IF clone_supported AND NOT clone_disabled THEN
                  PROMPT Clone, Recovery, Abort
              ELSE
                  PROMPT Recovery, Abort
      ELSE
          IF clone_supported AND NOT clone_disabled THEN
              PROMPT Clone, Abort
          ELSE
              ABORT
   */

  auto console = mysqlsh::current_console();

  bool recovery_possible;
  bool recovery_safe;

  Member_recovery_method recovery_method = Member_recovery_method::AUTO;

  check_gtid_consistency_and_recoverability(&m_replicaset, m_target_instance,
                                            &recovery_possible, &recovery_safe,
                                            m_clone_opts.recovery_method);

  if (m_clone_opts.recovery_method == Member_recovery_method::CLONE) {
    // User explicitly selected clone, so just do it.
    // Check for whether clone is supported was already done in Clone_options
    if (m_clone_disabled) {
      throw shcore::Exception::runtime_error(
          "Cannot use recoveryMethod=clone option because the disableClone "
          "option was set for the cluster.");
    }
    console->print_info(
        "Clone based recovery selected through the recoveryMethod option");
    recovery_method = Member_recovery_method::CLONE;

  } else if (m_clone_opts.recovery_method ==
             Member_recovery_method::INCREMENTAL) {
    if (recovery_possible) {
      console->print_info(
          "Incremental distributed state recovery selected through the "
          "recoveryMethod option");
      recovery_method = Member_recovery_method::INCREMENTAL;
    } else {
      throw shcore::Exception::runtime_error(
          "Cannot use recoveryMethod=incremental option because the GTID state "
          "is not compatible or cannot be recovered.");
    }

  } else {
    enum Prompt_type {
      None,
      Clone_incremental_abort,
      Clone_abort,
      Incremental_abort
    };

    Prompt_type prompt = None;

    if (recovery_possible) {
      if (recovery_safe) {
        // Even though we detected that incremental distributed recovery is
        // safely usable, it might not be used if
        // group_replication_clone_threshold was set to 1
        mysqlshdk::utils::nullable<int64_t> current_threshold =
            m_target_instance->get_sysvar_int(
                "group_replication_clone_threshold");

        if (!current_threshold.is_null() && *current_threshold != LLONG_MAX) {
          console->print_note(
              "Incremental distributed state recovery was determined to be "
              "safely usable, however, group_replication_clone_threshold has "
              "been set to " +
              std::to_string(*current_threshold) +
              ", which may cause Group Replication to use clone based "
              "recovery.");
        } else {
          console->print_info(
              "Incremental distributed state recovery was selected because it "
              "seems to be safely usable.");
          recovery_method = Member_recovery_method::INCREMENTAL;
        }
      } else {
        if (m_clone_supported && !m_clone_disabled) {
          prompt = Clone_incremental_abort;
        } else {
          prompt = Incremental_abort;
        }
      }
    } else {  // recovery not possible, can only do clone
      if (m_clone_supported && !m_clone_disabled) {
        prompt = Clone_abort;
      } else {
        console->print_error(
            "The target instance must be either cloned or fully provisioned "
            "before it can be added to the target cluster.");

        if (!m_clone_supported) {
          console->print_info(
              "Automatic clone support is available starting with MySQL "
              "8.0.17 and is the recommended method for provisioning "
              "instances.");
        }

        throw shcore::Exception::runtime_error(
            "Instance provisioning required");
      }
    }

    if (prompt != None) {
      if (!m_interactive) {
        throw shcore::Exception::runtime_error(
            "'recoveryMethod' option must be set to 'clone' or 'incremental'");
      }

      console->print_info();

      switch (prompt) {
        case Clone_incremental_abort:
          switch (console->confirm("Please select a recovery method",
                                   mysqlsh::Prompt_answer::YES, "&Clone",
                                   "&Incremental recovery", "&Abort")) {
            case mysqlsh::Prompt_answer::YES:
              recovery_method = Member_recovery_method::CLONE;
              break;
            case mysqlsh::Prompt_answer::NO:
              recovery_method = Member_recovery_method::INCREMENTAL;
              break;
            case mysqlsh::Prompt_answer::ALT:
            case mysqlsh::Prompt_answer::NONE:
              throw shcore::cancelled("Cancelled");
          }
          break;

        case Clone_abort:
          switch (console->confirm("Please select a recovery method",
                                   mysqlsh::Prompt_answer::NO, "&Clone",
                                   "&Abort")) {
            case mysqlsh::Prompt_answer::YES:
              recovery_method = Member_recovery_method::CLONE;
              break;
            default:
              throw shcore::cancelled("Cancelled");
          }
          break;

        case Incremental_abort:
          switch (console->confirm("Please select a recovery method",
                                   mysqlsh::Prompt_answer::YES,
                                   "&Incremental recovery", "&Abort")) {
            case mysqlsh::Prompt_answer::YES:
              recovery_method = Member_recovery_method::INCREMENTAL;
              break;
            default:
              throw shcore::cancelled("Cancelled");
          }
          break;

        case None:
          break;
      }
    }
  }
  return recovery_method;
}

void Add_instance::ensure_instance_version_compatibility() const {
  // Get the lowest server version of the online members of the cluster.
  mysqlshdk::utils::Version lowest_cluster_version =
      m_replicaset.get_cluster()->get_lowest_instance_version();

  try {
    // Check instance version compatibility according to Group Replication.
    mysqlshdk::gr::check_instance_version_compatibility(*m_target_instance,
                                                        lowest_cluster_version);
  } catch (const std::runtime_error &err) {
    auto console = mysqlsh::current_console();
    console->print_error(
        "Cannot join instance '" + m_instance_address +
        "' to cluster: instance version is incompatible with the cluster.");
    throw;
  }

  // Print a warning if the instance is only read compatible.
  if (mysqlshdk::gr::is_instance_only_read_compatible(*m_target_instance,
                                                      lowest_cluster_version)) {
    auto console = mysqlsh::current_console();
    console->print_warning("The instance '" + m_instance_address +
                           "' is only read compatible with the cluster, thus "
                           "it will join the cluster in R/O mode.");
  }
}

/**
 * Resolves SSL mode based on the given options.
 *
 * Determine the SSL mode for the instance based on the cluster SSL mode and
 * if SSL is enabled on the target instance. The same SSL mode of the cluster
 * is used for new instance joining it.
 */
void Add_instance::resolve_ssl_mode() {
  // Show deprecation message for memberSslMode option if if applies.
  if (!m_gr_opts.ssl_mode.is_null()) {
    auto console = mysqlsh::current_console();
    console->print_warning(mysqlsh::dba::ReplicaSet::kWarningDeprecateSslMode);
    console->println();
  }

  // Set SSL Mode to AUTO by default (not defined).
  if (m_gr_opts.ssl_mode.is_null()) {
    m_gr_opts.ssl_mode = dba::kMemberSSLModeAuto;
  }

  std::string new_ssl_mode = resolve_instance_ssl_mode(
      *m_target_instance, *m_peer_instance, *m_gr_opts.ssl_mode);

  if (new_ssl_mode != *m_gr_opts.ssl_mode) {
    m_gr_opts.ssl_mode = new_ssl_mode;
    log_warning("SSL mode used to configure the instance: '%s'",
                m_gr_opts.ssl_mode->c_str());
  }
}

void Add_instance::ensure_unique_server_id() const {
  try {
    m_replicaset.validate_server_id(*m_target_instance);
  } catch (const std::runtime_error &err) {
    auto console = mysqlsh::current_console();
    console->print_error("Cannot add instance '" + m_instance_address +
                         "' to the cluster because it has the same server ID "
                         "of a member of the cluster. Please change the server "
                         "ID of the instance to add: all members must have a "
                         "unique server ID.");
    throw;
  }
}

void Add_instance::handle_recovery_account() const {
  /*
    Since clone copies all the data, including mysql.slave_master_info (where
    the CHANGE MASTER credentials are stored), the following issue may happen:

    1) Group is bootstrapped on server1.
      user_account:  mysql_innodb_cluster_1
    2) server2 is added to the group
      user_account:  mysql_innodb_cluster_2
    But server2 is cloned from server1, which means its recovery account
      will be mysql_innodb_cluster_1
    3) server3 is added to the group
      user_account:  mysql_innodb_cluster_3
      But server3 is cloned from server1, which means its recovery account
      will be mysql_innodb_cluster_1
    4) server1 is removed from the group
      mysql_innodb_cluster_1 account is dropped on all group servers.

    To avoid such situation, we will re-issue the CHANGE MASTER query after
    clone to ensure the right account is used. On top of that, we will make
    sure that the account is not dropped if any other cluster member is using
    it.

    The approach to tackle the usage of the recovery accounts is to store them
    in the Metadata, i.e. in addInstance() the user created in the "donor" is
    stored in the Metadata table referring to the instance being added.

    In removeInstance(), the Metadata is verified to check whether the recovery
    user of that instance is registered for more than one instance and if that's
    the case then it won't be dropped.
   */

  // Get the "donor" recovery account
  mysqlshdk::mysql::Instance md_inst = mysqlshdk::mysql::Instance(
      m_replicaset.get_cluster()->get_metadata_storage()->get_session());

  std::string recovery_user, recovery_user_host;
  std::tie(recovery_user, recovery_user_host) =
      m_replicaset.get_cluster()
          ->get_metadata_storage()
          ->get_instance_recovery_account(md_inst.get_uuid());

  // Insert the "donor" recovery account into the MD of the target instance
  log_debug("Adding donor recovery account to metadata");
  m_replicaset.get_cluster()
      ->get_metadata_storage()
      ->update_instance_recovery_account(m_target_instance->get_uuid(),
                                         recovery_user, "%");
}

void Add_instance::update_change_master() const {
  // See note in handle_recovery_account()
  log_debug("Re-issuing the CHANGE MASTER command");

  mysqlshdk::gr::change_recovery_credentials(*m_target_instance, m_rpl_user,
                                             m_rpl_pwd);

  // Update the recovery account to the right one
  log_debug("Adding recovery account to metadata");
  m_replicaset.get_cluster()
      ->get_metadata_storage()
      ->update_instance_recovery_account(m_target_instance->get_uuid(),
                                         m_rpl_user, "%");
}

void Add_instance::prepare() {
  // Connect to the target instance (if needed).
  if (!m_reuse_session_for_target_instance) {
    // Get instance user information from the cluster session if missing.
    if (!m_instance_cnx_opts.has_user()) {
      Connection_options cluster_cnx_opt = m_replicaset.get_cluster()
                                               ->get_group_session()
                                               ->get_connection_options();

      if (!m_instance_cnx_opts.has_user() && cluster_cnx_opt.has_user())
        m_instance_cnx_opts.set_user(cluster_cnx_opt.get_user());
    }

    std::shared_ptr<mysqlshdk::db::ISession> session{establish_mysql_session(
        m_instance_cnx_opts, current_shell_options()->get().wizards)};
    m_target_instance = new mysqlshdk::mysql::Instance(session);
  }

  //  Override connection option if no password was initially provided.
  if (!m_instance_cnx_opts.has_password()) {
    m_instance_cnx_opts = m_target_instance->get_connection_options();
  }

  // Connect to the peer instance.
  if (!m_seed_instance) {
    mysqlshdk::db::Connection_options peer(m_replicaset.pick_seed_instance());
    std::shared_ptr<mysqlshdk::db::ISession> peer_session;
    if (peer.uri_endpoint() != m_replicaset.get_cluster()
                                   ->get_group_session()
                                   ->get_connection_options()
                                   .uri_endpoint()) {
      peer_session =
          establish_mysql_session(peer, current_shell_options()->get().wizards);
      m_use_cluster_session_for_peer = false;
    } else {
      peer_session = m_replicaset.get_cluster()->get_group_session();
    }
    m_peer_instance =
        shcore::make_unique<mysqlshdk::mysql::Instance>(peer_session);
  }

  // Validate the GR options.
  m_gr_opts.check_option_values(m_target_instance->get_version());

  // Print warning if auto-rejoin is set (not 0).
  if (!m_gr_opts.auto_rejoin_tries.is_null() &&
      *(m_gr_opts.auto_rejoin_tries) != 0) {
    auto console = mysqlsh::current_console();
    console->print_warning(
        "The member will only proceed according to its exitStateAction if "
        "auto-rejoin fails (i.e. all retry attempts are exhausted).");
    console->println();
  }

  // Validate the Clone options.
  m_clone_opts.check_option_values(m_target_instance->get_version(),
                                   &m_replicaset);

  // BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC
  // - exitStateAction default value must be READ_ONLY
  // - exitStateAction default value should only be set if supported in
  // the target instance
  if (m_gr_opts.exit_state_action.is_null() &&
      is_option_supported(m_target_instance->get_version(), kExpelTimeout,
                          k_global_replicaset_supported_options)) {
    m_gr_opts.exit_state_action = "READ_ONLY";
  }

  // Check if clone is disabled
  m_clone_disabled = m_replicaset.get_cluster()->get_disable_clone_option();

  // Check if clone is supported
  m_clone_supported = mysqlshdk::mysql::is_clone_available(*m_target_instance);

  if (!m_seed_instance) {
    // Check instance version compatibility with cluster.
    ensure_instance_version_compatibility();
  }

  // Resolve the SSL Mode to use to configure the instance.
  // TODO(pjesus): remove the 'if (!m_seed_instance)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_seed_instance) {
    resolve_ssl_mode();
  }

  // Make sure the target instance does not already belong to a cluster.
  mysqlsh::dba::checks::ensure_instance_not_belong_to_cluster(
      *m_target_instance, m_replicaset.get_cluster()->get_group_session());

  // Check GTID consistency and whether clone is needed
  m_clone_opts.recovery_method = validate_instance_recovery();

  // Verify if the instance is running asynchronous (master-slave) replication
  // NOTE: Only verify if this is being called from a addInstance() and not a
  // createCluster() command.
  // TODO(pjesus): remove the 'if (!m_seed_instance)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_seed_instance) {
    auto console = mysqlsh::current_console();

    log_debug(
        "Checking if instance '%s' is running asynchronous (master-slave) "
        "replication.",
        m_instance_address.c_str());

    if (mysqlshdk::mysql::is_async_replication_running(*m_target_instance)) {
      console->print_error(
          "Cannot add instance '" + m_instance_address +
          "' to the cluster because it has asynchronous (master-slave) "
          "replication configured and running. Please stop the slave threads "
          "by executing the query: 'STOP SLAVE;'");

      throw shcore::Exception::runtime_error(
          "The instance '" + m_instance_address +
          "' is running asynchronous (master-slave) replication.");
    }
  }

  // Check instance server UUID (must be unique among the cluster members).
  m_replicaset.validate_server_uuid(m_target_instance->get_session());

  // Ensure instance server ID is unique among the cluster members.
  ensure_unique_server_id();

  // Get the address used by GR for the added instance (used in MD).
  m_host_in_metadata = m_target_instance->get_canonical_hostname();
  m_address_in_metadata = m_target_instance->get_canonical_address();

  // Resolve GR local address.
  // NOTE: Must be done only after getting the report_host used by GR and for
  //       the metadata;
  m_gr_opts.local_address = mysqlsh::dba::resolve_gr_local_address(
      m_gr_opts.local_address, m_host_in_metadata,
      m_instance_cnx_opts.get_port());

  // If this is not seed instance, then we should try to read the
  // failoverConsistency and expelTimeout values from a a cluster member.
  // TODO(pjesus): remove the 'if (!m_seed_instance)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_seed_instance) {
    m_replicaset.query_group_wide_option_values(
        m_target_instance, &m_gr_opts.consistency, &m_gr_opts.expel_timeout);
  }

  // Check instance configuration and state, like dba.checkInstance
  // But don't do it if it was already done by the caller
  // TODO(pjesus): remove the 'if (!m_skip_instance_check)' for refactor of
  //               reboot cluster (WL#11561), i.e. always execute check, not
  //               supposed to use Add_instance operation anymore.
  if (!m_skip_instance_check) {
    ensure_instance_configuration_valid(*m_target_instance);
  }

  // Set the internal configuration object: read/write configs from the server.
  m_cfg = mysqlsh::dba::create_server_config(
      m_target_instance, mysqlshdk::config::k_dft_cfg_server_handler);
}

void Add_instance::handle_gr_protocol_version() {
  // Get the current protocol version in use in the group
  try {
    mysqlshdk::utils::Version gr_protocol_version =
        mysqlshdk::gr::get_group_protocol_version(*m_peer_instance);

    // If the target instance being added does not support the GR protocol
    // version in use on the group (because it is an older version), the
    // addInstance command must set the GR protocol of the cluster to the
    // version of the target instance.
    if (mysqlshdk::gr::is_protocol_downgrade_required(gr_protocol_version,
                                                      *m_target_instance)) {
      mysqlshdk::gr::set_group_protocol_version(
          *m_peer_instance, m_target_instance->get_version());
    }
  } catch (const shcore::Exception &error) {
    // The UDF may fail with MySQL Error 1123 if any of the members is
    // RECOVERING In such scenario, we must abort the upgrade protocol
    // version process and warn the user
    if (error.code() == ER_CANT_INITIALIZE_UDF) {
      auto console = mysqlsh::current_console();
      console->print_note(
          "Unable to determine the Group Replication protocol version, "
          "while verifying if a protocol downgrade is required: " +
          std::string(error.what()) + ".");
    } else {
      throw;
    }
  }
}

bool Add_instance::handle_replication_user() {
  // Creates the replication user ONLY if not already given and if
  // skip_rpl_user was not set to true.
  // NOTE: User always created at the seed instance.
  if (!m_skip_rpl_user && m_rpl_user.empty()) {
    auto user_creds =
        m_replicaset.get_cluster()->create_replication_user(m_target_instance);
    m_rpl_user = user_creds.user;
    m_rpl_pwd = user_creds.password.get_safe();

    return true;
  }
  return false;
}

/**
 * Clean (drop) the replication user only if it was created, in case the
 * operation fails.
 */
void Add_instance::clean_replication_user() {
  if (!m_rpl_user.empty()) {
    mysqlshdk::mysql::Instance primary(
        m_replicaset.get_cluster()->get_group_session());
    log_debug("Dropping recovery user '%s'@'%%' at instance '%s'.",
              m_rpl_user.c_str(), primary.descr().c_str());
    primary.drop_user(m_rpl_user, "%", true);
  }
}

void Add_instance::log_used_gr_options() {
  auto console = mysqlsh::current_console();

  if (!m_gr_opts.local_address.is_null() && !m_gr_opts.local_address->empty()) {
    log_info("Using Group Replication local address: %s",
             m_gr_opts.local_address->c_str());
  }

  if (!m_gr_opts.group_seeds.is_null() && !m_gr_opts.group_seeds->empty()) {
    log_info("Using Group Replication group seeds: %s",
             m_gr_opts.group_seeds->c_str());
  }

  if (!m_gr_opts.exit_state_action.is_null() &&
      !m_gr_opts.exit_state_action->empty()) {
    log_info("Using Group Replication exit state action: %s",
             m_gr_opts.exit_state_action->c_str());
  }

  if (!m_gr_opts.member_weight.is_null()) {
    log_info("Using Group Replication member weight: %s",
             std::to_string(*m_gr_opts.member_weight).c_str());
  }

  if (!m_gr_opts.consistency.is_null() && !m_gr_opts.consistency->empty()) {
    log_info("Using Group Replication failover consistency: %s",
             m_gr_opts.consistency->c_str());
  }

  if (!m_gr_opts.expel_timeout.is_null()) {
    log_info("Using Group Replication expel timeout: %s",
             std::to_string(*m_gr_opts.expel_timeout).c_str());
  }

  if (!m_gr_opts.auto_rejoin_tries.is_null()) {
    log_info("Using Group Replication auto-rejoin tries: %s",
             std::to_string(*m_gr_opts.auto_rejoin_tries).c_str());
  }
}

shcore::Value Add_instance::execute() {
  auto console = mysqlsh::current_console();

  // TODO(pjesus): remove the 'if (!m_seed_instance)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_seed_instance) {
    std::string msg =
        "A new instance will be added to the InnoDB cluster. Depending on the "
        "amount of data on the cluster this might take from a few seconds to "
        "several hours.";
    std::string message =
        mysqlshdk::textui::format_markup_text({msg}, 80, 0, false);
    console->print_info(message);
    console->println();
  }

  // Common informative logging
  log_used_gr_options();

  // TODO(pjesus): remove the 'if (!m_seed_instance)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_seed_instance) {
    console->print_info("Adding instance to the cluster...");
    console->println();
  }

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  mysqlshdk::gr::install_group_replication_plugin(*m_target_instance, nullptr);

  // Handle GR protocol version.
  // TODO(pjesus): remove the 'if (!m_seed_instance)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_seed_instance) {
    handle_gr_protocol_version();
  }

  // Get the current number of replicaSet members
  uint64_t replicaset_count =
      m_replicaset.get_cluster()->get_metadata_storage()->get_replicaset_count(
          m_replicaset.get_id());

  // Clone handling
  if (m_clone_supported) {
    bool enable_clone = !m_clone_disabled;

    // Force clone if requested
    if (m_clone_opts.recovery_method == Member_recovery_method::CLONE) {
      m_restore_clone_threshold =
          mysqlshdk::mysql::force_clone(*m_target_instance);

      // RESET MASTER to clear GTID_EXECUTED in case it's diverged, otherwise
      // clone is not executed and GR rejects the instance
      m_target_instance->query("RESET MASTER");
    } else if (m_clone_opts.recovery_method ==
               Member_recovery_method::INCREMENTAL) {
      // Force incremental distributed recovery if requested
      enable_clone = false;
    }

    // Make sure the Clone plugin is installed or uninstalled depending on the
    // value of disableClone
    if (enable_clone) {
      mysqlshdk::mysql::install_clone_plugin(*m_target_instance, nullptr);
    } else {
      mysqlshdk::mysql::uninstall_clone_plugin(*m_target_instance, nullptr);
    }
  }

  DBUG_EXECUTE_IF("dba_abort_join_group", { throw std::logic_error("debug"); });

  // Handle the replication user creation.
  bool owns_repl_user = handle_replication_user();

  // we need a point in time as close as possible, but still earlier than
  // when recovery starts to monitor the recovery phase
  std::string join_begin_time =
      m_target_instance->queryf_one_string(0, "", "SELECT NOW(6)");

  // TODO(pjesus): remove the 'if (m_seed_instance)' for refactor of reboot
  //               cluster (WL#11561), mysqlsh::dba::start_replicaset() should
  //               be used directly instead of the Add_instance operation.
  if (m_seed_instance) {
    if (!m_gr_opts.group_name.is_null() && !m_gr_opts.group_name->empty()) {
      log_info("Using Group Replication group name: %s",
               m_gr_opts.group_name->c_str());
    }

    log_info("Starting Replicaset with '%s' using account %s",
             m_instance_address.c_str(),
             m_instance_cnx_opts.get_user().c_str());

    // Determine the topology mode to use.
    // TODO(pjesus): the topology mode (multi_primary) should not be needed,
    //               remove it for refactor of reboot cluster (WL#11561) and
    //               just pass a null (bool). Now, the current behaviour is
    //               maintained.
    mysqlshdk::utils::nullable<bool> multi_primary =
        m_replicaset.get_topology_type() == ReplicaSet::kTopologyMultiPrimary;

    // Start the replicaset to bootstrap Group Replication.
    mysqlsh::dba::start_replicaset(*m_target_instance, m_gr_opts, multi_primary,
                                   m_cfg.get());

  } else {
    // If no group_seeds value was provided by the user, then,
    // before joining instance to cluster, get the values of the
    // gr_local_address from all the active members of the cluster
    if (m_gr_opts.group_seeds.is_null() || m_gr_opts.group_seeds->empty()) {
      m_gr_opts.group_seeds = m_replicaset.get_cluster_group_seeds();
    }

    log_info("Joining '%s' to ReplicaSet using account %s to peer '%s'.",
             m_instance_address.c_str(),
             m_peer_instance->get_connection_options().get_user().c_str(),
             m_peer_instance->descr().c_str());

    // Join the instance to the Group Replication group.
    mysqlsh::dba::join_replicaset(*m_target_instance, *m_peer_instance,
                                  m_rpl_user, m_rpl_pwd, m_gr_opts,
                                  replicaset_count, m_cfg.get());

    // Wait until recovery done. Will throw an exception if recovery fails.
    try {
      monitor_gr_recovery_status(m_instance_cnx_opts, join_begin_time,
                                 m_progress_style, k_recovery_start_timeout,
                                 k_server_recovery_restart_timeout);
    } catch (const shcore::Exception &e) {
      // If the recovery itself failed we abort, but monitoring errors can be
      // ignored.
      if (e.code() == SHERR_DBA_CLONE_RECOVERY_FAILED ||
          e.code() == SHERR_DBA_DISTRIBUTED_RECOVERY_FAILED) {
        console->print_error("Recovery error in added instance: " + e.format());
        throw;
      } else {
        console->print_warning(
            "Error while waiting for recovery of the added instance: " +
            e.format());
      }
    }

    // When clone is used, the target instance will restart and all
    // connections are closed so we need to test if the connection to the
    // target instance and MD are closed and re-open if necessary
    refresh_target_connections();
  }

  // Check if instance address already belong to replicaset (metadata).
  bool is_instance_on_md =
      m_replicaset.get_cluster()
          ->get_metadata_storage()
          ->is_instance_on_replicaset(m_replicaset.get_id(),
                                      m_address_in_metadata);

  log_debug("ReplicaSet %s: Instance '%s' %s",
            std::to_string(m_replicaset.get_id()).c_str(),
            m_instance_address.c_str(),
            is_instance_on_md ? "is already in the Metadata."
                              : "is being added to the Metadata...");

  // If the instance is not on the Metadata, we must add it.
  if (!is_instance_on_md) {
    // Get the Instance_definition of the target instance to be added to the
    // MD
    Instance_definition instance_def;
    std::string target_instance_canonical_hostname;
    {
      instance_def = query_instance_info(*m_target_instance);

      // Set the label
      if (!m_instance_label.is_null())
        instance_def.label = m_instance_label.get_safe();

      // Get the canonical hostname
      target_instance_canonical_hostname =
          m_target_instance->get_canonical_hostname();

      // Set the GR address endpoint
      instance_def.grendpoint = *m_gr_opts.local_address;
    }

    m_replicaset.add_instance_metadata(&instance_def,
                                       target_instance_canonical_hostname);
    m_replicaset.get_cluster()
        ->get_metadata_storage()
        ->update_instance_attribute(instance_def.uuid,
                                    k_instance_attribute_join_time,
                                    shcore::Value(join_begin_time));

    // Store the username in the Metadata instances table
    if (owns_repl_user) {
      handle_recovery_account();
    }
  }

  // Get the gr_address of the instance being added
  std::string added_instance_gr_address = *m_gr_opts.local_address;

  // Create a configuration object for the replicaset, ignoring the added
  // instance, to update the remaining replicaset members.
  // NOTE: only members that already belonged to the cluster and are either
  //       ONLINE or RECOVERING will be considered.
  std::vector<std::string> ignore_instances_vec = {m_address_in_metadata};
  std::unique_ptr<mysqlshdk::config::Config> replicaset_cfg =
      m_replicaset.create_config_object(ignore_instances_vec, true);

  // Update the group_replication_group_seeds of the replicaset members
  // by adding the gr_local_address of the instance that was just added.
  log_debug("Updating Group Replication seeds on all active members...");
  mysqlshdk::gr::update_group_seeds(replicaset_cfg.get(),
                                    added_instance_gr_address,
                                    mysqlshdk::gr::Gr_seeds_change_type::ADD);
  replicaset_cfg->apply();

  // Increase the replicaset_count counter
  replicaset_count++;

  // Auto-increment values must be updated according to:
  //
  // Set auto-increment for single-primary topology:
  // - auto_increment_increment = 1
  // - auto_increment_offset = 2
  //
  // Set auto-increment for multi-primary topology:
  // - auto_increment_increment = n;
  // - auto_increment_offset = 1 + server_id % n;
  // where n is the size of the GR group if > 7, otherwise n = 7.
  //
  // We must update the auto-increment values in add_instance for 2
  // scenarios
  //   - Multi-primary Replicaset
  //   - Replicaset that has 7 or more members after the add_instance
  //     operation
  //
  // NOTE: in the other scenarios, the Add_instance operation is in charge of
  // updating auto-increment accordingly

  // Get the topology mode of the replicaSet
  mysqlshdk::gr::Topology_mode topology_mode =
      m_replicaset.get_cluster()
          ->get_metadata_storage()
          ->get_replicaset_topology_mode(m_replicaset.get_id());

  if (topology_mode == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY &&
      replicaset_count > 7) {
    log_debug("Updating auto-increment settings on all active members...");
    mysqlshdk::gr::update_auto_increment(
        replicaset_cfg.get(), mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);
    replicaset_cfg->apply();
  }

  // Re-issue the CHANGE MASTER command
  // See note in handle_recovery_account()
  // NOTE: if waitRecover is zero we cannot do it since distributed recovery may
  // be running and the change master command will fail as the slave io thread
  // is running
  if (owns_repl_user && m_progress_style != Recovery_progress_style::NOWAIT) {
    update_change_master();
  }

  log_debug("Instance add finished");

  // TODO(pjesus): remove the 'if (!m_seed_instance)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_seed_instance) {
    console->print_info("The instance '" + m_instance_address +
                        "' was successfully added to the cluster.");
    console->println();
  }

  return shcore::Value();
}

void Add_instance::rollback() {
  // Clean (drop) the replication user (if created).
  clean_replication_user();
}

void Add_instance::finish() {
  try {
    // Check if group_replication_clone_threshold must be restored.
    // This would only be needed if the clone failed and the server didn't
    // restart.
    if (m_restore_clone_threshold != 0) {
      // TODO(miguel): 'start group_replication' returns before reading the
      // threshold value so we can have a race condition. We should wait until
      // the instance is 'RECOVERING'
      log_debug("Restoring value of group_replication_clone_threshold to: %s.",
                std::to_string(m_restore_clone_threshold).c_str());

      // If -1 we must restore it's default, otherwise we restore the initial
      // value
      if (m_restore_clone_threshold == -1) {
        m_target_instance->set_sysvar_default(
            "group_replication_clone_threshold");
      } else {
        m_target_instance->set_sysvar("group_replication_clone_threshold",
                                      m_restore_clone_threshold);
      }
    }
  } catch (const mysqlshdk::db::Error &e) {
    log_info(
        "Could not restore value of group_replication_clone_threshold: %s. "
        "Not "
        "a fatal error.",
        e.what());
  }

  // Close the instance and peer session at the end if available.
  if (!m_reuse_session_for_target_instance && m_target_instance) {
    m_target_instance->close_session();
  }

  if (!m_use_cluster_session_for_peer && m_peer_instance) {
    m_peer_instance->close_session();
  }
}

void Add_instance::refresh_target_connections() {
  {
    try {
      m_target_instance->query("SELECT 1");
    } catch (const mysqlshdk::db::Error &err) {
      auto e = shcore::Exception::mysql_error_with_code_and_state(
          err.what(), err.code(), err.sqlstate());

      if (CR_SERVER_LOST == e.code()) {
        log_debug(
            "Target instance connection lost: %s. Re-establishing a "
            "connection.",
            e.format().c_str());

        m_target_instance->get_session()->connect(m_instance_cnx_opts);
      } else {
        throw;
      }
    }

    try {
      m_replicaset.get_cluster()->get_metadata_storage()->get_session()->query(
          "SELECT 1");
    } catch (const mysqlshdk::db::Error &err) {
      auto e = shcore::Exception::mysql_error_with_code_and_state(
          err.what(), err.code(), err.sqlstate());

      if (CR_SERVER_LOST == e.code()) {
        log_debug(
            "Metadata connection lost: %s. Re-establishing a "
            "connection.",
            e.format().c_str());

        m_replicaset.get_cluster()
            ->get_metadata_storage()
            ->get_session()
            ->connect(m_replicaset.get_cluster()
                          ->get_metadata_storage()
                          ->get_session()
                          ->get_connection_options());
      } else {
        throw;
      }
    }
  }
}

}  // namespace dba
}  // namespace mysqlsh
