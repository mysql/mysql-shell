/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "interactive/interactive_global_dba.h"
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "modules/adminapi/mod_dba_common.h"
#include "interactive/interactive_dba_cluster.h"
//#include "modules/adminapi/mod_dba_instance.h"
#include "modules/mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include "utils/utils_file.h"
#include "utils/utils_help.h"
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace shcore;

void Global_dba::init() {
  add_varargs_method("deploySandboxInstance", std::bind(&Global_dba::deploy_sandbox_instance, this, _1, "deploySandboxInstance"));
  add_varargs_method("startSandboxInstance", std::bind(&Global_dba::start_sandbox_instance, this, _1));
  add_varargs_method("deleteSandboxInstance", std::bind(&Global_dba::delete_sandbox_instance, this, _1));
  add_varargs_method("killSandboxInstance", std::bind(&Global_dba::kill_sandbox_instance, this, _1));
  add_varargs_method("stopSandboxInstance", std::bind(&Global_dba::stop_sandbox_instance, this, _1));
  add_varargs_method("getCluster", std::bind(&Global_dba::get_cluster, this, _1));
  add_varargs_method("rebootClusterFromCompleteOutage", std::bind(&Global_dba::reboot_cluster_from_complete_outage, this, _1));

  add_method("createCluster", std::bind(&Global_dba::create_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Global_dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("checkInstanceConfiguration", std::bind(&Global_dba::check_instance_configuration, this, _1), "data", shcore::Map, NULL);
  add_method("configureLocalInstance", std::bind(&Global_dba::configure_local_instance, this, _1), "data", shcore::Map, NULL);
}

mysqlsh::dba::ReplicationGroupState Global_dba::check_preconditions(const std::string& function_name) const {
  ScopedStyle ss(_target.get(), naming_style);
  auto dba = std::dynamic_pointer_cast<mysqlsh::dba::Dba>(_target);
  return dba->check_preconditions(function_name);
}

std::vector<std::pair<std::string, std::string>> Global_dba::get_replicaset_instances_status(std::string *out_cluster_name,
          const shcore::Value::Map_type_ref &options) const {
  ScopedStyle ss(_target.get(), naming_style);
  auto dba = std::dynamic_pointer_cast<mysqlsh::dba::Dba>(_target);
  return dba->get_replicaset_instances_status(out_cluster_name, options);
}

void Global_dba::validate_instances_status_reboot_cluster(const shcore::Argument_list &args) const {
  ScopedStyle ss(_target.get(), naming_style);
  auto dba = std::dynamic_pointer_cast<mysqlsh::dba::Dba>(_target);
  return dba->validate_instances_status_reboot_cluster(args);
}

shcore::Argument_list Global_dba::check_instance_op_params(const shcore::Argument_list &args,
                                                           const std::string& function_name) {
  shcore::Value ret_val;
  shcore::Argument_list new_args;

  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string answer;
  bool proceed = true;
  // Initialize sandboxDir with the default sandboxValue
  std::string sandboxDir = (*shcore::Shell_core_options::get())[SHCORE_SANDBOX_DIR].as_string();

  int port = args.int_at(0);
  new_args.push_back(args[0]);

  if (args.size() == 2) {
    new_args.push_back(args[1]);
    options = args.map_at(1);

    shcore::Argument_map opt_map(*options);

    if (function_name == "deploySandboxInstance") {
      opt_map.ensure_keys({}, mysqlsh::dba::Dba::_deploy_instance_opts, "the instance definition");
    } else if (function_name == "stopSandboxInstance"){
      opt_map.ensure_keys({}, mysqlsh::dba::Dba::_stop_instance_opts, "the instance definition");
    }
    else{
      opt_map.ensure_keys({}, mysqlsh::dba::Dba::_default_local_instance_opts, "the instance definition");
    }

    if (opt_map.has_key("sandboxDir")) {
      sandboxDir = opt_map.string_at("sandboxDir");
      // When the user specifies the sandbox dir we validate it
      if (!shcore::is_folder(sandboxDir))
        throw shcore::Exception::argument_error("The sandboxDir path '" + sandboxDir + "' is not valid");
    }
    // Store sandboxDir value
    (*options)["sandboxDir"] = shcore::Value(sandboxDir);
  } else {
    options.reset(new shcore::Value::Map_type());
    (*options)["sandboxDir"] = shcore::Value(sandboxDir);
    new_args.push_back(shcore::Value(options));
  }
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
    valid_args = check_instance_op_params(args, fname);
    port = valid_args.int_at(0);
    auto options = valid_args.map_at(1);

    std::string sandbox_dir {options->get_string("sandboxDir")};

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
      if (password(message, answer))
          (*options)["password"] = shcore::Value(answer);
      else
        cancelled = true;
    }
    if (!options->has_key("allowRootFrom")) {
      // If the user didn't specify the option allowRootFrom we automatically use '%'
      std::string pattern = "%";
      (*options)["allowRootFrom"] = shcore::Value(pattern);
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

    println("Use shell.connect('root@localhost:" + std::to_string(port) + "'); to connect to the instance.");
    println();
  }

  return ret_val;
}

shcore::Value Global_dba::perform_instance_operation(const shcore::Argument_list &args, const std::string &fname, const std::string& progressive, const std::string& past) {
  shcore::Argument_list valid_args;
  args.ensure_count(1, 2, get_function_name(fname).c_str());

  try {
    valid_args = check_instance_op_params(args, fname);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(fname));

  int port = valid_args.int_at(0);
  auto options = valid_args.map_at(1);

  std::string sandboxDir {options->get_string("sandboxDir")};
  std::string message = "The MySQL sandbox instance on this host in \n"\
    "" + sandboxDir + "/" + std::to_string(port) + " will be " + past + "\n";

  println(message);

  if (fname == "stopSandboxInstance"){
    bool prompt_password = !options->has_key("password") && !options->has_key("dbPassword");

    if (prompt_password) {
      std::string message = "Please enter the MySQL root password for the instance 'localhost:" + std::to_string(port) + "': ";

      std::string answer;
      if (password(message, answer))
        (*options)["password"] = shcore::Value(answer);
    }
  }

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
  args.ensure_count(1, 2, get_function_name("createCluster").c_str());

  mysqlsh::dba::ReplicationGroupState state;

  try {
    state = check_preconditions("createCluster");
  } catch (shcore::Exception &e) {
    std::string error(e.what());
    if (error.find("already in an InnoDB cluster") != std::string::npos) {
      /*
      * For V1.0 we only support one single Cluster. That one shall be the default Cluster.
      * We must check if there's already a Default Cluster assigned, and if so thrown an exception.
      * And we must check if there's already one Cluster on the MD and if so assign it to Default
      */
      std::string nice_error = get_function_name("createCluster") + ": Cluster is already initialized. "\
        "Use " + get_function_name("getCluster") + "() to access it";
      throw Exception::runtime_error(nice_error);
    } else throw;
  }

  shcore::Value ret_val;
  shcore::Value::Map_type_ref options;
  std::string cluster_name;
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> session;

  try {
    cluster_name = args.string_at(0);
    std::string answer;
    bool multi_master = false;
    bool force = false;
    bool adopt_from_gr = false;

    if (cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

    if (!shcore::is_valid_identifier(cluster_name))
      throw Exception::argument_error("The Cluster name must be a valid identifier.");

    if (args.size() > 1) {
      // Map with the options
      options = args.map_at(1);

      // Verification of invalid attributes on the instance creation options
      shcore::Argument_map opt_map(*options);

      opt_map.ensure_keys({}, mysqlsh::dba::Dba::_create_cluster_opts,
                          "the options");

      // Validate SSL options for the cluster instance
      mysqlsh::dba::validate_ssl_instance_options(options);

      if (opt_map.has_key("multiMaster"))
        multi_master = opt_map.bool_at("multiMaster");

      if (opt_map.has_key("force"))
        force = opt_map.bool_at("force");

      if (opt_map.has_key("adoptFromGR"))
        adopt_from_gr = opt_map.bool_at("adoptFromGR");
    } else {
      options.reset(new shcore::Value::Map_type());
    }

    if (state.source_type == mysqlsh::dba::GRInstanceType::GroupReplication && !adopt_from_gr) {
      if (prompt("You are connected to an instance that belongs to an unmanaged replication group.\n"\
        "Do you want to setup an InnoDB cluster based on this replication group?", answer) && (answer == "y" || answer == "yes" || answer == "Yes"))
        (*options)["adoptFromGR"] = shcore::Value(true);
      else
        throw Exception::argument_error("Creating a cluster on an unmanaged replication group requires adoptFromGR option to be true");
    }

    auto dba = std::dynamic_pointer_cast<mysqlsh::dba::Dba>(_target);
    session = dba->get_active_session();
    println("A new InnoDB cluster will be created on instance '" + session->uri() + "'.\n");

    if (multi_master && !force) {
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
      } else {
        (*options)["force"] = shcore::Value(true);
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
  args.ensure_count(0, 1, get_function_name("dropMetadataSchema").c_str());

  check_preconditions("dropMetadataSchema");

  shcore::Value ret_val;
  shcore::Argument_list new_args;
  bool force = false;

  if (args.size() < 1) {
    std::string answer;

    if (prompt((boost::format("Are you sure you want to remove the Metadata? [y/N]: ")).str().c_str(), answer)) {
      if (!answer.compare("y") || !answer.compare("Y")) {
        Value::Map_type_ref options(new shcore::Value::Map_type);

        (*options)["force"] = shcore::Value(true);
        new_args.push_back(shcore::Value(options));
        force = true;
      }
    }
  } else {
    try {
      auto options = args.map_at(0);
      shcore::Argument_map opt_map(*options);
      opt_map.ensure_keys({}, {"force"}, "drop options");
      force = opt_map.bool_at("force");
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))
  }

  if (force) {
    ret_val = call_target("dropMetadataSchema", (args.size() < 1) ? new_args : args);
    println("Metadata Schema successfully removed.");
  } else {
    println("No changes made to the Metadata Schema.");
  }
  println();

  return ret_val;
}

shcore::Value Global_dba::get_cluster(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("getCluster").c_str());

  auto state = check_preconditions("getCluster");

  if (state.source_state == mysqlsh::dba::ManagedInstance::OnlineRO) {
    println("WARNING: The session is on a " + mysqlsh::dba::ManagedInstance::describe(static_cast<mysqlsh::dba::ManagedInstance::State>(state.source_state)) + " instance.\n"\
              "         Write operations on the InnoDB cluster will not be allowed\n");
  } else if (state.source_state != mysqlsh::dba::ManagedInstance::OnlineRW)
    println("WARNING: The session is on a " + mysqlsh::dba::ManagedInstance::describe(static_cast<mysqlsh::dba::ManagedInstance::State>(state.source_state)) + " instance.\n"\
      "         Write operations in the InnoDB cluster will not be allowed.\n"\
      "         The information retrieved with describe() and status() may be outdated.\n");

  Value raw_cluster = call_target("getCluster", args);

  Interactive_dba_cluster* cluster = new Interactive_dba_cluster(this->_shell_core);
  cluster->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(raw_cluster.as_object()));
  return shcore::Value::wrap<Interactive_dba_cluster>(cluster);
}

shcore::Value Global_dba::reboot_cluster_from_complete_outage(const shcore::Argument_list &args) {
  args.ensure_count(0, 2, get_function_name("rebootClusterFromCompleteOutage").c_str());

  shcore::Value ret_val;
  std::string cluster_name;
  shcore::Value::Map_type_ref options;
  bool confirm_rescan_rejoins = true, confirm_rescan_removes = true;
  shcore::Value::Array_type_ref confirmed_rescan_removes(new shcore::Value::Array_type());
  shcore::Value::Array_type_ref confirmed_rescan_rejoins(new shcore::Value::Array_type());
  Value::Array_type_ref remove_instances_ref, rejoin_instances_ref;
  shcore::Argument_list new_args;
  bool default_cluster = false;
  shcore::Argument_map opt_map;

  check_preconditions("rebootClusterFromCompleteOutage");

  try {
    if (args.size() == 0) {
      default_cluster = true;
    } else if (args.size() == 1) {
      cluster_name = args.string_at(0);
    } else {
      cluster_name = args.string_at(0);
      options = args.map_at(1);
    }

    if (options) {
      opt_map = *options;

      if (opt_map.has_key("removeInstances"))
        confirm_rescan_removes = false;

      if (opt_map.has_key("rejoinInstances"))
        confirm_rescan_rejoins = false;
    }

    // Verify the status of the instances
    validate_instances_status_reboot_cluster(args);

    // Get the all the instances and their status
    std::vector<std::pair<std::string, std::string>> instances_status
        = get_replicaset_instances_status(&cluster_name, options);

    if (confirm_rescan_rejoins) {
      for (auto &value : instances_status) {
        std::string instance_address = value.first;
        std::string instance_status = value.second;

        // if the status is not empty it means the connection failed
        // so we skip this instance
        if (!instance_status.empty()) {
          std::string msg = "The instance '" + instance_address + "' is not "
                            "reachable: '" + instance_status + "'. Skipping rejoin to the Cluster.";
          log_warning("%s", msg.c_str());
          continue;
        }

        println();
        println("The instance '" + instance_address + "' was part of the cluster configuration.");

        std::string answer;
        if (prompt("Would you like to rejoin it to the cluster? [Y|n]: ", answer)) {
          if (!answer.compare("y") || !answer.compare("Y") || answer.empty())
            confirmed_rescan_rejoins->push_back(shcore::Value(instance_address));
        }
      }
    } else {
      // Validate the rejoinInstance list
      rejoin_instances_ref = opt_map.array_at("rejoinInstances");

      for (auto value : *rejoin_instances_ref.get()) {
        std::string instance = value.as_string();

        auto it = std::find_if(instances_status.begin(), instances_status.end(),
                  [&instance](const std::pair<std::string, std::string> &p)
                  { return p.first == instance; });

        if (it != instances_status.end()) {
          if (!(it->second.empty())) {
            throw Exception::runtime_error("The instance '" + instance + "' is not reachable");
          }
        } else {
          throw Exception::runtime_error("The instance '" + instance + "' does not "
                                         "belong to the cluster.");
        }
      }
    }

    if (confirm_rescan_removes) {
      for (auto &value : instances_status) {
        std::string instance_address = value.first;
        std::string instance_status = value.second;

        // if the status is empty it means the connection succeeded
        // so we skip this instance
        if (instance_status.empty())
          continue;

        println();
        println("Could not open a connection to '" + instance_address + "': '" + instance_status + "'");

        std::string answer;
        if (prompt("Would you like to remove it from the cluster's metadata? [Y|n]: ", answer)) {
          if (!answer.compare("y") || !answer.compare("Y") || answer.empty())
            confirmed_rescan_removes->push_back(shcore::Value(instance_address));
        }
      }
    } else {
      // Validate the removeInstances list
      remove_instances_ref = opt_map.array_at("removeInstances");

      for (auto value : *remove_instances_ref.get()) {
        std::string instance = value.as_string();

        auto it = std::find_if(instances_status.begin(), instances_status.end(),
                  [&instance](const std::pair<std::string, std::string> &p)
                  { return p.first == instance; });

        if (it == instances_status.end()) {
          throw Exception::runtime_error("The instance '" + instance + "' does not "
                                         "belong to the cluster.");
        }
      }
    }

    println();

    if (default_cluster)
      println("Reconfiguring the default cluster from complete outage...");
    else
      println("Reconfiguring the cluster '" + cluster_name + "' from complete outage...");

    if (!confirmed_rescan_rejoins->empty() || !confirmed_rescan_removes->empty()) {
      shcore::Argument_list new_args;
      Value::Map_type_ref options(new shcore::Value::Map_type);

      if (!confirmed_rescan_rejoins->empty())
        (*options)["rejoinInstances"] = shcore::Value(confirmed_rescan_rejoins);

      if (!confirmed_rescan_removes->empty())
        (*options)["removeInstances"] = shcore::Value(confirmed_rescan_removes);

      // Check if the user provided any option
      if (!confirm_rescan_removes)
        (*options)["removeInstances"] = shcore::Value(opt_map.array_at("removeInstances"));

      if (!confirm_rescan_rejoins)
        (*options)["rejoinInstances"] = shcore::Value(opt_map.array_at("rejoinInstances"));

      new_args.push_back(shcore::Value(cluster_name));
      new_args.push_back(shcore::Value(options));
      ret_val = call_target("rebootClusterFromCompleteOutage", new_args);
    } else {
      ret_val = call_target("rebootClusterFromCompleteOutage", args);
    }

    println();
    println("The cluster was successfully rebooted.");
    println();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rebootClusterFromCompleteOutage"));

  return ret_val;
}

void Global_dba::print_validation_results(const shcore::Value::Map_type_ref& result) {
  println();
  println("The following issues were encountered:");
  println();

  auto errors = result->get_array("errors");
  for (auto error : *errors)
    println(" - " + error.as_string());

  bool restart_required = result->get_bool("restart_required");

  if (result->has_key("config_errors")) {
    println(" - Some configuration options need to be fixed.");
    println();

    auto config_errors = result->get_array("config_errors");

    for (auto option : *config_errors) {
      auto opt_map = option.as_map();
      std::string action = opt_map->get_string("action");
      std::string note;

      if (action == "server_update+config_update") {
        if (restart_required)
          note = "Update the config file and update or restart the server variable";
        else
          note = "Update the server variable and the config file";
      } else if (action == "config_update+restart")
        note = "Update the config file and restart the server";
      else if (action == "config_update")
        note = "Update the config file";
      else if (action == "server_update") {
        if (restart_required)
          note = "Update the server variable or restart the server";
        else
          note = "Update the server variable";
      } else if (action == "restart")
        note = "Restart the server";

      (*opt_map)["note"] = shcore::Value(note);
    }

    dump_table({"option", "current", "required", "note"}, {"Variable", "Current Value", "Required Value", "Note"}, config_errors);

    for (auto option : *config_errors) {
      auto opt_map = option.as_map();
      opt_map->erase("note");
    }
  }
}

shcore::Value Global_dba::check_instance_configuration(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();

  args.ensure_count(1, 2, get_function_name("checkInstanceConfiguration").c_str());

  std::string uri, user;

  auto instance_def = mysqlsh::dba::get_instance_options_map(args, mysqlsh::dba::PasswordFormat::OPTIONS);

  shcore::Argument_map opt_map(*instance_def);

  opt_map.ensure_keys({"host", "port"}, mysqlsh::dba::_instance_options, "instance definition");

  if (args.size() == 2) {
    shcore::Argument_map extra_opts(*args.map_at(1));
    extra_opts.ensure_keys({}, {"password", "dbPassword", "mycnfPath"}, "validation options");
  }

  // Gather username and password if missing
  mysqlsh::dba::resolve_instance_credentials(instance_def, _delegate);

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_def));

  if (args.size() == 2)
    new_args.push_back(args[1]);

  // Let's get the user to know we're starting to validate the instance
  println("Validating instance...");
  println();

  ret_val = call_target("checkInstanceConfiguration", new_args);

  if (format.find("json") != std::string::npos)
    print_value(ret_val, "");
  else {
    auto result = ret_val.as_map();

    if (result->get_string("status") == "ok")
      println("The instance '" + opt_map.string_at("host") + ":" + std::to_string(opt_map.int_at("port")) + "' is valid for Cluster usage");
    else {
      println("The instance '" + opt_map.string_at("host") + ":" + std::to_string(opt_map.int_at("port")) + "' is not valid for Cluster usage.");

      print_validation_results(result);

      std::string closing_error = "Please fix these issues ";
      if (result->get_bool("restart_required"))
        closing_error += ", restart the server";

      closing_error += "and try again.";

      println();
      println(closing_error);
      println();
    }
  }

  return ret_val;
}

