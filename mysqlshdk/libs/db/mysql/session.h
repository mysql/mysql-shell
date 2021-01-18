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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MYSQLSHDK_LIBS_DB_MYSQL_SESSION_H_
#define MYSQLSHDK_LIBS_DB_MYSQL_SESSION_H_

#include <mysql.h>
#include <mysqld_error.h>

#include <cstring>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

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

  std::shared_ptr<IResult> query(const char *sql, size_t len, bool buffered);
  void execute(const char *sql, size_t len);

  inline void execute(const char *sql) { execute(sql, ::strlen(sql)); }

  void start_transaction();
  void commit();
  void rollback();

  void close();

  bool next_resultset();
  void prepare_fetch(Result *target);

  std::string uri() { return _uri; }

  // Utility functions to retrieve session status
  uint64_t get_thread_id() {
    if (_mysql) return mysql_thread_id(_mysql);
    return 0;
  }
  uint64_t get_protocol_info() {
    if (_mysql) return mysql_get_proto_info(_mysql);
    return 0;
  }
  bool is_compression_enabled() const {
    return _mysql ? _mysql->net.compress : false;
  }
  const char *get_connection_info() {
    if (_mysql) return mysql_get_host_info(_mysql);
    return nullptr;
  }
  const char *get_server_info() {
    if (_mysql) return mysql_get_server_info(_mysql);
    return nullptr;
  }
  const char *get_stats() {
    _prev_result.reset();
    if (_mysql) return mysql_stat(_mysql);
    return nullptr;
  }
  const char *get_ssl_cipher() {
    if (_mysql) return mysql_get_ssl_cipher(_mysql);
    return nullptr;
  }

  const char *get_mysql_info() const { return mysql_info(_mysql); }

  virtual mysqlshdk::utils::Version get_server_version() const {
    if (_mysql) {
      auto ulversion = mysql_get_server_version(_mysql);
      return mysqlshdk::utils::Version(
          (ulversion / 10000) % 100, (ulversion / 100) % 100, ulversion % 100);
    }
    return mysqlshdk::utils::Version();
  }

  bool is_open() const { return _mysql ? true : false; }

  const char *get_last_error(int *out_code, const char **out_sqlstate) {
    if (out_code) *out_code = mysql_errno(_mysql);
    if (out_sqlstate) *out_sqlstate = mysql_sqlstate(_mysql);

    return mysql_error(_mysql);
  }

  Error *get_last_error() {
    if (nullptr == _mysql) {
      m_last_error.reset(nullptr);
    } else {
      int code = 0;
      const char *sqlstate = nullptr;
      const char *error = get_last_error(&code, &sqlstate);

      if (code == 0) {
        m_last_error.reset(nullptr);
      } else {
        m_last_error.reset(new Error{error, code, sqlstate});
      }
    }

    return m_last_error.get();
  }

  std::vector<std::string> get_last_gtids() const;

  uint64_t warning_count() const {
    return _mysql ? mysql_warning_count(_mysql) : 0;
  }

  const mysqlshdk::db::Connection_options &get_connection_options() const {
    return _connection_options;
  }

  std::shared_ptr<IResult> run_sql(const char *sql, size_t len,
                                   bool lazy_fetch = true);
  // Will return the SSL mode set in the connection, if any
  mysqlshdk::utils::nullable<mysqlshdk::db::Ssl_mode> setup_ssl(
      const mysqlshdk::db::Ssl_options &ssl_options) const;
  void throw_on_connection_fail();

  void setup_default_character_set();

  std::string _uri;
  MYSQL *_mysql = nullptr;
  std::shared_ptr<MYSQL_RES> _prev_result;
  mysqlshdk::db::Connection_options _connection_options;
  std::unique_ptr<Error> m_last_error;

  struct Local_infile_callbacks {
    int (*init)(void **, const char *, void *) = nullptr;
    int (*read)(void *, char *, unsigned int) = nullptr;
    void (*end)(void *) = nullptr;
    int (*error)(void *, char *, unsigned int) = nullptr;
    void *userdata = nullptr;
  };
  Local_infile_callbacks m_local_infile;
};

