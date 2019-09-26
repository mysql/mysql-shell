/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/cluster/cluster_impl.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_describe.h"
#include "modules/adminapi/cluster/cluster_options.h"
#include "modules/adminapi/cluster/cluster_set_option.h"
#include "modules/adminapi/cluster/cluster_status.h"
#include "modules/adminapi/cluster/replicaset/add_instance.h"
#include "modules/adminapi/cluster/replicaset/remove_instance.h"
#include "modules/adminapi/cluster/replicaset/replicaset.h"
#include "modules/adminapi/cluster/reset_recovery_accounts_password.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/dba_utils.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "shellcore/utils_help.h"
#include "utils/debug.h"
#include "utils/utils_general.h"

namespace mysqlsh {
namespace dba {

Cluster_impl::Cluster_impl(
    const Cluster_metadata &metadata,
    const std::shared_ptr<Instance> &group_server,
    const std::shared_ptr<MetadataStorage> &metadata_storage)
    : m_id(metadata.cluster_id),
      m_cluster_name(metadata.cluster_name),
      m_group_name(metadata.group_name),
      m_target_server(group_server),
      _metadata_storage(metadata_storage) {
  _default_replica_set =
      std::make_shared<GRReplicaSet>("default", metadata.topology_type);

  _default_replica_set->set_cluster(this);
}

Cluster_impl::Cluster_impl(
    const std::string &cluster_name, const std::string &group_name,
    const std::shared_ptr<Instance> &group_server,
    const std::shared_ptr<MetadataStorage> &metadata_storage)
    : m_cluster_name(cluster_name),
      m_group_name(group_name),
      m_target_server(group_server),
      _metadata_storage(metadata_storage) {}

Cluster_impl::~Cluster_impl() {}

bool Cluster_impl::get_gtid_set_is_complete() const {
  shcore::Value flag;
  if (get_metadata_storage()->query_cluster_attribute(
          get_id(), k_cluster_attribute_assume_gtid_set_complete, &flag))
    return flag.as_bool();
  return false;
}

void Cluster_impl::add_instance(const Connection_options &instance_def,
                                const shcore::Dictionary_t &options) {
  check_preconditions("addInstance");

  _default_replica_set->add_instance(instance_def, options);
}

shcore::Value Cluster_impl::rejoin_instance(
    const Connection_options &instance_def,
    const shcore::Dictionary_t &options) {
  // rejoin the Instance to the Default ReplicaSet
  shcore::Value ret_val;

  check_preconditions("rejoinInstance");

  Connection_options instance(instance_def);

  // if not, call mysqlprovision to join the instance to its own group
  ret_val = _default_replica_set->rejoin_instance(&instance, options);

  return ret_val;
}

shcore::Value Cluster_impl::remove_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  check_preconditions("removeInstance");

  ret_val = _default_replica_set->remove_instance(args);

