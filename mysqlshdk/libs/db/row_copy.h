/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
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

// Persistent copy of a Row object

#ifndef MYSQLSHDK_LIBS_DB_ROW_COPY_H_
#define MYSQLSHDK_LIBS_DB_ROW_COPY_H_

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/include/mysqlshdk_export.h"
#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/db/row.h"

namespace mysqlshdk {
namespace db {

class SHCORE_PUBLIC Mem_row : public IRow {
 public:
  Mem_row();
  Mem_row(const Mem_row &row);
  Mem_row &operator=(const Mem_row &row);
  virtual ~Mem_row() {}

  uint32_t num_fields() const override;

  Type get_type(uint32_t index) const override;
  bool is_null(uint32_t index) const override;
  std::string get_as_string(uint32_t index) const override;

  std::string get_string(uint32_t index) const override;
  int64_t get_int(uint32_t index) const override;
  uint64_t get_uint(uint32_t index) const override;
  float get_float(uint32_t index) const override;
  double get_double(uint32_t index) const override;
  std::pair<const char *, size_t> get_string_data(
      uint32_t index) const override;
  void get_raw_data(uint32_t index, const char **out_data,
                    size_t *out_size) const override;
  uint64_t get_bit(uint32_t index) const override;

  /** Inserts new field at specified offset. Field value is set to null.
   *
   * @param type field type.
   * @param offset offset at which to insert (default - last column).
   */
  void add_field(Type type);
  void add_field(Type type, uint32_t offset);

 protected:
  class Field_data_ {
   public:
    virtual ~Field_data_() {}
  };

  template <typename T>
  class Field_data : public Field_data_ {
   public:
    explicit Field_data(const T &v) : value(v) {}
    explicit Field_data(T &&v) : value(std::move(v)) {}
    T value;
  };

  template <typename T>
  const T &get(int field) const {
    return static_cast<Field_data<T> *>(_data->fields[field].get())->value;
  }

  struct Data {
    std::vector<Type> types;
    std::vector<std::unique_ptr<Field_data_>> fields;

    Data() {}

    explicit Data(const std::vector<Type> &types_)
        : types(types_), fields(types_.size()) {}
  };
  std::shared_ptr<Data> _data;
  mutable std::string m_raw_data_cache;
};

/**
 * A self-contained Row object that owns its own storage, as opposed to
 * mysql::Row or mysqlx::Row which are references to data owned by the
 * underlying client library.
 *
 * Can be created from the copy-constructor, from any instance of IRow.
 */
class SHCORE_PUBLIC Row_copy : public Mem_row {
 public:
  explicit Row_copy(const IRow &row);

  Row_copy() {}

  virtual ~Row_copy() {}
};

/**
 * A self-contained Row object like Row_copy, but can be created or modified
 * programmatically.
 */
class SHCORE_PUBLIC Mutable_row : public Mem_row {
 public:
  template <class T>
  typename std::enable_if<std::is_integral<T>::value>::type set_field(
      uint32_t index, T &&arg) {
    if (_data->types[index] == Type::Integer)
      _data->fields[index] = std::unique_ptr<Field_data_>(
          new Field_data<int64_t>(std::forward<T>(arg)));
    else if (_data->types[index] == Type::UInteger)
      _data->fields[index] = std::unique_ptr<Field_data_>(
          new Field_data<uint64_t>(std::forward<T>(arg)));
    else
      throw std::invalid_argument(
          "Attempt to write integer value to non integer field");
  }

  template <class T>
  typename std::enable_if<std::is_floating_point<T>::value>::type set_field(
      uint32_t index, T &&arg) {
    if (_data->types[index] != Type::Float &&
        _data->types[index] != Type::Double)
      throw std::invalid_argument(
          "Attempt to write floating point number to not neither float or "
          "double field.");
    _data->fields[index] =
        std::make_unique<Field_data<T>>(std::forward<T>(arg));
  }

  template <class T>
  typename std::enable_if<std::is_same<T, std::nullptr_t>::value>::type
  set_field(uint32_t index, T && /*arg*/) {
    _data->fields[index] = nullptr;
  }

  template <class T>
  typename std::enable_if<!std::is_arithmetic<T>::value &&
                          !std::is_same<T, std::nullptr_t>::value>::type
  set_field(uint32_t index, T &&arg) {
    if (_data->types[index] >= Type::Integer &&
        _data->types[index] <= Type::Double)
      throw std::invalid_argument(
          "Attempt to write arithmetic type to non arithmetic field");
    _data->fields[index] = std::unique_ptr<Field_data_>(
        new Field_data<std::string>(std::string(std::forward<T>(arg))));
  }

 private:
  template <class T, class... Args>
  void set_field(uint32_t start, T &&value, Args... args) {
    set_field(start, std::forward<T>(value));
    if (start < _data->fields.size())
      set_field(start + 1, std::forward<Args>(args)...);
  }

 public:
  template <class... Args>
  void set_row_values(Args... args) {
    constexpr int n = sizeof...(args);
    if (n != _data->fields.size())
      throw std::invalid_argument(
          "The number of arguments does not match the row size");
    set_field(0, std::forward<Args>(args)...);
  }

  explicit Mutable_row(const std::vector<Type> &types) {
    _data = std::make_shared<Data>(types);
  }

  template <class... Args>
  Mutable_row(const std::vector<Type> &types, Args... args) {
    _data = std::make_shared<Data>(types);
    set_row_values(std::forward<Args>(args)...);
  }

  virtual ~Mutable_row();
};

}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_ROW_COPY_H_
