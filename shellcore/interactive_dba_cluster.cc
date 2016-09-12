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
#include "shellcore/shell_registry.h"
#include "modules/mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include <boost/format.hpp>
#include <string>

using namespace std::placeholders;
using namespace shcore;

void Interactive_dba_cluster::init() {
  add_method("addInstance", std::bind(&Interactive_dba_cluster::add_instance, this, _1), "data");
  add_method("rejoinInstance", std::bind(&Interactive_dba_cluster::rejoin_instance, this, _1), "data");
  add_method("removeInstance", std::bind(&Interactive_dba_cluster::remove_instance, this, _1), "data");
  add_varargs_method("dissolve", std::bind(&Interactive_dba_cluster::dissolve, this, _1));
}

shcore::Value Interactive_dba_cluster::add_seed_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string function;

  std::shared_ptr<mysh::dba::ReplicaSet> object;
  auto cluster = std::dynamic_pointer_cast<mysh::dba::Cluster>(_target);
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
    shcore::Value::Map_type_ref options;

    if (resolve_instance_options(function, args, options)) {
      shcore::Argument_list new_args;
      new_args.push_back(shcore::Value(options));
      ret_val = _target->call(function, new_args);
    }
  }

  return ret_val;
}

shcore::Value Interactive_dba_cluster::add_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string function;

  std::shared_ptr<mysh::dba::ReplicaSet> object;
  auto cluster = std::dynamic_pointer_cast<mysh::dba::Cluster>(_target);
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
    shcore::Value::Map_type_ref options;

    std::string message = "A new instance will be added to the InnoDB cluster. Depending on the amount of\n"
                          "data on the cluster this might take from a few seconds to several hours.\n\n";

    print(message);

    if (resolve_instance_options(function, args, options)) {
      shcore::Argument_list new_args;
      new_args.push_back(shcore::Value(options));

      print("Adding instance to the cluster ...\n");
      ret_val = _target->call(function, new_args);

      print("The instance '" + build_connection_string(options, false) + "' was successfully added to the cluster.\n");
    }
  }
  return ret_val;
}

shcore::Value Interactive_dba_cluster::rejoin_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  shcore::Value::Map_type_ref options;

  std::string message = "The instance will try rejoining the InnoDB cluster. Depending on the original\n"
                        "problem that made the instance unavailable the rejoin, operation might not be\n"
                        "successful and further manual steps will be needed to fix the underlying\n"
                        "problem.\n"
                        "\n"
                        "Please monitor the output of the rejoin operation and take necessary action if\n"
                        "the instance cannot rejoin.\n";

  std::string answer;
  if (password("Please provide the password for '" + args.string_at(0) + "': ", answer)) {
    shcore::Argument_list new_args;
    new_args.push_back(args[0]);
    new_args.push_back(shcore::Value(answer));
    print(message);
    ret_val = _target->call("rejoinInstance", new_args);
  }

  return ret_val;
}

