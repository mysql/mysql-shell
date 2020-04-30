/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#include <mysql.h>
#include <mysqld_error.h>
#include <climits>
#include <memory>
#include <utility>
#include <vector>

#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

// # of seconds to wait until recovery starts
constexpr const int k_recovery_start_timeout = 30;

Add_instance::Add_instance(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    Cluster_impl *cluster, const Group_replication_options &gr_options,
    const Clone_options &clone_options,
    const mysqlshdk::utils::nullable<std::string> &instance_label,
    bool interactive, Recovery_progress_style wait_recovery,
    const std::string &replication_user,
    const std::string &replication_password, bool rebooting)
    : m_instance_cnx_opts(instance_cnx_opts),
      m_cluster(cluster),
      m_gr_opts(gr_options),
      m_clone_opts(clone_options),
      m_instance_label(instance_label),
      m_interactive(interactive),
      m_rpl_user(replication_user),
      m_rpl_pwd(replication_password),
      m_rebooting(rebooting),
      m_progress_style(wait_recovery) {
  assert(cluster);

  m_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
}

Add_instance::Add_instance(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
    Cluster_impl *cluster, const Group_replication_options &gr_options,
    const Clone_options &clone_options,
    const mysqlshdk::utils::nullable<std::string> &instance_label,
    bool interactive, Recovery_progress_style wait_recovery,
    const std::string &replication_user,
    const std::string &replication_password, bool rebooting)
    : m_cluster(cluster),
      m_gr_opts(gr_options),
      m_clone_opts(clone_options),
      m_instance_label(instance_label),
      m_interactive(interactive),
      m_rpl_user(replication_user),
      m_rpl_pwd(replication_password),
      m_rebooting(rebooting),
      m_progress_style(wait_recovery) {
  assert(cluster);
  assert(target_instance);

  m_reuse_session_for_target_instance = true;
  m_target_instance = target_instance;
  m_instance_cnx_opts = target_instance->get_connection_options();
  m_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
}

Add_instance::~Add_instance() {}

void Add_instance::ensure_instance_check_installed_schema_version() const {
  // Get the lowest server version of the online members of the cluster.
  mysqlshdk::utils::Version lowest_cluster_version =
      m_cluster->get_lowest_instance_version();

  try {
    // Check instance version compatibility according to Group Replication.
    mysqlshdk::gr::check_instance_check_installed_schema_version(
        *m_target_instance, lowest_cluster_version);
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

void Add_instance::validate_local_address_ip_compatibility() const {
  // local_address must have some value
  assert(!m_gr_opts.local_address.is_null() &&
         !m_gr_opts.local_address.get_safe().empty());

  std::string local_address = m_gr_opts.local_address.get_safe();

  // Validate that the group_replication_local_address is valid for the version
  // of the target instance.
  if (!mysqlshdk::gr::is_endpoint_supported_by_gr(
          local_address, m_target_instance->get_version())) {
    auto console = mysqlsh::current_console();
    console->print_error("Cannot join instance '" + m_instance_address +
                         "' to cluster: unsupported localAddress value.");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use value '%s' for option localAddress because it has "
        "an IPv6 address which is only supported by Group Replication "
        "from MySQL version >= 8.0.14 and the target instance version is %s.",
        local_address.c_str(),
        m_target_instance->get_version().get_base().c_str()));
  }

  // Validate that the localAddress values of the cluster members are
  // compatible with the target instance we are adding
  std::string cluster_local_addresses = m_cluster->get_cluster_group_seeds();
  auto local_address_list = shcore::str_split(cluster_local_addresses, ",");
  std::vector<std::string> unsupported_addresses;
  for (const auto &raw_local_addr : local_address_list) {
    std::string local_addr = shcore::str_strip(raw_local_addr);
    if (!mysqlshdk::gr::is_endpoint_supported_by_gr(
            local_addr, m_target_instance->get_version())) {
      unsupported_addresses.push_back(local_addr);
    }
  }
  if (!unsupported_addresses.empty()) {
    std::string value_str =
        (unsupported_addresses.size() == 1) ? "value" : "values";

    auto console = mysqlsh::current_console();
    console->print_error(
        "Cannot join instance '" + m_instance_address +
        "' to cluster: unsupported localAddress value on the cluster.");
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Instance does not support the following localAddress %s of the "
        "cluster: '%s'. IPv6 "
        "addresses/hostnames are only supported by Group "
        "Replication from MySQL version >= 8.0.14 and the target instance "
        "version is %s.",
        value_str.c_str(),
        shcore::str_join(unsupported_addresses.cbegin(),
                         unsupported_addresses.cend(), ", ")
            .c_str(),
        m_target_instance->get_version().get_base().c_str()));
  }

  // validate that the value of the localAddress is supported by all
  // existing cluster members

  // Get the lowest cluster version
  mysqlshdk::utils::Version lowest_cluster_version =
      m_cluster->get_lowest_instance_version();
  if (!mysqlshdk::gr::is_endpoint_supported_by_gr(local_address,
                                                  lowest_cluster_version)) {
    auto console = mysqlsh::current_console();
    console->print_error(
        "Cannot join instance '" + m_instance_address +
        "' to cluster: localAddress value not supported by the cluster.");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use value '%s' for option localAddress because it has "
        "an IPv6 address which is only supported by Group Replication "
        "from MySQL version >= 8.0.14 but the cluster "
        "contains at least one member with version %s.",
        local_address.c_str(), lowest_cluster_version.get_base().c_str()));
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
    console->print_warning(mysqlsh::dba::k_warning_deprecate_ssl_mode);
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
    m_cluster->validate_server_id(*m_target_instance);
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
  auto md_inst = m_cluster->get_metadata_storage()->get_md_server();

  std::string recovery_user, recovery_user_host;
  std::tie(recovery_user, recovery_user_host) =
      m_cluster->get_metadata_storage()->get_instance_recovery_account(
          md_inst->get_uuid());

  // Insert the "donor" recovery account into the MD of the target instance
  log_debug("Adding donor recovery account to metadata");
  m_cluster->get_metadata_storage()->update_instance_recovery_account(
      m_target_instance->get_uuid(), recovery_user, "%");
}

