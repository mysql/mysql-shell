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

#include <string>
#include <vector>

#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_row.h"
#include "utils/utils_general.h"

namespace testing {
Fake_result::Fake_result(const std::vector<std::string>& names,
                         const std::vector<mysqlshdk::db::Type>& types)
    : _index(0), _windex(0), _names(names), _types(types) {
}

const mysqlshdk::db::IRow* Fake_result::fetch_one() {
  if (_index < _records.size())
    return _records[_index++].get();

  return nullptr;
}

std::unique_ptr<mysqlshdk::db::Warning> Fake_result::fetch_one_warning() {
  if (_windex < _warnings.size())
    return std::move(_warnings[_windex++]);

  return {};
}

/**
 * Inserts a row on this fake result.
 * @param a formatted string with the colnames of each column.
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
void Fake_result::add_row(const std::vector<std::string>& data) {
  std::unique_ptr<NiceMock<Mock_row>> row(new NiceMock<Mock_row>());

  row->init(_names, _types, data);

  _records.push_back(std::move(row));
}

void Fake_result::add_warning(const mysqlshdk::db::Warning &warning) {
  _warnings.push_back(std::unique_ptr<mysqlshdk::db::Warning>(
      new mysqlshdk::db::Warning(warning)));
}

Mock_result::Mock_result() : _index(0) {
  ON_CALL(*this, next_resultset())
      .WillByDefault(Invoke(this, &Mock_result::fake_next_resultset));
}

const mysqlshdk::db::IRow* Mock_result::fetch_one() {
  if (_index < _results.size())
    return _results[_index]->fetch_one();

  return nullptr;
}

std::unique_ptr<mysqlshdk::db::Warning> Mock_result::fetch_one_warning() {
  if (_index < _results.size())
    return _results[_index]->fetch_one_warning();

  return {};
}

bool Mock_result::fake_next_resultset() {
  if (_index < _results.size())
    _index++;

  return (_index < _results.size());
}

/**
 * Inserts a new fake result into the Mock_result object.
 * @param names a formatted string with the column names to be included on the
 * result.
 * @param types a vector with int identifiers of the data types on each column
 *
 * The names string should have the next format
 *
 * "colname1|colname2|colname3|...|colnameN"
 *
 * Call add_row on the returned object to insert records into the resultset.
 */
void Mock_result::add_result(
    const std::vector<std::string>& names,
    const std::vector<mysqlshdk::db::Type>& types,
    const std::vector<std::vector<std::string>>& rows) {
  std::unique_ptr<Fake_result> result(new Fake_result(names, types));

  for (auto row : rows)
    result->add_row(row);

  _results.push_back(std::move(result));
}

void Mock_result::set_data(const std::vector<Fake_result_data>& data) {
  for (auto result : data) {
    std::unique_ptr<Fake_result> fake_result(
        new Fake_result(result.names, result.types));

    for (auto row : result.rows)
      fake_result->add_row(row);

    _results.push_back(std::move(fake_result));
  }
}

}  // namespace testing
