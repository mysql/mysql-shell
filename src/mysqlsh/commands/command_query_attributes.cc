/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "src/mysqlsh/commands/command_query_attributes.h"

#include <map>
#include <optional>
#include <set>

#include "modules/mod_mysql_session.h"
#include "mysqlshdk/include/shellcore/base_session.h"

namespace mysqlsh {

bool Command_query_attributes::execute(const std::vector<std::string> &args) {
  // Args 0 is the full command call, and we need to ensure we are getting part
  // number of attribute/values
  auto pair_args = (args.size() % 2) == 1;

  auto console = mysqlsh::current_console();

  //
  if (!pair_args) {
    console->print_error(
        "The \\query_attributes shell command receives attribute value pairs "
        "as arguments, odd number of arguments received.");
    console->print(
        "Usage: \\query_attributes name1 value1 name2 value2 ... nameN "
        "valueN.\n");
  } else {
    std::shared_ptr<mysqlsh::mysql::ClassicSession> csession;
    if (const auto session = _shell->get_dev_session();
        session && session->is_open()) {
      csession =
          std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(session);
    }

    if (!csession) {
      console->print_error(
          "An open session through the MySQL protocol is required to execute "
          "the \\query_attributes shell command.\n");
    } else {
      // Cleans previously defined attribute set
      auto &qas = csession->query_attribute_store();

      qas.clear();

      // Traverse the arguments 2 by 2 defining query attributes
      bool attributes_ok = true;

      for (size_t index = 1; index < args.size(); index += 2) {
        if (!qas.set(args[index], shcore::Value(args[index + 1]))) {
          attributes_ok = false;
        }
      }

      if (!attributes_ok) {
        qas.handle_errors(false);
      }
    }
  }

  return true;
}

}  // namespace mysqlsh