void Add_instance::update_change_master() const {
  // See note in handle_recovery_account()
  log_debug("Re-issuing the CHANGE MASTER command");

  mysqlshdk::gr::change_recovery_credentials(*m_target_instance, m_rpl_user,
                                             m_rpl_pwd);

  // Update the recovery account to the right one
  log_debug("Adding recovery account to metadata");
  m_cluster->get_metadata_storage()->update_instance_recovery_account(
      m_target_instance->get_uuid(), m_rpl_user, "%");
}

void Add_instance::refresh_target_connections() {
  {
    try {
      m_target_instance->query("SELECT 1");
    } catch (const shcore::Error &err) {
      auto e = shcore::Exception::mysql_error_with_code(err.what(), err.code());

      if (CR_SERVER_LOST == e.code()) {
        log_debug(
            "Target instance connection lost: %s. Re-establishing a "
            "connection.",
            e.format().c_str());

        m_target_instance->get_session()->connect(m_instance_cnx_opts);
        m_target_instance->prepare_session();
      } else {
        throw;
      }
    }

    try {
      m_cluster->get_metadata_storage()->get_md_server()->query("SELECT 1");
    } catch (const shcore::Error &err) {
      auto e = shcore::Exception::mysql_error_with_code(err.what(), err.code());

      if (CR_SERVER_LOST == e.code()) {
        log_debug(
            "Metadata connection lost: %s. Re-establishing a "
            "connection.",
            e.format().c_str());

        auto md_server = m_cluster->get_metadata_storage()->get_md_server();

        md_server->get_session()->connect(md_server->get_connection_options());
        md_server->prepare_session();
      } else {
        throw;
      }
    }
  }
}

void Add_instance::handle_clone_plugin_state(bool enable_clone) {
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
  m_cluster->setup_clone_plugin(enable_clone);
}

void Add_instance::check_cluster_members_limit() const {
  const std::vector<Instance_metadata> all_instances =
      m_cluster->get_metadata_storage()->get_all_instances(m_cluster->get_id());

  if (all_instances.size() >= mysqlsh::dba::k_group_replication_members_limit) {
    auto console = mysqlsh::current_console();
    console->print_error("Cannot join instance '" + m_instance_address +
                         "' to cluster: InnoDB Cluster maximum limit of 9 "
                         "members has been reached.");
    throw shcore::Exception::runtime_error(
        "InnoDB Cluster already has the maximum number of 9 members.");
  }
}

