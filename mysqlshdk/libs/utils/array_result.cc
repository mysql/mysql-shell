/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/utils/array_result.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace shcore {

// minimal implementation of a cursor over an array in order to reuse
// Resultset_dumper
Array_as_result::Array_as_result(const shcore::Array_t &array) {
  if (array->size() < 1) {
    throw std::logic_error("Expecting at least one row.");
  }

  for (const auto &row : *array) {
    if (row.type != shcore::Value_type::Array) {
      throw std::logic_error("Expecting a list of lists.");
    }

    m_data.emplace_back();

    for (const auto &value : *row.as_array()) {
      bool is_null = value.type == shcore::Value_type::Undefined ||
                     value.type == shcore::Value_type::Null;
      m_data.back().emplace_back(is_null ? "NULL" : value.descr());
    }
  }

  for (const auto &column : m_data[0]) {
    m_metadata.emplace_back(
        "unknown",  // catalog will not be used by dumper
        "unknown",  // schema will not be used by dumper
        "unknown",  // table name will not be used by dumper
        "unknown",  // table label will not be used by dumper
        column,     // column name
        column,     // column label
        1,  // length will not be used by dumper if zero-fill is set to false
        1,  // fractional digits will not be used by dumper
        mysqlshdk::db::Type::String,  // type
        1,                            // collation ID will not be used by dumper
        false,                        // unsigned
        false,                        // zero-fill
        false);                       // binary will not be used by dumper
  }
}

const mysqlshdk::db::IRow *Array_as_result::fetch_one() {
  if (!has_resultset()) {
    return nullptr;
  }

  if (m_current_row < m_data.size()) {
    m_row = std::make_unique<Vector_as_row>(m_data[m_current_row]);
    ++m_current_row;
    return m_row.get();
  } else {
    return nullptr;
  }
}

bool Array_as_result::next_resultset() {
  m_has_result = false;
  return has_resultset();
}

bool Array_as_result::has_resultset() {
  return static_cast<const Array_as_result *>(this)->has_resultset();
}

const std::vector<mysqlshdk::db::Column> &Array_as_result::get_metadata()
    const {
  if (!has_resultset()) {
    throw std::logic_error("No result, unable to fetch metadata");
  }

  return m_metadata;
}

std::shared_ptr<mysqlshdk::db::Field_names> Array_as_result::field_names()
    const {
  return std::make_shared<mysqlshdk::db::Field_names>(m_metadata);
}

void Array_as_result::buffer() {
  // data is always buffered
}

void Array_as_result::rewind() {
  m_has_result = true;
  m_current_row = 1;
}

}  // namespace shcore
