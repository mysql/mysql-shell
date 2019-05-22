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

#include "modules/adminapi/replicaset/replicaset.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster/dissolve.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/replicaset/add_instance.h"
#include "modules/adminapi/replicaset/check_instance_state.h"
#include "modules/adminapi/replicaset/remove_instance.h"
#include "modules/adminapi/replicaset/rescan.h"
#include "modules/adminapi/replicaset/set_instance_option.h"
#include "modules/adminapi/replicaset/set_primary_instance.h"
#include "modules/adminapi/replicaset/switch_to_multi_primary_mode.h"
#include "modules/adminapi/replicaset/switch_to_single_primary_mode.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_shell.h"
#include "modules/mysqlxtest_utils.h"
#include "my_dbug.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "shellcore/base_session.h"
#include "utils/utils_general.h"
#include "utils/utils_net.h"
#include "utils/utils_sqlstring.h"
#include "utils/utils_string.h"

namespace mysqlsh {
namespace dba {

using mysqlshdk::db::uri::formats::only_transport;
using mysqlshdk::db::uri::formats::user_transport;

// TODO(nelson): update the values to sm and mp respectively on the next version
// bump
char const *ReplicaSet::kTopologySinglePrimary = "pm";
char const *ReplicaSet::kTopologyMultiPrimary = "mm";

const char *ReplicaSet::kWarningDeprecateSslMode =
    "Option 'memberSslMode' is deprecated for this operation and it will be "
    "removed in a future release. This option is not needed because the SSL "
    "mode is automatically obtained from the cluster. Please do not use it "
    "here.";

ReplicaSet::ReplicaSet(const std::string &name,
                       const std::string &topology_type,
                       const std::string &group_name,
                       std::shared_ptr<MetadataStorage> metadata_storage)
    : _name(name),
      _topology_type(topology_type),
      _group_name(group_name),
      _metadata_storage(metadata_storage) {
  assert(topology_type == kTopologyMultiPrimary ||
         topology_type == kTopologySinglePrimary);
}

ReplicaSet::~ReplicaSet() {}

void ReplicaSet::sanity_check() const { verify_topology_type_change(); }

/*
 * Verify if the topology type changed and issue an error if needed.
 */
void ReplicaSet::verify_topology_type_change() const {
  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  Cluster_impl *cluster(get_cluster());

  std::string gr_primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      cluster->get_group_session(), nullptr);

  // Check if the topology type matches the real settings used by the
  // cluster instance, otherwise an error is issued.
  // NOTE: The GR primary mode is guaranteed (by GR) to be the same for all
  // instance of the same group.
  if (!gr_primary_uuid.empty() && _topology_type == kTopologyMultiPrimary)
    throw shcore::Exception::runtime_error(
        "The InnoDB Cluster topology type (Multi-Primary) does not match the "
        "current Group Replication configuration (Single-Primary). Please "
        "use <cluster>.rescan() or change the Group Replication "
        "configuration accordingly.");
  else if (gr_primary_uuid.empty() && _topology_type == kTopologySinglePrimary)
    throw shcore::Exception::runtime_error(
        "The InnoDB Cluster topology type (Single-Primary) does not match the "
        "current Group Replication configuration (Multi-Primary). Please "
        "use <cluster>.rescan() or change the Group Replication "
        "configuration accordingly.");
}

void ReplicaSet::set_instance_option(const Connection_options &instance_def,
                                     const std::string &option,
                                     const shcore::Value &value) {
  // Set ReplicaSet configuration option

  // Create the Replicaset Set_instance_option object and execute it.
  std::unique_ptr<Set_instance_option> op_set_instance_option;

  // Validation types due to a limitation on the expose() framework.
  // Currently, it's not possible to do overloading of functions that overload
  // an argument of type string/int since the type int is convertible to
  // string, thus overloading becomes ambiguous. As soon as that limitation is
  // gone, this type checking shall go away too.
  if (value.type == shcore::String) {
    std::string value_str = value.as_string();
    op_set_instance_option = shcore::make_unique<Set_instance_option>(
        *this, instance_def, option, value_str);
  } else if (value.type == shcore::Integer || value.type == shcore::UInteger) {
    int64_t value_int = value.as_int();
    op_set_instance_option = shcore::make_unique<Set_instance_option>(
        *this, instance_def, option, value_int);
  } else {
    throw shcore::Exception::argument_error(
        "Argument #2 is expected to be a string or an Integer.");
  }

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_set_instance_option]() { op_set_instance_option->finish(); });

  // Prepare the Replicaset_set_instance_option command execution (validations).
  op_set_instance_option->prepare();

  // Execute Replicaset_set_instance_option operations.
  op_set_instance_option->execute();
}

void ReplicaSet::adopt_from_gr() {
  shcore::Value ret_val;
  auto console = mysqlsh::current_console();

  auto newly_discovered_instances_list(
      get_newly_discovered_instances(_metadata_storage, _id));

  // Add all instances to the cluster metadata
  for (NewInstanceInfo &instance : newly_discovered_instances_list) {
    mysqlshdk::db::Connection_options newly_discovered_instance;

    newly_discovered_instance.set_host(instance.host);
    newly_discovered_instance.set_port(instance.port);

    log_info("Adopting member %s:%d from existing group", instance.host.c_str(),
             instance.port);
    console->println("Adding Instance '" + instance.host + ":" +
                     std::to_string(instance.port) + "'...");

    // TODO(somebody): what if the password is different on each server?
    // And what if is different from the current session?
    auto session = _metadata_storage->get_session();

    auto session_data = session->get_connection_options();

    newly_discovered_instance.set_user(session_data.get_user());
    newly_discovered_instance.set_password(session_data.get_password());

    add_instance_metadata(newly_discovered_instance);
  }
}

void ReplicaSet::add_instance(
    const mysqlshdk::db::Connection_options &connection_options,
    const shcore::Dictionary_t &options) {
  Group_replication_options gr_options(Group_replication_options::JOIN);
  mysqlshdk::utils::nullable<std::string> label;
  std::string password;

  // Get optional options.
  if (options) {
    // Retrieves optional options if exists
    Unpack_options(options)
        .unpack(&gr_options)
        .optional("label", &label)
        .optional_ci("password", &password)
        .end();
  }

  // Override password if provided in options dictionary.
  auto target_coptions = mysqlshdk::db::Connection_options(
      connection_options.as_uri(mysqlshdk::db::uri::formats::full()));
  if (!password.empty()) {
    target_coptions.set_password(password);
  }

  // Validate the label value.
  if (!label.is_null()) {
    mysqlsh::dba::validate_label(*label);

    if (!_metadata_storage->is_instance_label_unique(get_id(), *label))
      throw shcore::Exception::argument_error(
          "An instance with label '" + *label +
          "' is already part of this InnoDB cluster");
  }

  // Add the Instance to the ReplicaSet
  {
    // Create the add_instance command and execute it.
    Add_instance op_add_instance(target_coptions, *this, gr_options, label);

    // Always execute finish when leaving "try catch".
    auto finally = shcore::on_leave_scope(
        [&op_add_instance]() { op_add_instance.finish(); });

    // Prepare the add_instance command execution (validations).
    op_add_instance.prepare();

    // Execute add_instance operations.
    op_add_instance.execute();
  }
}

namespace {
template <typename T>
struct Option_info {
  bool found_non_default = false;
  bool found_not_supported = false;
  T non_default_value;
};
}  // namespace

