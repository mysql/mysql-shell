/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql-secret-store/core/argument_parser.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <string>

namespace mysql {
namespace secret_store {
namespace core {

Command *Argument_parser::parse(int argc, char *argv[]) const {
  auto throw_parsing_error = [this, argv](const std::string &msg) {
    throw std::runtime_error{
        msg + ", usage: " + argv[0] + " <" +
        std::accumulate(
            std::next(std::begin(m_commands)), std::end(m_commands),
            m_commands[0]->name(),
            [](const std::string &left, const std::unique_ptr<Command> &right) {
              return left + "|" + right->name();
            }) +
        ">"};
  };

  if (argc == 1) {
    throw_parsing_error("Missing command");
  }

  if (argc > 2) {
    throw_parsing_error("Too many commands");
  }

  const auto command =
      std::find_if(std::begin(m_commands), std::end(m_commands),
                   [argv](const std::unique_ptr<Command> &c) {
                     return c->name() == argv[1];
                   });

  if (command == m_commands.end()) {
    throw_parsing_error(std::string{"Unknown command: '"} + argv[1] + "'");
  }

  return command->get();
}

}  // namespace core
}  // namespace secret_store
}  // namespace mysql
