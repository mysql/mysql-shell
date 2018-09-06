/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/mutable_result.h"
#include <utility>
#include "mysqlshdk/libs/db/row_copy.h"
#include "mysqlshdk/libs/db/utils/diff.h"

namespace mysqlshdk {
namespace db {

Mutable_result::Mutable_result() {}

Mutable_result::Mutable_result(const std::vector<Type> &types) {
  for (size_t i = 0; i < types.size(); ++i) {
    add_column(Column("", "dummy", "generic", "", "A" + std::to_string(i), "",
                      0, 0, types[i], 63, false, false, false));
  }
}

Mutable_result::Mutable_result(IResult *result)
    : _metadata(result->get_metadata()) {
  auto *row = result->fetch_one();
  while (row) {
    _rows.emplace_back(std::unique_ptr<Row_copy>(new Row_copy(*row)));
    row = result->fetch_one();
  }
}

Mutable_result::~Mutable_result() {}

void Mutable_result::add_column(const Column &column, size_t offset) {
  if (offset > _metadata.size())
    throw std::invalid_argument("Attempt to add column past table size");

  _metadata.insert(_metadata.begin() + offset, column);
  for (auto &row : _rows) row->add_field(column.get_type(), offset);
}

void Mutable_result::add_column(const Column &column) {
  _metadata.push_back(column);
  for (auto &row : _rows)
    row->add_field(column.get_type(), _metadata.size() - 1);
}

void Mutable_result::add_row(std::unique_ptr<Mem_row> row) {
  if (_metadata.size() != row->num_fields())
    throw std::invalid_argument("row has wrong number of fields");
  _rows.emplace_back(std::move(row));
}

const IRow *Mutable_result::fetch_one() {
  if (_rows.size() > _fetched_row_count)
    return _rows[_fetched_row_count++].get();
  return nullptr;
}

void Mutable_result::add_from(IResult *result) {
  _metadata = result->get_metadata();
  auto *row = result->fetch_one();
  while (row) {
    _rows.emplace_back(new Row_copy(*row));
    row = result->fetch_one();
  }
}

bool operator==(const Mutable_result &left, const Mutable_result &right) {
  if (left._metadata != right._metadata) return false;
  if (left._rows.size() != right._rows.size()) return false;
  for (std::size_t i = 0; i < left._rows.size(); i++)
    if (compare(*left._rows[i], *right._rows[i]) != 0) return false;
  return true;
}

bool operator!=(const Mutable_result &left, const Mutable_result &right) {
  return !(left == right);
}

}  // namespace db
}  // namespace mysqlshdk
