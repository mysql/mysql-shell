/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_MYSQLX_ROW_H_
#define MYSQLSHDK_LIBS_DB_MYSQLX_ROW_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/db/mysqlx/mysqlxclient_clean.h"
#include "mysqlshdk/libs/db/row.h"

namespace mysqlshdk {
namespace db {
namespace mysqlx {

class Result;

class SHCORE_PUBLIC Row : public mysqlshdk::db::IRow {
 public:
  uint32_t num_fields() const override;

  Type get_type(uint32_t index) const override;
  bool is_null(uint32_t index) const override;
  std::string get_as_string(uint32_t index) const override;

  std::string get_string(uint32_t index) const override;
  int64_t get_int(uint32_t index) const override;
  uint64_t get_uint(uint32_t index) const override;
  float get_float(uint32_t index) const override;
  double get_double(uint32_t index) const override;
  std::pair<const char *, size_t> get_string_data(uint32_t index) const override;
  uint64_t get_bit(uint32_t index) const override;

 private:
  friend class Result;

  explicit Row(const Result *owner);
  void reset(const xcl::XRow *row);

  Row(const Row &) = delete;
  void operator=(const Row &) = delete;

 private:
  const Result *_owner;
  const xcl::XRow *_row;
};

}  // namespace mysqlx
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_MYSQLX_ROW_H_
