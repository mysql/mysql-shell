/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/mysql/session.h"

#include <mysql_version.h>

#include <mutex>
#include <regex>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/mysql/auth_plugins/common.h"
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
constexpr size_t K_MAX_QUERY_ATTRIBUTES = 32;
}  // namespace

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
//-------------------------- Query Attribute Implementation --------------------
Classic_query_attribute::Classic_query_attribute() noexcept = default;

Classic_query_attribute::Classic_query_attribute(int64_t val) {
  value.i = val;
  type = MYSQL_TYPE_LONGLONG;
  data_ptr = &value.i;
  size = sizeof(int64_t);
  is_null = false;
}

Classic_query_attribute::Classic_query_attribute(uint64_t val) {
  value.ui = val;
  type = MYSQL_TYPE_LONGLONG;
  data_ptr = &value.ui;
  size = sizeof(uint64_t);
  flags = UNSIGNED_FLAG;
  is_null = false;
}

Classic_query_attribute::Classic_query_attribute(double val) {
  value.d = val;
  type = MYSQL_TYPE_DOUBLE;
  data_ptr = &value.d;
  size = sizeof(double);
  is_null = false;
}

Classic_query_attribute::Classic_query_attribute(const std::string &val) {
  value.s = new std::string(val);
  type = MYSQL_TYPE_STRING;
  data_ptr = value.s->data();
  size = val.size();
  is_null = false;
}

Classic_query_attribute::Classic_query_attribute(const MYSQL_TIME &val,
                                                 enum_field_types t) {
  value.t = val;
  type = t;
  data_ptr = &value.t;
  size = sizeof(MYSQL_TIME);
  is_null = false;
}

Classic_query_attribute::~Classic_query_attribute() {
  if (type == MYSQL_TYPE_STRING) {
    delete value.s;
  }
}

void Classic_query_attribute::update_data_ptr() {
  switch (type) {
    case MYSQL_TYPE_LONGLONG:
      if (flags & UNSIGNED_FLAG) {
        data_ptr = &value.ui;
      } else {
        data_ptr = &value.i;
      }
      break;
    case MYSQL_TYPE_DOUBLE:
      data_ptr = &value.d;
      break;
    case MYSQL_TYPE_STRING:
      data_ptr = value.s->data();
      break;
    default:
      if (type != MYSQL_TYPE_NULL) {
        data_ptr = &value.t;
      } else {
        data_ptr = nullptr;
      }
  }
}

Classic_query_attribute &Classic_query_attribute::operator=(
    const Classic_query_attribute &other) {
  if (type == MYSQL_TYPE_STRING) {
    delete value.s;
  }

  value = other.value;
  type = other.type;
  size = other.size;
  flags = other.flags;

  if (type == MYSQL_TYPE_STRING) {
    value.s = new std::string(*other.value.s);
  }

  update_data_ptr();

  return *this;
}

Classic_query_attribute &Classic_query_attribute::operator=(
    Classic_query_attribute &&other) noexcept {
  if (type == MYSQL_TYPE_STRING) {
    delete value.s;
    value.s = nullptr;
  }

  std::swap(value, other.value);
  std::swap(type, other.type);
  std::swap(size, other.size);
  std::swap(flags, other.flags);

  update_data_ptr();

  return *this;
}

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

Session_impl::Session_impl() = default;

