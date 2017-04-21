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

#include "unittest/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/mocks/mysqlshdk/libs/db/mock_row.h"
#include "utils/utils_general.h"

namespace testing {
Fake_result::Fake_result(const std::vector<std::string>& names,
                         const std::vector<mysqlshdk::db::Type>& types)
    : _index(0), _windex(0), _names(names), _types(types) {
  _wnames = {"code", "level", "message"};
  _wtypes = {mysqlshdk::db::Type::Int24, mysqlshdk::db::Type::String,
             mysqlshdk::db::Type::String};
}

std::unique_ptr<mysqlshdk::db::IRow> Fake_result::fetch_one() {
  std::unique_ptr<mysqlshdk::db::IRow> ret_val;

  if (_index < _records.size())
    ret_val.reset(_records[_index++].release());

  return ret_val;
}

std::unique_ptr<mysqlshdk::db::IRow> Fake_result::fetch_one_warning() {
  std::unique_ptr<mysqlshdk::db::IRow> ret_val;

  if (_windex < _wrecords.size())
    ret_val.reset(_wrecords[_windex++].release());

  return ret_val;
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

void Fake_result::add_warning(const std::string& message, int code,
                              const std::string& level) {
  // TODO(rennox): Warning handling is divided atm
  // mysql returns the result of a query but
  // probably we should define a warning object (at least at low level)
  std::unique_ptr<NiceMock<Mock_row>> row(new NiceMock<Mock_row>());
  row->init(_wnames, _wtypes, {std::to_string(code), level, message});
  _wrecords.push_back(std::move(row));
}

Mock_result::Mock_result() : _index(0) {
  ON_CALL(*this, next_data_set())
      .WillByDefault(Invoke(this, &Mock_result::fake_next_dataset));
}

std::unique_ptr<mysqlshdk::db::IRow> Mock_result::fetch_one() {
  std::unique_ptr<mysqlshdk::db::IRow> ret_val;

  if (_index < _results.size())
    ret_val = _results[_index]->fetch_one();

  return ret_val;
}

std::unique_ptr<mysqlshdk::db::IRow> Mock_result::fetch_one_warning() {
  std::unique_ptr<mysqlshdk::db::IRow> ret_val;

  if (_index < _results.size())
    ret_val = _results[_index]->fetch_one_warning();

  return ret_val;
}

bool Mock_result::fake_next_dataset() {
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
