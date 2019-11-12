/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include <cmath>
#include <sstream>
#include <vector>

#include <mysql_version.h>
#include "my_dbug.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/utils_general.h"

#ifdef _WIN32
typedef unsigned int uint;
#endif

namespace mysqlshdk {
namespace db {
namespace mysql {
//-------------------------- Session Implementation ----------------------------
void Session_impl::throw_on_connection_fail() {
  auto exception = mysqlshdk::db::Error(
      mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));
  DBUG_LOG("sql", mysql_thread_id(_mysql) << ": ERROR: " << exception.format());
  close();
  throw exception;
}

Session_impl::Session_impl() {}

void Session_impl::connect(
    const mysqlshdk::db::Connection_options &connection_options) {
  long flags = CLIENT_MULTI_RESULTS | CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS;
  _mysql = mysql_init(nullptr);

  _connection_options = connection_options;

  setup_ssl(_connection_options.get_ssl_options());
  if (_connection_options.has_transport_type() &&
      _connection_options.get_transport_type() == mysqlshdk::db::Tcp) {
    unsigned int tcp = MYSQL_PROTOCOL_TCP;
    mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &tcp);
  }

  // Connection attributes are only sent if they are not disabled
  if (_connection_options.is_connection_attributes_enabled()) {
    mysql_options(_mysql, MYSQL_OPT_CONNECT_ATTR_RESET, nullptr);
    mysql_options4(_mysql, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name",
                   "mysqlsh");

    for (const auto &att : _connection_options.get_connection_attributes()) {
      std::string attribute = att.first;
      std::string value;
      if (!att.second.is_null()) {
        value = *att.second;
      }

      mysql_options4(_mysql, MYSQL_OPT_CONNECT_ATTR_ADD, attribute.c_str(),
                     value.c_str());
    }
  }

  if (connection_options.has(mysqlshdk::db::kGetServerPublicKey)) {
    const std::string &server_public_key =
        connection_options.get(mysqlshdk::db::kGetServerPublicKey);
    bool get_pub_key = (server_public_key == "true" || server_public_key == "1")
                           ? true
                           : false;
    mysql_options(_mysql, MYSQL_OPT_GET_SERVER_PUBLIC_KEY, &get_pub_key);
  }

  if (connection_options.has(mysqlshdk::db::kServerPublicKeyPath)) {
    std::string server_public_key =
        connection_options.get_extra_options().get_value(
            mysqlshdk::db::kServerPublicKeyPath);
    mysql_options(_mysql, MYSQL_SERVER_PUBLIC_KEY, server_public_key.c_str());
  }

  if (connection_options.has(mysqlshdk::db::kAuthMethod)) {
    const auto &auth = connection_options.get(mysqlshdk::db::kAuthMethod);
    mysql_options(_mysql, MYSQL_DEFAULT_AUTH, auth.c_str());
  }

#ifdef _WIN32
  // Enable pipe connection if required
  if (!_connection_options.has_port() &&
      ((_connection_options.has_host() &&
        _connection_options.get_host() == ".") ||
       _connection_options.has_pipe())) {
    uint pipe = MYSQL_PROTOCOL_PIPE;
    mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &pipe);
  }
#endif