void Session_impl::connect(
    const mysqlshdk::db::Connection_options &connection_options) {
  long flags = CLIENT_MULTI_RESULTS;

  if (connection_options.is_interactive()) {
    flags |= CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS | CLIENT_INTERACTIVE;
  }

  _mysql = mysql_init(nullptr);

  // A tracer plugin needs to be enabled as soon as the first instance of MYSQL
  // is created in order to properly support pluggable authentication as it is
  // the tracer who can identify the authentication plugin is being used so
  // plugin options can be defined.
  std::call_once(trace_register_flag, register_tracer_plugin, _mysql);

  _connection_options = connection_options;
  _connection_options.set_default_data();

  auth::register_connection_options_for_mysql(_mysql, _connection_options);
  shcore::on_leave_scope unregister_conn_options(
      [this]() { auth::unregister_connection_options_for_mysql(_mysql); });

  auto used_ssl_mode = setup_ssl(_connection_options.get_ssl_options());
  if (_connection_options.has_transport_type()) {
    unsigned int proto = MYSQL_PROTOCOL_TCP;

    switch (_connection_options.get_transport_type()) {
      case mysqlshdk::db::Transport_type::Tcp:
        proto = MYSQL_PROTOCOL_TCP;
        break;

      case mysqlshdk::db::Transport_type::Socket:
        proto = MYSQL_PROTOCOL_SOCKET;
        break;

      case mysqlshdk::db::Transport_type::Pipe:
#ifndef _WIN32
        assert(0);
#endif
        // Enable pipe connection if required
        proto = MYSQL_PROTOCOL_PIPE;
        break;
    }
    mysql_options(_mysql, MYSQL_OPT_PROTOCOL, &proto);
  }

  // Connection attributes are only sent if they are not disabled
  if (_connection_options.is_connection_attributes_enabled()) {
    mysql_options(_mysql, MYSQL_OPT_CONNECT_ATTR_RESET, nullptr);
    mysql_options4(_mysql, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name",
                   "mysqlsh");

    for (const auto &att : _connection_options.get_connection_attributes()) {
      std::string attribute = att.first;
      std::string value;
      if (att.second.has_value()) {
        value = *att.second;
      }

      mysql_options4(_mysql, MYSQL_OPT_CONNECT_ATTR_ADD, attribute.c_str(),
                     value.c_str());
    }
  }

  if (bool get_pub_key =
          connection_options.is_enabled(mysqlshdk::db::kGetServerPublicKey)) {
    mysql_options(_mysql, MYSQL_OPT_GET_SERVER_PUBLIC_KEY, &get_pub_key);
  }

  if (connection_options.has(mysqlshdk::db::kServerPublicKeyPath)) {
    std::string server_public_key =
        connection_options.get(mysqlshdk::db::kServerPublicKeyPath);
    mysql_options(_mysql, MYSQL_SERVER_PUBLIC_KEY, server_public_key.c_str());
  }

  if (connection_options.has(mysqlshdk::db::kAuthMethod)) {
    const auto &auth = connection_options.get(mysqlshdk::db::kAuthMethod);

    if (auth == kAuthMethodClearPassword) {
      if (!connection_options.has_socket() && !connection_options.has_pipe()) {
        auto final_ssl_mode =
            used_ssl_mode.value_or(mysqlshdk::db::Ssl_mode::Disabled);

        if (final_ssl_mode == mysqlshdk::db::Ssl_mode::Disabled) {
          throw std::runtime_error(
              "Clear password authentication is not supported over insecure "
              "channels.");
        } else if (final_ssl_mode == mysqlshdk::db::Ssl_mode::Preferred) {
          throw std::runtime_error(
              "Clear password authentication requires a secure channel, "
              "please "
              "use ssl-mode=REQUIRED to guarantee a secure channel.");
        }
      }
      uint value = 1;
      mysql_options(_mysql, MYSQL_ENABLE_CLEARTEXT_PLUGIN, &value);
    }

    mysql_options(_mysql, MYSQL_DEFAULT_AUTH, auth.c_str());
  }

  {  // Sets the connection timeout (comes in milliseconds, must be in
     // seconds)
    uint connect_timeout = mysqlshdk::db::default_connect_timeout();

    if (_connection_options.has_connect_timeout()) {
      connect_timeout = _connection_options.get_connect_timeout();
    }
    connect_timeout = std::ceil(connect_timeout / 1000.0);
    mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout);
  }

  if (_connection_options.has_net_read_timeout()) {
    uint read_timeout = _connection_options.get_net_read_timeout();

    read_timeout = std::ceil(read_timeout / 1000.0);
    mysql_options(_mysql, MYSQL_OPT_READ_TIMEOUT, &read_timeout);
  }

  if (_connection_options.has_net_write_timeout()) {
    uint write_timeout = _connection_options.get_net_write_timeout();

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
    const int local_infile =
        connection_options.is_enabled(mysqlshdk::db::kLocalInfile);
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
  // packets from the server. Multiple buffer reads may be needed to
  // completely read a packet, so net_buffer_length can be smaller, but
  // max_allowed_packet must be as big as the biggest expected row to be
  // fetched.
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

  auto mysql_plugin_dir = mysqlshdk::db::default_mysql_plugins_dir();
  if (connection_options.has(mysqlshdk::db::kMysqlPluginDir)) {
    mysql_plugin_dir = connection_options.get(mysqlshdk::db::kMysqlPluginDir);
  }

  if (!mysql_plugin_dir.empty()) {
    mysql_options(_mysql, MYSQL_PLUGIN_DIR, mysql_plugin_dir.c_str());
  }

  DBUG_LOG("sqlall", "CONNECT: " << _connection_options.uri_endpoint());

  // If this fails Mfa_passwords definition has to be changed
  static_assert(MAX_AUTH_FACTORS == 3);
  for (int factor = 1; factor <= 3; factor++) {
    if (_connection_options.has_mfa_password(factor - 1)) {
      mysql_options4(_mysql, MYSQL_OPT_USER_PASSWORD, &factor,
                     _connection_options.get_mfa_password(factor - 1).c_str());
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
      // If connection was not through TCP/IP it means either the default
      // socket path or windows named pipe was used
#ifdef _WIN32
      _connection_options.set_pipe(mysql_unix_port);
#else
      _connection_options.set_socket(mysql_unix_port);
#endif
    }
  }

  m_thread_id = mysql_thread_id(_mysql);
}

