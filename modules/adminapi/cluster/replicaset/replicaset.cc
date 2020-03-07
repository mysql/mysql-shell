/*
 * Copyright (c) 2016, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/cluster/replicaset/replicaset.h"

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
#include "modules/adminapi/cluster/replicaset/add_instance.h"
#include "modules/adminapi/cluster/replicaset/check_instance_state.h"
#include "modules/adminapi/cluster/replicaset/remove_instance.h"
#include "modules/adminapi/cluster/replicaset/rescan.h"
#include "modules/adminapi/cluster/replicaset/set_instance_option.h"
#include "modules/adminapi/cluster/replicaset/set_primary_instance.h"
#include "modules/adminapi/cluster/replicaset/switch_to_multi_primary_mode.h"
#include "modules/adminapi/cluster/replicaset/switch_to_single_primary_mode.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_shell.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
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
char const *GRReplicaSet::kTopologySinglePrimary = "pm";
char const *GRReplicaSet::kTopologyMultiPrimary = "mm";

const char *GRReplicaSet::kWarningDeprecateSslMode =
    "Option 'memberSslMode' is deprecated for this operation and it will be "
    "removed in a future release. This option is not needed because the SSL "
    "mode is automatically obtained from the cluster. Please do not use it "
    "here.";

GRReplicaSet::GRReplicaSet(const std::string &name,
                           const std::string &topology_type)
    : _name(name), _topology_type(topology_type) {
  assert(topology_type == kTopologyMultiPrimary ||
         topology_type == kTopologySinglePrimary);
}

GRReplicaSet::~GRReplicaSet() {}

void GRReplicaSet::sanity_check() const { verify_topology_type_change(); }

/*
 * Verify if the topology type changed and issue an error if needed.
 */
void GRReplicaSet::verify_topology_type_change() const {
  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  Cluster_impl *cluster(get_cluster());

  std::string gr_primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      *cluster->get_target_server(), nullptr);

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

void GRReplicaSet::validate_rejoin_gtid_consistency(
    const mysqlshdk::mysql::IInstance &target_instance) {
  auto console = mysqlsh::current_console();
  std::string errant_gtid_set;

  // Get the gtid state in regards to the cluster_session
  mysqlshdk::mysql::Replica_gtid_state state =
      mysqlshdk::mysql::check_replica_gtid_state(
          *get_cluster()->get_target_server(), target_instance, nullptr,
          &errant_gtid_set);

  if (state == mysqlshdk::mysql::Replica_gtid_state::NEW) {
    console->print_info();
    console->print_error(
        "The target instance '" + target_instance.descr() +
        "' has an empty GTID set so it cannot be safely rejoined to the "
        "cluster. Please remove it and add it back to the cluster.");
    console->print_info();

    throw shcore::Exception::runtime_error("The instance '" +
                                           target_instance.descr() +
                                           "' has an empty GTID set.");
  } else if (state == mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE) {
    console->print_info();
    console->print_error("A GTID set check of the MySQL instance at '" +
                         target_instance.descr() +
                         "' determined that it is missing transactions that "
                         "were purged from all cluster members.");
    console->print_info();
    throw shcore::Exception::runtime_error(
        "The instance '" + target_instance.descr() +
        "' is missing transactions that "
        "were purged from all cluster members.");
  } else if (state == mysqlshdk::mysql::Replica_gtid_state::DIVERGED) {
    console->print_info();
    console->print_error("A GTID set check of the MySQL instance at '" +
                         target_instance.descr() +
                         "' determined that it contains transactions that "
                         "do not originate from the cluster, which must be "
                         "discarded before it can join the cluster.");

    console->print_info();
    console->print_info(target_instance.descr() +
                        " has the following errant GTIDs that do not exist "
                        "in the cluster:\n" +
                        errant_gtid_set);
    console->print_info();

    console->print_info(
        "Having extra GTID events is not expected, and it is "
        "recommended to investigate this further and ensure that the data "
        "can be removed prior to rejoining the instance to the cluster.");

    if (target_instance.get_version() >=
        mysqlshdk::mysql::k_mysql_clone_plugin_initial_version) {
      console->print_info();
      console->print_info(
          "Discarding these extra GTID events can either be done manually "
          "or by completely overwriting the state of " +
          target_instance.descr() +
          " with a physical snapshot from an existing cluster member. To "
          "achieve this remove the instance from the cluster and add it "
          "back using <Cluster>.<<<addInstance>>>() and setting the "
          "'recoveryMethod' option to 'clone'.");
    }

    console->print_info();

    throw shcore::Exception::runtime_error(
        "The instance '" + target_instance.descr() +
        "' contains errant transactions that did not originate from the "
        "cluster.");
  }
}

void GRReplicaSet::set_instance_option(const Connection_options &instance_def,
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
    op_set_instance_option = std::make_unique<Set_instance_option>(
        *this, instance_def, option, value_str);
  } else if (value.type == shcore::Integer || value.type == shcore::UInteger) {
    int64_t value_int = value.as_int();
    op_set_instance_option = std::make_unique<Set_instance_option>(
        *this, instance_def, option, value_int);
  } else {
    throw shcore::Exception::argument_error(
        "Argument #3 is expected to be a string or an integer.");
  }

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_set_instance_option]() { op_set_instance_option->finish(); });

  // Prepare the Replicaset_set_instance_option command execution (validations).
  op_set_instance_option->prepare();

  // Execute Replicaset_set_instance_option operations.
  op_set_instance_option->execute();
}

void GRReplicaSet::adopt_from_gr() {
  shcore::Value ret_val;
  auto console = mysqlsh::current_console();

  auto newly_discovered_instances_list(get_newly_discovered_instances(
      *get_cluster()->get_target_server(),
      get_cluster()->get_metadata_storage(), get_cluster()->get_id()));

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
    auto md_instance = get_cluster()->get_metadata_storage()->get_md_server();

    auto session_data = md_instance->get_connection_options();

    newly_discovered_instance.set_login_options_from(session_data);

    add_instance_metadata(newly_discovered_instance);
  }
}