void ReplicaSet::query_group_wide_option_values(
    mysqlshdk::mysql::IInstance *target_instance,
    mysqlshdk::utils::nullable<std::string> *out_gr_consistency,
    mysqlshdk::utils::nullable<int64_t> *out_gr_member_expel_timeout) const {
  auto console = mysqlsh::current_console();

  Option_info<std::string> gr_consistency;
  Option_info<int64_t> gr_member_expel_timeout;

  // loop though all members to check if there is any member that doesn't:
  // - have support for the group_replication_consistency option (null value)
  // or a member that doesn't have the default value. The default value
  // Eventual has the same behavior as members had before the option was
  // introduced. As such, having that value or having no support for the
  // group_replication_consistency is the same.
  // - have support for the group_replication_member_expel_timeout option
  // (null value) or a member that doesn't have the default value. The default
  // value 0 has the same behavior as members had before the option was
  // introduced. As such, having the 0 value or having no support for the
  // group_replication_member_expel_timeout is the same.
  execute_in_members(
      {"'ONLINE'", "'RECOVERING'"}, target_instance->get_connection_options(),
      {},
      [&gr_consistency, &gr_member_expel_timeout](
          const std::shared_ptr<mysqlshdk::db::ISession> &session) {
        mysqlshdk::mysql::Instance instance(session);

        {
          mysqlshdk::utils::nullable<std::string> value;
          value = instance.get_sysvar_string(
              "group_replication_consistency",
              mysqlshdk::mysql::Var_qualifier::GLOBAL);

          if (value.is_null()) {
            gr_consistency.found_not_supported = true;
          } else if (*value != "EVENTUAL" && *value != "0") {
            gr_consistency.found_non_default = true;
            gr_consistency.non_default_value = *value;
          }
        }

        {
          mysqlshdk::utils::nullable<std::int64_t> value;
          value =
              instance.get_sysvar_int("group_replication_member_expel_timeout",
                                      mysqlshdk::mysql::Var_qualifier::GLOBAL);

          if (value.is_null()) {
            gr_member_expel_timeout.found_not_supported = true;
          } else if (*value != 0) {
            gr_member_expel_timeout.found_non_default = true;
            gr_member_expel_timeout.non_default_value = *value;
          }
        }
        // if we have found both a instance that doesnt have support for the
        // option and an instance that doesn't have the default value, then we
        // don't need to look at any other instance on the cluster.
        return !(gr_consistency.found_not_supported &&
                 gr_consistency.found_non_default &&
                 gr_member_expel_timeout.found_not_supported &&
                 gr_member_expel_timeout.found_non_default);
      });

  if (target_instance->get_version() < mysqlshdk::utils::Version(8, 0, 14)) {
    if (gr_consistency.found_non_default) {
      console->print_warning(
          "The " + gr_consistency.non_default_value +
          " consistency value of the cluster "
          "is not supported by the instance '" +
          target_instance->get_connection_options().uri_endpoint() +
          "' (version >= 8.0.14 is required). In single-primary mode, upon "
          "failover, the member with the lowest version is the one elected as"
          " primary.");
    }
  } else {
    *out_gr_consistency = "EVENTUAL";

    if (gr_consistency.found_non_default) {
      // if we found any non default group_replication_consistency value, then
      // we use that value on the instance being added
      *out_gr_consistency = gr_consistency.non_default_value;

      if (gr_consistency.found_not_supported) {
        console->print_warning(
            "The instance '" +
            target_instance->get_connection_options().uri_endpoint() +
            "' inherited the " + gr_consistency.non_default_value +
            " consistency value from the cluster, however some instances on "
            "the group do not support this feature (version < 8.0.14). In "
            "single-primary mode, upon failover, the member with the lowest "
            "version will be the one elected and it doesn't support this "
            "option.");
      }
    }
  }

  if (target_instance->get_version() < mysqlshdk::utils::Version(8, 0, 13)) {
    if (gr_member_expel_timeout.found_non_default) {
      console->print_warning(
          "The expelTimeout value of the cluster '" +
          std::to_string(gr_member_expel_timeout.non_default_value) +
          "' is not supported by the instance '" +
          target_instance->get_connection_options().uri_endpoint() +
          "' (version >= 8.0.13 is required). A member "
          "that doesn't have support for the expelTimeout option has the "
          "same behavior as a member with expelTimeout=0.");
    }
  } else {
    *out_gr_member_expel_timeout = int64_t();

    if (gr_member_expel_timeout.found_non_default) {
      // if we found any non default group_replication_member_expel_timeout
      // value, then we use that value on the instance being added
      *out_gr_member_expel_timeout = gr_member_expel_timeout.non_default_value;

      if (gr_member_expel_timeout.found_not_supported) {
        console->print_warning(
            "The instance '" +
            target_instance->get_connection_options().uri_endpoint() +
            "' inherited the '" +
            std::to_string(gr_member_expel_timeout.non_default_value) +
            "' consistency value from the cluster, however some instances on "
            "the group do not support this feature (version < 8.0.13). There "
            "is a possibility that the cluster member (killer node), "
            "responsible for expelling the member suspected of having "
            "failed, does not support the expelTimeout option. In "
            "this case the behavior would be the same as if having "
            "expelTimeout=0.");
      }
    }
  }
}

/**
 * Get an up-to-date group seeds value based on the current list of active
 * members.
 *
 * An optional instance_session parameter can be provide that will be used to
 * get its current GR group seeds value and add the local address from the
 * active members (avoiding duplicates) to that initial value, allowing to
 * preserve the GR group seeds of the specified instance. If no
 * instance_session is given (nullptr) then the returned groups seeds value
 * will only be based on the currently active members, disregarding any current
 * GR group seed value on any instance (allowing to reset the group seed only
 * based on the currently active members).
 *
 * @param instance_session Session to the target instance to get the group
 *                         seeds for.
 * @return a string with a comma separated list of all the GR local address
 *         values of the currently active (ONLINE or RECOVERING) instances in
 *         the replicaset.
 */
std::string ReplicaSet::get_cluster_group_seeds(
    const std::shared_ptr<mysqlshdk::db::ISession> &instance_session) const {
  // Get connection option for the metadata.
  Cluster_impl *cluster(get_cluster());

  std::shared_ptr<mysqlshdk::db::ISession> cluster_session =
      cluster->get_group_session();
  Connection_options cluster_cnx_opt =
      cluster_session->get_connection_options();

  // Get list of active instances (ONLINE or RECOVERING)
  std::vector<Instance_definition> active_instances =
      _metadata_storage->get_replicaset_active_instances(_id);

  std::vector<std::string> gr_group_seeds_list;
  // If the target instance is provided, use its current GR group seed variable
  // value as starting point to append new (missing) values to it.
  if (instance_session) {
    auto instance = mysqlshdk::mysql::Instance(instance_session);
    // Get the instance GR group seeds and save it to the GR group seeds list.
    std::string gr_group_seeds =
        *instance.get_sysvar_string("group_replication_group_seeds",
                                    mysqlshdk::mysql::Var_qualifier::GLOBAL);
    if (!gr_group_seeds.empty()) {
      gr_group_seeds_list = shcore::split_string(gr_group_seeds, ",");
    }
  }

  // Get the update GR group seed from local address of all active instances.
  for (Instance_definition &instance_def : active_instances) {
    std::string instance_address = instance_def.endpoint;
    Connection_options target_coptions =
        shcore::get_connection_options(instance_address, false);
    // It is assumed that the same user and password is used by all members.
    if (cluster_cnx_opt.has_user())
      target_coptions.set_user(cluster_cnx_opt.get_user());
    if (cluster_cnx_opt.has_password())
      target_coptions.set_password(cluster_cnx_opt.get_password());
    // Connect to the instance.
    std::shared_ptr<mysqlshdk::db::ISession> session;
    try {
      log_debug(
          "Connecting to instance '%s' to get its value for the "
          "group_replication_local_address variable.",
          instance_address.c_str());
      session = establish_mysql_session(target_coptions,
                                        current_shell_options()->get().wizards);
    } catch (const std::exception &e) {
      // Do not issue an error if we are unable to connect to the instance,
      // it might have failed in the meantime, just skip the use of its GR
      // local address.
      log_info(
          "Could not connect to instance '%s', its local address will not "
          "be used for the group seeds: %s",
          instance_address.c_str(), e.what());
      break;
    }
    auto instance = mysqlshdk::mysql::Instance(session);
    // Get the instance GR local address and add it to the GR group seeds list.
    std::string local_address =
        *instance.get_sysvar_string("group_replication_local_address",
                                    mysqlshdk::mysql::Var_qualifier::GLOBAL);
    if (std::find(gr_group_seeds_list.begin(), gr_group_seeds_list.end(),
                  local_address) == gr_group_seeds_list.end()) {
      // Only add the local address if not already in the group seed list,
      // avoiding duplicates.
      gr_group_seeds_list.push_back(local_address);
    }
    session->close();
  }
  return shcore::str_join(gr_group_seeds_list, ",");
}

