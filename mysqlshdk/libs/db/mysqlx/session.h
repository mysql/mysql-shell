/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_MYSQLX_SESSION_H_
#define MYSQLSHDK_LIBS_DB_MYSQLX_SESSION_H_

#include <memory>
#include <set>
#include <string>
#include <utility>
#include "mysqlshdk/libs/db/mysqlx/mysqlxclient_clean.h"

#include "mysqlshdk/libs/db/mysqlx/result.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlshdk {
namespace db {
namespace mysqlx {
/*
 * Session implementation for the MySQL protocol.
 *
 * This class can only be used by the Session class in order to guarantee
 * that any instance is always contained on a shared pointer, this is required
 * because the results returned by the query function require a reference to
 * the Session that generated them.
 */
class XSession_impl : public std::enable_shared_from_this<XSession_impl> {
  friend class Session;  // The Session class instantiates this class
  friend class Result;   // The Reslt class uses some functions of this class

 public:
  ~XSession_impl();

 private:
  XSession_impl();
  void connect(const mysqlshdk::db::Connection_options& data);

  const mysqlshdk::db::Connection_options & get_connection_options() {
    return _connection_options;
  }

  void enable_trace(bool flag);

  std::shared_ptr<IResult> query(const std::string &sql, bool buffered = false);
  void execute(const std::string &sql);

  void close();

  void before_query();

  bool valid() const { return _mysql.get() != nullptr; }

  std::shared_ptr<IResult> after_query(
      std::unique_ptr<xcl::XQuery_result> result, bool buffered = false);

  // Utility functions to retriev session status
  uint64_t get_thread_id() const {
    return _connection_id;
  }

  const std::string &get_connection_info() const {
    return _connection_info;
  }

  const char *get_ssl_cipher() const {
    return _ssl_cipher.empty() ? nullptr : _ssl_cipher.c_str();
  }

  std::shared_ptr<IResult> execute_stmt(const std::string &ns,
                                        const std::string &stmt,
                                        const ::xcl::Arguments &args);

  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Insert &msg);
  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Update &msg);
  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Delete &msg);
  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Find &msg);

  void load_session_info();

  std::unique_ptr<xcl::XSession> _mysql;
  std::string _ssl_cipher;
  std::string _connection_info;
  std::pair<xcl::XProtocol::Handler_id, xcl::XProtocol::Handler_id>
      _trace_handler;
  uint64_t _connection_id = 0;
  bool _enable_trace = false;
  bool _expired_account = false;
  bool _case_sensitive_table_names = false;

  std::weak_ptr<Result> _prev_result;
  mysqlshdk::db::Connection_options _connection_options;
};

class SHCORE_PUBLIC Session : public ISession,
                              public std::enable_shared_from_this<Session> {
 public:
  static void set_factory_function(
      std::function<std::shared_ptr<Session>()> factory);

  static std::shared_ptr<Session> create();

  void connect(const mysqlshdk::db::Connection_options& data) override {
    _impl->connect(data);
  }

  const mysqlshdk::db::Connection_options &get_connection_options()
      const override {
    return _impl->get_connection_options();
  }

  void close() override {
    _impl->close();
    _impl.reset(new XSession_impl());
  }

  bool valid() const {
    return _impl->valid();
  }

  uint64_t get_connection_id() const {
    return _impl->get_thread_id();
  }

  const char *get_ssl_cipher() const override {
    return _impl->get_ssl_cipher();
  }

  const std::string &get_connection_info() const {
    return _impl->get_connection_info();
  }

  void enable_protocol_trace(bool flag) {
    _impl->enable_trace(flag);
  }

  std::shared_ptr<IResult> query(const std::string &sql,
                                 bool buffered = false) override {
    return _impl->query(sql, buffered);
  }

  void execute(const std::string &sql) override {
    _impl->execute(sql);
  }

  std::shared_ptr<IResult> execute_stmt(const std::string &ns,
                                        const std::string &stmt,
                                        const ::xcl::Arguments &args) {
    return _impl->execute_stmt(ns, stmt, args);
  }

  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Insert &msg) {
    return _impl->execute_crud(msg);
  }

  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Update &msg) {
    return _impl->execute_crud(msg);
  }

  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Delete &msg) {
    return _impl->execute_crud(msg);
  }

  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Find &msg) {
    return _impl->execute_crud(msg);
  }

  bool is_open() const override {
    return valid();
  };

 protected:
  Session() {
    _impl.reset(new XSession_impl());
  }

 private:
  std::shared_ptr<XSession_impl> _impl;
};

}  // namespace mysqlx
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_MYSQLX_SESSION_H_
