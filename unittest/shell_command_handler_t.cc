/* Copyright (c) 2014, 2024, Oracle and/or its affiliates.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is designed to work with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms,
 as designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have either included with
 the program or referenced in the documentation.

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
#include <string>

#include "gtest_clean.h"
#include "scripting/lang_base.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

#include "shellcore/shell_core.h"
#include "shellcore/shell_sql.h"
#include "utils/utils_general.h"

using namespace std::placeholders;

namespace shcore {
namespace command_handler_tests {
class Environment {
 public:
  Environment() {}

  ~Environment() {}
};

class Command_handler_tests : public ::testing::Test {
 public:
  Command_handler_tests() : _shell_command_handler(false) {
    SET_SHELL_COMMAND("cmdone", "No shortcut command.",
                      Command_handler_tests::cmd_one);
    SET_SHELL_COMMAND("cmd2|\\2", "Shortcut command.",
                      Command_handler_tests::cmd_two);
    SET_CUSTOM_SHELL_COMMAND(
        "three", "No shortcut command with help.",
        std::bind(&Command_handler_tests::cmd_other, this, "cmd_three", _1),
        true, IShell_core::Mode_mask::all());
    SET_CUSTOM_SHELL_COMMAND(
        "four|\\4", "Shortcut command with help.",
        std::bind(&Command_handler_tests::cmd_other, this, "cmd_four", _1),
        true, IShell_core::Mode_mask::all());
    SET_CUSTOM_SHELL_COMMAND(
        "quoted", "No shortcut command with help.",
        std::bind(&Command_handler_tests::cmd_quoted, this, "cmd_quoted", _1),
        true, IShell_core::Mode_mask::all(), true, "\"'`");
  }

 protected:
  Shell_command_handler _shell_command_handler;
  std::string _function;
  std::vector<std::string> _params;
  Environment env;

  Shell_command_handler *command_handler() { return &_shell_command_handler; }

  bool cmd_one(const std::vector<std::string> &params) {
    _function = "cmd_one";
    _params.assign(params.begin(), params.end());

    return true;
  }

  bool cmd_two(const std::vector<std::string> &params) {
    _function = "cmd_two";
    _params.assign(params.begin(), params.end());

    return true;
  }

  bool cmd_other(const std::string &name,
                 const std::vector<std::string> &params) {
    _function = name;
    _params.assign(params.begin(), params.end());

    return true;
  }

  bool cmd_quoted(const std::string &name,
                  const std::vector<std::string> &params) {
    _function = name;
    _params.assign(params.begin(), params.end());

    return true;
  }
};

TEST_F(Command_handler_tests, processing_commands) {
  EXPECT_FALSE(_shell_command_handler.process("whatever",
                                              IShell_core::Mode::JavaScript));

  EXPECT_TRUE(
      _shell_command_handler.process("cmdone", IShell_core::Mode::JavaScript));
  EXPECT_EQ("cmd_one", _function);
  ASSERT_EQ(1, static_cast<int>(_params.size()));
  EXPECT_EQ("cmdone", _params[0]);

  EXPECT_TRUE(_shell_command_handler.process(
      "  cmdone   \" sample param \"  with  \" more spaces\" ",
      IShell_core::Mode::JavaScript));
  EXPECT_EQ("cmd_one", _function);
  ASSERT_EQ(4, static_cast<int>(_params.size()));
  EXPECT_EQ("  cmdone   \" sample param \"  with  \" more spaces\" ",
            _params[0]);
  EXPECT_EQ(" sample param ", _params[1]);
  EXPECT_EQ("with", _params[2]);
  EXPECT_EQ(" more spaces", _params[3]);

  EXPECT_TRUE(_shell_command_handler.process("cmdone parameter",
                                             IShell_core::Mode::JavaScript));
  EXPECT_EQ("cmd_one", _function);
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("cmdone parameter", _params[0]);
  EXPECT_EQ("parameter", _params[1]);

  EXPECT_TRUE(
      _shell_command_handler.process("cmd2", IShell_core::Mode::JavaScript));
  EXPECT_EQ("cmd_two", _function);
  ASSERT_EQ(1, static_cast<int>(_params.size()));
  EXPECT_EQ("cmd2", _params[0]);

  EXPECT_TRUE(_shell_command_handler.process("\\2 two parameters",
                                             IShell_core::Mode::JavaScript));
  EXPECT_EQ("cmd_two", _function);
  ASSERT_EQ(3, static_cast<int>(_params.size()));
  EXPECT_EQ("\\2 two parameters", _params[0]);
  EXPECT_EQ("two", _params[1]);
  EXPECT_EQ("parameters", _params[2]);

  EXPECT_TRUE(
      _shell_command_handler.process("three", IShell_core::Mode::JavaScript));
  EXPECT_EQ("cmd_three", _function);
  ASSERT_EQ(1, static_cast<int>(_params.size()));
  EXPECT_EQ("three", _params[0]);

  EXPECT_TRUE(_shell_command_handler.process("three three different parameters",
                                             IShell_core::Mode::JavaScript));
  EXPECT_EQ("cmd_three", _function);
  ASSERT_EQ(4, static_cast<int>(_params.size()));
  EXPECT_EQ("three three different parameters", _params[0]);
  EXPECT_EQ("three", _params[1]);
  EXPECT_EQ("different", _params[2]);
  EXPECT_EQ("parameters", _params[3]);

  EXPECT_TRUE(
      _shell_command_handler.process("four", IShell_core::Mode::JavaScript));
  EXPECT_EQ("cmd_four", _function);
  ASSERT_EQ(1, static_cast<int>(_params.size()));
  EXPECT_EQ("four", _params[0]);

  EXPECT_TRUE(_shell_command_handler.process(
      "\\4 now four different parameters", IShell_core::Mode::JavaScript));
  EXPECT_EQ("cmd_four", _function);
  ASSERT_EQ(5, static_cast<int>(_params.size()));
  EXPECT_EQ("\\4 now four different parameters", _params[0]);
  EXPECT_EQ("now", _params[1]);
  EXPECT_EQ("four", _params[2]);
  EXPECT_EQ("different", _params[3]);
  EXPECT_EQ("parameters", _params[4]);

  EXPECT_THROW(_shell_command_handler.process("\\4 \"missing closing quote",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 \"missing closing quote\\\"",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 \"\"missing space",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 missing\"\"space",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 missing space\"\"",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 missing\"space\"",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 \"too many quotes\"\"",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 \"too many\"\" quotes",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 \"too many\"\"quotes",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 \"too many \"\"quotes",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 \"\"\" too many quotes\"",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 \"\"\"too many quotes\"",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);
  EXPECT_THROW(_shell_command_handler.process("\\4 unexpected\"quote",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  EXPECT_TRUE(_shell_command_handler.process("\\4 escaped\\\"quote",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("escaped\"quote", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("\\4 escaped\\\"\\\"quotes",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("escaped\"\"quotes", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("\\4 \"\"",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("\\4 \"\" second",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(3, static_cast<int>(_params.size()));
  EXPECT_EQ("", _params[1]);
  EXPECT_EQ("second", _params[2]);

  EXPECT_TRUE(_shell_command_handler.process("\\4 first \"\"",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(3, static_cast<int>(_params.size()));
  EXPECT_EQ("first", _params[1]);
  EXPECT_EQ("", _params[2]);

  EXPECT_TRUE(
      _shell_command_handler.process("\\4 \"some \\\"quoted\\\" string \\\\ \"",
                                     IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("some \"quoted\" string \\ ", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process(
      "\\4 \"another \\\"quoted string\\\"\"", IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("another \"quoted string\"", _params[1]);
}

TEST_F(Command_handler_tests, processing_multi_quote_command) {
  // --- Escaped quotes are honored in unquoted params ---
  EXPECT_TRUE(_shell_command_handler.process("quoted escaped\\\"\\\"quotes",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("escaped\"\"quotes", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("quoted escaped\\'\\'quotes",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("escaped''quotes", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("quoted escaped\\`\\`quotes",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("escaped``quotes", _params[1]);

  // --- Escaped quotes are honored in quoted params ---
  EXPECT_TRUE(_shell_command_handler.process("quoted \"quoted\\\"param\"",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("quoted\"param", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("quoted 'quoted\\'param'",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("quoted'param", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("quoted `quoted\\`param`",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("quoted`param", _params[1]);

  // --- Unescaped quotes are rejected in unquoted params
  EXPECT_THROW(_shell_command_handler.process("quoted unexpected\"quote",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  EXPECT_THROW(_shell_command_handler.process("quoted unexpected'quote",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  EXPECT_THROW(_shell_command_handler.process("quoted unexpected`quote",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  // --- Opening and closing quotes must be of the same type
  EXPECT_THROW(_shell_command_handler.process("quoted \"missing closing quote'",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  EXPECT_THROW(_shell_command_handler.process("quoted \"missing closing quote`",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  EXPECT_THROW(_shell_command_handler.process("quoted 'missing closing quote\"",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  EXPECT_THROW(_shell_command_handler.process("quoted 'missing closing quote`",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  EXPECT_THROW(_shell_command_handler.process("quoted `missing closing quote\"",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  EXPECT_THROW(_shell_command_handler.process("quoted `missing closing quote'",
                                              IShell_core::Mode::JavaScript),
               shcore::Exception);

  // --- Escaped quotes are honored in unquoted params ---
  EXPECT_TRUE(_shell_command_handler.process("quoted \"quoted param\"",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("quoted param", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("quoted 'quoted param'",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("quoted param", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("quoted `quoted param`",
                                             IShell_core::Mode::JavaScript));
  ASSERT_EQ(2, static_cast<int>(_params.size()));
  EXPECT_EQ("quoted param", _params[1]);
}

}  // namespace command_handler_tests
}  // namespace shcore
