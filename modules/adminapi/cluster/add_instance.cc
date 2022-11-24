/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh::dba::cluster {

namespace {
constexpr std::chrono::seconds k_recovery_start_timeout{30};
}

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
    Group_replication_options *gr_options,
    const Group_replication_options &user_gr_options) {
  auto hostname = m_target_instance->get_canonical_hostname();
  auto communication_stack = m_cluster_impl->get_communication_stack();

  gr_options->local_address = mysqlsh::dba::resolve_gr_local_address(
      user_gr_options.local_address.value_or("").empty()
          ? gr_options->local_address
          : user_gr_options.local_address,
      communication_stack, hostname, *m_target_instance->get_sysvar_int("port"),
      !m_already_member, false);

  // Validate that the local_address value we want to use as well as the
  // local address values in use on the cluster are compatible with the
  // version of the instance being added to the cluster.
  cluster_topology_executor_ops::validate_local_address_ip_compatibility(
      m_target_instance, gr_options->local_address.value_or(""),
      gr_options->group_seeds.value_or(""),
      m_cluster_impl->get_lowest_instance_version());
}

Member_recovery_method Add_instance::check_recovery_method(
    bool clone_disabled) {
  // Check GTID consistency and whether clone is needed
  auto check_recoverable = [this](mysqlshdk::mysql::IInstance *target) {
    // Check if the GTIDs were purged from the whole cluster
    bool recoverable = false;

    m_cluster_impl->execute_in_members(
        {mysqlshdk::gr::Member_state::ONLINE}, target->get_connection_options(),
        {},
        [&recoverable, &target](const std::shared_ptr<Instance> &instance,
                                const mysqlshdk::gr::Member &) {
          // Get the gtid state in regards to the cluster_session
          mysqlshdk::mysql::Replica_gtid_state state =
              mysqlshdk::mysql::check_replica_gtid_state(*instance, *target,
                                                         nullptr, nullptr);

          if (state != mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE) {
            recoverable = true;
            // stop searching as soon as we find a possible donor
            return false;
          }
          return true;
        });

    return recoverable;
  };

  std::shared_ptr<Instance> donor_instance =
      m_cluster_impl->get_cluster_server();

  auto recovery_method = mysqlsh::dba::validate_instance_recovery(
      Cluster_type::GROUP_REPLICATION, Member_op_action::ADD_INSTANCE,
      donor_instance.get(), m_target_instance.get(), check_recoverable,
      *m_options.clone_options.recovery_method,
      m_cluster_impl->get_gtid_set_is_complete(), m_options.interactive(),
      clone_disabled);

  // If recovery method was selected as clone, check that there is at least one
  // ONLINE cluster instance not using IPV6, otherwise throw an error
  if (recovery_method == Member_recovery_method::CLONE) {
    bool found_ipv4 = false;
    std::string full_msg;
    // get online members
    const auto online_instances =
        m_cluster_impl->get_instances({mysqlshdk::gr::Member_state::ONLINE});
    // check if at least one of them is not IPv6, otherwise throw an error
    for (const auto &instance : online_instances) {
      if (!mysqlshdk::utils::Net::is_ipv6(
              mysqlshdk::utils::split_host_and_port(instance.endpoint).first)) {
        found_ipv4 = true;
        break;
      } else {
        std::string msg = "Instance '" + instance.endpoint +
                          "' is not a suitable clone donor: Instance "
                          "hostname/report_host is an IPv6 address, which is "
                          "not supported for cloning.";
        log_info("%s", msg.c_str());
        full_msg += msg + "\n";
      }
    }
    if (!found_ipv4) {
      auto console = current_console();
      console->print_error(
          "None of the ONLINE members in the cluster are compatible to be used "
          "as clone donors for " +
          m_target_instance->descr());
      console->print_info(full_msg);
      throw shcore::Exception("The Cluster has no compatible clone donors.",
                              SHERR_DBA_CLONE_NO_DONORS);
    }
  }

  return recovery_method;
}

