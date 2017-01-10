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
#include "utils/utils_general.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

using namespace std::placeholders;
using namespace shcore;

void Global_session::init() {
  add_method("getSchema", std::bind(&Global_session::get_schema, this, _1), "name", shcore::String, NULL);
  add_method("isOpen", std::bind(&Global_session::is_open, this, _1), NULL);
  set_wrapper_function("isOpen");
}

void Global_session::resolve() const {
  std::string answer;
  bool error = true;
  std::string message = "The global session is not set, do you want to establish a session?\n\n"\
  "   1) MySQL Document Store Session through X Protocol\n"\
  "   2) Classic MySQL Session\n\n"\
  "Please select the session type or ENTER to cancel: ";
  
  if (prompt(message, answer)) {
    if (!answer.empty() && answer.length() == 1) {
      mysqlsh::SessionType type;
      std::string options = "12";
      switch (options.find(answer)) {
        case 0:
          type = mysqlsh::SessionType::Node;
          error = false;
          break;
        case 1:
          type = mysqlsh::SessionType::Classic;
          error = false;
          break;
        default:
          break;
      }

      if (!error) {
        if (prompt("Please specify the MySQL server URI: ", answer)) {
          Value::Map_type_ref connection_data = shcore::get_connection_data(answer);

          if (!connection_data->has_key("dbPassword")) {
            if (password("Enter password: ", answer))
              (*connection_data)["dbPassword"] = shcore::Value(answer);
          }

          if (connection_data) {
            shcore::Argument_list args;
            args.push_back(shcore::Value(connection_data));

            _shell_core.connect_dev_session(args, type);
          }
        }
      }
    }
    
    if(error)
      print("Invalid session type\n");
  }
}

shcore::Value Global_session::get_schema(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  try {
    ret_val = call_target("getSchema", args);
  } catch (shcore::Exception &e) {
    std::string error(e.what());
    if (error.find("Unknown database") != std::string::npos) {
      std::string answer;
      if (prompt((boost::format("The schema %1% does not exist, do you want to create it? [y/N]: ") % args.string_at(0)).str().c_str(), answer)) {
        if (!answer.compare("y") || !answer.compare("Y")) {
          ret_val = call_target("createSchema", args);
        }
      }
    } else
      throw;
  }

  return ret_val;
}

shcore::Value Global_session::is_open(const shcore::Argument_list &args) {
  return _target ? call_target("isOpen", args) : shcore::Value::False();
}
