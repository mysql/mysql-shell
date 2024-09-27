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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MYSQLSHDK_LIBS_DB_MYSQL_RESULT_H_
#define MYSQLSHDK_LIBS_DB_MYSQL_RESULT_H_

#include "mysqlshdk/libs/db/result.h"

#include <deque>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <mysql.h>

#include "mysqlshdk/libs/utils/atomic_flag.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
class Session_impl;
class Row;

class SHCORE_PUBLIC Result : public mysqlshdk::db::IResult,
                             public std::enable_shared_from_this<Result> {
  friend class Session_impl;

 public:
  virtual ~Result();

  // Data Retrieving
  virtual const IRow *fetch_one();
  virtual bool next_resultset();
  virtual std::unique_ptr<Warning> fetch_one_warning();

  // Metadata retrieval
  virtual int64_t get_auto_increment_value() const { return _last_insert_id; }
  virtual bool has_resultset() { return _has_resultset; }
  virtual uint64_t get_affected_row_count() const {
    return (_affected_rows == ~(my_ulonglong)0) ? 0 : _affected_rows;
  }
  virtual uint64_t get_fetched_row_count() const { return _fetched_row_count; }
  virtual uint64_t get_warning_count() const;
  virtual std::string get_info() const { return _info; }
  virtual const std::vector<std::string> &get_gtids() const { return _gtids; }
  virtual const std::vector<Column> &get_metadata() const { return _metadata; }
  virtual std::string get_statement_id() const;

  virtual void buffer();
  virtual void rewind();

  bool is_buffered() { return m_buffered; }

 protected:
  Result(std::shared_ptr<mysqlshdk::db::mysql::Session_impl> owner,
         uint64_t affected_rows, uint64_t last_insert_id, const char *info,
         bool buffered);
  void reset(std::shared_ptr<MYSQL_RES> res);

  std::deque<mysqlshdk::db::Row_copy> _pre_fetched_rows;
  // size_t _fetched_row_count = 0;
  // size_t _fetched_warning_count = 0;
  shcore::atomic_flag _stop_pre_fetch;
  bool _pre_fetched = false;
  bool _persistent_pre_fetch = false;
  bool _pre_fetched_clear_at_end = false;

  bool pre_fetch_row();
  bool pre_fetch_rows(bool persistent);
  void stop_pre_fetch();

  void fetch_metadata();
  void fetch_statement_id();
  Type map_data_type(int raw_type, int flags, int collation_id);

  virtual std::shared_ptr<Field_names> field_names() const;

  std::weak_ptr<mysqlshdk::db::mysql::Session_impl> _session;
  std::vector<Column> _metadata;
  std::unique_ptr<Row> _row;
  std::weak_ptr<MYSQL_RES> _result;
  std::vector<std::string> _gtids;
  mutable std::shared_ptr<Field_names> _field_names;
  uint64_t _affected_rows = 0;
  uint64_t _last_insert_id = 0;
  uint64_t _fetched_row_count = 0;
  std::string _info;
  std::list<std::unique_ptr<Warning>> _warnings;
  bool _has_resultset = false;
  bool _fetched_warnings = false;
  bool m_buffered = false;
  std::optional<std::string> m_statement_id;
};
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_MYSQL_RESULT_H_
