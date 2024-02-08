/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/process_launcher.h"

#include "gtest_clean.h"
#ifdef _WIN32
#include <Shellapi.h>
#endif
#include <cassert>
#include <memory>

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

#ifdef WIN32
static void check_argv(const char **argv, const std::string &cmd) {
  const std::wstring wstr = shcore::utf8_to_wide(cmd);
  int nargs = 0;
  std::unique_ptr<LPWSTR[], decltype(&LocalFree)> parsed_argv(
      CommandLineToArgvW(&wstr[0], &nargs), LocalFree);

  ASSERT_NE(nullptr, parsed_argv);

  int i = 0;
  for (i = 0; i < nargs; i++) {
    ASSERT_NE(nullptr, argv[i]);
    const std::string actual = shcore::wide_to_utf8(parsed_argv[i]);
    EXPECT_STREQ(argv[i], actual.c_str());
  }
  // both should be null
  EXPECT_EQ(nullptr, parsed_argv[i]);
  EXPECT_EQ(nullptr, argv[i]);
}
#else
// no-op outside windows
static void check_argv(const char **, const std::string &) {}
#endif

TEST(Process_launcher, windows_cmdline_join) {
  {
    const char *argv[] = {"python.exe", nullptr};
    EXPECT_EQ("python.exe", Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("check_argv");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "bla", nullptr};
    EXPECT_EQ("python.exe bla", Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("check_argv");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "foo bar", nullptr};
    EXPECT_EQ("python.exe \"foo bar\"",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("check_argv");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "foo", "bar", nullptr};
    EXPECT_EQ("python.exe foo bar",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("check_argv");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "f\"o", "\"bar", nullptr};
    EXPECT_EQ("python.exe \"f\\\"o\" \"\\\"bar\"",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("check_argv");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "f\"o", "c:\\foo\\bar\\", nullptr};
    EXPECT_EQ("python.exe \"f\\\"o\" c:\\foo\\bar\\",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("check_argv");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "\\", "\\\"", "\\\\x",
                          "\\\\\\x",    "\"", nullptr};
    EXPECT_EQ("python.exe \\ \"\\\\\\\"\" \\\\x \\\\\\x \"\\\"\"",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("check_argv");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "f\\\"o", nullptr};
    EXPECT_EQ("python.exe \"f\\\\\\\"o\"",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("check_argv");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "f\"o\\", nullptr};
    EXPECT_EQ("python.exe \"f\\\"o\\\\\"",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("check_argv");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
}

}  // namespace shcore
