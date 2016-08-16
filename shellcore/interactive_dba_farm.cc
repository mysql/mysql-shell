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

#include "interactive_dba_farm.h"
#include "interactive_global_dba.h"
#include "modules/adminapi/mod_dba_farm.h"
#include "shellcore/shell_registry.h"
#include "modules/mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace shcore;

void Interactive_dba_cluster::init()
{
  add_method("addSeedInstance", std::bind(&Interactive_dba_cluster::add_seed_instance, this, _1), "data");
  add_method("addInstance", std::bind(&Interactive_dba_cluster::add_instance, this, _1), "data");
}

shcore::Value Interactive_dba_cluster::add_seed_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  std::string function;

  shcore::Value rset = _target->call("getReplicaSet", shcore::Argument_list());
  auto object = rset.as_object<mysh::mysqlx::ReplicaSet>();
  if (object)
  {
    std::string answer;
    if (prompt("The default ReplicaSet is already initialized. Do you want to add a new instance? [Y|n]: ", answer))
    {
      if (!answer.compare("y") || !answer.compare("Y") || answer.empty())
        function = "addInstance";
    }
  }
  else
    function = "addSeedInstance";

  if (!function.empty())
  {
    shcore::Value::Map_type_ref options;
    std::string cluster_admin_password;

    if (resolve_instance_options(function, args, options))
    {
      shcore::Argument_list new_args;
      cluster_admin_password = _shell_core.get_global("dba").as_object<Global_dba>()->get_cluster_admin_password();
      new_args.push_back(shcore::Value(cluster_admin_password));
      new_args.push_back(shcore::Value(options));
      ret_val = _target->call(function, new_args);
    }
  }

  return ret_val;
}

shcore::Value Interactive_dba_cluster::add_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  std::string function;

  shcore::Value rset = _target->call("getReplicaSet", shcore::Argument_list());
  auto object = rset.as_object<mysh::mysqlx::ReplicaSet>();
  if (!object)
  {
    std::string answer;
    if (prompt("The default ReplicaSet is not initialized. Do you want to initialize it adding a seed instance? [Y|n]: ", answer))
    {
      if (!answer.compare("y") || !answer.compare("Y") || answer.empty())
        function = "addSeedInstance";
    }
  }
  else
    function = "addInstance";

  if (!function.empty())
  {
    shcore::Value::Map_type_ref options;
    std::string cluster_admin_password;

    if (resolve_instance_options(function, args, options))
    {
      shcore::Argument_list new_args;
      cluster_admin_password = _shell_core.get_global("dba").as_object<Global_dba>()->get_cluster_admin_password();
      new_args.push_back(shcore::Value(cluster_admin_password));
      new_args.push_back(shcore::Value(options));
      ret_val = _target->call(function, new_args);
    }
  }

  return ret_val;
}

int Interactive_dba_cluster::identify_connection_options(const std::string &function, const shcore::Argument_list &args) const
{
  int options_index = 0;

  if ((args.size() == 2 && args[0].type == shcore::Map) || args.size() == 3)
    options_index++;
  else if (args.size() == 2 && args[0].type == shcore::String && args[1].type == shcore::String)
  {
    std::string message = "Ambiguous call for to" + get_function_name(function) + ", from the parameters:\n\n" \
      " 1) " + args[0].as_string() + "\n"\
      " 2) " + args[1].as_string() + "\n"\
      " 3) Cancel the operation.\n\n"\
      "Which is the connection data? [3] :";

    std::string answer;
    if (prompt(message, answer))
    {
      if (answer.empty() || answer == "3")
        options_index = -1;
      else if (answer == "2")
        options_index++;
    }
  }

  return options_index;
}

bool Interactive_dba_cluster::resolve_instance_options(const std::string& function, const shcore::Argument_list &args, shcore::Value::Map_type_ref &options) const
{
  std::string answer;
  args.ensure_count(1, 3, get_function_name(function).c_str());

  bool proceed = true;

  // The signature of the addInstance and addSeedInstance functions is as follows
  // addInstance([clusterPwd,] connOptions[, rootPwd])
  //
  // connOptions can be either a map or a URI
  // When URI is used with one of hte passwords, call is ambiguos so the user needs to
  // determine what parameter is the URI
  int options_index = identify_connection_options(function, args);

  if (options_index >= 0)
  {
    if (args[options_index].type == String)
      options = get_connection_data(args.string_at(options_index), false);
    else if (args[options_index].type == Map)
       options = args.map_at(options_index);
    else
       throw shcore::Exception::argument_error(get_function_name(function) + ": Invalid connection options, expected either a URI or a Dictionary.");

    auto invalids = shcore::get_additional_keys(options, mysh::mysqlx::ReplicaSet::_add_instance_opts);

    // Verification of invalid attributes on the connection data
    if (invalids.size())
    {
      std::string error = "The connection data contains the following invalid attributes: ";
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

  // Verification of the host attribute
  if (proceed && !options->has_key("host"))
  {
    if (prompt("The connection data is missing the host, would you like to: 1) Use localhost  2) Specify a host  3) Cancel  Please select an option [1]: ", answer))
    {
      if (answer == "1")
        (*options)["host"] = shcore::Value("localhost");
      else if (answer == "2")
      {
        if (prompt("Please specify the host: ", answer))
          (*options)["host"] = shcore::Value(answer);
        else
          proceed = false;
      }
      else
        proceed = false;
    }
  }

  if (proceed)
  {
    // Verification of the user attribute
    std::string user;
    if (options->has_key("user"))
      user = options->get_string("user");
    else if (options->has_key("dbUser"))
      user = options->get_string("dbUser");
    else
    {
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
    else if ((args.size() == 2 && options_index == 0) || args.size() == 3)
    {
      user_password = args.string_at(args.size() - 1);
      (*options)["dbPassword"] = shcore::Value(user_password);
    }
    else
      has_password = false;

    if (!has_password)
    {
      proceed = false;
      if (password("Please provide the password for '" + build_connection_string(options, false) + "': ", answer))
      {
        (*options)["dbPassword"] = shcore::Value(answer);
        proceed = true;
      }
    }
  }

  // Verify the clusterAdminPassword
  std::string cluster_password;

  if (options_index == 1)
    cluster_password = args.string_at(0);

  if (cluster_password.empty())
    cluster_password = _shell_core.get_global("dba").as_object<Global_dba>()->get_cluster_admin_password();

  bool prompt_password = true;
  while (prompt_password && cluster_password.empty())
  {
    prompt_password = password("Please enter Cluster administrative MASTER password: ", cluster_password);
    if (prompt_password)
    {
      if (!cluster_password.empty())
      // update the cache
      _shell_core.get_global("dba").as_object<Global_dba>()->set_cluster_admin_password(cluster_password);
    }
  }

  return proceed;
}