/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#include <cstring>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include "mysqlshdk/libs/db/mysqlx/mysqlxclient_clean.h"

#include "mysqlshdk/libs/db/mysqlx/result.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlshdk {
namespace db {
namespace mysqlx {

struct GlobalNotice {
  enum Type { GRViewChanged };

  struct GRViewInfo {
    std::string view_id;
  };

  struct {
    GRViewInfo gr_view_change;
  } info;

  Type type;
};

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
  uint32_t m_prep_stmt_count;
  std::set<uint32_t> m_prepared_statements;

  XSession_impl();
  void connect(const mysqlshdk::db::Connection_options &data);

  const mysqlshdk::db::Connection_options &get_connection_options() {
    return _connection_options;
  }

  void enable_trace(bool flag);

  std::shared_ptr<IResult> query(const char *sql, size_t len,
                                 bool buffered = false);
  void execute(const char *sql, size_t len);

  inline void execute(const char *sql) { execute(sql, ::strlen(sql)); }

  void close();

  void before_query();

  bool valid() const { return _mysql.get() != nullptr; }

  std::shared_ptr<IResult> after_query(
      std::unique_ptr<xcl::XQuery_result> result, bool buffered = false);

  // Utility functions to retriev session status
  uint64_t get_thread_id() const { return _connection_id; }

  const std::string &get_connection_info() const { return _connection_info; }

  const char *get_ssl_cipher() const {
    return _ssl_cipher.empty() ? nullptr : _ssl_cipher.c_str();
  }

  mysqlshdk::utils::Version get_server_version() const { return _version; }

  std::shared_ptr<IResult> execute_stmt(const std::string &ns,
                                        const std::string &stmt,
                                        const ::xcl::Argument_array &args);

  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Insert &msg);
  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Update &msg);
  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Delete &msg);
  std::shared_ptr<IResult> execute_crud(const ::Mysqlx::Crud::Find &msg);

  uint32_t next_prep_stmt_id() { return ++m_prep_stmt_count; }
  void prepare_stmt(const ::Mysqlx::Prepare::Prepare &msg);

  std::shared_ptr<IResult> execute_prep_stmt(
      const ::Mysqlx::Prepare::Execute &msg);

  void deallocate_prep_stmt(uint32_t stmt_id);

  void enable_notices(const std::vector<GlobalNotice::Type> &types);

  /** Registers a callback called when an async notice is received
   *
   * Callback must return true to keep it registered, false to
   * unregister it from further calls.
   */
  void add_notice_listener(
      const std::function<bool(const GlobalNotice &)> &listener);

  void load_session_info();

  void check_error_and_throw(const xcl::XError &error,
                             const char *context = nullptr);

  void store_error_and_throw(const Error &error, const char *context = nullptr);

  Error *get_last_error() const { return m_last_error.get(); }

  void setup_default_character_set();

  std::unique_ptr<xcl::XSession> _mysql;
  std::string _ssl_cipher;
  std::string _connection_info;
  std::pair<xcl::XProtocol::Handler_id, xcl::XProtocol::Handler_id>
      _trace_handler;
  mysqlshdk::utils::Version _version;
  uint64_t _connection_id = 0;
  bool _enable_trace = false;
  bool _expired_account = false;
  bool _case_sensitive_table_names = false;

  std::weak_ptr<Result> _prev_result;
  mysqlshdk::db::Connection_options _connection_options;
  std::unique_ptr<Error> m_last_error;

  std::list<std::function<bool(const GlobalNotice &)>> m_notice_listeners;
  xcl::Handler_result global_notice_handler(
      const xcl::XProtocol *, const bool /*is_global*/,
      const Mysqlx::Notice::Frame::Type type, const char *, const uint32_t);
  bool m_handler_installed = false;
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

  uint64_t get_connection_id() const override { return _impl->get_thread_id(); }

  const char *get_ssl_cipher() const override {
    return _impl->get_ssl_cipher();
  }

  virtual const std::string &get_connection_info() const {
    return _impl->get_connection_info();
  }

  mysqlshdk::utils::Version get_server_version() const override {
    return _impl->get_server_version();
  }

  void enable_protocol_trace(bool flag) { _impl->enable_trace(flag); }

  std::shared_ptr<IResult> querys(const char *sql, size_t len,
                                  bool buffered = false) override {
    return _impl->query(sql, len, buffered);
  }

  std::shared_ptr<IResult> query_udf(std::string_view sql,
                                     bool buffered = false) override {
    return _impl->query(sql.data(), sql.size(), buffered);
  }

  void executes(const char *sql, size_t len) override {
    _impl->execute(sql, len);
  }

  virtual std::shared_ptr<IResult> execute_stmt(
      const std::string &ns, const std::string &stmt,
      const ::xcl::Argument_array &args) {
    return _impl->execute_stmt(ns, stmt, args);
  }

  virtual std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Insert &msg) {
    return _impl->execute_crud(msg);
  }

  virtual std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Update &msg) {
    return _impl->execute_crud(msg);
  }

  virtual std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Delete &msg) {
    return _impl->execute_crud(msg);
  }

  virtual std::shared_ptr<IResult> execute_crud(
      const ::Mysqlx::Crud::Find &msg) {
    return _impl->execute_crud(msg);
  }

  uint32_t next_prep_stmt_id() { return _impl->next_prep_stmt_id(); }
  void prepare_stmt(const ::Mysqlx::Prepare::Prepare &msg) {
    _impl->prepare_stmt(msg);
  }

  std::shared_ptr<IResult> execute_prep_stmt(
      const ::Mysqlx::Prepare::Execute &msg) {
    return _impl->execute_prep_stmt(msg);
  }

  void deallocate_prep_stmt(uint32_t id) { _impl->deallocate_prep_stmt(id); }

  bool is_open() const override { return _impl->valid(); };

  const Error *get_last_error() const override {
    return _impl->get_last_error();
  }

  std::string escape_string(const std::string &s) const override;
  std::string escape_string(const char *buffer, size_t len) const override;

  socket_t get_socket_fd() const override {
    if (!_impl || !_impl->_mysql) throw std::logic_error("Invalid session");
    return _impl->_mysql->get_protocol().get_connection().get_socket_fd();
  }

  ~Session() override { close(); }

  void enable_notices(const std::vector<GlobalNotice::Type> &types) {
    _impl->enable_notices(types);
  }

  void add_notice_listener(
      const std::function<bool(const GlobalNotice &)> &listener) {
    _impl->add_notice_listener(listener);
  }

 public:
  xcl::XSession *get_driver_obj() { return _impl->_mysql.get(); }

 protected:
  Session() { _impl.reset(new XSession_impl()); }

  void do_connect(const mysqlshdk::db::Connection_options &data) override {
    _impl->connect(data);
    _impl->setup_default_character_set();
  }

  void do_close() override { _impl->close(); }

 private:
  std::shared_ptr<XSession_impl> _impl;
};

inline std::shared_ptr<Session> open_session(
    const mysqlshdk::db::Connection_options &copts) {
  auto session = Session::create();
  session->connect(copts);
  return session;
}

}  // namespace mysqlx
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_MYSQLX_SESSION_H_
