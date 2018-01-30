/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <vector>
#include "interactive_dba_cluster.h"
#include "interactive_global_dba.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"
#include "modules/adminapi/mod_dba_common.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/version.h"
#include "mysql/instance.h"
#include "db/mysql/session.h"
#include "modules/mod_utils.h"

using namespace std::placeholders;
using namespace shcore;
using mysqlshdk::db::uri::formats::only_transport;
using mysqlshdk::db::uri::formats::user_transport;
void Interactive_dba_cluster::init() {
  add_method("addInstance",
             std::bind(&Interactive_dba_cluster::add_instance, this, _1),
             "data");
  add_method("rejoinInstance",
             std::bind(&Interactive_dba_cluster::rejoin_instance, this, _1),
             "data");
  add_method("removeInstance",
             std::bind(&Interactive_dba_cluster::remove_instance, this, _1),
             "data");
  add_varargs_method("dissolve", std::bind(&Interactive_dba_cluster::dissolve,
                     this, _1));
  add_varargs_method("checkInstanceState",
                     std::bind(&Interactive_dba_cluster::check_instance_state,
                     this, _1));
  add_varargs_method("rescan", std::bind(&Interactive_dba_cluster::rescan,
                     this, _1));
  add_varargs_method(
      "forceQuorumUsingPartitionOf",
      std::bind(&Interactive_dba_cluster::force_quorum_using_partition_of,
      this, _1));
}

mysqlsh::dba::ReplicationGroupState
    Interactive_dba_cluster::check_preconditions(
        const std::string& function_name) const {
  ScopedStyle ss(_target.get(), naming_style);
  auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
  return cluster->check_preconditions(function_name);
}

void Interactive_dba_cluster::assert_valid(
      const std::string& function_name) const {
  ScopedStyle ss(_target.get(), naming_style);
  auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
  cluster->assert_valid(function_name);
}

shcore::Value Interactive_dba_cluster::add_seed_instance(
      const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string function;
  std::shared_ptr<mysqlsh::dba::ReplicaSet> object;

  auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster> (_target);

  if (cluster) {
    // Throw an error if the cluster has already been dissolved
    assert_valid("addInstance");
    object = cluster->get_default_replicaset();
  }
  if (object) {
    if (prompt("The default ReplicaSet is already initialized. \
                Do you want to add a new instance?")  == Prompt_answer::YES)
      function = "addInstance";
  } else {
    function = "addSeedInstance";
  }

  if (!function.empty()) {
    auto instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    mysqlsh::resolve_connection_credentials(&instance_def, _delegate);

    auto instance_map = mysqlsh::get_connection_map(instance_def);

    shcore::Argument_list new_args;
    new_args.push_back(shcore::Value(instance_map));

    if (args.size() == 2)
      new_args.push_back(args[1]);

    ret_val = call_target(function, new_args);
  }

  return ret_val;
}

shcore::Value Interactive_dba_cluster::add_instance(
    const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string function;

  // Throw an error if the cluster has already been dissolved
  assert_valid("addInstance");

  args.ensure_count(1, 2, get_function_name("addInstance").c_str());

  check_preconditions("addInstance");

  shcore::Value::Map_type_ref options;
  shcore::Value::Map_type_ref instance_map;
  mysqlshdk::db::Connection_options instance_def;

  try {
    std::shared_ptr<mysqlsh::dba::ReplicaSet> object;
    auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
    if (cluster)
      object = cluster->get_default_replicaset();

    if (!object) {
      if (prompt("The default ReplicaSet is not initialized. Do you want to \
                  initialize it adding a seed instance?") == Prompt_answer::YES)
          function = "addSeedInstance";
    } else {
      function = "addInstance";
    }

    if (!function.empty()) {
      std::string message =
          "A new instance will be added to the InnoDB cluster. "
          "Depending on the amount of\n"
          "data on the cluster this might take from a few seconds to "
          "several hours.\n\n";

      print(message);

      instance_def = mysqlsh::get_connection_options(
          args, mysqlsh::PasswordFormat::OPTIONS);

      if (args.size() == 2) {
        options = args.map_at(1);
        shcore::Argument_map options_map(*options);
        options_map.ensure_keys({},
            mysqlsh::dba::ReplicaSet::_add_instance_opts,
            "instance definition");

        // Validate SSL options for the cluster instance
        mysqlsh::dba::validate_ssl_instance_options(options);

        // Validate ip whitelist option
        mysqlsh::dba::validate_ip_whitelist_option(options);
      }

      mysqlsh::resolve_connection_credentials(&instance_def, _delegate);

      instance_map = mysqlsh::get_connection_map(instance_def);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_map));

  if (options)
    new_args.push_back(args[1]);

  println("Adding instance to the cluster ...");
  println();
  ret_val = call_target(function, new_args);

  println("The instance '" +
    instance_def.as_uri(user_transport()) + ""
    "' was successfully added to the cluster.");
  println();

  return ret_val;
}