std::optional<mysqlshdk::db::Ssl_mode> Session_impl::setup_ssl(
    const mysqlshdk::db::Ssl_options &ssl_options) const {
  int value;

  std::optional<mysqlshdk::db::Ssl_mode> ssl_mode;
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
    m_thread_id = 0;
    _mysql = nullptr;
  }
}

std::shared_ptr<IResult> Session_impl::query(
    const char *sql, size_t len, bool buffered,
    const std::vector<Query_attribute> &query_attributes) {
  return run_sql(sql, len, buffered, false, query_attributes);
}

std::shared_ptr<IResult> Session_impl::query_udf(std::string_view sql,
                                                 bool buffered) {
  return run_sql(sql.data(), sql.size(), buffered, true);
}

void Session_impl::execute(const char *sql, size_t len) {
  auto result = run_sql(sql, len, true, false);
}

std::shared_ptr<IResult> Session_impl::run_sql(
    const char *sql, size_t len, bool buffered, bool is_udf,
    const std::vector<Query_attribute> &query_attributes) {
  if (_mysql == nullptr) throw std::runtime_error("Not connected");
  mysqlshdk::utils::Profile_timer timer;
  timer.stage_begin("run_sql");

  m_session_tracker_info.reset();

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

    if (filter_set == 0 ||
        (filter_set > 0 && std::regex_match(sql, sql + len, filter))) {
      static int count = std::stoi(getenv("TEST_SQL_UNTIL_CRASH"));
      if (--count == 0) {
        fprintf(stderr, "%s SQL statements executed, will throw now\n",
                getenv("TEST_SQL_UNTIL_CRASH"));
        throw mysqlshdk::db::Error("Injected error when about to execute '" +
                                       std::string(sql, len) + "'",
                                   CR_UNKNOWN_ERROR);
      }
    }
  });

  auto log_sql_handler = shcore::current_log_sql();
  log_sql_handler->log(get_thread_id(), std::string_view{sql, len});

  DBUG_LOG("sqlall", get_thread_id() << ": QUERY: " << std::string(sql, len));

  FI_TRIGGER_TRAP(mysql, mysqlshdk::utils::FI::Trigger_options(
                             {{"sql", std::string(sql, len)},
                              {"uri", _connection_options.uri_endpoint()}}));

  auto process_error = [this](std::string_view sql_script) {
    auto err =
        Error(mysql_error(_mysql), mysql_errno(_mysql), mysql_sqlstate(_mysql));

    shcore::current_log_sql()->log(get_thread_id(), sql_script, err);

    if (DBUG_EVALUATE_IF("sqlall", 1, 0)) {
      DBUG_LOG("sql", get_thread_id() << ": ERROR: " << err.format());
    } else {
      DBUG_LOG("sql", get_thread_id() << ": ERROR: " << err.format()
                                      << "\n\twhile executing: "
                                      << std::string(sql_script));
    }

    return err;
  };

  // Attribute references need to be alive while the query is executed
  const char *attribute_names[K_MAX_QUERY_ATTRIBUTES];
  MYSQL_BIND attribute_values[K_MAX_QUERY_ATTRIBUTES];

  if (!query_attributes.empty()) {
    memset(attribute_values, 0, sizeof(attribute_values));
    size_t attribute_count = 0;
    for (const auto &att : query_attributes) {
      attribute_names[attribute_count] = att.name.data();
      const auto &value =
          dynamic_cast<Classic_query_attribute *>(att.value.get());

      attribute_values[attribute_count].buffer_type = value->type;
      attribute_values[attribute_count].buffer = value->data_ptr;
      attribute_values[attribute_count].length = &value->size;
      attribute_values[attribute_count].is_null = &value->is_null;
      attribute_values[attribute_count].is_unsigned =
          (value->flags & UNSIGNED_FLAG) ? true : false;
      attribute_count++;
    }

    mysql_bind_param(_mysql, attribute_count, attribute_values,
                     attribute_names);
  }

  if (mysql_real_query(_mysql, sql, len) != 0) {
    throw process_error({sql, len});
  }

  // warning count can only be obtained after consuming the result data
  std::shared_ptr<Result> result(
      new Result(shared_from_this(), mysql_affected_rows(_mysql),
                 mysql_insert_id(_mysql), mysql_info(_mysql), buffered));

  check_session_track_system_variables();

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

  int rc = mysql_next_result(_mysql);

  if (rc > 0) {
    throw Error(mysql_error(_mysql), mysql_errno(_mysql),
                mysql_sqlstate(_mysql));
  }

  return rc == 0;
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