  return ret_val;
}

std::shared_ptr<GRReplicaSet> Cluster_impl::create_default_replicaset(
    const std::string &name, bool multi_primary) {
  std::string topology_type = GRReplicaSet::kTopologySinglePrimary;
  if (multi_primary) {
    topology_type = GRReplicaSet::kTopologyMultiPrimary;
  }
  _default_replica_set = std::make_shared<GRReplicaSet>(name, topology_type);

  _default_replica_set->set_cluster(this);

  return _default_replica_set;
}

shcore::Value Cluster_impl::describe(void) {
  // Throw an error if the cluster has already been dissolved

  check_preconditions("describe");

  // Create the Cluster_describe command and execute it.
  Cluster_describe op_describe(*this);
  // Always execute finish when leaving "try catch".
  auto finally =
      shcore::on_leave_scope([&op_describe]() { op_describe.finish(); });
  // Prepare the Cluster_describe command execution (validations).
  op_describe.prepare();
  // Execute Cluster_describe operations.
  return op_describe.execute();
}

shcore::Value Cluster_impl::status(uint64_t extended) {
  // Throw an error if the cluster has already been dissolved
  check_preconditions("status");

  // Create the Cluster_status command and execute it.
  Cluster_status op_status(*this, extended);
  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope([&op_status]() { op_status.finish(); });
  // Prepare the Cluster_status command execution (validations).
  op_status.prepare();
  // Execute Cluster_status operations.
  return op_status.execute();
}

shcore::Value Cluster_impl::list_routers(bool only_upgrade_required) {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["clusterName"] = shcore::Value(get_name());
  (*dict)["routers"] = router_list(get_metadata_storage().get(), get_id(),
                                   only_upgrade_required);

  return shcore::Value(dict);
}

void Cluster_impl::reset_recovery_password(
    const shcore::Dictionary_t &options) {
  check_preconditions("resetRecoveryAccountsPassword");

  mysqlshdk::utils::nullable<bool> force;
  bool interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (options) {
    Unpack_options(options)
        .optional("force", &force)
        .optional("interactive", &interactive)
        .end();
  }

  // Create the reset_recovery_command.
  {
    Reset_recovery_accounts_password op_reset(interactive, force, *this);
    // Always execute finish when leaving "try catch".
    auto finally = shcore::on_leave_scope([&op_reset]() { op_reset.finish(); });
    // Prepare the reset_recovery_accounts_password execution
    op_reset.prepare();
    // Execute the reset_recovery_accounts_password command.
    op_reset.execute();
  }
}

shcore::Value Cluster_impl::options(const shcore::Dictionary_t &options) {
  // Throw an error if the cluster has already been dissolved
  check_preconditions("options");

  bool all = false;
  // Retrieves optional options
  Unpack_options(options).optional("all", &all).end();

  // Create the Cluster_options command and execute it.
  Cluster_options op_option(*this, all);
  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope([&op_option]() { op_option.finish(); });
  // Prepare the Cluster_options command execution (validations).
  op_option.prepare();
  // Execute Cluster_options operations.
  return op_option.execute();
}

void Cluster_impl::dissolve(const shcore::Dictionary_t &options) {
  // We need to check if the group has quorum and if not we must abort the
  // operation otherwise GR blocks the writes to preserve the consistency
  // of the group and we end up with a hang.
  // This check is done at check_preconditions()
  check_preconditions("dissolve");

  _default_replica_set->dissolve(options);
}

shcore::Value Cluster_impl::force_quorum_using_partition_of(
    const shcore::Argument_list &args) {
  check_preconditions("forceQuorumUsingPartitionOf");

  // TODO(alfredo) - unpack the arguments here insetad of at
  // force_quorum_using_partition_of
  return _default_replica_set->force_quorum_using_partition_of(args);
}

void Cluster_impl::rescan(const shcore::Dictionary_t &options) {
  check_preconditions("rescan");

  _default_replica_set->rescan(options);
}

void Cluster_impl::disconnect() {
  if (m_target_server) {
    // no preconditions check needed for just disconnecting everything
    m_target_server->close_session();
    m_target_server.reset();
  }

  // If _group_session is the same as the session from the metadata,
  // then get_session would thrown an exception since the session is already
  // closed
  try {
    if (_metadata_storage->get_md_server()) {
      _metadata_storage->get_md_server()->close_session();
    }
  } catch (...) {
  }
}

bool Cluster_impl::get_disable_clone_option() const {
  shcore::Value value;
  if (_metadata_storage->query_cluster_attribute(
          get_id(), k_cluster_attribute_disable_clone, &value))
    return value.as_bool();
  return false;
}

void Cluster_impl::set_disable_clone_option(const bool disable_clone) {
  _metadata_storage->update_cluster_attribute(
      get_id(), k_cluster_attribute_disable_clone,
      disable_clone ? shcore::Value::True() : shcore::Value::False());
}

shcore::Value Cluster_impl::check_instance_state(
    const Connection_options &instance_def) {
  check_preconditions("checkInstanceState");

  return _default_replica_set->check_instance_state(instance_def);
}

void Cluster_impl::switch_to_single_primary_mode(
    const Connection_options &instance_def) {
  check_preconditions("switchToSinglePrimaryMode");

  // Switch to single-primary mode
  _default_replica_set->switch_to_single_primary_mode(instance_def);
}

void Cluster_impl::switch_to_multi_primary_mode(void) {
  check_preconditions("switchToMultiPrimaryMode");

  // Switch to single-primary mode
  _default_replica_set->switch_to_multi_primary_mode();
}

void Cluster_impl::set_primary_instance(
    const Connection_options &instance_def) {
  check_preconditions("setPrimaryInstance");

  // Set primary instance

  _default_replica_set->set_primary_instance(instance_def);
}

void Cluster_impl::set_option(const std::string &option,
                              const shcore::Value &value) {
  check_preconditions("setOption");

  // Set Cluster configuration option

  // Create the Cluster_set_option object and execute it.
  std::unique_ptr<Cluster_set_option> op_cluster_set_option;

  // Validation types due to a limitation on the expose() framework.
  // Currently, it's not possible to do overloading of functions that overload
  // an argument of type string/int since the type int is convertible to
  // string, thus overloading becomes ambiguous. As soon as that limitation is
  // gone, this type checking shall go away too.
  if (value.type == shcore::String) {
    std::string value_str = value.as_string();
    op_cluster_set_option =
        std::make_unique<Cluster_set_option>(this, option, value_str);
  } else if (value.type == shcore::Integer || value.type == shcore::UInteger) {
    int64_t value_int = value.as_int();
    op_cluster_set_option =
        std::make_unique<Cluster_set_option>(this, option, value_int);
  } else if (value.type == shcore::Bool) {
    bool value_bool = value.as_bool();
    op_cluster_set_option =
        std::make_unique<Cluster_set_option>(this, option, value_bool);
  } else {
    throw shcore::Exception::argument_error(
        "Argument #2 is expected to be a string, an integer or a boolean.");
  }

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_cluster_set_option]() { op_cluster_set_option->finish(); });

  // Prepare the Set_option command execution (validations).
  op_cluster_set_option->prepare();

  // Execute Set_instance_option operations.
  op_cluster_set_option->execute();
}