void GRReplicaSet::add_instance(
    const mysqlshdk::db::Connection_options &connection_options,
    const shcore::Dictionary_t &options) {
  Group_replication_options gr_options(Group_replication_options::JOIN);
  Clone_options clone_options(Clone_options::JOIN_CLUSTER);
  mysqlshdk::utils::nullable<std::string> label;
  int wait_recovery = isatty(STDOUT_FILENO) ? 3 : 2;
  bool interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (options) {
    // Retrieves optional options if exists
    Unpack_options(options)
        .unpack(&gr_options)
        .unpack(&clone_options)
        .optional("interactive", &interactive)
        .optional("label", &label)
        .optional("waitRecovery", &wait_recovery)
        .end();
  }

  // Validate waitRecovery option UInteger [0, 3]
  if (wait_recovery > 3) {
    throw shcore::Exception::argument_error(
        "Invalid value '" + std::to_string(wait_recovery) +
        "' for option 'waitRecovery'. It must be an integer in the range [0, "
        "3].");
  }

  // Validate the label value.
  if (!label.is_null()) {
    mysqlsh::dba::validate_label(*label);

    if (!get_cluster()->get_metadata_storage()->is_instance_label_unique(
            get_cluster()->get_id(), *label))
      throw shcore::Exception::argument_error(
          "An instance with label '" + *label +
          "' is already part of this InnoDB cluster");
  }

  // Create the add_instance command to be executed.
  Add_instance op_add_instance(connection_options, *this, gr_options,
                               clone_options, label, interactive,
                               wait_recovery);

  // Add the Instance to the GRReplicaSet
  try {
    // Always execute finish when leaving "try catch".
    auto finally = shcore::on_leave_scope(
        [&op_add_instance]() { op_add_instance.finish(); });

    // Prepare the add_instance command execution (validations).
    op_add_instance.prepare();

    // Execute add_instance operations.
    op_add_instance.execute();
  } catch (...) {
    op_add_instance.rollback();
    throw;
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

void GRReplicaSet::query_group_wide_option_values(
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
  execute_in_members({mysqlshdk::gr::Member_state::ONLINE,
                      mysqlshdk::gr::Member_state::RECOVERING},
                     target_instance->get_connection_options(), {},
                     [&gr_consistency, &gr_member_expel_timeout](
                         const std::shared_ptr<Instance> &instance) {
                       {
                         mysqlshdk::utils::nullable<std::string> value;
                         value = instance->get_sysvar_string(
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
                         value = instance->get_sysvar_int(
                             "group_replication_member_expel_timeout",
                             mysqlshdk::mysql::Var_qualifier::GLOBAL);

                         if (value.is_null()) {
                           gr_member_expel_timeout.found_not_supported = true;
                         } else if (*value != 0) {
                           gr_member_expel_timeout.found_non_default = true;
                           gr_member_expel_timeout.non_default_value = *value;
                         }
                       }
                       // if we have found both a instance that doesnt have
                       // support for the option and an instance that doesn't
                       // have the default value, then we don't need to look at
                       // any other instance on the cluster.
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
 * An optional instance_session parameter can be provided that will be used to
 * get its current GR group seeds value and add the local address from the
 * active members (avoiding duplicates) to that initial value, allowing to
 * preserve the GR group seeds of the specified instance. If no
 * instance_session is given (nullptr) then the returned groups seeds value
 * will only be based on the currently active members, disregarding any current
 * GR group seed value on any instance (allowing to reset the group seed only
 * based on the currently active members).
 *
 * @param target_instance Session to the target instance to get the group
 *                         seeds for.
 * @return a string with a comma separated list of all the GR local address
 *         values of the currently active (ONLINE or RECOVERING) instances in
 *         the replicaset.
 */
std::string GRReplicaSet::get_cluster_group_seeds(
    const std::shared_ptr<Instance> &target_instance) const {
  // Get connection option for the metadata.
  Cluster_impl *cluster(get_cluster());

  std::shared_ptr<Instance> cluster_instance = cluster->get_target_server();
  Connection_options cluster_cnx_opt =
      cluster_instance->get_connection_options();

  // Get list of active instances (ONLINE or RECOVERING)
  std::vector<Instance_metadata> active_instances = get_active_instances();

  std::vector<std::string> gr_group_seeds_list;
  // If the target instance is provided, use its current GR group seed variable
  // value as starting point to append new (missing) values to it.
  if (target_instance) {
    // Get the instance GR group seeds and save it to the GR group seeds list.
    std::string gr_group_seeds = *target_instance->get_sysvar_string(
        "group_replication_group_seeds",
        mysqlshdk::mysql::Var_qualifier::GLOBAL);
    if (!gr_group_seeds.empty()) {
      gr_group_seeds_list = shcore::split_string(gr_group_seeds, ",");
    }
  }

  // Get the update GR group seed from local address of all active instances.
  for (Instance_metadata &instance_def : active_instances) {
    std::string instance_address = instance_def.endpoint;
    Connection_options target_coptions =
        shcore::get_connection_options(instance_address, false);
    // It is assumed that the same user and password is used by all members.
    target_coptions.set_login_options_from(cluster_cnx_opt);

    // Connect to the instance.
    std::shared_ptr<Instance> instance;
    try {
      log_debug(
          "Connecting to instance '%s' to get its value for the "
          "group_replication_local_address variable.",
          instance_address.c_str());
      instance = Instance::connect(target_coptions,
                                   current_shell_options()->get().wizards);
    } catch (const shcore::Error &e) {
      if (mysqlshdk::db::is_mysql_client_error(e.code())) {
        log_info(
            "Could not connect to instance '%s', its local address will not "
            "be used for the group seeds: %s",
            instance_address.c_str(), e.format().c_str());
        break;
      } else {
        throw shcore::Exception::runtime_error("While connecting to " +
                                               target_coptions.uri_endpoint() +
                                               ": " + e.what());
      }
    }

    // Get the instance GR local address and add it to the GR group seeds list.
    std::string local_address =
        *instance->get_sysvar_string("group_replication_local_address",
                                     mysqlshdk::mysql::Var_qualifier::GLOBAL);
    if (std::find(gr_group_seeds_list.begin(), gr_group_seeds_list.end(),
                  local_address) == gr_group_seeds_list.end()) {
      // Only add the local address if not already in the group seed list,
      // avoiding duplicates.
      gr_group_seeds_list.push_back(local_address);
    }
  }

  return shcore::str_join(gr_group_seeds_list, ",");
}

void GRReplicaSet::rejoin_instance(
    const Connection_options &instance_def_,
    const shcore::Value::Map_type_ref &rejoin_options) {
  auto instance_def = instance_def_;
  Cluster_impl *cluster(get_cluster());

  Group_replication_options gr_options(Group_replication_options::REJOIN);
  // SSL Mode AUTO by default
  gr_options.ssl_mode = mysqlsh::dba::kMemberSSLModeAuto;
  std::string user, password;
  shcore::Value::Array_type_ref errors;
  std::shared_ptr<Instance> instance;
  std::shared_ptr<Instance> seed_instance;

  auto console = mysqlsh::current_console();

  // Retrieves the options
  if (rejoin_options) {
    Unpack_options(rejoin_options).unpack(&gr_options).end();

    if (rejoin_options->has_key("memberSslMode")) {
      console->print_warning(kWarningDeprecateSslMode);
      console->println();
    }
  }

  if (!instance_def.has_port() && !instance_def.has_socket())
    instance_def.set_port(mysqlshdk::db::k_default_mysql_port);

  instance_def.set_default_connection_data();

  // Get the target instance.
  try {
    log_info("Opening a new session to the rejoining instance %s",
             instance_def.uri_endpoint().c_str());
    instance =
        Instance::connect(instance_def, current_shell_options()->get().wizards);
  } catch (const std::exception &e) {
    std::string err_msg = "Could not open connection to '" +
                          instance_def.uri_endpoint() + "': " + e.what();
    throw shcore::Exception::runtime_error(err_msg);
  }

  // Before rejoining an instance we must verify if the instance's
  // 'group_replication_group_name' matches the one registered in the
  // Metadata (BUG #26159339)
  //
  // Before rejoining an instance we must also verify if the group has quorum
  // and if the gr plugin is active otherwise we may end up hanging the system

  // Validate 'group_replication_group_name'
  {
    // Perform quick config checks
    ensure_gr_instance_configuration_valid(instance.get(), false);

    // Check if the instance is part of the Metadata
    Instance_metadata instance_md;
    try {
      instance_md = get_cluster()->get_metadata_storage()->get_instance_by_uuid(
          instance->get_uuid());
    } catch (const shcore::Error &e) {
      if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) throw;
    }

    if (instance_md.cluster_id != get_cluster()->get_id()) {
      std::string message = "The instance '" + instance_def.uri_endpoint() +
                            "' " + "does not belong to the cluster: '" +
                            get_cluster()->get_name() + "'.";

      throw shcore::Exception::runtime_error(message);
    }

    gr_options.check_option_values(instance->get_version());

    std::string nice_error =
        "The instance '" + instance_def.uri_endpoint() +
        "' may belong to a different cluster as the one registered "
        "in the Metadata since the value of "
        "'group_replication_group_name' does not match the one "
        "registered in the cluster's Metadata: possible split-brain "
        "scenario. Please remove the instance from the cluster.";

    try {
      if (!validate_replicaset_group_name(*instance,
                                          cluster->get_group_name())) {
        throw shcore::Exception::runtime_error(nice_error);
      }
    } catch (const shcore::Error &e) {
      // If the GR plugin is not installed, we can get this error.
      // In that case, we install the GR plugin and retry.
      if (e.code() == ER_UNKNOWN_SYSTEM_VARIABLE) {
        log_info("%s: installing GR plugin (%s)",
                 instance_def.uri_endpoint().c_str(), e.format().c_str());
        mysqlshdk::gr::install_group_replication_plugin(*instance, nullptr);

        if (!validate_replicaset_group_name(*instance,
                                            cluster->get_group_name())) {
          throw shcore::Exception::runtime_error(nice_error);
        }
      } else {
        throw;
      }
    }
  }

  // Check GTID consistency to determine if the instance can safely rejoin the
  // cluster BUG#29953812: ADD_INSTANCE() PICKY ABOUT GTID_EXECUTED,
  // REJOIN_INSTANCE() NOT: DATA NOT COPIED
  validate_rejoin_gtid_consistency(*instance);

  // In order to be able to rejoin the instance to the cluster we need the seed
  // instance.

  // Get the seed (peer) instance.
  {
    mysqlshdk::db::Connection_options seed_cnx_opt(pick_seed_instance());

    // To be able to establish a session to the seed instance we need a username
    // and password. Taking into consideration the assumption that all instances
    // of the cluster use the same credentials we can obtain the ones of the
    // current target group session

    seed_cnx_opt.set_login_options_from(
        cluster->get_target_server()->get_connection_options());

    // Establish a session to the seed instance
    try {
      log_info("Opening a new session to seed instance: %s",
               seed_cnx_opt.uri_endpoint().c_str());
      seed_instance = Instance::connect(seed_cnx_opt,
                                        current_shell_options()->get().wizards);
    } catch (const std::exception &e) {
      throw shcore::Exception::runtime_error("Could not open a connection to " +
                                             seed_cnx_opt.uri_endpoint() +
                                             ": " + e.what() + ".");
    }
  }

  // Verify if the instance is running asynchronous (master-slave) replication
  {
    log_debug(
        "Checking if instance '%s' is running asynchronous (master-slave) "
        "replication.",
        instance->descr().c_str());

    if (mysqlshdk::mysql::is_async_replication_running(*instance)) {
      console->print_error(
          "Cannot rejoin instance '" + instance->descr() +
          "' to the cluster because it has asynchronous (master-slave) "
          "replication configured and running. Please stop the slave threads "
          "by executing the query: 'STOP SLAVE;'");

      throw shcore::Exception::runtime_error(
          "The instance '" + instance->descr() +
          "' is running asynchronous (master-slave) replication.");
    }
  }

  // Verify if the group_replication plugin is active on the seed instance
  {
    log_info(
        "Verifying if the group_replication plugin is active on the seed "
        "instance %s",
        seed_instance->descr().c_str());

    auto plugin_status = seed_instance->get_plugin_status("group_replication");

    if (plugin_status.get_safe() != "ACTIVE") {
      throw shcore::Exception::runtime_error(
          "Cannot rejoin instance. The seed instance doesn't have "
          "group-replication active.");
    }
  }

  {
    // Check if instance was doing auto-rejoin and let the user know that the
    // rejoin operation will override the auto-rejoin
    bool is_running_rejoin =
        mysqlshdk::gr::is_running_gr_auto_rejoin(*instance);
    if (is_running_rejoin) {
      console->print_info(
          "The instance '" + instance->get_connection_options().uri_endpoint() +
          "' is running auto-rejoin process, however the rejoinInstance has "
          "precedence and will override that process.");
      console->println();
    }
  }

  // Verify if the instance being added is MISSING, otherwise throw an error
  // Bug#26870329
  {
    // get server_uuid from the instance that we're trying to rejoin
    if (!validate_instance_rejoinable(*instance,
                                      get_cluster()->get_metadata_storage(),
                                      get_cluster()->get_id())) {
      // instance not missing, so throw an error
      auto member_state =
          mysqlshdk::gr::to_string(mysqlshdk::gr::get_member_state(*instance));
      std::string nice_error_msg = "Cannot rejoin instance '" +
                                   instance->descr() + "' to the cluster '" +
                                   get_cluster()->get_name() +
                                   "' since it is an active (" + member_state +
                                   ") member of the cluster.";

      throw shcore::Exception::runtime_error(nice_error_msg);
    }
  }

  // Get the up-to-date GR group seeds values (with the GR local address from
  // all currently active instances).
  gr_options.group_seeds = get_cluster_group_seeds(instance);

  // join Instance to cluster
  {
    std::string replication_user, replication_user_pwd;

    std::string new_ssl_mode;
    // Resolve the SSL Mode to use to configure the instance.
    new_ssl_mode = resolve_instance_ssl_mode(*instance, *seed_instance,
                                             *gr_options.ssl_mode);
    if (gr_options.ssl_mode.is_null() || new_ssl_mode != *gr_options.ssl_mode) {
      gr_options.ssl_mode = new_ssl_mode;
      log_warning("SSL mode used to configure the instance: '%s'",
                  gr_options.ssl_mode->c_str());
    }

    // Get SSL values to connect to peer instance
    auto seed_instance_def = seed_instance->get_connection_options();

    // Stop group-replication
    log_info("Stopping group-replication at instance %s",
             instance->get_connection_options().uri_endpoint().c_str());
    instance->execute("STOP GROUP_REPLICATION");

    // Handling of GR protocol version
    try {
      // Get the current protocol version in use in the group
      mysqlshdk::utils::Version gr_protocol_version =
          mysqlshdk::gr::get_group_protocol_version(*seed_instance);

      // If the target instance being rejoined does not support the GR
      // protocol version in use on the group (because it is an older
      // version), the rejoinInstance command must set the GR protocol of the
      // cluster to the version of the target instance.

      if (mysqlshdk::gr::is_protocol_downgrade_required(gr_protocol_version,
                                                        *instance)) {
        mysqlshdk::gr::set_group_protocol_version(*seed_instance,
                                                  instance->get_version());
      }
    } catch (const shcore::Error &error) {
      // The UDF may fail with MySQL Error 1123 if any of the members is
      // RECOVERING In such scenario, we must abort the upgrade protocol
      // version process and warn the user
      if (error.code() == ER_CANT_INITIALIZE_UDF) {
        console->print_note(
            "Unable to determine the Group Replication protocol version, "
            "while verifying if a protocol upgrade would be possible: " +
            std::string(error.what()));
      } else {
        throw;
      }
    }

    // BUG#29265869: reboot cluster overrides some GR settings.
    // Read actual GR configurations to preserve them when rejoining the
    // instance.
    gr_options.read_option_values(*instance);

    // Resolve GR local address.
    // NOTE: Must be done only after getting the report_host used by GR and for
    //       the metadata;
    int port = instance->get_canonical_port();
    std::string hostname = instance->get_canonical_hostname();
    gr_options.local_address = mysqlsh::dba::resolve_gr_local_address(
        gr_options.local_address, hostname, port);

    // Set a Config object for the target instance (required to configure GR).
    std::unique_ptr<mysqlshdk::config::Config> cfg = create_server_config(
        instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);

    // (Re-)join the instance to the replicaset (setting up GR properly).
    // NOTE: the join_replicaset() function expects the number of members in
    //       the cluster excluding the joining node, thus replicaset_count must
    //       exclude the rejoining node (cluster size - 1) since it already
    //       belongs to the metadata (BUG#30174191).
    mysqlshdk::utils::nullable<uint64_t> replicaset_count =
        get_cluster()->get_metadata_storage()->get_cluster_size(
            get_cluster()->get_id()) -
        1;
    mysqlsh::dba::join_replicaset(*instance, *seed_instance, replication_user,
                                  replication_user_pwd, gr_options,
                                  replicaset_count, cfg.get());
    log_info("The instance '%s' was successfully rejoined on the cluster.",
             seed_instance->descr().c_str());
  }
}

void GRReplicaSet::remove_instance(const Connection_options &instance_def,
                                   const shcore::Dictionary_t &options) {
  mysqlshdk::utils::nullable<bool> force;
  bool interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (options) {
    Unpack_options(options)
        .optional("force", &force)
        .optional("interactive", &interactive)
        .end();
  }

  // Remove the Instance from the GRReplicaSet
  try {
    // Create the remove_instance command and execute it.
    Remove_instance op_remove_instance(instance_def, interactive, force, *this);
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
}

void GRReplicaSet::update_group_members_for_removed_member(
    const std::string &local_gr_address,
    const mysqlsh::dba::Instance & /* instance */) {
  // Get the ReplicaSet Config Object
  auto cfg = create_config_object({}, true);

  // Iterate through all ONLINE and RECOVERING cluster members and update
  // their group_replication_group_seeds value by removing the
  // gr_local_address of the instance that was removed
  log_debug("Updating group_replication_group_seeds of cluster members");
  mysqlshdk::gr::update_group_seeds(
      cfg.get(), local_gr_address, mysqlshdk::gr::Gr_seeds_change_type::REMOVE);
  cfg->apply();

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
    //   - Replicaset that had more 7 or more members before the
    //   Remove_instance
    //     operation
    //
    // NOTE: in the other scenarios, the Add_instance operation is in charge
    // of updating auto-increment accordingly

    mysqlshdk::gr::Topology_mode topology_mode =
        get_cluster()->get_metadata_storage()->get_cluster_topology_mode(
            get_cluster()->get_id());

    // Get the current number of members of the Replicaset
    uint64_t replicaset_count =
        get_cluster()->get_metadata_storage()->get_cluster_size(
            get_cluster()->get_id());

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

void GRReplicaSet::dissolve(const shcore::Dictionary_t &options) {
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

  // Dissolve the GRReplicaSet
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
      try {
        auto cnx_opt = mysqlsh::get_connection_options(value);

        if (cnx_opt.get_host().empty()) {
          throw shcore::Exception::argument_error("host cannot be empty.");
        }

        if (!cnx_opt.has_port()) {
          throw shcore::Exception::argument_error("port is missing.");
        }

        instances_list->emplace_back(std::move(cnx_opt));
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

void GRReplicaSet::rescan(const shcore::Dictionary_t &options) {
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

mysqlshdk::db::Connection_options GRReplicaSet::pick_seed_instance() const {
  Cluster_impl *cluster(get_cluster());

  bool single_primary;
  std::string primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      *cluster->get_target_server(), &single_primary);
  if (single_primary) {
    if (!primary_uuid.empty()) {
      Instance_metadata info =
          get_cluster()->get_metadata_storage()->get_instance_by_uuid(
              primary_uuid);

      mysqlshdk::db::Connection_options coptions(info.endpoint);
      mysqlshdk::db::Connection_options group_session_target(
          cluster->get_target_server()->get_connection_options());

      coptions.set_login_options_from(group_session_target);
      coptions.set_ssl_connection_options_from(
          group_session_target.get_ssl_options());

      return coptions;
    }
    throw shcore::Exception::runtime_error(
        "Unable to determine a suitable peer instance to join the group");
  } else {
    // instance we're connected to should be OK if we're multi-master
    return cluster->get_target_server()->get_connection_options();
  }
}

shcore::Value GRReplicaSet::check_instance_state(
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

void GRReplicaSet::add_instance_metadata(
    const mysqlshdk::db::Connection_options &instance_definition,
    const std::string &label) const {
  log_debug("Adding instance to metadata");

  // Check if the instance was already added
  std::string instance_address = instance_definition.as_uri(only_transport());

  log_debug("Connecting to '%s' to query for metadata information...",
            instance_address.c_str());
  // Get the required data from the joining instance to store in the metadata:
  // - server UUID, reported_host, etc
  std::shared_ptr<Instance> classic;
  try {
    classic = Instance::connect(instance_definition,
                                current_shell_options()->get().wizards);
  } catch (const shcore::Error &e) {
    std::stringstream ss;
    ss << "Error opening session to '" << instance_address << "': " << e.what();
    log_warning("%s", ss.str().c_str());

    // TODO(alfredo) - this should be validated before adding the instance
    // Check if we're adopting a GR cluster, if so, it could happen that
    // we can't connect to it because root@localhost exists but root@hostname
    // doesn't (GR keeps the hostname in the members table)
    if (e.code() == ER_ACCESS_DENIED_ERROR) {
      std::stringstream se;
      se << "Access denied connecting to new instance " << instance_address
         << ".\n"
         << "Please ensure all instances in the same group/cluster have"
            " the same password for account '"
            ""
         << instance_definition.get_user()
         << "' and that it is accessible from the host mysqlsh is running "
            "from.";
      throw shcore::Exception::runtime_error(se.str());
    }
    throw shcore::Exception::runtime_error(ss.str());
  }

  add_instance_metadata(*classic, label);
}

void GRReplicaSet::add_instance_metadata(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &label) const {
  Instance_metadata instance_md(query_instance_info(instance));

  if (!label.empty()) instance_md.label = label;

  instance_md.cluster_id = get_cluster()->get_id();

  // Add the instance to the metadata.
  get_cluster()->get_metadata_storage()->insert_instance(instance_md);
}

void GRReplicaSet::remove_instance_metadata(
    const mysqlshdk::db::Connection_options &instance_def) {
  log_debug("Removing instance from metadata");

  std::string port = std::to_string(instance_def.get_port());

  std::string host = instance_def.get_host();

  // Check if the instance was already added
  std::string instance_address = host + ":" + port;

  get_cluster()->get_metadata_storage()->remove_instance(instance_address);
}

std::vector<Instance_metadata> GRReplicaSet::get_active_instances() const {
  return get_instances({mysqlshdk::gr::Member_state::ONLINE,
                        mysqlshdk::gr::Member_state::RECOVERING});
}

std::vector<Instance_metadata> GRReplicaSet::get_instances(
    const std::vector<mysqlshdk::gr::Member_state> &states) const {
  std::vector<Instance_metadata> all_instances =
      m_cluster->get_metadata_storage()->get_all_instances(m_cluster->get_id());

  if (states.empty()) {
    return all_instances;
  } else {
    std::vector<Instance_metadata> res;
    std::vector<mysqlshdk::gr::Member> members(
        mysqlshdk::gr::get_members(*m_cluster->get_target_server()));

    for (const auto &i : all_instances) {
      auto m = std::find_if(members.begin(), members.end(),
                            [&i](const mysqlshdk::gr::Member &member) {
                              return member.uuid == i.uuid;
                            });
      if (m != members.end() &&
          std::find(states.begin(), states.end(), m->state) != states.end()) {
        res.push_back(i);
      }
    }
    return res;
  }
}

std::shared_ptr<mysqlsh::dba::Instance> GRReplicaSet::get_online_instance(
    const std::string &exclude_uuid) const {
  std::vector<Instance_metadata> instances(get_active_instances());

  // Get the cluster connection credentials to use to connect to instances.
  mysqlshdk::db::Connection_options cluster_cnx_opts =
      m_cluster->get_target_server()->get_connection_options();

  for (const auto &instance : instances) {
    // Skip instance with the provided UUID exception (if specified).
    if (exclude_uuid.empty() || instance.uuid != exclude_uuid) {
      try {
        // Use the cluster connection credentials.
        mysqlshdk::db::Connection_options coptions(instance.endpoint);
        coptions.set_login_options_from(cluster_cnx_opts);

        log_info("Opening session to the member of InnoDB cluster at %s...",
                 coptions.as_uri().c_str());

        // Return the first valid (reachable) instance.
        return Instance::connect(coptions);

      } catch (const std::exception &e) {
        log_debug(
            "Unable to establish a session to the cluster member '%s': %s",
            instance.endpoint.c_str(), e.what());
      }
    }
  }

  // No reachable online instance was found.
  return nullptr;
}

void GRReplicaSet::force_quorum_using_partition_of(
    const Connection_options &instance_def_) {
  auto instance_def = instance_def_;
  std::shared_ptr<Instance> target_instance;

  validate_connection_options(instance_def);

  if (!instance_def.has_port() && !instance_def.has_socket())
    instance_def.set_port(mysqlshdk::db::k_default_mysql_port);

  instance_def.set_default_connection_data();

  std::string instance_address = instance_def.as_uri(only_transport());

  // TODO(miguel): test if there's already quorum and add a 'force' option to be
  // used if so

  // TODO(miguel): test if the instance if part of the current cluster, for the
  // scenario of restoring a cluster quorum from another

  try {
    log_info("Opening a new session to the partition instance %s",
             instance_address.c_str());
    target_instance =
        Instance::connect(instance_def, current_shell_options()->get().wizards);
  } catch (const std::exception &e) {
    log_error("Could not open connection to '%s': %s", instance_address.c_str(),
              e.what());
    throw;
  }

  // Before rejoining an instance we must verify if the instance's
  // 'group_replication_group_name' matches the one registered in the
  // Metadata (BUG #26159339)
  {
    // Get instance address in metadata.
    std::string md_address = target_instance->get_canonical_address();

    // Check if the instance belongs to the ReplicaSet on the Metadata
    if (!get_cluster()->contains_instance_with_address(md_address)) {
      std::string message = "The instance '" + instance_address + "'";
      message.append(" does not belong to the cluster: '" +
                     get_cluster()->get_name() + "'.");
      throw shcore::Exception::runtime_error(message);
    }

    if (!validate_replicaset_group_name(*target_instance,
                                        get_cluster()->get_group_name())) {
      std::string nice_error =
          "The instance '" + instance_address +
          "' "
          "cannot be used to restore the cluster as it "
          "may belong to a different cluster as the one registered "
          "in the Metadata since the value of "
          "'group_replication_group_name' does not match the one "
          "registered in the cluster's Metadata: possible split-brain "
          "scenario.";

      throw shcore::Exception::runtime_error(nice_error);
    }
  }

  // Get the instance state
  Cluster_check_info state;

  auto instance_type = get_gr_instance_type(*target_instance);

  if (instance_type != GRInstanceType::Standalone &&
      instance_type != GRInstanceType::StandaloneWithMetadata &&
      instance_type != GRInstanceType::StandaloneInMetadata) {
    state = get_replication_group_state(*target_instance, instance_type);

    if (state.source_state != ManagedInstance::OnlineRW &&
        state.source_state != ManagedInstance::OnlineRO) {
      std::string message = "The instance '" + instance_address + "'";
      message.append(" cannot be used to restore the cluster as it is on a ");
      message.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      message.append(" state, and should be ONLINE");

      throw shcore::Exception::runtime_error(message);
    }
  } else {
    std::string message = "The instance '" + instance_address + "'";
    message.append(
        " cannot be used to restore the cluster as it is not an active member "
        "of replication group.");

    throw shcore::Exception::runtime_error(message);
  }

  // Check if there is quorum to issue an error.
  if (mysqlshdk::gr::has_quorum(*target_instance, nullptr, nullptr)) {
    mysqlsh::current_console()->print_error(
        "Cannot perform operation on an healthy cluster because it can only "
        "be used to restore a cluster from quorum loss.");

    throw shcore::Exception::runtime_error(
        "The cluster has quorum according to instance '" + instance_address +
        "'");
  }

  // Get the all instances from MD and members visible by the target instance.
  std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>>
      instances_info = get_instances_with_state();

  std::string group_peers;
  int online_count = 0;

  for (auto &instance : instances_info) {
    std::string instance_host = instance.first.endpoint;
    auto target_coptions = shcore::get_connection_options(instance_host, false);
    // We assume the login credentials are the same on all instances
    target_coptions.set_login_options_from(
        target_instance->get_connection_options());

    std::shared_ptr<Instance> instance_session;
    try {
      log_info(
          "Opening a new session to a group_peer instance to obtain the XCOM "
          "address %s",
          instance_host.c_str());
      instance_session = Instance::connect(
          target_coptions, current_shell_options()->get().wizards);

      if (instance.second.state == mysqlshdk::gr::Member_state::ONLINE ||
          instance.second.state == mysqlshdk::gr::Member_state::RECOVERING) {
        // Add GCS address of active instance to the force quorum membership.
        std::string group_peer_instance_xcom_address =
            instance_session
                ->get_sysvar_string("group_replication_local_address")
                .get_safe();

        group_peers.append(group_peer_instance_xcom_address);
        group_peers.append(",");

        online_count++;
      } else {
        // Stop GR on not active instances.
        mysqlshdk::gr::stop_group_replication(*instance_session);
      }
    } catch (const std::exception &e) {
      log_error("Could not open connection to %s: %s", instance_address.c_str(),
                e.what());

      // Only throw errors if the instance is active, otherwise ignore it.
      if (instance.second.state == mysqlshdk::gr::Member_state::ONLINE ||
          instance.second.state == mysqlshdk::gr::Member_state::RECOVERING) {
        throw;
      }
    }
  }

  if (online_count == 0) {
    throw shcore::Exception::logic_error(
        "No online instances are visible from the given one.");
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
    target_instance->set_sysvar("group_replication_force_members", group_peers);

    // Clear group_replication_force_members at the end to allow GR to be
    // restarted later on the instance (without error).
    target_instance->set_sysvar("group_replication_force_members",
                                std::string());
  }
}

void GRReplicaSet::switch_to_single_primary_mode(
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

void GRReplicaSet::switch_to_multi_primary_mode(void) {
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

void GRReplicaSet::set_primary_instance(
    const Connection_options &instance_def) {
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

void GRReplicaSet::remove_instances(
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

void GRReplicaSet::rejoin_instances(
    const std::vector<std::string> &rejoin_instances,
    const shcore::Value::Map_type_ref &options) {
  auto instance_data = m_cluster->get_target_server()->get_connection_options();

  if (!rejoin_instances.empty()) {
    // Get the user and password from the options
    // or from the instance session
    if (options) {
      // Check if the password is specified on the options and if not prompt it
      mysqlsh::set_user_from_map(&instance_data, options);
      mysqlsh::set_password_from_map(&instance_data, options);
    }

    for (const auto &instance : rejoin_instances) {
      // NOTE: Verification if the instance is on the metadata was already
      // performed by the caller Dba::reboot_cluster_from_complete_outage().
      auto connection_options = shcore::get_connection_options(instance, false);

      connection_options.set_login_options_from(instance_data);

      // If rejoinInstance fails we don't want to stop the execution of the
      // function, but to log the error.
      auto console = mysqlsh::current_console();
      try {
        console->print_info("Rejoining '" + connection_options.uri_endpoint() +
                            "' to the cluster.");
        rejoin_instance(connection_options, {});
      } catch (const shcore::Error &e) {
        console->print_warning(connection_options.uri_endpoint() + ": " +
                               e.format());
        // TODO(miguel) Once WL#13535 is implemented and rejoin supports clone,
        // simplify the following note by telling the user to use
        // rejoinInstance. E.g: “%s’ could not be automatically rejoined. Please
        // use cluster.rejoinInstance() to manually re-add it.”
        console->print_note(shcore::str_format(
            "Unable to rejoin instance '%s' to the cluster but the "
            "dba.<<<rebootClusterFromCompleteOutage>>>() operation will "
            "continue.",
            connection_options.uri_endpoint().c_str()));
        console->print_info();
        log_error("Failed to rejoin instance: %s", e.what());
      }
    }
  }
}

/**
 * Check the instance server UUID of the specified instance.
 *
 * The server UUID must be unique for all instances in a cluster. This function
 * checks if the server_uuid of the target instance is unique among all
 * members of the cluster.
 *
 * @param instance_session Session to the target instance to check its server
 *                         UUID.
 */
void GRReplicaSet::validate_server_uuid(
    const mysqlshdk::mysql::IInstance &instance) const {
  // Get the server_uuid of the target instance.
  std::string server_uuid = *instance.get_sysvar_string(
      "server_uuid", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Get connection option for the metadata.
  Cluster_impl *cluster(get_cluster());

  std::shared_ptr<Instance> cluster_instance = cluster->get_target_server();
  Connection_options cluster_cnx_opt =
      cluster_instance->get_connection_options();

  // Get list of instances in the metadata
  std::vector<Instance_metadata> metadata_instances = get_instances();

  // Get and compare the server UUID of all instances with the one of
  // the target instance.
  for (Instance_metadata &instance_def : metadata_instances) {
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

std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>>
GRReplicaSet::get_instances_with_state() const {
  std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>> ret;

  std::vector<mysqlshdk::gr::Member> members(
      mysqlshdk::gr::get_members(*m_cluster->get_target_server()));

  std::vector<Instance_metadata> md(get_instances());

  for (const auto &i : md) {
    auto m = std::find_if(members.begin(), members.end(),
                          [&i](const mysqlshdk::gr::Member &member) {
                            return member.uuid == i.uuid;
                          });
    if (m != members.end()) {
      ret.push_back({i, *m});
    } else {
      mysqlshdk::gr::Member mm;
      mm.uuid = i.uuid;
      mm.state = mysqlshdk::gr::Member_state::MISSING;
      ret.push_back({i, mm});
    }
  }

  return ret;
}

std::unique_ptr<mysqlshdk::config::Config> GRReplicaSet::create_config_object(
    std::vector<std::string> ignored_instances, bool skip_invalid_state,
    bool persist_only) const {
  auto cfg = std::make_unique<mysqlshdk::config::Config>();

  auto console = mysqlsh::current_console();

  // Get all cluster instances, including state information to update
  // auto-increment values.
  std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>>
      instance_defs = get_instances_with_state();

  for (const auto &instance_def : instance_defs) {
    // If instance is on the list of instances to be ignored, skip it.
    if (std::find(ignored_instances.begin(), ignored_instances.end(),
                  instance_def.first.endpoint) != ignored_instances.end())
      continue;

    // Use the GR state hold by instance_def.state (but convert it to a proper
    // mysqlshdk::gr::Member_state to be handled properly).
    mysqlshdk::gr::Member_state state = instance_def.second.state;

    if (state == mysqlshdk::gr::Member_state::ONLINE ||
        state == mysqlshdk::gr::Member_state::RECOVERING) {
      // Set login credentials to connect to instance.
      // NOTE: It is assumed that the same login credentials can be used to
      //       connect to all cluster instances.
      Connection_options instance_cnx_opts =
          shcore::get_connection_options(instance_def.first.endpoint, false);
      instance_cnx_opts.set_login_options_from(
          get_cluster()->get_target_server()->get_connection_options());

      // Try to connect to instance.
      log_debug("Connecting to instance '%s'",
                instance_def.first.endpoint.c_str());
      std::shared_ptr<mysqlsh::dba::Instance> instance;
      try {
        instance = Instance::connect(instance_cnx_opts);
        log_debug("Successfully connected to instance");
      } catch (const std::exception &err) {
        log_debug("Failed to connect to instance: %s", err.what());
        console->print_error(
            "Unable to connect to instance '" + instance_def.first.endpoint +
            "'. Please verify connection credentials and make sure the "
            "instance is available.");

        throw shcore::Exception::runtime_error(err.what());
      }

      // Determine if SET PERSIST is supported.
      mysqlshdk::utils::nullable<bool> support_set_persist =
          instance->is_set_persist_supported();
      mysqlshdk::mysql::Var_qualifier set_type =
          mysqlshdk::mysql::Var_qualifier::GLOBAL;
      bool skip = false;
      if (!support_set_persist.is_null() && *support_set_persist) {
        if (persist_only)
          set_type = mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY;
        else
          set_type = mysqlshdk::mysql::Var_qualifier::PERSIST;
      } else {
        // If we want persist_only but it's not supported, we skip it since it
        // can't help us
        if (persist_only) skip = true;
      }

      // Add configuration handler for server.
      if (!skip)
        cfg->add_handler(
            instance_def.first.endpoint,
            std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                std::make_unique<mysqlshdk::config::Config_server_handler>(
                    instance, set_type)));

      // Print a warning if SET PERSIST is not supported, for users to execute
      // dba.configureLocalInstance().
      if (support_set_persist.is_null()) {
        console->print_warning(
            "Instance '" + instance_def.first.endpoint +
            "' cannot persist configuration since MySQL version " +
            instance->get_version().get_base() +
            " does not support the SET PERSIST command "
            "(MySQL version >= 8.0.11 required). Please use the dba." +
            "<<<configureLocalInstance>>>() command locally to persist the "
            "changes.");
      } else if (!*support_set_persist) {
        console->print_warning(
            "Instance '" + instance_def.first.endpoint +
            "' will not load the persisted cluster configuration upon reboot "
            "since 'persisted-globals-load' is set to 'OFF'. Please use the "
            "dba.<<<configureLocalInstance>>>"
            "() command locally to persist the changes or set "
            "'persisted-globals-load' to 'ON' on the configuration file.");
      }
    } else {
      // Ignore instance with an invalid state (i.e., do not issue an erro), if
      // skip_invalid_state is true.
      if (skip_invalid_state) continue;

      // Issue an error if the instance is not active.
      console->print_error("The settings cannot be updated for instance '" +
                           instance_def.first.endpoint +
                           "' because it is on a '" +
                           mysqlshdk::gr::to_string(state) + "' state.");

      throw shcore::Exception::runtime_error(
          "The instance '" + instance_def.first.endpoint + "' is '" +
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
void GRReplicaSet::execute_in_members(
    const std::vector<mysqlshdk::gr::Member_state> &states,
    const mysqlshdk::db::Connection_options &cnx_opt,
    const std::vector<std::string> &ignore_instances_vector,
    const std::function<bool(const std::shared_ptr<Instance> &instance)>
        &functor,
    bool ignore_network_conn_errors) const {
  std::shared_ptr<Instance> instance_session;
  // Note (nelson): should we handle the super_read_only behavior here or should
  // it be the responsibility of the functor?
  auto instance_definitions = get_instances_with_state();

  for (auto &instance_def : instance_definitions) {
    std::string instance_address = instance_def.first.endpoint;
    // if instance is on the list of instances to be ignored, skip it
    if (std::find(ignore_instances_vector.begin(),
                  ignore_instances_vector.end(),
                  instance_address) != ignore_instances_vector.end())
      continue;
    // if state list is given but it doesn't match, skip too
    if (!states.empty() && std::find(states.begin(), states.end(),
                                     instance_def.second.state) == states.end())
      continue;

    auto target_coptions =
        shcore::get_connection_options(instance_address, false);

    target_coptions.set_login_options_from(cnx_opt);
    try {
      log_debug(
          "Opening a new session to instance '%s' while iterating "
          "cluster members",
          instance_address.c_str());
      instance_session = Instance::connect(
          target_coptions, current_shell_options()->get().wizards);
    } catch (const shcore::Error &e) {
      if (ignore_network_conn_errors && e.code() == CR_CONN_HOST_ERROR) {
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
    if (!continue_loop) {
      log_debug("Cluster iteration stopped because functor returned false.");
      break;
    }
  }
}

void GRReplicaSet::validate_server_id(
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
      [&server_id](const std::shared_ptr<Instance> &instance) {
        mysqlshdk::utils::nullable<int64_t> id =
            instance->get_sysvar_int("server_id");

        if (!server_id.is_null() && !id.is_null() && *server_id == *id) {
          throw std::runtime_error{"The server_id '" + std::to_string(*id) +
                                   "' is already used by instance '" +
                                   instance->descr() + "'."};
        }
        return true;
      });
}

}  // namespace dba
}  // namespace mysqlsh
