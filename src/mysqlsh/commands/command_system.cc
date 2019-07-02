/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "src/mysqlsh/commands/command_system.h"

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {

bool Command_system::execute(const std::vector<std::string> &args) {
  if (args.size() == 1) {
    // no arguments -> display help
    current_console()->print_diag(
        _shell->get_helper()->get_help("Shell Commands \\system"));
  } else {
    const auto &full_command = args[0];
    // command string has to contain at least one space, as there is more than
    // one argument
    auto pos = full_command.find(' ');

    while (std::isspace(full_command[pos])) {
      ++pos;
    }

    // we're using the full command to avoid problems with quoting
    const auto command = full_command.substr(pos);
    const int status = system(command.c_str());
    std::string error;

    if (!shcore::verify_status_code(status, &error)) {
      current_console()->print_error("System command \"" + command + "\" " +
                                     error + ".");
    }
  }

  return true;
}

}  // namespace mysqlsh
