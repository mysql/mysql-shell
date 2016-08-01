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

#include "interactive_global_session.h"
#include "shellcore/shell_registry.h"
#include "utils/utils_general.h"
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace shcore;

void Global_session::init()
{
  add_method("getSchema", std::bind(&Global_session::get_schema, this, _1), "name", shcore::String, NULL);
  add_method("isOpen", std::bind(&Global_session::is_open, this, _1), NULL);
  set_wrapper_function("isOpen");
}

void Global_session::resolve() const
{
  std::string answer;
  bool error = false;
  if (prompt("The global session is not set, do you want to establish a session? [y/N]: ", answer))
  {
    if (!answer.compare("y") || !answer.compare("Y"))
    {
      if (prompt("Please specify the session type:\n\n   1) X\n   2) Node\n   3) Classic\n\nType: ", answer))
      {
        mysh::SessionType type;
        std::string options = "123";
        switch (options.find(answer))
        {
          case 0:
            type = mysh::Application;
            break;
          case 1:
            type = mysh::Node;
            break;
          case 2:
            type = mysh::Classic;
            break;
          default:
            error = true;
        }

        if (!error)
        {
          if (prompt("Please specify the MySQL server URI (or $alias): ", answer))
          {
            Value::Map_type_ref connection_data;
            if (answer.find("$") == 0)
            {
              std::string stored_session_name = answer.substr(1);
              if (StoredSessions::get_instance()->connections()->has_key(stored_session_name))
                connection_data = (*StoredSessions::get_instance()->connections())[stored_session_name].as_map();
              else
                throw shcore::Exception::argument_error((boost::format("The stored connection %1% was not found") % stored_session_name).str());
            }
            else
              connection_data = shcore::get_connection_data(answer);

            if (!connection_data->has_key("dbPassword"))
            {
              if (password("Enter password: ", answer))
                (*connection_data)["dbPassword"] = shcore::Value(answer);
            }

            if (connection_data)
            {
              shcore::Argument_list args;
              args.push_back(shcore::Value(connection_data));

              _shell_core.connect_dev_session(args, type);
            }
          }
        }
      }
    }
  }
}

shcore::Value Global_session::get_schema(const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  try
  {
    ret_val = _target->call("getSchema", args);
  }
  catch (shcore::Exception &e)
  {
    std::string error(e.what());
    if (error.find("Unknown database") != std::string::npos)
    {
      std::string answer;
      if (prompt((boost::format("The schema %1% does not exist, do you want to create it? [y/N]: ") % args.string_at(0)).str().c_str(), answer))
      {
        if (!answer.compare("y") || !answer.compare("Y"))
        {
          ret_val = _target->call("createSchema", args);
        }
      }
    }
    else
      throw;
  }

  return ret_val;
}

shcore::Value Global_session::is_open(const shcore::Argument_list &args)
{
  return _target ? _target->call("isOpen", args) : shcore::Value::False();
}