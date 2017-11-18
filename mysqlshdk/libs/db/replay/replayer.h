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

#ifndef MYSQLSHDK_LIBS_DB_REPLAY_REPLAYER_H_
#define MYSQLSHDK_LIBS_DB_REPLAY_REPLAYER_H_

#include <list>
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

  static std::shared_ptr<mysql::Session> create() {
    return std::shared_ptr<mysql::Session>{new Replayer_mysql()};
  }

  void connect(const mysqlshdk::db::Connection_options& data) override;

  std::shared_ptr<IResult> query(const std::string& sql,
                                 bool buffered) override;

  void execute(const std::string& sql) override;

  void close() override;

  bool is_open() const override;

  const char* get_ssl_cipher() const override;

  const mysqlshdk::db::Connection_options& get_connection_options()
      const override;

  ~Replayer_mysql();

 private:
  Replayer_mysql();

  Replayer_impl* _impl;
};

class Result_mysql : public db::mysql::Result {
 public:
  Result_mysql(uint64_t affected_rows, unsigned int warning_count,
               uint64_t last_insert_id, const char* info);

  const db::IRow* fetch_one() override {
    if (_rows.size() > _fetched_row_count)
      return &_rows[_fetched_row_count++];
    return nullptr;
  }

  bool next_resultset() override {
    return false;
  }

  std::unique_ptr<db::Warning> fetch_one_warning() override {
    return {};
  }

  bool has_resultset() override {
    return _has_resultset;
  }

 private:
  friend class Trace;

  std::vector<db::Row_copy> _rows;

  std::list<std::unique_ptr<Warning>> _warnings;
  bool _has_resultset = false;
  bool _fetched_warnings = false;
};

// ---


class Replayer_mysqlx : public mysqlx::Session {
 public:
  using super = mysqlx::Session;

  static std::shared_ptr<mysqlx::Session> create() {
    return std::shared_ptr<mysqlx::Session>{new Replayer_mysqlx()};
  }

  void connect(const mysqlshdk::db::Connection_options& data) override;

  std::shared_ptr<IResult> query(const std::string& sql,
                                 bool buffered) override;

  void execute(const std::string& sql) override;

  void close() override;

  bool is_open() const override;

  const char* get_ssl_cipher() const override;

  const mysqlshdk::db::Connection_options& get_connection_options()
      const override;

  std::shared_ptr<IResult> execute_stmt(const std::string & /*ns*/,
                                        const std::string & /*stmt*/,
                                        const ::xcl::Arguments & /*args*/) {
    throw std::logic_error("not implemented for replaying");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Insert & /*msg*/) {
    throw std::logic_error("not implemented for replaying");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Update & /*msg*/) {
    throw std::logic_error("not implemented for replaying");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Delete & /*msg*/) {
    throw std::logic_error("not implemented for replaying");
  }

  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Find & /*msg*/) {
    throw std::logic_error("not implemented for replaying");
  }

  ~Replayer_mysqlx();

 private:
  Replayer_mysqlx();

  Replayer_impl *_impl;
};


class Result_mysqlx : public db::mysqlx::Result {
 public:
  Result_mysqlx(uint64_t affected_rows, unsigned int warning_count,
               uint64_t last_insert_id, const char* info);

  const db::IRow* fetch_one() override {
    if (_rows.size() > _fetched_row_count)
      return &_rows[_fetched_row_count++];
    return nullptr;
  }

  bool next_resultset() override {
    return false;
  }

  std::unique_ptr<db::Warning> fetch_one_warning() override {
    return {};
  }

  bool has_resultset() override {
    return _has_resultset;
  }

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
