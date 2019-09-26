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

#ifndef MYSQLSHDK_LIBS_UTILS_ARRAY_RESULT_H_
#define MYSQLSHDK_LIBS_UTILS_ARRAY_RESULT_H_

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/result.h"

#include <string>
#include <vector>

namespace shcore {
class Array_as_result : public mysqlshdk::db::IResult {
 public:
  explicit Array_as_result(const shcore::Array_t &array);
  const mysqlshdk::db::IRow *fetch_one() override;
  bool next_resultset() override;
  bool has_resultset() override;
  bool has_resultset() const { return m_has_result; }
  const std::vector<mysqlshdk::db::Column> &get_metadata() const override;
  void buffer() override;
  void rewind() override;

  std::unique_ptr<mysqlshdk::db::Warning> fetch_one_warning() override {
    return {};
  }

  int64_t get_auto_increment_value() const override { return 0; }

  uint64_t get_affected_row_count() const override { return 0; }

  uint64_t get_fetched_row_count() const override { return m_current_row - 1; }

  uint64_t get_warning_count() const override { return 0; }

  std::string get_info() const override { return ""; }

  const std::vector<std::string> &get_gtids() const override {
    throw std::logic_error("Not implemented.");
  }

  std::shared_ptr<mysqlshdk::db::Field_names> field_names() const override;

  class Vector_as_row : public mysqlshdk::db::IRow {
   public:
    explicit Vector_as_row(const std::vector<std::string> &row) : m_row(row) {}

    mysqlshdk::db::Type get_type(uint32_t) const override {
      return mysqlshdk::db::Type::String;
    }

    bool is_null(uint32_t idx) const override { return idx >= m_row.size(); }

    std::string get_as_string(uint32_t idx) const override {
      return get_string(idx);
    }

    std::string get_string(uint32_t idx) const override {
      if (idx < m_row.size()) {
        return m_row[idx];
      } else {
        return "NULL";
      }
    }

    int64_t get_int(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

    uint64_t get_uint(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

    float get_float(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

    double get_double(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

    uint32_t num_fields() const override {
      return static_cast<uint32_t>(m_row.size());
    }

    std::pair<const char *, size_t> get_string_data(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

    uint64_t get_bit(uint32_t) const override {
      throw std::logic_error("Not implemented.");
    }

   private:
    const std::vector<std::string> &m_row;
  };

  bool m_has_result = true;
  size_t m_current_row = 1;
  std::vector<mysqlshdk::db::Column> m_metadata;
  std::vector<std::vector<std::string>> m_data;
  std::unique_ptr<Vector_as_row> m_row;
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_ARRAY_RESULT_H_
