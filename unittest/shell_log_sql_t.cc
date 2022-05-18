/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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

#include <array>
#include <fstream>
#include <string>
#include <tuple>

#include "mysqlshdk/include/scripting/common.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace {
int grep_count(const std::string &path, const std::string &pattern) {
  std::ifstream fh(path);
  if (!fh.good()) {
    throw std::runtime_error("grep error: " + path + ": " + strerror(errno));
  }
  int result = 0;
  const std::string glob_pattern = "*" + pattern + "*";
  std::string line;
  while (!fh.eof()) {
    std::getline(fh, line);
    if (shcore::match_glob(glob_pattern, line)) {
      result++;
    }
  }
  fh.close();
  return result;
}

char sql_ok[] = "select 1;";
char sql_error[] = "select asdf;";
char sql_filter[] = "show status;";
char sql_mask[] = "select /*((*/'secret'/*))*/;";
char sql_set[] = "set @mysecretvar = /*((*/ 42 /*))*/;";
char sql_filter_error[] = "showasdf;";

}  // namespace

struct Log_sql_test_params {
  std::string context_name;
  int dba_log_level;          // (0),1,2
  std::string log_sql_level;  // off,(error),on,unfiltered
  std::array<int, 11> log_count_mask;
};

std::ostream &operator<<(std::ostream &os, const Log_sql_test_params &p) {
  os << "Log_sql_test_params(context_name=" << p.context_name
     << ", dba_log=" << p.dba_log_level << ", log_sql=" << p.log_sql_level
     << ")";
  return os;
}

namespace shcore {
class Shell_log_sql_parameterized_tests
    : public Shell_core_test_wrapper,
      public ::testing::WithParamInterface<
          std::tuple<Log_sql_test_params, std::string>> {
  struct Test_query {
    std::string query_string;
    std::vector<std::string> expect_log_message;
    bool throws;
  };

 protected:
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();

    m_log_path =
        shcore::path::join_path(shcore::get_user_config_path(), "mysqlsh.log");
    m_logger = shcore::Logger::create_instance(m_log_path.c_str(), false,
                                               shcore::Logger::LOG_DEBUG);
    m_log_sql = std::make_shared<shcore::Log_sql>(*_opts);

    m_queries.emplace_back(
        Test_query{sql_ok, {std::string("SQL: ") + sql_ok}, false});
    m_queries.emplace_back(Test_query{
        sql_error,
        {
            std::string("Error while executing: '") + sql_error +
                "': *Unknown column 'asdf' in 'field list'",
            std::string("SQL: ") + sql_error,
            std::string("Unknown column 'asdf' in 'field list'"),
            std::string("Unknown column 'asdf' in 'field list', SQL: ") +
                sql_error,
        },
        true});
    m_queries.emplace_back(
        Test_query{sql_filter, {std::string("SQL: ") + sql_filter}, false});
    m_queries.emplace_back(
        Test_query{sql_mask, {"SQL: select \\*\\*\\*\\*;"}, false});
    m_queries.emplace_back(
        Test_query{sql_set, {"SQL: set @mysecretvar = \\*\\*\\*\\*;"}, false});
    m_queries.emplace_back(Test_query{
        sql_filter_error,
        {std::string(": SQL: ") + sql_filter_error,
         std::string("You have an error in your SQL syntax; check the manual "
                     "that corresponds to your MySQL server version for the "
                     "right syntax to use near 'showasdf' at line 1, SQL: ") +
             sql_filter_error,
         std::string(
             "You have an error in your SQL syntax; check the manual that "
             "corresponds to your MySQL server version for the right syntax to "
             "use near 'showasdf' at line 1, SQL: <filtered>")},
        true});
  }

  void TearDown() override { delete_file(m_log_path); }

  int grep_log(const std::string &pattern) {
    return grep_count(m_log_path, pattern);
  }

  std::vector<Test_query> m_queries;
  std::string m_log_path;
  std::shared_ptr<shcore::Logger> m_logger;
  std::shared_ptr<shcore::Log_sql> m_log_sql;
};

