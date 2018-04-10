/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_ROW_BY_NAME_H_
#define MYSQLSHDK_LIBS_DB_ROW_BY_NAME_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/row.h"
#include "mysqlshdk/libs/db/row_copy.h"

namespace mysqlshdk {
namespace db {

class Row_ref_by_name {
 public:
  Row_ref_by_name() {}

  Row_ref_by_name(std::shared_ptr<IResult> result, const IRow *row)
      : Row_ref_by_name(result) {
    _row_ref = row;
  }

  explicit Row_ref_by_name(Row_ref_by_name &&p)
      : _field_names(std::move(p._field_names)),
        _row_ref(std::move(p._row_ref)) {}

  virtual ~Row_ref_by_name() {}

  void operator=(const IRow *row) { _row_ref = row; }

  Row_ref_by_name &operator=(Row_ref_by_name &&o) {
    _field_names = std::move(o._field_names);
    _row_ref = std::move(o._row_ref);
    return *this;
  }

  uint32_t num_fields() const { return _row_ref->num_fields(); }

  Type get_type(const std::string &field) const {
    return _row_ref->get_type(field_index(field));
  }

  bool is_null(const std::string &field) const {
    return _row_ref->is_null(field_index(field));
  }

  std::string get_as_string(const std::string &field) const {
    return _row_ref->get_as_string(field_index(field));
  }

  std::string get_string(const std::string &field) const {
    return _row_ref->get_string(field_index(field));
  }

  int64_t get_int(const std::string &field) const {
    return _row_ref->get_int(field_index(field));
  }

  uint64_t get_uint(const std::string &field) const {
    return _row_ref->get_uint(field_index(field));
  }

  float get_float(const std::string &field) const {
    return _row_ref->get_float(field_index(field));
  }

  double get_double(const std::string &field) const {
    return _row_ref->get_double(field_index(field));
  }

  std::pair<const char *, size_t> get_string_data(
      const std::string &field) const {
    return _row_ref->get_string_data(field_index(field));
  }

  uint64_t get_bit(const std::string &field) const {
    return _row_ref->get_bit(field_index(field));
  }

 protected:
  explicit Row_ref_by_name(std::shared_ptr<IResult> result)
      : _row_ref(nullptr) {
    const auto &columns = result->get_metadata();
    uint32_t i = 0;
    for (const auto &col : columns) {
      _field_names[col.get_column_label()] = i++;
    }
  }

  uint32_t field_index(const std::string &field) const {
    auto it = _field_names.find(field);
    if (it == _field_names.end())
      throw std::invalid_argument("invalid field name " + field);
    return it->second;
  }

  std::map<std::string, uint32_t> _field_names;
  const IRow *_row_ref = nullptr;
};

class Row_by_name : public Row_ref_by_name {
 public:
  Row_by_name() {}

  Row_by_name(std::shared_ptr<IResult> result, const IRow &row)
      : Row_ref_by_name(result), _row_copy(row) {
    _row_ref = &_row_copy;
  }

  Row_by_name(std::shared_ptr<IResult> result, Row_copy &&row_copy)
      : Row_ref_by_name(result), _row_copy(std::move(row_copy)) {
    _row_ref = &_row_copy;
  }

  explicit Row_by_name(Row_by_name &&rbn)
      : Row_ref_by_name(std::move(rbn)), _row_copy(std::move(rbn._row_copy)) {
    _row_ref = &_row_copy;
  }

  Row_by_name &operator=(Row_by_name &&o) {
    Row_ref_by_name::operator=(std::move(o));
    _row_copy = std::move(o._row_copy);
    _row_ref = &_row_copy;
    return *this;
  }

 private:
  Row_copy _row_copy;
};

}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_ROW_BY_NAME_H_
