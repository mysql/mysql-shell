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

#ifndef MYSQLSHDK_LIBS_DB_REPLAY_RECORDER_H_
#define MYSQLSHDK_LIBS_DB_REPLAY_RECORDER_H_

#include <memory>
#include <string>
#include <string_view>

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
extern std::function<std::string(std::string_view sql)>
    on_recorder_query_replace_hook;

extern std::function<std::string(const std::string &value)>
    on_recorder_result_value_replace_hook;

class Recorder_mysql : public mysql::Session {
 public:
  using super = mysql::Session;

  Recorder_mysql();

  std::shared_ptr<IResult> querys(
      const char *sql, size_t length, bool buffered,
      const std::vector<Query_attribute> &query_attributes = {}) override;

  std::shared_ptr<IResult> query_udf(std::string_view sql,
                                     bool buffered) override;

  void executes(const char *sql, size_t length) override;

 protected:
  void do_connect(const mysqlshdk::db::Connection_options &data) override;

  void do_close() override;

 private:
  std::unique_ptr<Trace_writer> _trace;
  int _port = 0;
  bool _closed = false;
};

class Recorder_mysqlx : public mysqlx::Session {
 public:
  using super = mysqlx::Session;

  Recorder_mysqlx();

  std::shared_ptr<IResult> querys(
      const char *sql, size_t length, bool buffered,
      const std::vector<Query_attribute> &query_attributes = {}) override;

  void executes(const char *sql, size_t length) override;

  std::shared_ptr<IResult> execute_stmt(
      const std::string &ns, std::string_view stmt,
      const ::xcl::Argument_array &args) override;

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

 protected:
  void do_connect(const mysqlshdk::db::Connection_options &data) override;

  void do_close() override;

 private:
  std::unique_ptr<Trace_writer> _trace;
  int _port = 0;
  bool _closed = false;
};

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_REPLAY_RECORDER_H_
