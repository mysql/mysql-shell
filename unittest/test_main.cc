/* Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.

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

#ifdef HAVE_PYTHON
#include <Python.h>
#endif

#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>

int main(int argc, char **argv) {
#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif

  if (!getenv("MYSQL_URI")) {
    std::cerr << "WARNING: The MYSQL_URI MYSQL_PWD and MYSQL_PORT environment variables are not set\n";
    std::cerr << "Note: MYSQL_URI must not contain the port number\n";
    std::cerr << "Note: Use MYSQL_PORT to define the MySQL protocol port (if != 3306)\n";
    std::cerr << "Note: Use MYSQLX_PORT to define the XProtocol port (if != 33060)\n";
    exit(1);
  }

  ::testing::InitGoogleTest(&argc, argv);

  // Helper code for DBA specific groups of tests;
  std::string flags = ::testing::GTEST_FLAG(filter);

  if (!flags.empty()) {
    std::string new_flags;

    if (flags == "DBA")
      new_flags = "Shell_py_dba_tests.*:Shell_js_dba_tests.*";
    else if (flags == "DBAJS")
      new_flags = "Shell_js_dba_tests.*";
    else if (flags == "DBAPY")
      new_flags = "Shell_py_dba_tests.*";
    else if (flags == "DBAJSNIG")
      new_flags = "Shell_js_dba_tests.no_interactive_de*:Shell_js_dba_tests.no_interactive_classic_global*";
    else if (flags == "DBAPYNIG")
      new_flags = "Shell_py_dba_tests.no_interactive_de*:Shell_py_dba_tests.no_interactive_classic_global*";
    else if (flags == "DBAJSNIC")
      new_flags = "Shell_js_dba_tests.no_interactive_de*:Shell_js_dba_tests.no_interactive_classic_custom*";
    else if (flags == "DBAPYNIC")
      new_flags = "Shell_py_dba_tests.no_interactive_de*:Shell_py_dba_tests.no_interactive_classic_custom*";
    else if (flags == "DBAJSNI")
      new_flags = "Shell_js_dba_tests.no_interactive_de*:Shell_js_dba_tests.no_interactive_classic_*";
    else if (flags == "DBAPYNI")
      new_flags = "Shell_py_dba_tests.no_interactive_de*:Shell_py_dba_tests.no_interactive_classic_*";
    else if (flags == "DBAJSIG")
      new_flags = "Shell_js_dba_tests.no_interactive_de*:Shell_js_dba_tests.interactive_classic_global*";
    else if (flags == "DBAPYIG")
      new_flags = "Shell_py_dba_tests.no_interactive_de*:Shell_py_dba_tests.interactive_classic_global*";
    else if (flags == "DBAJSIC")
      new_flags = "Shell_js_dba_tests.no_interactive_de*:Shell_js_dba_tests.interactive_classic_custom*";
    else if (flags == "DBAPYIC")
      new_flags = "Shell_py_dba_tests.no_interactive_de*:Shell_py_dba_tests.interactive_classic_custom*";
    else if (flags == "DBAJSI")
      new_flags = "Shell_js_dba_tests.no_interactive_de*:Shell_js_dba_tests.interactive_classic_*";
    else if (flags == "DBAPYI")
      new_flags = "Shell_py_dba_tests.no_interactive_de*:Shell_py_dba_tests.interactive_classic_*";
    else if (flags == "ALLBUTDBA")
      new_flags = "*:-Shell_py_dba_tests.*:Shell_js_dba_tests.*";

    if (!new_flags.empty())
      ::testing::GTEST_FLAG(filter) = new_flags.c_str();
  }

  const char *generate_option = "--generate_test_groups=";
  if (argc > 1 && strncmp(argv[1], generate_option, strlen(generate_option)) == 0) {
    const char *path = strchr(argv[1], '=') + 1;
    std::ofstream f(path);

    std::cout << "Updating " << path << "...\n";
    f << "# Automatically generated, use make testgroups to update\n";

    ::testing::UnitTest *ut = ::testing::UnitTest::GetInstance();
    for (int i = 0; i < ut->total_test_case_count(); i++) {
      const char *name = ut->GetTestCase(i)->name();
      f << "add_test(" << name << " run_unit_tests --gtest_filter=" << name << ".*)\n";
    }
    return 0;
  }

  int ret_val = RUN_ALL_TESTS();

  return ret_val;
}