shcore::Value ReplicaSet::rejoin_instance(
    mysqlshdk::db::Connection_options *instance_def,
    const shcore::Value::Map_type_ref &rejoin_options) {
  Cluster_impl *cluster(get_cluster());

  shcore::Value ret_val;
  Group_replication_options gr_options(Group_replication_options::REJOIN);
  // SSL Mode AUTO by default
  gr_options.ssl_mode = mysqlsh::dba::kMemberSSLModeAuto;
  std::string user, password;
  shcore::Value::Array_type_ref errors;
  std::shared_ptr<mysqlshdk::db::ISession> session, seed_session;

  auto console = mysqlsh::current_console();

  // Retrieves the options
  if (rejoin_options) {
    Unpack_options(rejoin_options).unpack(&gr_options).end();

    if (rejoin_options->has_key("memberSslMode")) {
      console->print_warning(kWarningDeprecateSslMode);
      console->println();
    }
  }

  if (!instance_def->has_port())
    instance_def->set_port(mysqlshdk::db::k_default_mysql_port);

  instance_def->set_default_connection_data();

  // Before rejoining an instance we must verify if the instance's
  // 'group_replication_group_name' matches the one registered in the
  // Metadata (BUG #26159339)
  //
  // Before rejoining an instance we must also verify if the group has quorum
  // and if the gr plugin is active otherwise we may end up hanging the system

  mysqlshdk::mysql::Instance instance;

  // Validate 'group_replication_group_name'
  {
    try {
      log_info("Opening a new session to the rejoining instance %s",
               instance_def->uri_endpoint().c_str());
      session = establish_mysql_session(*instance_def,
                                        current_shell_options()->get().wizards);
    } catch (const std::exception &e) {
      std::string err_msg = "Could not open connection to '" +
                            instance_def->uri_endpoint() + "': " + e.what();
      throw shcore::Exception::runtime_error(err_msg);
    }

    // Get instance address in metadata.
    instance = mysqlshdk::mysql::Instance(session);

    // Perform quick config checks
    ensure_instance_configuration_valid(instance, false);

    std::string md_address = instance.get_canonical_address();

    // Check if the instance is part of the Metadata
    if (!_metadata_storage->is_instance_on_replicaset(get_id(), md_address)) {
      std::string message = "The instance '" + instance_def->uri_endpoint() +
                            "' " + "does not belong to the ReplicaSet: '" +
                            get_name() + "'.";

      throw shcore::Exception::runtime_error(message);
    }

    gr_options.check_option_values(instance.get_version());

    if (!validate_replicaset_group_name(session, get_group_name())) {
      std::string nice_error =
          "The instance '" + instance_def->uri_endpoint() +
          "' "
          "may belong to a different ReplicaSet as the one registered "
          "in the Metadata since the value of "
          "'group_replication_group_name' does not match the one "
          "registered in the ReplicaSet's Metadata: possible split-brain "
          "scenario. Please remove the instance from the cluster.";

      session->close();

      throw shcore::Exception::runtime_error(nice_error);
    }
  }

  // In order to be able to rejoin the instance to the cluster we need the seed
  // instance.

  // Get the seed instance
  {
    mysqlshdk::db::Connection_options seed_instance(pick_seed_instance());

    // To be able to establish a session to the seed instance we need a username
    // and password. Taking into consideration the assumption that all instances
    // of the cluster use the same credentials we can obtain the ones of the
    // current target group session

    seed_instance.set_login_options_from(
        cluster->get_group_session()->get_connection_options());

    // Establish a session to the seed instance
    try {
      log_info("Opening a new session to seed instance: %s",
               seed_instance.uri_endpoint().c_str());
      seed_session = establish_mysql_session(
          seed_instance, current_shell_options()->get().wizards);
    } catch (const std::exception &e) {
      throw shcore::Exception::runtime_error("Could not open a connection to " +
                                             seed_instance.uri_endpoint() +
                                             ": " + e.what() + ".");
    }
  }
  auto seed_instance = mysqlshdk::mysql::Instance(seed_session);

  // Verify if the instance is running asynchronous (master-slave) replication
  {
    auto console = mysqlsh::current_console();

    log_debug(
        "Checking if instance '%s' is running asynchronous (master-slave) "
        "replication.",
        instance.descr().c_str());

    if (mysqlshdk::mysql::is_async_replication_running(instance)) {
      console->print_error(
          "Cannot rejoin instance '" + instance.descr() +
          "' to the cluster because it has asynchronous (master-slave) "
          "replication configured and running. Please stop the slave threads "
          "by executing the query: 'STOP SLAVE;'");

      throw shcore::Exception::runtime_error(
          "The instance '" + instance.descr() +
          "' is running asynchronous (master-slave) replication.");
    }
  }

  // Verify if the group_replication plugin is active on the seed instance
  {
    log_info(
        "Verifying if the group_replication plugin is active on the seed "
        "instance %s",
        seed_instance.descr().c_str());

    auto plugin_status = seed_instance.get_plugin_status("group_replication");

    if (plugin_status.get_safe() != "ACTIVE") {
      throw shcore::Exception::runtime_error(
          "Cannot rejoin instance. The seed instance doesn't have "
          "group-replication active.");
    }
  }

  // Verify if the instance being added is MISSING, otherwise throw an error
  // Bug#26870329
  {
    // get server_uuid from the instance that we're trying to rejoin
    if (!validate_instance_rejoinable(instance, _metadata_storage, _id)) {
      // instance not missing, so throw an error
      auto member_state =
          mysqlshdk::gr::to_string(mysqlshdk::gr::get_member_state(instance));
      std::string nice_error_msg = "Cannot rejoin instance '" +
                                   instance.descr() + "' to the ReplicaSet '" +
                                   get_name() + "' since it is an active (" +
                                   member_state + ") member of the ReplicaSet.";
      session->close();
      throw shcore::Exception::runtime_error(nice_error_msg);
    }
  }
  {
    // Check if instance was doing auto-rejoin and let the user know that the
    // rejoin operation will override the auto-rejoin
    bool is_running_rejoin = mysqlshdk::gr::is_running_gr_auto_rejoin(instance);
    if (is_running_rejoin) {
      console->print_info(
          "The instance '" + instance.get_connection_options().uri_endpoint() +
          "' is running auto-rejoin process, however the rejoinInstance has "
          "precedence and will override that process.");
      console->println();
    }
  }
  // Get the up-to-date GR group seeds values (with the GR local address from
  // all currently active instances).
  gr_options.group_seeds = get_cluster_group_seeds(session);

  // join Instance to cluster
  {
    std::string replication_user, replication_user_pwd;

    std::string new_ssl_mode;
    // Resolve the SSL Mode to use to configure the instance.
    new_ssl_mode = resolve_instance_ssl_mode(
        instance, mysqlshdk::mysql::Instance(seed_session),
        *gr_options.ssl_mode);
    if (gr_options.ssl_mode.is_null() || new_ssl_mode != *gr_options.ssl_mode) {
      gr_options.ssl_mode = new_ssl_mode;
      log_warning("SSL mode used to configure the instance: '%s'",
                  gr_options.ssl_mode->c_str());
    }

    // Get SSL values to connect to peer instance
    auto seed_instance_def = seed_session->get_connection_options();

    // Stop group-replication
    log_info("Stopping group-replication at instance %s",
             session->get_connection_options().uri_endpoint().c_str());
    session->execute("STOP GROUP_REPLICATION");

    // F4. When a valid 'ipWhitelist' is used on the .rejoinInstance() command,
    // the previously existing "replication-user" must be removed from all the
    // cluster members and a new one created to match the 'ipWhitelist' defined
    // filter.
    bool keep_repl_user =
        gr_options.ip_whitelist.is_null() || gr_options.ip_whitelist->empty();

    if (!keep_repl_user) {
      mysqlshdk::mysql::Instance instance(seed_session);

      log_info("Recreating replication accounts due to 'ipWhitelist' change.");

      // Remove all the replication users of the instance and the
      // replication-user of the rejoining instance on all the members of the
      // replicaSet
      remove_replication_users(mysqlshdk::mysql::Instance(session), true);

      // Create a new replication user to match the ipWhitelist filter
      mysqlshdk::gr::create_replication_random_user_pass(
          instance, &replication_user,
          convert_ipwhitelist_to_netmask(gr_options.ip_whitelist.get_safe()),
          &replication_user_pwd);

      log_debug("Created replication user '%s'", replication_user.c_str());
    }

    // Get the seed session connection data
    // use mysqlprovision to rejoin the cluster.
    // on the rejoin operation there is no need adjust the the number of
    // members on the replicaset
    // Get the target and peer instance.
    auto peer_instance = mysqlshdk::mysql::Instance(seed_session);
    auto target_instance = mysqlshdk::mysql::Instance(session);

    // Handling of GR protocol version
    {
      // Get the current protocol version in use in the group
      mysqlshdk::utils::Version gr_protocol_version =
          mysqlshdk::gr::get_group_protocol_version(peer_instance);

      // If the target instance being rejoined does not support the GR protocol
      // version in use on the group (because it is an older version), the
      // rejoinInstance command must set the GR protocol of the cluster to the
      // version of the target instance.
      try {
        if (mysqlshdk::gr::is_protocol_downgrade_required(gr_protocol_version,
                                                          target_instance)) {
          mysqlshdk::gr::set_group_protocol_version(
              peer_instance, target_instance.get_version());
        }
      } catch (const shcore::Exception &error) {
        // The UDF may fail with MySQL Error 1123 if any of the members is
        // RECOVERING In such scenario, we must abort the upgrade protocol
        // version process and warn the user
        if (error.code() == ER_CANT_INITIALIZE_UDF) {
          auto console = mysqlsh::current_console();
          console->print_note(
              "Unable to determine the Group Replication protocol version, "
              "while verifying if a protocol upgrade would be possible: " +
              std::string(error.what()) + ".");
        } else {
          throw;
        }
      }

      // BUG#29265869: reboot cluster overrides some GR settings.
      // Read actual GR configurations to preserve them when rejoining the
      // instance.
      gr_options.read_option_values(target_instance);
    }

    // Resolve GR local address.
    // NOTE: Must be done only after getting the report_host used by GR and for
    //       the metadata;
    int port = target_instance.get_canonical_port();
    std::string hostname = target_instance.get_canonical_hostname();
    gr_options.local_address = mysqlsh::dba::resolve_gr_local_address(
        gr_options.local_address, hostname, port);

    // Set a Config object for the target instance (required to configure GR).
    std::unique_ptr<mysqlshdk::config::Config> cfg = create_server_config(
        &target_instance, mysqlshdk::config::k_dft_cfg_server_handler);

    // (Re-)join the instance to the replicaset (setting up GR properly).
    // NOTE: on the rejoin operation there is no need adjust the the number of
    //       members on the replicaset
    mysqlshdk::utils::nullable<uint64_t> replicaset_count;
    mysqlsh::dba::join_replicaset(target_instance, peer_instance,
                                  replication_user, replication_user_pwd,
                                  gr_options, replicaset_count, cfg.get());
    log_info("The instance '%s' was successfully rejoined on the cluster.",
             seed_instance.descr().c_str());
  }
  return ret_val;
}

