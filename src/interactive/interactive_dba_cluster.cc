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

#include "interactive_dba_cluster.h"
#include <string>
#include <vector>
#include "db/mysql/session.h"
#include "interactive_global_dba.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysql/instance.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/version.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

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
  add_varargs_method(
      "forceQuorumUsingPartitionOf",
      std::bind(&Interactive_dba_cluster::force_quorum_using_partition_of, this,
                _1));
}

mysqlsh::dba::Cluster_check_info Interactive_dba_cluster::check_preconditions(
    const std::string &function_name) const {
  ScopedStyle ss(_target.get(), naming_style);
  auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
  return cluster->check_preconditions(function_name);
}

void Interactive_dba_cluster::assert_valid(
    const std::string &function_name) const {
  ScopedStyle ss(_target.get(), naming_style);
  auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
  cluster->assert_valid(function_name);
}

shcore::Value Interactive_dba_cluster::add_seed_instance(
    const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string function;
  std::shared_ptr<mysqlsh::dba::ReplicaSet> object;

  auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);

  if (cluster) {
    // Throw an error if the cluster has already been dissolved
    assert_valid("addInstance");
    object = cluster->get_default_replicaset();
  }
  if (object) {
    if (confirm("The default ReplicaSet is already initialized. \
                Do you want to add a new instance?") ==
        mysqlsh::Prompt_answer::YES)
      function = "addInstance";
  } else {
    function = "addSeedInstance";
  }

  if (!function.empty()) {
    auto instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    auto instance_map = mysqlsh::get_connection_map(instance_def);

    shcore::Argument_list new_args;
    new_args.push_back(shcore::Value(instance_map));

    if (args.size() == 2) new_args.push_back(args[1]);

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

  shcore::Value::Map_type_ref options;
  shcore::Value::Map_type_ref instance_map;
  mysqlshdk::db::Connection_options instance_def;

  try {
    check_preconditions("addInstance");

    std::shared_ptr<mysqlsh::dba::ReplicaSet> object;
    auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);
    if (cluster) object = cluster->get_default_replicaset();

    if (!object) {
      if (confirm("The default ReplicaSet is not initialized. Do you want to \
                  initialize it adding a seed instance?") ==
          mysqlsh::Prompt_answer::YES)
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

        std::string ssl_mode, ip_whitelist, instance_label, local_address,
            group_seeds, exit_state_action;

        mysqlshdk::utils::nullable<int64_t> member_weight;

        // Retrieves optional options if exists
        mysqlsh::Unpack_options(options)
            .optional("memberSslMode", &ssl_mode)
            .optional("ipWhitelist", &ip_whitelist)
            .optional("label", &instance_label)
            .optional("localAddress", &local_address)
            .optional("groupSeeds", &group_seeds)
            .optional("exitStateAction", &exit_state_action)
            .optional("memberWeight", &member_weight)
            .end();

        // Validate SSL options for the cluster instance
        mysqlsh::dba::validate_ssl_instance_options(options);
      }

      instance_def.set_default_connection_data();

      instance_map = mysqlsh::get_connection_map(instance_def);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_map));

  if (options) new_args.push_back(args[1]);

  println("Adding instance to the cluster ...");
  println();
  ret_val = call_target(function, new_args);

  println("The instance '" + instance_def.as_uri(user_transport()) +
          ""
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

  shcore::Value::Map_type_ref instance_map, options;
  mysqlshdk::db::Connection_options instance_def;

  try {
    check_preconditions("rejoinInstance");

    instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    if (args.size() == 2) {
      options = args.map_at(1);
      shcore::Argument_map options_map(*options);
      options_map.ensure_keys({},
                              mysqlsh::dba::ReplicaSet::_rejoin_instance_opts,
                              "instance definition");

      // Validate SSL options for the cluster instance
      mysqlsh::dba::validate_ssl_instance_options(options);
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

    instance_def.set_default_connection_data();

    instance_map = mysqlsh::get_connection_map(instance_def);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_map));

  if (options) new_args.push_back(args[1]);

  println("Rejoining instance to the cluster ...");
  println();
  ret_val = call_target("rejoinInstance", new_args);

  println("The instance '" + instance_def.as_uri(only_transport()) +
          ""
          "' was successfully rejoined on the cluster.");
  println();

  return ret_val;
}

shcore::Value Interactive_dba_cluster::force_quorum_using_partition_of(
    const shcore::Argument_list &args) {
  shcore::Value ret_val;

  // Throw an error if the cluster has already been dissolved
  assert_valid("forceQuorumUsingPartitionOf");

  args.ensure_count(1, 2,
                    get_function_name("forceQuorumUsingPartitionOf").c_str());

  mysqlshdk::db::Connection_options instance_def;

  try {
    check_preconditions("forceQuorumUsingPartitionOf");

    instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::STRING);

    std::shared_ptr<mysqlsh::dba::ReplicaSet> default_replica_set;
    auto cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(_target);

    default_replica_set = cluster->get_default_replicaset();

    std::string rs_name;

    if (default_replica_set)
      rs_name = default_replica_set->get_member("name").get_string();
    else
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    std::vector<std::string> online_instances_array =
        default_replica_set->get_online_instances();

    if (online_instances_array.empty())
      throw shcore::Exception::logic_error(
          "No online instances are visible "
          "from the given one.");

    auto group_peers = shcore::str_join(online_instances_array, ",");

    // Remove the trailing comma of group_peers
    if (group_peers.back() == ',') group_peers.pop_back();

    std::string message = "Restoring replicaset '" + rs_name +
                          "'"
                          " from loss of quorum, by using the partition "
                          "composed of [" +
                          group_peers + "]\n\n";
    print(message);

    instance_def.set_default_connection_data();
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
