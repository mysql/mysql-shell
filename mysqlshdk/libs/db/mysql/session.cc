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

#include "mysqlshdk/libs/db/mysql/session.h"
#include <sstream>
#include <vector>

#include "utils/utils_general.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
//-------------------------- Session Implementation ----------------------------
void Session_impl::throw_on_connection_fail() {
  auto exception =
      shcore::database_error(mysql_error(_mysql), mysql_errno(_mysql),
                             mysql_error(_mysql), mysql_sqlstate(_mysql));
  close();
  throw exception;
}

Session_impl::Session_impl() : _mysql(NULL) {}

void Session_impl::connect(
    const mysqlshdk::db::Connection_options &connection_options) {
  long flags = CLIENT_MULTI_RESULTS |   // NOLINT runtime/int
               CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS;
  int port = 0;
  std::string host;
  std::string user;

  if (connection_options.has_port())
    port = connection_options.get_port();
  else if (!connection_options.has_socket())
    port = 3306;

  host = connection_options.has_host() ? connection_options.get_host()
                                       : "localhost";

  user = connection_options.has_user() ? connection_options.get_user()
                                       : shcore::get_system_user();

  _mysql = mysql_init(NULL);

  setup_ssl(connection_options.get_ssl_options());
  if (port != 0 || (host != "localhost" && host != "")) {
    unsigned int tcp = MYSQL_PROTOCOL_TCP;
    mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &tcp);
  }

  mysql_options(_mysql, MYSQL_OPT_CONNECT_ATTR_RESET, 0);
  mysql_options4(_mysql, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name", "mysqlsh");

#ifdef _WIN32
  // Enable pipe connection if required
  if (!connection_options.has_port() &&
      ((connection_options.has_host() &&
        connection_options.get_host() == ".") ||
       connection_options.has_pipe())) {
    unsigned int pipe = MYSQL_PROTOCOL_PIPE;
    mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &pipe);
  }
#endif

  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(),
                          connection_options.has_password()
                              ? connection_options.get_password().c_str()
                              : "",
                          connection_options.has_schema()
                              ? connection_options.get_schema().c_str()
                              : NULL,
                          port,
                          connection_options.has_socket()
                              ? connection_options.get_socket().c_str()
                              : NULL,
                          flags)) {
    throw_on_connection_fail();
  }
}

bool Session_impl::setup_ssl(
    const mysqlshdk::db::Ssl_options &ssl_options) const {
  int value;

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

  if (ssl_options.has_ciphers())
    mysql_options(_mysql, MYSQL_OPT_SSL_CIPHER,
                  (ssl_options.get_ciphers().c_str()));

  if (ssl_options.has_tls_version())
    mysql_options(_mysql, MYSQL_OPT_TLS_VERSION,
                  (ssl_options.get_tls_version().c_str()));

  if (ssl_options.has_cert())
    mysql_options(_mysql, MYSQL_OPT_SSL_CERT, (ssl_options.get_cert().c_str()));

  if (ssl_options.has_key())
    mysql_options(_mysql, MYSQL_OPT_SSL_KEY, (ssl_options.get_key().c_str()));

  if (ssl_options.has_mode())
    value = ssl_options.get_mode();
  else
    value = static_cast<int>(mysqlshdk::db::Ssl_mode::Preferred);

  mysql_options(_mysql, MYSQL_OPT_SSL_MODE, &value);

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
    throw shcore::database_error(mysql_error(_mysql), mysql_errno(_mysql),
                                 mysql_error(_mysql), mysql_sqlstate(_mysql));
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

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
