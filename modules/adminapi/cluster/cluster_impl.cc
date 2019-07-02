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
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/replicaset/add_instance.h"
#include "modules/adminapi/replicaset/remove_instance.h"
#include "modules/adminapi/replicaset/replicaset.h"
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
    const std::string &name,
    std::shared_ptr<mysqlshdk::db::ISession> group_session,
    std::shared_ptr<MetadataStorage> metadata_storage)
    : _name(name),
      _group_session(group_session),
      _metadata_storage(metadata_storage) {}

Cluster_impl::~Cluster_impl() {}

std::shared_ptr<mysqlshdk::innodbcluster::Metadata_mysql>
Cluster_impl::metadata() const {
  return _metadata_storage->get_new_metadata();
}

void Cluster_impl::insert_default_replicaset(bool multi_primary,
                                             bool is_adopted) {
  std::shared_ptr<ReplicaSet> default_rs = get_default_replicaset();

  // Check if we have a Default ReplicaSet, if so it means we already added the
  // Seed Instance
  if (default_rs != nullptr) {
    uint64_t rs_id = default_rs->get_id();
    if (!_metadata_storage->is_replicaset_empty(rs_id))
      throw shcore::Exception::logic_error(
          "Default ReplicaSet already initialized. Please use: addInstance() "
          "to add more Instances to the ReplicaSet.");
  } else {
    // Create the Default ReplicaSet and assign it to the Cluster's
    // default_replica_set var
    create_default_replicaset("default", multi_primary, "", is_adopted);
  }
}

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

void Cluster_impl::set_default_replicaset(const std::string &name,
                                          const std::string &topology_type,
                                          const std::string &group_name) {
  _default_replica_set = std::make_shared<ReplicaSet>(
      name, topology_type, group_name, _metadata_storage);

  _default_replica_set->set_cluster(this);
}

