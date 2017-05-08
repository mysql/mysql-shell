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

#include "gtest/gtest.h"

#ifndef _WIN32
#include <unistd.h>
#endif

extern "C" const char *g_argv0;

namespace tests {

#ifndef _WIN32

TEST(Bug25684798, regression_python_cmdline) {

  std::string prefix = g_argv0;
  // strip unittest/run_unit_tests
  auto pos = prefix.rfind('/');
  ASSERT_TRUE(pos != std::string::npos);
  prefix = prefix.substr(0, pos);
  pos = prefix.rfind('/');
  if (pos == std::string::npos) {
    if (prefix == ".") {
      prefix = "..";
    } else {
      prefix = ".";
    }
  } else {
    prefix = prefix.substr(0, pos);
  }
  std::string cmd = prefix+"/mysqlsh";
  cmd.append(" --py -e '1'");

  EXPECT_EQ(0, system(cmd.c_str()));

  cmd = prefix+"/mysqlsh";
  cmd.append(" --js -e '1'");
  EXPECT_EQ(0, system(cmd.c_str()));

  cmd = prefix+"/mysqlsh";
  cmd.append(" --sql -e '1'");
  EXPECT_EQ(256, system(cmd.c_str()));
}


#endif

}
