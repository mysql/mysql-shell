/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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
#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

#include "src/mysqlsh/json_shell.h"

namespace mysqlsh {

static bool print_capture(void *cdata, const char *text) {
  std::string *capture = static_cast<std::string *>(cdata);
  capture->append(text).append("\n");
  return true;
}

TEST(Json_shell, invalid_input) {
  mysqlsh::Json_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shcore::Interpreter_print_handler handler{&capture, print_capture,
                                            print_capture, print_capture};
  current_console()->add_print_handler(&handler);

  // --- INVALID INPUT TESTS --- //
  // The Json_shell only accepts Json documents as input
  std::vector<std::string> invalid_inputs = {
      "session", "\\status", "[\"Json\", \"array\"]", "\"json-string\""};

  for (const auto &input : invalid_inputs) {
    capture.clear();
    shell.process_line(input);
    SCOPED_TRACE("Testing: " + input);
    EXPECT_THAT(capture, ::testing::HasSubstr("{\"error\":\"Invalid input:"));
  }

  // --- INVALID COMMAND TESTS --- //
  // The Json_shell requires Json objects with one of the following commands:
  // command, execute, complete
  std::vector<std::string> invalid_commands = {
      "{}", "{\"\":\"whatever\"}", "{\"invalid-command\":\"whatever\"}"};

  for (const auto &input : invalid_commands) {
    capture.clear();
    shell.process_line(input);
    SCOPED_TRACE("Testing: " + input);
    EXPECT_THAT(capture,
                ::testing::HasSubstr("{\"error\":\"Invalid command in input:"));
  }

  // --- INVALID DATA FOR EXECUTE COMMAND --- //
  // The Json object for execute should be: {"execute": <json-string>}
  std::vector<std::string> invalid_execute_input = {
      "{\"execute\":1}", "{\"execute\":true}",
      "{\"execute\":[\"value-array\"]}", "{\"execute\":{\"json\":\"doc\"}}"};

  for (const auto &input : invalid_execute_input) {
    capture.clear();
    shell.process_line(input);
    SCOPED_TRACE("Testing: " + input);
    EXPECT_THAT(capture,
                ::testing::HasSubstr("{\"error\":\"Invalid input for "
                                     "'execute', string expected in value:"));
  }

  // --- INVALID DATA FOR COMMAND COMMAND --- //
  // The Json object for execute should be: {"command": <json-string>}
  std::vector<std::string> invalid_command_input = {
      "{\"command\":1}", "{\"command\":true}",
      "{\"command\":[\"value-array\"]}", "{\"command\":{\"json\":\"doc\"}}"};

  for (const auto &input : invalid_command_input) {
    capture.clear();
    shell.process_line(input);
    SCOPED_TRACE("Testing: " + input);
    EXPECT_THAT(capture,
                ::testing::HasSubstr("{\"error\":\"Invalid input for "
                                     "'command', string expected in value:"));
  }

  // --- INVALID DATA FOR COMPLETE COMMAND --- //
  // The Json object for execute should be:
  //   {"complete": {"data":<string>[,"offset":<uint>]}}
  std::vector<std::string> invalid_complete_input = {
      "{\"complete\":1}",
      "{\"complete\":true}",
      "{\"complete\":[\"value-array\"]}",
      "{\"complete\":{\"json\":\"doc\"}}",
      "{\"complete\":{\"data\":1}}",
      "{\"complete\":{\"data\":[]}}",
      "{\"complete\":{\"data\":true}}",
      "{\"complete\":{\"data\":\"text\",\"offset\":\"\"}}",
      "{\"complete\":{\"data\":\"text\",\"offset\":5.3}}",
      "{\"complete\":{\"data\":\"text\",\"offset\":[]}}",
      "{\"complete\":{\"data\":\"text\",\"offset\":{}}}"};

  for (const auto &input : invalid_complete_input) {
    capture.clear();
    shell.process_line(input);
    SCOPED_TRACE("Testing: " + input);
    EXPECT_THAT(
        capture,
        ::testing::HasSubstr(
            "{\"error\":\"Invalid input for 'complete', object with 'data' "
            "member (and optional uint 'offset' member) expected: "));
  }

  current_console()->remove_print_handler(&handler);
}

TEST(Json_shell, valid_commands) {
  mysqlsh::Json_shell shell(std::make_shared<Shell_options>());

  std::string capture;
  shcore::Interpreter_print_handler handler{&capture, print_capture,
                                            print_capture, print_capture};
  current_console()->add_print_handler(&handler);

  capture.clear();
  shell.process_line("{\"execute\":\"print('success!')\"}");
  EXPECT_THAT(capture, ::testing::HasSubstr("{\"info\":\"success!\"}"));

  capture.clear();
  shell.process_line("{\"complete\":{\"data\":\"dba.\"}}");
  EXPECT_THAT(capture, ::testing::HasSubstr(
                           "{\"info\":\"{\\\"offset\\\":4,\\\"options\\\":["
                           "\\\"checkInstanceConfiguration()\\\","));

  capture.clear();
  shell.process_line("{\"command\":\"\\\\py\"}");
  EXPECT_THAT(capture, ::testing::HasSubstr(
                           "{\"info\":\"Switching to Python mode...\"}"));

  capture.clear();
  shell.process_line("{\"complete\":{\"data\":\"dba.\"}}");
  EXPECT_THAT(capture, ::testing::HasSubstr(
                           "{\"info\":\"{\\\"offset\\\":4,\\\"options\\\":["
                           "\\\"check_instance_configuration()\\\","));

  current_console()->remove_print_handler(&handler);
}

}  // namespace mysqlsh
