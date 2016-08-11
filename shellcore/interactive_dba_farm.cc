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
#include "modules/adminapi/mod_dba_farm.h"
#include "shellcore/shell_registry.h"
#include "modules/mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace shcore;

void Interactive_dba_farm::init()
{
  add_method("addSeedInstance", std::bind(&Interactive_dba_farm::add_seed_instance, this, _1), "data");
  add_method("addInstance", std::bind(&Interactive_dba_farm::add_instance, this, _1), "data");
}

shcore::Value Interactive_dba_farm::add_seed_instance(const shcore::Argument_list &args)
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
    if (resolve_instance_options(function, args, options))
    {
      shcore::Argument_list new_args;
      new_args.push_back(shcore::Value(options));
      ret_val = _target->call(function, args);
    }
  }

  return ret_val;
}

shcore::Value Interactive_dba_farm::add_instance(const shcore::Argument_list &args)
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
    if (resolve_instance_options(function, args, options))
    {
      shcore::Argument_list new_args;
      new_args.push_back(shcore::Value(options));
      ret_val = _target->call(function, args);
    }
  }

  return ret_val;
}

bool Interactive_dba_farm::resolve_instance_options(const std::string& function, const shcore::Argument_list &args, shcore::Value::Map_type_ref &options) const
{
  std::string answer;
  args.ensure_count(1, 2, get_function_name(function).c_str());

  // Gets the connection options which come on the first parameter
  // Either as URI or as JSON
  if (args[0].type == String)
    options = get_connection_data(args.string_at(0), false);
  else if (args[0].type == Map)
    options = args.map_at(0);
  else
    throw shcore::Exception::argument_error(get_function_name(function) + ": Invalid connection options, expected either a URI or a Dictionary.");

  // Gets the list of incoming attributes
  std::set<std::string> attributes;
  for (auto option : *options)
    attributes.insert(option.first);

  auto invalids = mysh::mysqlx::ReplicaSet::get_invalid_attributes(attributes);

  // Verification of invalid attributes on the connection data
  bool proceed = true;
  if (invalids.size())
  {
    std::string error = "The connection data contains the next invalid attributes: ";
    std::string first = *invalids.begin();
    error += first;

    invalids.erase(invalids.begin());

    for (auto attribute : invalids)
      error += ", " + attribute;

    invalids.insert(first);

    proceed = false;
    if (prompt((boost::format("%s.\nDo you want to ignore these attributes and continue? [Y/n]: ") % error).str().c_str(), answer))
    {
      proceed = (!answer.compare("y") || !answer.compare("Y") || answer.empty());

      if (proceed)
      {
        for (auto attribute : invalids)
          options->erase(attribute);
      }
    }
  }

  // Verification of the host attribute
  if (proceed && !options->has_key("host"))
  {
    if (prompt("The connection data is missing the host, would you like to: \n  1) Use localhost\n 2) Specify a host\n 3) Cancel\n\nPlease select an option [1]: ", answer))
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
      print("The connection data does not contain a user, using 'root'.\n");
    }

    // Verification of the password
    std::string password;
    bool has_password = true;
    if (options->has_key("password"))
      password = options->get_string("password");
    else if (options->has_key("dbPassword"))
      password = options->get_string("dbPassword");
    else if (args.size() == 2)
      password = args.string_at(1);
    else
      has_password = false;

    if (!has_password)
    {
      proceed = false;
      if (prompt("Please provide a password for " + build_connection_string(options, false), answer))
      {
        (*options)["dbPassword"] = shcore::Value(answer);
        proceed = true;
      }
    }
  }

  return proceed;
}