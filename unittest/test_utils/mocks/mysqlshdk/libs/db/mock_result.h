/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_RESULT_H_
#define UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_RESULT_H_

#include <string>
#include <vector>

#include "mysqlshdk/libs/db/result.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace testing {
/**
 * Represents a single result within a Mock_result object
 *
 * A result object may contain several inner results sets, i.e.
 * if a stored procedure is called and it performed several select
 * operations.
 *
 * A single result represents the output of an individual select operation.
 *
 * This object is created when a fake result is added into a Mock_result object
 */
class Fake_result {
 public:
  Fake_result(const std::vector<std::string>& names,
              const std::vector<mysqlshdk::db::Type>& types);
  const mysqlshdk::db::IRow* fetch_one();
  std::unique_ptr<mysqlshdk::db::Warning> fetch_one_warning();
  void add_row(const std::vector<std::string>& data);
  void add_warning(const mysqlshdk::db::Warning &warning);

 private:
  size_t _index;
  size_t _windex;

  std::vector<std::string> _names;
  std::vector<mysqlshdk::db::Type> _types;
  std::vector<std::unique_ptr<mysqlshdk::db::IRow> > _records;

  std::vector<std::unique_ptr<mysqlshdk::db::Warning> > _warnings;
};

struct Fake_result_data {
  std::string sql;
  std::vector<std::string> names;
  std::vector<mysqlshdk::db::Type> types;
  std::vector<std::vector<std::string> > rows;
};

/**
 * Mock for a Result object
 *
 * This class supports handling of fake results.
 *
 * Simple call expectations and return values can be defined with:
 *
 * EXPECT_CALL(result, fetch_one());
 *
 * Where:
 *   - First parameter is this result object
 *   - Second parameter is the function and parameters that is expected to be
 * called - After closing the EXPECT_CALL() some actions can be defined to
 * return specific results Keep in mind that the returned data must match the
 * return type of the function called
 *
 * This class allows defining fake resultsets to be returned, it means
 * they are created on the fly by calling: add_result
 */
class Mock_result : public mysqlshdk::db::IResult {
 public:
  Mock_result();

  MOCK_METHOD0(next_resultset, bool());

  // Metadata retrieval
  MOCK_CONST_METHOD0(get_auto_increment_value, int64_t());
  MOCK_METHOD0(has_resultset, bool());
  MOCK_CONST_METHOD0(get_affected_row_count, uint64_t());
  MOCK_CONST_METHOD0(get_fetched_row_count, uint64_t());
  MOCK_CONST_METHOD0(get_warning_count, uint64_t());
  MOCK_CONST_METHOD0(get_execution_time, unsigned long());
  MOCK_CONST_METHOD0(get_info, std::string());

  MOCK_CONST_METHOD0(get_metadata, std::vector<mysqlshdk::db::Column>&());

  virtual const mysqlshdk::db::IRow* fetch_one();
  virtual std::unique_ptr<mysqlshdk::db::Warning> fetch_one_warning();

  virtual ~Mock_result() {}

  void add_result(const std::vector<std::string>& names,
                  const std::vector<mysqlshdk::db::Type>& types,
                  const std::vector<std::vector<std::string> >& rows);

  void set_data(const std::vector<Fake_result_data>& data);

 private:
  size_t _index;
  std::vector<std::unique_ptr<Fake_result> > _results;

  const mysqlshdk::db::IRow* fake_fetch_one();
  bool fake_next_resultset();
};
}  // namespace testing

#endif  // UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_RESULT_H_