bool Interactive_dba_cluster::resolve_instance_options(const std::string& function, const shcore::Argument_list &args, shcore::Value::Map_type_ref &options) const {
  std::string answer;
  args.ensure_count(1, 2, get_function_name(function).c_str());

  bool proceed = true;

  // Identify the type of connection data (String or Document)
  if (args[0].type == String)
    options = get_connection_data(args.string_at(0), false);
  // Connection data comes in a dictionary
  else if (args[0].type == Map)
    options = args.map_at(0);
  else
    throw shcore::Exception::argument_error("Invalid connection options, expected either a URI or a Dictionary.");

  auto invalids = shcore::get_additional_keys(options, mysh::dba::ReplicaSet::_add_instance_opts);

  // Verification of invalid attributes on the connection data
  if (invalids.size()) {
    std::string error = "The connection data contains the following invalid attributes: ";
    error += shcore::join_strings(invalids, ", ");

    proceed = false;
    if (prompt((boost::format("%s. Do you want to ignore these attributes and continue? [Y/n]: ") % error).str().c_str(), answer)) {
      proceed = (!answer.compare("y") || !answer.compare("Y") || answer.empty());

      if (proceed) {
        for (auto attribute : invalids)
          options->erase(attribute);
      }
    }
  }

  // Verification of the host attribute
  if (proceed && !options->has_key("host")) {
    if (prompt("The connection data is missing the host, would you like to: 1) Use localhost  2) Specify a host  3) Cancel  Please select an option [1]: ", answer)) {
      if (answer == "1")
        (*options)["host"] = shcore::Value("localhost");
      else if (answer == "2") {
        if (prompt("Please specify the host: ", answer))
          (*options)["host"] = shcore::Value(answer);
        else
          proceed = false;
      } else
        proceed = false;
    }
  }

  if (proceed) {
    // Verification of the user attribute
    std::string user;
    if (options->has_key("user"))
      user = options->get_string("user");
    else if (options->has_key("dbUser"))
      user = options->get_string("dbUser");
    else {
      user = "root";
      (*options)["dbUser"] = shcore::Value(user);
    }

    // Verification of the password
    std::string user_password;
    bool has_password = true;
    if (options->has_key("password"))
      user_password = options->get_string("password");
    else if (options->has_key("dbPassword"))
      user_password = options->get_string("dbPassword");
    else if (args.size() == 2) {
      user_password = args.string_at(1);
      (*options)["dbPassword"] = shcore::Value(user_password);
    } else
      has_password = false;

    if (!has_password) {
      proceed = false;
      if (password("Please provide the password for '" + build_connection_string(options, false) + "': ", answer)) {
        (*options)["dbPassword"] = shcore::Value(answer);
        proceed = true;
      }
    }
  }

  return proceed;
}

shcore::Value Interactive_dba_cluster::remove_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  args.ensure_count(1, get_function_name("removeInstance").c_str());

  std::string message = "The instance will be removed from the InnoDB cluster. Depending on the \n"
                        "instance being the Seed or not, the Metadata session might become invalid. \n"
                        "If so, please start a new session to the Metadata Storage R/W instance.\n\n";

  print(message);

  // Identify the type of connection data (String or Document)
  if (args[0].type == String) {
    uri = args.string_at(0);
    options = get_connection_data(uri, false);
  }

  // TODO: what if args[0] is a String containing the name of the instance?

  // Connection data comes in a dictionary
  else if (args[0].type == Map)
    options = args.map_at(0);

  ret_val = _target->call("removeInstance", args);

  println("The instance '" + build_connection_string(options, false) + "' was successfully removed from the cluster.");

  return ret_val;
}

shcore::Value Interactive_dba_cluster::dissolve(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  bool force = false;
  shcore::Value::Map_type_ref options;

  args.ensure_count(0, 1, get_function_name("dissolve").c_str());

  try {
    if (args.size() == 1)
        options = args.map_at(0);

    if (options) {
      // Verification of invalid attributes on the instance creation options
      auto invalids = shcore::get_additional_keys(options, { "force", "verbose", });
      if (invalids.size()) {
        std::string error = "The options contain the following invalid attributes: ";
        error += shcore::join_strings(invalids, ", ");
        throw shcore::Exception::argument_error(error);
      }

      if (options->has_key("force") && (*options)["force"].type != shcore::Bool)
        throw shcore::Exception::type_error("Invalid data type for 'force' field, should be a boolean");
      else
        force = options->get_bool("force");
    }
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dissolve"));

  if (!force) {
    std::shared_ptr<mysh::dba::ReplicaSet> object;
    auto cluster = std::dynamic_pointer_cast<mysh::dba::Cluster>(_target);

    if (cluster)
      object = cluster->get_default_replicaset();

    if (object) {
      println("The cluster still has active ReplicaSets.");
      println("Please use cluster.dissolve({force: true}) to deactivate replication");
      println("and unregister the ReplicaSets from the cluster.");
      println();

      println("The following replicasets are currently registered:");

      ret_val = _target->call("describe", shcore::Argument_list());
    }
  } else {
    ret_val = _target->call("dissolve", args);

    println("The cluster was successfully dissolved.");
    println("Replication was disabled but user data was left intact.");
  }

  return ret_val;
}