void Cluster_impl::set_instance_option(const Connection_options &instance_def,
                                       const std::string &option,
                                       const shcore::Value &value) {
  check_preconditions("setInstanceOption");

  // Set the option in the Default ReplicaSet
  _default_replica_set->set_instance_option(instance_def, option, value);
}

Cluster_check_info Cluster_impl::check_preconditions(
    const std::string &function_name) const {
  return check_function_preconditions("Cluster." + function_name,
                                      get_target_instance());
}

void Cluster_impl::sync_transactions(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  // Must get the value of the 'gtid_executed' variable with GLOBAL scope to get
  // the GTID of ALL transactions, otherwise only a set of transactions written
  // to the cache in the current session might be returned.
  std::string gtid_set = m_target_server->query("SELECT @@GLOBAL.GTID_EXECUTED")
                             ->fetch_one()
                             ->get_string(0);

  bool sync_res = mysqlshdk::mysql::wait_for_gtid_set(
      target_instance, gtid_set,
      current_shell_options()->get().dba_gtid_wait_timeout);
  if (!sync_res) {
    std::string instance_address =
        target_instance.get_connection_options().as_uri(
            mysqlshdk::db::uri::formats::only_transport());
    throw shcore::Exception::runtime_error(
        "Timeout reached waiting for cluster transactions to be applied on "
        "instance '" +
        instance_address + "'");
  }
}

mysqlshdk::utils::Version Cluster_impl::get_lowest_instance_version() const {
  // Get the server version of the current cluster instance and initialize
  // the lowest cluster session.
  mysqlshdk::utils::Version lowest_version =
      get_target_instance()->get_version();

  // Get address of the cluster instance to skip it in the next step.
  std::string cluster_instance_address =
      get_target_instance()->get_canonical_address();

  // Get the lowest server version from the available cluster instances.
  get_default_replicaset()->execute_in_members(
      {mysqlshdk::gr::Member_state::ONLINE,
       mysqlshdk::gr::Member_state::RECOVERING},
      get_target_instance()->get_connection_options(),
      {cluster_instance_address},
      [&lowest_version](
          const std::shared_ptr<mysqlshdk::db::ISession> &session) {
        mysqlsh::dba::Instance instance(session);
        mysqlshdk::utils::Version version = instance.get_version();
        if (version < lowest_version) {
          lowest_version = version;
        }
        return true;
      });

  return lowest_version;
}

