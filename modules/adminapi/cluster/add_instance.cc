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

#include "modules/adminapi/cluster/add_instance.h"

#include "adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/connectivity_check.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/server_features.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/debug.h"

namespace mysqlsh::dba::cluster {

void Add_instance::check_cluster_members_limit() const {
  const std::vector<Instance_metadata> all_instances =
      m_cluster_impl->get_metadata_storage()->get_all_instances(
          m_cluster_impl->get_id());

  if (all_instances.size() >= mysqlsh::dba::k_group_replication_members_limit) {
    auto console = mysqlsh::current_console();
    console->print_error(
        "Cannot join instance '" + m_target_instance->descr() +
        "' to cluster: InnoDB Cluster maximum limit of " +
        std::to_string(mysqlsh::dba::k_group_replication_members_limit) +
        " members has been reached.");
    throw shcore::Exception::runtime_error(
        "InnoDB Cluster already has the maximum number of " +
        std::to_string(mysqlsh::dba::k_group_replication_members_limit) +
        " members.");
  }
}

void Add_instance::resolve_local_address(
    checks::Check_type check_type, Group_replication_options *gr_options,
    const Group_replication_options &user_gr_options) {
  auto hostname = m_target_instance->get_canonical_hostname();
  auto communication_stack =
      get_communication_stack(*m_cluster_impl->get_cluster_server());

  bool check_if_busy =
      check_type == checks::Check_type::REJOIN ? false : !m_already_member;
  bool quiet = (check_type == checks::Check_type::REJOIN);

  gr_options->local_address = mysqlsh::dba::resolve_gr_local_address(
      user_gr_options.local_address.value_or("").empty()
          ? gr_options->local_address
          : user_gr_options.local_address,
      communication_stack, hostname, *m_target_instance->get_sysvar_int("port"),
      check_if_busy, quiet);

  // Validate that the local_address value we want to use as well as the
  // local address values in use on the cluster are compatible with the
  // version of the instance being added to the cluster.
  cluster_topology_executor_ops::validate_local_address_ip_compatibility(
      m_target_instance, gr_options->local_address.value_or(""),
      gr_options->group_seeds.value_or(""),
      m_cluster_impl->get_lowest_instance_version());
}

void Add_instance::check_and_resolve_instance_configuration(
    checks::Check_type check_type, bool is_switching_comm_stack) {
  Group_replication_options user_options(m_gr_opts);

  bool using_clone_recovery = !m_clone_opts.recovery_method
                                  ? false
                                  : m_clone_opts.recovery_method.get_safe() ==
                                        Member_recovery_method::CLONE;

  // Check instance version compatibility with cluster.
  m_cluster_impl->check_instance_configuration(
      m_target_instance, using_clone_recovery, check_type, m_already_member);

  // Resolve the SSL Mode to use to configure the instance.
  if (m_primary_instance) {
    resolve_instance_ssl_mode_option(*m_target_instance, *m_primary_instance,
                                     &m_gr_opts.ssl_mode);
    log_info("SSL mode used to configure the instance: '%s'",
             to_string(m_gr_opts.ssl_mode).c_str());
  }

  if (check_type == checks::Check_type::REJOIN) {
    // Read actual GR configurations to preserve them when rejoining the
    // instance.
    m_gr_opts.read_option_values(*m_target_instance, is_switching_comm_stack);
  }

  // If this is not seed instance, then we should try to read the
  // failoverConsistency and expelTimeout values from a a cluster member.
  m_cluster_impl->query_group_wide_option_values(m_target_instance.get(),
                                                 &m_gr_opts.consistency,
                                                 &m_gr_opts.expel_timeout);

  // Compute group_seeds using local_address from each active cluster member
  if (check_type == checks::Check_type::JOIN) {
    m_gr_opts.group_seeds = m_cluster_impl->get_cluster_group_seeds_list();
  } else {
    m_gr_opts.group_seeds = m_cluster_impl->get_cluster_group_seeds_list(
        m_target_instance->get_uuid());
  }

  // Resolve and validate GR local address.
  // NOTE: Must be done only after getting the report_host used by GR and for
  //       the metadata;
  resolve_local_address(check_type, &m_gr_opts, user_options);
}

void Add_instance::refresh_target_connections() const {
  try {
    m_target_instance->reconnect_if_needed("Target");
  } catch (const shcore::Error &err) {
    auto cluster_coptions =
        m_cluster_impl->get_cluster_server()->get_connection_options();

    if (err.code() == ER_ACCESS_DENIED_ERROR &&
        m_target_instance->get_connection_options().get_user() !=
            cluster_coptions.get_user()) {
      // try again with cluster credentials
      auto copy = m_target_instance->get_connection_options();
      copy.set_login_options_from(cluster_coptions);
      m_target_instance->get_session()->connect(copy);
      m_target_instance->prepare_session();
    } else {
      throw;
    }
  }

  m_cluster_impl->refresh_connections();
}

void Add_instance::store_local_replication_account() const {
  // Insert the recovery account created for the joining member in all
  // cluster instances and also itself
  //
  // This is required when using clone with waitRecovery zero and the commStack
  // in use by the Cluster is MySQL.
  // The reason is that with this commStack when an instance joins the Cluster
  // the recovery account created for it is set to be used by the replication
  // channel in all Cluster members and itself. Only when the whole joining
  // process finishes the command can reconfigure the recovery account in every
  // single active member of the cluster to use its own GR recovery replication
  // credentials and not the one that was created for the joining instance.
  // However, when waitRecovery is zero this cannot be done. To avoid
  // removeInstance() dropping an account still in use, we must insert it in the
  // MD schema for every cluster member (similar to what's done in
  // store_cloned_replication_account()).
  log_debug("Adding joining member recovery account to metadata");
  m_cluster_impl->execute_in_members(
      [this](const std::shared_ptr<Instance> &instance,
             const Cluster_impl::Instance_md_and_gr_member &info) {
        if (info.second.state == mysqlshdk::gr::Member_state::ONLINE) {
          try {
            m_cluster_impl->get_metadata_storage()
                ->update_instance_repl_account(
                    instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
                    Replica_type::GROUP_MEMBER,
                    m_gr_opts.recovery_credentials->user, m_account_host);
          } catch (const std::exception &) {
            log_info(
                "Failed updating Metadata of '%s' to include the recovery "
                "account of the joining member",
                instance->descr().c_str());
          }
        }
        return true;
      },
      [](const shcore::Error &,
         const Cluster_impl::Instance_md_and_gr_member &) { return true; });

  m_cluster_impl->get_metadata_storage()->update_instance_repl_account(
      m_target_instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
      Replica_type::GROUP_MEMBER, m_gr_opts.recovery_credentials->user,
      m_account_host);
}

void Add_instance::store_cloned_replication_account() const {
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
    clone to ensure the Metadata Schema for each instance holds the user used by
    GR. On top of that, we will make sure that the account is not dropped if any
    other cluster member is using it.

    The approach to tackle the usage of the recovery accounts is to store them
    in the Metadata, i.e. in addInstance() the user created in the "donor" is
    stored in the Metadata table referring to the instance being added.

    In removeInstance(), the Metadata is verified to check whether the recovery
    user of that instance is registered for more than one instance and if that's
    the case then it won't be dropped.

    createCluster() @ instance1:
      1) create recovery acct: foo1@%
      2) insert into MD for instance1: foo1
      3) start GR

    addInstance(instance2):
      1) create recovery acct on primary: foo2@%
      2) insert into MD for instance2: foo1@%
      3) clone
      4) change master to use foo2@%
      5) update MD for instance2 with user: foo2@%
   */

  // Get the "donor" recovery account
  auto md_inst = m_cluster_impl->get_metadata_storage()->get_md_server();

  std::string recovery_user, recovery_user_host;
  std::tie(recovery_user, recovery_user_host) =
      m_cluster_impl->get_metadata_storage()->get_instance_repl_account(
          md_inst->get_uuid(), Cluster_type::GROUP_REPLICATION,
          Replica_type::GROUP_MEMBER);

  // Insert the "donor" recovery account into the MD of the target instance
  log_debug("Adding donor recovery account to metadata");
  m_cluster_impl->get_metadata_storage()->update_instance_repl_account(
      m_target_instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
      Replica_type::GROUP_MEMBER, recovery_user, recovery_user_host);
}