bool Global_dba::resolve_cnf_path(const shcore::Argument_list& connection_args, const shcore::Value::Map_type_ref& extra_options) {
  // Path is not given, let's try to autodetect it
  int port = 0;
  std::string datadir;
  auto session = mysqlsh::dba::Dba::get_session(connection_args);
  mysqlsh::dba::get_port_and_datadir(session->connection(), port, datadir);

  std::string path_separator = datadir.substr(datadir.size() - 1);
  auto path_elements = shcore::split_string(datadir, path_separator);

  // Removes the empty element at the end
  path_elements.pop_back();

  std::string tmpPath;
  std::string cnfPath;

  // Sandbox deployment structure would be:
  // - <root_path>/<port>/sandboxdata
  // - <root_path>/<port>/my.cnf
  // So we validate such structure to determine it is a sandbox
  if (path_elements[path_elements.size() - 2] == std::to_string(port)) {
    path_elements[path_elements.size() - 1] = "my.cnf";

    tmpPath = shcore::join_strings(path_elements, path_separator);
    if (shcore::file_exists(tmpPath)) {
      println();
      println("Detected as sandbox instance.");
      println();
      println("Validating MySQL configuration file at: " + tmpPath);
      cnfPath = tmpPath;
    }
  }

  if (cnfPath.empty()) {
    bool done = false;
    while (!done && prompt("Please specify the path to the MySQL configuration file: ", tmpPath)) {
      if (tmpPath.empty())
        done = true;
      else {
        if (shcore::file_exists(tmpPath)) {
          cnfPath = tmpPath;
          done = true;
        } else {
          println("The given path to the MySQL configuration file is invalid.");
          println();
        }
      }
    }
  }

  if (!cnfPath.empty())
    (*extra_options)["mycnfPath"] = shcore::Value(cnfPath);

  return !cnfPath.empty();
}

