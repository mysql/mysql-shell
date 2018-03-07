/* Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.

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
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/utils/options.h"

#ifdef _MSC_VER
#define putenv _putenv
#endif

namespace shcore {

using opts::Source;
using opts::cmdline;
using opts::Basic_type;
using opts::Range;
using opts::Read_only;

class Options_test : public Shell_core_test_wrapper, public Options {
 public:
  Options_test(std::string options_file =
                   get_options_file_name("options_test_options.json"))
      : Options(options_file), options_file(options_file) {
  }

  void SetUp() {
    Shell_core_test_wrapper::SetUp();
    options.clear();
    named_options.clear();
  }

  void DummySetter(const std::string &val) {
    dummy_value = val;
  }

  void HandleMode(const std::string &opt, const char *value) {
    assert(value == nullptr);
    if (opt == "--some-mode")
      some_mode = true;
  }

  void CreateTestOptions() {
    using std::placeholders::_1;
    using std::placeholders::_2;

    // clang-format off
    ASSERT_NO_THROW(add_named_options()
     (&interactive, true, "interactive", "DUMMY_SHELL_INTERACTIVE",
       cmdline("-i", "--dummy-interactive[=bool]", "--long-dummy-interactive"),
       "Enables interactive mode", Read_only<bool>())
     (&wizards, false, "wizards", "DUMMY_USE_WIZARDS",
         cmdline("--use-wizards=bool"), "Use wizards")
     (&history_max_size, 1000, "history.maxSize", cmdline("--history-size=#"),
         "Determines max history size", Range<int>(0, 5000))
     (&sandbox_dir, "/tmp", "sandboxDir", cmdline("--sandbox-dir=dir", "-s"),
      "Default sandbox directory"));

    ASSERT_NO_THROW(add_startup_options()
      // Custom setter specified instead of storage
      (cmdline("--dummy-value=name"),
        "This is a super long help message that does not say really anything.",
        std::bind(&Options_test::DummySetter, this, _2))
      // Option with custom command line handling
      (cmdline("--some-mode"),
        "Elaborate description of mode.",
        std::bind(&Options_test::HandleMode, this, _1, _2))
      // This option is supposed to be handled externally by handler
      //  specified in constructor. This simply adds help to be printed.
      (cmdline("--only-help"),
        "This option is handled outside, only help is being displayed.")
      (cmdline("--deprecated"), opts::deprecated()));
    // clang-format on
  }

  bool interactive = true;
  bool wizards = false;
  int history_max_size = 1000;
  std::string sandbox_dir;
  std::string dummy_value;
  bool some_mode = false;

  std::string options_file;
};

TEST_F(Options_test, basic_type_validation) {
  // Boolean
  EXPECT_THROW(Basic_type<bool>()("foo", Source::User), std::invalid_argument);
  EXPECT_THROW(Basic_type<bool>()("2", Source::User), std::invalid_argument);

  EXPECT_NO_THROW({
    bool resb = Basic_type<bool>()("1", Source::User);
    EXPECT_TRUE(resb);
  });

  EXPECT_NO_THROW({
    bool resb = Basic_type<bool>()("0", Source::User);
    EXPECT_FALSE(resb);
  });

  EXPECT_NO_THROW({
    bool resb = Basic_type<bool>()("true", Source::User);
    EXPECT_TRUE(resb);
  });

  EXPECT_NO_THROW({
    bool resb = Basic_type<bool>()("false", Source::User);
    EXPECT_FALSE(resb);
  });

  // Integer
  EXPECT_THROW(Basic_type<int>()("foo", Source::User), std::invalid_argument);
  EXPECT_THROW(Basic_type<int>()("2r", Source::User), std::invalid_argument);

  int resi;
  EXPECT_NO_THROW(resi = Basic_type<int>()("1", Source::User));
  EXPECT_EQ(1, resi);

  EXPECT_NO_THROW(resi = Basic_type<int>()("9999999", Source::User));
  EXPECT_EQ(9999999, resi);

  EXPECT_NO_THROW(resi = Basic_type<int>()("-10", Source::User));
  EXPECT_EQ(-10, resi);

  // Double
  EXPECT_THROW(Basic_type<double>()("foo", Source::User),
               std::invalid_argument);
  EXPECT_THROW(Basic_type<double>()("2r", Source::User), std::invalid_argument);

  double resd;
  EXPECT_NO_THROW(resd = Basic_type<double>()("1", Source::User));
  EXPECT_EQ(1.0, resd);

  EXPECT_NO_THROW(resd = Basic_type<double>()("999.9999", Source::User));
  EXPECT_EQ(999.9999, resd);

  EXPECT_NO_THROW(resd = Basic_type<double>()("-10.5", Source::User));
  EXPECT_EQ(-10.5, resd);
}

TEST_F(Options_test, range_validation) {
  int res;

  EXPECT_THROW(Range<int>(0, 10)("11", Source::User), std::out_of_range);
  EXPECT_THROW(Range<int>(0, 10000)("-1", Source::User), std::out_of_range);

  EXPECT_NO_THROW(res = Range<int>(0, 10)("1", Source::User));
  EXPECT_EQ(1, res);

  EXPECT_NO_THROW(res = Range<int>(-10, 0)("-1", Source::User));
  EXPECT_EQ(-1, res);

  double resd;
  EXPECT_THROW(Range<double>(-0.5, 0.0)("1", Source::User), std::out_of_range);
  EXPECT_NO_THROW(resd = Range<double>(0.1, 0.2)("0.15", Source::User));
  EXPECT_EQ(0.15, resd);
}

TEST_F(Options_test, read_only_options) {
  int res;

  EXPECT_THROW(Read_only<int>()("1", Source::User), std::logic_error);

  EXPECT_NO_THROW(res = Read_only<int>()("1", Source::Command_line));
  EXPECT_EQ(1, res);

  EXPECT_NO_THROW(res = Read_only<int>()("1", Source::Environment_variable));
  EXPECT_EQ(1, res);

  EXPECT_NO_THROW(res = Read_only<int>()("1", Source::Configuration_file));
  EXPECT_EQ(1, res);
}

TEST_F(Options_test, add_options) {
  CreateTestOptions();

  ASSERT_EQ(options.size(), 8);
  EXPECT_TRUE(interactive);
  EXPECT_FALSE(wizards);
  EXPECT_EQ(1000, history_max_size);
  EXPECT_EQ("/tmp", sandbox_dir);

  bool place;
  EXPECT_THROW(
      add_startup_options()(&place, false, cmdline("-sa"), "some help"),
      std::invalid_argument);

  int history;
  EXPECT_NO_THROW(
      add_startup_options()(&history, 1, cmdline("--history=#"), "some help"));
}

TEST_F(Options_test, read_environment) {
  CreateTestOptions();

  auto it = named_options.find("interactive");
  ASSERT_TRUE(it != named_options.end());
  EXPECT_TRUE(this->interactive);

  // Env variable with incorrect value
  std::string val_decl = "DUMMY_SHELL_INTERACTIVE=DUMMY";
  ASSERT_EQ(putenv(const_cast<char *>(val_decl.c_str())), 0);
  EXPECT_THROW(it->second->handle_environment_variable(),
               std::invalid_argument);
  EXPECT_TRUE(this->interactive);

  // Correctly defined env variable
  val_decl = "DUMMY_SHELL_INTERACTIVE=0";
  ASSERT_EQ(putenv(const_cast<char *>(val_decl.c_str())), 0);
  EXPECT_NO_THROW(it->second->handle_environment_variable());
  EXPECT_FALSE(this->interactive);

  // Undefined env variable
  it = named_options.find("interactive");
  bool old = this->wizards;
  EXPECT_NO_THROW(it->second->handle_environment_variable());
  EXPECT_EQ(this->wizards, old);
}

TEST_F(Options_test, cmd_line_handling) {
  CreateTestOptions();

  EXPECT_TRUE(interactive);
  char *argv[] = {const_cast<char *>("ut"),
                  const_cast<char *>("--dummy-interactive=0"), NULL};
  ASSERT_NO_THROW(handle_cmdline_options(2, argv));
  EXPECT_FALSE(interactive);

  char *argv1[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--dummy-interactive"), NULL};
  ASSERT_NO_THROW(handle_cmdline_options(2, argv1));
  EXPECT_TRUE(interactive);

  EXPECT_FALSE(wizards);

  char *argv2[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--use-wizards=1"), NULL};
  ASSERT_NO_THROW(handle_cmdline_options(2, argv2));
  EXPECT_TRUE(wizards);

  char *argv3[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--use-wizards"), NULL};
  ASSERT_THROW(handle_cmdline_options(2, argv3), std::invalid_argument);
  EXPECT_TRUE(wizards);

  char *argv4[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--history-size=777"), NULL};
  ASSERT_NO_THROW(handle_cmdline_options(2, argv4));
  EXPECT_EQ(777, history_max_size);

  char *argv5[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--history-size=-777"), NULL};
  ASSERT_THROW(handle_cmdline_options(2, argv5), std::out_of_range);
  EXPECT_EQ(777, history_max_size);

  char *argv6[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--history-size"), NULL};
  ASSERT_THROW(handle_cmdline_options(2, argv6), std::invalid_argument);
  EXPECT_EQ(777, history_max_size);

  char *argv7[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--sandbox-dir=/tmp/dummy"), NULL};
  ASSERT_NO_THROW(handle_cmdline_options(2, argv7));
  EXPECT_EQ("/tmp/dummy", sandbox_dir);

  char *argv8[] = {const_cast<char *>("ut"), const_cast<char *>("-s/tmp"),
                   NULL};
  ASSERT_NO_THROW(handle_cmdline_options(2, argv8));
  EXPECT_EQ("/tmp", sandbox_dir);

  char *argv9[] = {const_cast<char *>("ut"), const_cast<char *>("-s"),
                   const_cast<char *>("/tmp/dummy"), NULL};
  ASSERT_NO_THROW(handle_cmdline_options(3, argv9));
  EXPECT_EQ("/tmp/dummy", sandbox_dir);

  char *argv10[] = {const_cast<char *>("ut"), const_cast<char *>("-s"), NULL};
  ASSERT_THROW(handle_cmdline_options(2, argv10), std::invalid_argument);
  EXPECT_EQ("/tmp/dummy", sandbox_dir);

  char *argv11[] = {const_cast<char *>("ut"),
                    const_cast<char *>("--dummy-value=oops"), NULL};
  ASSERT_NO_THROW(handle_cmdline_options(2, argv11));
  EXPECT_EQ("oops", dummy_value);

  char *argv12[] = {const_cast<char *>("ut"), const_cast<char *>("--some-mode"),
                    NULL};
  ASSERT_NO_THROW(handle_cmdline_options(2, argv12));
  EXPECT_TRUE(some_mode);

  char *argv13[] = {const_cast<char *>("ut"),
                    const_cast<char *>("-s"),
                    const_cast<char *>("/tmp"),
                    const_cast<char *>("--history-size=100"),
                    const_cast<char *>("--use-wizards=0"),
                    const_cast<char *>("-ifalse"),
                    NULL};
  ASSERT_NO_THROW(handle_cmdline_options(6, argv13));
  EXPECT_EQ("/tmp", sandbox_dir);
  EXPECT_EQ(100, history_max_size);
  EXPECT_FALSE(wizards);
  EXPECT_FALSE(interactive);

  char *argv14[] = {const_cast<char *>("ut"),
                    const_cast<char *>("--deprecated"), NULL};
  // Redirect cout.
  std::streambuf *cout_backup = std::cout.rdbuf();
  std::ostringstream cout;
  std::cout.rdbuf(cout.rdbuf());

  ASSERT_NO_THROW(handle_cmdline_options(2, argv14));

  // Restore old cout.
  std::cout.rdbuf(cout_backup);

  MY_EXPECT_OUTPUT_CONTAINS("The --deprecated option has been deprecated.",
                            cout.str());
}

TEST_F(Options_test, persisting) {
  CreateTestOptions();
  // No file present
  ASSERT_FALSE(options_file.empty());
  remove(options_file.c_str());
  ASSERT_NO_THROW(handle_persisted_options());

  ASSERT_FALSE(wizards);
  ASSERT_EQ(history_max_size, 1000);
  ASSERT_EQ(sandbox_dir, "/tmp");

  // Test saving options
  ASSERT_NO_THROW(save("wizards"));
  ASSERT_NO_THROW(save("history.maxSize"));
  ASSERT_NO_THROW(save("sandboxDir"));
  ASSERT_THROW(save("dummyDummy"), std::invalid_argument);
  wizards = true;
  history_max_size = 0;
  sandbox_dir = "";
  ASSERT_NO_THROW(handle_persisted_options());
  ASSERT_FALSE(wizards);
  ASSERT_EQ(history_max_size, 1000);
  ASSERT_EQ(sandbox_dir, "/tmp");

  // Test changing value of saved options
  history_max_size = 10;
  sandbox_dir = "/tmp/mysqlsh";
  ASSERT_NO_THROW(save("history.maxSize"));
  ASSERT_NO_THROW(save("sandboxDir"));
  history_max_size = 0;
  sandbox_dir = "";
  ASSERT_NO_THROW(handle_persisted_options());
  ASSERT_EQ(history_max_size, 10);
  ASSERT_EQ(sandbox_dir, "/tmp/mysqlsh");

  // Test unsaving options
  ASSERT_NO_THROW(unsave("wizards"));
  ASSERT_NO_THROW(unsave("history.maxSize"));
  ASSERT_NO_THROW(unsave("sandboxDir"));
  ASSERT_THROW(unsave("sandboxDir"), std::invalid_argument);
  wizards = true;
  history_max_size = 0;
  sandbox_dir = "";
  ASSERT_NO_THROW(handle_persisted_options());
  ASSERT_TRUE(wizards);
  ASSERT_EQ(history_max_size, 0);
  ASSERT_EQ(sandbox_dir, "");

#ifndef _MSC_VER
  // Check if processor is available
  ASSERT_NE(system(NULL), 0);
  ASSERT_EQ(system(("echo '{\n\"sandboxDir\": \n}' > " + options_file).c_str()),
            0);
  EXPECT_THROW(handle_persisted_options(), std::runtime_error);

  ASSERT_EQ(
      system(("echo '{\n\"sandboxDir\": 1\n}' > " + options_file).c_str()), 0);
  EXPECT_THROW(handle_persisted_options(), std::runtime_error);

  ASSERT_EQ(
      system(
          ("echo '{\n\"simplyWrong\": \"true\"\n}' > " + options_file).c_str()),
      0);
  EXPECT_THROW(handle_persisted_options(), std::runtime_error);
#endif
  remove(options_file.c_str());
}

TEST_F(Options_test, access_from_code) {
  CreateTestOptions();
  Options &test_options = *this;
  ASSERT_TRUE(interactive);

  EXPECT_THROW(test_options.set("interactive", "false"), std::logic_error);
  ASSERT_NO_THROW(test_options.set("wizards", "false"));
  EXPECT_FALSE(wizards);
  EXPECT_EQ(wizards, test_options.get<bool>("wizards"));

  ASSERT_NO_THROW(test_options.set("sandboxDir", "/dummy"));
  EXPECT_EQ("/dummy", sandbox_dir);
  EXPECT_EQ("/dummy", test_options.get<std::string>("sandboxDir"));
  EXPECT_THROW(test_options.get<int>("sandboxDir"), std::invalid_argument);
  EXPECT_EQ(11, test_options.get_cmdline_help().size());
  EXPECT_EQ(named_options.size(), get_options_description().size());
}
}  // namespace shcore
