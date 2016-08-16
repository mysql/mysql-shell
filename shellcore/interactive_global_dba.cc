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
#include "shellcore/interactive_dba_farm.h"
#include "modules/mysqlxtest_utils.h"
#include "modules/adminapi/mod_dba.h"
#include "utils/utils_general.h"
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace shcore;

void Global_dba::init()
{
  add_varargs_method("deployLocalInstance", std::bind(&Global_dba::deploy_local_instance, this, _1));
  add_method("dropCluster", std::bind(&Global_dba::drop_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("isOpen", std::bind(&Global_dba::is_open, this, _1), NULL);
  add_method("createCluster", std::bind(&Global_dba::create_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Global_dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("getCluster", std::bind(&Global_dba::get_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("getDefaultCluster", std::bind(&Global_dba::get_default_cluster, this, _1), NULL);
  add_method("validateInstance", std::bind(&Global_dba::validate_instance, this, _1), "data", shcore::Map, NULL);
  set_wrapper_function("isOpen");
}

shcore::Value Global_dba::deploy_local_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("deployLocalInstance").c_str());

  shcore::Value ret_val;

  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string answer;
  bool proceed = true;

  try
  {
    options = args.map_at(0);

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

    if (proceed)
    {
      // Verification of required attributes on the instance deployment data
      auto missing = shcore::get_missing_keys(options, { "password|dbPassword" });

      if (missing.size())
      {
        if (args.size() == 2)
          (*options)["password"] = shcore::Value(args.string_at(1));
        else
        {
          proceed = false;
          bool prompt_password = true;
          while (prompt_password && !proceed)
          {
            prompt_password = password("Please enter root password for the deployed instance: ", answer);
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
    }

    if (proceed)
    {
      shcore::print("Deploying instance...\n");
      ret_val = _target->call("deployLocalInstance", args);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("deployLocalInstance"));

  return ret_val;
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
      shcore::print((boost::format("To remove the Cluster '%1%' the default replica set needs to be removed.\n") % cluster_name).str());
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

shcore::Value Global_dba::is_open(const shcore::Argument_list &args)
{
  return _target ? _target->call("isOpen", args) : shcore::Value::False();
}

shcore::Value Global_dba::create_cluster(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  args.ensure_count(1, 3, get_function_name("createCluster").c_str());

  try
  {
    std::string cluster_name = args.string_at(0);
    std::string answer, cluster_password, instance_admin_user_pwd;
    shcore::Value::Map_type_ref options;

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
        if (options->has_key("instanceAdminUser"))
        {
          if (!options->has_key("instanceAdminPassword"))
          {
            if (password("Please enter '" + cluster_name + "' MySQL instance administration user password: ", answer))
              instance_admin_user_pwd = answer;
          }
        }
      }
    }

    bool prompt_password = true;
    while (prompt_password && cluster_password.empty())
    {
      prompt_password = password("Please enter an administrative MASTER password to be used for the Cluster '" + cluster_name + "': ", answer);
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

      if (!instance_admin_user_pwd.empty())
        (*options)["instanceAdminPassword"] = shcore::Value(answer);

      if (options != NULL)
        new_args.push_back(shcore::Value(options));

      // This is an instance of the API cluster
      auto raw_cluster = _target->call("createCluster", new_args);

      // Returns an interactive wrapper of this instance
      Interactive_dba_cluster* cluster = new Interactive_dba_cluster(this->_shell_core);
      cluster->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(raw_cluster.as_object()));
      ret_val = shcore::Value::wrap<Interactive_dba_cluster>(cluster);
    }
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createFarm"));

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

    shcore::print("Metadata Schema successfully removed.\n");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))

  return ret_val;
}

shcore::Value Global_dba::get_cluster(const shcore::Argument_list &args)
{
  ScopedStyle ss(_target.get(), naming_style);

  Value raw_cluster = _target->call("getCluster", args);

  Interactive_dba_cluster* cluster = new Interactive_dba_cluster(this->_shell_core);
  cluster->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(raw_cluster.as_object()));
  return shcore::Value::wrap<Interactive_dba_cluster>(cluster);
}

shcore::Value Global_dba::get_default_cluster(const shcore::Argument_list &args)
{
  ScopedStyle ss(_target.get(), naming_style);

  Value raw_cluster = _target->call("getDefaultCluster", args);

  Interactive_dba_cluster* cluster = new Interactive_dba_cluster(this->_shell_core);
  cluster->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(raw_cluster.as_object()));
  return shcore::Value::wrap<Interactive_dba_cluster>(cluster);
}

shcore::Value Global_dba::validate_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

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

  // Verification of the password
  std::string user_password;
  bool has_password = true;

  // Sets a default user if not specified
  if (options->has_key("user"))
    user = options->get_string("user");
  else if (options->has_key("dbUser"))
    user = options->get_string("dbUser");
  else
  {
    user = "root";
    (*options)["dbUser"] = shcore::Value(user);
  }

  if (options->has_key("password"))
    user_password = options->get_string("password");
  else if (options->has_key("dbPassword"))
    user_password = options->get_string("dbPassword");
  else
    has_password = false;

  if (!has_password)
  {
    if (password("Please provide a password for '" + build_connection_string(options, false) + "': ", answer))
      (*options)["dbPassword"] = shcore::Value(answer);
  }

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(options));

  // Let's get the user to know we're starting to validate the instance
  shcore::print("Validating instance...\n");

  ret_val = _target->call("validateInstance", new_args);

  return ret_val;
}