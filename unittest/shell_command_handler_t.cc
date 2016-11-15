/* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.

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

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"

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
  Command_handler_tests() {
    SET_SHELL_COMMAND("cmdone", "No shortcut command.", "", Command_handler_tests::cmd_one);
    SET_SHELL_COMMAND("cmd2|\\2", "Shortcut command.", "", Command_handler_tests::cmd_two);
    SET_CUSTOM_SHELL_COMMAND("three", "No shortcut command with help.", "Third command.", std::bind(&Command_handler_tests::cmd_other, this, "cmd_three", _1));
    SET_CUSTOM_SHELL_COMMAND("four|\\4", "Shortcut command with help.", "Fourth command.", std::bind(&Command_handler_tests::cmd_other, this, "cmd_four", _1));
  }
protected:
  Shell_command_handler _shell_command_handler;
  std::string _function;
  std::vector<std::string> _params;
  Environment env;

  bool cmd_one(const std::vector<std::string>& params) {
    _function = "cmd_one";
    _params.assign(params.begin(), params.end());

    return true;
  }

  bool cmd_two(const std::vector<std::string>& params) {
    _function = "cmd_two";
    _params.assign(params.begin(), params.end());

    return true;
  }

  bool cmd_other(const std::string& name, const std::vector<std::string>& params) {
    _function = name;
    _params.assign(params.begin(), params.end());

    return true;
  }
};

TEST_F(Command_handler_tests, processing_commands) {
  EXPECT_FALSE(_shell_command_handler.process("whatever"));

  EXPECT_TRUE(_shell_command_handler.process("cmdone"));
  EXPECT_EQ("cmd_one", _function);
  ASSERT_EQ(1, static_cast<int>(static_cast<int>(_params.size())));
  EXPECT_EQ("cmdone", _params[0]);

  EXPECT_TRUE(_shell_command_handler.process("  cmdone   \" sample param \"  with  \" more spaces\" "));
  EXPECT_EQ("cmd_one", _function);
  ASSERT_EQ(4, static_cast<int>(static_cast<int>(_params.size())));
  EXPECT_EQ("  cmdone   \" sample param \"  with  \" more spaces\" ", _params[0]);
  EXPECT_EQ("\" sample param \"", _params[1]);
  EXPECT_EQ("with", _params[2]);
  EXPECT_EQ("\" more spaces\"", _params[3]);


  EXPECT_TRUE(_shell_command_handler.process("cmdone parameter"));
  EXPECT_EQ("cmd_one", _function);
  ASSERT_EQ(2, static_cast<int>(static_cast<int>(_params.size())));
  EXPECT_EQ("cmdone parameter", _params[0]);
  EXPECT_EQ("parameter", _params[1]);

  EXPECT_TRUE(_shell_command_handler.process("cmd2"));
  EXPECT_EQ("cmd_two", _function);
  ASSERT_EQ(1, static_cast<int>(static_cast<int>(_params.size())));
  EXPECT_EQ("cmd2", _params[0]);

  EXPECT_TRUE(_shell_command_handler.process("\\2 two parameters"));
  EXPECT_EQ("cmd_two", _function);
  ASSERT_EQ(3, static_cast<int>(static_cast<int>(_params.size())));
  EXPECT_EQ("\\2 two parameters", _params[0]);
  EXPECT_EQ("two", _params[1]);
  EXPECT_EQ("parameters", _params[2]);

  EXPECT_TRUE(_shell_command_handler.process("three"));
  EXPECT_EQ("cmd_three", _function);
  ASSERT_EQ(1, static_cast<int>(static_cast<int>(_params.size())));
  EXPECT_EQ("three", _params[0]);

  EXPECT_TRUE(_shell_command_handler.process("three three different parameters"));
  EXPECT_EQ("cmd_three", _function);
  ASSERT_EQ(4, static_cast<int>(_params.size()));
  EXPECT_EQ("three three different parameters", _params[0]);
  EXPECT_EQ("three", _params[1]);
  EXPECT_EQ("different", _params[2]);
  EXPECT_EQ("parameters", _params[3]);

  EXPECT_TRUE(_shell_command_handler.process("four"));
  EXPECT_EQ("cmd_four", _function);
  ASSERT_EQ(1, static_cast<int>(_params.size()));
  EXPECT_EQ("four", _params[0]);

  EXPECT_TRUE(_shell_command_handler.process("\\4 now four different parameters"));
  EXPECT_EQ("cmd_four", _function);
  ASSERT_EQ(5, static_cast<int>(_params.size()));
  EXPECT_EQ("\\4 now four different parameters", _params[0]);
  EXPECT_EQ("now", _params[1]);
  EXPECT_EQ("four", _params[2]);
  EXPECT_EQ("different", _params[3]);
  EXPECT_EQ("parameters", _params[4]);
}

TEST_F(Command_handler_tests, getting_commands) {
  std::string commands = _shell_command_handler.get_commands("These are the test commands:");
  auto lines = shcore::split_string(commands, "\n");

  EXPECT_EQ("These are the test commands:", lines[0]);
  EXPECT_EQ("cmdone      No shortcut command.", lines[1]);
  EXPECT_EQ("cmd2   (\\2) Shortcut command.", lines[2]);
  EXPECT_EQ("three       No shortcut command with help.", lines[3]);
  EXPECT_EQ("four   (\\4) Shortcut command with help.", lines[4]);
}

TEST_F(Command_handler_tests, getting_command_help) {
  EXPECT_FALSE(_shell_command_handler.process("whatever"));

  std::string help;

  EXPECT_TRUE(_shell_command_handler.get_command_help("cmdone", help));
  EXPECT_EQ("No shortcut command.", help);

  help.clear();
  EXPECT_TRUE(_shell_command_handler.get_command_help("cmd2", help));
  EXPECT_EQ("Shortcut command.\n\nTRIGGERS: cmd2 or \\2", help);

  help.clear();
  EXPECT_TRUE(_shell_command_handler.get_command_help("\\2", help));
  EXPECT_EQ("Shortcut command.\n\nTRIGGERS: cmd2 or \\2", help);

  help.clear();
  EXPECT_TRUE(_shell_command_handler.get_command_help("three", help));
  EXPECT_EQ("No shortcut command with help.\n\nThird command.", help);

  help.clear();
  EXPECT_TRUE(_shell_command_handler.get_command_help("four", help));
  EXPECT_EQ("Shortcut command with help.\n\nTRIGGERS: four or \\4\n\nFourth command.", help);

  help.clear();
  EXPECT_TRUE(_shell_command_handler.get_command_help("\\4", help));
  EXPECT_EQ("Shortcut command with help.\n\nTRIGGERS: four or \\4\n\nFourth command.", help);
}
}
}
