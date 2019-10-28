/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <vector>

#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_row.h"
#include "utils/utils_general.h"

namespace testing {
Fake_result::Fake_result(const std::vector<std::string> &names,
                         const std::vector<mysqlshdk::db::Type> &types)
    : _index(0), _windex(0), _names(names), _types(types) {
  m_field_names.reset(
      new mysqlshdk::db::Field_names(std::vector<mysqlshdk::db::Column>()));
  for (const auto &n : names) {
    m_field_names->add(n);
  }
}

const mysqlshdk::db::IRow *Fake_result::fetch_one() {
  if (_index < _records.size()) return _records[_index++].get();

  return nullptr;
}

std::unique_ptr<mysqlshdk::db::Warning> Fake_result::fetch_one_warning() {
  if (_windex < _warnings.size()) return std::move(_warnings[_windex++]);

  return {};
}

/**
 * Inserts a row on this fake result.
 * @param data The values for each field on this result.
 *
 * A row is defined with a string with the format of:
 *
 * "colname1|colname2|colname3|...|colnameN"
 *
 * The number of elements must match the number of names/types
 * passed to this object on creation.
 *
 * Each element should be a valid representation of the type at the
 * same position.
 */
void Fake_result::add_row(const std::vector<std::string> &data) {
  auto row = std::make_unique<NiceMock<Mock_row>>();

  row->init(_names, _types, data);

  _records.push_back(std::move(row));
}

void Fake_result::add_warning(const mysqlshdk::db::Warning &warning) {
  _warnings.push_back(std::make_unique<mysqlshdk::db::Warning>(warning));
}

Mock_result::Mock_result() : _index(0) {
  ON_CALL(*this, next_resultset())
      .WillByDefault(Invoke(this, &Mock_result::fake_next_resultset));
}

std::shared_ptr<mysqlshdk::db::Field_names> Mock_result::field_names() const {
  if (_index < _results.size()) return _results[_index]->field_names();

  return {};
}

const mysqlshdk::db::IRow *Mock_result::fetch_one() {
  if (_index < _results.size()) return _results[_index]->fetch_one();

  return nullptr;
}

std::unique_ptr<mysqlshdk::db::Warning> Mock_result::fetch_one_warning() {
  if (_index < _results.size()) return _results[_index]->fetch_one_warning();

  return {};
}

bool Mock_result::fake_next_resultset() {
  if (_index < _results.size()) _index++;

  return (_index < _results.size());
}

/**
 * Inserts a new fake result into the Mock_result object.
 * @param names a formatted string with the column names to be included on the
 * result.
 * @param types a vector with int identifiers of the data types on each column
 * @param rows a vector of string vectors that contain the values to be added
 * for each column in the row.
 * The names string should have the next format
 *
 * "colname1|colname2|colname3|...|colnameN"
 *
 * Call add_row on the returned object to insert records into the resultset.
 */
void Mock_result::add_result(
    const std::vector<std::string> &names,
    const std::vector<mysqlshdk::db::Type> &types,
    const std::vector<std::vector<std::string>> &rows) {
  auto result = std::make_unique<Fake_result>(names, types);

  for (auto row : rows) result->add_row(row);

  _results.push_back(std::move(result));
}

void Mock_result::set_data(const std::vector<Fake_result_data> &data) {
  for (auto result : data) {
    std::unique_ptr<Fake_result> fake_result(
        new Fake_result(result.names, result.types));

    for (auto row : result.rows) fake_result->add_row(row);

    _results.push_back(std::move(fake_result));
  }
}

}  // namespace testing