mysqlshdk::mysql::Auth_options Cluster_impl::create_replication_user(
    mysqlshdk::mysql::IInstance *target) {
  assert(target);

  std::string recovery_user =
      mysqlshdk::gr::k_group_recovery_user_prefix +
      std::to_string(target->get_sysvar_int("server_id").get_safe());

  log_info("Creating recovery account '%s'@'%%' for instance '%s'",
           recovery_user.c_str(), target->descr().c_str());

  // Check if the replication user already exists and delete it if it does,
  // before creating it again.
  bool rpl_user_exists = get_target_instance()->user_exists(recovery_user, "%");
  if (rpl_user_exists) {
    auto console = current_console();
    console->print_warning(shcore::str_format(
        "User '%s'@'%%' already existed at instance '%s'. It will be "
        "deleted and created again with a new password.",
        recovery_user.c_str(), get_target_instance()->descr().c_str()));
    get_target_instance()->drop_user(recovery_user, "%");
  }

  // Check if clone is available on ALL cluster members, to avoid a failing SQL
  // query because BACKUP_ADMIN is not supported
  bool clone_available_all_members = false;
  {
    get_default_replicaset()->execute_in_members(
        {mysqlshdk::gr::Member_state::ONLINE,
         mysqlshdk::gr::Member_state::RECOVERING},
        get_target_instance()->get_connection_options(), {},
        [&clone_available_all_members](
            const std::shared_ptr<mysqlshdk::db::ISession> &session) {
          mysqlsh::dba::Instance instance(session);
          mysqlshdk::utils::Version version = instance.get_version();
          if (version >=
              mysqlshdk::mysql::k_mysql_clone_plugin_initial_version) {
            clone_available_all_members = true;
          } else {
            clone_available_all_members = false;
          }

          return clone_available_all_members;
        });
  }

  // Check if clone is supported on the target instance
  bool clone_available_on_target =
      target->get_version() >=
      mysqlshdk::mysql::k_mysql_clone_plugin_initial_version;

  bool clone_available =
      clone_available_all_members && clone_available_on_target;

  return mysqlshdk::gr::create_recovery_user(
      recovery_user, *get_target_instance(), {"%"}, {}, clone_available);
}

void Cluster_impl::drop_replication_user(mysqlshdk::mysql::IInstance *target) {
  assert(target);
  auto console = current_console();

  std::string recovery_user;
  std::vector<std::string> recovery_user_hosts;
  bool from_metadata = false;
  try {
    std::tie(recovery_user, recovery_user_hosts, from_metadata) =
        get_replication_user(*target);
  } catch (const std::exception &e) {
    console->print_note(
        "The recovery user name for instance '" + target->descr() +
        "' does not match the expected format for users "
        "created automatically by InnoDB Cluster. Skipping its removal.");
  }
  if (!from_metadata) {
    log_info("Removing replication user '%s'", recovery_user.c_str());
    try {
      mysqlshdk::mysql::drop_all_accounts_for_user(*get_target_instance(),
                                                   recovery_user);
    } catch (const std::exception &e) {
      console->print_warning("Error dropping recovery accounts for user " +
                             recovery_user + ": " + e.what());
    }
  } else {
    /*
      Since clone copies all the data, including mysql.slave_master_info (where
      the CHANGE MASTER credentials are stored) an instance may be using the
      info stored in the donor's mysql.slave_master_info.

      To avoid such situation, we re-issue the CHANGE MASTER query after
      clone to ensure the right account is used. However, if the monitoring
      process is interrupted or waitRecovery = 0, that won't be done.

      The approach to tackle this issue is to store the donor recovery account
      in the target instance MD.instances table before doing the new CHANGE
      and only update it to the right account after a successful CHANGE MASTER.
      With this approach we can ensure that the account is not removed if it is
      being used.

      As so were we must query the Metadata to check whether the
      recovery user of that instance is registered for more than one instance
      and if that's the case then it won't be dropped.
    */
    if (get_metadata_storage()->is_recovery_account_unique(recovery_user)) {
      log_info("Dropping recovery account '%s'@'%%' for instance '%s'",
               recovery_user.c_str(), target->descr().c_str());

      try {
        get_target_instance()->drop_user(recovery_user, "%", true);
      } catch (const std::exception &e) {
        console->print_warning(shcore::str_format(
            "Error dropping recovery account '%s'@'%%' for instance '%s'",
            recovery_user.c_str(), target->descr().c_str()));
      }

      // Also update metadata
      try {
        get_metadata_storage()->update_instance_recovery_account(
            target->get_uuid(), "", "");
      } catch (const std::exception &e) {
        log_warning("Could not update recovery account metadata for '%s': %s",
                    target->descr().c_str(), e.what());
      }
    }
  }
}

