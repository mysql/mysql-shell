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

#include "mysqlshdk/libs/utils/utils_json.h"
#include "src/mysqlsh/json_shell.h"
#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

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
  shell.process_line("{\"execute\":\"\\\\py\"}");
  EXPECT_THAT(capture, ::testing::HasSubstr(
                           "{\"info\":\"Switching to Python mode...\"}"));

  capture.clear();
  shell.process_line("{\"complete\":{\"data\":\"dba.\"}}");
  EXPECT_THAT(capture, ::testing::HasSubstr(
                           "{\"info\":\"{\\\"offset\\\":4,\\\"options\\\":["
                           "\\\"check_instance_configuration()\\\","));

  current_console()->remove_print_handler(&handler);
}

TEST(Json_shell, incomplete_javascript) {
  mysqlsh::Json_shell shell(std::make_shared<Shell_options>());
  shell.process_line({"{\"execute\":\"\\\\js\"}"});

  std::string capture;
  shcore::Interpreter_print_handler handler{&capture, print_capture,
                                            print_capture, print_capture};
  current_console()->add_print_handler(&handler);

  std::vector<std::tuple<std::string, std::string>> invalid_inputs = {
      {
          R"*(function sample() {
        print("sample");
      )*",
          "{\"error\":\"SyntaxError: Unexpected end of input at (shell):4:1"},
      {
          R"*(function sample() {
        print("sample";
        }
      )*",
          "{\"error\":\"SyntaxError: missing ) after argument list at "
          "(shell):2:15"},
      {
          R"*(function sample() {
        print("sample);
        }
      )*",
          "{\"error\":\"SyntaxError: Invalid or unexpected token at "
          "(shell):2:15"},
  };  // namespace mysqlsh

  for (const auto &input : invalid_inputs) {
    shcore::JSON_dumper doc;

    doc.start_object();
    doc.append_string("execute");
    doc.append_string(std::get<0>(input));
    doc.end_object();

    shell.process_line(doc.str());
    SCOPED_TRACE("Testing: " + std::get<0>(input));
    EXPECT_THAT(capture, ::testing::HasSubstr(std::get<1>(input)));
    capture.clear();
  }

  current_console()->remove_print_handler(&handler);
}

TEST(Json_shell, incomplete_python) {
  mysqlsh::Json_shell shell(std::make_shared<Shell_options>());
  shell.process_line({"{\"execute\":\"\\\\py\"}"});

  std::string capture;
  shcore::Interpreter_print_handler handler{&capture, print_capture,
                                            print_capture, print_capture};
  current_console()->add_print_handler(&handler);

  std::vector<std::tuple<std::string, std::string>> invalid_inputs = {
      {R"*(
def sample(data):
  print(data)

sample("some text"
)*",
       "{\"error\":\"unexpected EOF while parsing\"}"},
      {R"*(
def sample(data):
  print(data)

sample("some text)
)*",
       "\"error\":\"EOL while scanning string literal\"}"}};

  for (const auto &input : invalid_inputs) {
    shcore::JSON_dumper doc;

    doc.start_object();
    doc.append_string("execute");
    doc.append_string(std::get<0>(input));
    doc.end_object();

    shell.process_line(doc.str());
    SCOPED_TRACE("Testing: " + std::get<0>(input));
    EXPECT_THAT(capture, ::testing::HasSubstr(std::get<1>(input)));
    capture.clear();
  }

  current_console()->remove_print_handler(&handler);
}

TEST(Json_shell, incomplete_sql) {
  const char *pwd = getenv("MYSQL_PWD");
  auto coptions = shcore::get_connection_options("mysql://root@localhost");
  if (pwd)
    coptions.set_password(pwd);
  else
    coptions.set_password("");
  coptions.set_port(getenv("MYSQL_PORT") ? atoi(getenv("MYSQL_PORT")) : 3306);

  mysqlsh::Json_shell shell(std::make_shared<Shell_options>());
  shell.process_line({"{\"execute\":\"\\\\sql\"}"});
  shell.process_line({"{\"execute\":\"\\\\c " +
                      coptions.as_uri(mysqlshdk::db::uri::formats::full()) +
                      "\"}"});

  std::string capture;
  shcore::Interpreter_print_handler handler{&capture, print_capture,
                                            print_capture, print_capture};
  current_console()->add_print_handler(&handler);

  std::vector<std::tuple<std::string, std::string>> invalid_inputs = {
      {R"*(select *
from mysql
where)*",
       "{\"error\":{\"code\":1064,\"message\":\"You have an error in your SQL "
       "syntax; check the manual that corresponds to your MySQL server version "
       "for the right syntax to use near '' at line "
       "3\",\"state\":\"42000\",\"type\":\"MySQL Error\"}}"},
      {R"*(select * from mysql.user
where user = 'weirdo)*",
       "{\"error\":{\"code\":1064,\"message\":\"You have an error in your SQL "
       "syntax; check the manual that corresponds to your MySQL server version "
       "for the right syntax to use near ''weirdo' at line "
       "2\",\"state\":\"42000\",\"type\":\"MySQL Error\"}}"}};

  for (const auto &input : invalid_inputs) {
    shcore::JSON_dumper doc;

    doc.start_object();
    doc.append_string("execute");
    doc.append_string(std::get<0>(input));
    doc.end_object();

    shell.process_line(doc.str());
    SCOPED_TRACE("Testing: " + std::get<0>(input));
    EXPECT_THAT(capture, ::testing::HasSubstr(std::get<1>(input)));
    capture.clear();
  }

  current_console()->remove_print_handler(&handler);
  shell.process_line({"{\"execute\":\"\\\\disconnect\"}"});
}