shcore::Value ReplicaSet::remove_instance(const shcore::Argument_list &args) {
  mysqlshdk::utils::nullable<bool> force;
  bool interactive;
  std::string password;
  mysqlshdk::db::Connection_options target_coptions;

  // Get target instance connection options.
  target_coptions =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

  interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (args.size() == 2) {
    Unpack_options(args.map_at(1))
        .optional("force", &force)
        .optional("interactive", &interactive)
        .optional_ci("password", &password)
        .end();
  }

  // Overide password if provided in options dictionary.
  if (!password.empty()) {
    target_coptions.set_password(password);
  }

  // Remove the Instance from the ReplicaSet
  try {
    // Create the remove_instance command and execute it.
    Remove_instance op_remove_instance(target_coptions, interactive, force,
                                       *this);
    // Always execute finish when leaving "try catch".
    auto finally = shcore::on_leave_scope(
        [&op_remove_instance]() { op_remove_instance.finish(); });
    // Prepare the remove_instance command execution (validations).
    op_remove_instance.prepare();
    // Execute remove_instance operations.
    op_remove_instance.execute();
  } catch (...) {
    throw;
  }

  return shcore::Value();
}

/**
 * Auxiliary function to re-enable super_read_only.
 *
 * @param super_read_only boolean with the initial super_read_only value.
 * @param instance target instance object.
 * @param instance_address string with the target instance address.
 */
static void reenable_super_read_only(
    const mysqlshdk::utils::nullable<bool> &super_read_only,
    const mysqlshdk::mysql::Instance &instance,
    const std::string &instance_address) {
  // Re-enable super_read_only if previously enabled.
  if (*super_read_only) {
    log_debug("Re-enabling super_read_only on instance '%s'.",
              instance_address.c_str());
    instance.set_sysvar("super_read_only", true,
                        mysqlshdk::mysql::Var_qualifier::GLOBAL);
  }
}

