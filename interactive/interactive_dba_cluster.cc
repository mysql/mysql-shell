/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "interactive_dba_cluster.h"
#include "interactive_global_dba.h"
#include "modules/adminapi/mod_dba_cluster.h"
//#include "modules/adminapi/mod_dba_instance.h"
#include "modules/mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include "modules/adminapi/mod_dba_common.h"
#include <boost/format.hpp>
#include <string>
#include <vector>

using namespace std::placeholders;
using namespace shcore;

void Interactive_dba_cluster::init() {
  add_method("addInstance", std::bind(&Interactive_dba_cluster::add_instance, this, _1), "data");
  add_method("rejoinInstance", std::bind(&Interactive_dba_cluster::rejoin_instance, this, _1), "data");
  add_method("removeInstance", std::bind(&Interactive_dba_cluster::remove_instance, this, _1), "data");
  add_varargs_method("dissolve", std::bind(&Interactive_dba_cluster::dissolve, this, _1));
  add_varargs_method("checkInstanceState", std::bind(&Interactive_dba_cluster::check_instace_state, this, _1));
  add_varargs_method("rescan", std::bind(&Interactive_dba_cluster::rescan, this, _1));
  add_varargs_method("forceQuorumUsingPartitionOf", std::bind(&Interactive_dba_cluster::force_quorum_using_partition_of, this, _1));
}

mysqlsh::dba::ReplicationGroupState Interactive_dba_cluster::check_preconditions(const std::string& function_name) const {
  ScopedStyle ss(_target.get(), naming_style);
  auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
  return cluster->check_preconditions(function_name);
}

shcore::Value Interactive_dba_cluster::add_seed_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string function;

  std::shared_ptr<mysqlsh::dba::ReplicaSet> object;
  auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster> (_target);

  if (cluster)
    object = cluster->get_default_replicaset();

  if (object) {
    std::string answer;

    if (prompt("The default ReplicaSet is already initialized. Do you want to add a new instance? [Y|n]: ", answer)) {
      if (!answer.compare("y") || !answer.compare("Y") || answer.empty())
        function = "addInstance";
    }
  } else
    function = "addSeedInstance";

  if (!function.empty()) {
    auto options = mysqlsh::dba::get_instance_options_map(args, false);
    mysqlsh::dba::resolve_instance_credentials(options, _delegate);

    shcore::Argument_list new_args;
    new_args.push_back(shcore::Value(options));
    ret_val = call_target(function, new_args);
  }

  return ret_val;
}

shcore::Value Interactive_dba_cluster::add_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string function;

  args.ensure_count(1, 2, get_function_name("addInstance").c_str());

  check_preconditions("addInstance");

  shcore::Value::Map_type_ref options;

  try {
    std::shared_ptr<mysqlsh::dba::ReplicaSet> object;
    auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
    if (cluster)
      object = cluster->get_default_replicaset();

    if (!object) {
      std::string answer;

      if (prompt("The default ReplicaSet is not initialized. Do you want to initialize it adding a seed instance? [Y|n]: ", answer)) {
        if (!answer.compare("y") || !answer.compare("Y") || answer.empty())
          function = "addSeedInstance";
      }
    } else
      function = "addInstance";

    if (!function.empty()) {
      std::string message = "A new instance will be added to the InnoDB cluster. Depending on the amount of\n"
                            "data on the cluster this might take from a few seconds to several hours.\n\n";

      print(message);

      options = mysqlsh::dba::get_instance_options_map(args, false);

      shcore::Argument_map opt_map(*options);
      opt_map.ensure_keys({"host"}, mysqlsh::dba::ReplicaSet::_add_instance_opts, "instance definition");

      // Validate SSL options for the cluster instance
      mysqlsh::dba::validate_ssl_instance_options(options);

      mysqlsh::dba::resolve_instance_credentials(options, _delegate);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  if (options) {
    shcore::Argument_list new_args;
    new_args.push_back(shcore::Value(options));

    println("Adding instance to the cluster ...");
    println();
    ret_val = call_target(function, new_args);

    println("The instance '" + build_connection_string(options, false) + "' was successfully added to the cluster.");
    println();
  }

  return ret_val;
}

shcore::Value Interactive_dba_cluster::rejoin_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("rejoinInstance").c_str());

  check_preconditions("rejoinInstance");

  shcore::Value::Map_type_ref options;

  try {
    options = mysqlsh::dba::get_instance_options_map(args, false);

    shcore::Argument_map opt_map(*options);
    opt_map.ensure_keys({"host"}, mysqlsh::dba::ReplicaSet::_add_instance_opts, "instance definition");

    // Validate SSL options for the cluster instance
    mysqlsh::dba::validate_ssl_instance_options(options);

    std::string message = "Rejoining the instance to the InnoDB cluster. Depending on the original\n"
                        "problem that made the instance unavailable, the rejoin operation might not be\n"
                        "successful and further manual steps will be needed to fix the underlying\n"
                        "problem.\n"
                        "\n"
                        "Please monitor the output of the rejoin operation and take necessary action if\n"
                        "the instance cannot rejoin.\n\n";

    print(message);

    mysqlsh::dba::resolve_instance_credentials(options, _delegate);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  if (options) {
    shcore::Argument_list new_args;
    new_args.push_back(shcore::Value(options));

    println("Rejoining instance to the cluster ...");
    println();
    ret_val = call_target("rejoinInstance", new_args);

    println("The instance '" + build_connection_string(options, false) + "' was successfully rejoined on the cluster.");
    println();
  }

  return ret_val;
}

