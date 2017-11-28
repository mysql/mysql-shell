/* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>

#include "test_utils/shell_test_env.h"
#include "gtest_clean.h"

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

}