void ReplicaSet::update_group_members_for_removed_member(
    const std::string &local_gr_address,
    const mysqlshdk::mysql::Instance &instance, bool remove_rpl_user_on_group) {
  // Get the ReplicaSet Config Object
  auto cfg = create_config_object({}, true);

  // Iterate through all ONLINE and RECOVERING cluster members and update their
  // group_replication_group_seeds value by removing the gr_local_address
  // of the instance that was removed
  log_debug("Updating group_replication_group_seeds of cluster members");
  mysqlshdk::gr::update_group_seeds(
      cfg.get(), local_gr_address, mysqlshdk::gr::Gr_seeds_change_type::REMOVE);
  cfg->apply();

  // Remove the replication users on the instance and members if
  // remove_rpl_user_on_group = true.
  if (remove_rpl_user_on_group) {
    log_debug("Removing replication user on instance and replicaset members");
  } else {
    log_debug("Removing replication user on instance");
  }
  remove_replication_users(instance, remove_rpl_user_on_group);

  // Update the auto-increment values
  {
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
    // We must update the auto-increment values in Remove_instance for 2
    // scenarios
    //   - Multi-primary Replicaset
    //   - Replicaset that had more 7 or more members before the Remove_instance
    //     operation
    //
    // NOTE: in the other scenarios, the Add_instance operation is in charge of
    // updating auto-increment accordingly

    mysqlshdk::gr::Topology_mode topology_mode =
        get_cluster()->get_metadata_storage()->get_replicaset_topology_mode(
            get_id());

    // Get the current number of members of the Replicaset
    uint64_t replicaset_count =
        get_cluster()->get_metadata_storage()->get_replicaset_count(get_id());

    bool update_auto_inc = (replicaset_count + 1) > 7 ? true : false;

    if (topology_mode == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY &&
        update_auto_inc) {
      // Call update_auto_increment to do the job in all instances
      mysqlshdk::gr::update_auto_increment(
          cfg.get(), mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

      cfg->apply();
    }
  }
}

void ReplicaSet::remove_replication_users(
    const mysqlshdk::mysql::Instance &instance, bool remove_rpl_user_on_group) {
  std::string instance_address =
      instance.get_connection_options().as_uri(only_transport());
  // Check if super_read_only is enabled and disable it to remove replication
  // users and metadata.
  mysqlshdk::utils::nullable<bool> super_read_only = instance.get_sysvar_bool(
      "super_read_only", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  if (*super_read_only) {
    log_debug(
        "Disabling super_read_only to remove replication users on "
        "instance '%s'.",
        instance_address.c_str());
    instance.set_sysvar("super_read_only", false,
                        mysqlshdk::mysql::Var_qualifier::GLOBAL);
  }

  // Remove all replication (recovery users) on the removed instance,
  // disabling binary logging (avoid being replicated).
  try {
    // Re-enable super_read_only if previously enabled when leaving "try catch".
    auto finally =
        shcore::on_leave_scope([super_read_only, instance, instance_address]() {
          reenable_super_read_only(super_read_only, instance, instance_address);
        });
    instance.set_sysvar("sql_log_bin", static_cast<const int64_t>(0),
                        mysqlshdk::mysql::Var_qualifier::SESSION);

    log_debug("Removing InnoDB Cluster replication users on instance '%s'.",
              instance_address.c_str());
    instance.drop_users_with_regexp("'mysql_innodb_cluster_r[0-9]{10}.*");

    instance.set_sysvar("sql_log_bin", static_cast<const int64_t>(1),
                        mysqlshdk::mysql::Var_qualifier::SESSION);
  } catch (const shcore::Exception &err) {
    throw;
  }

  if (remove_rpl_user_on_group) {
    // Get replication user (recovery) used by the instance to remove
    // on remaining members.
    std::string rpl_user = mysqlshdk::gr::get_recovery_user(instance);
    Cluster_impl *cluster(get_cluster());

    // Remove the replication user used by the removed instance on all
    // cluster members through the primary (using replication).
    // NOTE: Make sure to remove the user if it was an user created by
    // the Shell, i.e. with the format: mysql_innodb_cluster_r[0-9]{10}
    if (!rpl_user.empty() &&
        shcore::str_beginswith(rpl_user, "mysql_innodb_cluster_r")) {
      log_debug("Removing replication user '%s'", rpl_user.c_str());
      try {
        mysqlshdk::mysql::drop_all_accounts_for_user(
            cluster->get_group_session(), rpl_user);
      } catch (const std::exception &drop_accounts_error) {
        auto console = mysqlsh::current_console();
        console->print_warning("Failed to remove replication user '" +
                               rpl_user + "': " + drop_accounts_error.what());
      }
    } else {
      auto console = mysqlsh::current_console();
      console->print_warning(
          "Unable to determine replication user used for recovery. Skipping "
          "removal of it.");
    }
  }
}

void ReplicaSet::dissolve(const shcore::Dictionary_t &options) {
  mysqlshdk::utils::nullable<bool> force;
  bool interactive;

  Cluster_impl *cluster(get_cluster());

  interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (options) {
    Unpack_options(options)
        .optional("force", &force)
        .optional("interactive", &interactive)
        .end();
  }

  // Dissolve the ReplicaSet
  try {
    // Create the Dissolve command and execute it.
    Dissolve op_dissolve(interactive, force, cluster);
    // Always execute finish when leaving "try catch".
    auto finally =
        shcore::on_leave_scope([&op_dissolve]() { op_dissolve.finish(); });
    // Prepare the dissolve command execution (validations).
    op_dissolve.prepare();
    // Execute dissolve operations.
    op_dissolve.execute();
  } catch (...) {
    throw;
  }
}

namespace {
void unpack_auto_instances_list(
    mysqlsh::Unpack_options *opts_unpack, const std::string &option_name,
    bool *out_auto,
    std::vector<mysqlshdk::db::Connection_options> *instances_list) {
  // Extract value for addInstances, it can be a string "auto" or a list.
  shcore::Array_t instances_array;
  try {
    // Try to extract the "auto" string.
    mysqlshdk::utils::nullable<std::string> auto_option_str;
    opts_unpack->optional(option_name.c_str(), &auto_option_str);

    // Validate if "auto" was specified (case insensitive).
    if (!auto_option_str.is_null() &&
        shcore::str_casecmp(*auto_option_str, "auto") == 0) {
      *out_auto = true;
    } else if (!auto_option_str.is_null()) {
      throw shcore::Exception::argument_error(
          "Option '" + option_name +
          "' only accepts 'auto' as a valid string "
          "value, otherwise a list of instances is expected.");
    }
  } catch (const shcore::Exception &err) {
    // Try to extract a list of instances (will fail with a TypeError when
    // trying to read it as a string previously).
    if (std::string(err.type()).compare("TypeError") == 0) {
      opts_unpack->optional(option_name.c_str(), &instances_array);
    } else {
      throw;
    }
  }

  if (instances_array) {
    if (instances_array.get()->empty()) {
      throw shcore::Exception::argument_error("The list for '" + option_name +
                                              "' option cannot be empty.");
    }

    // Process values from addInstances list (must be valid connection data).
    for (const shcore::Value &value : *instances_array.get()) {
      shcore::Argument_list args;
      args.push_back(shcore::Value(value));

      try {
        mysqlshdk::db::Connection_options cnx_opt =
            mysqlsh::get_connection_options(args,
                                            mysqlsh::PasswordFormat::NONE);

        if (cnx_opt.get_host().empty()) {
          throw shcore::Exception::argument_error("host cannot be empty.");
        }

        if (!cnx_opt.has_port()) {
          throw shcore::Exception::argument_error("port is missing.");
        }

        instances_list->push_back(cnx_opt);
      } catch (const std::exception &err) {
        std::string error(err.what());
        throw shcore::Exception::argument_error(
            "Invalid value '" + value.descr() + "' for '" + option_name +
            "' option: " + error);
      }
    }
  }
}
}  // namespace

void ReplicaSet::rescan(const shcore::Dictionary_t &options) {
  bool interactive, auto_add_instance = false, auto_remove_instance = false;
  mysqlshdk::utils::nullable<bool> update_topology_mode;
  std::vector<mysqlshdk::db::Connection_options> add_instances_list,
      remove_instances_list;

  interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (options) {
    shcore::Array_t add_instances_array, remove_instances_array;

    auto opts_unpack = Unpack_options(options);
    opts_unpack.optional("updateTopologyMode", &update_topology_mode)
        .optional("interactive", &interactive);

    // Extract value for addInstances, it can be a string "auto" or a list.
    unpack_auto_instances_list(&opts_unpack, "addInstances", &auto_add_instance,
                               &add_instances_list);

    // Extract value for removeInstances, it can be a string "auto" or a list.
    unpack_auto_instances_list(&opts_unpack, "removeInstances",
                               &auto_remove_instance, &remove_instances_list);

    opts_unpack.end();
  }

  // Rescan replicaset.
  {
    // Create the rescan command and execute it.
    Rescan op_rescan(interactive, update_topology_mode, auto_add_instance,
                     auto_remove_instance, add_instances_list,
                     remove_instances_list, this);

    // Always execute finish when leaving "try catch".
    auto finally =
        shcore::on_leave_scope([&op_rescan]() { op_rescan.finish(); });

    // Prepare the rescan command execution (validations).
    op_rescan.prepare();

    // Execute rescan operation.
    op_rescan.execute();
  }
}

mysqlshdk::db::Connection_options ReplicaSet::pick_seed_instance() const {
  Cluster_impl *cluster(get_cluster());

  bool single_primary;
  std::string primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      cluster->get_group_session(), &single_primary);
  if (single_primary) {
    if (!primary_uuid.empty()) {
      mysqlshdk::utils::nullable<mysqlshdk::innodbcluster::Instance_info> info(
          _metadata_storage->get_new_metadata()->get_instance_info_by_uuid(
              primary_uuid));
      if (info) {
        mysqlshdk::db::Connection_options coptions(info->classic_endpoint);
        mysqlshdk::db::Connection_options group_session_target(
            cluster->get_group_session()->get_connection_options());

        coptions.set_login_options_from(group_session_target);
        coptions.set_ssl_connection_options_from(
            group_session_target.get_ssl_options());

        return coptions;
      }
    }
    throw shcore::Exception::runtime_error(
        "Unable to determine a suitable peer instance to join the group");
  } else {
    // instance we're connected to should be OK if we're multi-master
    return cluster->get_group_session()->get_connection_options();
  }
}

shcore::Value ReplicaSet::check_instance_state(
    const Connection_options &instance_def) {
  // Create the ReplicaSet Check_instance_state object and execute it.
  Check_instance_state op_check_instance_state(*this, instance_def);

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_check_instance_state]() { op_check_instance_state.finish(); });

  // Prepare the Replicaset_check_instance_state command execution
  // (validations).
  op_check_instance_state.prepare();

  // Execute Replicaset_check_instance_state operations.
  return op_check_instance_state.execute();
}

