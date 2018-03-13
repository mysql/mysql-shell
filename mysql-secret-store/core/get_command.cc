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

#include "mysql-secret-store/core/get_command.h"

#include <sstream>

#include "mysql-secret-store/core/json_converter.h"

namespace mysql {
namespace secret_store {
namespace core {

std::string Get_command::help() const { return "Gets the secret."; }

void Get_command::execute(std::istream *input, std::ostream *output) {
  std::stringstream buffer;
  buffer << input->rdbuf();
  auto secret_id = converter::to_secret_id(buffer.str());
  std::string secret;

  helper()->get(secret_id, &secret);

  *output << converter::to_string({secret, secret_id});
}

}  // namespace core
}  // namespace secret_store
}  // namespace mysql
