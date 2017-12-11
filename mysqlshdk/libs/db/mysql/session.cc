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

#include "mysqlshdk/libs/db/mysql/session.h"
#include <sstream>
#include <vector>

#include "utils/utils_general.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
//-------------------------- Session Implementation ----------------------------
void Session_impl::throw_on_connection_fail() {
  auto exception = mysqlshdk::db::Error(
      mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  close();
  throw exception;
}

Session_impl::Session_impl() : _mysql(NULL) {
}

void Session_impl::connect(
    const mysqlshdk::db::Connection_options &connection_options) {
  long flags = CLIENT_MULTI_RESULTS | CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS;
  _mysql = mysql_init(NULL);

  _connection_options = connection_options;

  // All connections should use mode = VERIFY_CA if no ssl mode is specified
  // and either ssl-ca or ssl-capath are specified
  if (!_connection_options.has_value(mysqlshdk::db::kSslMode) &&
      (_connection_options.has_value(mysqlshdk::db::kSslCa) ||
       _connection_options.has_value(mysqlshdk::db::kSslCaPath))) {
    _connection_options.set(mysqlshdk::db::kSslMode,
                           {mysqlshdk::db::kSslModeVerifyCA});
  }

  setup_ssl(_connection_options.get_ssl_options());
  if (_connection_options.has_transport_type() &&
      _connection_options.get_transport_type() == mysqlshdk::db::Tcp) {
    unsigned int tcp = MYSQL_PROTOCOL_TCP;
    mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &tcp);
  }

  mysql_options(_mysql, MYSQL_OPT_CONNECT_ATTR_RESET, 0);
  mysql_options4(_mysql, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name", "mysqlsh");

#ifdef _WIN32
  // Enable pipe connection if required
  if (!_connection_options.has_port() &&
      ((_connection_options.has_host() &&
        _connection_options.get_host() == ".") ||
       _connection_options.has_pipe())) {
    unsigned int pipe = MYSQL_PROTOCOL_PIPE;
    mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &pipe);
  }
#endif

  if (!mysql_real_connect(_mysql,
                          _connection_options.has_host() ?
                          _connection_options.get_host().c_str() : NULL,
                          _connection_options.has_user() ?
                          _connection_options.get_user().c_str() : NULL,
                          _connection_options.has_password() ?
                          _connection_options.get_password().c_str() : NULL,
                          _connection_options.has_schema() ?
                          _connection_options.get_schema().c_str() : NULL,
                          _connection_options.has_port() ?
                          _connection_options.get_port() : 0,
                          _connection_options.has_socket() ?
                          _connection_options.get_socket().c_str() : NULL,
                          flags)) {
    throw_on_connection_fail();
  }

  if (!_connection_options.has_scheme())
    _connection_options.set_scheme("mysql");

  if (!_connection_options.has_port() && !_connection_options.has_socket()) {
    std::string connection_info(get_connection_info());
    if (connection_info.find("via TCP/IP") != std::string::npos) {
      _connection_options.set_port(3306);
    } else {
      std::string variable;
#ifdef _WIN32
      variable = "named_pipe";
#else
      variable = "socket";
#endif
      auto result = query("show variables like '" + variable + "'", true);
      auto row = result->fetch_one();
      std::string value = row->get_as_string(1);
      if (!value.empty()) {
#ifdef _WIN32
        _connection_options.set_pipe(value);
#else
        _connection_options.set_socket(value);
#endif
      }
    }
  }
}

