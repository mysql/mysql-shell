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

#ifndef MYSQLSHDK_LIBS_DB_ROW_MAP_H_
#define MYSQLSHDK_LIBS_DB_ROW_MAP_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/db/row.h"
#include "mysqlshdk/libs/db/row_copy.h"

namespace mysqlshdk {
namespace db {

class Field_names {
 public:
  explicit Field_names(const std::vector<Column> &metadata) {
    for (const auto &col : metadata) add(col.get_column_label());
  }

  Field_names(const Field_names &) = delete;

  Field_names() {}

  inline void add(const std::string &name) {
    uint32_t idx = m_fields.size();
    m_fields[name] = idx;
  }

  inline uint32_t field_index(const std::string &name) {
    auto it = m_fields.find(name);
    if (it == m_fields.end())
      throw std::invalid_argument("invalid field name " + name);
    return it->second;
  }

  inline const std::string &field_name(uint32_t index) {
    for (const auto &f : m_fields) {
      if (f.second == index) return f.first;
    }
    throw std::invalid_argument("invalid field index " + std::to_string(index));
  }

 private:
  std::map<std::string, uint32_t> m_fields;
};

class Row_ref_map {
 public:
  Row_ref_map() {}

  Row_ref_map(const std::shared_ptr<Field_names> &field_names, const IRow *row)
      : Row_ref_map(field_names) {
    _row_ref = row;
  }

  virtual ~Row_ref_map() {}

  const IRow &operator*() const { return *ref(); }

  void operator=(const IRow *row) { _row_ref = row; }

  operator bool() const { return _row_ref != nullptr; }

  uint32_t num_fields() const { return ref()->num_fields(); }

  Type get_type(const std::string &field) const {
    return ref()->get_type(field_index(field));
  }

  bool is_null(const std::string &field) const {
    return ref()->is_null(field_index(field));
  }

  std::string get_as_string(const std::string &field) const {
    return ref()->get_as_string(field_index(field));
  }

  std::string get_string(const std::string &field) const {
    return ref()->get_string(field_index(field));
  }

  int64_t get_int(const std::string &field) const {
    return ref()->get_int(field_index(field));
  }

  uint64_t get_uint(const std::string &field) const {
    return ref()->get_uint(field_index(field));
  }

  float get_float(const std::string &field) const {
    return ref()->get_float(field_index(field));
  }

  double get_double(const std::string &field) const {
    return ref()->get_double(field_index(field));
  }

  std::pair<const char *, size_t> get_string_data(
      const std::string &field) const {
    return ref()->get_string_data(field_index(field));
  }

  uint64_t get_bit(const std::string &field) const {
    return ref()->get_bit(field_index(field));
  }

  uint32_t field_index(const std::string &field) const {
    return _field_names->field_index(field);
  }

  const std::string &field_name(uint32_t i) const {
    return _field_names->field_name(i);
  }

 protected:
  explicit Row_ref_map(const std::shared_ptr<Field_names> &field_names)
      : _field_names(field_names), _row_ref(nullptr) {}

  std::shared_ptr<Field_names> _field_names;
  const IRow *_row_ref = nullptr;

  const IRow *ref() const {
    if (!_row_ref) throw std::invalid_argument("invalid row reference");
    return _row_ref;
  }

 public:
  std::shared_ptr<Field_names> field_names() const { return _field_names; }

  const IRow *row_ref() const { return _row_ref; }
};

class Row_map : public Row_ref_map {
 public:
  Row_map(const std::shared_ptr<Field_names> &field_names, const IRow &row)
      : Row_ref_map(field_names), _row_copy(row) {
    _row_ref = &_row_copy;
  }

  Row_map(const std::shared_ptr<Field_names> &field_names, Row_copy &&row_copy)
      : Row_ref_map(field_names), _row_copy(std::move(row_copy)) {
    _row_ref = &_row_copy;
  }

  explicit Row_map(const Row_ref_map &rrbn) : Row_ref_map(rrbn.field_names()) {
    if (rrbn.row_ref()) _row_copy = Row_copy(*rrbn.row_ref());
    _row_ref = &_row_copy;
  }

  explicit Row_map(Row_map &&rbn)
      : Row_ref_map(std::move(rbn)), _row_copy(std::move(rbn._row_copy)) {
    _row_ref = &_row_copy;
  }

  Row_map &operator=(Row_map &&o) {
    Row_ref_map::operator=(std::move(o));
    _row_copy = std::move(o._row_copy);
    _row_ref = &_row_copy;
    return *this;
  }

 private:
  Row_copy _row_copy;
};

}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_ROW_MAP_H_