void ReplicaSet::add_instance_metadata(
    const mysqlshdk::db::Connection_options &instance_definition,
    const std::string &label) const {
  log_debug("Adding instance to metadata");

  MetadataStorage::Transaction tx(_metadata_storage);

  // Check if the instance was already added
  std::string instance_address = instance_definition.as_uri(only_transport());

  log_debug("Connecting to '%s' to query for metadata information...",
            instance_address.c_str());
  // Get the required data from the joining instance to store in the metadata:
  // - server UUID, reported_host, etc
  std::shared_ptr<mysqlshdk::db::ISession> classic;
  try {
    classic = establish_mysql_session(instance_definition,
                                      current_shell_options()->get().wizards);
  } catch (const shcore::Exception &e) {
    std::stringstream ss;
    ss << "Error opening session to '" << instance_address << "': " << e.what();
    log_warning("%s", ss.str().c_str());

    // Check if we're adopting a GR cluster, if so, it could happen that
    // we can't connect to it because root@localhost exists but root@hostname
    // doesn't (GR keeps the hostname in the members table)
    if (e.is_mysql() && e.code() == 1045) {  // access denied
      std::stringstream se;
      se << "Access denied connecting to new instance " << instance_address
         << ".\n"
         << "Please ensure all instances in the same group/replicaset have"
            " the same password for account '"
            ""
         << instance_definition.get_user()
         << "' and that it is accessible from the host mysqlsh is running "
            "from.";
      throw shcore::Exception::runtime_error(se.str());
    }
    throw shcore::Exception::runtime_error(ss.str());
  }

  mysqlshdk::mysql::Instance target_instance(classic);

  Instance_definition instance(query_instance_info(target_instance));

  if (!label.empty()) instance.label = label;

  // Add the host to the metadata.
  uint32_t host_id = _metadata_storage->insert_host(
      target_instance.get_canonical_hostname(), "", "");

  instance.host_id = host_id;
  instance.replicaset_id = get_id();

  // Add the instance to the metadata.
  _metadata_storage->insert_instance(instance);

  tx.commit();
}

void ReplicaSet::remove_instance_metadata(
    const mysqlshdk::db::Connection_options &instance_def) {
  log_debug("Removing instance from metadata");

  std::string port = std::to_string(instance_def.get_port());

  std::string host = instance_def.get_host();

  // Check if the instance was already added
  std::string instance_address = host + ":" + port;

  _metadata_storage->remove_instance(instance_address);
}

std::vector<std::string> ReplicaSet::get_online_instances() const {
  std::vector<std::string> online_instances_array;

  auto online_instances =
      _metadata_storage->get_replicaset_online_instances(_id);

  for (auto &instance : online_instances) {
    // TODO(miguel): Review if end point is the right thing
    std::string instance_host = instance.endpoint;
    online_instances_array.push_back(instance_host);
  }

  return online_instances_array;
}

shcore::Value ReplicaSet::force_quorum_using_partition_of(
    const shcore::Argument_list &args) {
  shcore::Value ret_val;
  uint64_t rset_id = get_id();
  std::shared_ptr<mysqlshdk::db::ISession> session;

  auto instance_def =
      mysqlsh::get_connection_options(args, PasswordFormat::STRING);

  validate_connection_options(instance_def);

  if (!instance_def.has_port())
    instance_def.set_port(mysqlshdk::db::k_default_mysql_port);

  instance_def.set_default_connection_data();

  std::string instance_address = instance_def.as_uri(only_transport());

  // TODO(miguel): test if there's already quorum and add a 'force' option to be
  // used if so

  // TODO(miguel): test if the instance if part of the current cluster, for the
  // scenario of restoring a cluster quorum from another

  mysqlshdk::mysql::Instance target_instance;
  // Before rejoining an instance we must verify if the instance's
  // 'group_replication_group_name' matches the one registered in the
  // Metadata (BUG #26159339)
  {
    try {
      log_info("Opening a new session to the partition instance %s",
               instance_address.c_str());
      session = establish_mysql_session(instance_def,
                                        current_shell_options()->get().wizards);
      instance_def = session->get_connection_options();
    } catch (const std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_address.c_str(), e.what());
      throw;
    }

    target_instance = mysqlshdk::mysql::Instance(session);

    // Get instance address in metadata.
    std::string md_address = target_instance.get_canonical_address();

    // Check if the instance belongs to the ReplicaSet on the Metadata
    if (!_metadata_storage->is_instance_on_replicaset(rset_id, md_address)) {
      std::string message = "The instance '" + instance_address + "'";
      message.append(" does not belong to the ReplicaSet: '" + get_name() +
                     "'.");
      throw shcore::Exception::runtime_error(message);
    }

    if (!validate_replicaset_group_name(session, get_group_name())) {
      std::string nice_error =
          "The instance '" + instance_address +
          "' "
          "cannot be used to restore the cluster as it "
          "may belong to a different ReplicaSet as the one registered "
          "in the Metadata since the value of "
          "'group_replication_group_name' does not match the one "
          "registered in the ReplicaSet's Metadata: possible split-brain "
          "scenario.";

      session->close();

      throw shcore::Exception::runtime_error(nice_error);
    }
  }

  // Get the instance state
  Cluster_check_info state;

  auto instance_type = get_gr_instance_type(session);

  if (instance_type != GRInstanceType::Standalone &&
      instance_type != GRInstanceType::StandaloneWithMetadata) {
    state = get_replication_group_state(target_instance, instance_type);

    if (state.source_state != ManagedInstance::OnlineRW &&
        state.source_state != ManagedInstance::OnlineRO) {
      std::string message = "The instance '" + instance_address + "'";
      message.append(" cannot be used to restore the cluster as it is on a ");
      message.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      message.append(" state, and should be ONLINE");

      session->close();

      throw shcore::Exception::runtime_error(message);
    }
  } else {
    std::string message = "The instance '" + instance_address + "'";
    message.append(
        " cannot be used to restore the cluster as it is not an active member "
        "of replication group.");

    session->close();

    throw shcore::Exception::runtime_error(message);
  }

  // Check if there is quorum to issue an error.
  if (mysqlshdk::gr::has_quorum(target_instance, nullptr, nullptr)) {
    mysqlsh::current_console()->print_error(
        "Cannot perform operation on an healthy cluster because it can only "
        "be used to restore a cluster from quorum loss.");

    target_instance.close_session();

    throw shcore::Exception::runtime_error(
        "The cluster has quorum according to instance '" + instance_address +
        "'");
  }

  // Get the online instances of the ReplicaSet to user as group_peers
  auto online_instances =
      _metadata_storage->get_replicaset_online_instances(rset_id);

  if (online_instances.empty()) {
    session->close();
    throw shcore::Exception::logic_error(
        "No online instances are visible from the given one.");
  }

  std::string group_peers;

  for (auto &instance : online_instances) {
    std::string instance_host = instance.endpoint;
    auto target_coptions = shcore::get_connection_options(instance_host, false);
    // We assume the login credentials are the same on all instances
    target_coptions.set_login_options_from(instance_def);

    std::shared_ptr<mysqlshdk::db::ISession> instance_session;
    try {
      log_info(
          "Opening a new session to a group_peer instance to obtain the XCOM "
          "address %s",
          instance_host.c_str());
      instance_session = establish_mysql_session(
          target_coptions, current_shell_options()->get().wizards);
    } catch (const std::exception &e) {
      log_error("Could not open connection to %s: %s", instance_address.c_str(),
                e.what());
      session->close();
      throw;
    }

    // Get @@group_replication_local_address
    std::string group_peer_instance_xcom_address =
        mysqlshdk::mysql::Instance(instance_session)
            .get_sysvar_string("group_replication_local_address")
            .get_safe();

    group_peers.append(group_peer_instance_xcom_address);
    group_peers.append(",");

    instance_session->close();
  }

  // Force the reconfiguration of the GR group
  {
    // Remove the trailing comma of group_peers
    if (group_peers.back() == ',') group_peers.pop_back();

    log_info("Setting group_replication_force_members at instance %s",
             instance_address.c_str());

    // Setting the group_replication_force_members will force a new group
    // membership, triggering the necessary actions from GR upon being set to
    // force the quorum. Therefore, the variable can be cleared immediately
    // after it is set.
    target_instance.set_sysvar("group_replication_force_members", group_peers);

    // Clear group_replication_force_members at the end to allow GR to be
    // restarted later on the instance (without error).
    target_instance.set_sysvar("group_replication_force_members",
                               std::string());

    session->close();
  }

  return ret_val;
}

