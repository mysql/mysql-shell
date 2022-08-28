/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _CORELIBS_DB_MYSQL_ROW_H_
#define _CORELIBS_DB_MYSQL_ROW_H_

#include <map>
#include <set>
#include <vector>
#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/db/row.h"

#include <mysql.h>
#include <memory>

namespace mysqlshdk {
namespace db {
namespace mysql {
class Result;
class SHCORE_PUBLIC Row : public mysqlshdk::db::IRow {
 public:
  Row(const Row &) = delete;
  void operator=(const Row &) = delete;

  uint32_t num_fields() const override;

  Type get_type(uint32_t index) const override;
  bool is_null(uint32_t index) const override;
  std::string get_as_string(uint32_t index) const override;

  std::string get_string(uint32_t index) const override;
  std::wstring get_wstring(uint32_t index) const override;
  int64_t get_int(uint32_t index) const override;
  uint64_t get_uint(uint32_t index) const override;
  float get_float(uint32_t index) const override;
  double get_double(uint32_t index) const override;
  std::pair<const char *, size_t> get_string_data(
      uint32_t index) const override;
  void get_raw_data(uint32_t index, const char **out_data,
                    size_t *out_size) const override;
  std::tuple<uint64_t, int> get_bit(uint32_t index) const override;

  inline void reset(MYSQL_ROW row, const unsigned long *lengths) {
    _row = row;
    _lengths = lengths;
  }

 private:
  friend class Result;
  explicit Row(Result *result);
  Row(Result *result, MYSQL_ROW row, const unsigned long *lengths);

  Result &_result;
  MYSQL_ROW _row;
  const unsigned long *_lengths;
  uint32_t m_num_fields;
};

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
#endif
