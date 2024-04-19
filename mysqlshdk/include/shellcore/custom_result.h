/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_CUSTOM_RESULT_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_CUSTOM_RESULT_H_

#include <memory>
#include <variant>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/result.h"

namespace mysqlshdk {

class Custom_result;
class Custom_row : public mysqlshdk::db::IRow {
 public:
  Custom_row(const std::shared_ptr<Custom_result> &result,
             const shcore::Dictionary_t &data, size_t row_index);
  Custom_row(const std::shared_ptr<Custom_result> &result,
             const shcore::Array_t &data, size_t row_index);

  // non-copiable but movable
  Custom_row(const Custom_row &) = delete;
  Custom_row &operator=(const Custom_row &) = delete;
  Custom_row(Custom_row &&) = default;
  Custom_row &operator=(Custom_row &&) = default;

  uint32_t num_fields() const override;

  mysqlshdk::db::Type get_type(uint32_t index) const override;
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

 private:
  const mysqlshdk::db::Column &column(size_t index) const;
  const shcore::Value &data(size_t index) const;
  shcore::Exception format_exception(const shcore::Exception &error,
                                     size_t index) const;
  size_t m_num_fields = 0;

  std::weak_ptr<Custom_result> m_result;
  std::variant<shcore::Dictionary_t, shcore::Array_t> m_data;
  mutable std::string m_raw_data_cache;
  size_t m_row_index = 0;
};

/**
 * Constructs a Result given a dictionary with elements according to the
 * following specification:
 *
 * {
 *    "affectedItemsCount": integer, number of affected items by the executed
 * SQL.
 *    "info": string, additional description of the result.
 *    "executionTime": string, the time the SQL statement took to execute.
 *    "autoIncrementValue": integer, value corresponding to the last insert id
 *      auto generated
 *    "warnings": list of warning definitions. "data": list of
 *      dictionaries defining the rows in the Result.
 * }
 *
 * A warning definition follows this structure;
 * {
 *    "level": one of error, warning or note
 *    "code": numeric code for the warning
 *    "message": message describing the warning
 * }
 *
 * Currently the only supported data format is a list of dictionaries.
 *
 * A dictionary represents a record in the result.
 *
 * In the future other formats may be supported, in such case, the data
 * element could be a dictionary with the specific data needed for each
 * supported format.
 *
 * This way future implementations are backward compatible.
 */
class Custom_result : public mysqlshdk::db::IResult,
                      public std::enable_shared_from_this<Custom_result> {
 public:
  explicit Custom_result(const shcore::Dictionary_t &data);

  Custom_result(Custom_result &&other) = default;

  const mysqlshdk::db::IRow *fetch_one() override;

  bool next_resultset() override;

  std::unique_ptr<mysqlshdk::db::Warning> fetch_one_warning() override;

  // Metadata retrieval
  int64_t get_auto_increment_value() const override {
    return m_auto_increment_value;
  }

  bool has_resultset() override;

  uint64_t get_affected_row_count() const override;
  uint64_t get_fetched_row_count() const override;

  // In case of classic result this will only return real value after fetching
  // the result data
  uint64_t get_warning_count() const override;
  std::string get_info() const override;
  const std::vector<std::string> &get_gtids() const override;

  const std::vector<mysqlshdk::db::Column> &get_metadata() const override;
  std::shared_ptr<mysqlshdk::db::Field_names> field_names() const override;
  std::string get_statement_id() const override { return ""; }

  void buffer() override;
  void rewind() override;

  ~Custom_result() override = default;

 private:
  void check_data_consistency();
  void parse_column_metadata();
  void deduce_column_metadata();

  shcore::Dictionary_t m_raw_result;
  shcore::Array_t m_result_columns;
  shcore::Array_t m_result_data;

  std::vector<mysqlshdk::db::Column> m_columns;
  std::vector<std::unique_ptr<Custom_row>> m_rows;

  size_t m_warning_index = 0;

  size_t m_row_index = 0;

  bool m_has_result = false;
  uint64_t m_affected_items_count = 0;
  int64_t m_auto_increment_value = 0;
  std::string m_info;
  std::vector<std::string> m_gtids;
  std::vector<mysqlshdk::db::Warning> m_warnings;
};

class Custom_result_set : public mysqlshdk::db::IResult {
 public:
  explicit Custom_result_set(const shcore::Array_t &data);

  const mysqlshdk::db::IRow *fetch_one() override;

  bool next_resultset() override;

  std::unique_ptr<mysqlshdk::db::Warning> fetch_one_warning() override;

  // Metadata retrieval
  int64_t get_auto_increment_value() const override { return 0; }

  bool has_resultset() override;

  uint64_t get_affected_row_count() const override;
  uint64_t get_fetched_row_count() const override;
  double get_execution_time() const override;

  // In case of classic result this will only return real value after fetching
  // the result data
  uint64_t get_warning_count() const override;
  std::string get_info() const override;
  const std::vector<std::string> &get_gtids() const override;

  const std::vector<mysqlshdk::db::Column> &get_metadata() const override;
  std::shared_ptr<mysqlshdk::db::Field_names> field_names() const override;
  std::string get_statement_id() const override { return ""; }

  void buffer() override;
  void rewind() override;

  ~Custom_result_set() override = default;

 private:
  shcore::Array_t m_raw_data;

  std::vector<std::shared_ptr<Custom_result>> m_results;
  size_t m_next_result_index = 0;
  std::shared_ptr<Custom_result> m_current_result;

  std::vector<std::string> m_gtids;
  std::vector<mysqlshdk::db::Column> m_metadata;
};

}  // namespace mysqlshdk

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_CUSTOM_RESULT_H_
