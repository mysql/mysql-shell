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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MYSQLSHDK_LIBS_DB_ROW_H_
#define MYSQLSHDK_LIBS_DB_ROW_H_

#include <mysqlxclient/xdatetime.h>
#include <cstdint>
#include <set>
#include <string>
#include <utility>

#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk_export.h"

namespace mysqlshdk {
namespace db {

class bad_field : public std::invalid_argument {
 public:
  bad_field(const char *msg, uint32_t index)
      : std::invalid_argument(msg), field(index) {}
  uint32_t field;
};

class SHCORE_PUBLIC IRow {
 public:
  IRow() {}
  // non-copiable
  IRow(const IRow &) = delete;
  void operator=(const IRow &) = delete;

  virtual uint32_t num_fields() const = 0;

  virtual Type get_type(uint32_t index) const = 0;
  virtual bool is_null(uint32_t index) const = 0;
  virtual std::string get_as_string(uint32_t index) const = 0;

  virtual std::string get_string(uint32_t index) const = 0;
  virtual int64_t get_int(uint32_t index) const = 0;
  virtual uint64_t get_uint(uint32_t index) const = 0;
  virtual float get_float(uint32_t index) const = 0;
  virtual double get_double(uint32_t index) const = 0;
  virtual std::pair<const char *, size_t> get_string_data(
      uint32_t index) const = 0;
  virtual void get_raw_data(uint32_t index, const char **out_data,
                            size_t *out_size) const = 0;
  virtual uint64_t get_bit(uint32_t index) const = 0;

  inline std::string get_as_string(uint32_t index,
                                   const std::string &default_if_null) const {
    if (is_null(index)) return default_if_null;
    return get_as_string(index);
  }

  inline std::string get_string(uint32_t index,
                                const std::string &default_if_null) const {
    if (is_null(index)) return default_if_null;
    return get_string(index);
  }

  inline int64_t get_int(uint32_t index, int64_t default_if_null) const {
    if (is_null(index)) return default_if_null;
    return get_int(index);
  }

  inline uint64_t get_uint(uint32_t index, uint64_t default_if_null) const {
    if (is_null(index)) return default_if_null;
    return get_uint(index);
  }

  inline double get_double(uint32_t index, double default_if_null) const {
    if (is_null(index)) return default_if_null;
    return get_double(index);
  }

  virtual ~IRow() {}
};
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_ROW_H_