shcore::Value Interactive_dba_cluster::remove_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  args.ensure_count(1, get_function_name("removeInstance").c_str());

  check_preconditions("removeInstance");

  std::string message = "The instance will be removed from the InnoDB cluster. Depending on the \n"
                        "instance being the Seed or not, the Metadata session might become invalid. \n"
                        "If so, please start a new session to the Metadata Storage R/W instance.\n\n";

  print(message);

  std::string name;

  //auto instance = args.object_at<mysqlsh::dba::Instance> (0);

  // Identify the type of connection data (String or Document)
  if (args[0].type == String) {
    uri = args.string_at(0);
    options = get_connection_data(uri, false);
  }

  // TODO: what if args[0] is a String containing the name of the instance?

  // Connection data comes in a dictionary
  else if (args[0].type == Map)
    options = args.map_at(0);

  else
    throw shcore::Exception::argument_error("Invalid connection options, expected either a URI or a Dictionary");

  //if (instance)
  //  name = instance->get_name();
  //else
  name = build_connection_string(options, false);

  ret_val = call_target("removeInstance", args);

  println("The instance '" + name + "' was successfully removed from the cluster.");
  println();

  return ret_val;
}

shcore::Value Interactive_dba_cluster::dissolve(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  bool force = false;
  shcore::Value::Map_type_ref options;

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
      println("Please use cluster.dissolve({force: true}) to deactivate replication");
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

shcore::Value Interactive_dba_cluster::check_instace_state(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("checkInstanceState").c_str());

  check_preconditions("checkInstanceState");

  shcore::Value::Map_type_ref options;
  shcore::Argument_list target_args;

  try {
    options = mysqlsh::dba::get_instance_options_map(args, false);

    shcore::Argument_map opt_map(*options);
    std::set<std::string> check_instance_config_opts = {"host", "port", "user", "dbUser", "password", "dbPassword", "socket", "sslCa", "sslCert", "sslKey"};
    opt_map.ensure_keys({"host", "port"}, check_instance_config_opts, "instance definition");

    // Gather username and password if missing
    mysqlsh::dba::resolve_instance_credentials(options, _delegate);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("checkInstanceState"));

  println("Analyzing the instance replication state...");

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(options));
  shcore::Value ret_val = call_target("checkInstanceState", new_args);

  auto result = ret_val.as_map();

  println();

  if (result->get_string("state") == "ok") {
    println("The instance '" + options->get_string("host") + ":" + std::to_string(options->get_int("port")) + "' is valid for the cluster.");

    if (result->get_string("reason") == "new")
      println("The instance is new to Group Replication.");
    else
      println("The instance is fully recoverable.");
  } else {
    println("The instance '" + options->get_string("host") + ":" + std::to_string(options->get_int("port")) + "' is invalid for the cluster.");

    if (result->get_string("reason") == "diverged")
      println("The instance contains additional transactions in relation to the cluster.");
    else
      println("There are transactions in the cluster that can't be recovered on the instance.");
  }
  println();

  return ret_val;
}