void Add_instance::check_and_resolve_instance_configuration() {
  Group_replication_options user_options(m_options.gr_options);

  bool using_clone_recovery =
      !m_options.clone_options.recovery_method
          ? false
          : m_options.clone_options.recovery_method.get_safe() ==
                Member_recovery_method::CLONE;

  // Check instance version compatibility with cluster.
  m_cluster_impl->check_instance_configuration(
      m_target_instance, using_clone_recovery, checks::Check_type::JOIN,
      m_already_member);

  // Resolve the SSL Mode to use to configure the instance.
  if (m_primary_instance) {
    resolve_instance_ssl_mode_option(*m_target_instance, *m_primary_instance,
                                     &m_options.gr_options.ssl_mode);
    log_info("SSL mode used to configure the instance: '%s'",
             to_string(m_options.gr_options.ssl_mode).c_str());
  }

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
        group_seeds(m_cluster_impl->get_cluster_group_seeds());
  }

  // Resolve and validate GR local address.
  // NOTE: Must be done only after getting the report_host used by GR and for
  //       the metadata;
  resolve_local_address(&m_options.gr_options, user_options);
}

void Add_instance::handle_clone_plugin_state(bool enable_clone) const {
  // First, handle the target instance
  if (enable_clone) {
    log_info("Installing the clone plugin on instance '%s'.",
             m_target_instance->get_canonical_address().c_str());
    mysqlshdk::mysql::install_clone_plugin(*m_target_instance, nullptr);
  } else {
    log_info("Uninstalling the clone plugin on instance '%s'.",
             m_target_instance->get_canonical_address().c_str());
    mysqlshdk::mysql::uninstall_clone_plugin(*m_target_instance, nullptr);
  }

  // Then, handle the cluster members
  m_cluster_impl->setup_clone_plugin(enable_clone);
}

bool Add_instance::create_replication_user() {
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

void Add_instance::clean_replication_user() {
  if (m_options.gr_options.recovery_credentials &&
      !m_options.gr_options.recovery_credentials->user.empty()) {
    log_debug("Dropping recovery user '%s'@'%s' at instance '%s'.",
              m_options.gr_options.recovery_credentials->user.c_str(),
              m_account_host.c_str(), m_primary_instance->descr().c_str());
    m_primary_instance->drop_user(
        m_options.gr_options.recovery_credentials->user, m_account_host, true);
  }
}

void Add_instance::wait_recovery(const std::string &join_begin_time,
                                 Recovery_progress_style progress_style) const {
  auto console = current_console();
  try {
    auto post_clone_auth = m_target_instance->get_connection_options();
    post_clone_auth.set_login_options_from(
        m_cluster_impl->get_cluster_server()->get_connection_options());

    monitor_gr_recovery_status(
        m_target_instance->get_connection_options(), post_clone_auth,
        join_begin_time, progress_style, k_recovery_start_timeout.count(),
        current_shell_options()->get().dba_restart_wait_timeout);
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
  } catch (const restart_timeout &) {
    console->print_warning(
        "Clone process appears to have finished and tried to restart the "
        "MySQL server, but it has not yet started back up.");

    console->print_info();
    console->print_info(
        "Please make sure the MySQL server at '" + m_target_instance->descr() +
        "' is restarted and call <Cluster>.rescan() to complete the process. "
        "To increase the timeout, change "
        "shell.options[\"dba.restartWaitTimeout\"].");

    throw shcore::Exception("Timeout waiting for server to restart",
                            SHERR_DBA_SERVER_RESTART_TIMEOUT);
  }
}

void Add_instance::refresh_target_connections() const {
  try {
    m_target_instance->query("SELECT 1");
  } catch (const shcore::Error &err) {
    auto e = shcore::Exception::mysql_error_with_code(err.what(), err.code());

    if (CR_SERVER_LOST == e.code()) {
      log_debug(
          "Connection to instance '%s' lost: %s. Re-establishing a "
          "connection.",
          m_target_instance->get_canonical_address().c_str(),
          e.format().c_str());

      try {
        m_target_instance->get_session()->connect(
            m_target_instance->get_connection_options());
      } catch (const shcore::Error &ie) {
        auto cluster_coptions =
            m_cluster_impl->get_cluster_server()->get_connection_options();

        if (ie.code() == ER_ACCESS_DENIED_ERROR &&
            m_target_instance->get_connection_options().get_user() !=
                cluster_coptions.get_user()) {
          // try again with cluster credentials
          auto copy = m_target_instance->get_connection_options();
          copy.set_login_options_from(cluster_coptions);
          m_target_instance->get_session()->connect(copy);
        } else {
          throw;
        }
      }
      m_target_instance->prepare_session();
    } else {
      throw;
    }
  }

  try {
    m_cluster_impl->get_metadata_storage()->get_md_server()->query("SELECT 1");
  } catch (const shcore::Error &err) {
    auto e = shcore::Exception::mysql_error_with_code(err.what(), err.code());

    if (CR_SERVER_LOST == e.code()) {
      log_debug(
          "Metadata connection lost: %s. Re-establishing a "
          "connection.",
          e.format().c_str());

      auto md_server = m_cluster_impl->get_metadata_storage()->get_md_server();

      md_server->get_session()->connect(md_server->get_connection_options());
      md_server->prepare_session();
    } else {
      throw;
    }
  }
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
             const Instance_md_and_gr_member &info) {
        if (info.second.state == mysqlshdk::gr::Member_state::ONLINE) {
          try {
            m_cluster_impl->get_metadata_storage()
                ->update_instance_repl_account(
                    instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
                    m_options.gr_options.recovery_credentials->user,
                    m_account_host);
          } catch (const std::exception &e) {
            log_info(
                "Failed updating Metatada of '%s' to include the recovery "
                "account of the joining member",
                instance->descr().c_str());
          }
        }
        return true;
      },
      [](const shcore::Error &, const Instance_md_and_gr_member &) {
        return true;
      });

  m_cluster_impl->get_metadata_storage()->update_instance_repl_account(
      m_target_instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
      m_options.gr_options.recovery_credentials->user, m_account_host);
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
          md_inst->get_uuid(), Cluster_type::GROUP_REPLICATION);

  // Insert the "donor" recovery account into the MD of the target instance
  log_debug("Adding donor recovery account to metadata");
  m_cluster_impl->get_metadata_storage()->update_instance_repl_account(
      m_target_instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
      recovery_user, recovery_user_host);
}