TEST(Json_shell, js_completed_without_new_line) {
  mysqlsh::Json_shell shell(std::make_shared<Shell_options>());
  shell.process_line({"{\"execute\":\"\\\\js\"}"});

  std::string capture;
  shcore::Interpreter_print_handler handler{&capture, print_capture,
                                            print_capture, print_capture};
  current_console()->add_print_handler(&handler);

  // The processing of the following function will make the shell enter in
  // multiline mode, expecting for an empty line to get out, however, the
  // JSON_shell expects complete statements so it will force the execution of
  // the code, exiting the multiline mode and letting the shell ready for the
  // next command
  shcore::JSON_dumper doc;
  doc.start_object();
  doc.append_string("execute");
  doc.append_string(R"*(function sample(data) {
  print(data);
})*");
  doc.end_object();
  capture.clear();

  // No errors and the function is properly defined
  shell.process_line(doc.str());
  EXPECT_STREQ("", capture.c_str());
  shell.process_line("{\"execute\":\"sample('Successful!!!')\"}");
  EXPECT_THAT(capture, ::testing::HasSubstr("{\"info\":\"Successful!!!\"}"));
}

TEST(Json_shell, py_completed_without_new_line) {
  mysqlsh::Json_shell shell(std::make_shared<Shell_options>());
  shell.process_line({"{\"execute\":\"\\\\py\"}"});

  std::string capture;
  shcore::Interpreter_print_handler handler{&capture, print_capture,
                                            print_capture, print_capture};
  current_console()->add_print_handler(&handler);

  // The processing of the following function will make the shell enter in
  // multiline mode, expecting for an empty line to get out, however, the
  // JSON_shell expects complete statements so it will force the execution of
  // the code, exiting the multiline mode and letting the shell ready for the
  // next command
  shcore::JSON_dumper doc;
  doc.start_object();
  doc.append_string("execute");
  doc.append_string(R"*(def sample(data):
  print(data))*");
  doc.end_object();
  capture.clear();

  // No errors and the function is properly defined
  shell.process_line(doc.str());
  EXPECT_STREQ("", capture.c_str());
  shell.process_line("{\"execute\":\"sample('Successful!!!')\"}");
  EXPECT_THAT(capture, ::testing::HasSubstr("{\"info\":\"Successful!!!\"}"));
}

TEST(Json_shell, sql_completed_without_delimiter) {
  const char *pwd = getenv("MYSQL_PWD");
  auto coptions = shcore::get_connection_options("mysql://root@localhost");
  if (pwd)
    coptions.set_password(pwd);
  else
    coptions.set_password("");
  coptions.set_port(getenv("MYSQL_PORT") ? atoi(getenv("MYSQL_PORT")) : 3306);

  mysqlsh::Json_shell shell(std::make_shared<Shell_options>());
  shell.process_line({"{\"execute\":\"\\\\sql\"}"});
  shell.process_line({"{\"execute\":\"\\\\c " +
                      coptions.as_uri(mysqlshdk::db::uri::formats::full()) +
                      "\"}"});

  std::string capture;
  shcore::Interpreter_print_handler handler{&capture, print_capture,
                                            print_capture, print_capture};
  current_console()->add_print_handler(&handler);

  // The processing of the following function will make the shell enter in
  // multiline mode, expecting for the SQL Delimiter, however, the
  // Json_shell expects complete statements so it will force the execution of
  // the code, successfully processing the SQL command even the delimiter is not
  // present
  shell.process_line("{\"execute\":\"show databases\"}");
  EXPECT_THAT(capture, ::testing::HasSubstr("{\"hasData\":true,\"rows\":["));

  current_console()->remove_print_handler(&handler);
  shell.process_line({"{\"execute\":\"\\\\disconnect\"}"});
}

}  // namespace mysqlsh
