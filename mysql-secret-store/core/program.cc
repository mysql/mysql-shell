/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include "mysql-secret-store/core/program.h"

#include <iostream>
#include <utility>

#include "mysql-secret-store/core/argument_parser.h"
#include "mysql-secret-store/core/erase_command.h"
#include "mysql-secret-store/core/get_command.h"
#include "mysql-secret-store/core/list_command.h"
#include "mysql-secret-store/core/store_command.h"
#include "mysql-secret-store/core/version_command.h"

namespace mysql {
namespace secret_store {
namespace core {

Program::Program(std::unique_ptr<common::Helper> helper)
    : m_helper{std::move(helper)} {
  const auto ptr = m_helper.get();
  m_commands.emplace_back(std::make_unique<Version_command>(ptr));
  m_commands.emplace_back(std::make_unique<Store_command>(ptr));
  m_commands.emplace_back(std::make_unique<Get_command>(ptr));
  m_commands.emplace_back(std::make_unique<Erase_command>(ptr));
  m_commands.emplace_back(std::make_unique<List_command>(ptr));
}

int Program::run(int argc, char *argv[]) {
  try {
    m_helper->check_requirements();

    Argument_parser(m_commands)
        .parse(argc, argv)
        ->execute(&std::cin, &std::cout);

    return 0;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}

}  // namespace core
}  // namespace secret_store
}  // namespace mysql
