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

#include "interactive_global_schema.h"
using namespace shcore;

void Global_schema::resolve() const {
  auto session = _shell_core.get_dev_session();

  if (session) {
    std::string answer;
    if (prompt("The db variable is not set, do you want to set the active schema? [y/N]:", answer)) {
      if (!answer.compare("y") || !answer.compare("Y")) {
        if (prompt("Please specify the schema:", answer)) {
          std::string error;
          if (answer.empty())
            throw shcore::Exception::argument_error("Invalid schema specified.");
          else {
            auto shell_global = _shell_core.get_global("shell");
            shcore::Argument_list current_schema;
            current_schema.push_back(shcore::Value(answer));

            shcore::Value schema = shell_global.as_object()->call("setCurrentSchema", current_schema);
          }
        }
      }
    }
  } else
    throw shcore::Exception::logic_error("The db variable is not set, establish a global session first.\n");
}