TEST_P(Shell_log_sql_parameterized_tests, log_sql) {
  mysqlsh::Scoped_logger log(m_logger);
  mysqlsh::Scoped_log_sql log_sql(m_log_sql);

  const auto params = std::get<0>(GetParam());
  const auto proto = std::get<1>(GetParam());

  Log_sql_guard g(params.context_name.c_str());

  // (0),1,2
  mysqlsh::current_shell_options()->set_and_notify(
      "dba.logSql", std::to_string(params.dba_log_level));

  // off,(error),on,unfiltered
  mysqlsh::current_shell_options()->set_and_notify("logSql",
                                                   params.log_sql_level);

  mysqlsh::current_shell_options()->set_and_notify("logSql.ignorePattern",
                                                   "SHOW*:*PASSWORD*");

  auto conn_opts = [&proto]() {
    if (proto == "X") {
      return mysqlshdk::db::Connection_options(_uri);
    }
    return mysqlshdk::db::Connection_options(_mysql_uri);
  }();
  auto session = mysqlsh::establish_session(conn_opts, false);

  for (const auto &q : m_queries) {
    if (q.throws) {
      EXPECT_ANY_THROW({ auto r = session->query(q.query_string); });
    } else {
      EXPECT_NO_THROW({ auto r = session->query(q.query_string); });
    }
  }

  const auto &log_count = params.log_count_mask;
  auto p = log_count.begin();

  for (const auto &q : m_queries) {
    for (const auto &expect : q.expect_log_message) {
      SCOPED_TRACE(expect.c_str());
      EXPECT_EQ(*p++, grep_log(expect));
    }
  }

  ASSERT_EQ(log_count.end(), p);
}

INSTANTIATE_TEST_SUITE_P(
    Shell_log_sql, Shell_log_sql_parameterized_tests,
    ::testing::Combine(
        ::testing::Values(
            Log_sql_test_params{
                "js", 0, "off", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
            Log_sql_test_params{
                "js", 0, "error", {0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0}},
            Log_sql_test_params{
                "js", 0, "on", {1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1}},
            Log_sql_test_params{
                "js", 0, "unfiltered", {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},

            Log_sql_test_params{
                "js", 1, "off", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
            Log_sql_test_params{
                "js", 1, "error", {0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0}},
            Log_sql_test_params{
                "js", 1, "on", {1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1}},
            Log_sql_test_params{
                "js", 1, "unfiltered", {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},

            Log_sql_test_params{
                "js", 2, "off", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
            Log_sql_test_params{
                "js", 2, "error", {0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0}},
            Log_sql_test_params{
                "js", 2, "on", {1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1}},
            Log_sql_test_params{
                "js", 2, "unfiltered", {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},

            Log_sql_test_params{
                "Dba.test", 0, "off", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
            Log_sql_test_params{
                "Dba.test", 0, "error", {0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0}},
            Log_sql_test_params{
                "Dba.test", 0, "on", {1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1}},
            Log_sql_test_params{
                "Dba.test", 0, "unfiltered", {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},

            Log_sql_test_params{
                "Dba.test", 1, "off", {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},
            Log_sql_test_params{
                "Dba.test", 1, "error", {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},
            Log_sql_test_params{
                "Dba.test", 1, "on", {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},
            Log_sql_test_params{
                "Dba.test", 1, "unfiltered", {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},

            Log_sql_test_params{
                "Dba.test", 2, "off", {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},
            Log_sql_test_params{
                "Dba.test", 2, "error", {1, 0, 2, 1, 1, 1, 1, 1, 1, 1, 0}},
            Log_sql_test_params{
                "Dba.test", 2, "on", {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},
            Log_sql_test_params{
                "Dba.test", 2, "unfiltered", {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},

            Log_sql_test_params{"Cluster.test",
                                0,
                                "off",
                                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
            Log_sql_test_params{"Cluster.test",
                                0,
                                "error",
                                {0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0}},
            Log_sql_test_params{"Cluster.test",
                                0,
                                "on",
                                {1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1}},
            Log_sql_test_params{"Cluster.test",
                                0,
                                "unfiltered",
                                {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},

            Log_sql_test_params{"Cluster.test",
                                1,
                                "off",
                                {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},
            Log_sql_test_params{"Cluster.test",
                                1,
                                "error",
                                {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},
            Log_sql_test_params{"Cluster.test",
                                1,
                                "on",
                                {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},
            Log_sql_test_params{"Cluster.test",
                                1,
                                "unfiltered",
                                {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},

            Log_sql_test_params{"Cluster.test",
                                2,
                                "off",
                                {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},
            Log_sql_test_params{"Cluster.test",
                                2,
                                "error",
                                {1, 0, 2, 1, 1, 1, 1, 1, 1, 1, 0}},
            Log_sql_test_params{"Cluster.test",
                                2,
                                "on",
                                {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}},
            Log_sql_test_params{"Cluster.test",
                                2,
                                "unfiltered",
                                {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0}}),
        ::testing::Values("Classic", "X")));

}  // namespace shcore
