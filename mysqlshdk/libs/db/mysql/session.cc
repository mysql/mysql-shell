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

#include "mysqlshdk/libs/db/mysql/session.h"

#include <mysql_version.h>
#include <cmath>
#include <mutex>
#include <regex>
#include <sstream>
#include <vector>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/auth_plugins/fido.h"
#include "mysqlshdk/libs/db/mysql/auth_plugins/mysql_event_handler_plugin.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/log_sql.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/utils_general.h"

#ifdef _WIN32
typedef unsigned int uint;
#endif

namespace mysqlshdk {
namespace db {
namespace mysql {
namespace {
std::once_flag trace_register_flag;
}

FI_DEFINE(mysql, [](const mysqlshdk::utils::FI::Args &args) {
  if (args.get_int("abort", 0)) {
    abort();
  }
  if (args.get_int("code", -1) < 0) {
    throw std::logic_error(args.get_string("msg"));
  }
  throw mysqlshdk::db::Error(args.get_string("msg").c_str(),
                             args.get_int("code"),
                             args.get_string("state", {""}).c_str());
});

//-------------------------- Session Implementation ----------------------------
void Session_impl::throw_on_connection_fail() {
  unsigned int code = mysql_errno(_mysql);

  auto exception =
      mysqlshdk::db::Error(mysql_error(_mysql), code, mysql_sqlstate(_mysql));
  DBUG_LOG("sql", mysql_thread_id(_mysql) << ": ERROR: " << exception.format());

  close();

  // When the connection is done through SSH tunnel and the tunnel fails to
  // open, this error is received from the server
  if (code == CR_SERVER_LOST &&
      _connection_options.get_ssh_options().has_data()) {
    throw mysqlshdk::db::Error(
        shcore::str_format("Error opening MySQL session through SSH tunnel: %s",
                           exception.what()),
        code);
  }
  throw exception;
}

Session_impl::Session_impl() {}

void Session_impl::connect(
    const mysqlshdk::db::Connection_options &connection_options) {
  long flags = CLIENT_MULTI_RESULTS | CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS;
  _mysql = mysql_init(nullptr);

  // A tracer plugin needs to be enabled as soon as the first instance of MYSQL
  // is created in order to properly support FIDO authentication as it is the
  // tracer who can identify when authentication_fido_plugin is being used so
  // the print callback can be set.
  std::call_once(trace_register_flag, register_tracer_plugin, _mysql);

  _connection_options = connection_options;

  auto used_ssl_mode = setup_ssl(_connection_options.get_ssl_options());
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

    if (auth == kAuthMethodClearPassword) {
      if (!connection_options.has_socket() && !connection_options.has_pipe()) {
        auto final_ssl_mode =
            used_ssl_mode.get_safe(mysqlshdk::db::Ssl_mode::Disabled);

        if (final_ssl_mode == mysqlshdk::db::Ssl_mode::Disabled) {
          throw std::runtime_error(
              "Clear password authentication is not supported over insecure "
              "channels.");
        } else if (final_ssl_mode == mysqlshdk::db::Ssl_mode::Preferred) {
          throw std::runtime_error(
              "Clear password authentication requires a secure channel, please "
              "use ssl-mode=REQUIRED to guarantee a secure channel.");
        }
      }
      uint value = 1;
      mysql_options(_mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN, &value);
    }

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
    uint connect_timeout = mysqlshdk::db::default_connect_timeout();

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

  if (_connection_options.has(kNetWriteTimeout)) {
    uint write_timeout = std::stoi(_connection_options.get(kNetWriteTimeout));

    write_timeout = std::ceil(write_timeout / 1000.0);
    mysql_options(_mysql, MYSQL_OPT_WRITE_TIMEOUT, &write_timeout);
  }

  if (_connection_options.has_compression()) {
    auto compress = _connection_options.get_compression();
    if (!compress.empty() && compress != kCompressionDisabled)
      mysql_options(_mysql, MYSQL_OPT_COMPRESS, nullptr);
  }

  if (_connection_options.has_compression_algorithms())
    mysql_options(_mysql, MYSQL_OPT_COMPRESSION_ALGORITHMS,
                  _connection_options.get_compression_algorithms().c_str());

  if (_connection_options.has_compression_level()) {
    uint zcl = static_cast<uint>(_connection_options.get_compression_level());
    if (zcl < 1 || zcl > 22)
      throw std::invalid_argument(
          "Valid range for zstd compression level in classic protocol is "
          "1-22.");
    mysql_options(_mysql, MYSQL_OPT_ZSTD_COMPRESSION_LEVEL, &zcl);
  }

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
  // Note: Manual is wrong about docs for this, but my understanding is:
  // max_allowed_packet sets the maximum size of a full packet that can
  // be received from the server, such as a single row.
  // net_buffer_length is the size of the buffer used for reading results
  // packets from the server. Multiple buffer reads may be needed to completely
  // read a packet, so net_buffer_length can be smaller, but max_allowed_packet
  // must be as big as the biggest expected row to be fetched.
  if (connection_options.has(mysqlshdk::db::kMaxAllowedPacket)) {
    const unsigned long max_allowed_packet =  // NOLINT(runtime/int)
        std::stoul(connection_options.get(kMaxAllowedPacket));
    mysql_options(_mysql, MYSQL_OPT_MAX_ALLOWED_PACKET, &max_allowed_packet);
  } else {
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

  if (connection_options.has(mysqlshdk::db::kMysqlPluginDir)) {
    auto mysql_plugin_dir =
        connection_options.get(mysqlshdk::db::kMysqlPluginDir);
    if (!mysql_plugin_dir.empty()) {
      mysql_options(_mysql, MYSQL_PLUGIN_DIR, mysql_plugin_dir.c_str());
    }
  }

  DBUG_LOG("sqlall", "CONNECT: " << _connection_options.uri_endpoint());

  // If this fails Mfa_passwords definition has to be changed
  static_assert(MAX_AUTH_FACTORS == 3);
  auto passwords = _connection_options.get_mfa_passwords();
  for (unsigned int factor = 1; factor <= passwords.size(); factor++) {
    if (passwords[factor - 1]) {
      mysql_options4(_mysql, MYSQL_OPT_USER_PASSWORD, &factor,
                     passwords[factor - 1]->c_str());
    }
  }

  const char *host = _connection_options.has_host() &&
                             !_connection_options.get_ssh_options().has_data()
                         ? _connection_options.get_host().c_str()
                         : nullptr;

  if (!mysql_real_connect(_mysql, host,
                          _connection_options.has_user()
                              ? _connection_options.get_user().c_str()
                              : NULL,
                          NULL,
                          _connection_options.has_schema()
                              ? _connection_options.get_schema().c_str()
                              : NULL,
                          _connection_options.get_target_port(),
                          _connection_options.has_socket() &&
                                  !_connection_options.get_socket().empty()
                              ? _connection_options.get_socket().c_str()
                              : NULL,
                          flags)) {
    throw_on_connection_fail();
  }

  if (connection_options.has(mysqlshdk::db::kFidoRegisterFactor)) {
    auto factor = connection_options.get(mysqlshdk::db::kFidoRegisterFactor);
    fido::register_device(_mysql, factor.c_str());
  }

  DBUG_LOG("sql", get_thread_id()
                      << ": CONNECTED: " << _connection_options.uri_endpoint());
  {
    auto log_sql_handler = shcore::current_log_sql();
    log_sql_handler->log_connect(_connection_options.uri_endpoint(),
                                 get_thread_id());
  }

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

mysqlshdk::utils::nullable<mysqlshdk::db::Ssl_mode> Session_impl::setup_ssl(
    const mysqlshdk::db::Ssl_options &ssl_options) const {
  int value;

  mysqlshdk::utils::nullable<mysqlshdk::db::Ssl_mode> ssl_mode;
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

    value = static_cast<int>(*ssl_mode);
    mysql_options(_mysql, MYSQL_OPT_SSL_MODE, &value);
  }

  return ssl_mode;
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
  return run_sql(sql, len, buffered, false);
}

std::shared_ptr<IResult> Session_impl::query_udf(std::string_view sql,
                                                 bool buffered) {
  return run_sql(sql.data(), sql.size(), buffered, true);
}

void Session_impl::execute(const char *sql, size_t len) {
  auto result = run_sql(sql, len, true, false);
}

std::shared_ptr<IResult> Session_impl::run_sql(const char *sql, size_t len,
                                               bool buffered, bool is_udf) {
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

  DBUG_EXECUTE_IF("sql_test_abort", {
    static int count = std::stoi(getenv("TEST_SQL_UNTIL_CRASH"));
    if (--count < 1) {
      fprintf(stderr, "%s SQL statements executed, will abort() now\n",
              getenv("TEST_SQL_UNTIL_CRASH"));
      abort();
    }
  });

  DBUG_EXECUTE_IF("sql_test_error", {
    static int filter_set = -1;
    static std::regex filter;
    if (filter_set < 0) {
      const char *pat = getenv("TEST_SQL_UNTIL_CRASH_IF");
      if (pat) {
        filter = std::regex(pat, std::regex_constants::icase |
                                     std::regex_constants::ECMAScript);
        filter_set = 1;
      } else {
        filter_set = 0;
      }
    }

    if (filter_set == 0 || (filter_set > 0 && std::regex_match(sql, filter))) {
      static int count = std::stoi(getenv("TEST_SQL_UNTIL_CRASH"));
      if (--count == 0) {
        fprintf(stderr, "%s SQL statements executed, will throw now\n",
                getenv("TEST_SQL_UNTIL_CRASH"));
        throw mysqlshdk::db::Error(
            "Injected error when about to execute '" + std::string(sql) + "'",
            CR_UNKNOWN_ERROR);
      }
    }
  });

  auto log_sql_handler = shcore::current_log_sql();
  log_sql_handler->log(get_thread_id(), sql, len);

  DBUG_LOG("sqlall", get_thread_id() << ": QUERY: " << std::string(sql, len));

  FI_TRIGGER_TRAP(mysql, mysqlshdk::utils::FI::Trigger_options(
                             {{"sql", std::string(sql, len)},
                              {"uri", _connection_options.uri_endpoint()}}));

  auto process_error = [this](std::string_view sql_script) {
    auto err =
        Error(mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));

    shcore::current_log_sql()->log(get_thread_id(), sql_script.data(),
                                   sql_script.size(), err);

    if (DBUG_EVALUATE_IF("sqlall", 1, 0)) {
      DBUG_LOG("sql", get_thread_id() << ": ERROR: " << err.format());
    } else {
      DBUG_LOG("sql", get_thread_id() << ": ERROR: " << err.format()
                                      << "\n\twhile executing: "
                                      << std::string(sql_script));
    }

    return err;
  };

  if (mysql_real_query(_mysql, sql, len) != 0) {
    throw process_error({sql, len});
  }

  // warning count can only be obtained after consuming the result data
  std::shared_ptr<Result> result(
      new Result(shared_from_this(), mysql_affected_rows(_mysql),
                 mysql_insert_id(_mysql), mysql_info(_mysql), buffered));

  /*
  Because of the way UDFs are implemented in the server, they don't return
  errors the same way "regular" functions do. In short, they have two parts: an
  initialization part and a fecth data part, which means that we may only know
  if the UDF returned an error when we try and read the results returned.

  This also means that calling mysql_store_result (which executes both parts in
  sequence), is completly different than calling mysql_use_result (which only
  does the first part because the data must be read explicitly). Hence the code
  below:
   - if buffered (mysql_store_result) then the data was already read and so we
  only need to check for errors (like in a non UDF) if no result was returned
   - if not buffered (mysql_use_result), then we need to fetch the first row to
  check for errors (this row is read but kept in result to avoid being lost)
  */
  if (is_udf) {
    if (buffered && !result->has_resultset()) {
      throw process_error({sql, len});
    } else if (!buffered) {
      result->pre_fetch_row();  // this will throw in case of error
    }
  }

  timer.stage_end();
  result->set_execution_time(timer.total_seconds_elapsed());
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

std::string Session::escape_string(const std::string &s) const {
  std::string res;
  res.resize(s.length() * 2 + 1);
  size_t l = mysql_real_escape_string_quote(_impl->_mysql, &res[0], s.data(),
                                            s.size(), '\'');
  res.resize(l);
  return res;
}

std::string Session::escape_string(const char *buffer, size_t len) const {
  std::string res;
  res.resize(len * 2 + 1);
  size_t l =
      mysql_real_escape_string_quote(_impl->_mysql, &res[0], buffer, len, '\'');
  res.resize(l);
  return res;
}

void Session_impl::setup_default_character_set() {
  // libmysqlclient 8.0 by default is using 'utf8mb4' character set for the new
  // connections, however when negotiating the connection details it sends the
  // collation name, not the character set name
  //
  // since we're linked against the 8.0 version of libmysqlclient, the collation
  // that corresponds to 'utf8mb4' is 'utf8mb4_0900_ai_ci', which is not known
  // to the 5.7 servers
  //
  // if server receives a collation it does not know, it falls back to the
  // compiled-in default, which in case of 5.7 server is 'latin1'
  if (get_server_version() < mysqlshdk::utils::Version(8, 0, 0)) {
    execute("SET NAMES 'utf8mb4';");
  }
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