shcore::Value Global_dba::configure_local_instance(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("configureLocalInstance").c_str());

  std::string uri, answer, user;
  shcore::Value::Map_type_ref instance_def;
  shcore::Argument_list target_args;

  try {
    instance_def = mysqlsh::dba::get_instance_options_map(args, mysqlsh::dba::PasswordFormat::OPTIONS);
    shcore::Argument_map opt_map(*instance_def);
    opt_map.ensure_keys({"host", "port"}, mysqlsh::dba::_instance_options, "instance definition");

    if (!shcore::is_local_host(opt_map.string_at("host"), true))
      throw shcore::Exception::runtime_error("This function only works with local instances");

    // Gather username and password if missing
    mysqlsh::dba::resolve_instance_credentials(instance_def, _delegate);

    shcore::Value::Map_type_ref extra_options;
    if (args.size() == 2) {
      extra_options = args.map_at(1);
      shcore::Argument_map extra_opts(*extra_options);
      extra_opts.ensure_keys({}, {"password", "dbPassword", "mycnfPath"}, "validation options");
    } else {
      extra_options.reset(new shcore::Value::Map_type());
    }

    // Target args contain the args received on the interactive layer
    // including the resolved user and password
    target_args.push_back(shcore::Value(instance_def));
    target_args.push_back(shcore::Value(extra_options));

    // Session args holds all the connection data, including the resolved user/password
    shcore::Argument_list session_args;
    session_args.push_back(shcore::Value(instance_def));

    // Attempts to resolve the configuration file path
    if (!extra_options->has_key("mycnfPath")) {
      bool resolved = resolve_cnf_path(session_args, extra_options);

      if (!resolved) {
        println();
        println("The path to the MySQL Configuration is required to verify and fix the InnoDB Cluster settings");
        return shcore::Value();
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("configureLocalInstance"));

  println("Validating instance...");
  println();

  // Algorithm Step 2: Call mp on the provided MySQL URI with the validate option
  // Algorithm Step 3: IF GR is already active on the remote server, stop and tell user that the server is already part of a group and return
  shcore::Value validation_report = call_target("configureLocalInstance", target_args);

  auto result = validation_report.as_map();

  // Algorithm Step 4: IF instance is ready for GR, stop and return
  if (result->get_string("status") == "ok") {
    println("The instance '" + instance_def->get_string("host") + ":" + std::to_string(instance_def->get_int("port")) + "' is valid for Cluster usage");
    println("You can now add it to an InnoDB Cluster with the <Cluster>." + get_member_name("addInstance", naming_style) + "() function.");
    println();
  } else {
    auto errors = result->get_array("errors");

    // If there are only configuration errors, a simple restart may solve them
    bool restart_required = result->get_bool("restart_required");
    bool just_restart = false;
    bool just_dynamic = !restart_required;

    if (restart_required) {
      just_restart = errors->empty();
      if (just_restart && result->has_key("config_errors")) {
        auto config_errors = result->get_array("config_errors");

        // We gotta review item by item to determine if a simple restart is enough
        for (auto config_error : *config_errors) {
          auto error_map = config_error.as_map();

          std::string action = error_map->get_string("action");

          // If restart is required anyway, these two will be fixed on that process
          if (action == "server_update+config_update" || action == "server_update") {
            if (!restart_required)
              just_restart = false;
          } else if (action != "restart")
            just_restart = false;

          if (!just_restart)
            break;
        }
      }
    }

    if (just_restart)
      println("The configuration has been updated but it is required to restart the server.");
    else if (just_dynamic)
      println("The issues above can be fixed dynamically to get the server ready for InnoDB Cluster.");
    else {
      println("Failed to resolve all the issues, the instance is still not good for InnoDB Cluster.");
      println("Please fix these issues to get the instance ready for InnoDB Cluster:");
      println();

      print_validation_results(result);
    }

    println();
  }

  return validation_report;
}

void Global_dba::dump_table(const std::vector<std::string>& column_names, const std::vector<std::string>& column_labels, shcore::Value::Array_type_ref documents) {
  std::vector<uint64_t> max_lengths;
  std::vector<bool> numerics;

  size_t field_count = column_names.size();

  // Updates the max_length array with the maximum length between column name, min column length and column max length
  for (size_t field_index = 0; field_index < field_count; field_index++) {
    max_lengths.push_back(0);
    max_lengths[field_index] = std::max<uint64_t>(max_lengths[field_index], column_labels[field_index].size());
  }

  // Now updates the length with the real column data lengths
  if (documents) {
    size_t row_index;
    for (auto map : *documents) {
      auto document = map.as_map();
      for (size_t field_index = 0; field_index < field_count; field_index++)
        max_lengths[field_index] = std::max<uint64_t>(max_lengths[field_index], document->get_string(column_names[field_index]).size());
    }
  }

  //-----------

  size_t index = 0;
  std::vector<std::string> formats(field_count, "%-");

  // Calculates the max column widths and constructs the separator line.
  std::string separator("+");
  for (index = 0; index < field_count; index++) {
    // Creates the format string to print each field
    formats[index].append(std::to_string(max_lengths[index]));
    if (index == field_count - 1)
      formats[index].append("s |");
    else
      formats[index].append("s | ");

    std::string field_separator(max_lengths[index] + 2, '-');
    field_separator.append("+");
    separator.append(field_separator);
  }
  separator.append("\n");

  // Prints the initial separator line and the column headers
  print(separator);
  print("| ");

  for (index = 0; index < field_count; index++) {
    std::string data = (boost::format(formats[index]) % column_labels[index]).str();
    print(data.c_str());

    // Once the header is printed, updates the numeric fields formats
    // so they are right aligned
    //if (numerics[index])
    //  formats[index] = formats[index].replace(1, 1, "");
  }
  println();
  print(separator);

  if (documents) {
    // Now prints the records
    for (auto map : *documents) {
      auto document = map.as_map();

      print("| ");

      for (size_t field_index = 0; field_index < field_count; field_index++) {
        std::string raw_value = document->get_string(column_names[field_index]);
        std::string data = (boost::format(formats[field_index]) % (raw_value)).str();

        print(data.c_str());
      }
      println();
    }
  }

  println(separator);
}