shcore::Value Interactive_dba_cluster::rejoin_instance(
    const shcore::Argument_list &args) {
  shcore::Value ret_val;
  // Throw an error if the cluster has already been dissolved
  assert_valid("rejoinInstance");

  args.ensure_count(1, 2, get_function_name("rejoinInstance").c_str());

  check_preconditions("rejoinInstance");

  shcore::Value::Map_type_ref instance_map, options;
  mysqlshdk::db::Connection_options instance_def;

  try {
    instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    if (args.size() == 2) {
      options = args.map_at(1);
      shcore::Argument_map options_map(*options);
      options_map.ensure_keys({},
          mysqlsh::dba::ReplicaSet::_add_instance_opts, "instance definition");

      // Validate SSL options for the cluster instance
      mysqlsh::dba::validate_ssl_instance_options(options);

      // Validate ip whitelist option
      mysqlsh::dba::validate_ip_whitelist_option(options);
    }

    std::string message =
        "Rejoining the instance to the InnoDB cluster. "
        "Depending on the original\n"
        "problem that made the instance unavailable, the rejoin operation "
        "might not be\n"
        "successful and further manual steps will be needed to fix the "
        "underlying\n"
        "problem.\n"
        "\n"
        "Please monitor the output of the rejoin operation and take necessary "
        "action if\n"
        "the instance cannot rejoin.\n\n";

    print(message);

    mysqlsh::resolve_connection_credentials(&instance_def, _delegate);

    instance_map = mysqlsh::get_connection_map(instance_def);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_map));

  if (options)
    new_args.push_back(args[1]);

  println("Rejoining instance to the cluster ...");
  println();
  ret_val = call_target("rejoinInstance", new_args);

  println("The instance '" +
    instance_def.as_uri(only_transport()) + ""
    "' was successfully rejoined on the cluster.");
  println();

  return ret_val;
}

