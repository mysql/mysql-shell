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

class Recorder_mysql : public mysql::Session {
 public:
  using super = mysql::Session;

  static std::shared_ptr<mysql::Session> create() {
    return std::shared_ptr<mysql::Session>{new Recorder_mysql()};
  }

  void connect(const mysqlshdk::db::Connection_options &data) override;

  std::shared_ptr<IResult> query(const std::string &sql,
                                 bool buffered) override;

  void execute(const std::string &sql) override;

  void close() override;

 private:
  Recorder_mysql();

  std::unique_ptr<Trace_writer> _trace;
  int _port;
};

class Recorder_mysqlx : public mysqlx::Session {
 public:
  using super = mysqlx::Session;

  static std::shared_ptr<mysqlx::Session> create() {
    return std::shared_ptr<mysqlx::Session>{new Recorder_mysqlx()};
  }

  void connect(const mysqlshdk::db::Connection_options &data) override;

  std::shared_ptr<IResult> query(const std::string &sql,
                                 bool buffered) override;

  void execute(const std::string &sql) override;

  void close() override;

  std::shared_ptr<IResult> execute_stmt(const std::string & /*ns*/,
                                        const std::string & /*stmt*/,
                                        const ::xcl::Arguments & /*args*/) {
    throw std::logic_error("not implemented for recording");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Insert & /*msg*/) {
    throw std::logic_error("not implemented for recording");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Update & /*msg*/) {
    throw std::logic_error("not implemented for recording");
  }

  std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Delete & /*msg*/) {
    throw std::logic_error("not implemented for recording");
  }

  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Find & /*msg*/) {
    throw std::logic_error("not implemented for recording");
  }

 private:
  Recorder_mysqlx();

  std::unique_ptr<Trace_writer> _trace;
  int _port;
};

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_REPLAY_RECORDER_H_