bool Cluster_impl::contains_instance_with_address(
    const std::string &host_port) {
  return get_metadata_storage()->is_instance_on_cluster(get_id(), host_port);
}

/*
 * Ensures the cluster object (and metadata) can perform update operations.
 *
 * For a cluster to be updatable, it's necessary that:
 * - the MD object is connected to the PRIMARY of the
 * primary cluster, so that the MD can be updated (and is also not lagged)
 * - the primary of the cluster is reachable, so that cluster accounts
 * can be created there.
 *
 * An exception is thrown if not possible to connect to the primary.
 *
 * An Instance object connected to the primary is returned. The session
 * is owned by the cluster object.
 */
mysqlshdk::mysql::IInstance *Cluster_impl::ensure_updatable(bool reset) {
  auto console = current_console();

  if (!m_primary_master || reset) {
    m_primary_master = m_target_server;
    // m_primary_master->retain();
  }
  std::string uuid = m_primary_master->get_uuid();
  std::string primary_uuid;

  std::string primary_url =
      find_primary_member_uri(m_primary_master, false, nullptr);

  if (primary_url.empty()) {
    // Shouldn't happen, because validation for PRIMARY is done first
    throw std::logic_error("No PRIMARY member found");
  } else if (primary_url !=
             m_primary_master->get_connection_options().uri_endpoint()) {
    mysqlshdk::db::Connection_options copts(primary_url);
    log_info("Connecting to %s...", primary_url.c_str());
    copts.set_login_options_from(m_primary_master->get_connection_options());

    auto session = establish_session(copts, false);
    m_primary_master = std::make_shared<Instance>(session);

    _metadata_storage = std::make_shared<MetadataStorage>(m_primary_master);
  }

  return m_primary_master.get();
}

std::tuple<std::string, std::vector<std::string>, bool>
Cluster_impl::get_replication_user(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  // First check the metadata for the recovery user
  std::string recovery_user, recovery_user_host;
  bool from_metadata = false;
  std::vector<std::string> recovery_user_hosts;
  std::tie(recovery_user, recovery_user_host) =
      get_metadata_storage()->get_instance_recovery_account(
          target_instance.get_uuid());
  if (recovery_user.empty()) {
    // Assuming the account was created by an older version of the shell,
    // which did not store account name in metadata
    log_info(
        "No recovery account details in metadata for instance '%s', assuming "
        "old account style",
        target_instance.descr().c_str());

    recovery_user = mysqlshdk::gr::get_recovery_user(target_instance);
    assert(!recovery_user.empty());

    if (shcore::str_beginswith(
            recovery_user, mysqlshdk::gr::k_group_recovery_old_user_prefix)) {
      log_info("Found old account style recovery user '%s'",
               recovery_user.c_str());
      // old accounts were created for several hostnames
      recovery_user_hosts = mysqlshdk::mysql::get_all_hostnames_for_user(
          *(get_target_instance()), recovery_user);
    } else {
      // account not created by InnoDB cluster
      throw shcore::Exception::runtime_error(
          shcore::str_format("Recovery user '%s' not created by InnoDB Cluster",
                             recovery_user.c_str()));
    }
  } else {
    from_metadata = true;
    // new recovery user format, stored in Metadata.
    recovery_user_hosts.push_back(recovery_user_host);
  }
  return std::make_tuple(recovery_user, recovery_user_hosts, from_metadata);
}