class SHCORE_PUBLIC Session : public ISession,
                              public std::enable_shared_from_this<Session> {
 public:
  static std::function<std::shared_ptr<Session>()> set_factory_function(
      std::function<std::shared_ptr<Session>()> factory);

  static std::shared_ptr<Session> create();

  const mysqlshdk::db::Connection_options &get_connection_options()
      const override {
    return _impl->get_connection_options();
  }

  std::shared_ptr<IResult> querys(const char *sql, size_t len,
                                  bool buffered = false) override {
    return _impl->query(sql, len, buffered);
  }

  void executes(const char *sql, size_t len) override {
    _impl->execute(sql, len);
  }

  const char *get_ssl_cipher() const override {
    return _impl->get_ssl_cipher();
  }
  bool is_open() const override { return _impl->is_open(); }

  uint64_t get_connection_id() const override { return _impl->get_thread_id(); }

  virtual uint64_t get_protocol_info() { return _impl->get_protocol_info(); }

  virtual bool is_compression_enabled() const {
    return _impl->is_compression_enabled();
  }

  virtual const char *get_connection_info() {
    return _impl->get_connection_info();
  }

  virtual const char *get_server_info() { return _impl->get_server_info(); }

  virtual const char *get_stats() { return _impl->get_stats(); }

  std::string escape_string(const std::string &s) const override;
  std::string escape_string(const char *buffer, size_t len) const override;

  mysqlshdk::utils::Version get_server_version() const override {
    return _impl->get_server_version();
  }

  const Error *get_last_error() const override {
    return _impl->get_last_error();
  }

  const char *get_mysql_info() const { return _impl->get_mysql_info(); }

  // function callback registration for local infile support
  void set_local_infile_init(int (*local_infile_init)(void **, const char *,
                                                      void *)) {
    _impl->m_local_infile.init = local_infile_init;
    if (_impl->_mysql)
      _impl->_mysql->options.local_infile_init = local_infile_init;
  }

  void set_local_infile_read(int (*local_infile_read)(void *, char *,
                                                      unsigned int)) {
    _impl->m_local_infile.read = local_infile_read;
    if (_impl->_mysql)
      _impl->_mysql->options.local_infile_read = local_infile_read;
  }

  void set_local_infile_end(void (*local_infile_end)(void *)) {
    _impl->m_local_infile.end = local_infile_end;
    if (_impl->_mysql)
      _impl->_mysql->options.local_infile_end = local_infile_end;
  }

  void set_local_infile_error(int (*local_infile_error)(void *, char *,
                                                        unsigned int)) {
    _impl->m_local_infile.error = local_infile_error;
    if (_impl->_mysql)
      _impl->_mysql->options.local_infile_error = local_infile_error;
  }

  void set_local_infile_userdata(void *local_infile_userdata) {
    _impl->m_local_infile.userdata = local_infile_userdata;
    if (_impl->_mysql)
      _impl->_mysql->options.local_infile_userdata = local_infile_userdata;
  }

  socket_t get_socket_fd() const override {
    if (!_impl || !_impl->_mysql || _impl->_mysql->net.fd == -1)
      throw std::invalid_argument("Invalid session");

    return _impl->_mysql->net.fd;
  }

  ~Session() override { close(); }

 protected:
  Session() { _impl.reset(new Session_impl()); }

  void do_connect(
      const mysqlshdk::db::Connection_options &connection_options) override {
    _impl->connect(connection_options);
    _impl->setup_default_character_set();
  }

  void do_close() override { _impl->close(); }

 private:
  std::shared_ptr<Session_impl> _impl;
};

inline std::shared_ptr<Session> open_session(
    const mysqlshdk::db::Connection_options &copts) {
  auto session = Session::create();
  session->connect(copts);
  return session;
}

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_MYSQL_SESSION_H_
