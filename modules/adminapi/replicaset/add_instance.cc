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

#include <memory>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "my_dbug.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/textui/textui.h"

namespace mysqlsh {
namespace dba {

Add_instance::Add_instance(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const ReplicaSet &replicaset, const Group_replication_options &gr_options,
    const mysqlshdk::utils::nullable<std::string> &instance_label,
    const std::string &replication_user,
    const std::string &replication_password, bool overwrite_seed,
    bool skip_instance_check, bool skip_rpl_user)
    : m_instance_cnx_opts(instance_cnx_opts),
      m_replicaset(replicaset),
      m_gr_opts(gr_options),
      m_instance_label(instance_label),
      m_rpl_user(replication_user),
      m_rpl_pwd(replication_password),
      m_seed_instance(overwrite_seed),
      m_skip_instance_check(skip_instance_check),
      m_skip_rpl_user(skip_rpl_user) {
  m_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
}

Add_instance::Add_instance(
    mysqlshdk::mysql::IInstance *target_instance, const ReplicaSet &replicaset,
    const Group_replication_options &gr_options,
    const mysqlshdk::utils::nullable<std::string> &instance_label,
    const std::string &replication_user,
    const std::string &replication_password, bool overwrite_seed,
    bool skip_instance_check, bool skip_rpl_user)
    : m_replicaset(replicaset),
      m_gr_opts(gr_options),
      m_instance_label(instance_label),
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
}

Add_instance::~Add_instance() {
  if (!m_reuse_session_for_target_instance) {
    delete m_target_instance;
  }
}

/**
 * Resolves SSL mode based on the given options.
 *
 * Determine the SSL mode for the cluster based on the provided SSL option and
 * if SSL is enabled on the target instance used to bootstrap the cluster (seed
 * instance). In case a new instance is joining an existing cluster, then the
 * same SSL mode of the cluster is used.
 */
void Add_instance::resolve_ssl_mode() {
  // Show deprecation message for memberSslMode option if if applies.
  if (!m_gr_opts.ssl_mode.is_null() && !m_seed_instance) {
    auto console = mysqlsh::current_console();
    console->print_warning(mysqlsh::dba::ReplicaSet::kWarningDeprecateSslMode);
    console->println();
  }

  // Set SSL Mode to AUTO by default (not defined).
  if (m_gr_opts.ssl_mode.is_null()) {
    m_gr_opts.ssl_mode = dba::kMemberSSLModeAuto;
  }

  std::string new_ssl_mode;
  std::string target;
  if (m_seed_instance) {
    new_ssl_mode = resolve_cluster_ssl_mode(m_target_instance->get_session(),
                                            *m_gr_opts.ssl_mode);
    target = "cluster";
  } else {
    new_ssl_mode = resolve_instance_ssl_mode(m_target_instance->get_session(),
                                             m_peer_instance->get_session(),
                                             *m_gr_opts.ssl_mode);
    target = "instance";
  }

  if (new_ssl_mode != *m_gr_opts.ssl_mode) {
    m_gr_opts.ssl_mode = new_ssl_mode;
    log_warning("SSL mode used to configure the %s: '%s'", target.c_str(),
                m_gr_opts.ssl_mode->c_str());
  }
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

  // BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC
  // - exitStateAction default value must be READ_ONLY
  // - exitStateAction default value should only be set if supported in
  // the target instance
  if (m_gr_opts.exit_state_action.is_null() &&
      is_group_replication_option_supported(m_target_instance->get_version(),
                                            kExpelTimeout)) {
    m_gr_opts.exit_state_action = "READ_ONLY";
  }

  // Check replication filters before creating the Metadata.
  validate_replication_filters(m_target_instance->get_session());

  // Resolve the SSL Mode to use to configure the instance.
  resolve_ssl_mode();

  // Make sure the target instance does not already belong to a replicaset.
  mysqlsh::dba::checks::ensure_instance_not_belong_to_replicaset(
      *m_target_instance, m_replicaset);

  // Verify if the instance is running asynchronous (master-slave) replication
  // NOTE: Only verify if this is being called from a addInstance() and not a
  // createCluster() command.
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

  // Get the address used by GR for the added instance (used in MD).
  m_host_in_metadata = mysqlshdk::mysql::get_report_host(*m_target_instance);
  m_address_in_metadata =
      m_host_in_metadata + ":" + std::to_string(m_instance_cnx_opts.get_port());

  // Resolve GR local address.
  // NOTE: Must be done only after getting the report_host used by GR and for
  //       the metadata;
  m_gr_opts.local_address = mysqlsh::dba::resolve_gr_local_address(
      m_gr_opts.local_address, m_host_in_metadata,
      m_instance_cnx_opts.get_port());

  // If this is not seed instance, then we should try to read the
  // failoverConsistency and expelTimeout values from a a cluster member.
  if (!m_seed_instance) {
    m_replicaset.query_group_wide_option_values(
        m_target_instance, &m_gr_opts.consistency, &m_gr_opts.expel_timeout);
  }

  // Check instance configuration and state, like dba.checkInstance
  // But don't do it if it was already done by the caller
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

void Add_instance::handle_replication_user() {
  // Creates the replication user ONLY if not already given and if
  // skip_rpl_user was not set to true.
  // NOTE: always create it at the seed instance.
  if (!m_skip_rpl_user && m_rpl_user.empty()) {
    mysqlshdk::gr::create_replication_random_user_pass(
        m_seed_instance ? *m_target_instance : *m_peer_instance, &m_rpl_user,
        convert_ipwhitelist_to_netmask(m_gr_opts.ip_whitelist.get_safe()),
        &m_rpl_pwd);

    log_debug("Created replication user '%s'", m_rpl_user.c_str());
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

  if (!m_seed_instance) {
    console->print_info("Adding instance to the cluster ...");
    console->println();
  }

  // Handle the replication user creation.
  handle_replication_user();

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  mysqlshdk::gr::install_plugin(*m_target_instance, nullptr, true);

  // Handle GR protocol version.
  if (!m_seed_instance) {
    handle_gr_protocol_version();
  }

  // Get the current number of replicaSet members
  uint64_t replicaset_count =
      m_replicaset.get_cluster()->get_metadata_storage()->get_replicaset_count(
          m_replicaset.get_id());

  DBUG_EXECUTE_IF("dba_abort_join_group", { throw std::logic_error("debug"); });

  // Call MP
  // TODO(pjesus): This success should be removed for the refactor of
  //               createCluster(), exceptions will be raised if an error occurs
  //               when MP call is removed.
  bool success = false;
  if (m_seed_instance) {
    if (!m_gr_opts.group_name.is_null() && !m_gr_opts.group_name->empty()) {
      log_info("Using Group Replication group name: %s",
               m_gr_opts.group_name->c_str());
    }

    // Set the ssl mode
    m_replicaset.set_group_replication_member_options(
        m_target_instance->get_session(), *m_gr_opts.ssl_mode);

    // If no group_seeds value was provided by the user, then,
    // set it ti empty to maintain the current behaviour.
    // TODO(pjesus): Review need for this for createCluster() refactoring.
    if (m_gr_opts.group_seeds.is_null()) {
      m_gr_opts.group_seeds = "";
    }

    log_info("Starting Replicaset with '%s' using account %s",
             m_instance_address.c_str(),
             m_instance_cnx_opts.get_user().c_str());

    // Call mysqlprovision to bootstrap the group using "start"
    success = m_replicaset.do_join_replicaset(m_instance_cnx_opts, nullptr,
                                              m_rpl_user, m_rpl_pwd,
                                              m_skip_rpl_user, 0, m_gr_opts);
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

    mysqlsh::dba::join_replicaset(*m_target_instance, *m_peer_instance,
                                  m_rpl_user, m_rpl_pwd, m_gr_opts,
                                  replicaset_count, m_cfg.get());
    success = true;
  }

  if (success) {
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
    if (!is_instance_on_md)
      m_replicaset.add_instance_metadata(m_instance_cnx_opts,
                                         m_instance_label.get_safe());

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

    log_debug("Instance add finished");
  }

  if (!m_seed_instance) {
    console->print_info("The instance '" + m_instance_address +
                        "' was successfully added to the cluster.");
    console->println();
  }

  return shcore::Value();
}

void Add_instance::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Add_instance::finish() {
  // Close the instance and peer session at the end if available.
  if (!m_reuse_session_for_target_instance && m_target_instance) {
    m_target_instance->close_session();
  }

  if (!m_use_cluster_session_for_peer && m_peer_instance) {
    m_peer_instance->close_session();
  }
}

}  // namespace dba
}  // namespace mysqlsh
