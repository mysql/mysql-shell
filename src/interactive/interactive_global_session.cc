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

#include "interactive_global_session.h"
#include "interactive_global_shell.h"
#include "utils/utils_general.h"
#include "modules/mod_shell.h"
#include "utils/utils_string.h"
#include "modules/mod_utils.h"

using namespace std::placeholders;
using namespace shcore;

void Global_session::init() {
  add_method("getSchema", std::bind(&Global_session::get_schema, this, _1), "name", shcore::String, NULL);
  add_method("isOpen", std::bind(&Global_session::is_open, this, _1), NULL);
}

void Global_session::resolve() const {
  std::string answer;
  bool error = true;
  std::string message = "There is no active session, do you want to establish one?\n\n"\
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
          auto connection_options = shcore::get_connection_options(answer);

          if (!connection_options.has_password()) {
            if (password("Enter password: ", answer))
              connection_options.set_password(answer);
          }

          auto session =
              mysqlsh::Shell::connect_session(connection_options, type);

          // Since this is an interactive global,
          // it means the shell global is also interactive
          auto shell_global =
              _shell_core.get_global("shell").as_object<Global_shell>();
          auto shell_object = std::dynamic_pointer_cast<mysqlsh::Shell>(
              shell_global->get_target());
          shell_object->set_dev_session(session);
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
      if (prompt(str_format("The schema %s does not exist, do you want to\
          create it?", args.string_at(0).c_str()), Prompt_answer::NO)
          == Prompt_answer::YES)
        ret_val = call_target("createSchema", args);
    } else
      throw;
  }

  return ret_val;
}

shcore::Value Global_session::is_open(const shcore::Argument_list &args) {
  return _target ? call_target("isOpen", args) : shcore::Value::False();
}
