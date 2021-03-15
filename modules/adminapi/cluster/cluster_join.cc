/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/cluster_join.h"

#include <mysql.h>
#include <mysqld_error.h>
#include <climits>
#include <memory>
#include <utility>
#include <vector>

#include "adminapi/common/async_topology.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
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

Cluster_join::Cluster_join(
    Cluster_impl *cluster, mysqlsh::dba::Instance *primary_instance,
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
    const Group_replication_options &gr_options,
    const Clone_options &clone_options, bool interactive)
    : m_cluster(cluster),
      m_primary_instance(primary_instance),
      m_target_instance(target_instance),
      m_gr_opts(gr_options),
      m_clone_opts(clone_options),
      m_interactive(interactive) {
  // Validate the GR options.
  // Note: If the user provides no group_seeds value, it is automatically
  // assigned a value with the local_address values of the existing cluster
  // members and those local_address values are already validated on the
  // validate_local_address_ip_compatibility method, so we only need to validate
  // the group_seeds value provided by the user.
  m_gr_opts.check_option_values(m_target_instance->get_version());

  // Validate the Clone options.
  m_clone_opts.check_option_values(m_target_instance->get_version());
}

void Cluster_join::ensure_instance_check_installed_schema_version() const {
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
        "Cannot join instance '" + m_target_instance->descr() +
        "' to cluster: instance version is incompatible with the cluster.");
    throw;
  }

  // Print a warning if the instance is only read compatible.
  if (mysqlshdk::gr::is_instance_only_read_compatible(*m_target_instance,
                                                      lowest_cluster_version)) {
    auto console = mysqlsh::current_console();
    console->print_warning("The instance '" + m_target_instance->descr() +
                           "' is only read compatible with the cluster, thus "
                           "it will join the cluster in R/O mode.");
  }
}

void Cluster_join::resolve_local_address(
    Group_replication_options *gr_options,
    const Group_replication_options &user_gr_options,
    checks::Check_type check_type) {
  std::string hostname = m_target_instance->get_canonical_hostname();

  gr_options->local_address = mysqlsh::dba::resolve_gr_local_address(
      user_gr_options.local_address.get_safe().empty()
          ? gr_options->local_address
          : user_gr_options.local_address,
      hostname, *m_target_instance->get_sysvar_int("port"),
      !m_already_member && check_type == checks::Check_type::JOIN,
      check_type != checks::Check_type::JOIN);

  // Validate that the local_address value we want to use as well as the
  // local address values in use on the cluster are compatible with the
  // version of the instance being added to the cluster.
  validate_local_address_ip_compatibility(gr_options->local_address.get_safe(),
                                          gr_options->group_seeds.get_safe(),
                                          check_type);
}

void Cluster_join::validate_local_address_ip_compatibility(
    const std::string &local_address, const std::string &group_seeds,
    checks::Check_type check_type) const {
  // local_address must have some value
  assert(!local_address.empty());

  // Validate that the group_replication_local_address is valid for the version
  // of the target instance.
  if (!mysqlshdk::gr::is_endpoint_supported_by_gr(
          local_address, m_target_instance->get_version())) {
    auto console = mysqlsh::current_console();
    console->print_error("Cannot join instance '" + m_target_instance->descr() +
                         "' to cluster: unsupported localAddress value.");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use value '%s' for option localAddress because it has "
        "an IPv6 address which is only supported by Group Replication "
        "from MySQL version >= 8.0.14 and the target instance version is %s.",
        local_address.c_str(),
        m_target_instance->get_version().get_base().c_str()));
  }

  if (check_type != checks::Check_type::BOOTSTRAP) {
    // Validate that the localAddress values of the cluster members are
    // compatible with the target instance we are adding
    auto local_address_list = shcore::str_split(group_seeds, ",");
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
          "Cannot join instance '" + m_target_instance->descr() +
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
          "Cannot join instance '" + m_target_instance->descr() +
          "' to cluster: localAddress value not supported by the cluster.");
      throw shcore::Exception::argument_error(shcore::str_format(
          "Cannot use value '%s' for option localAddress because it has "
          "an IPv6 address which is only supported by Group Replication "
          "from MySQL version >= 8.0.14 but the cluster "
          "contains at least one member with version %s.",
          local_address.c_str(), lowest_cluster_version.get_base().c_str()));
    }
  }
}