std::shared_ptr<ReplicaSet> Cluster_impl::create_default_replicaset(
    const std::string &name, bool multi_primary, const std::string &group_name,
    bool is_adopted) {
  std::string topology_type = ReplicaSet::kTopologySinglePrimary;
  if (multi_primary) {
    topology_type = ReplicaSet::kTopologyMultiPrimary;
  }
  _default_replica_set = std::make_shared<ReplicaSet>(
      name, topology_type, group_name, _metadata_storage);

  _default_replica_set->set_cluster(this);

  // Update the Cluster table with the Default ReplicaSet on the Metadata
  _metadata_storage->insert_replica_set(_default_replica_set, true, is_adopted);

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
  if (_group_session) {
    // no preconditions check needed for just disconnecting everything
    _group_session->close();
    _group_session.reset();
  }

  // If _group_session is the same as the session from the metadata,
  // then get_session would thrown an exception since the session is already
  // closed
  try {
    if (_metadata_storage->get_session()) {
      _metadata_storage->get_session()->close();
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
        shcore::make_unique<Cluster_set_option>(this, option, value_str);
  } else if (value.type == shcore::Integer || value.type == shcore::UInteger) {
    int64_t value_int = value.as_int();
    op_cluster_set_option =
        shcore::make_unique<Cluster_set_option>(this, option, value_int);
  } else if (value.type == shcore::Bool) {
    bool value_bool = value.as_bool();
    op_cluster_set_option =
        shcore::make_unique<Cluster_set_option>(this, option, value_bool);
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
                                      _group_session);
}

void Cluster_impl::sync_transactions(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  // Must get the value of the 'gtid_executed' variable with GLOBAL scope to get
  // the GTID of ALL transactions, otherwise only a set of transactions written
  // to the cache in the current session might be returned.
  std::string gtid_set = _group_session->query("SELECT @@GLOBAL.GTID_EXECUTED")
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
  mysqlshdk::mysql::Instance cluster_instance(get_group_session());
  mysqlshdk::utils::Version lowest_version = cluster_instance.get_version();

  // Get address of the cluster instance to skip it in the next step.
  std::string cluster_instance_address =
      cluster_instance.get_canonical_address();

  // Get the lowest server version from the available cluster instances.
  get_default_replicaset()->execute_in_members(
      {"'ONLINE'", "'RECOVERING'"},
      get_group_session()->get_connection_options(), {cluster_instance_address},
      [&lowest_version](
          const std::shared_ptr<mysqlshdk::db::ISession> &session) {
        mysqlshdk::mysql::Instance instance(session);
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
  mysqlshdk::mysql::Instance primary_master(get_group_session());
  std::string recovery_user =
      mysqlshdk::gr::k_group_recovery_user_prefix +
      std::to_string(target->get_sysvar_int("server_id").get_safe());

  log_info("Creating recovery account '%s'@'%%' for instance '%s'",
           recovery_user.c_str(), target->descr().c_str());

  // Check if the replication user already exists and delete it if it does,
  // before creating it again.
  bool rpl_user_exists = primary_master.user_exists(recovery_user, "%");
  if (rpl_user_exists) {
    auto console = current_console();
    console->print_warning(shcore::str_format(
        "User '%s'@'%%' already existed at instance '%s'. It will be "
        "deleted and created again with a new password.",
        recovery_user.c_str(), primary_master.descr().c_str()));
    primary_master.drop_user(recovery_user, "%");
  }

  // Check if clone is available on ALL cluster members, to avoid a failing SQL
  // query because BACKUP_ADMIN is not supported
  bool clone_available_all_members = false;
  {
    get_default_replicaset()->execute_in_members(
        {"'ONLINE'", "'RECOVERING'"},
        get_group_session()->get_connection_options(), {},
        [&clone_available_all_members](
            const std::shared_ptr<mysqlshdk::db::ISession> &session) {
          mysqlshdk::mysql::Instance instance(session);
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

  return mysqlshdk::gr::create_recovery_user(recovery_user, &primary_master,
                                             {"%"}, {}, clone_available);
}

void Cluster_impl::drop_replication_user(mysqlshdk::mysql::IInstance *target) {
  assert(target);
  mysqlshdk::mysql::Instance primary_master(get_group_session());
  auto console = current_console();

  std::string recovery_user, recovery_user_host;
  std::tie(recovery_user, recovery_user_host) =
      get_metadata_storage()->get_instance_recovery_account(target->get_uuid());
  if (recovery_user.empty()) {
    // Assuming the account was created by an older version of the shell,
    // which did not store account name in metadata
    log_info(
        "No recovery account details in metadata for instance '%s', assuming "
        "old account style",
        target->descr().c_str());

    // Get replication user (recovery) used by the instance to remove
    // on remaining members.
    recovery_user = mysqlshdk::gr::get_recovery_user(*target);

    // Remove the replication user used by the removed instance on all
    // cluster members through the primary (using replication).
    // NOTE: Make sure to remove the user if it was an user created by
    // the Shell, i.e. with the format: mysql_innodb_cluster_r[0-9]{10}
    if (shcore::str_beginswith(
            recovery_user, mysqlshdk::gr::k_group_recovery_old_user_prefix)) {
      log_info("Removing replication user '%s'", recovery_user.c_str());
      try {
        mysqlshdk::mysql::drop_all_accounts_for_user(primary_master,
                                                     recovery_user);
      } catch (const std::exception &e) {
        console->print_warning("Error dropping recovery accounts for user " +
                               recovery_user + ": " + e.what());
      }
    } else {
      console->print_note("The recovery user name for instance '" +
                          target->descr() +
                          "' does not match the expected format for users "
                          "created automatically "
                          "by InnoDB Cluster. Skipping its removal.");
    }
  } else {
    /*
      Since clone copies all the data, including mysql.slave_master_info (where
      the CHANGE MASTER credentials are stored) an instance may be using the
      info stored in the donor's mysql.slave_master_info.

      To avoid such situation, we re-issue the CHANGE MASTER query after
      clone to ensure the right account is used. However, if the monitoring
      process is interruped or waitRecovery = 0, that won't be done.

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
      log_info("Dropping recovery account '%s'@'%s' for instance '%s'",
               recovery_user.c_str(), recovery_user_host.c_str(),
               target->descr().c_str());

      try {
        primary_master.drop_user(recovery_user, "%", true);
      } catch (const std::exception &e) {
        console->print_warning(shcore::str_format(
            "Error dropping recovery account '%s'@'%s' for instance '%s'",
            recovery_user.c_str(), recovery_user_host.c_str(),
            target->descr().c_str()));
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

}  // namespace dba
}  // namespace mysqlsh
