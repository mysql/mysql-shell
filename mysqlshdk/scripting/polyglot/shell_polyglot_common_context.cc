/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/scripting/polyglot/shell_polyglot_common_context.h"

#include <iostream>

#include "mysqlshdk/libs/utils/logger.h"

namespace shcore {

void Shell_polyglot_common_context::fatal_error() {
  // Something really bad happened, so we simply ensure it is indicated on the
  // screen.
  std::cout << "Polyglot: FATAL error occurred" << std::endl;
}

void Shell_polyglot_common_context::flush() {}

void Shell_polyglot_common_context::log(const char *bytes, size_t length) {
  log_debug("Polyglot: %.*s", static_cast<int>(length), bytes);
}

polyglot::Garbage_collector::Config Shell_polyglot_common_context::gc_config() {
  polyglot::Garbage_collector::Config config;

  // Perform Garbage Collection in 10 seconds intervals if at least 10
  // statements were executed
  config.interval = 10;
  config.statements = 10;

  return config;
}

}  // namespace shcore