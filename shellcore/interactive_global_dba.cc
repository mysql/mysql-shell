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
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace shcore;

void Global_dba::init()
{
  add_varargs_method("deployLocalInstance", std::bind(&Global_dba::deploy_local_instance, this, _1));
  add_varargs_method("startLocalInstance", std::bind(&Global_dba::start_local_instance, this, _1));
  add_varargs_method("deleteLocalInstance", std::bind(&Global_dba::delete_local_instance, this, _1));
  add_varargs_method("killLocalInstance", std::bind(&Global_dba::kill_local_instance, this, _1));
  add_varargs_method("stopLocalInstance", std::bind(&Global_dba::stop_local_instance, this, _1));

  add_method("dropCluster", std::bind(&Global_dba::drop_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("createCluster", std::bind(&Global_dba::create_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Global_dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("getCluster", std::bind(&Global_dba::get_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("validateInstance", std::bind(&Global_dba::validate_instance, this, _1), "data", shcore::Map, NULL);
}

shcore::Value Global_dba::exec_instance_op(const std::string &function, const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  shcore::Argument_list new_args;

  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string answer;
  bool proceed = true;
  std::string sandboxDir;

  try
  {
    int port = args.int_at(0);
    new_args.push_back(args[0]);

    if (args.size() == 2)
    {
      new_args.push_back(args[1]);
      options = args.map_at(1);
      // Verification of invalid attributes on the instance deployment data
      auto invalids = shcore::get_additional_keys(options, mysh::mysqlx::Dba::_deploy_instance_opts);
      if (invalids.size())
      {
        std::string error = "The instance data contains the following invalid attributes: ";
        error += shcore::join_strings(invalids, ", ");

        proceed = false;
        if (prompt((boost::format("%s. Do you want to ignore these attributes and continue? [Y/n]: ") % error).str().c_str(), answer))
        {
          proceed = (!answer.compare("y") || !answer.compare("Y") || answer.empty());

          if (proceed)
          {
            for (auto attribute : invalids)
              options->erase(attribute);
          }
        }
      }
    }
    else
    {
      options.reset(new shcore::Value::Map_type());
      new_args.push_back(shcore::Value(options));
    }
    if (!options->has_key("sandboxDir"))
    {
      if (shcore::Shell_core_options::get()->has_key(SHCORE_SANDBOX_DIR))
      {
        sandboxDir = (*shcore::Shell_core_options::get())[SHCORE_SANDBOX_DIR].as_string();
        (*options)["sandboxDir"] = shcore::Value(sandboxDir);
      }
    }
    else
      sandboxDir = (*options)["sandboxDir"].as_string();

    if (proceed)
    {
      std::string message;

      if (function == "deploy" || function == "start")
      {
        // Verification of required attributes on the instance deployment data
        auto missing = shcore::get_missing_keys(options, { "password|dbPassword" });

        if (missing.size())
        {
          proceed = false;
          bool prompt_password = true;

          while (prompt_password && !proceed)
          {
            if (function == "deploy")
            {
              message = "A new MySQL sandbox instance will be created on this host in \n"\
                        "" + sandboxDir + "/" + std::to_string(port) + "\n\n"
                        "Please enter a MySQL root password for the new instance: ";
            }

            if (function == "start")
            {
              message = "The MySQL sandbox instance on this host in \n"\
                        "" + sandboxDir + "/" + std::to_string(port) + " will be started\n\n"
                        "Please enter the MySQL root password of the instance: ";
            }

            prompt_password = password(message, answer);
            if (prompt_password)
            {
              if (!answer.empty())
              {
                (*options)["password"] = shcore::Value(answer);
                proceed = true;
              }
            }
          }
        }
      }
      else
      {
        if (function == "delete")
        {
          message = "The MySQL sandbox instance on this host in \n"\
                    "" + sandboxDir + "/" + std::to_string(port) + " will be deleted\n";
          println(message);
          proceed = true;
        }
        else if (function == "kill")
        {
          message = "The MySQL sandbox instance on this host in \n"\
                    "" + sandboxDir + "/" + std::to_string(port) + " will be killed\n";
          println(message);
          proceed = true;
        }

        else if (function == "stop")
        {
          message = "The MySQL sandbox instance on this host in \n"\
                    "" + sandboxDir + "/" + std::to_string(port) + " will be stopped\n";
          _shell_core.println(message);
          proceed = true;
        }
      }
    }

    if (proceed)
    {
      if (function == "deploy")
      {
        println("Deploying new MySQL instance...");
        ret_val = _target->call("deployLocalInstance", new_args);

        println("Instance localhost:" + std::to_string(port) +
            " successfully deployed and started.\n");
        println("Use '\\connect -c root@localhost:" + std::to_string(port) + "' to connect to the new instance.");
      }

      if (function == "start")
      {
        println("Starting MySQL instance...");
        ret_val = _target->call("startLocalInstance", new_args);

        println("Instance localhost:" + std::to_string(port) + " successfully started.\n");
      }

      if (function == "delete")
      {
        println("Deleting MySQL instance...");
        ret_val = _target->call("deleteLocalInstance", new_args);

        println("Instance localhost:" + std::to_string(port) + " successfully deleted.\n");
      }

      if (function == "kill")
      {
        println("Killing MySQL instance...");
        ret_val = _target->call("killLocalInstance", new_args);

        println("Instance localhost:" + std::to_string(port) + " successfully killed.\n");
      }

      if (function == "stop")
      {
        _shell_core.println("Stopping MySQL instance...");
        ret_val = _target->call("stopLocalInstance", new_args);

        _shell_core.println("Instance localhost:" + std::to_string(port) +
            " successfully stopped.\n");
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(function);

  return ret_val;
}

shcore::Value Global_dba::deploy_local_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("deployLocalInstance").c_str());

  return exec_instance_op("deploy", args);
}

shcore::Value Global_dba::start_local_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("startLocalInstance").c_str());

  return exec_instance_op("start", args);
}

shcore::Value Global_dba::delete_local_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("deleteLocalInstance").c_str());

  return exec_instance_op("delete", args);
}

shcore::Value Global_dba::kill_local_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("killLocalInstance").c_str());

  return exec_instance_op("kill", args);
}

shcore::Value Global_dba::stop_local_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("stopLocalInstance").c_str());

  return exec_instance_op("stop", args);
}

