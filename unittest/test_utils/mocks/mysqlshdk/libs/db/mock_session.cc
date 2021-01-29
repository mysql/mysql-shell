/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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

#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include <mysql.h>
#include <memory>
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"

namespace testing {
Mock_session::Mock_session() {
  ON_CALL(*this, is_open()).WillByDefault(Return(true));
}

void Mock_session_common::do_expect_query(const std::string &query) {
  _last_query = _queries.size();
  _queries.push_back(query);
  _throws.push_back(false);
}

std::string Mock_session::escape_string(const std::string &s) const {
  std::string res;
  res.resize(s.length() * 2 + 1);
  size_t l = mysql_escape_string(&res[0], s.data(), s.length());
  res.resize(l);
  return res;
}

std::string Mock_session::escape_string(const char *buffer, size_t len) const {
  std::string res;
  res.resize(len * 2 + 1);
  size_t l = mysql_escape_string(&res[0], buffer, len);
  res.resize(l);
  return res;
}

void Mock_session_common::then_return(
    const std::vector<Fake_result_data> &data) {
  if (_last_query < _queries.size()) {
    auto result = std::shared_ptr<Mock_result>(new Mock_result());

    result->set_data(data);

    _results[_queries[_last_query++]] = result;
  } else {
    throw std::logic_error("Attempted to set result with no query.");
  }
}

Fake_result &Mock_session_common::then(
    const std::vector<std::string> &names,
    const std::vector<mysqlshdk::db::Type> &types) {
  if (_last_query < _queries.size()) {
    auto result = std::shared_ptr<Mock_result>(new Mock_result());

    _results[_queries[_last_query++]] = result;

    if (types.empty()) {
      return result->add_result(
          names, std::vector<mysqlshdk::db::Type>(names.size(),
                                                  mysqlshdk::db::Type::String));
    } else {
      return result->add_result(names, types);
    }
  } else {
    throw std::logic_error("Attempted to set result with no query.");
  }
}

void Mock_session_common::then_throw() {
  if (_last_query < _queries.size())
    _throws[_last_query++] = true;
  else
    throw std::logic_error("Attempted to set throw condition with no query.");
}

std::shared_ptr<mysqlshdk::db::IResult> Mock_session_common::do_querys(
    const char *sql, size_t length, bool /*buffered*/) {
  std::string s(sql, length);

  if (m_query_handler) {
    auto res = m_query_handler(s);
    if (res) return res;
  }

  if (_queries.empty()) throw std::logic_error("Unexpected query: " + s);

  // Ensures the expected query got received
  EXPECT_EQ(s, _queries[0]);

  // Removes the query
  _queries.erase(_queries.begin());

  // Throws if that's the plan
  if (_throws[0]) {
    _throws.erase(_throws.begin());
    throw std::runtime_error("Error executing session.query");
  }

  // Returns the assigned result if that's the plan
  return _results[s];
}

}  // namespace testing
