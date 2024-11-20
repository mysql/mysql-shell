/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_DB_MUTABLE_RESULT_H_
#define MYSQLSHDK_LIBS_DB_MUTABLE_RESULT_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/row_copy.h"

namespace mysqlshdk {
namespace db {

class Mutable_result : public IResult {
 public:
  static inline Column make_column(const std::string &name,
                                   Type type = Type::String,
                                   const std::string &label = "") {
    return Column("", "", "", "", name, label.empty() ? name : label, 0, 0,
                  type, 0, false, false, false);
  }

 public:
  Mutable_result();
  explicit Mutable_result(const std::vector<Column> &metadata)
      : _metadata(metadata) {}
  explicit Mutable_result(const std::vector<Type> &types);
  explicit Mutable_result(IResult *result);
  ~Mutable_result();

  Mutable_result(const Mutable_result &result) = delete;
  Mutable_result &operator=(const Mutable_result &row) = delete;

  uint64_t get_affected_row_count() const override { return _rows.size(); }

  uint64_t get_fetched_row_count() const override { return _rows.size(); }

  uint64_t get_warning_count() const override { return 0; }

  std::string get_info() const override { return ""; }

  const std::vector<std::string> &get_gtids() const override { return _gtids; }

  const std::vector<Column> &get_metadata() const override { return _metadata; }

  /** Adds column at specified offset.
   *
   * For existing rows field value is set to null.
   * @param type field type.
   * @param offset offset at which to insert (default - last column).
   */
  void add_column(const Column &column, size_t offset);
  void add_column(const Column &column);
  void add_row(std::unique_ptr<Mem_row> row);

  Mutable_row &add_row() {
    auto row = std::make_unique<Mutable_row>(_metadata);
    auto tmp = row.get();
    _rows.emplace_back(std::move(row));
    return *tmp;
  }

  void add_from(IResult *result);

  template <class... Args>
  void append(Args... args) {
    if (sizeof...(args) != _metadata.size())
      throw std::invalid_argument(
          "Mismatch between row size and data to append");
    _rows.emplace_back(
        std::move(std::make_unique<Mutable_row>(_metadata, args...)));
  }

  void reset() { _fetched_row_count = 0; }
  void rewind() override {}
  void buffer() override {}

 public:
  const IRow *fetch_one() override;

  bool next_resultset() override { return false; }

  std::unique_ptr<Warning> fetch_one_warning() override { return {}; }

  bool has_resultset() override { return true; }

  int64_t get_auto_increment_value() const override { return 0; }

  std::shared_ptr<Field_names> field_names() const override {
    if (!m_field_names) {
      m_field_names = std::make_shared<Field_names>(get_metadata());
    }
    return m_field_names;
  }

 private:
  std::vector<Column> _metadata;
  std::vector<std::unique_ptr<Mem_row>> _rows;
  std::vector<std::string> _gtids;
  size_t _fetched_row_count = 0;
  mutable std::shared_ptr<Field_names> m_field_names;

  friend bool operator==(const Mutable_result &left,
                         const Mutable_result &right);
  friend bool operator!=(const Mutable_result &left,
                         const Mutable_result &right);
};

}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_MUTABLE_RESULT_H_
