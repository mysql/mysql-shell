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
#include "modules/mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace shcore;

void Global_dba::init()
{
  add_method("dropFarm", std::bind(&Global_dba::drop_farm, this, _1), "name", shcore::String, NULL);
  add_method("isOpen", std::bind(&Global_dba::is_open, this, _1), NULL);
  add_method("createFarm", std::bind(&Global_dba::create_farm, this, _1), "name", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Global_dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  set_wrapper_function("isOpen");
}

shcore::Value Global_dba::drop_farm(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("dropFarm").c_str());

  try
  {
    std::string farm_name = args.string_at(0);

    if (farm_name.empty())
      throw shcore::Exception::argument_error("The Farm name cannot be empty");

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
      shcore::print((boost::format("To remove the Farm '%1%' the default replica set needs to be removed.\n") % farm_name).str());
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
      new_args.push_back(shcore::Value(farm_name));
      new_args.push_back(shcore::Value(options));

      ret_val = _target->call("dropFarm", new_args);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropFarm"));

  return ret_val;
}

shcore::Value Global_dba::is_open(const shcore::Argument_list &args)
{
  return _target ? _target->call("isOpen", args) : shcore::Value::False();
}

shcore::Value Global_dba::create_farm(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  args.ensure_count(1, 3, get_function_name("createFarm").c_str());

  try
  {
    std::string farm_name = args.string_at(0);
    std::string answer, farm_password, instance_admin_user_pwd;
    shcore::Value::Map_type_ref options;

    if (farm_name.empty())
      throw Exception::argument_error("The Farm name cannot be empty.");

    if (args.size() > 1)
    {
      int opts_index = 1;
      if (args[1].type == shcore::String)
      {
        farm_password = args.string_at(1);
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
            if (password("Please enter '" + farm_name + "' MySQL instance administration user password: ", answer))
              instance_admin_user_pwd = answer;
          }
        }
      }
    }

    bool prompt_password = true;
    while (prompt_password && farm_password.empty())
    {
      prompt_password = password("Please enter an administration password to be used for the Farm '" + farm_name + "': ", answer);
      if (prompt_password)
        farm_password = answer;
    }

    if (!farm_password.empty())
    {
      shcore::Argument_list new_args;
      new_args.push_back(shcore::Value(farm_name));
      new_args.push_back(shcore::Value(farm_password));

      if (!instance_admin_user_pwd.empty())
        (*options)["instanceAdminPassword"] = shcore::Value(answer);

      if (options != NULL)
        new_args.push_back(shcore::Value(options));

      ret_val = _target->call("createFarm", new_args);
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
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))

  return ret_val;
}