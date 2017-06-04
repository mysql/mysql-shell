/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/utils/process_launcher.h"

#include <gtest/gtest.h>
#ifdef _WIN32
#include <Shellapi.h>
#endif
#include <cassert>

#include "mysqlshdk/libs/utils/utils_general.h"

namespace shcore {

#ifdef WIN32
static void check_argv(const char **argv, const std::string &cmd) {
  std::wstring wstr = shcore::win_a_to_w_string(const_cast<char *>(cmd.c_str()));
  int nargs;
  LPWSTR *parsed_argv = CommandLineToArgvW(&wstr[0], &nargs);

  ASSERT_NE(nullptr, parsed_argv);

  int i;
  for (i = 0; i < nargs; i++) {
    ASSERT_NE(nullptr, argv[i]);
    std::string actual = shcore::win_w_to_a_string(parsed_argv[i], 0);
    EXPECT_STREQ(argv[i], actual.c_str());
  }
  // both should be null
  EXPECT_EQ(nullptr, parsed_argv[i]);
  EXPECT_EQ(nullptr, argv[i]);
  LocalFree(parsed_argv);
}
#else
// no-op outside windows
static void check_argv(const char **argv, const std::string &cmd) {}
#endif

TEST(Process_launcher, windows_cmdline_join) {
  {
    const char *argv[] = {"python.exe", nullptr};
    EXPECT_EQ("python.exe", Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "bla", nullptr};
    EXPECT_EQ("python.exe bla", Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "foo bar", nullptr};
    EXPECT_EQ("python.exe \"foo bar\"",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "foo", "bar", nullptr};
    EXPECT_EQ("python.exe foo bar",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "f\"o", "\"bar", nullptr};
    EXPECT_EQ("python.exe \"f\\\"o\" \"\\\"bar\"",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "f\"o", "c:\\foo\\bar\\", nullptr};
    EXPECT_EQ("python.exe \"f\\\"o\" c:\\foo\\bar\\",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
  {
    const char *argv[] = {"python.exe", "\\", "\\\"", "\\\\x",
                          "\\\\\\x",    "\"", nullptr};
    EXPECT_EQ("python.exe \\ \"\\\\\\\"\" \\\\x \\\\\\x \"\\\"\"",
              Process_launcher::make_windows_cmdline(argv));
    SCOPED_TRACE("");
    check_argv(argv, Process_launcher::make_windows_cmdline(argv));
  }
}
}  // namespace shcore
