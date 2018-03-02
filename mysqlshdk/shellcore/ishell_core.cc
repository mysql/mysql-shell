/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/ishell_core.h"
#include "utils/utils_string.h"

namespace shcore {

IShell_core::IShell_core(void) {
}
IShell_core::~IShell_core() {
}

std::string to_string(const IShell_core::Mode mode) {
  switch (mode) {
    case IShell_core::Mode::SQL:
      return "sql";
    case IShell_core::Mode::Python:
      return "py";
    case IShell_core::Mode::JavaScript:
      return "js";
    case IShell_core::Mode::None:
      return "none";
    default:
      throw std::runtime_error("Unrecognized IShell_core::Mode found");
  }
}

IShell_core::Mode parse_mode(const std::string& value) {
  if (str_casecmp(value, "sql") == 0)
    return shcore::IShell_core::Mode::SQL;
  if (str_casecmp(value, "py") == 0)
    return shcore::IShell_core::Mode::Python;
  if (str_casecmp(value, "js") == 0)
    return shcore::IShell_core::Mode::JavaScript;
  if (str_casecmp(value, "none") == 0)
    return shcore::IShell_core::Mode::JavaScript;
  throw std::invalid_argument(
      "Valid values for shell mode are sql, js, py or none.");
}
}  // namespace shcore