  {  // Sets the connection timeout (comes in milliseconds, must be in seconds)
    uint connect_timeout = mysqlshdk::db::k_default_connect_timeout;

    // TODO(alfredo) Hack for connections to fail fast during testing. Should be
    // replaced by a global defaultConnectTimeout option in the shell.
    connect_timeout = DBUG_EVALUATE_IF("contimeout", 1, connect_timeout);

    if (_connection_options.has(kConnectTimeout)) {
      connect_timeout = std::stoi(_connection_options.get(kConnectTimeout));
    }
    connect_timeout = std::ceil(connect_timeout / 1000.0);
    mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout);
  }

  if (_connection_options.has(kNetReadTimeout)) {
    uint read_timeout = std::stoi(_connection_options.get(kNetReadTimeout));

    read_timeout = std::ceil(read_timeout / 1000.0);
    mysql_options(_mysql, MYSQL_OPT_READ_TIMEOUT, &read_timeout);
  }

  if (_connection_options.has_compression() &&
      _connection_options.get_compression())
    mysql_options(_mysql, MYSQL_OPT_COMPRESS, nullptr);

  if (connection_options.has(mysqlshdk::db::kLocalInfile)) {
    const int local_infile = 1;
    mysql_options(_mysql, MYSQL_OPT_LOCAL_INFILE, &local_infile);
  }

  _mysql->options.local_infile_init = m_local_infile.init;
  _mysql->options.local_infile_read = m_local_infile.read;
  _mysql->options.local_infile_end = m_local_infile.end;
  _mysql->options.local_infile_error = m_local_infile.error;
  _mysql->options.local_infile_userdata = m_local_infile.userdata;

  // set max_allowed_packet and net_buffer_length
  {
    unsigned long opt_max_allowed_packet = 32 * 1024L * 1024L;  // 32MB
    mysql_options(_mysql, MYSQL_OPT_MAX_ALLOWED_PACKET,
                  &opt_max_allowed_packet);
  }

  if (connection_options.has(mysqlshdk::db::kNetBufferLength)) {
    const unsigned long net_buffer_length =
        std::stoul(connection_options.get(kNetBufferLength));
    mysql_options(_mysql, MYSQL_OPT_NET_BUFFER_LENGTH, &net_buffer_length);
  } else {
    unsigned long opt_net_buffer_length = 2 * 1024L * 1024L;
    mysql_options(_mysql, MYSQL_OPT_NET_BUFFER_LENGTH, &opt_net_buffer_length);
  }

  DBUG_LOG("sqlall", "CONNECT: " << _connection_options.uri_endpoint());

  if (!mysql_real_connect(
          _mysql,
          _connection_options.has_host()
              ? _connection_options.get_host().c_str()
              : NULL,
          _connection_options.has_user()
              ? _connection_options.get_user().c_str()
              : NULL,
          _connection_options.has_password()
              ? _connection_options.get_password().c_str()
              : NULL,
          _connection_options.has_schema()
              ? _connection_options.get_schema().c_str()
              : NULL,
          _connection_options.has_port() ? _connection_options.get_port() : 0,
          _connection_options.has_socket() &&
                  !_connection_options.get_socket().empty()
              ? _connection_options.get_socket().c_str()
              : NULL,
          flags)) {
    throw_on_connection_fail();
  }
  DBUG_LOG("sql", get_thread_id()
                      << ": CONNECTED: " << _connection_options.uri_endpoint());

  if (!_connection_options.has_scheme())
    _connection_options.set_scheme("mysql");

  // When neither port or socket were specified on the connection data
  // it means it was able to use default connection data
  if (!_connection_options.has_port() && !_connection_options.has_socket()) {
    std::string connection_info(get_connection_info());

    // If connection is through TCP/IP it means the default port was used
    if (connection_info.find("via TCP/IP") != std::string::npos) {
      _connection_options.set_port(MYSQL_PORT);
    } else {
      // If connection was not through TCP/IP it means either the default socket
      // path or windows named pipe was used
#ifdef _WIN32
      _connection_options.set_pipe("MySQL");
#else
      _connection_options.set_socket(mysql_unix_port);
#endif
    }
  }
}

bool Session_impl::setup_ssl(
    const mysqlshdk::db::Ssl_options &ssl_options) const {
  int value;

  mysqlshdk::db::Ssl_mode ssl_mode;
  if (ssl_options.has_data()) {
    ssl_options.validate();

    if (!ssl_options.has_value(mysqlshdk::db::kSslMode)) {
      if (ssl_options.has_value(mysqlshdk::db::kSslCa) ||
          ssl_options.has_value(mysqlshdk::db::kSslCaPath)) {
        ssl_mode = mysqlshdk::db::Ssl_mode::VerifyCa;
      } else {
        ssl_mode = mysqlshdk::db::Ssl_mode::Preferred;
      }
    } else {
      ssl_mode = ssl_options.get_mode();
    }

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

    if (ssl_options.has_tls_ciphersuites())
      mysql_options(_mysql, MYSQL_OPT_TLS_CIPHERSUITES,
                    (ssl_options.get_tls_ciphersuites().c_str()));

    if (ssl_options.has_cert())
      mysql_options(_mysql, MYSQL_OPT_SSL_CERT,
                    (ssl_options.get_cert().c_str()));

    if (ssl_options.has_key())
      mysql_options(_mysql, MYSQL_OPT_SSL_KEY, (ssl_options.get_key().c_str()));

    value = static_cast<int>(ssl_mode);
    mysql_options(_mysql, MYSQL_OPT_SSL_MODE, &value);
  }

  return true;
}

