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

#include "interactive_global_dba.h"
#include "shellcore/shell_registry.h"
#include "shellcore/interactive_dba_cluster.h"
#include "modules/mysqlxtest_utils.h"
#include "modules/adminapi/mod_dba.h"
#include "utils/utils_general.h"
#include "utils/utils_file.h"
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace shcore;

void Global_dba::init() {
  add_varargs_method("deployLocalInstance", std::bind(&Global_dba::deploy_local_instance, this, _1, "deployLocalInstance"));
  add_varargs_method("startLocalInstance", std::bind(&Global_dba::deploy_local_instance, this, _1, "startLocalInstance"));
  add_varargs_method("deleteLocalInstance", std::bind(&Global_dba::delete_local_instance, this, _1));
  add_varargs_method("killLocalInstance", std::bind(&Global_dba::kill_local_instance, this, _1));
  add_varargs_method("stopLocalInstance", std::bind(&Global_dba::stop_local_instance, this, _1));

  add_method("dropCluster", std::bind(&Global_dba::drop_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("createCluster", std::bind(&Global_dba::create_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Global_dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("getCluster", std::bind(&Global_dba::get_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("validateInstance", std::bind(&Global_dba::validate_instance, this, _1), "data", shcore::Map, NULL);
}

shcore::Argument_list Global_dba::check_instance_op_params(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  shcore::Argument_list new_args;

  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string answer;
  bool proceed = true;
  std::string sandboxDir;

  int port = args.int_at(0);
  new_args.push_back(args[0]);

  if (args.size() == 2) {
    new_args.push_back(args[1]);
    options = args.map_at(1);
    // Verification of invalid attributes on the instance deployment data
    auto invalids = shcore::get_additional_keys(options, mysh::dba::Dba::_deploy_instance_opts);
    if (invalids.size()) {
      std::string error = "The following options are invalid: ";
      error += shcore::join_strings(invalids, ", ");
      throw shcore::Exception::argument_error(error);
    }
  } else {
    options.reset(new shcore::Value::Map_type());
    new_args.push_back(shcore::Value(options));
  }

  if (!options->has_key("sandboxDir")) {
    if (shcore::Shell_core_options::get()->has_key(SHCORE_SANDBOX_DIR)) {
      sandboxDir = (*shcore::Shell_core_options::get())[SHCORE_SANDBOX_DIR].as_string();
      (*options)["sandboxDir"] = shcore::Value(sandboxDir);
    }
  } else {
    sandboxDir = (*options)["sandboxDir"].as_string();

    // When the user specifies the sandbox dir we validate it
    if (!sandboxDir.empty() && !shcore::is_folder(sandboxDir))
      throw shcore::Exception::argument_error("The sandboxDir path '" + sandboxDir + "' is not valid");
  }

  return new_args;
}

shcore::Value Global_dba::deploy_local_instance(const shcore::Argument_list &args, const std::string& fname) {
  args.ensure_count(1, 2, get_function_name(fname).c_str());

  shcore::Argument_list valid_args;
  int port;
  bool deploying = (fname == "deployLocalInstance");
  bool cancelled = false;
  try {
    // Verifies and sets default args
    // After this there is port and options
    // options at least contains sandboxDir
    valid_args = check_instance_op_params(args);
    port = valid_args.int_at(0);

    bool prompt_password = false;
    std::string sandboxDir;

    auto options = valid_args.map_at(1);

    // Verification of required attributes on the instance deployment data

    auto missing = shcore::get_missing_keys(options, { "password|dbPassword" });

    prompt_password = !missing.empty();

    std::string message;
    if (prompt_password) {
      if (deploying) {
        message = "A new MySQL sandbox instance will be created on this host in \n"\
          "" + sandboxDir + "/" + std::to_string(port) + "\n\n"
          "Please enter a MySQL root password for the new instance: ";
      } else {
        message = "The MySQL sandbox instance on this host in \n"\
          "" + sandboxDir + "/" + std::to_string(port) + " will be started\n\n"
          "Please enter the MySQL root password of the instance: ";
      }

      std::string answer;
      if (password(message, answer)) {
        if (answer.empty())
          throw shcore::Exception::runtime_error("A password must be specified for the new instance.");
        else
          (*options)["password"] = shcore::Value(answer);
      } else
        cancelled = true;
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("deployLocalInstance"));

  shcore::Value ret_val;

  if (!cancelled) {
    if (deploying)
      println("Deploying new MySQL instance...");
    else
      println("Starting MySQL instance...");

    ret_val = _target->call(fname, valid_args);

    println();
    if (deploying)
      println("Instance localhost:" + std::to_string(port) + " successfully deployed and started.");
    else
      println("Instance localhost:" + std::to_string(port) + " successfully started.");

    println("Use '\\connect root@localhost:" + std::to_string(port) + "' to connect to the instance.");
  }

  return ret_val;
}

shcore::Value Global_dba::perform_instance_operation(const shcore::Argument_list &args, const std::string &fname, const std::string& progressive, const std::string& past) {
  shcore::Argument_list valid_args;
  args.ensure_count(1, 2, get_function_name(fname).c_str());

  try {
    valid_args = check_instance_op_params(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(fname));

  int port = valid_args.int_at(0);
  std::string sandboxDir = valid_args.map_at(1)->get_string("sandboxDir");

  std::string message = "The MySQL sandbox instance on this host in \n"\
    "" + sandboxDir + "/" + std::to_string(port) + " will be " + past + "\n";

  println(message);

  println(progressive + " MySQL instance...");

  shcore::Value ret_val = _target->call(fname, valid_args);

  println("Instance localhost:" + std::to_string(port) + " successfully " + past + ".\n");

  return ret_val;
}

shcore::Value Global_dba::delete_local_instance(const shcore::Argument_list &args) {
  return perform_instance_operation(args, "deleteLocalInstance", "Deleting", "deleted");
}

shcore::Value Global_dba::kill_local_instance(const shcore::Argument_list &args) {
  return perform_instance_operation(args, "killLocalInstance", "Killing", "killed");
}

shcore::Value Global_dba::stop_local_instance(const shcore::Argument_list &args) {
  return perform_instance_operation(args, "stopLocalInstance", "Stopping", "stopped");
}

shcore::Value Global_dba::drop_cluster(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  validate_session(get_function_name("dropCluster"));

  args.ensure_count(1, 2, get_function_name("dropCluster").c_str());

  try {
    std::string cluster_name = args.string_at(0);

    if (cluster_name.empty())
      throw shcore::Exception::argument_error("The Cluster name cannot be empty");

    shcore::Value::Map_type_ref options;
    bool valid_options = false;
    if (args.size() == 2) {
      options = args.map_at(1);
      valid_options = options->has_key("dropDefaultReplicaSet");
    }

    if (!valid_options) {
      std::string answer;
      println((boost::format("To remove the Cluster '%1%' the default replica set needs to be removed.") % cluster_name).str());
      if (prompt((boost::format("Do you want to remove the default replica set? [y/N]: ")).str().c_str(), answer)) {
        if (!answer.compare("y") || !answer.compare("Y")) {
          options.reset(new shcore::Value::Map_type);
          (*options)["dropDefaultReplicaSet"] = shcore::Value(true);

          valid_options = true;
        }
      }
    }

    if (valid_options) {
      shcore::Argument_list new_args;
      new_args.push_back(shcore::Value(cluster_name));
      new_args.push_back(shcore::Value(options));

      ret_val = _target->call("dropCluster", new_args);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropCluster"));

  return ret_val;
}

shcore::Value Global_dba::create_cluster(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  validate_session(get_function_name("createCluster"));

  args.ensure_count(1, 3, get_function_name("createCluster").c_str());

  try {
    std::string cluster_name = args.string_at(0);
    std::string answer, cluster_password;
    shcore::Value::Map_type_ref options;
    bool verbose = false; // Default is false
    bool multi_master = false;

    if (cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

    if (!shcore::is_valid_identifier(cluster_name))
      throw Exception::argument_error("The Cluster name must be a valid identifier.");

    if (args.size() > 1) {
      int opts_index = 1;
      if (args[1].type == shcore::String) {
        cluster_password = args.string_at(1);
        opts_index++;
      }

      if (args.size() > opts_index) {
        options = args.map_at(opts_index);
        // Check if some option is missing
        // TODO: Validate adminType parameter value

        if (options->has_key("verbose"))
          verbose = options->get_bool("verbose");

        if (options->has_key("multiMaster")) {
          multi_master = true;
        }
      }
    }

    auto dba = std::dynamic_pointer_cast<mysh::dba::Dba>(_target);
    auto session = dba->get_active_session();
    println("A new InnoDB cluster will be created on instance '" + session->uri() + "'.\n");

    if (multi_master) {
      println(
        "The MySQL InnoDB cluster is going to be setup in advanced Multi-Master Mode.\n"
        "Before continuing you have to confirm that you understand the requirements and\n"
        "limitations of Multi-Master Mode. Please read the manual before proceeding.\n"
        "\n");

      std::string r;
      println("I have read the MySQL InnoDB cluster manual and I understand the requirements\n"
              "and limitations of advanced Multi-Master Mode.");
      if (!(prompt("Confirm (Yes/No): ", r) && (r == "y" || r == "yes" || r == "Yes"))) {
        println("Cancelled");
        return shcore::Value();
      }
    }
    if (cluster_password.empty()) {
      println("When setting up a new InnoDB cluster it is required to define an administrative\n"
              "MASTER key for the cluster. This MASTER key needs to be re-entered when making\n"
              "changes to the cluster later on, e.g.adding new MySQL instances or configuring\n"
              "MySQL Routers. Losing this MASTER key will require the configuration of all\n"
              "InnoDB cluster entities to be changed.\n");

      if (!password("Please specify an administrative MASTER key for the cluster '"
          + cluster_name + "': ", cluster_password) || cluster_password.empty()) {
        println("Cancelled");
        return shcore::Value();
      }
    }
    {
      shcore::Argument_list new_args;
      new_args.push_back(shcore::Value(cluster_name));
      new_args.push_back(shcore::Value(cluster_password));

      // Update the cache as well
      set_cluster_admin_password(cluster_password);

      if (options != NULL)
        new_args.push_back(shcore::Value(options));

      println("Creating InnoDB cluster '" + cluster_name + "' on '" + session->uri() + "'...");

      // This is an instance of the API cluster
      auto raw_cluster = _target->call("createCluster", new_args);

      if (verbose)
        print("Adding Seed Instance...");

      // Returns an interactive wrapper of this instance
      Interactive_dba_cluster* cluster = new Interactive_dba_cluster(this->_shell_core);
      cluster->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(raw_cluster.as_object()));
      ret_val = shcore::Value::wrap<Interactive_dba_cluster>(cluster);

      println();

      std::string message = "Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.\n"\
                            "At least 3 instances are needed for the cluster to be able to withstand up to\n"\
                            "one server failure.";

      println(message);
    }
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createCluster"));

  return ret_val;
}

shcore::Value Global_dba::drop_metadata_schema(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  try {
    if (args.size() < 1) {
      std::string answer;

      if (prompt((boost::format("Are you sure you want to remove the Metadata? [y/N]: ")).str().c_str(), answer)) {
        if (!answer.compare("y") || !answer.compare("Y")) {
          shcore::Argument_list new_args;
          Value::Map_type_ref options(new shcore::Value::Map_type);

          (*options)["enforce"] = shcore::Value(true);
          new_args.push_back(shcore::Value(options));

          ret_val = _target->call("dropMetadataSchema", new_args);
        }
      }
    } else
      ret_val = _target->call("dropMetadataSchema", args);

    println("Metadata Schema successfully removed.");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))

  return ret_val;
}

shcore::Value Global_dba::get_cluster(const shcore::Argument_list &args) {
  validate_session(get_function_name("getCluster"));

  args.ensure_count(0, 2, get_function_name("getCluster").c_str());

  std::string master_key;
  shcore::Value::Map_type_ref options;

  shcore::Argument_list new_args(args);
  std::string cluster_name;
  bool get_default_cluster = false;
  bool cancelled = false;

  try {
    if (args.size()) {
      if (args.size() == 1) {
        if (args[0].type == shcore::String)
          cluster_name = args[0].as_string();
        else if (args[0].type == shcore::Map) {
          options = args[0].as_map();
          get_default_cluster = true;
        } else
          throw shcore::Exception::argument_error("Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options");
      } else {
        cluster_name = args.string_at(0);
        options = args.map_at(1);
      }
    } else
      get_default_cluster = true;

    if (!options) {
      options.reset(new shcore::Value::Map_type());
      new_args.push_back(shcore::Value(options));
    }

    if (!get_default_cluster && cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

    if (options->has_key("masterKey"))
      master_key = options->get_string("masterKey");

    std::string message = "When the InnoDB cluster was setup, a MASTER key was defined in order to enable\n"\
      "performing administrative tasks on the cluster.\n";

    println(message);

    if (master_key.empty()) {
      if (get_default_cluster)
        message = "Please specify the administrative MASTER key for the default cluster: ";
      else
        message = "Please specify the administrative MASTER key for the cluster '" + cluster_name + "': ";

      std::string answer;
      if (password(message, answer)) {
        if (answer.empty())
          throw shcore::Exception::runtime_error("Missing the administrative MASTER key for the cluster.");
        else
          (*options)["masterKey"] = shcore::Value(answer);
      } else
        cancelled = true;
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getCluster"));

  Value ret_val;
  if (!cancelled) {
    ScopedStyle ss(_target.get(), naming_style);
    Value raw_cluster = _target->call("getCluster", new_args);

    Interactive_dba_cluster* cluster = new Interactive_dba_cluster(this->_shell_core);
    cluster->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(raw_cluster.as_object()));
    ret_val = shcore::Value::wrap<Interactive_dba_cluster>(cluster);
  }

  return ret_val;
}

shcore::Value Global_dba::validate_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  shcore::Argument_list new_args;

  validate_session(get_function_name("validateInstance"));

  args.ensure_count(1, get_function_name("validateInstance").c_str());

  std::string uri, answer, user;
  shcore::Value::Map_type_ref options; // Map with the connection data

  // Identify the type of connection data (String or Document)
  if (args[0].type == shcore::String) {
    uri = args.string_at(0);
    options = shcore::get_connection_data(uri, false);
  }
  // Connection data comes in a dictionary
  else if (args[0].type == shcore::Map)
    options = args.map_at(0);

  // Verification of required attributes on the connection data
  auto missing = shcore::get_missing_keys(options, { "host", "port" });

  if (missing.size()) {
    std::string error = "Missing instance options: ";
    error += shcore::join_strings(missing, ", ");
    throw shcore::Exception::argument_error(error);
  }

  // Verification of the password
  std::string user_password;
  bool has_password = true;

  if (options->has_key("password"))
    user_password = options->get_string("password");
  else if (options->has_key("dbPassword"))
    user_password = options->get_string("dbPassword");
  else if (args.size() == 2 && args[1].type == shcore::String) {
    user_password = args.string_at(1);
    (*options)["dbPassword"] = shcore::Value(user_password);
  } else
    has_password = false;

  if (!has_password) {
    if (password("Please provide a password for '" + build_connection_string(options, false) + "': ", answer))
      (*options)["dbPassword"] = shcore::Value(answer);
  }

  new_args.push_back(shcore::Value(options));

  // Let's get the user to know we're starting to validate the instance
  println("Validating instance...");

  ret_val = _target->call("validateInstance", new_args);

  return ret_val;
}

void Global_dba::validate_session(const std::string &source) const {
  auto dba = std::dynamic_pointer_cast<mysh::dba::Dba>(_target);
  dba->validate_session(source);
}
