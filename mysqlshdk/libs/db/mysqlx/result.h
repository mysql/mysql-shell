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

#ifndef MYSQLSHDK_LIBS_DB_MYSQLX_RESULT_H_
#define MYSQLSHDK_LIBS_DB_MYSQLX_RESULT_H_

#include "mysqlshdk/libs/db/result.h"

#include <deque>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/mysqlx/mysqlxclient_clean.h"
#include "mysqlshdk/libs/db/mysqlx/row.h"
#include "mysqlshdk/libs/db/row_copy.h"

namespace mysqlshdk {
namespace db {
namespace mysqlx {

class SHCORE_PUBLIC Result : public mysqlshdk::db::IResult,
                             public std::enable_shared_from_this<Result> {
  friend class XSession_impl;

 public:
  ~Result() override;

  // Data Retrieving
  const IRow *fetch_one() override;
  bool next_resultset() override;
  std::unique_ptr<Warning> fetch_one_warning() override;
  std::string get_info() const override;
  const std::vector<std::string> &get_gtids() const override {
    throw std::logic_error("not implemented");
  }

  // Read and buffer all rows for the active data set
  bool pre_fetch_rows(bool persistent = false);
  void stop_pre_fetch();
  void rewind() override;
  void buffer() override;

  // Metadata retrieval
  int64_t get_auto_increment_value() const override;
  bool has_resultset() override;
  uint64_t get_affected_row_count() const override;
  uint64_t get_fetched_row_count() const override { return _fetched_row_count; }
  uint64_t get_warning_count() const override;
  const std::vector<Column> &get_metadata() const override { return _metadata; }
  std::vector<std::string> get_generated_ids();

  // read all resultsets without affecting the prefetched rows
  void drain_resultset() const;

 protected:
  explicit Result(std::unique_ptr<xcl::XQuery_result> result);
  void fetch_metadata();
  std::shared_ptr<Field_names> field_names() const override;

  std::vector<Column> _metadata;

  std::deque<mysqlshdk::db::Row_copy> _pre_fetched_rows;
  std::unique_ptr<xcl::XQuery_result> _result;
  mutable std::shared_ptr<Field_names> _field_names;

  Row _row;
  size_t _fetched_row_count = 0;
  size_t _fetched_warning_count = 0;
  /// Tracks the number of rows retrieved by fetch_one before pre_fetch happened
  size_t m_fetched_before_prefetch = 0;
  std::string _info;
  bool _stop_pre_fetch = false;
  bool _pre_fetched = false;
  bool _persistent_pre_fetch = false;
};
}  // namespace mysqlx
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_MYSQLX_RESULT_H_