void Add_instance::restore_group_replication_account() const {
  // See note in store_cloned_replication_account()
  std::string source_term = mysqlshdk::mysql::get_replication_source_keyword(
      m_target_instance->get_version());
  log_debug("Re-issuing the CHANGE %s command", source_term.c_str());

  mysqlshdk::mysql::Replication_credentials_options options;
  options.password = m_gr_opts.recovery_credentials->password.value_or("");

  mysqlshdk::mysql::change_replication_credentials(
      *m_target_instance, mysqlshdk::gr::k_gr_recovery_channel,
      m_gr_opts.recovery_credentials->user, options);

  // Update the recovery account to the right one
  log_debug("Adding recovery account to metadata");
  m_cluster_impl->get_metadata_storage()->update_instance_repl_account(
      m_target_instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
      Replica_type::GROUP_MEMBER, m_gr_opts.recovery_credentials->user,
      m_account_host);
}

void Add_instance::prepare(checks::Check_type check_type,
                           bool *is_switching_comm_stack) {
  assert(check_type == checks::Check_type::JOIN ||
         check_type == checks::Check_type::REJOIN);

  bool add_instance = check_type == checks::Check_type::JOIN ? true : false;

  // Validations and variables initialization
  m_primary_instance = m_cluster_impl->get_cluster_server();

  // Validate the GR options.
  // Note: If the user provides no group_seeds value, it is automatically
  // assigned a value with the local_address values of the existing cluster
  // members and those local_address values are already validated on the
  // validate_local_address_ip_compatibility method, so we only need to
  // validate the group_seeds value provided by the user.
  m_gr_opts.check_option_values(m_target_instance->get_version(),
                                m_target_instance->get_canonical_port());

  m_gr_opts.manual_start_on_boot =
      m_cluster_impl->get_manual_start_on_boot_option();

  // Validate the Clone options.
  m_clone_opts.check_option_values(m_target_instance->get_version(), false);

  if (add_instance) {
    check_cluster_members_limit();
  }

  // Validate the options used
  cluster_topology_executor_ops::validate_add_rejoin_options(
      m_gr_opts, get_communication_stack(*m_primary_instance));

  // Make sure there isn't some leftover auto-rejoin active
  m_is_autorejoining = cluster_topology_executor_ops::is_member_auto_rejoining(
      m_target_instance);

  // Make sure the target instance does not already belong to a cluster.
  // Unless it's our cluster, in that case we keep adding it since it
  // may be a retry.
  if (add_instance) {
    try {
      mysqlsh::dba::checks::ensure_instance_not_belong_to_cluster(
          m_target_instance, m_primary_instance, m_cluster_impl->get_id());
    } catch (const shcore::Exception &exp) {
      m_already_member =
          (exp.code() == SHERR_DBA_ASYNC_MEMBER_INCONSISTENT) ||
          (exp.code() == SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER);

      if (exp.code() == SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER) {
        throw shcore::Exception::runtime_error(
            "The instance '" + m_target_instance->descr() +
            "' is already part of this InnoDB Cluster");
      }

      if (!m_already_member) throw;
    }
  }

  // Verify whether the instance supports the communication stack in use in
  // the Cluster and set it in the options handler
  m_comm_stack = get_communication_stack(*m_primary_instance);

  cluster_topology_executor_ops::check_comm_stack_support(
      m_target_instance, &m_gr_opts, m_comm_stack);

  // Check if the Cluster's communication stack has switched
  if (!add_instance) {
    if (is_switching_comm_stack && !*is_switching_comm_stack) {
      std::string persisted_comm_stack =
          m_target_instance
              ->get_sysvar_string("group_replication_communication_stack")
              .value_or(kCommunicationStackXCom);

      if (persisted_comm_stack != m_comm_stack) {
        *is_switching_comm_stack = true;
      }
    }

    // get the cert subject to use (we're in a rejoin, so it should be there,
    // but check it anyway)
    m_auth_cert_subject =
        m_cluster_impl->query_cluster_instance_auth_cert_subject(
            *m_target_instance);

    switch (auto auth_type = m_cluster_impl->query_cluster_auth_type();
            auth_type) {
      case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT:
      case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT_PASSWORD:
        if (m_auth_cert_subject.empty()) {
          current_console()->print_error(shcore::str_format(
              "The cluster's SSL mode is set to '%s' but the instance being "
              "rejoined doesn't have a valid 'certSubject' option. Please stop "
              "GR on that instance and then add it back using "
              "Cluster.<<<addInstance>>>() with the appropriate authentication "
              "options.",
              to_string(auth_type).c_str()));

          throw shcore::Exception(
              shcore::str_format(
                  "The cluster's SSL mode is set to '%s' but the 'certSubject' "
                  "option for the instance isn't valid.",
                  to_string(auth_type).c_str()),
              SHERR_DBA_MISSING_CERT_OPTION);
        }
        break;
      default:
        break;
    }
  }

  // Pick recovery method and check if it's actually usable
  // Done last to not request interaction before validations
  m_clone_opts.recovery_method = m_cluster_impl->check_recovery_method(
      *m_target_instance,
      add_instance ? Member_op_action::ADD_INSTANCE
                   : Member_op_action::REJOIN_INSTANCE,
      *m_clone_opts.recovery_method, m_options.interactive());

  // Check for various instance specific configuration issues
  check_and_resolve_instance_configuration(
      check_type, is_switching_comm_stack && *is_switching_comm_stack);

  if (add_instance) {
    // Validate localAddress
    {
      // reaching this point, localAddress must have been resolved / generated
      assert(m_gr_opts.local_address.has_value());

      validate_local_address_option(*m_gr_opts.local_address,
                                    m_gr_opts.communication_stack.value_or(""),
                                    m_target_instance->get_canonical_port());

      validate_ip_allow_list(
          *m_target_instance, m_gr_opts.ip_allowlist.value_or("AUTOMATIC"),
          m_gr_opts.local_address.value_or(""), m_comm_stack, false);
    }

    // recovery auth checks
    auto auth_type = m_cluster_impl->query_cluster_auth_type();

    m_auth_cert_subject = m_options.cert_subject;

    // check if member auth request mode is supported
    validate_instance_member_auth_type(*m_target_instance, false,
                                       m_gr_opts.ssl_mode, "memberSslMode",
                                       auth_type);

    // check if certSubject was correctly supplied
    validate_instance_member_auth_options("cluster", false, auth_type,
                                          m_auth_cert_subject);

    // Check connectivity and SSL
    if (current_shell_options()->get().dba_connectivity_checks) {
      current_console()->print_info(
          "* Checking connectivity and SSL configuration...");

      auto cert_issuer = m_cluster_impl->query_cluster_auth_cert_issuer();
      auto instance_cert_subject =
          m_cluster_impl->query_cluster_instance_auth_cert_subject(
              m_primary_instance->get_uuid());

      auto gr_local_address =
          m_primary_instance
              ->get_sysvar_string("group_replication_local_address")
              .value_or("");

      mysqlshdk::mysql::Set_variable disable_sro(
          *m_target_instance, "super_read_only", false, true);

      test_peer_connection(
          *m_target_instance, m_gr_opts.local_address.value_or(""),
          m_options.cert_subject, *m_primary_instance, gr_local_address,
          instance_cert_subject, m_gr_opts.ssl_mode, auth_type, cert_issuer,
          m_comm_stack);

      current_console()->print_info();
    }
  }

  // Set the transaction size limit
  m_gr_opts.transaction_size_limit =
      get_transaction_size_limit(*m_primary_instance);

  // Set the paxos_single_leader option
  // Verify whether the primary supports it, meaning it's in use. Checking if
  // the target supports it is not valid since the target might be 8.0 and the
  // primary 5.7
  if (supports_paxos_single_leader(m_primary_instance->get_version())) {
    m_gr_opts.paxos_single_leader =
        get_paxos_single_leader_enabled(*m_primary_instance).value_or(false);
  }
}