void ReplicaSet::switch_to_single_primary_mode(
    const Connection_options &instance_def) {
  // Switch to single-primary mode

  // Create the Switch_to_single_primary_mode object and execute it.
  Switch_to_single_primary_mode op_switch_to_single_primary_mode(instance_def,
                                                                 this);

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope([&op_switch_to_single_primary_mode]() {
    op_switch_to_single_primary_mode.finish();
  });

  // Prepare the Switch_to_single_primary_mode command execution (validations).
  op_switch_to_single_primary_mode.prepare();

  // Execute Switch_to_single_primary_mode operation.
  op_switch_to_single_primary_mode.execute();
}

void ReplicaSet::switch_to_multi_primary_mode(void) {
  // Switch to multi-primary mode

  // Create the Switch_to_multi_primary_mode object and execute it.
  Switch_to_multi_primary_mode op_switch_to_multi_primary_mode(this);

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope([&op_switch_to_multi_primary_mode]() {
    op_switch_to_multi_primary_mode.finish();
  });

  // Prepare the Switch_to_multi_primary_mode command execution (validations).
  op_switch_to_multi_primary_mode.prepare();

  // Execute Switch_to_multi_primary_mode operation.
  op_switch_to_multi_primary_mode.execute();
}

void ReplicaSet::set_primary_instance(const Connection_options &instance_def) {
  // Set primary instance

  // Create the Set_primary_instance object and execute it.
  Set_primary_instance op_set_primary_instance(instance_def, this);

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_set_primary_instance]() { op_set_primary_instance.finish(); });

  // Prepare the Set_primary_instance command execution (validations).
  op_set_primary_instance.prepare();

  // Execute Set_primary_instance operation.
  op_set_primary_instance.execute();
}

void ReplicaSet::remove_instances(
    const std::vector<std::string> &remove_instances) {
  if (!remove_instances.empty()) {
    for (auto instance : remove_instances) {
      // NOTE: Verification if the instance is on the metadata was already
      // performed by the caller Dba::reboot_cluster_from_complete_outage().
      shcore::Value::Map_type_ref options(new shcore::Value::Map_type);

      auto connection_options = shcore::get_connection_options(instance, false);
      remove_instance_metadata(connection_options);
    }
  }
}

void ReplicaSet::rejoin_instances(
    const std::vector<std::string> &rejoin_instances,
    const shcore::Value::Map_type_ref &options) {
  auto instance_session(_metadata_storage->get_session());
  auto instance_data = instance_session->get_connection_options();

  if (!rejoin_instances.empty()) {
    // Get the user and password from the options
    // or from the instance session
    if (options) {
      // Check if the password is specified on the options and if not prompt it
      mysqlsh::set_user_from_map(&instance_data, options);
      mysqlsh::set_password_from_map(&instance_data, options);
    }

    for (auto instance : rejoin_instances) {
      // NOTE: Verification if the instance is on the metadata was already
      // performed by the caller Dba::reboot_cluster_from_complete_outage().
      auto connection_options = shcore::get_connection_options(instance, false);

      connection_options.set_user(instance_data.get_user());
      connection_options.set_password(instance_data.get_password());

      // If rejoinInstance fails we don't want to stop the execution of the
      // function, but to log the error.
      try {
        std::string msg = "Rejoining the instance '" + instance +
                          "' to the "
                          "cluster's default replicaset.";
        log_warning("%s", msg.c_str());
        rejoin_instance(&connection_options, shcore::Value::Map_type_ref());
      } catch (const shcore::Exception &e) {
        log_error("Failed to rejoin instance: %s", e.what());
      }
    }
  }
}

/**
 * Check the instance server UUID of the specified intance.
 *
 * The server UUID must be unique for all instances in a cluster. This function
 * checks if the server_uuid of the target instance is unique among all active
 * members of the cluster.
 *
 * @param instance_session Session to the target instance to check its server
 *                         UUID.
 */
