/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_REPLAY_RECORDER_H_
#define MYSQLSHDK_LIBS_DB_REPLAY_RECORDER_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/replay/trace.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlshdk {
namespace db {
namespace replay {

extern std::function<void(std::shared_ptr<mysqlshdk::db::ISession>)>
    on_recorder_connect_hook;
extern std::function<void(std::shared_ptr<mysqlshdk::db::ISession>)>
    on_recorder_close_hook;

class Recorder_mysql : public mysql::Session {
 public:
  using super = mysql::Session;

  explicit Recorder_mysql(int print_traces);

  void connect(const mysqlshdk::db::Connection_options &data) override;

  std::shared_ptr<IResult> querys(const char *sql, size_t length,
                                  bool buffered) override;

  void executes(const char *sql, size_t length) override;

  void close() override;

 private:
  std::unique_ptr<Trace_writer> _trace;
  int _port;
  bool _closed = false;
  int _print_traces = 0;
};

class Recorder_mysqlx : public mysqlx::Session {
 public:
  using super = mysqlx::Session;

  explicit Recorder_mysqlx(int print_traces);

  void connect(const mysqlshdk::db::Connection_options &data) override;

  std::shared_ptr<IResult> querys(const char *sql, size_t length,
                                  bool buffered) override;

  void executes(const char *sql, size_t length) override;

  void close() override;

  std::shared_ptr<IResult> execute_stmt(const std::string &ns,
                                        const std::string &stmt,
                                        const ::xcl::Arguments &args) override;

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Insert & /*msg*/) override {
    throw std::logic_error("not implemented for recording");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Update & /*msg*/) override {
    throw std::logic_error("not implemented for recording");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Delete & /*msg*/) override {
    throw std::logic_error("not implemented for recording");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Find & /*msg*/) override {
    throw std::logic_error("not implemented for recording");
  }

 private:
  std::unique_ptr<Trace_writer> _trace;
  int _port;
  bool _closed = false;
  int _print_traces = 0;
};

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_REPLAY_RECORDER_H_