shcore::Value Interactive_dba_cluster::rescan(const shcore::Argument_list &args) {
  shcore::Value ret_val;

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
      auto unknown_instances = default_rs->get_array("newlyDiscoveredInstances");

      for (auto instance : *unknown_instances) {
        auto instance_map = instance.as_map();
        println();
        println("A new instance '" + instance_map->get_string("host") + "' was discovered in the HA setup.");

        std::string answer;
        if (prompt("Would you like to add it to the cluster metadata? [Y|n]: ", answer)) {
          if (!answer.compare("y") || !answer.compare("Y") || answer.empty()) {
            std::string full_host = instance_map->get_string("host");

            Value::Map_type_ref options(new shcore::Value::Map_type);

            std::string delimiter = ":";
            std::string host = full_host.substr(0, full_host.find(delimiter));
            std::string port = full_host.substr(full_host.find(delimiter) + 1, full_host.length());

            (*options)["host"] = shcore::Value(host);
            (*options)["port"] = shcore::Value(atoi(port.c_str()));
            mysqlsh::dba::resolve_instance_credentials(options, _delegate);

            println("Adding instance to the cluster metadata...");
            println();

            std::shared_ptr<mysqlsh::dba::ReplicaSet> object;

            object = cluster->get_default_replicaset();

            object->add_instance_metadata(options);

            println("The instance '" + build_connection_string(options, false) + "' was successfully added to the cluster metadata.");
            println();
          }
        }
      }
    }

    // Check if there are missing instances
    if (default_rs->has_key("unavailableInstances")) {
      auto missing_instances = default_rs->get_array("unavailableInstances");

      for (auto instance : *missing_instances) {
        auto instance_map = instance.as_map();
        println();
        println("The instance '" + instance_map->get_string("host") + "' is no longer part of the HA setup. "
                "It is either offline or left the HA group.");
        println("You can try to add it to the cluster again with the cluster.rejoinInstance('" + instance_map->get_string("host") + "') "
                "command or you can remove it from the cluster configuration.");

        std::string answer;
        if (prompt("Would you like to remove it from the cluster metadata? [Y|n]: ", answer)) {
          if (!answer.compare("y") || !answer.compare("Y") || answer.empty()) {
            shcore::Argument_list args;

            std::string full_host = instance_map->get_string("host");

            Value::Map_type_ref options(new shcore::Value::Map_type);

            std::string delimiter = ":";
            std::string host = full_host.substr(0, full_host.find(delimiter));
            std::string port = full_host.substr(full_host.find(delimiter) + 1, full_host.length());

            (*options)["host"] = shcore::Value(host);
            (*options)["port"] = shcore::Value(atoi(port.c_str()));
            args.push_back(shcore::Value(options));

            println("Removing instance from the cluster metadata...");
            println();

            std::shared_ptr<mysqlsh::dba::ReplicaSet> object;

            object = cluster->get_default_replicaset();

            object->remove_instance_metadata(args);

            println("The instance '" + build_connection_string(options, false) + "' was successfully removed from the cluster metadata.");
            println();
          }
        }
      }
    }
  }

  return shcore::Value();
}

shcore::Value Interactive_dba_cluster::force_quorum_using_partition_of(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("forceQuorumUsingPartitionOf").c_str());

  check_preconditions("forceQuorumUsingPartitionOf");

  shcore::Value::Map_type_ref options;

  try {
    options = mysqlsh::dba::get_instance_options_map(args, false);

    shcore::Argument_map opt_map(*options);
    opt_map.ensure_keys({"host"},
                        {"name", "host", "port", "user", "dbUser", "password",
                         "dbPassword", "socket", "memberSslCa", "memberSslCert", "memberSslKey", "memberSsl"},
                        "instance definition");

    // Validate SSL options for the cluster instance
    mysqlsh::dba::validate_ssl_instance_options(options);

    std::shared_ptr<mysqlsh::dba::ReplicaSet> default_replica_set;
    auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster> (_target);

    default_replica_set = cluster->get_default_replicaset();

    std::string rs_name;

    if (default_replica_set)
      rs_name = default_replica_set->get_member("name").as_string();
    else
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    std::vector<std::string> online_instances_array = default_replica_set->get_online_instances();

    if (online_instances_array.empty())
      throw shcore::Exception::logic_error("No online instances are visible from the given one.");

    auto group_peers = shcore::join_strings(online_instances_array, ",");

    // Remove the trailing comma of group_peers
    if (group_peers.back() == ',')
      group_peers.pop_back();

    std::string message = "Restoring replicaset '" + rs_name + "'"
                          " from loss of quorum, by using the partition composed of [" + group_peers + "]\n\n";
    print(message);

    mysqlsh::dba::resolve_instance_credentials(options, _delegate);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("forceQuorumUsingPartitionOf"));

  if (options) {
    shcore::Argument_list new_args;
    new_args.push_back(shcore::Value(options));

    println("Restoring the InnoDB cluster ...");
    println();
    ret_val = call_target("forceQuorumUsingPartitionOf", new_args);

    println("The InnoDB cluster was successfully restored using the partition from the instance '" +
             build_connection_string(options, false) + "'.");
    println();
    println("WARNING: To avoid a split-brain scenario, ensure that all other members of the replicaset "
            "are removed or joined back to the group that was restored.");
    println();
  }

  return ret_val;
}
