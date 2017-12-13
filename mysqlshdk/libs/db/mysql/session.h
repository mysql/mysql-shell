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

#ifndef MYSQLSHDK_LIBS_DB_MYSQL_SESSION_H_
#define MYSQLSHDK_LIBS_DB_MYSQL_SESSION_H_

#include <mysql.h>
#include <mysqld_error.h>
#include <functional>
#include <memory>
#include <set>
#include <string>

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
/*
 * Session implementation for the MySQL protocol.
 *
 * This class can only be used by the Session class in order to guarantee
 * that any instance is always contained on a shared pointer, this is required
 * because the results returned by the query function require a reference to
 * the Session that generated them.
 */
class Session_impl : public std::enable_shared_from_this<Session_impl> {
  friend class Session;  // The Session class instantiates this class
  friend class Result;   // The Result class uses some functions of this class
 public:
  virtual ~Session_impl();

 private:
  Session_impl();
  void connect(const mysqlshdk::db::Connection_options &connection_info);

  std::shared_ptr<IResult> query(const std::string &sql, bool buffered);
  void execute(const std::string &sql);

  void start_transaction();
  void commit();
  void rollback();

  void close();

  bool next_resultset();
  void prepare_fetch(Result *target, bool buffered);

  std::string uri() { return _uri; }

  // Utility functions to retriev session status
  uint64_t get_thread_id() {
    _prev_result.reset();
    if (_mysql)
      return mysql_thread_id(_mysql);
    return 0;
  }
  uint64_t get_protocol_info() {
    _prev_result.reset();
    if (_mysql)
      return mysql_get_proto_info(_mysql);
    return 0;
  }
  const char *get_connection_info() {
    _prev_result.reset();
    if (_mysql)
      return mysql_get_host_info(_mysql);
    return nullptr;
  }
  const char *get_server_info() {
    _prev_result.reset();
    if (_mysql)
      return mysql_get_server_info(_mysql);
    return nullptr;
  }
  const char *get_stats() {
    _prev_result.reset();
    if (_mysql)
      return mysql_stat(_mysql);
    return nullptr;
  }
  const char *get_ssl_cipher() {
    _prev_result.reset();
    if (_mysql)
      return mysql_get_ssl_cipher(_mysql);
    return nullptr;
  }

  virtual mysqlshdk::utils::Version get_server_version() const {
    if (_mysql) {
      auto ulversion = mysql_get_server_version(_mysql);
      return mysqlshdk::utils::Version(
          (ulversion / 10000) % 100, (ulversion / 100) % 100, ulversion % 100);
    }
    return mysqlshdk::utils::Version();
  }

  bool is_open() const { return _mysql ? true: false; }

  const char* get_last_error(int* out_code, const char** out_sqlstate) {
    if (out_code)
      *out_code = mysql_errno(_mysql);
    if (out_sqlstate)
      *out_sqlstate = mysql_sqlstate(_mysql);

    return mysql_error(_mysql);
  }


  const mysqlshdk::db::Connection_options & get_connection_options() const {
    return _connection_options;
  }

  std::shared_ptr<IResult> run_sql(const std::string &sql,
                                   bool lazy_fetch = true);
  bool setup_ssl(const mysqlshdk::db::Ssl_options &ssl_options) const;
  void throw_on_connection_fail();
  std::string _uri;
  MYSQL *_mysql;

  std::shared_ptr<MYSQL_RES> _prev_result;
  mysqlshdk::db::Connection_options _connection_options;
};

class SHCORE_PUBLIC Session : public ISession,
                              public std::enable_shared_from_this<Session> {
 public:
  static void set_factory_function(
      std::function<std::shared_ptr<Session>()> factory);

  static std::shared_ptr<Session> create();

  virtual void connect(
      const mysqlshdk::db::Connection_options &connection_options) {
    _impl->connect(connection_options);
  }

  virtual const mysqlshdk::db::Connection_options &get_connection_options()
      const {
    return _impl->get_connection_options();
  }

  virtual std::shared_ptr<IResult> query(const std::string &sql,
                                         bool buffered = false) {
    return _impl->query(sql, buffered);
  }

  virtual void execute(const std::string &sql) { _impl->execute(sql); }
  virtual void close() { _impl->close(); }
  virtual const char *get_ssl_cipher() const { return _impl->get_ssl_cipher(); }
  virtual bool is_open() const { return _impl->is_open(); }

  virtual uint64_t get_connection_id() const {
    return _impl->get_thread_id();
  }

  virtual uint64_t get_protocol_info() {
    return _impl->get_protocol_info();
  }

  virtual const char *get_connection_info() {
    return _impl->get_connection_info();
  }

  virtual const char *get_server_info() {
    return _impl->get_server_info();
  }

  virtual const char *get_stats() {
    return _impl->get_stats();
  }

  virtual mysqlshdk::utils::Version get_server_version() const {
    return _impl->get_server_version();
  }

  ~Session() {
    if (_impl)
      _impl->close();
  }

 protected:
  Session() {
    _impl.reset(new Session_impl());
  }

 private:
  std::shared_ptr<Session_impl> _impl;
};
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_MYSQL_SESSION_H_
