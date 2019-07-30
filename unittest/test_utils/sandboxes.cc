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

#include "unittest/test_utils/sandboxes.h"

#include <string>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace tests {
namespace sandbox {

int k_ports[k_num_ports] = {0};

void initialize() {
  static bool initialized = false;

  if (initialized) {
    return;
  }

  for (int i = 0; i < k_num_ports; ++i) {
    const auto sandbox_port =
        getenv(shcore::str_format("MYSQL_SANDBOX_PORT%i", i + 1).c_str());
    if (sandbox_port) {
      k_ports[i] = std::stoi(sandbox_port);
    } else {
      k_ports[i] = std::stoi(getenv("MYSQL_PORT")) + (i + 1) * 10;
    }
  }

  initialized = true;
}

}  // namespace sandbox
}  // namespace tests
