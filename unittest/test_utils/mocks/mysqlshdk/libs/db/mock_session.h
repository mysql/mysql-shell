/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
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

#ifndef UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_SESSION_H_
#define UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_SESSION_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"

/**
 * Sometimes the generated queries vary from the expected ones in the order in
 * which some literals are places in the query. (i.e. when using unordered_sets
 * to hold the list of literals). The order depends on the platform dependent
 * hash function used to store the data, on cases like this a callback can be
 * used to normalize the values before the actual query is compared with the
 * expected one.
 */
using Query_expect =
    std::pair<std::string,
              std::function<std::string(const std::string &processor)>>;

namespace testing {
/**
 * Mock for a Session object
 *
 * Simple call expectations and return values can be defined with:
 *
 * auto sresult = result.add_result("@@server_id", {MYSQL_TYPE_INT24});
 * auto row = sresult->add_row("1");
 * EXPECT_CALL(session, query("SELECT @@server_id",
 * false)).WillOnce(Return(&result));
 *
 * Where:
 *   - First parameter is this session object
 *   - Second parameter is the function and parameters that is expected to be
 * called - After closing the EXPECT_CALL() some actions can be defined to
 * return specific results Keep in mind that the returned data must match the
 * return type of the function called
 *
 * Look at Mock_result to see how to create a fake result to be returned.
 */
class Mock_session_common {
 public:
  std::shared_ptr<mysqlshdk::db::IResult> do_querys(const char *, size_t,
                                                    bool buffered);

  void do_expect_query(Query_expect expect);
  void then_return(const std::vector<Fake_result_data> &data);
  Fake_result &then(const std::vector<std::string> &names,
                    const std::vector<mysqlshdk::db::Type> &types = {});
  void then_throw(const char *what, int code, const char *sqlstate = nullptr);

  void set_query_handler(
      const std::function<std::shared_ptr<mysqlshdk::db::IResult>(
          const std::string &sql)> &handler) {
    m_query_handler = handler;
  }

  const std::vector<Query_expect> &queries() const { return m_queries; }

 protected:
  size_t m_last_query;
  std::vector<Query_expect> m_queries;
  std::unordered_map<std::string, std::shared_ptr<mysqlshdk::db::IResult>>
      m_results;
  std::vector<std::unique_ptr<mysqlshdk::db::Error>> m_throws;

  std::function<std::shared_ptr<mysqlshdk::db::IResult>(const std::string &)>
      m_query_handler;
};

class Mock_session : public mysqlshdk::db::ISession,
                     public Mock_session_common {
 public:
  Mock_session();
  MOCK_METHOD1(
      do_connect,
      void(const mysqlshdk::db::Connection_options &connection_options));
  // Execution
  std::shared_ptr<mysqlshdk::db::IResult> querys(
      const char *sql, size_t len, bool buffered,
      [[maybe_unused]] const std::vector<mysqlshdk::db::Query_attribute>
          &query_attributes = {}) override {
    return Mock_session_common::do_querys(sql, len, buffered);
  }

  std::shared_ptr<mysqlshdk::db::IResult> query_udf(std::string_view sql,
                                                    bool buffered) override {
    return Mock_session_common::do_querys(sql.data(), sql.size(), buffered);
  }

  bool no_backslash_escapes_enabled() override { return false; }

  MOCK_METHOD2(executes, void(const char *, size_t));
  MOCK_METHOD1(execute, void(const std::string &));
  MOCK_METHOD0(start_transaction, void());
  MOCK_METHOD0(commit, void());
  MOCK_METHOD0(rollback, void());
  MOCK_CONST_METHOD0(get_connection_id, uint64_t());
  MOCK_CONST_METHOD0(get_ssl_cipher, const char *());
  MOCK_CONST_METHOD0(get_connection_options,
                     const mysqlshdk::db::Connection_options &());
  MOCK_CONST_METHOD0(is_open, bool());
  MOCK_CONST_METHOD0(get_server_version, mysqlshdk::utils::Version());

  MOCK_CONST_METHOD0(get_socket_fd, socket_t());

  // Error handling
  MOCK_CONST_METHOD0(get_last_error, mysqlshdk::db::Error *());

  MOCK_CONST_METHOD0(get_server_status, uint32_t());

  /**
   * Adds query expectation, which will be compared verbatim woth the actual
   * query received.
   */
  Mock_session &expect_query(const std::string &query) {
    Mock_session_common::do_expect_query({query, nullptr});
    return *this;
  }

  /**
   * Adds query expectation, including a normalization callback that will be
   * applied to both the expected query and the actual query before they are
   * verified for equality.
   */
  Mock_session &expect_query(Query_expect expect) {
    Mock_session_common::do_expect_query(std::move(expect));
    return *this;
  }
  // Disconnection
  MOCK_METHOD0(do_close, void());

  void expect_query(const Fake_result_data &data) {
    expect_query(data.sql).then_return({data});
  }

  std::string escape_string(std::string_view s) const override;
};
}  // namespace testing

#endif  // UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_SESSION_H_
