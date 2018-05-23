/* Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "gtest_clean.h"
#include "test_utils/shell_test_env.h"

#ifndef _WIN32
#include <unistd.h>
#endif

namespace tests {

#ifndef _WIN32

TEST(Bug25684798, regression_python_cmdline) {
  std::string mysqlsh_path = Shell_test_env::get_path_to_mysqlsh();
  std::string cmd = mysqlsh_path + " --py -e '1'";

  EXPECT_EQ(0, system(cmd.c_str()));

  cmd = mysqlsh_path + " --js -e '1'";
  EXPECT_EQ(0, system(cmd.c_str()));

  cmd = mysqlsh_path + " --sql -e '1'";
  EXPECT_EQ(256, system(cmd.c_str()));
}

#endif
}  // namespace tests
