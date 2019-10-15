/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_utils.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/shellcore/shell_cli_operation.h"
#include "unittest/test_utils.h"

namespace shcore {

class Shell_cli_operation_test : public Shell_core_test_wrapper,
                                 public Shell_cli_operation {
 public:
  void parse(Options::Cmdline_iterator *iterator) {
    m_object_name.clear();
    m_method_name.clear();
    m_method_name_original.clear();
    m_argument_list.clear();
    m_argument_map.reset();
    Shell_cli_operation::parse(iterator);
  }
};

TEST_F(Shell_cli_operation_test, parse_commandline) {
  const char *arg0[] = {"--"};
  std::unique_ptr<Options::Cmdline_iterator> it(
      new Options::Cmdline_iterator(1, arg0, 1));
  ASSERT_THROW(parse(it.get()), Mapping_error);

  const char *arg01[] = {"dba"};
  it.reset(new Options::Cmdline_iterator(1, arg01, 0));
  ASSERT_THROW(parse(it.get()), Mapping_error);

  const char *arg1[] = {"util",
                        "check-for-server-upgrade",
                        "root@localhost",
                        "0",
                        "1",
                        "true",
                        "false",
                        "123456789",
                        "12345.12345",
                        "-"};
  it.reset(new Options::Cmdline_iterator(10, arg1, 0));
  parse(it.get());
  EXPECT_EQ("util", m_object_name);
  EXPECT_EQ("checkForServerUpgrade", m_method_name);
  EXPECT_EQ(8, m_argument_list.size());
  EXPECT_EQ("root@localhost", m_argument_list.string_at(0));
  EXPECT_FALSE(m_argument_list.bool_at(1));
  EXPECT_TRUE(m_argument_list.bool_at(2));
  EXPECT_TRUE(m_argument_list.bool_at(3));
  EXPECT_FALSE(m_argument_list.bool_at(4));
  EXPECT_EQ(123456789, m_argument_list.int_at(5));
  EXPECT_EQ(12345.12345, m_argument_list.double_at(6));
  EXPECT_EQ(Value::Null(), m_argument_list.at(7));

  const char *arg2[] = {"dba", "deploy-sandbox-instance", "3307",
                        "--password=foo", "--sandbox-dir=-"};
  it.reset(new Options::Cmdline_iterator(5, arg2, 0));
  parse(it.get());
  EXPECT_EQ("dba", m_object_name);
  EXPECT_EQ("deploySandboxInstance", m_method_name);
  EXPECT_EQ(2, m_argument_list.size());
  EXPECT_EQ(3307, m_argument_list.int_at(0));
  EXPECT_NO_THROW(m_argument_list.map_at(1));
  EXPECT_EQ(2, m_argument_map->size());
  EXPECT_EQ("foo", m_argument_map->get_string("password"));
  EXPECT_NO_THROW(
      m_argument_map->at("sandboxDir").check_type(Value_type::Null));

  const char *arg3[] = {"cluster",      "addInstance",      "--port=3307",
                        "--force",      "--bool=false",     "--std-dev=0.7",
                        "--pass-word=", "--sandboxDir=/tmp"};
  it.reset(new Options::Cmdline_iterator(8, arg3, 0));
  parse(it.get());
  EXPECT_EQ("cluster", m_object_name);
  EXPECT_EQ("addInstance", m_method_name);
  EXPECT_EQ(1, m_argument_list.size());
  ASSERT_NO_THROW(m_argument_list.map_at(0));
  EXPECT_EQ(6, m_argument_map->size());
  Argument_map map(*m_argument_map);
  ASSERT_NO_THROW(map.ensure_keys(
      {"port", "force", "bool", "stdDev", "passWord", "sandboxDir"}, {},
      "test1"));
  EXPECT_EQ(3307, map.int_at("port"));
  EXPECT_TRUE(map.bool_at("force"));
  EXPECT_FALSE(map.bool_at("bool"));
  EXPECT_EQ(0.7, map.double_at("stdDev"));
  EXPECT_EQ("", map.string_at("passWord"));
  EXPECT_EQ("/tmp", map.string_at("sandboxDir"));

  int64_t port;
  bool force;
  double std;
  std::string pwd;
  EXPECT_NO_THROW(mysqlsh::Unpack_options(m_argument_map)
                      .required("port", &port)
                      .required("force", &force)
                      .optional("bool", &force)
                      .optional_ci("stddev", &std)
                      .optional_ci("password", &pwd)
                      .optional_exact("sandboxDir", &pwd)
                      .end());

  mysqlsh::Unpack_options up(m_argument_map);
  EXPECT_THROW(up.required("stdDev", &force), Exception);
  EXPECT_THROW(up.optional("sandboxDir", &force), Exception);
}

TEST_F(Shell_cli_operation_test, local_dict) {
  const char *arg1[] = {
      "util", "check-for-server-upgrade", "{", "--host=localhost", "}", "123"};
  std::unique_ptr<Options::Cmdline_iterator> it(
      new Options::Cmdline_iterator(6, arg1, 0));
  EXPECT_NO_THROW(parse(it.get()));
  EXPECT_EQ("util", m_object_name);
  EXPECT_EQ("checkForServerUpgrade", m_method_name);
  EXPECT_EQ(2, m_argument_list.size());
  ASSERT_TRUE(m_argument_list[0].type == Value_type::Map);
  EXPECT_EQ("localhost", m_argument_list.map_at(0)->get_string("host"));
  EXPECT_EQ("123", m_argument_list.string_at(1));

  const char *arg2[] = {
      "util", "check-for-server-upgrade", "{", "--host=localhost", "1", "}",
      "123"};
  it.reset(new Options::Cmdline_iterator(7, arg2, 0));
  EXPECT_THROW(parse(it.get()), Mapping_error);

  const char *arg3[] = {"util", "check-for-server-upgrade", "{",
                        "--host=localhost"};
  it.reset(new Options::Cmdline_iterator(4, arg3, 0));
  EXPECT_THROW(parse(it.get()), Mapping_error);

  const char *arg4[] = {"util",        "check-for-server-upgrade",
                        "--port=3306", "{--host=localhost",
                        "}",           "--lines=123"};
  it.reset(new Options::Cmdline_iterator(6, arg4, 0));
  EXPECT_NO_THROW(parse(it.get()));
  EXPECT_EQ("util", m_object_name);
  EXPECT_EQ("checkForServerUpgrade", m_method_name);
  EXPECT_EQ(2, m_argument_list.size());
  ASSERT_TRUE(m_argument_list[0].type == Value_type::Map);
  EXPECT_EQ("localhost", m_argument_list.map_at(0)->get_string("host"));
  ASSERT_TRUE(m_argument_list[1].type == Value_type::Map);
  EXPECT_EQ("123", m_argument_list.map_at(1)->get_string("lines"));
  EXPECT_EQ(3306, m_argument_list.map_at(1)->get_int("port"));
}

TEST_F(Shell_cli_operation_test, connection_options) {
  const char *arg1[] = {"util",
                        "check-for-server-upgrade",
                        "{",
                        "--sslMode=required",
                        "--compression-algorithms=zstd",
                        "--db-user=root",
                        "--host=localhost",
                        "}",
                        "123"};
  std::unique_ptr<Options::Cmdline_iterator> it(
      new Options::Cmdline_iterator(9, arg1, 0));
  EXPECT_NO_THROW(parse(it.get()));
  EXPECT_EQ("util", m_object_name);
  EXPECT_EQ("checkForServerUpgrade", m_method_name);
  EXPECT_EQ(2, m_argument_list.size());
  ASSERT_TRUE(m_argument_list[0].type == Value_type::Map);
  EXPECT_EQ("localhost", m_argument_list.map_at(0)->get_string("host"));
  EXPECT_EQ("required", m_argument_list.map_at(0)->get_string("ssl-mode"));
  EXPECT_EQ("zstd",
            m_argument_list.map_at(0)->get_string("compression-algorithms"));
  EXPECT_EQ("root", m_argument_list.map_at(0)->get_string("dbUser"));
  EXPECT_EQ("123", m_argument_list.string_at(1));

  const char *arg2[] = {"util",
                        "check-for-server-upgrade",
                        "--port=3306",
                        "{",
                        "--sslMode=required",
                        "--lines=123",
                        "}",
                        "--sslMode=required",
                        "--compression-algorithms=zstd",
                        "--db-user=root"};
  it.reset(new Options::Cmdline_iterator(10, arg2, 0));
  EXPECT_NO_THROW(parse(it.get()));
  EXPECT_EQ("util", m_object_name);
  EXPECT_EQ("checkForServerUpgrade", m_method_name);
  EXPECT_EQ(2, m_argument_list.size());
  ASSERT_TRUE(m_argument_list[0].type == Value_type::Map);
  // not a connection options dictionary
  EXPECT_EQ("required", m_argument_list.map_at(0)->get_string("sslMode"));
  EXPECT_EQ("123", m_argument_list.map_at(0)->get_string("lines"));

  // a connection options dictionary
  ASSERT_TRUE(m_argument_list[1].type == Value_type::Map);
  auto co = m_argument_list.map_at(1);
  EXPECT_EQ(3306, co->get_int("port"));
  EXPECT_EQ("required", co->get_string("ssl-mode"));
  EXPECT_EQ("zstd", co->get_string("compression-algorithms"));
  EXPECT_EQ("root", co->get_string("dbUser"));
}

TEST_F(Shell_cli_operation_test, dba_object) {
  char *arg[] = {const_cast<char *>("ut"), const_cast<char *>("--"),
                 const_cast<char *>("dba"),
                 const_cast<char *>("drop-metadata-schema"),
                 const_cast<char *>("--force")};
  auto options = std::make_shared<mysqlsh::Shell_options>(5, arg);
  mysqlsh::Mysql_shell shell(options, &output_handler.deleg);
  ASSERT_TRUE(options->get_shell_cli_operation() != nullptr);
  EXPECT_THROW_LIKE(options->get_shell_cli_operation()->execute(),
                    shcore::Exception,
                    "Dba.dropMetadataSchema: An open session is required to "
                    "perform this operation");

  char *arg1[] = {const_cast<char *>("ut"), const_cast<char *>("--"),
                  const_cast<char *>("dba"), const_cast<char *>("dummy")};
  options.reset(new mysqlsh::Shell_options(4, arg1));
  mysqlsh::Mysql_shell shell1(options, &output_handler.deleg);
  ASSERT_THROW(options->get_shell_cli_operation()->execute(),
               Shell_cli_operation::Mapping_error);
}

TEST_F(Shell_cli_operation_test, cluster_object) {
  char *arg[] = {const_cast<char *>("ut"), const_cast<char *>("--"),
                 const_cast<char *>("cluster"), const_cast<char *>("status")};
  auto options = std::make_shared<mysqlsh::Shell_options>(4, arg);
  mysqlsh::Mysql_shell shell(options, &output_handler.deleg);
  ASSERT_TRUE(options->get_shell_cli_operation() != nullptr);
  EXPECT_THROW_LIKE(options->get_shell_cli_operation()->execute(),
                    shcore::Exception,
                    "An open session is required to perform this operation.");
}

TEST_F(Shell_cli_operation_test, shell_object) {
  char *arg[] = {const_cast<char *>("ut"), const_cast<char *>("--"),
                 const_cast<char *>("shell"), const_cast<char *>("status")};
  auto options = std::make_shared<mysqlsh::Shell_options>(4, arg);
  mysqlsh::Mysql_shell shell(options, &output_handler.deleg);
  ASSERT_TRUE(options->get_shell_cli_operation() != nullptr);
  ASSERT_NO_THROW(options->get_shell_cli_operation()->execute());
  EXPECT_NE(std::string::npos,
            output_handler.std_out.find("MySQL Shell version"));
  EXPECT_NE(std::string::npos, output_handler.std_err.find("Not Connected"));

  char *arg1[] = {const_cast<char *>("ut"), const_cast<char *>("--"),
                  const_cast<char *>("shell"), const_cast<char *>("dummy")};
  options.reset(new mysqlsh::Shell_options(4, arg1));
  mysqlsh::Mysql_shell shell1(options, &output_handler.deleg);
  ASSERT_THROW(options->get_shell_cli_operation()->execute(),
               Shell_cli_operation::Mapping_error);
}

TEST_F(Shell_cli_operation_test, shell_options_object) {
  char *arg[] = {const_cast<char *>("ut"),
                 const_cast<char *>("--"),
                 const_cast<char *>("shell.options"),
                 const_cast<char *>("set"),
                 const_cast<char *>("resultFormat"),
                 const_cast<char *>("table")};
  auto options = std::make_shared<mysqlsh::Shell_options>(6, arg);
  mysqlsh::Mysql_shell shell(options, &output_handler.deleg);
  ASSERT_TRUE(options->get_shell_cli_operation() != nullptr);
  ASSERT_NO_THROW(options->get_shell_cli_operation()->execute());
  EXPECT_TRUE(output_handler.std_err.empty());

  char *arg1[] = {const_cast<char *>("ut"), const_cast<char *>("--"),
                  const_cast<char *>("shell.options"),
                  const_cast<char *>("dummy")};
  options.reset(new mysqlsh::Shell_options(4, arg1));
  mysqlsh::Mysql_shell shell1(options, &output_handler.deleg);
  ASSERT_THROW(options->get_shell_cli_operation()->execute(),
               Shell_cli_operation::Mapping_error);
}

TEST_F(Shell_cli_operation_test, util_object) {
  char *arg[] = {const_cast<char *>("ut"), const_cast<char *>("--"),
                 const_cast<char *>("util"),
                 const_cast<char *>("check-for-server-upgrade")};
  auto options = std::make_shared<mysqlsh::Shell_options>(4, arg);
  mysqlsh::Mysql_shell shell(options, &output_handler.deleg);
  ASSERT_TRUE(options->get_shell_cli_operation() != nullptr);
  EXPECT_THROW_LIKE(options->get_shell_cli_operation()->execute(),
                    shcore::Exception, "Util.checkForServerUpgrade");

  char *arg1[] = {const_cast<char *>("ut"), const_cast<char *>("--"),
                  const_cast<char *>("util"), const_cast<char *>("dummy")};
  options.reset(new mysqlsh::Shell_options(4, arg1));
  mysqlsh::Mysql_shell shell1(options, &output_handler.deleg);
  ASSERT_THROW(options->get_shell_cli_operation()->execute(),
               Shell_cli_operation::Mapping_error);
}

#define MY_EXPECT_EQ_OR_DUMP(a, b)          \
  {                                         \
    const auto &av = a;                     \
    const auto &bv = b;                     \
    if (av != bv) {                         \
      puts(output_handler.std_out.c_str()); \
      puts(output_handler.std_err.c_str()); \
    }                                       \
    EXPECT_EQ(av, bv);                      \
  }

#define MY_ASSERT_EQ_OR_DUMP(a, b)          \
  {                                         \
    const auto &av = a;                     \
    const auto &bv = b;                     \
    if (av != bv) {                         \
      puts(output_handler.std_out.c_str()); \
      puts(output_handler.std_err.c_str()); \
    }                                       \
    ASSERT_EQ(av, bv);                      \
  }

TEST_F(Shell_cli_operation_test, integration_test) {
  SKIP_UNLESS_DIRECT_MODE();

  std::vector<std::string> env{"MYSQLSH_TERM_COLOR_MODE=nocolor"};
  MY_EXPECT_EQ_OR_DUMP(
      0, testutil->call_mysqlsh_c({"--", "shell", "status"}, "", env));
  MY_EXPECT_STDOUT_CONTAINS("MySQL Shell version");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(
      0, testutil->call_mysqlsh_c(
             {"--", "shell.options", "set-persist", "defaultMode", "py"}, "",
             env));
  EXPECT_TRUE(output_handler.std_err.empty() && output_handler.std_out.empty());
  MY_EXPECT_EQ_OR_DUMP(
      0, testutil->call_mysqlsh_c(
             {"--", "shell.options", "unset-persist", "defaultMode"}, "", env));
  EXPECT_TRUE(output_handler.std_err.empty() && output_handler.std_out.empty());

#ifndef _MSC_VER
  MY_ASSERT_EQ_OR_DUMP(
      0,
      testutil->call_mysqlsh_c(
          {"--", "dba", "deploySandboxInstance",
           std::to_string(_mysql_sandbox_ports[0]).c_str(),
           ("--portx=" + std::to_string(_mysql_sandbox_ports[0]) + "0").c_str(),
           "--password=abc", "--allow-root-from=%"},
          "", env));
  MY_EXPECT_STDOUT_CONTAINS("successfully deployed and started");
  output_handler.wipe_all();

  std::string uri =
      "root:abc@localhost:" + std::to_string(_mysql_sandbox_ports[0]);

  EXPECT_NE(10, testutil->call_mysqlsh_c(
                    {"--", "util", "check-for-server-upgrade", uri.c_str()}, "",
                    env));
  MY_EXPECT_STDOUT_CONTAINS("will now be checked for compatibility issues");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(
      0,
      testutil->call_mysqlsh_c(
          {uri.c_str(), "--", "dba", "create-cluster", "cluster_X"}, "", env));
  MY_EXPECT_STDOUT_CONTAINS("Cluster successfully created");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(
      0, testutil->call_mysqlsh_c({uri.c_str(), "--", "cluster", "status"}, "",
                                  env));
  MY_EXPECT_STDOUT_CONTAINS("\"clusterName\": \"cluster_X\"");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(0, testutil->call_mysqlsh_c(
                              {"--", "dba", "stopSandboxInstance",
                               std::to_string(_mysql_sandbox_ports[0]).c_str(),
                               "--password=abc"},
                              "", env));
  MY_EXPECT_STDOUT_CONTAINS("successfully stopped");
  output_handler.wipe_all();

  MY_EXPECT_EQ_OR_DUMP(0, testutil->call_mysqlsh_c(
                              {"--", "dba", "deleteSandboxInstance",
                               std::to_string(_mysql_sandbox_ports[0]).c_str()},
                              "", env));
  MY_EXPECT_STDOUT_CONTAINS("successfully deleted");
  output_handler.wipe_all();
#endif
}

}  // namespace shcore