void ReplicaSet::validate_server_uuid(
    const std::shared_ptr<mysqlshdk::db::ISession> &instance_session) const {
  // Get the server_uuid of the target instance.
  auto instance = mysqlshdk::mysql::Instance(instance_session);
  std::string server_uuid = *instance.get_sysvar_string(
      "server_uuid", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Get connection option for the metadata.
  Cluster_impl *cluster(get_cluster());

  std::shared_ptr<mysqlshdk::db::ISession> cluster_session =
      cluster->get_group_session();
  Connection_options cluster_cnx_opt =
      cluster_session->get_connection_options();

  // Get list of instances in the metadata
  std::vector<Instance_definition> metadata_instances =
      _metadata_storage->get_replicaset_active_instances(_id);

  // Get and compare the server UUID of all instances with the one of
  // the target instance.
  for (Instance_definition &instance_def : metadata_instances) {
    if (server_uuid == instance_def.uuid) {
      // Raise an error if the server uuid is the same of a cluster member.
      throw shcore::Exception::runtime_error(
          "Cannot add an instance with the same server UUID (" + server_uuid +
          ") of an active member of the cluster '" + instance_def.endpoint +
          "'. Please change the server UUID of the instance to add, all "
          "members must have a unique server UUID.");
    }
  }
}

std::vector<Instance_definition> ReplicaSet::get_instances_from_metadata()
    const {
  return _metadata_storage->get_replicaset_instances(get_id());
}

std::vector<ReplicaSet::Instance_info> ReplicaSet::get_instances() const {
  return _metadata_storage->get_new_metadata()->get_replicaset_instances(
      get_id());
}

std::unique_ptr<mysqlshdk::config::Config> ReplicaSet::create_config_object(
    std::vector<std::string> ignored_instances, bool skip_invalid_state) const {
  auto cfg = shcore::make_unique<mysqlshdk::config::Config>();

  auto console = mysqlsh::current_console();

  // Get all cluster instances, including state information to update
  // auto-increment values.
  std::vector<Instance_definition> instance_defs =
      _metadata_storage->get_replicaset_instances(get_id(), true);

  for (const auto &instance_def : instance_defs) {
    // If instance is on the list of instances to be ignored, skip it.
    if (std::find(ignored_instances.begin(), ignored_instances.end(),
                  instance_def.endpoint) != ignored_instances.end())
      continue;

    // Use the GR state hold by instance_def.state (but convert it to a proper
    // mysqlshdk::gr::Member_state to be handled properly).
    mysqlshdk::gr::Member_state state =
        mysqlshdk::gr::to_member_state(instance_def.state);

    if (state == mysqlshdk::gr::Member_state::ONLINE ||
        state == mysqlshdk::gr::Member_state::RECOVERING) {
      // Set login credentials to connect to instance.
      // NOTE: It is assumed that the same login credentials can be used to
      //       connect to all cluster instances.
      Connection_options instance_cnx_opts =
          shcore::get_connection_options(instance_def.endpoint, false);
      instance_cnx_opts.set_login_options_from(
          get_cluster()->get_group_session()->get_connection_options());

      // Try to connect to instance.
      log_debug("Connecting to instance '%s'", instance_def.endpoint.c_str());
      std::shared_ptr<mysqlshdk::db::ISession> session;
      try {
        session = mysqlshdk::db::mysql::Session::create();
        session->connect(instance_cnx_opts);
        log_debug("Successfully connected to instance");
      } catch (const std::exception &err) {
        log_debug("Failed to connect to instance: %s", err.what());
        console->print_error(
            "Unable to connect to instance '" + instance_def.endpoint +
            "'. Please, verify connection credentials and make sure the "
            "instance is available.");

        throw shcore::Exception::runtime_error(err.what());
      }

      auto instance = mysqlshdk::mysql::Instance(session);

      // Determine if SET PERSIST is supported.
      mysqlshdk::utils::nullable<bool> support_set_persist =
          instance.is_set_persist_supported();
      mysqlshdk::mysql::Var_qualifier set_type =
          mysqlshdk::mysql::Var_qualifier::GLOBAL;
      if (!support_set_persist.is_null() && *support_set_persist) {
        set_type = mysqlshdk::mysql::Var_qualifier::PERSIST;
      }

      // Add configuration handler for server.
      cfg->add_handler(
          instance_def.endpoint,
          std::unique_ptr<mysqlshdk::config::IConfig_handler>(
              shcore::make_unique<mysqlshdk::config::Config_server_handler>(
                  shcore::make_unique<mysqlshdk::mysql::Instance>(instance),
                  set_type)));

      // Print a warning if SET PERSIST is not supported, for users to execute
      // dba.configureLocalInstance().
      if (support_set_persist.is_null()) {
        console->print_warning(
            "Instance '" + instance_def.endpoint +
            "' cannot persist configuration since MySQL version " +
            instance.get_version().get_base() +
            " does not support the SET PERSIST command "
            "(MySQL version >= 8.0.11 required). Please use the <Dba>." +
            "<<<configureLocalInstance>>>() command locally to persist the "
            "changes.");
      } else if (!*support_set_persist) {
        console->print_warning(
            "Instance '" + instance_def.endpoint +
            "' will not load the persisted cluster configuration upon reboot "
            "since 'persisted-globals-load' is set to 'OFF'. Please use the "
            "<Dba>.<<<configureLocalInstance>>>"
            "() command locally to persist the changes or set "
            "'persisted-globals-load' to 'ON' on the configuration file.");
      }
    } else {
      // Ignore instance with an invalid state (i.e., do not issue an erro), if
      // skip_invalid_state is true.
      if (skip_invalid_state) continue;

      // Issue an error if the instance is not active.
      console->print_error("The settings cannot be updated for instance '" +
                           instance_def.endpoint + "' because it is on a '" +
                           mysqlshdk::gr::to_string(state) + "' state.");

      throw shcore::Exception::runtime_error(
          "The instance '" + instance_def.endpoint + "' is '" +
          mysqlshdk::gr::to_string(state) + "'");
    }
  }

  return cfg;
}

/** Iterates through all the cluster members in a given state calling the given
 * function on each of then.
 * @param states Vector of strings with the states of members on which the
 * functor will be called.
 * @param cnx_opt Connection options to be used to connect to the cluster
 * members
 * @param ignore_instances_vector Vector with addresses of instances to be
 * ignored even if their state is specified in the states vector.
 * @param functor Function that is called on each member of the cluster whose
 * state is specified in the states vector.
 */

void ReplicaSet::execute_in_members(
    const std::vector<std::string> &states,
    const mysqlshdk::db::Connection_options &cnx_opt,
    const std::vector<std::string> &ignore_instances_vector,
    std::function<bool(std::shared_ptr<mysqlshdk::db::ISession> session)>
        functor,
    bool ignore_network_conn_errors) const {
  const int kNetworkConnRefused = 2003;
  std::shared_ptr<mysqlshdk::db::ISession> instance_session;
  // Note (nelson): should we handle the super_read_only behavior here or should
  // it be the responsibility of the functor?
  auto instance_definitions =
      _metadata_storage->get_replicaset_instances(_id, false, states);

  for (auto &instance_def : instance_definitions) {
    std::string instance_address = instance_def.endpoint;
    // if instance is on the list of instances to be ignored, skip it
    if (std::find(ignore_instances_vector.begin(),
                  ignore_instances_vector.end(),
                  instance_address) != ignore_instances_vector.end())
      continue;
    auto target_coptions =
        shcore::get_connection_options(instance_address, false);

    target_coptions.set_login_options_from(cnx_opt);
    try {
      log_debug(
          "Opening a new session to instance '%s' while iterating "
          "cluster members",
          instance_address.c_str());
      instance_session = establish_mysql_session(
          target_coptions, current_shell_options()->get().wizards);
    } catch (const mysqlshdk::db::Error &e) {
      if (ignore_network_conn_errors && e.code() == kNetworkConnRefused) {
        log_error("Could not open connection to '%s': %s, but ignoring it.",
                  instance_address.c_str(), e.what());
        continue;
      } else {
        log_error("Could not open connection to '%s': %s",
                  instance_address.c_str(), e.what());
        throw;
      }
    } catch (const std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_address.c_str(), e.what());
      throw;
    }
    bool continue_loop = functor(instance_session);
    instance_session->close();
    if (!continue_loop) {
      log_debug("Cluster iteration stopped because functor returned false.");
      break;
    }
  }
}

void ReplicaSet::set_group_name(const std::string &group_name) {
  _group_name = group_name;
  _metadata_storage->set_replicaset_group_name(get_id(), group_name);
}

void ReplicaSet::validate_server_id(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  // Get the server_id of the target instance.
  mysqlshdk::utils::nullable<int64_t> server_id =
      target_instance.get_sysvar_int("server_id");

  // Check if there is a member with the same server_id and throw an exception
  // in that case.
  // NOTE: attempt to check all members (except itself) independently of their
  //       state, but skip it if the connection fails.
  execute_in_members(
      {}, target_instance.get_connection_options(),
      {target_instance.get_canonical_address()},
      [&server_id](const std::shared_ptr<mysqlshdk::db::ISession> &session) {
        mysqlshdk::mysql::Instance instance(session);

        mysqlshdk::utils::nullable<int64_t> id =
            instance.get_sysvar_int("server_id");

        if (!server_id.is_null() && !id.is_null() && *server_id == *id) {
          throw std::runtime_error{"The server_id '" + std::to_string(*id) +
                                   "' is already used by instance '" +
                                   instance.descr() + "'."};
        }
        return true;
      });
}

}  // namespace dba
}  // namespace mysqlsh
