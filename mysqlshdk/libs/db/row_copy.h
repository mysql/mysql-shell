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

// Persistent copy of a Row object

#ifndef MYSQLSHDK_LIBS_DB_ROW_COPY_H_
#define MYSQLSHDK_LIBS_DB_ROW_COPY_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk_export.h"
#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/db/row.h"

namespace mysqlshdk {
namespace db {
class SHCORE_PUBLIC Row_copy : public IRow {
 public:
  Row_copy();
  explicit Row_copy(const IRow &row);
  Row_copy(const Row_copy &row);
  Row_copy &operator=(const Row_copy &row);
  ~Row_copy();

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
  uint64_t get_bit(uint32_t index) const override;

 private:
  class Field_data_ {};
  template <typename T>
  class Field_data : public Field_data_ {
   public:
    explicit Field_data(const T &v) : value(v) {
    }
    T value;
  };

  template <typename T>
  const T &get(int field) const {
    return static_cast<Field_data<T> *>(_data->fields[field])->value;
  }

  struct Data {
    std::vector<Type> types;
    std::vector<Field_data_ *> fields;
  };
  std::shared_ptr<Data> _data;
};
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_ROW_COPY_H_