shcore::Value Interactive_dba_cluster::remove_instance(
      const shcore::Argument_list &args) {
  shcore::Value ret_val;
  shcore::Value::Map_type_ref instance_map;  // Map with the connection data
  mysqlshdk::db::Connection_options instance_def;

  // Throw an error if the cluster has already been dissolved
  assert_valid("removeInstance");

  args.ensure_count(1, 2, get_function_name("removeInstance").c_str());

  check_preconditions("removeInstance");

  // Retrieve the 'force' option value (if used).
  bool force = false;  // force is false by default.
  try {
    if (args.size() == 2) {
      auto remove_options = args.map_at(1);
      shcore::Argument_map remove_options_map(*remove_options);
      if (remove_options->has_key("force"))
        force = remove_options->get_bool("force");
    }

    std::string message =
        "The instance will be removed from the InnoDB cluster. "
        "Depending on the \n"
        "instance being the Seed or not, the Metadata session might become "
        "invalid. \n"
        "If so, please start a new session to the Metadata Storage R/W "
        "instance.\n\n";

    print(message);

    instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeInstance"));

  ret_val = call_target("removeInstance", args);

  try {
    println("The instance '" +
      instance_def.as_uri(mysqlshdk::db::uri::formats::user_transport()) + ""
      "' was successfully removed from the cluster.");

    println();

    // User and pass might not be specified in args and in that case we need to
    // get them from the metadata session.
    if (!instance_def.has_user() || !instance_def.has_password()) {

      auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
      auto cluster_session = cluster->get_group_session();
      auto cluster_cnx_opts = cluster_session->get_connection_options();

      if (!instance_def.has_user() && cluster_cnx_opts.has_user())
        instance_def.set_user(cluster_cnx_opts.get_user());

      if (!instance_def.has_password() && cluster_cnx_opts.has_password())
        instance_def.set_password(cluster_cnx_opts.get_password());
    }
    // Issue a warning for user to manually disable GR in the configuration file
    // for servers that do not support the SET PERSIST feature (< 8.0.4).
    // NOTE: Supported version to use SET PERSIST must be >= 8.0.4 due to
    //       BUG#26495619.
    try {
      std::shared_ptr<mysqlshdk::db::ISession> _session =
          mysqlshdk::db::mysql::Session::create();
      _session->connect(instance_def);
      mysqlshdk::mysql::Instance *instance =
          new mysqlshdk::mysql::Instance(_session);
      if (instance->get_version() < mysqlshdk::utils::Version(8, 0, 4)) {
        println("WARNING: The 'group_replication_start_on_boot' variable must "
                "be set to 'OFF' in the server configuration file, otherwise "
                "it might silently rejoin the cluster upon restart.");
        println();
      }
      _session->close();
    } catch (std::exception&) {
      // If the force option was used then the instance is most likely not
      // available, therefore an error is expected when trying to connect and in
      // that case the error should be ignored (not reported).
      if (!force)
        throw;
    }
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeInstance"));

  return ret_val;
}

shcore::Value Interactive_dba_cluster::dissolve(
      const shcore::Argument_list &args) {
  shcore::Value ret_val;
  bool force = false;
  shcore::Value::Map_type_ref options;

  // Throw an error if the cluster has already been dissolved
  assert_valid("dissolve");

  args.ensure_count(0, 1, get_function_name("dissolve").c_str());

  check_preconditions("dissolve");

  try {
    if (args.size() == 1)
      options = args.map_at(0);

    if (options) {
      shcore::Argument_map opt_map(*options);

      opt_map.ensure_keys({}, {"force"}, "dissolve options");

      if (opt_map.has_key("force"))
        force = opt_map.bool_at("force");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dissolve"));

  if (!force) {
    std::shared_ptr<mysqlsh::dba::ReplicaSet> object;
    auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
    if (cluster)
      object = cluster->get_default_replicaset();

    if (object) {
      println("The cluster still has active ReplicaSets.");
      println("Please use cluster.dissolve({force: true}) to deactivate "
              "replication");
      println("and unregister the ReplicaSets from the cluster.");
      println();

      println("The following replicasets are currently registered:");

      ret_val = call_target("describe", shcore::Argument_list());
    }
  } else {
    ret_val = call_target("dissolve", args);

    println("The cluster was successfully dissolved.");
    println("Replication was disabled but user data was left intact.");
    println();
  }

  return ret_val;
}

shcore::Value Interactive_dba_cluster::check_instance_state(
      const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("checkInstanceState");

  args.ensure_count(1, 2, get_function_name("checkInstanceState").c_str());

  check_preconditions("checkInstanceState");

  mysqlshdk::db::Connection_options instance_def;
  shcore::Argument_list target_args;

  try {
    instance_def =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::STRING);

    // Gather username and password if missing
    mysqlsh::resolve_connection_credentials(&instance_def, _delegate);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("checkInstanceState"));

  println("Analyzing the instance replication state...");

  auto instance_map = mysqlsh::get_connection_map(instance_def);
  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_map));
  shcore::Value ret_val = call_target("checkInstanceState", new_args);

  auto result = ret_val.as_map();

  println();

  if (result->get_string("state") == "ok") {
    println("The instance '" +
      instance_def.as_uri(user_transport()) + "' is valid for "
      "the cluster.");

    if (result->get_string("reason") == "new")
      println("The instance is new to Group Replication.");
    else
      println("The instance is fully recoverable.");
  } else {
    println("The instance '" +
      instance_def.as_uri(user_transport()) + "' is invalid for "
      "the cluster.");

    if (result->get_string("reason") == "diverged")
      println("The instance contains additional transactions in relation to "
              "the cluster.");
    else
      println("There are transactions in the cluster that can't be recovered "
              "on the instance.");
  }
  println();

  return ret_val;
}

shcore::Value Interactive_dba_cluster::rescan(
      const shcore::Argument_list &args) {
  shcore::Value ret_val;

  // Throw an error if the cluster has already been dissolved
  assert_valid("rescan");

  args.ensure_count(0, get_function_name("rescan").c_str());

  check_preconditions("rescan");

  println("Rescanning the cluster...");
  println();

  println("Result of the rescanning operation:");

  shcore::Value rescan_report = call_target("rescan", shcore::Argument_list());

  print_value(rescan_report, "");

  auto result = rescan_report.as_map();

  // We only support 1 ReplicaSet now, the DefaultReplicaSet
  if (result->has_key("defaultReplicaSet")) {
    auto default_rs = result->get_map("defaultReplicaSet");
    auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
    // Check if there are unknown instances
    if (default_rs->has_key("newlyDiscoveredInstances")) {
      auto unknown_instances =
          default_rs->get_array("newlyDiscoveredInstances");

      for (auto instance : *unknown_instances) {
        auto instance_map = instance.as_map();
        println();
        println("A new instance '" + instance_map->get_string("host") +
                "' was discovered in the HA setup.");

        if (prompt("Would you like to add it to the cluster metadata?")
            == Prompt_answer::YES) {
          std::string full_host = instance_map->get_string("host");
          auto instance_def = shcore::get_connection_options(full_host, false);

          mysqlsh::resolve_connection_credentials(&instance_def, _delegate);

          println("Adding instance to the cluster metadata...");
          println();

          std::shared_ptr<mysqlsh::dba::ReplicaSet> object;

          object = cluster->get_default_replicaset();

          object->add_instance_metadata(instance_def);

          println("The instance '" +
            instance_def.as_uri(user_transport()) + "' was "
            "successfully added to the cluster metadata.");
          println();
        }
      }
    }

    // Check if there are missing instances
    if (default_rs->has_key("unavailableInstances")) {
      auto missing_instances = default_rs->get_array("unavailableInstances");

      for (auto instance : *missing_instances) {
        auto instance_map = instance.as_map();
        println();
        println("The instance '" + instance_map->get_string("host") +
                "' is no longer part of the HA setup. "
                "It is either offline or left the HA group.");
        println("You can try to add it to the cluster again with the "
                "cluster.rejoinInstance('" + instance_map->get_string("host") +
                "') command or you can remove it from the cluster "
                "configuration.");

        if (prompt("Would you like to remove it from the cluster metadata?") ==
            Prompt_answer::YES) {
          std::string full_host = instance_map->get_string("host");
          auto instance_def = shcore::get_connection_options(full_host, false);

          println("Removing instance from the cluster metadata...");
          println();

          std::shared_ptr<mysqlsh::dba::ReplicaSet> object;

          object = cluster->get_default_replicaset();

          object->remove_instance_metadata(instance_def);

          println("The instance '" +
            instance_def.as_uri(user_transport()) + "' was "
            "successfully removed from the cluster metadata.");

          println();
        }
      }
    }
  }

  return shcore::Value();
}

shcore::Value Interactive_dba_cluster::force_quorum_using_partition_of(
      const shcore::Argument_list &args) {
  shcore::Value ret_val;

  // Throw an error if the cluster has already been dissolved
  assert_valid("forceQuorumUsingPartitionOf");

  args.ensure_count(1, 2,
                    get_function_name("forceQuorumUsingPartitionOf").c_str());

  check_preconditions("forceQuorumUsingPartitionOf");

  mysqlshdk::db::Connection_options instance_def;

  try {
    instance_def =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::STRING);

    std::shared_ptr<mysqlsh::dba::ReplicaSet> default_replica_set;
    auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster> (_target);

    default_replica_set = cluster->get_default_replicaset();

    std::string rs_name;

    if (default_replica_set)
      rs_name = default_replica_set->get_member("name").as_string();
    else
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    std::vector<std::string> online_instances_array =
        default_replica_set->get_online_instances();

    if (online_instances_array.empty())
      throw shcore::Exception::logic_error("No online instances are visible "
                                           "from the given one.");

    auto group_peers = shcore::str_join(online_instances_array, ",");

    // Remove the trailing comma of group_peers
    if (group_peers.back() == ',')
      group_peers.pop_back();

    std::string message = "Restoring replicaset '" + rs_name + "'"
                          " from loss of quorum, by using the partition "
                          "composed of [" + group_peers + "]\n\n";
    print(message);

    mysqlsh::resolve_connection_credentials(&instance_def, _delegate);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("forceQuorumUsingPartitionOf"));

  auto instance_map = mysqlsh::get_connection_map(instance_def);
  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_map));

  println("Restoring the InnoDB cluster ...");
  println();
  ret_val = call_target("forceQuorumUsingPartitionOf", new_args);

  println(
      "The InnoDB cluster was successfully restored using the partition from "
      "the instance '" +
      instance_def.as_uri(user_transport()) + "'.");
  println();
  println(
      "WARNING: To avoid a split-brain scenario, ensure that all other members "
      "of the replicaset "
      "are removed or joined back to the group that was restored.");
  println();

  return ret_val;
}
