/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_DB_REPLAY_REPLAYER_H_
#define MYSQLSHDK_LIBS_DB_REPLAY_REPLAYER_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/replay/trace.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlshdk {
namespace db {
namespace replay {

class Replayer_impl;

class Replayer_mysql : public mysql::Session {
 public:
  using super = mysql::Session;

  Replayer_mysql();

  void connect(const mysqlshdk::db::Connection_options &data) override;

  std::shared_ptr<IResult> querys(const char *sql, size_t length,
                                  bool buffered) override;

  void executes(const char *sql, size_t length) override;

  void close() override;

  bool is_open() const override;

  uint64_t get_connection_id() const override;
  uint64_t get_protocol_info() override;
  const char *get_ssl_cipher() const override;
  const char *get_connection_info() override;
  const char *get_server_info() override;
  mysqlshdk::utils::Version get_server_version() const override;

  const mysqlshdk::db::Connection_options &get_connection_options()
      const override;

  ~Replayer_mysql();

 private:
  std::unique_ptr<Replayer_impl> _impl;
};

class Result_mysql : public db::mysql::Result {
 public:
  Result_mysql(uint64_t affected_rows, unsigned int warning_count,
               uint64_t last_insert_id, const char *info, bool buffered,
               const std::vector<std::string> &gtids);

  const db::IRow *fetch_one() override {
    if (_rows.size() > _fetched_row_count) return &_rows[_fetched_row_count++];
    return nullptr;
  }

  bool next_resultset() override { return false; }

  std::unique_ptr<db::Warning> fetch_one_warning() override { return {}; }

  bool has_resultset() override { return _has_resultset; }

  uint64_t get_warning_count() const override { return _warning_count; }

  const std::vector<std::string> &get_gtids() const override { return _gtids; }

 private:
  friend class Trace;

  std::vector<db::Row_copy> _rows;

  uint64_t _warning_count = 0;
  std::list<std::unique_ptr<db::Warning>> _warnings;
  std::vector<std::string> _gtids;
  bool _has_resultset = false;
  bool _fetched_warnings = false;
};

// ---

class Replayer_mysqlx : public mysqlx::Session {
 public:
  using super = mysqlx::Session;

  Replayer_mysqlx();

  void connect(const mysqlshdk::db::Connection_options &data) override;

  std::shared_ptr<IResult> querys(const char *sql, size_t length,
                                  bool buffered) override;

  void executes(const char *sql, size_t length) override;

  void close() override;

  bool is_open() const override;

  uint64_t get_connection_id() const override;
  const char *get_ssl_cipher() const override;
  const std::string &get_connection_info() const override;
  mysqlshdk::utils::Version get_server_version() const override;

  const mysqlshdk::db::Connection_options &get_connection_options()
      const override;

  std::shared_ptr<IResult> execute_stmt(
      const std::string &ns, const std::string &stmt,
      const ::xcl::Argument_array &args) override;

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Insert & /*msg*/) override {
    throw std::logic_error("not implemented for replaying");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Update & /*msg*/) override {
    throw std::logic_error("not implemented for replaying");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Delete & /*msg*/) override {
    throw std::logic_error("not implemented for replaying");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Find & /*msg*/) override {
    throw std::logic_error("not implemented for replaying");
  }

  ~Replayer_mysqlx();

 private:
  std::unique_ptr<Replayer_impl> _impl;
};

class Result_mysqlx : public db::mysqlx::Result {
 public:
  Result_mysqlx(uint64_t affected_rows, unsigned int warning_count,
                uint64_t last_insert_id, const char *info);

  const db::IRow *fetch_one() override {
    if (_rows.size() > _fetched_row_count) return &_rows[_fetched_row_count++];
    return nullptr;
  }

  bool next_resultset() override { return false; }

  std::unique_ptr<Warning> fetch_one_warning() override { return {}; }

  bool has_resultset() override { return _has_resultset; }

  int64_t get_auto_increment_value() const override { return _last_insert_id; }

 private:
  friend class Trace;

  std::vector<db::Row_copy> _rows;
  uint64_t _affected_rows = 0;
  uint64_t _last_insert_id = 0;
  std::list<std::unique_ptr<Warning>> _warnings;
  bool _has_resultset = false;
  bool _fetched_warnings = false;
};

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_REPLAY_REPLAYER_H_