void Session_impl::close() {
  // This should be logged, for now commenting to
  // avoid having unneeded output on the script mode
  if (_prev_result) _prev_result.reset();

  if (_mysql) {
    DBUG_LOG("sql", get_thread_id() << ": DISCONNECT");
    mysql_close(_mysql);
    _mysql = nullptr;
  }
}

std::shared_ptr<IResult> Session_impl::query(const char *sql, size_t len,
                                             bool buffered) {
  return run_sql(sql, len, buffered);
}

void Session_impl::execute(const char *sql, size_t len) {
  auto result = run_sql(sql, len, true);
}

std::shared_ptr<IResult> Session_impl::run_sql(const char *sql, size_t len,
                                               bool buffered) {
  if (_mysql == nullptr) throw std::runtime_error("Not connected");
  mysqlshdk::utils::Profile_timer timer;
  timer.stage_begin("run_sql");
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

  DBUG_LOG("sqlall", get_thread_id() << ": QUERY: " << std::string(sql, len));

  if (mysql_real_query(_mysql, sql, len) != 0) {
    auto err =
        Error(mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));

    if (DBUG_EVALUATE_IF("sqlall", 1, 0)) {
      DBUG_LOG("sql", get_thread_id() << ": ERROR: " << err.format());
    } else {
      DBUG_LOG("sql", get_thread_id()
                          << ": ERROR: " << err.format()
                          << "\n\twhile executing: " << std::string(sql, len));
    }

    throw err;
  }

  std::shared_ptr<Result> result(
      new Result(shared_from_this(), mysql_affected_rows(_mysql),
                 mysql_warning_count(_mysql), mysql_insert_id(_mysql),
                 mysql_info(_mysql), buffered));

  timer.stage_end();
  result->set_execution_time(timer.total_seconds_ellapsed());
  return std::static_pointer_cast<IResult>(result);
}

template <class T>
static void free_result(T *result) {
  mysql_free_result(result);
  result = NULL;
}

bool Session_impl::next_resultset() {
  if (_prev_result) _prev_result.reset();

  return mysql_next_result(_mysql) == 0;
}

void Session_impl::prepare_fetch(Result *target) {
  MYSQL_RES *result;

  if (target->is_buffered())
    result = mysql_store_result(_mysql);
  else
    result = mysql_use_result(_mysql);

  if (result)
    _prev_result = std::shared_ptr<MYSQL_RES>(result, &free_result<MYSQL_RES>);

  if (_prev_result) {
    // We need to update the received result object with the information
    // for the next result set
    target->reset(_prev_result);
  } else {
    // Update the result object for stmts that don't return a result
    target->reset(nullptr);
  }
}

Session_impl::~Session_impl() { close(); }

std::vector<std::string> Session_impl::get_last_gtids() const {
  const char *data;
  size_t length;
  std::vector<std::string> gtids;

  if (mysql_session_track_get_first(_mysql, SESSION_TRACK_GTIDS, &data,
                                    &length) == 0) {
    gtids.emplace_back(data, length);

    while (mysql_session_track_get_next(_mysql, SESSION_TRACK_GTIDS, &data,
                                        &length) == 0) {
      gtids.emplace_back(std::string(data, length));
    }
  }
  return gtids;
}

std::function<std::shared_ptr<Session>()> g_session_factory;

std::function<std::shared_ptr<Session>()> Session::set_factory_function(
    std::function<std::shared_ptr<Session>()> factory) {
  auto old = g_session_factory;
  g_session_factory = factory;
  return old;
}

std::shared_ptr<Session> Session::create() {
  if (g_session_factory) return g_session_factory();
  return std::shared_ptr<Session>(new Session());
}

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