bool Session_impl::setup_ssl(
    const mysqlshdk::db::Ssl_options &ssl_options) const {
  int value;

  if (ssl_options.has_data()) {
    ssl_options.validate();

    if (ssl_options.has_ca())
      mysql_options(_mysql, MYSQL_OPT_SSL_CA, (ssl_options.get_ca().c_str()));

    if (ssl_options.has_capath())
      mysql_options(_mysql, MYSQL_OPT_SSL_CAPATH,
                    (ssl_options.get_capath().c_str()));

    if (ssl_options.has_crl())
      mysql_options(_mysql, MYSQL_OPT_SSL_CRL, (ssl_options.get_crl().c_str()));

    if (ssl_options.has_crlpath())
      mysql_options(_mysql, MYSQL_OPT_SSL_CRLPATH,
                    (ssl_options.get_crlpath().c_str()));

    if (ssl_options.has_cipher())
      mysql_options(_mysql, MYSQL_OPT_SSL_CIPHER,
                    (ssl_options.get_cipher().c_str()));

    if (ssl_options.has_tls_version())
      mysql_options(_mysql, MYSQL_OPT_TLS_VERSION,
                    (ssl_options.get_tls_version().c_str()));

    if (ssl_options.has_cert())
      mysql_options(_mysql, MYSQL_OPT_SSL_CERT, (ssl_options.get_cert().c_str()));

    if (ssl_options.has_key())
      mysql_options(_mysql, MYSQL_OPT_SSL_KEY, (ssl_options.get_key().c_str()));

    if (ssl_options.has_mode())
      value = static_cast<int>(ssl_options.get_mode());
    else
      value = static_cast<int>(mysqlshdk::db::Ssl_mode::Preferred);

    mysql_options(_mysql, MYSQL_OPT_SSL_MODE, &value);
  }

  return true;
}

void Session_impl::close() {
  // This should be logged, for now commenting to
  // avoid having unneeded output on the script mode
  if (_prev_result)
    _prev_result.reset();

  if (_mysql)
    mysql_close(_mysql);
  _mysql = nullptr;
}

std::shared_ptr<IResult> Session_impl::query(const std::string &sql,
                                             bool buffered) {
  return run_sql(sql, buffered);
}

void Session_impl::execute(const std::string &sql) {
  auto result = run_sql(sql, true);
}

std::shared_ptr<IResult> Session_impl::run_sql(const std::string &query,
                                               bool buffered) {
  if (_mysql == nullptr)
    throw std::runtime_error("Not connected");
  if (_prev_result) {
    _prev_result.reset();
  } else {
    MYSQL_RES *unread_result = mysql_use_result(_mysql);
    mysql_free_result(unread_result);
  }

  // Discards any pending result
  while (mysql_next_result(_mysql) == 0) {
    MYSQL_RES *trailing_result = mysql_use_result(_mysql);
    mysql_free_result(trailing_result);
  }

  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0) {
    throw Error(mysql_error(_mysql), mysql_errno(_mysql),
                mysql_sqlstate(_mysql));
  }

  std::shared_ptr<Result> result(
      new Result(shared_from_this(), mysql_affected_rows(_mysql),
                 mysql_warning_count(_mysql), mysql_insert_id(_mysql),
                 mysql_info(_mysql)));

  prepare_fetch(result.get(), buffered);

  return std::static_pointer_cast<IResult>(result);
}

template <class T>
static void free_result(T *result) {
  mysql_free_result(result);
  result = NULL;
}

bool Session_impl::next_resultset() {
  if (_prev_result)
    _prev_result.reset();

  return mysql_next_result(_mysql) == 0;
}

void Session_impl::prepare_fetch(Result *target, bool buffered) {
  Result *real_target = dynamic_cast<Result *>(target);

  MYSQL_RES *result;

  if (buffered)
    result = mysql_store_result(_mysql);
  else
    result = mysql_use_result(_mysql);

  if (result)
    _prev_result = std::shared_ptr<MYSQL_RES>(result, &free_result<MYSQL_RES>);

  if (_prev_result) {
    // We need to update the received result object with the information
    // for the next result set
    real_target->reset(_prev_result);

    real_target->fetch_metadata();
  }
}

Session_impl::~Session_impl() {
  _prev_result.reset();
  close();
}


std::function<std::shared_ptr<Session>()> g_session_factory;

void Session::set_factory_function(
    std::function<std::shared_ptr<Session>()> factory) {
  g_session_factory = factory;
}

std::shared_ptr<Session> Session::create() {
  if (g_session_factory)
    return g_session_factory();
  return std::shared_ptr<Session>(new Session());
}

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
