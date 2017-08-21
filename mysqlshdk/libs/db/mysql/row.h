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
  int64_t get_int(uint32_t index) const override;
  uint64_t get_uint(uint32_t index) const override;
  float get_float(uint32_t index) const override;
  double get_double(uint32_t index) const override;
  std::pair<const char *, size_t> get_string_data(
      uint32_t index) const override;
  uint64_t get_bit(uint32_t index) const override;

 private:
  friend class Result;
  Row(std::shared_ptr<Result> result, MYSQL_ROW row,
      const unsigned long *lengths);

  std::shared_ptr<Result> _result;
  MYSQL_ROW _row;
  std::vector<uint64_t> _lengths;
};

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
#endif