void Add_instance::prepare() {
  check_cluster_members_limit();

  // Connect to the target instance (if needed).
  if (!m_reuse_session_for_target_instance) {
    // Get instance user information from the cluster session if missing.
    if (!m_instance_cnx_opts.has_user()) {
      Connection_options cluster_cnx_opt =
          m_cluster->get_target_server()->get_connection_options();

      if (!m_instance_cnx_opts.has_user() && cluster_cnx_opt.has_user())
        m_instance_cnx_opts.set_user(cluster_cnx_opt.get_user());
    }

    try {
      m_target_instance = Instance::connect(
          m_instance_cnx_opts, current_shell_options()->get().wizards);
    }
    CATCH_REPORT_AND_THROW_CONNECTION_ERROR(m_instance_cnx_opts.uri_endpoint())
  }

  //  Override connection option if no password was initially provided.
  if (!m_instance_cnx_opts.has_password()) {
    m_instance_cnx_opts = m_target_instance->get_connection_options();
  }

  // User name and password must be the same on all instances that belong to a
  // cluster. Check if we can connect to primary (seed) instance using provided
  // credentials.
  if (!m_rebooting) {
    auto seed = m_cluster->pick_seed_instance();
    std::shared_ptr<Instance> seed_session =
        checks::ensure_matching_credentials_with_seed(&seed,
                                                      m_instance_cnx_opts);

    // Connect to the peer instance.
    if (seed.uri_endpoint() != m_cluster->get_target_server()
                                   ->get_connection_options()
                                   .uri_endpoint()) {
      m_peer_instance = std::move(seed_session);
      m_use_cluster_session_for_peer = false;
    } else {
      m_peer_instance = m_cluster->get_target_server();
    }
  }

  // Validate the GR options.

  // Note: If the user provides no group_seeds value, it is automatically
  // assigned a value with the local_address values of the existing cluster
  // members and those local_address values are already validated on the
  // validate_local_address_ip_compatibility method, so we only need to validate
  // the group_seeds value provided by the user.
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
  m_clone_opts.check_option_values(m_target_instance->get_version());

  // BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC
  // - exitStateAction default value must be READ_ONLY
  // - exitStateAction default value should only be set if supported in
  // the target instance
  if (m_gr_opts.exit_state_action.is_null() &&
      is_option_supported(m_target_instance->get_version(), kExpelTimeout,
                          k_global_cluster_supported_options)) {
    m_gr_opts.exit_state_action = "READ_ONLY";
  }

  // Check if clone is disabled
  m_clone_disabled = m_cluster->get_disable_clone_option();

  // Check if clone is supported
  m_clone_supported = mysqlshdk::mysql::is_clone_available(*m_target_instance);

  // TODO(pjesus): remove the 'if (!m_rebooting)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_rebooting) {
    // Check instance version compatibility with cluster.
    ensure_instance_check_installed_schema_version();

    // Resolve the SSL Mode to use to configure the instance.
    resolve_ssl_mode();

    // Validate the lower_case_table_names and default_table_encryption
    // variables. Their values must be the same on the target instance as they
    // are on the cluster.

    // The lower_case_table_names can only be set the first time the server
    // boots, as such there is no need to validate it other than the first time
    // the instance is added to the cluster.
    m_cluster->validate_variable_compatibility(*m_target_instance,
                                               "lower_case_table_names");

    // The default_table_encryption is a dynamic variable, so we validate it on
    // the add_instance and on the rejoin operation. The reboot operation does a
    // rejoin in the background, so running the check on the rejoin will cover
    // both operations.
    if (m_cluster->get_lowest_instance_version() >=
            mysqlshdk::utils::Version(8, 0, 16) &&
        m_target_instance->get_version() >=
            mysqlshdk::utils::Version(8, 0, 16)) {
      m_cluster->validate_variable_compatibility(*m_target_instance,
                                                 "default_table_encryption");
    }
  }

  // Make sure the target instance does not already belong to a cluster.
  // Unless it's our cluster, in that case we keep adding it since it
  // may be a retry.
  mysqlsh::dba::checks::ensure_instance_not_belong_to_cluster(
      *m_target_instance, m_cluster->get_target_server(), &m_already_member);

  // Check GTID consistency and whether clone is needed (not needed in
  // rebootCluster)
  if (!m_rebooting) {
    auto cluster = m_cluster;
    auto check_recoverable = [cluster](mysqlshdk::mysql::IInstance *target) {
      // Check if the GTIDs were purged from the whole cluster
      bool recoverable = false;

      cluster->execute_in_members(
          {mysqlshdk::gr::Member_state::ONLINE},
          target->get_connection_options(), {},
          [&recoverable, &target](const std::shared_ptr<Instance> &instance) {
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

    std::shared_ptr<Instance> donor_instance = m_cluster->get_target_server();

    m_clone_opts.recovery_method = mysqlsh::dba::validate_instance_recovery(
        Cluster_type::GROUP_REPLICATION, Member_op_action::ADD_INSTANCE,
        donor_instance.get(), m_target_instance.get(), check_recoverable,
        *m_clone_opts.recovery_method, m_cluster->get_gtid_set_is_complete(),
        m_interactive, m_clone_disabled);
  }

  // If recovery method was selected as clone, check that there is at least one
  // ONLINE cluster instance not using IPV6, otherwise throw an error
  if (m_clone_opts.recovery_method.get_safe() ==
      Member_recovery_method::CLONE) {
    bool found_ipv4 = false;
    std::string full_msg;
    // get online members
    const auto online_instances =
        m_cluster->get_instances({mysqlshdk::gr::Member_state::ONLINE});
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
          "as "
          "clone donors for " +
          m_target_instance->descr());
      console->print_info(full_msg);
      throw shcore::Exception("The Cluster has no compatible clone donors.",
                              SHERR_DBA_CLONE_NO_DONORS);
    }
  }

  // Verify if the instance is running asynchronous (master-slave)
  // replication NOTE: Only verify if this is being called from a
  // addInstance() and not a createCluster() command.
  // TODO(pjesus): remove the 'if (!m_rebooting)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_rebooting) {
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

  // TODO(alfredo) - m_rebooting is only true when this is called from
  // rebootCluster. Once rebootCluster is rewritten to not use addInstance,
  // this check can be removed
  if (!m_rebooting && !m_already_member) {
    // Check instance server UUID (must be unique among the cluster members).
    m_cluster->validate_server_uuid(*m_target_instance);

    // Ensure instance server ID is unique among the cluster members.
    ensure_unique_server_id();
  }
  // Get the address used by GR for the added instance (used in MD).
  std::string hostname = m_target_instance->get_canonical_hostname();
  m_address_in_metadata = m_target_instance->get_canonical_address();

  // Resolve GR local address.
  // NOTE: Must be done only after getting the report_host used by GR and for
  //       the metadata;
  try {
    m_gr_opts.local_address = mysqlsh::dba::resolve_gr_local_address(
        m_gr_opts.local_address, hostname,
        *m_target_instance->get_sysvar_int("port"), !m_already_member);
  } catch (const std::runtime_error &e) {
    // TODO: this is a hack.. rebootCluster shouldn't do the busy port check,
    // which can fail if the instance is trying to auto-rejoin. Once
    // rebootCluster is refactored and doesn't call addInstance() directly
    // anymore, this can be removed.
    if (!m_rebooting || !strstr(e.what(), "is already in use")) {
      throw;
    }
  }

  // If this is not seed instance, then we should try to read the
  // failoverConsistency and expelTimeout values from a a cluster member.
  // TODO(pjesus): remove the 'if (!m_rebooting)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_rebooting) {
    m_cluster->query_group_wide_option_values(m_target_instance.get(),
                                              &m_gr_opts.consistency,
                                              &m_gr_opts.expel_timeout);

    // Check instance configuration and state, like dba.checkInstance
    // But don't do it if it was already done by the caller
    ensure_gr_instance_configuration_valid(m_target_instance.get());

    // Validate that the local_address value we want to use as well as the
    // local address values in use on the cluster are compatible with the
    // version of the instance being added to the cluster.
    validate_local_address_ip_compatibility();
  }

  // Set the internal configuration object: read/write configs from the server.
  m_cfg = mysqlsh::dba::create_server_config(
      m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);
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
  // rebooting was not set to true.
  // NOTE: User always created at the seed instance.
  if (!m_rebooting && m_rpl_user.empty()) {
    auto user_creds =
        m_cluster->create_replication_user(m_target_instance.get());
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
    auto primary = m_cluster->get_target_server();
    log_debug("Dropping recovery user '%s'@'%%' at instance '%s'.",
              m_rpl_user.c_str(), primary->descr().c_str());
    primary->drop_user(m_rpl_user, "%", true);
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

  // TODO(pjesus): remove the 'if (!m_rebooting)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_rebooting) {
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

  // TODO(pjesus): remove the 'if (!m_rebooting)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_rebooting) {
    console->print_info("Adding instance to the cluster...");
    console->println();
  }

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  mysqlshdk::gr::install_group_replication_plugin(*m_target_instance, nullptr);

  // TODO(pjesus): remove the 'if (!m_rebooting)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_rebooting) {
    // Validate group_replication_gtid_assignment_block_size. Its value must be
    // the same on the instance as it is on the cluster but can only be checked
    // after the GR plugin is installed. This check is also done on the rejoin
    // operation which covers the rejoin and rebootCluster operations.
    m_cluster->validate_variable_compatibility(
        *m_target_instance, "group_replication_gtid_assignment_block_size");

    // Handle GR protocol version.
    handle_gr_protocol_version();
  }

  // Get the current number of cluster members
  uint64_t cluster_member_count =
      m_cluster->get_metadata_storage()->get_cluster_size(m_cluster->get_id());

  // Clone handling

  if (!m_rebooting && m_clone_supported) {
    bool enable_clone = !m_clone_disabled;

    // Force clone if requested
    if (*m_clone_opts.recovery_method == Member_recovery_method::CLONE) {
      m_restore_clone_threshold =
          mysqlshdk::mysql::force_clone(*m_target_instance);

      // RESET MASTER to clear GTID_EXECUTED in case it's diverged, otherwise
      // clone is not executed and GR rejects the instance
      m_target_instance->query("RESET MASTER");
    } else if (*m_clone_opts.recovery_method ==
               Member_recovery_method::INCREMENTAL) {
      // Force incremental distributed recovery if requested
      enable_clone = false;
    }

    // Make sure the Clone plugin is installed or uninstalled depending on the
    // value of disableClone
    //
    // NOTE: If the clone usage is not disabled on the cluster (disableClone),
    // we must ensure the clone plugin is installed on all members. Otherwise,
    // if the cluster was creating an Older shell (<8.0.17) or if the cluster
    // was set up using the Incremental recovery only and the primary is removed
    // (or a failover happened) the instances won't have the clone plugin
    // installed and GR's recovery using clone will fail.
    //
    // See BUG#29954085 and BUG#29960838
    handle_clone_plugin_state(enable_clone);
  }

  // Handle the replication user creation.
  bool owns_repl_user = handle_replication_user();

  // we need a point in time as close as possible, but still earlier than
  // when recovery starts to monitor the recovery phase. The timestamp
  // resolution is timestamp(3) irrespective of platform
  std::string join_begin_time =
      m_target_instance->queryf_one_string(0, "", "SELECT NOW(3)");

  // TODO(pjesus): remove the 'if (m_rebooting)' for refactor of reboot
  //               cluster (WL#11561), mysqlsh::dba::start_cluster() should
  //               be used directly instead of the Add_instance
  //               operation.
  if (m_rebooting) {
    if (!m_gr_opts.group_name.is_null() && !m_gr_opts.group_name->empty()) {
      log_info("Using Group Replication group name: %s",
               m_gr_opts.group_name->c_str());
    }

    log_info("Starting cluster with '%s' using account %s",
             m_instance_address.c_str(),
             m_instance_cnx_opts.get_user().c_str());

    // Determine the topology mode to use.
    // TODO(pjesus): the topology mode (multi_primary) should not be needed,
    //               remove it for refactor of reboot cluster (WL#11561) and
    //               just pass a null (bool). Now, the current behaviour is
    //               maintained.
    mysqlshdk::utils::nullable<bool> multi_primary =
        m_cluster->get_cluster_topology_type() ==
        mysqlshdk::gr::Topology_mode::MULTI_PRIMARY;

    // Start the cluster to bootstrap Group Replication.
    mysqlsh::dba::start_cluster(*m_target_instance, m_gr_opts, multi_primary,
                                m_cfg.get());

  } else {
    // If no group_seeds value was provided by the user, then,
    // before joining instance to cluster, get the values of the
    // gr_local_address from all the active members of the cluster
    if (m_gr_opts.group_seeds.is_null() || m_gr_opts.group_seeds->empty()) {
      m_gr_opts.group_seeds = m_cluster->get_cluster_group_seeds();
    }

    if (m_already_member) {
      // START GROUP_REPLICATION is the last thing that happens in join_cluster,
      // so if it's already running, we don't need to do that again
      log_info("%s is already a group member, skipping join",
               m_instance_address.c_str());
    } else {
      log_info("Joining '%s' to cluster using account %s to peer '%s'.",
               m_instance_address.c_str(),
               m_peer_instance->get_connection_options().get_user().c_str(),
               m_peer_instance->descr().c_str());

      // Join the instance to the Group Replication group.
      mysqlsh::dba::join_cluster(*m_target_instance, *m_peer_instance,
                                 m_rpl_user, m_rpl_pwd, m_gr_opts,
                                 cluster_member_count, m_cfg.get());
    }

    // Wait until recovery done. Will throw an exception if recovery fails.
    try {
      monitor_gr_recovery_status(
          m_instance_cnx_opts, join_begin_time, m_progress_style,
          k_recovery_start_timeout,
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
          "Please make sure the MySQL server at '" +
          m_target_instance->descr() +
          "' is restarted and call <Cluster>.rescan() to complete the process. "
          "To increase the timeout, change "
          "shell.options[\"dba.restartWaitTimeout\"].");

      throw shcore::Exception("Timeout waiting for server to restart",
                              SHERR_DBA_SERVER_RESTART_TIMEOUT);
    }

    // When clone is used, the target instance will restart and all
    // connections are closed so we need to test if the connection to the
    // target instance and MD are closed and re-open if necessary
    refresh_target_connections();
  }

  // Check if instance address already belong to cluster (metadata).
  bool is_instance_on_md =
      m_cluster->contains_instance_with_address(m_address_in_metadata);

  log_debug("Cluster %s: Instance '%s' %s", m_cluster->get_name().c_str(),
            m_instance_address.c_str(),
            is_instance_on_md ? "is already in the Metadata."
                              : "is being added to the Metadata...");

  MetadataStorage::Transaction trx(m_cluster->get_metadata_storage());

  // If the instance is not on the Metadata, we must add it.
  if (!is_instance_on_md) {
    m_cluster->add_instance_metadata(*m_target_instance,
                                     m_instance_label.get_safe());
    m_cluster->get_metadata_storage()->update_instance_attribute(
        m_target_instance->get_uuid(), k_instance_attribute_join_time,
        shcore::Value(join_begin_time));

    // Store the username in the Metadata instances table
    if (owns_repl_user) {
      handle_recovery_account();
    }
  }

  // Get the gr_address of the instance being added
  std::string added_instance_gr_address = *m_gr_opts.local_address;

  // Create a configuration object for the cluster, ignoring the added
  // instance, to update the remaining cluster members.
  // NOTE: only members that already belonged to the cluster and are either
  //       ONLINE or RECOVERING will be considered.
  std::vector<std::string> ignore_instances_vec = {m_address_in_metadata};
  std::unique_ptr<mysqlshdk::config::Config> cluster_cfg =
      m_cluster->create_config_object(ignore_instances_vec, true);

  // Update the group_replication_group_seeds of the cluster members
  // by adding the gr_local_address of the instance that was just added.
  log_debug("Updating Group Replication seeds on all active members...");
  mysqlshdk::gr::update_group_seeds(cluster_cfg.get(),
                                    added_instance_gr_address,
                                    mysqlshdk::gr::Gr_seeds_change_type::ADD);
  cluster_cfg->apply();

  // Increase the cluster_member_count counter
  cluster_member_count++;

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
  //   - Multi-primary Cluster
  //   - Cluster that has 7 or more members after the add_instance
  //     operation
  //
  // NOTE: in the other scenarios, the Add_instance operation is in
  // charge of updating auto-increment accordingly

  // Get the topology mode of the replicaSet
  mysqlshdk::gr::Topology_mode topology_mode =
      m_cluster->get_metadata_storage()->get_cluster_topology_mode(
          m_cluster->get_id());

  if (topology_mode == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY &&
      cluster_member_count > 7) {
    log_debug("Updating auto-increment settings on all active members...");
    mysqlshdk::gr::update_auto_increment(
        cluster_cfg.get(), mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);
    cluster_cfg->apply();
  }

  // Re-issue the CHANGE MASTER command
  // See note in handle_recovery_account()
  // NOTE: if waitRecover is zero we cannot do it since distributed recovery may
  // be running and the change master command will fail as the slave io thread
  // is running
  if (owns_repl_user && m_progress_style != Recovery_progress_style::NOWAIT) {
    update_change_master();
  }

  // Only commit transaction once everything is done, so that things don't look
  // all fine if something fails in the middle
  trx.commit();

  log_debug("Instance add finished");

  // TODO(pjesus): remove the 'if (!m_rebooting)' for refactor of reboot
  //               cluster (WL#11561), i.e. always execute code inside, not
  //               supposed to use Add_instance operation anymore.
  if (!m_rebooting) {
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
  } catch (const shcore::Error &e) {
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

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