std::unique_ptr<Instance> Cluster_impl::get_session_to_cluster_instance(
    const std::string &instance_address) const {
  // Set login credentials to connect to instance.
  // use the host and port from the instance address
  Connection_options instance_cnx_opts =
      shcore::get_connection_options(instance_address, false);
  // Use the password from the cluster session.
  Connection_options cluster_cnx_opt =
      get_target_instance()->get_connection_options();
  instance_cnx_opts.set_login_options_from(cluster_cnx_opt);

  std::shared_ptr<mysqlshdk::db::ISession> session;
  log_debug("Connecting to instance '%s'", instance_address.c_str());
  try {
    // Try to connect to instance
    session = mysqlshdk::db::mysql::Session::create();
    session->connect(instance_cnx_opts);
    log_debug("Successfully connected to instance");
    return std::make_unique<mysqlsh::dba::Instance>(session);
  } catch (const std::exception &err) {
    log_debug("Failed to connect to instance: %s", err.what());
    throw;
  }
}

size_t Cluster_impl::setup_clone_plugin(bool enable_clone) {
  // Get all cluster instances
  std::vector<Instance_metadata> instance_defs =
      get_default_replicaset()->get_instances();

  // Counter for instances that failed to be updated
  size_t count = 0;

  for (const Instance_metadata &instance_def : instance_defs) {
    std::string instance_address = instance_def.endpoint;

    // Establish a session to the cluster instance
    try {
      auto instance = get_session_to_cluster_instance(instance_address);

      // Handle the plugin setup only if the target instance supports it
      if (instance->get_version() >=
          mysqlshdk::mysql::k_mysql_clone_plugin_initial_version) {
        if (!enable_clone) {
          // Uninstall the clone plugin
          log_info("Uninstalling the clone plugin on instance '%s'.",
                   instance_address.c_str());
          mysqlshdk::mysql::uninstall_clone_plugin(*instance, nullptr);
        } else {
          // Install the clone plugin
          log_info("Installing the clone plugin on instance '%s'.",
                   instance_address.c_str());
          mysqlshdk::mysql::install_clone_plugin(*instance, nullptr);

          // Get the recovery account in use by the instance so the grant
          // BACKUP_ADMIN is granted to its recovery account
          {
            std::string recovery_user;
            std::vector<std::string> recovery_user_hosts;
            std::tie(recovery_user, recovery_user_hosts, std::ignore) =
                get_replication_user(*instance);

            // Add the BACKUP_ADMIN grant to the instance's recovery account
            // since it may not be there if the instance was added to a cluster
            // non-supporting clone
            for (const auto &host : recovery_user_hosts) {
              shcore::sqlstring grant("GRANT BACKUP_ADMIN ON *.* TO ?@?", 0);
              grant << recovery_user;
              grant << host;
              grant.done();
              get_target_instance()->execute(grant);
            }
          }
        }
      }

      // Close the instance's session
      instance->get_session()->close();
    } catch (const mysqlshdk::db::Error &err) {
      auto console = mysqlsh::current_console();

      std::string op = enable_clone ? "enable" : "disable";

      std::string err_msg = "Unable to " + op + " clone on the instance '" +
                            instance_address + "': " + std::string(err.what());

      // If a cluster member is unreachable, just print a warning. Otherwise
      // print error
      if (err.code() == CR_CONNECTION_ERROR ||
          err.code() == CR_CONN_HOST_ERROR) {
        console->print_warning(err_msg);
      } else {
        console->print_error(err_msg);
      }
      console->println();

      // It failed to update this instance, so increment the counter
      count++;
    }
  }

  return count;
}

}  // namespace dba
}  // namespace mysqlsh