void Add_instance::do_run() {
  // Validations and variables initialization
  prepare(checks::Check_type::JOIN);

  /**
   * Execute the add_instance command.
   * More specifically:
   * - Log used GR options/settings;
   * - Handle creation of recovery (replication) user;
   * - Install GR plugin (if needed);
   * - If seed instance: start cluster (bootstrap GR);
   * - If not seed instance: join cluster;
   * - Add instance to Metadata (if needed);
   * - Update GR group seeds on cluster members;
   * - Update auto-increment setting in cluster members;
   *
   * bootstrap and rejoin will skip one-time preparations, but configurations
   * will be reset.
   */
  auto console = mysqlsh::current_console();

  {
    std::string msg =
        "A new instance will be added to the InnoDB Cluster. Depending on "
        "the amount of data on the cluster this might take from a few "
        "seconds to several hours.";
    std::string message =
        mysqlshdk::textui::format_markup_text({msg}, 80, 0, false);
    console->print_info(message);
    console->print_info();
  }

  // Set the internal configuration object: read/write configs from the
  // server.
  auto cfg = mysqlsh::dba::create_server_config(
      m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);

  // Common informative logging
  cluster_topology_executor_ops::log_used_gr_options(m_gr_opts);

  console->print_info("Adding instance to the cluster...");
  console->print_info();

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  mysqlshdk::gr::install_group_replication_plugin(*m_target_instance, nullptr);

  // Validate group_replication_gtid_assignment_block_size. Its value must be
  // the same on the instance as it is on the cluster but can only be checked
  // after the GR plugin is installed. This check is also done on the rejoin
  // operation which covers the rejoin and rebootCluster operations.
  m_cluster_impl->validate_variable_compatibility(
      *m_target_instance, "group_replication_gtid_assignment_block_size");

  // Note: We used to auto-downgrade the GR protocol version if the joining
  // member is too old, but that's not allowed for other reasons anyway

  // Get the current number of cluster members
  uint64_t cluster_member_count =
      m_cluster_impl->get_metadata_storage()->get_cluster_size(
          m_cluster_impl->get_id());

  // Clone handling
  bool clone_supported =
      mysqlshdk::mysql::is_clone_available(*m_target_instance);
  int64_t restore_clone_threshold = 0;
  bool restore_recovery_accounts = false;

  if (clone_supported) {
    restore_clone_threshold = m_cluster_impl->prepare_clone_recovery(
        *m_target_instance,
        *m_clone_opts.recovery_method == Member_recovery_method::CLONE);
  }

  // Handle the replication user creation.
  auto owns_repl_user = std::invoke([this](void) {
    // Creates the replication user ONLY if not already given.
    // NOTE: User always created at the seed instance.
    if (m_gr_opts.recovery_credentials &&
        !m_gr_opts.recovery_credentials->user.empty()) {
      return false;
    }

    std::tie(m_gr_opts.recovery_credentials, m_account_host) =
        m_cluster_impl->create_replication_user(
            m_target_instance.get(), m_auth_cert_subject,
            Replication_account_type::GROUP_REPLICATION);

    return true;
  });

  // enable skip_slave_start if the cluster belongs to a ClusterSet, and
  // configure the managed replication channel if the cluster is a replica.
  if (m_cluster_impl->is_cluster_set_member()) {
    m_cluster_impl->configure_cluster_set_member(m_target_instance);
  }

  const auto post_failure_actions = [this, &cfg](bool owns_rpl_user,
                                                 bool drop_rpl_user,
                                                 int64_t restore_clone_thrsh,
                                                 bool restore_rec_accounts) {
    assert(restore_clone_thrsh >= -1);

    bool credentials_exist = m_gr_opts.recovery_credentials &&
                             !m_gr_opts.recovery_credentials->user.empty();

    if (owns_rpl_user && drop_rpl_user && credentials_exist) {
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
            "Could not restore value of group_replication_clone_threshold: %s. "
            "Not a fatal error.",
            e.what());
      }
    }

    // Restore remaining variables
    {
      auto srv_cfg = dynamic_cast<mysqlshdk::config::Config_server_handler *>(
          cfg->get_handler(mysqlshdk::config::k_dft_cfg_server_handler));

      if (srv_cfg) {
        srv_cfg->remove_from_undo("group_replication_clone_threshold");
        srv_cfg->undo_changes();
      }
    }
  };

  try {
    // we need a point in time as close as possible, but still earlier than
    // when recovery starts to monitor the recovery phase. The timestamp
    // resolution is timestamp(3) irrespective of platform
    auto join_begin_time =
        m_target_instance->queryf_one_string(0, "", "SELECT NOW(3)");

    bool recovery_certificates{false};
    bool recovery_needs_password{true};
    switch (m_cluster_impl->query_cluster_auth_type()) {
      case mysqlsh::dba::Replication_auth_type::CERT_ISSUER:
      case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT:
        recovery_needs_password = false;
        [[fallthrough]];
      case mysqlsh::dba::Replication_auth_type::CERT_ISSUER_PASSWORD:
      case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT_PASSWORD:
        recovery_certificates = true;
        break;
      default:
        break;
    }

    {
      if (m_already_member) {
        // START GROUP_REPLICATION is the last thing that happens in
        // join_cluster, so if it's already running, we don't need to do that
        // again
        log_info("%s is already a group member, skipping join",
                 m_target_instance->descr().c_str());
      } else {
        log_info(
            "Joining '%s' to cluster using account '%s' to peer '%s'.",
            m_target_instance->descr().c_str(),
            m_primary_instance->get_connection_options().get_user().c_str(),
            m_primary_instance->descr().c_str());

        // If using 'MySQL' communication stack, the account must already
        // exist in the joining member since authentication is based on MySQL
        // credentials so the account must exist in both ends before GR
        // starts. For those reasons, we must create the account with binlog
        // disabled before starting GR, otherwise we'd create errant
        // transaction
        if (m_comm_stack == kCommunicationStackMySQL) {
          // NOTE: if the recovery users use certificates, we can't change GR's
          // recovery replication credentials in all possible donors. That would
          // simply fail, because in any given donor other than the new
          // instance, using the recovery user of the new instance will fail
          // (because the certificate only exists on the new instance server).
          m_cluster_impl->create_local_replication_user(
              m_target_instance, m_options.cert_subject, m_gr_opts,
              !recovery_certificates);

          // if recovery accounts need certificates, we must ensure that the
          // recovery accounts of all members also exist on the target instance
          if (recovery_certificates) {
            m_cluster_impl->create_replication_users_at_instance(
                m_target_instance);
          }

          // At this point, all Cluster members got the recovery credentials
          // changed (if certificates aren't needed), to be able to use the
          // account created for the joining member so in case of failure, we
          // must restore all
          restore_recovery_accounts = !recovery_certificates;

          DBUG_EXECUTE_IF("fail_add_instance_mysql_stack", {
            // Throw an exception to simulate a failure at
            // this point in addInstance()
            throw std::logic_error("debug");
          });
        } else if (recovery_certificates && (restore_clone_threshold > 0)) {
          // reaching this point, we're using 'XCOM' communication stack, the
          // recovery users use certificates and the recovery process will use
          // clone. This means that, at the end of the clone, GR in the new
          // instance will use the recovery user from the seed instance, which
          // will fail because of the certificates. To solve this, we kind of
          // imitate the behavior of the MySQL comm stack: we create the new
          // replication user on all existing members, change the recovery
          // credentials on them to use the new user (so that the clone leaves
          // the new instance configured properly) and then revert everything
          // back
          m_cluster_impl->create_local_replication_user(
              m_target_instance, m_options.cert_subject, m_gr_opts, true);

          restore_recovery_accounts = true;
        }

        DBUG_EXECUTE_IF("dba_revert_trigger_exception", {
          throw shcore::Exception("Exception while adding instance.", 0);
        });

        // Join the instance to the Group Replication group.
        mysqlsh::dba::join_cluster(*m_target_instance, *m_primary_instance,
                                   m_gr_opts, recovery_certificates,
                                   cluster_member_count, cfg.get());
      }

      // Wait until recovery done. Will throw an exception if recovery fails.
      m_cluster_impl->wait_instance_recovery(
          *m_target_instance, join_begin_time, m_options.get_wait_recovery());

      // When clone is used, the target instance will restart and all
      // connections are closed so we need to test if the connection to the
      // target instance and MD are closed and re-open if necessary
      refresh_target_connections();
    }

    // Get the address used by GR for the added instance (used in MD).
    std::string address_in_metadata =
        m_target_instance->get_canonical_address();

    // Check if instance address already belong to cluster (metadata).
    bool is_instance_on_md =
        m_cluster_impl->contains_instance_with_address(address_in_metadata);

    log_debug("Cluster %s: Instance '%s' %s",
              m_cluster_impl->get_name().c_str(),
              m_target_instance->descr().c_str(),
              is_instance_on_md ? "is already in the Metadata."
                                : "is being added to the Metadata...");

    // at this point, GR is already running, so if something wrong happens we
    // don't want to clear any variables
    {
      auto srv_cfg = dynamic_cast<mysqlshdk::config::Config_server_handler *>(
          cfg->get_handler(mysqlshdk::config::k_dft_cfg_server_handler));

      if (srv_cfg) {
        srv_cfg->remove_all_from_undo();
      }
    }

    MetadataStorage::Transaction trx(m_cluster_impl->get_metadata_storage());

    // If the instance is not on the Metadata, we must add it.
    if (!is_instance_on_md) {
      m_cluster_impl->add_metadata_for_instance(*m_target_instance,
                                                Instance_type::GROUP_MEMBER,
                                                m_options.label.value_or(""));
      m_cluster_impl->get_metadata_storage()->update_instance_attribute(
          m_target_instance->get_uuid(), k_instance_attribute_join_time,
          shcore::Value(join_begin_time));

      m_cluster_impl->get_metadata_storage()->update_instance_attribute(
          m_target_instance->get_uuid(), k_instance_attribute_cert_subject,
          shcore::Value(m_options.cert_subject));

      // Store the username in the Metadata instances table
      if (owns_repl_user) {
        if (m_comm_stack == kCommunicationStackMySQL &&
            m_options.get_wait_recovery() == Recovery_progress_style::NOWAIT) {
          store_local_replication_account();
        } else {
          store_cloned_replication_account();
        }
      }
    }

    // Update group_seeds and auto_increment in other members
    m_cluster_impl->update_group_peers(*m_target_instance, m_gr_opts,
                                       cluster_member_count,
                                       address_in_metadata);

    // Re-issue the CHANGE MASTER command
    // See note in store_cloned_replication_account()
    // NOTE: if waitRecover is zero we cannot do it since distributed recovery
    // may be running and the change master command will fail as the slave io
    // thread is running
    if (owns_repl_user &&
        m_options.get_wait_recovery() != Recovery_progress_style::NOWAIT) {
      restore_group_replication_account();
    }

    // Reconfigure the recovery account in every single active member of the
    // cluster to use its own GR recovery replication credentials and
    // not the one that was created for the joining instance.
    //
    // A joining member needs to be able to connect to itself and to all
    // cluster members, as well as all cluster members need to be able to
    // connect to the joining member.
    //
    // 1) Group is bootstrapped on server1.
    //  create recovery account:  mysql_innodb_cluster_1
    //  change master to use mysql_innodb_cluster_1
    //
    // 2) server2 is added to the group
    //    create recovery account at server 2 and group:
    //    mysql_innodb_cluster_2 change master at server 2 to use
    //    mysql_innodb_cluster_2 change master at server seed (server 1) to
    //    use mysql_innodb_cluster_2 start GR, join OK change master at seed
    //    (server1) to use mysql_innodb_cluster_1
    //
    // 3) server3 is added to the group
    //    create recovery account at server 3 and group:
    //    mysql_innodb_cluster_3 change master at server 3 to use
    //    mysql_innodb_cluster_3 change master at all online members (server
    //    1, server 2) to use mysql_innodb_cluster_3 start GR, join OK change
    //    master at server1 to use mysql_innodb_cluster_1 change master at
    //    server2 to use mysql_innodb_cluster_2
    //
    // 4) server3 is removed from the group
    //      mysql_innodb_cluster_3 account is dropped on all group servers.
    //
    // 5) server1 goes offline and restarts
    //  recovery kicks-in, using mysql_innodb_cluster_1 -> OK
    //
    // NOTE: if waitRecovery is zero we cannot do it because
    // restore_recovery_account_all_members() will happen before clone has
    // finished so the credentials for the replication channel (that are
    // cloned to the instance joining) won't match since they were already
    // reset in the Cluster by restore_recovery_account_all_members().
    //
    // Another scenario to take into account (also detailed above), where in
    // case of XCOM comm stack, the recovery users needing certificates and
    // using clone, all members are currently using the new recovery user, which
    // means that we need to restore them all back to their corresponding users.
    {
      auto restore_accounts =
          (m_comm_stack == kCommunicationStackMySQL) &&
          !recovery_certificates &&
          (m_options.get_wait_recovery() != Recovery_progress_style::NOWAIT);
      restore_accounts |= (m_comm_stack != kCommunicationStackMySQL) &&
                          recovery_certificates &&
                          (restore_clone_threshold > 0);
      if (restore_accounts)
        m_cluster_impl->restore_recovery_account_all_members(
            recovery_needs_password);
    }

    // Only commit transaction once everything is done, so that things don't
    // look all fine if something fails in the middle
    trx.commit();

    log_debug("Instance add finished");

    console->print_info("The instance '" + m_target_instance->descr() +
                        "' was successfully added to the cluster.");
    console->print_info();
  } catch (const shcore::Exception &error) {
    bool drop_replication_user = false;
    // Do not remove the replication account if there was a timeout waiting
    // for clone to complete, otherwise, whenever restarting the instance to
    // complete the process it will fail since the account doesn't exist
    // anymore. This is more impacting when using 'MySQL' communication stack
    if (error.code() != SHERR_DBA_SERVER_RESTART_TIMEOUT) {
      drop_replication_user = true;
    } else {
      // Do not restore the recovery accounts if the exception was due to a
      // timeout waiting for clone to complete, otherwise, restarting the
      // instance to complete the process fails
      restore_recovery_accounts = false;
    }

    post_failure_actions(owns_repl_user, drop_replication_user,
                         restore_clone_threshold, restore_recovery_accounts);

    throw;
  } catch (...) {
    post_failure_actions(owns_repl_user, false, restore_clone_threshold,
                         restore_recovery_accounts);

    throw;
  }
}
}  // namespace mysqlsh::dba::cluster
