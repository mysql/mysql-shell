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
#include "interactive/interactive_dba_cluster.h"
#include "modules/adminapi/mod_dba_instance.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/mysqlxtest_utils.h"
#include "modules/adminapi/mod_dba.h"
#include "utils/utils_general.h"
#include "utils/utils_file.h"
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace shcore;

void Global_dba::init() {
  add_varargs_method("deploySandboxInstance", std::bind(&Global_dba::deploy_sandbox_instance, this, _1, "deploySandboxInstance"));
  add_varargs_method("startSandboxInstance", std::bind(&Global_dba::start_sandbox_instance, this, _1));
  add_varargs_method("deleteSandboxInstance", std::bind(&Global_dba::delete_sandbox_instance, this, _1));
  add_varargs_method("killSandboxInstance", std::bind(&Global_dba::kill_sandbox_instance, this, _1));
  add_varargs_method("stopSandboxInstance", std::bind(&Global_dba::stop_sandbox_instance, this, _1));

  add_method("createCluster", std::bind(&Global_dba::create_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Global_dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("getCluster", std::bind(&Global_dba::get_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("validateInstance", std::bind(&Global_dba::validate_instance, this, _1), "data", shcore::Map, NULL);
  add_method("prepareInstance", std::bind(&Global_dba::prepare_instance, this, _1), "data", shcore::Map, NULL);
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
    
    shcore::Argument_map opt_map (*options);
    
    opt_map.ensure_keys({}, mysh::dba::Dba::_deploy_instance_opts, "the instance definition");
    
    if (opt_map.has_key("sandboxDir"))
      sandboxDir = opt_map.string_at("sandboxDir");
    
  } else {
    options.reset(new shcore::Value::Map_type());
    new_args.push_back(shcore::Value(options));
  }

  // Gets the global sandboxDir if not set by the user
  if (sandboxDir.empty()) {
    if (shcore::Shell_core_options::get()->has_key(SHCORE_SANDBOX_DIR)) {
      sandboxDir = (*shcore::Shell_core_options::get())[SHCORE_SANDBOX_DIR].as_string();
      (*options)["sandboxDir"] = shcore::Value(sandboxDir);
    }
  }

  // When the user specifies the sandbox dir we validate it
  if (!sandboxDir.empty() && !shcore::is_folder(sandboxDir))
    throw shcore::Exception::argument_error("The sandboxDir path '" + sandboxDir + "' is not valid");

  return new_args;
}

shcore::Value Global_dba::deploy_sandbox_instance(const shcore::Argument_list &args, const std::string& fname) {
  args.ensure_count(1, 2, get_function_name(fname).c_str());

  shcore::Argument_list valid_args;
  int port;
  bool deploying = (fname == "deploySandboxInstance");
  bool cancelled = false;
  try {
    // Verifies and sets default args
    // After this there is port and options
    // options at least contains sandboxDir
    valid_args = check_instance_op_params(args);
    port = valid_args.int_at(0);

    std::string sandbox_dir;

    auto options = valid_args.map_at(1);

    if (options && options->has_key("sandboxDir")) {
      sandbox_dir = options->get_string("sandboxDir");
    } else {
      // we get the default value
      sandbox_dir = (*shcore::Shell_core_options::get())[SHCORE_SANDBOX_DIR].as_string();
    }

    bool prompt_password = !options->has_key("password") && !options->has_key("dbPassword");

    std::string message;
    if (prompt_password) {
      if (deploying) {
        message = "A new MySQL sandbox instance will be created on this host in \n"\
          "" + sandbox_dir + "/" + std::to_string(port) + "\n\n"
          "Please enter a MySQL root password for the new instance: ";
      } else {
        message = "The MySQL sandbox instance on this host in \n"\
          "" + sandbox_dir + "/" + std::to_string(port) + " will be started\n\n"
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
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(fname));

  shcore::Value ret_val;

  if (!cancelled) {
    if (deploying)
      println("Deploying new MySQL instance...");
    else
      println("Starting MySQL instance...");

    ret_val = call_target(fname, valid_args);

    println();
    if (deploying)
      println("Instance localhost:" + std::to_string(port) + " successfully deployed and started.");
    else
      println("Instance localhost:" + std::to_string(port) + " successfully started.");

    println("Use '\\connect root@localhost:" + std::to_string(port) + "' to connect to the instance.");
    println();
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
    "" + sandboxDir + "/" + std::to_string(port) + " will be " + past;

  println(message);
  println();

  println(progressive + " MySQL instance...");

  shcore::Value ret_val = call_target(fname, valid_args);

  println();
  println("Instance localhost:" + std::to_string(port) + " successfully " + past + ".");
  println();

  return ret_val;
}

shcore::Value Global_dba::delete_sandbox_instance(const shcore::Argument_list &args) {
  return perform_instance_operation(args, "deleteSandboxInstance", "Deleting", "deleted");
}

shcore::Value Global_dba::kill_sandbox_instance(const shcore::Argument_list &args) {
  return perform_instance_operation(args, "killSandboxInstance", "Killing", "killed");
}

shcore::Value Global_dba::stop_sandbox_instance(const shcore::Argument_list &args) {
  return perform_instance_operation(args, "stopSandboxInstance", "Stopping", "stopped");
}

shcore::Value Global_dba::start_sandbox_instance(const shcore::Argument_list &args) {
  return perform_instance_operation(args, "startSandboxInstance", "Starting", "started");
}

shcore::Value Global_dba::create_cluster(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  validate_session(get_function_name("createCluster"));

  args.ensure_count(1, 2, get_function_name("createCluster").c_str());

  shcore::Value::Map_type_ref options;
  
  std::string cluster_name;
  std::shared_ptr<mysh::ShellDevelopmentSession> session;
  
  try {
    cluster_name = args.string_at(0);
    std::string answer;
    bool multi_master = false;

    if (cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

    if (!shcore::is_valid_identifier(cluster_name))
      throw Exception::argument_error("The Cluster name must be a valid identifier.");

    if (args.size() > 1) {
      options = args.map_at(1);
      if (options->has_key("multiMaster")) {
        multi_master = true;
      }
    }

    auto dba = std::dynamic_pointer_cast<mysh::dba::Dba>(_target);
    session = dba->get_active_session();
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
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createCluster"));
  
  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(cluster_name));

  if (options != NULL)
    new_args.push_back(shcore::Value(options));

  println("Creating InnoDB cluster '" + cluster_name + "' on '" + session->uri() + "'...");

  // This is an instance of the API cluster
  auto raw_cluster = call_target("createCluster", new_args);

  print("Adding Seed Instance...");
  println();

  // Returns an interactive wrapper of this instance
  Interactive_dba_cluster* cluster = new Interactive_dba_cluster(this->_shell_core);
  cluster->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(raw_cluster.as_object()));
  ret_val = shcore::Value::wrap<Interactive_dba_cluster>(cluster);

  println();

  std::string message = "Cluster successfully created. Use Cluster." + get_member_name("addInstance", naming_style) + "() to add MySQL instances.\n"\
                        "At least 3 instances are needed for the cluster to be able to withstand up to\n"\
                        "one server failure.";

  println(message);
  println();

  return ret_val;
}

shcore::Value Global_dba::drop_metadata_schema(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  if (args.size() < 1) {
    std::string answer;

    if (prompt((boost::format("Are you sure you want to remove the Metadata? [y/N]: ")).str().c_str(), answer)) {
      if (!answer.compare("y") || !answer.compare("Y")) {
        shcore::Argument_list new_args;
        Value::Map_type_ref options(new shcore::Value::Map_type);

        (*options)["enforce"] = shcore::Value(true);
        new_args.push_back(shcore::Value(options));

        ret_val = call_target("dropMetadataSchema", new_args);
      }
    }
  } else {
    ret_val = call_target("dropMetadataSchema", args);
  }

  println("Metadata Schema successfully removed.");
  println();

  return ret_val;
}

shcore::Value Global_dba::get_cluster(const shcore::Argument_list &args) {
  validate_session(get_function_name("getCluster"));

  Value raw_cluster = call_target("getCluster", args);

  Interactive_dba_cluster* cluster = new Interactive_dba_cluster(this->_shell_core);
  cluster->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(raw_cluster.as_object()));
  return shcore::Value::wrap<Interactive_dba_cluster>(cluster);
}

shcore::Value Global_dba::validate_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  shcore::Argument_list new_args;

  args.ensure_count(1, 2, get_function_name("validateInstance").c_str());

  std::string uri, answer, user;

  auto options = mysh::dba::get_instance_options_map(args);
  
  shcore::Argument_map opt_map(*options);
  opt_map.ensure_keys({"host", "port"}, {}, "instance definition");
  
  // Sets root user by default if no specified
  if (!options->has_key("user") && !options->has_key("dbUser"))
    (*options)["user"] = shcore::Value("root");

  // Verification of the password
  std::string user_password;
  bool has_password = true;

  if (options->has_key("password"))
    user_password = options->get_string("password");
  else if (options->has_key("dbPassword"))
    user_password = options->get_string("dbPassword");
  else
    has_password = false;

  if (!has_password) {
    if (password("Please provide a password for '" + build_connection_string(options, false) + "': ", answer))
      (*options)["dbPassword"] = shcore::Value(answer);
  }

  new_args.push_back(shcore::Value(options));

  // Let's get the user to know we're starting to validate the instance
  println("Validating instance...");
  println();

  ret_val = call_target("validateInstance", new_args);

  return ret_val;
}

shcore::Value Global_dba::prepare_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  shcore::Argument_list new_args;

  args.ensure_count(1, 2, get_function_name("prepareInstance").c_str());

  std::string uri, answer, user;
  
  auto options = mysh::dba::get_instance_options_map(args);
  
  shcore::Argument_map opt_map(*options);
  opt_map.ensure_keys({"host", "port"}, {}, "instance definition");
  
  // Sets root user by default if no specified
  if (!options->has_key("user") && !options->has_key("dbUser"))
    (*options)["user"] = shcore::Value("root");

  // Verification of the password
  std::string user_password;
  bool has_password = true;

  if (options->has_key("password"))
    user_password = options->get_string("password");
  else if (options->has_key("dbPassword"))
    user_password = options->get_string("dbPassword");
  else
    has_password = false;

  if (!has_password) {
    if (password("Please provide a password for '" + build_connection_string(options, false) + "': ", answer))
      (*options)["dbPassword"] = shcore::Value(answer);
  }

  new_args.push_back(shcore::Value(options));

  // Let's get the user to know we're starting to prepare the instance
  println("Preparing instance...");
  println();

  ret_val = call_target("prepareInstance", new_args);

  println("Instance successfully prepared. You can now add it to the InnoDB cluster with the cluster."+ get_member_name("addInstance", naming_style) +"() function.");
  println();

  return ret_val;
}

void Global_dba::validate_session(const std::string &source) const {
  auto dba = std::dynamic_pointer_cast<mysh::dba::Dba>(_target);
  dba->validate_session(source);
}