void Add_instance::restore_group_replication_account() const {
  // See note in store_cloned_replication_account()
  std::string source_term = mysqlshdk::mysql::get_replication_source_keyword(
      m_target_instance->get_version());
  log_debug("Re-issuing the CHANGE %s command", source_term.c_str());

  mysqlshdk::mysql::change_replication_credentials(
      *m_target_instance, m_options.gr_options.recovery_credentials->user,
      m_options.gr_options.recovery_credentials->password.value_or(""),
      mysqlshdk::gr::k_gr_recovery_channel);

  // Update the recovery account to the right one
  log_debug("Adding recovery account to metadata");
  m_cluster_impl->get_metadata_storage()->update_instance_repl_account(
      m_target_instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
      m_options.gr_options.recovery_credentials->user, m_account_host);
}

void Add_instance::prepare() {
  // Validations and variables initialization
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
  m_options.clone_options.check_option_values(m_target_instance->get_version());

  check_cluster_members_limit();

  // Validate the options used
  cluster_topology_executor_ops::validate_add_rejoin_options(
      m_options.gr_options, m_cluster_impl->get_communication_stack());

  // Make sure there isn't some leftover auto-rejoin active
  cluster_topology_executor_ops::is_member_auto_rejoining(m_target_instance);

  // Make sure the target instance does not already belong to a cluster.
  // Unless it's our cluster, in that case we keep adding it since it
  // may be a retry.
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

  // Verify whether the instance supports the communication stack in use in
  // the Cluster and set it in the options handler
  m_comm_stack = m_cluster_impl->get_communication_stack();

  cluster_topology_executor_ops::check_comm_stack_support(
      m_target_instance, &m_options.gr_options, m_comm_stack);

  // Pick recovery method and check if it's actually usable
  // Done last to not request interaction before validatiopns
  m_options.clone_options.recovery_method =
      check_recovery_method(m_cluster_impl->get_disable_clone_option());

  // Check for various instance specific configuration issues
  check_and_resolve_instance_configuration();

  // Validate localAddress
  if (m_options.gr_options.local_address.has_value()) {
    validate_local_address_option(
        *m_options.gr_options.local_address,
        m_options.gr_options.communication_stack.value_or(""),
        m_target_instance->get_canonical_port());
  }

  // Set the transaction size limit
  m_options.gr_options.transaction_size_limit =
      m_cluster_impl->get_transaction_size_limit();
}