/**
 * Resolves SSL mode based on the given options.
 *
 * Determine the SSL mode for the instance based on the cluster SSL mode and
 * if SSL is enabled on the target instance. The same SSL mode of the cluster
 * is used for new instance joining it.
 */
void Cluster_join::resolve_ssl_mode() {
  if (m_primary_instance) {
    resolve_instance_ssl_mode(*m_target_instance, *m_primary_instance,
                              &m_gr_opts.ssl_mode);
    log_info("SSL mode used to configure the instance: '%s'",
             to_string(m_gr_opts.ssl_mode).c_str());
  }
}

void Cluster_join::ensure_unique_server_id() const {
  try {
    m_cluster->validate_server_id(*m_target_instance);
  } catch (const std::runtime_error &err) {
    auto console = mysqlsh::current_console();
    console->print_error("Cannot join instance '" + m_target_instance->descr() +
                         "' to the cluster because it has the same server ID "
                         "of a member of the cluster. Please change the server "
                         "ID of the instance to add: all members must have a "
                         "unique server ID.");
    throw;
  }
}

void Cluster_join::handle_recovery_account() const {
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

void Cluster_join::update_change_master() const {
  // See note in handle_recovery_account()
  std::string source_term = mysqlshdk::mysql::get_replication_source_keyword(
      m_target_instance->get_version());
  log_debug("Re-issuing the CHANGE %s command", source_term.c_str());

  mysqlshdk::gr::change_recovery_credentials(
      *m_target_instance, m_gr_opts.recovery_credentials->user,
      m_gr_opts.recovery_credentials->password.get_safe());

  // Update the recovery account to the right one
  log_debug("Adding recovery account to metadata");
  m_cluster->get_metadata_storage()->update_instance_recovery_account(
      m_target_instance->get_uuid(), m_gr_opts.recovery_credentials->user, "%");
}

void Cluster_join::refresh_target_connections() {
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

        try {
          m_target_instance->get_session()->connect(
              m_target_instance->get_connection_options());
        } catch (const shcore::Error &ie) {
          auto cluster_coptions =
              m_cluster->get_cluster_server()->get_connection_options();

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

void Cluster_join::handle_clone_plugin_state(bool enable_clone) {
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

void Cluster_join::configure_cluster_set_member() {
  try {
    m_target_instance->set_sysvar(
        "skip_replica_start", true,
        mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);
  } catch (const mysqlshdk::db::Error &e) {
    throw shcore::Exception::mysql_error_with_code_and_state(e.what(), e.code(),
                                                             e.sqlstate());
  }

  if (!m_cluster->is_primary_cluster()) {
    m_cluster->sync_transactions(*m_primary_instance,
                                 k_clusterset_async_channel_name, 0);
    auto cs = m_cluster->get_cluster_set();

    auto ar_options = cs->get_clusterset_replication_options();

    ar_options.repl_credentials =
        m_cluster->refresh_clusterset_replication_user();

    // Update the credentials on all cluster members
    m_cluster->execute_in_members(
        {}, m_cluster->get_global_primary_master()->get_connection_options(),
        {},
        [&](const std::shared_ptr<Instance> &target,
            const mysqlshdk::gr::Member &) {
          async_update_replica_credentials(
              target.get(), k_clusterset_async_channel_name, ar_options, false);
          return true;
        });

    // Setup the replication channel at the target instance but do not start
    // it since that's handled by Group Replication
    async_add_replica(m_cluster->get_global_primary_master().get(),
                      m_target_instance.get(), k_clusterset_async_channel_name,
                      ar_options, true, false, false);
  }
}

void Cluster_join::check_cluster_members_limit() const {
  const std::vector<Instance_metadata> all_instances =
      m_cluster->get_metadata_storage()->get_all_instances(m_cluster->get_id());

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

Member_recovery_method Cluster_join::check_recovery_method(
    bool clone_disabled) {
  // Check GTID consistency and whether clone is needed
  auto check_recoverable = [this](mysqlshdk::mysql::IInstance *target) {
    // Check if the GTIDs were purged from the whole cluster
    bool recoverable = false;

    m_cluster->execute_in_members(
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

  std::shared_ptr<Instance> donor_instance = m_cluster->get_cluster_server();

  auto recovery_method = mysqlsh::dba::validate_instance_recovery(
      Cluster_type::GROUP_REPLICATION, Member_op_action::ADD_INSTANCE,
      donor_instance.get(), m_target_instance.get(), check_recoverable,
      *m_clone_opts.recovery_method, m_cluster->get_gtid_set_is_complete(),
      m_interactive, clone_disabled);

  // If recovery method was selected as clone, check that there is at least one
  // ONLINE cluster instance not using IPV6, otherwise throw an error
  if (recovery_method == Member_recovery_method::CLONE) {
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
          "as clone donors for " +
          m_target_instance->descr());
      console->print_info(full_msg);
      throw shcore::Exception("The Cluster has no compatible clone donors.",
                              SHERR_DBA_CLONE_NO_DONORS);
    }
  }

  return recovery_method;
}

void Cluster_join::check_instance_configuration(checks::Check_type type) {
  Group_replication_options user_options(m_gr_opts);

  if (type != checks::Check_type::BOOTSTRAP) {
    // Check instance version compatibility with cluster.
    ensure_instance_check_installed_schema_version();

    // Resolve the SSL Mode to use to configure the instance.
    resolve_ssl_mode();
  }

  if (type == checks::Check_type::BOOTSTRAP) {
    // The current 'group_replication_group_name' must be kept otherwise
    // if instances are rejoined later the operation may fail because
    // a new group_name started being used.
    m_gr_opts.group_name = m_cluster->get_group_name();
  }
  if (type != checks::Check_type::JOIN) {
    // Read actual GR configurations to preserve them when rejoining the
    // instance.
    m_gr_opts.read_option_values(*m_target_instance);
  }

  // Check instance configuration and state, like dba.checkInstance
  // But don't do it if it was already done by the caller
  ensure_gr_instance_configuration_valid(m_target_instance.get(),
                                         type == checks::Check_type::JOIN);

  // Validate the lower_case_table_names and default_table_encryption
  // variables. Their values must be the same on the target instance as they
  // are on the cluster.
  if (type != checks::Check_type::BOOTSTRAP) {
    // The lower_case_table_names can only be set the first time the server
    // boots, as such there is no need to validate it other than the first time
    // the instance is added to the cluster.
    m_cluster->validate_variable_compatibility(*m_target_instance,
                                               "lower_case_table_names");

    // The default_table_encryption is a dynamic variable, so we validate it on
    // the Cluster_join and on the rejoin operation. The reboot operation does a
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

  // Verify if the instance is running asynchronous
  // replication
  // NOTE: Verify for all operations: addInstance(), rejoinInstance() and
  // rebootClusterFromCompleteOutage()
  validate_async_channels(
      *m_target_instance,
      m_cluster->is_cluster_set_member()
          ? std::unordered_set<std::string>{k_clusterset_async_channel_name}
          : std::unordered_set<std::string>{},
      type);

  if (type != checks::Check_type::BOOTSTRAP) {
    // If this is not seed instance, then we should try to read the
    // failoverConsistency and expelTimeout values from a a cluster member.
    m_cluster->query_group_wide_option_values(m_target_instance.get(),
                                              &m_gr_opts.consistency,
                                              &m_gr_opts.expel_timeout);
  }

  if (type == checks::Check_type::JOIN) {
    if (!m_already_member) {
      // Check instance server UUID (must be unique among the cluster members).
      m_cluster->validate_server_uuid(*m_target_instance);

      // Ensure instance server ID is unique among the cluster members.
      ensure_unique_server_id();
    }
  }

  // If no group_seeds value was provided by the user, then,
  // before joining instance to cluster, get the values of the
  // gr_local_address from all the active members of the cluster
  if (user_options.group_seeds.is_null() || user_options.group_seeds->empty()) {
    if (type == checks::Check_type::REJOIN) {
      m_gr_opts.group_seeds =
          m_cluster->get_cluster_group_seeds(m_target_instance);
    } else if (type == checks::Check_type::JOIN) {
      m_gr_opts.group_seeds = m_cluster->get_cluster_group_seeds();
    }
  }

  // Resolve and validate GR local address.
  // NOTE: Must be done only after getting the report_host used by GR and for
  //       the metadata;
  resolve_local_address(&m_gr_opts, user_options, type);
}

namespace {
bool check_auto_rejoining(Instance *instance) {
  // Check if instance was doing auto-rejoin and let the user know that the
  // rejoin operation will override the auto-rejoin
  if (mysqlshdk::gr::is_running_gr_auto_rejoin(*instance) ||
      mysqlshdk::gr::is_group_replication_delayed_starting(*instance)) {
    auto console = current_console();
    console->print_note(
        "The instance '" + instance->get_connection_options().uri_endpoint() +
        "' is running auto-rejoin process, which will be cancelled.");
    console->print_info();

    return true;
  }

  return false;
}

void ensure_not_auto_rejoining(Instance *instance) {
  auto console = current_console();
  console->print_note("Cancelling active GR auto-initialization at " +
                      instance->descr());
  mysqlshdk::gr::stop_group_replication(*instance);
}

}  // namespace

bool Cluster_join::check_rejoinable(bool *out_uuid_mistmatch) {
  // Check if the instance is part of the Metadata
  auto status = validate_instance_rejoinable(
      *m_target_instance, m_cluster->get_metadata_storage(),
      m_cluster->get_id(), out_uuid_mistmatch);

  switch (status) {
    case Instance_rejoinability::NOT_MEMBER:
      throw shcore::Exception::runtime_error(
          "The instance '" + m_target_instance->descr() + "' " +
          "does not belong to the cluster: '" + m_cluster->get_name() + "'.");

    case Instance_rejoinability::ONLINE:
      current_console()->print_note(
          m_target_instance->descr() +
          " is already an active (ONLINE) member of cluster '" +
          m_cluster->get_name() + "'.");
      return false;

    case Instance_rejoinability::RECOVERING:
      current_console()->print_note(
          m_target_instance->descr() +
          " is already an active (RECOVERING) member of cluster '" +
          m_cluster->get_name() + "'.");
      return false;

    case Instance_rejoinability::REJOINABLE:
      break;
  }

  std::string nice_error =
      "The instance '" + m_target_instance->descr() +
      "' may belong to a different cluster as the one registered "
      "in the Metadata since the value of "
      "'group_replication_group_name' does not match the one "
      "registered in the cluster's Metadata: possible split-brain "
      "scenario. Please remove the instance from the cluster.";

  try {
    if (!validate_cluster_group_name(*m_target_instance,
                                     m_cluster->get_group_name())) {
      throw shcore::Exception::runtime_error(nice_error);
    }
  } catch (const shcore::Error &e) {
    // If the GR plugin is not installed, we can get this error.
    // In that case, we install the GR plugin and retry.
    if (e.code() == ER_UNKNOWN_SYSTEM_VARIABLE) {
      log_info("%s: installing GR plugin (%s)",
               m_target_instance->descr().c_str(), e.format().c_str());
      mysqlshdk::gr::install_group_replication_plugin(*m_target_instance,
                                                      nullptr);

      if (!validate_cluster_group_name(*m_target_instance,
                                       m_cluster->get_group_name())) {
        throw shcore::Exception::runtime_error(nice_error);
      }
    } else {
      throw;
    }
  }

  return true;
}

void Cluster_join::prepare_reboot() {
  m_is_autorejoining = check_auto_rejoining(m_target_instance.get());

  // Make sure the target instance does not already belong to a different
  // cluster.
  mysqlsh::dba::checks::ensure_instance_not_belong_to_cluster(
      *m_target_instance, m_cluster->get_cluster_server(), &m_already_member);

  check_instance_configuration(checks::Check_type::BOOTSTRAP);
}

void Cluster_join::prepare_join(
    const mysqlshdk::utils::nullable<std::string> &instance_label) {
  m_instance_label = instance_label;

  check_cluster_members_limit();

  // Make sure there isn't some leftover auto-rejoin active
  m_is_autorejoining = check_auto_rejoining(m_target_instance.get());

  // Make sure the target instance does not already belong to a cluster.
  // Unless it's our cluster, in that case we keep adding it since it
  // may be a retry.
  if (mysqlsh::dba::checks::ensure_instance_not_belong_to_cluster(
          *m_target_instance, m_cluster->get_cluster_server(),
          &m_already_member) == TargetType::InnoDBCluster) {
    throw shcore::Exception::runtime_error(
        "The instance '" + m_target_instance->descr() +
        "' is already part of this InnoDB cluster");
  }

  // Pick recovery method and check if it's actually usable
  // Done last to not request interaction before validations
  m_clone_opts.recovery_method =
      check_recovery_method(m_cluster->get_disable_clone_option());

  // Check for various instance specific configuration issues
  check_instance_configuration(checks::Check_type::JOIN);
}

bool Cluster_join::prepare_rejoin(bool *out_uuid_mistmatch) {
  m_is_autorejoining = check_auto_rejoining(m_target_instance.get());

  if (!check_rejoinable(out_uuid_mistmatch)) return false;

  // Check for various instance specific configuration issues
  check_instance_configuration(checks::Check_type::REJOIN);

  // Check GTID consistency to determine if the instance can safely rejoin the
  // cluster BUG#29953812: ADD_INSTANCE() PICKY ABOUT GTID_EXECUTED,
  // REJOIN_INSTANCE() NOT: DATA NOT COPIED
  m_cluster->validate_rejoin_gtid_consistency(*m_target_instance);

  return true;
}

bool Cluster_join::handle_replication_user() {
  // Creates the replication user ONLY if not already given.
  // NOTE: User always created at the seed instance.
  if (!m_gr_opts.recovery_credentials ||
      m_gr_opts.recovery_credentials->user.empty()) {
    m_gr_opts.recovery_credentials =
        m_cluster->create_replication_user(m_target_instance.get());
    return true;
  }
  return false;
}

/**
 * Clean (drop) the replication user, in case the operation fails.
 */
void Cluster_join::clean_replication_user() {
  if (m_gr_opts.recovery_credentials &&
      !m_gr_opts.recovery_credentials->user.empty()) {
    auto primary = m_cluster->get_cluster_server();
    log_debug("Dropping recovery user '%s'@'%%' at instance '%s'.",
              m_gr_opts.recovery_credentials->user.c_str(),
              primary->descr().c_str());
    primary->drop_user(m_gr_opts.recovery_credentials->user, "%", true);
  }
}

void Cluster_join::log_used_gr_options() {
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

void Cluster_join::update_group_peers(int cluster_member_count,
                                      const std::string &self_address) {
  // Get the gr_address of the instance being added
  std::string added_instance_gr_address = *m_gr_opts.local_address;

  // Create a configuration object for the cluster, ignoring the added
  // instance, to update the remaining cluster members.
  // NOTE: only members that already belonged to the cluster and are either
  //       ONLINE or RECOVERING will be considered.
  std::vector<std::string> ignore_instances_vec = {self_address};
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
  // We must update the auto-increment values in Cluster_join for 2
  // scenarios
  //   - Multi-primary Cluster
  //   - Cluster that has 7 or more members after the Cluster_join
  //     operation
  //
  // NOTE: in the other scenarios, the Cluster_join operation is in
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
}

void Cluster_join::wait_recovery(const std::string &join_begin_time,
                                 Recovery_progress_style progress_style) {
  auto console = current_console();
  try {
    auto post_clone_auth = m_target_instance->get_connection_options();
    post_clone_auth.set_login_options_from(
        m_cluster->get_cluster_server()->get_connection_options());

    monitor_gr_recovery_status(
        m_target_instance->get_connection_options(), post_clone_auth,
        join_begin_time, progress_style, k_recovery_start_timeout,
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

void Cluster_join::join(Recovery_progress_style progress_style) {
  auto console = mysqlsh::current_console();

  // Set the internal configuration object: read/write configs from the
  // server.
  auto cfg = mysqlsh::dba::create_server_config(
      m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);

  // Common informative logging
  log_used_gr_options();

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
  m_cluster->validate_variable_compatibility(
      *m_target_instance, "group_replication_gtid_assignment_block_size");

  // Note: We used to auto-downgrade the GR protocol version if the joining
  // member is too old, but that's not allowed for other reasons anyway

  // Get the current number of cluster members
  uint64_t cluster_member_count =
      m_cluster->get_metadata_storage()->get_cluster_size(m_cluster->get_id());

  // Clone handling
  bool clone_disabled = m_cluster->get_disable_clone_option();
  bool clone_supported =
      mysqlshdk::mysql::is_clone_available(*m_target_instance);

  int64_t restore_clone_threshold = 0;

  if (clone_supported) {
    bool enable_clone = !clone_disabled;

    // Force clone if requested
    if (*m_clone_opts.recovery_method == Member_recovery_method::CLONE) {
      restore_clone_threshold =
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
    // was set up using the Incremental recovery only and the primary is
    // removed (or a failover happened) the instances won't have the clone
    // plugin installed and GR's recovery using clone will fail.
    //
    // See BUG#29954085 and BUG#29960838
    handle_clone_plugin_state(enable_clone);
  }

  // Handle the replication user creation.
  bool owns_repl_user = handle_replication_user();

  // enable skip_slave_start if the cluster belongs to a ClusterSet, and
  // configure the managed replication channel if the cluster is a replica.
  if (m_cluster->is_cluster_set_member()) {
    configure_cluster_set_member();
  }

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

        // Join the instance to the Group Replication group.
        mysqlsh::dba::join_cluster(*m_target_instance, *m_primary_instance,
                                   m_gr_opts, cluster_member_count, cfg.get());
      }

      // Wait until recovery done. Will throw an exception if recovery fails.
      wait_recovery(join_begin_time, progress_style);

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
        m_cluster->contains_instance_with_address(address_in_metadata);

    log_debug("Cluster %s: Instance '%s' %s", m_cluster->get_name().c_str(),
              m_target_instance->descr().c_str(),
              is_instance_on_md ? "is already in the Metadata."
                                : "is being added to the Metadata...");

    MetadataStorage::Transaction trx(m_cluster->get_metadata_storage());

    // If the instance is not on the Metadata, we must add it.
    if (!is_instance_on_md) {
      m_cluster->add_metadata_for_instance(*m_target_instance,
                                           m_instance_label.get_safe());
      m_cluster->get_metadata_storage()->update_instance_attribute(
          m_target_instance->get_uuid(), k_instance_attribute_join_time,
          shcore::Value(join_begin_time));

      // Store the username in the Metadata instances table
      if (owns_repl_user) {
        handle_recovery_account();
      }
    }

    // Update group_seeds and auto_increment in other members
    update_group_peers(cluster_member_count, address_in_metadata);

    // Re-issue the CHANGE MASTER command
    // See note in handle_recovery_account()
    // NOTE: if waitRecover is zero we cannot do it since distributed recovery
    // may be running and the change master command will fail as the slave io
    // thread is running
    if (owns_repl_user && progress_style != Recovery_progress_style::NOWAIT) {
      update_change_master();
    }

    // Only commit transaction once everything is done, so that things don't
    // look all fine if something fails in the middle
    trx.commit();

    log_debug("Instance add finished");

    console->print_info("The instance '" + m_target_instance->descr() +
                        "' was successfully added to the cluster.");
    console->print_info();
  } catch (...) {
    if (owns_repl_user) clean_replication_user();

    try {
      // Check if group_replication_clone_threshold must be restored.
      // This would only be needed if the clone failed and the server didn't
      // restart.
      if (restore_clone_threshold != 0) {
        // TODO(miguel): 'start group_replication' returns before reading the
        // threshold value so we can have a race condition. We should wait
        // until the instance is 'RECOVERING'
        log_debug(
            "Restoring value of group_replication_clone_threshold to: %s.",
            std::to_string(restore_clone_threshold).c_str());

        // If -1 we must restore it's default, otherwise we restore the
        // initial value
        if (restore_clone_threshold == -1) {
          m_target_instance->set_sysvar_default(
              "group_replication_clone_threshold");
        } else {
          m_target_instance->set_sysvar("group_replication_clone_threshold",
                                        restore_clone_threshold);
        }
      }
    } catch (const shcore::Error &e) {
      log_info(
          "Could not restore value of group_replication_clone_threshold: %s. "
          "Not a fatal error.",
          e.what());
    }
    throw;
  }
}

void Cluster_join::rejoin() {
  auto console = current_console();

  // Set a Config object for the target instance (required to configure GR).
  std::unique_ptr<mysqlshdk::config::Config> cfg = create_server_config(
      m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);

  console->print_info("Rejoining instance '" + m_target_instance->descr() +
                      "' to cluster '" + m_cluster->get_name() + "'...");

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  mysqlshdk::gr::install_group_replication_plugin(*m_target_instance, nullptr);

  // Ensure GR is not auto-rejoining, but after the GR plugin is installed
  if (m_is_autorejoining) ensure_not_auto_rejoining(m_target_instance.get());

  // Validate group_replication_gtid_assignment_block_size. Its value must be
  // the same on the instance as it is on the cluster but can only be checked
  // after the GR plugin is installed. This check is also done on the rejoin
  // operation which covers the rejoin and rebootCluster operations.
  m_cluster->validate_variable_compatibility(
      *m_target_instance, "group_replication_gtid_assignment_block_size");

  // enable skip_slave_start if the cluster belongs to a ClusterSet, and
  // configure the managed replication channel if the cluster is a
  // replica.
  if (m_cluster->is_cluster_set_member()) {
    configure_cluster_set_member();
  }

  // TODO(alfredo) - when clone support is added to rejoin, join() can
  // probably be simplified into create repl user + rejoin() + update metadata

  // (Re-)join the instance to the cluster (setting up GR properly).
  // NOTE: the join_cluster() function expects the number of members in
  //       the cluster excluding the joining node, thus cluster_count must
  //       exclude the rejoining node (cluster size - 1) since it already
  //       belongs to the metadata (BUG#30174191).
  mysqlshdk::utils::nullable<uint64_t> cluster_count =
      m_cluster->get_metadata_storage()->get_cluster_size(m_cluster->get_id()) -
      1;
  mysqlsh::dba::join_cluster(*m_target_instance, *m_primary_instance, m_gr_opts,
                             cluster_count, cfg.get());

  console->print_info("The instance '" + m_target_instance->descr() +
                      "' was successfully rejoined to the cluster.");
  console->print_info();
}

void Cluster_join::reboot() {
  // Set the internal configuration object: read/write configs from the
  // server.
  auto cfg = mysqlsh::dba::create_server_config(
      m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);

  // Common informative logging
  log_used_gr_options();

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  mysqlshdk::gr::install_group_replication_plugin(*m_target_instance, nullptr);

  if (m_is_autorejoining) ensure_not_auto_rejoining(m_target_instance.get());

  if (!m_gr_opts.group_name.is_null() && !m_gr_opts.group_name->empty()) {
    log_info("Using Group Replication group name: %s",
             m_gr_opts.group_name->c_str());
  }

  log_info("Starting cluster with '%s' using account %s",
           m_target_instance->descr().c_str(),
           m_target_instance->get_connection_options().get_user().c_str());

  // Determine the topology mode to use.
  mysqlshdk::utils::nullable<bool> multi_primary =
      m_cluster->get_cluster_topology_type() ==
      mysqlshdk::gr::Topology_mode::MULTI_PRIMARY;

  // Start the cluster to bootstrap Group Replication.
  mysqlsh::dba::start_cluster(*m_target_instance, m_gr_opts, multi_primary,
                              cfg.get());

  log_debug("Instance add finished");
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
