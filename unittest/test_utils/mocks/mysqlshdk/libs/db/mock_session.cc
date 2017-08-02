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

#include <memory>
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace testing {

Mock_session& Mock_session::expect_query(const std::string& query) {
  _last_query = _queries.size();
  _queries.push_back(query);
  _throws.push_back(false);

  return *this;
}

void Mock_session::then_return(const std::vector<Fake_result_data>& data) {
  if (_last_query < _queries.size()) {
    auto result = std::shared_ptr<Mock_result>(new Mock_result());

    result->set_data(data);

    _results[_queries[_last_query++]] = result;
  } else {
    throw std::logic_error("Attempted to set result with no query.");
  }
}

void Mock_session::then_throw() {
  if (_last_query < _queries.size())
    _throws[_last_query++] = true;
  else
    throw std::logic_error("Attempted to set throw condition with no query.");
}

std::shared_ptr<mysqlshdk::db::IResult> Mock_session::query(
    const std::string& sql, bool buffered) {
  // Ensures the expected query got received
  EXPECT_EQ(sql, _queries[0]);

  // Removes the query
  _queries.erase(_queries.begin());

  // Throws if that's the plan
  if (_throws[0]) {
    _throws.erase(_throws.begin());
    throw std::runtime_error("Error executing session.query");
  }

  // Returns the assigned result if that's the plan
  return _results[sql];
}

}  // namespace testing