shcore::Value Global_dba::drop_cluster(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("dropCluster").c_str());

  try
  {
    std::string cluster_name = args.string_at(0);

    if (cluster_name.empty())
      throw shcore::Exception::argument_error("The Cluster name cannot be empty");

    shcore::Value::Map_type_ref options;
    bool valid_options = false;
    if (args.size() == 2)
    {
      options = args.map_at(1);
      valid_options = options->has_key("dropDefaultReplicaSet");
    }

    if (!valid_options)
    {
      std::string answer;
      println((boost::format("To remove the Cluster '%1%' the default replica set needs to be removed.") % cluster_name).str());
      if (prompt((boost::format("Do you want to remove the default replica set? [y/n]: ")).str().c_str(), answer))
      {
        if (!answer.compare("y") || !answer.compare("Y"))
        {
          options.reset(new shcore::Value::Map_type);
          (*options)["dropDefaultReplicaSet"] = shcore::Value(true);

          valid_options = true;
        }
      }
    }

    if (valid_options)
    {
      shcore::Argument_list new_args;
      new_args.push_back(shcore::Value(cluster_name));
      new_args.push_back(shcore::Value(options));

      ret_val = _target->call("dropCluster", new_args);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropCluster"));

  return ret_val;
}

shcore::Value Global_dba::create_cluster(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  args.ensure_count(1, 3, get_function_name("createCluster").c_str());

  try
  {
    std::string cluster_name = args.string_at(0);
    std::string answer, cluster_password;
    shcore::Value::Map_type_ref options;
    bool verbose = false; // Default is false

    if (cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

    if (args.size() > 1)
    {
      int opts_index = 1;
      if (args[1].type == shcore::String)
      {
        cluster_password = args.string_at(1);
        opts_index++;
      }

      if (args.size() > opts_index)
      {
        options = args.map_at(opts_index);
        // Check if some option is missing
        // TODO: Validate adminType parameter value

        if (options->has_key("verbose"))
          verbose = options->get_bool("verbose");
      }
    }

    auto dba = std::dynamic_pointer_cast<mysh::mysqlx::Dba>(_target);
    auto session = dba->get_active_session();
    bool prompt_password = true;
    while (prompt_password && cluster_password.empty())
    {
      std::string message = "A new InnoDB cluster will be created on instance '" + session->uri() + "'.\n\n"
                            "When setting up a new InnoDB cluster it is required to define an administrative\n"\
                            "MASTER key for the cluster.This MASTER key needs to be re - entered when making\n"\
                            "changes to the cluster later on, e.g.adding new MySQL instances or configuring\n"\
                            "MySQL Routers.Losing this MASTER key will require the configuration of all\n"\
                            "InnoDB cluster entities to be changed.\n";

      println(message);

      prompt_password = password("Please specify an administrative MASTER key for the cluster '" + cluster_name + "':", answer);
      if (prompt_password)
      {
        if (!answer.empty())
          cluster_password = answer;
      }
    }

    if (!cluster_password.empty())
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

shcore::Value Global_dba::drop_metadata_schema(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  try
  {
    if (args.size() < 1)
    {
      std::string answer;

      if (prompt((boost::format("Are you sure you want to remove the Metadata? [y/N]: ")).str().c_str(), answer))
      {
        if (!answer.compare("y") || !answer.compare("Y"))
        {
          shcore::Argument_list new_args;
          Value::Map_type_ref options(new shcore::Value::Map_type);

          (*options)["enforce"] = shcore::Value(true);
          new_args.push_back(shcore::Value(options));

          ret_val = _target->call("dropMetadataSchema", new_args);
        }
      }
    }
    else
      ret_val = _target->call("dropMetadataSchema", args);

    println("Metadata Schema successfully removed.");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))

  return ret_val;
}

shcore::Value Global_dba::get_cluster(const shcore::Argument_list &args)
{
  Value ret_val;
  args.ensure_count(0, 2, get_function_name("getCluster").c_str());

  std::string master_key;
  shcore::Value::Map_type_ref options;

  shcore::Argument_list new_args(args);
  std::string cluster_name;
  bool get_default_cluster = false;

  if (args.size())
  {
    if (args.size() == 1)
    {
      if (args[0].type == shcore::String)
        cluster_name = args[0].as_string();
      else if (args[0].type == shcore::Map)
      {
        options = args[0].as_map();
        get_default_cluster = true;
      }
      else
        throw shcore::Exception::argument_error("Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options");
    }
    else
    {
      cluster_name = args.string_at(0);
      options = args[1].as_map();
    }
  }
  else
    get_default_cluster = true;

  if (!options)
  {
    options.reset(new shcore::Value::Map_type());
    new_args.push_back(shcore::Value(options));
  }

  if (options->has_key("masterKey"))
    master_key = options->get_string("masterKey");

  bool prompt_key = true;
  while (prompt_key && master_key.empty())
  {
    std::string message = "When the InnoDB cluster was setup, a MASTER key was defined in order to enable\n"\
                          "performing administrative tasks on the cluster.\n";

    println(message);

    if (get_default_cluster)
      message = "Please specify the administrative MASTER key for the default cluster: ";
    else
      message = "Please specify the administrative MASTER key for the cluster '" + cluster_name + "':";

    prompt_key = password(message, master_key);
    if (prompt_key)
    {
      if (!master_key.empty())
        (*options)["masterKey"] = shcore::Value(master_key);
    }
  }

  ScopedStyle ss(_target.get(), naming_style);
  Value raw_cluster = _target->call("getCluster", new_args);

  Interactive_dba_cluster* cluster = new Interactive_dba_cluster(this->_shell_core);
  cluster->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(raw_cluster.as_object()));
  return shcore::Value::wrap<Interactive_dba_cluster>(cluster);
}

shcore::Value Global_dba::validate_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  shcore::Argument_list new_args;

  args.ensure_count(1, get_function_name("validateInstance").c_str());

  std::string uri, answer, user;
  shcore::Value::Map_type_ref options; // Map with the connection data

  // Identify the type of connection data (String or Document)
  if (args[0].type == shcore::String)
  {
    uri = args.string_at(0);
    options = shcore::get_connection_data(uri, false);
  }
  // Connection data comes in a dictionary
  else if (args[0].type == shcore::Map)
    options = args.map_at(0);

  // Verification of required attributes on the connection data
  auto missing = shcore::get_missing_keys(options, { "host", "port" });

  if (missing.size())
  {
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
  else if (args.size() == 2 && args[1].type == shcore::String)
  {
    user_password = args.string_at(1);
    (*options)["dbPassword"] = shcore::Value(user_password);
  }
  else
    has_password = false;

  if (!has_password)
  {
    if (password("Please provide a password for '" + build_connection_string(options, false) + "': ", answer))
      (*options)["dbPassword"] = shcore::Value(answer);
  }

  new_args.push_back(shcore::Value(options));

  // Let's get the user to know we're starting to validate the instance
  println("Validating instance...");

  ret_val = _target->call("validateInstance", new_args);

  return ret_val;
}