namespace {
enum class Tracked_system_variable { UNINTERESTING, STATEMENT_ID, SQL_MODE };

Tracked_system_variable variable_by_name(const char *data, size_t length) {
  if (strncmp(data, "statement_id", length) == 0)
    return Tracked_system_variable::STATEMENT_ID;
  if (strncmp(data, "sql_mode", length) == 0)
    return Tracked_system_variable::SQL_MODE;
  return Tracked_system_variable::UNINTERESTING;
}
}  // namespace

void Session_impl::check_session_track_system_variables() const {
  // Must be called after results are fetched, statement_id only
  // comes in after that
  const char *data;
  size_t length;
  int found_variables = 0;

  if (mysql_session_track_get_first(_mysql, SESSION_TRACK_SYSTEM_VARIABLES,
                                    &data, &length) == 0) {
    Tracked_system_variable var = variable_by_name(data, length);

    while (mysql_session_track_get_next(_mysql, SESSION_TRACK_SYSTEM_VARIABLES,
                                        &data, &length) == 0) {
      if (var == Tracked_system_variable::STATEMENT_ID) {
        m_session_tracker_info.statement_id = std::string(data, length);
        found_variables++;
      } else if (var == Tracked_system_variable::SQL_MODE) {
        m_session_tracker_info.sql_mode = std::string(data, length);
        found_variables++;
      }
      if (found_variables == 2) break;
      if (mysql_session_track_get_next(_mysql, SESSION_TRACK_SYSTEM_VARIABLES,
                                       &data, &length) != 0)
        break;
      var = variable_by_name(data, length);
    }
  }
}

const std::optional<std::string> &Session_impl::get_last_statement_id() const {
  return m_session_tracker_info.statement_id;
}

const std::optional<std::string> &Session_impl::get_last_sql_mode() const {
  return m_session_tracker_info.sql_mode;
}

std::string Session::escape_string(std::string_view s) const {
  std::string res;
  res.resize(s.size() * 2 + 1);
  size_t l = mysql_real_escape_string_quote(_impl->_mysql, &res[0], s.data(),
                                            s.size(), '\'');
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

MYSQL *Session_impl::release_connection() {
  return std::exchange(_mysql, nullptr);
}

void Session_impl::set_character_set(const std::string &charset_name) {
  if (!_mysql) throw std::runtime_error("Not connected");

  if (mysql_set_character_set(_mysql, charset_name.c_str())) {
    throw_on_connection_fail();
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

std::string Session::track_system_variable(const std::string &variable) {
  if (get_server_version() < mysqlshdk::utils::Version(5, 7, 0))
    throw std::invalid_argument("Session tracker not supported by server");

  if (variable != "sql_mode")
    throw std::invalid_argument("Variable unsupported");

  const auto res = query("SELECT @@SESSION.session_track_system_variables");
  const auto row = res->fetch_one_or_throw();

  auto current_value = shcore::str_lower(row->get_string(0));

  if (shcore::has_session_track_system_variable(current_value, variable))
    return current_value;

  if (!current_value.empty()) current_value.append(",");
  current_value.append(variable);

  executef("SET SESSION session_track_system_variables = ?", current_value);

  return current_value;
}

void Session::refresh_sql_mode_tracked() {
  if (_impl->m_session_tracker_info.sql_mode.has_value()) {
    set_sql_mode(*_impl->m_session_tracker_info.sql_mode);
  }
}

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