void Add_instance::do_run() {
  // Validations and variables initialization
  prepare();

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
  cluster_topology_executor_ops::log_used_gr_options(m_options.gr_options);

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
  bool clone_disabled = m_cluster_impl->get_disable_clone_option();
  bool clone_supported =
      mysqlshdk::mysql::is_clone_available(*m_target_instance);

  int64_t restore_clone_threshold = 0;
  bool restore_recovery_accounts = false;

  if (clone_supported) {
    // Clone must be uninstalled only when disableClone is enabled on the
    // Cluster
    bool enable_clone = !clone_disabled;

    // Force clone if requested
    if (*m_options.clone_options.recovery_method ==
        Member_recovery_method::CLONE) {
      restore_clone_threshold =
          mysqlshdk::mysql::force_clone(*m_target_instance);

      // RESET MASTER to clear GTID_EXECUTED in case it's diverged, otherwise
      // clone is not executed and GR rejects the instance
      m_target_instance->query("RESET MASTER");
    }

    // Make sure the Clone plugin is installed or uninstalled depending on the
    // value of disableClone
    //
    // NOTE: If the clone usage is not disabled on the cluster (disableClone),
    // we must ensure the clone plugin is installed on all members. Otherwise,
    // if the cluster was creating an Older shell (<8.0.17) or if the cluster
    // was set up using the Incremental recovery only and the primary is
    // removed (or a failover happened) the instances won't have the clone
    // plugin installed and GR's recovery using clone will fail.
    //
    // See BUG#29954085 and BUG#29960838
    handle_clone_plugin_state(enable_clone);
  }

  // Handle the replication user creation.
  bool owns_repl_user = create_replication_user();

  // enable skip_slave_start if the cluster belongs to a ClusterSet, and
  // configure the managed replication channel if the cluster is a replica.
  if (m_cluster_impl->is_cluster_set_member()) {
    m_cluster_impl->configure_cluster_set_member(m_target_instance);
  }

  const auto post_failure_actions =
      [this](bool owns_rpl_user, bool drop_rpl_user,
             int64_t restore_clone_thrsh, bool restore_rec_accounts) {
        assert(restore_clone_thrsh >= -1);

        if (owns_rpl_user && drop_rpl_user) {
          clean_replication_user();
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
                "%s. "
                "Not a fatal error.",
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

    {
      if (m_already_member) {
        // START GROUP_REPLICATION is the last thing that happens in
        // join_cluster, so if it's already running, we don't need to do that
        // again
        log_info("%s is already a group member, skipping join",
                 m_target_instance->descr().c_str());
      } else {
        log_info(
            "Joining '%s' to cluster using account %s to peer '%s'.",
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
          m_cluster_impl->create_local_replication_user(m_target_instance,
                                                        m_options.gr_options);

          // At this point, all Cluster members got the recovery credentials
          // changed to be able to use the account created for the joining
          // member so in case of failure, we must restore all
          restore_recovery_accounts = true;

          DBUG_EXECUTE_IF("fail_add_instance_mysql_stack", {
            // Throw an exception to simulate a failure at
            // this point in addInstance()
            throw std::logic_error("debug");
          });
        }

        // Join the instance to the Group Replication group.
        mysqlsh::dba::join_cluster(*m_target_instance, *m_primary_instance,
                                   m_options.gr_options, cluster_member_count,
                                   cfg.get());
      }

      // Wait until recovery done. Will throw an exception if recovery fails.
      wait_recovery(join_begin_time, m_progress_style);

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

    MetadataStorage::Transaction trx(m_cluster_impl->get_metadata_storage());

    // If the instance is not on the Metadata, we must add it.
    if (!is_instance_on_md) {
      m_cluster_impl->add_metadata_for_instance(*m_target_instance,
                                                m_options.label.get_safe());
      m_cluster_impl->get_metadata_storage()->update_instance_attribute(
          m_target_instance->get_uuid(), k_instance_attribute_join_time,
          shcore::Value(join_begin_time));

      // Store the username in the Metadata instances table
      if (owns_repl_user) {
        if (m_comm_stack == kCommunicationStackMySQL &&
            m_progress_style == Recovery_progress_style::NOWAIT) {
          store_local_replication_account();
        } else {
          store_cloned_replication_account();
        }
      }
    }

    // Update group_seeds and auto_increment in other members
    m_cluster_impl->update_group_peers(m_target_instance, m_options.gr_options,
                                       cluster_member_count,
                                       address_in_metadata);

    // Re-issue the CHANGE MASTER command
    // See note in store_cloned_replication_account()
    // NOTE: if waitRecover is zero we cannot do it since distributed recovery
    // may be running and the change master command will fail as the slave io
    // thread is running
    if (owns_repl_user && m_progress_style != Recovery_progress_style::NOWAIT) {
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
    if (m_comm_stack == kCommunicationStackMySQL &&
        m_progress_style != Recovery_progress_style::NOWAIT) {
      m_cluster_impl->restore_recovery_account_all_members();
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
