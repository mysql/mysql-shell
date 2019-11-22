/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include <mysqlx_version.h>

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/mysqlx/xpl_error.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace db {
namespace mysqlx {

FI_DEFINE(mysqlx, [](const mysqlshdk::utils::FI::Args &args) {
  if (args.get_int("abort", 0)) {
    abort();
  }
  if (args.get_int("code", -1) < 0) {
    throw std::logic_error(args.get_string("msg"));
  }
  throw mysqlshdk::db::Error(args.get_string("msg"), args.get_int("code"));
});

namespace {
template <typename Message_type>
std::string message_to_text(const std::string &binary_message) {
  std::string result;
  Message_type msg;

  msg.ParseFromString(binary_message);
  google::protobuf::TextFormat::PrintToString(msg, &result);

  return msg.GetDescriptor()->full_name() + " { " + result + " }";
}

std::ostream &operator<<(std::ostream &os,
                         const xcl::XProtocol::Message &message) {
  std::string output;
  std::string name;
  google::protobuf::TextFormat::Printer printer;

  printer.SetInitialIndentLevel(1);

  // special handling for nested messages (at least for Notices)
  if (message.GetDescriptor()->full_name() == "Mysqlx.Notice.Frame") {
    Mysqlx::Notice::Frame frame =
        *static_cast<const Mysqlx::Notice::Frame *>(&message);

    switch (frame.type()) {
      case ::Mysqlx::Notice::Frame_Type_WARNING: {
        const auto payload_as_text =
            message_to_text<Mysqlx::Notice::Warning>(frame.payload());

        frame.set_payload(payload_as_text);
        break;
      }
      case ::Mysqlx::Notice::Frame_Type_SESSION_VARIABLE_CHANGED: {
        const auto payload_as_text =
            message_to_text<Mysqlx::Notice::SessionVariableChanged>(
                frame.payload());

        frame.set_payload(payload_as_text);
        break;
      }
      case ::Mysqlx::Notice::Frame_Type_SESSION_STATE_CHANGED: {
        const auto payload_as_text =
            message_to_text<Mysqlx::Notice::SessionStateChanged>(
                frame.payload());

        frame.set_payload(payload_as_text);
        break;
      }
    }

    printer.PrintToString(frame, &output);
  } else {
    printer.PrintToString(message, &output);
  }

  return os << message.GetDescriptor()->full_name() << " {\n"
            << output << "}\n";
}

std::pair<xcl::XProtocol::Handler_id, xcl::XProtocol::Handler_id>
do_enable_trace(xcl::XSession *session) {
  xcl::XProtocol::Handler_id rh, sh;
  auto &protocol = session->get_protocol();

  rh = protocol.add_received_message_handler(
      [](xcl::XProtocol *, const xcl::XProtocol::Server_message_type_id,
         const xcl::XProtocol::Message &msg) -> xcl::Handler_result {
        std::cout << "<<<< RECEIVE " << msg << "\n";

        return xcl::Handler_result::Continue;
      });

  sh = protocol.add_send_message_handler(
      [](xcl::XProtocol *, const xcl::XProtocol::Client_message_type_id,
         const xcl::XProtocol::Message &msg) -> xcl::Handler_result {
        std::cout << ">>>> SEND " << msg << "\n";

        return xcl::Handler_result::Continue;
      });
  return {sh, rh};
}

}  // namespace

//-------------------------- Session Implementation ----------------------------

DEBUG_OBJ_ENABLE(db_mysqlx_Session);

XSession_impl::XSession_impl() : m_prep_stmt_count(0) {
  _enable_trace = false;

  DEBUG_OBJ_ALLOC(db_mysqlx_Session);
}

void XSession_impl::connect(const mysqlshdk::db::Connection_options &data) {
  _mysql.reset(::xcl::create_session().release());
  if (_enable_trace) _trace_handler = do_enable_trace(_mysql.get());

  _connection_options = data;

  if (_connection_options.has(mysqlshdk::db::kGetServerPublicKey)) {
    _mysql.reset();
    throw std::runtime_error(
        "X Protocol: Option get-server-public-key is not supported.");
  }

  if (_connection_options.has(mysqlshdk::db::kServerPublicKeyPath)) {
    _mysql.reset();
    throw std::runtime_error(
        "X Protocol: Option server-public-key-path is not supported.");
  }

  auto &ssl_options(_connection_options.get_ssl_options());

  std::string ssl_mode;

  // In shell, the default ssl-mode is preferred, but in the case of DevAPI
  // the default mode is required and it is set at mysqlx.getSession()
  if (ssl_options.has_default(mysqlshdk::db::kSslMode))
    ssl_mode = ssl_options.get_default(mysqlshdk::db::kSslMode);
  else
    ssl_mode = mysqlshdk::db::kSslModePreferred;

  if (ssl_options.has_data()) {
    // If no mode is specified and either ssl-ca or ssl-capath are specified
    // then it uses VERIFY_CA
    if (!ssl_options.has_mode()) {
      if (ssl_options.has_value(mysqlshdk::db::kSslCa) ||
          ssl_options.has_value(mysqlshdk::db::kSslCaPath)) {
        ssl_mode = mysqlshdk::db::kSslModeVerifyCA;
      }
      // If ssl-mode is specified, then it is used
    } else {
      ssl_mode = ssl_options.get_mode_name();
    }

    ssl_options.validate();

    if (ssl_options.has_ca())
      _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_ca,
                               ssl_options.get_ca());

    if (ssl_options.has_cert())
      _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_cert,
                               ssl_options.get_cert());

    if (ssl_options.has_key())
      _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_key,
                               ssl_options.get_key());

    if (ssl_options.has_capath())
      _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_ca_path,
                               ssl_options.get_capath());

    if (ssl_options.has_crl())
      _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_crl,
                               ssl_options.get_crl());

    if (ssl_options.has_crlpath())
      _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_crl_path,
                               ssl_options.get_crlpath());

    if (ssl_options.has_tls_version())
      _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Allowed_tls,
                               ssl_options.get_tls_version());

    if (ssl_options.has_cipher())
      _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_cipher,
                               ssl_options.get_cipher());
  }

  _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_mode, ssl_mode);

  _mysql->set_capability(xcl::XSession::Capability_can_handle_expired_password,
                         true);

  auto algs =
      _connection_options.has_compression_algorithms()
          ? shcore::str_lower(_connection_options.get_compression_algorithms())
          : "";
  if (_connection_options.has_compression()) {
    _mysql->set_mysql_option(
        xcl::XSession::Mysqlx_option::Compression_negotiation_mode,
        _connection_options.get_compression());
  } else {
    if (algs == "uncompressed")
      _mysql->set_mysql_option(
          xcl::XSession::Mysqlx_option::Compression_negotiation_mode,
          kCompressionDisabled);
    else if (algs.empty() || algs.find("uncompressed") != std::string::npos)
      _mysql->set_mysql_option(
          xcl::XSession::Mysqlx_option::Compression_negotiation_mode,
          kCompressionPreferred);
    else
      _mysql->set_mysql_option(
          xcl::XSession::Mysqlx_option::Compression_negotiation_mode,
          kCompressionRequired);
  }

  // default ["deflate_stream","lz4_message","zstd_stream"]
  if (!algs.empty()) {
    std::vector<std::string> av;
    for (const auto &a : shcore::split_string(algs, ",")) {
      if (a == "zlib")
        av.emplace_back("deflate_stream");
      else if (a == "zstd")
        av.emplace_back("zstd_stream");
      else if (a == "lz4")
        av.emplace_back("lz4_message");
      else if (a != "uncompressed")
        av.emplace_back(a);
    }
    _mysql->set_mysql_option(
        xcl::XSession::Mysqlx_option::Compression_algorithms, av);
  }

  bool user_defined_connection_attributes = false;
  if (_connection_options.is_connection_attributes_enabled()) {
    auto attrs = _mysql->get_connect_attrs();
    attrs.emplace_back("program_name", xcl::Argument_value{"mysqlsh"});

    if (!_connection_options.get_connection_attributes().empty()) {
      user_defined_connection_attributes = true;

      for (const auto &att : _connection_options.get_connection_attributes()) {
        std::string attribute = att.first;
        std::string value;
        if (!att.second.is_null()) {
          value = *att.second;
        }

        attrs.emplace_back(attribute, xcl::Argument_value{value});
      }
    }
    _mysql->set_capability(xcl::XSession::Capability_session_connect_attrs,
                           attrs);
  }

  // If a specific authentication type was given, it is used
  if (_connection_options.has(mysqlshdk::db::kAuthMethod)) {
    const auto error = _mysql->set_mysql_option(
        xcl::XSession::Mysqlx_option::Authentication_method,
        _connection_options.get(mysqlshdk::db::kAuthMethod));

    if (error) {
      _mysql.reset();
      store_error_and_throw(
          Error(std::string{"Failed to set the authentication method: "} +
                    error.what(),
                error.error()));
    }
  } else {
    // In 8.0.4, trying to connect without SSL to a caching_sha2_password
    // account will not work. The error message that's given is also confusing,
    // because there's no hint the error is because of no SSL instead of wrong
    // password So as a workaround, we force the PLAIN auth type to be always
    // attempted at last, at least until libmysqlxclient is fixed to produce a
    // specific error msg. In addition, in servers < 8.0.4 the plugin kicks the
    // user after the frst authentication attempt, so it is required that
    // MYSQL41 is used as the first authentication attempt in order the
    // connection to suceed on those servers.
    _mysql->set_mysql_option(
        xcl::XSession::Mysqlx_option::Authentication_method,
        std::vector<std::string>{"MYSQL41", "SHA256_MEMORY", "PLAIN"});
  }

  // Sets the connection timeout
  int64_t connect_timeout = mysqlshdk::db::k_default_connect_timeout;
  if (_connection_options.has(kConnectTimeout)) {
    connect_timeout = std::stoi(_connection_options.get(kConnectTimeout));
  }
  _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Connect_timeout,
                           connect_timeout);
  // Set read timeout
  if (_connection_options.has(kNetReadTimeout)) {
    int64_t read_timeout = std::stoi(_connection_options.get(kNetReadTimeout));
    _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Read_timeout,
                             read_timeout);
  }

  auto handler_id = _mysql->get_protocol().add_notice_handler(
      [this](xcl::XProtocol *, const bool,
             const Mysqlx::Notice::Frame::Type type, const char *payload,
             const uint32_t payload_len) {
        if (type == Mysqlx::Notice::Frame_Type_SESSION_STATE_CHANGED) {
          Mysqlx::Notice::SessionStateChanged change;
          change.ParseFromArray(payload, payload_len);
          if (!change.IsInitialized()) {
            log_error("Protocol error: Invalid notice received from server: %s",
                      change.InitializationErrorString().c_str());
          } else if (change.param() ==
                     Mysqlx::Notice::SessionStateChanged::ACCOUNT_EXPIRED) {
            _expired_account = true;
          }
        }
        return xcl::Handler_result::Continue;
      });

  auto unregister_handler_id = shcore::Scoped_callback([this, &handler_id]() {
    if (_mysql) _mysql->get_protocol().remove_notice_handler(handler_id);
  });

  std::vector<Mysqlx::Error> xproto_errors;

  auto xproto_errors_handler_id =
      _mysql->get_protocol().add_received_message_handler(
          [&xproto_errors](
              xcl::XProtocol *,
              const xcl::XProtocol::Server_message_type_id type_id,
              const xcl::XProtocol::Message &msg) -> xcl::Handler_result {
            if (type_id == Mysqlx::ServerMessages::ERROR) {
              const Mysqlx::Error e = *static_cast<const Mysqlx::Error *>(&msg);
              xproto_errors.push_back(e);
            }

            return xcl::Handler_result::Continue;
          });

  auto unregister_xproto_errors_handler_id =
      shcore::Scoped_callback([this, &xproto_errors_handler_id]() {
        if (_mysql)
          _mysql->get_protocol().remove_received_message_handler(
              xproto_errors_handler_id);
      });

  xcl::XError err;

  std::string host = data.has_host() ? data.get_host() : "localhost";

  DBUG_LOG("sqlall", "CONNECT: " << data.uri_endpoint());

  if ((host.empty() || host == "localhost") && data.has_socket()) {
    err = _mysql->connect(
        data.has_socket()
            ? (data.get_socket().empty() ? MYSQLX_UNIX_ADDR
                                         : data.get_socket().c_str())
            : nullptr,
        data.has_user() ? data.get_user().c_str() : "",
        data.has_password() ? data.get_password().c_str() : "",
        data.has_schema() ? data.get_schema().c_str() : "");
#ifdef _WIN32
    _connection_info = "Localhost via Named pipe";
#else
    _connection_info = "Localhost via UNIX socket";
#endif
  } else {
    err =
        _mysql->connect(host.c_str(), data.has_port() ? data.get_port() : 0,
                        data.has_user() ? data.get_user().c_str() : "",
                        data.has_password() ? data.get_password().c_str() : "",
                        data.has_schema() ? data.get_schema().c_str() : "");
    _connection_info = host + " via TCP/IP";
  }
  if (err) {
    _mysql.reset();

    if (err.error() == CR_MALFORMED_PACKET &&
        strstr(err.what(), "Unexpected response received from server")) {
      std::string message = "Requested session assumes MySQL X Protocol but '" +
                            data.as_uri(uri::formats::only_transport()) +
                            "' seems to speak the classic MySQL protocol";
      message.append(" (").append(err.what()).append(")");
      store_error_and_throw(Error(message.c_str(), CR_MALFORMED_PACKET));
    } else if (!user_defined_connection_attributes &&
               err.error() == ER_X_CAPABILITY_NOT_FOUND &&
               strstr(err.what(), "session_connect_attrs")) {
      log_warning(
          "Server does not support connection attributes, retrying with then "
          "disabled: (%d) %s",
          err.error(), err.what());

      // When connection attributes is not supported, and the user did not
      // explicitly request the registration of connection attributes, a second
      // connection attempt with them disabled will be done.
      mysqlshdk::db::Connection_options connection_data(data);
      connection_data.set(kConnectionAttributes, "false");
      connect(connection_data);
      return;
    } else {
      if (!xproto_errors.empty()) {
        auto e = xproto_errors.back();
        store_error_and_throw(Error(e.msg().c_str(), e.code()));
      }
      store_error_and_throw(Error(err.what(), err.error()));
    }
  }

  if (ssl_options.has_tls_ciphersuites())
    mysqlsh::current_console()->print_warning(
        "X Protocol: tls-ciphersuites option is not yet supported and will "
        "be ignored.");

  // If the account is not expired, retrieves additional session information
  if (!_expired_account) load_session_info();

  DBUG_LOG("sql", get_thread_id() << ": CONNECTED: " << data.uri_endpoint());

  // fill in defaults
  if (!_connection_options.has_scheme())
    _connection_options.set_scheme("mysqlx");

  // When neither port or socket were specified on the connection data
  // it means it was able to use default connection data
  if (!_connection_options.has_port() && !_connection_options.has_socket()) {
    // if neither port nor socket are set, connection is going to use default
    // X port
    _connection_options.set_port(MYSQLX_TCP_PORT);
  }
}

void XSession_impl::close() {
  // This should be logged, for now commenting to
  // avoid having unneeded output on the script mode
  if (auto result = _prev_result.lock()) {
    while (result->next_resultset()) {
    }
  }

  auto prep_ids = m_prepared_statements;
  for (auto stmt_id : prep_ids) {
    deallocate_prep_stmt(stmt_id);
  }

  if (_enable_trace) {
    enable_trace(false);
    _enable_trace = true;
  }

  DBUG_LOG("sql", get_thread_id() << ": DISCONNECT");

  m_prep_stmt_count = 0;
  _connection_id = 0;
  _connection_info.clear();
  _ssl_cipher.clear();
  _mysql.reset();
  _version = utils::Version();
  _expired_account = false;
  _case_sensitive_table_names = false;
  _prev_result.reset();
  _connection_options = Connection_options();
}

void XSession_impl::enable_trace(bool flag) {
  _enable_trace = flag;
  if (_mysql) {
    if (flag) {
      if (_trace_handler.first == 0 && _trace_handler.second == 0)
        _trace_handler = do_enable_trace(_mysql.get());
    } else {
      _mysql->get_protocol().remove_received_message_handler(
          _trace_handler.first);
      _mysql->get_protocol().remove_received_message_handler(
          _trace_handler.second);
    }
  }
}

void XSession_impl::load_session_info() {
  static constexpr char sql[] =
      "select @@lower_case_table_names, @@version, connection_id(), "
      "variable_value from performance_schema.session_status where "
      "variable_name = 'mysqlx_ssl_cipher'";
  std::shared_ptr<IResult> result(query(sql, sizeof(sql) - 1));

  const IRow *row = result->fetch_one();
  if (!row) throw std::logic_error("Unexpected empty result");
  _case_sensitive_table_names = row->get_uint(0) == 0;
  if (!row->is_null(1)) {
    _version = mysqlshdk::utils::Version(row->get_string(1));
  }
  if (!row->is_null(2)) {
    _connection_id = row->get_uint(2);
  }
  if (!row->is_null(3)) {
    _ssl_cipher = row->get_string(3);
  }
}

XSession_impl::~XSession_impl() {
  DEBUG_OBJ_DEALLOC(db_mysqlx_Session);

  _prev_result.reset();
  close();
}

void XSession_impl::before_query() {
  if (!_mysql) throw std::logic_error("Not connected");

  if (auto result = _prev_result.lock()) {
    if (result->has_resultset()) {
      // buffer the previous result to remove it from the connection
      // in case it's still active
      result->pre_fetch_rows(false);
    } else {
      // there's no data, but we need to call next_resultset() anyway
      // bug#26581651 filed for this
      while (result->next_resultset()) {
      }
    }
  }
}

std::shared_ptr<IResult> XSession_impl::after_query(
    std::unique_ptr<xcl::XQuery_result> result, bool buffered) {
  std::shared_ptr<Result> res(new Result(std::move(result)));
  res->fetch_metadata();
  _prev_result = res;

  if (buffered) res->pre_fetch_rows(buffered);

  return std::static_pointer_cast<IResult>(res);
}

std::shared_ptr<IResult> XSession_impl::query(const char *sql, size_t len,
                                              bool buffered) {
  mysqlshdk::utils::Profile_timer timer;
  timer.stage_begin("query");
  before_query();

  FI_TRIGGER_TRAP(mysqlx, mysqlshdk::utils::FI::Trigger_options(
                              {{"sql", std::string(sql, len)},
                               {"uri", _connection_options.uri_endpoint()}}));

  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult;
  {
    ::Mysqlx::Sql::StmtExecute stmt;

    stmt.set_stmt(sql, len);

    DBUG_LOG("sqlall", get_thread_id() << ": QUERY: " << stmt.stmt());
    xresult = _mysql->get_protocol().execute_stmt(stmt, &error);

    check_error_and_throw(error, stmt.stmt().c_str());
  }
  auto result = after_query(std::move(xresult), buffered);
  timer.stage_end();
  result->set_execution_time(timer.total_seconds_elapsed());
  return result;
}

void XSession_impl::execute(const char *sql, size_t len) {
  std::shared_ptr<IResult> result = query(sql, len, false);
  while (result->next_resultset()) {
  }
}

std::shared_ptr<IResult> XSession_impl::execute_stmt(
    const std::string &ns, const std::string &stmt,
    const xcl::Argument_array &args) {
  mysqlshdk::utils::Profile_timer timer;
  timer.stage_begin("execute_stmt");
  before_query();

  FI_TRIGGER_TRAP(mysqlx, mysqlshdk::utils::FI::Trigger_options(
                              {{"sql", stmt},
                               {"uri", _connection_options.uri_endpoint()}}));

  xcl::XError error;
  if (ns.empty() || ns == "sql")
    DBUG_LOG("sqlall", get_thread_id() << ": QUERY: " << stmt);
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->execute_stmt(ns, stmt, args, &error));
  check_error_and_throw(error, stmt.c_str());
  auto result = after_query(std::move(xresult));
  timer.stage_end();
  result->set_execution_time(timer.total_seconds_elapsed());
  return result;
}

std::shared_ptr<IResult> XSession_impl::execute_crud(
    const ::Mysqlx::Crud::Insert &msg) {
  mysqlshdk::utils::Profile_timer timer;
  timer.stage_begin("Mysqlx::Crud::Insert");
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->get_protocol().execute_insert(msg, &error));
  check_error_and_throw(error);
  auto result = after_query(std::move(xresult));
  timer.stage_end();
  result->set_execution_time(timer.total_seconds_elapsed());
  return result;
}

std::shared_ptr<IResult> XSession_impl::execute_crud(
    const ::Mysqlx::Crud::Update &msg) {
  before_query();
  mysqlshdk::utils::Profile_timer timer;
  timer.stage_begin("Mysqlx::Crud::Update");
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->get_protocol().execute_update(msg, &error));
  check_error_and_throw(error);
  auto result = after_query(std::move(xresult));
  timer.stage_end();
  result->set_execution_time(timer.total_seconds_elapsed());
  return result;
}

std::shared_ptr<IResult> XSession_impl::execute_crud(
    const ::Mysqlx::Crud::Delete &msg) {
  mysqlshdk::utils::Profile_timer timer;
  timer.stage_begin("Mysqlx::Crud::Delete");
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->get_protocol().execute_delete(msg, &error));
  check_error_and_throw(error);
  auto result = after_query(std::move(xresult));
  timer.stage_end();
  result->set_execution_time(timer.total_seconds_elapsed());
  return result;
}

std::shared_ptr<IResult> XSession_impl::execute_crud(
    const ::Mysqlx::Crud::Find &msg) {
  mysqlshdk::utils::Profile_timer timer;
  timer.stage_begin("Mysqlx::Crud::Find");
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->get_protocol().execute_find(msg, &error));
  check_error_and_throw(error);
  auto result = after_query(std::move(xresult));
  timer.stage_end();
  result->set_execution_time(timer.total_seconds_elapsed());
  return result;
}

void XSession_impl::prepare_stmt(const ::Mysqlx::Prepare::Prepare &msg) {
  before_query();
  xcl::XError error = _mysql->get_protocol().send(msg);
  check_error_and_throw(error);
  error = _mysql->get_protocol().recv_ok();
  check_error_and_throw(error);
  m_prepared_statements.insert(msg.stmt_id());
}

std::shared_ptr<IResult> XSession_impl::execute_prep_stmt(
    const ::Mysqlx::Prepare::Execute &msg) {
  mysqlshdk::utils::Profile_timer timer;
  timer.stage_begin("execute_prep_stmt");
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->get_protocol().execute_prep_stmt(msg, &error));
  check_error_and_throw(error);
  auto result = after_query(std::move(xresult));
  timer.stage_end();
  result->set_execution_time(timer.total_seconds_elapsed());
  return result;
}

void XSession_impl::deallocate_prep_stmt(uint32_t stmt_id) {
  before_query();
  ::Mysqlx::Prepare::Deallocate deallocate;
  deallocate.set_stmt_id(stmt_id);
  xcl::XError error = _mysql->get_protocol().send(deallocate);
  check_error_and_throw(error);
  error = _mysql->get_protocol().recv_ok();
  check_error_and_throw(error);

  // Removes the prepared statement from the list
  m_prepared_statements.erase(stmt_id);
}

void XSession_impl::check_error_and_throw(const xcl::XError &error,
                                          const char *context) {
  if (error) {
    store_error_and_throw(Error(error.what(), error.error()), context);
  } else {
    m_last_error.reset(nullptr);
  }
}

void XSession_impl::store_error_and_throw(const Error &error,
                                          const char *context) {
  if (DBUG_EVALUATE_IF("sqlall", 1, 0) || !context) {
    DBUG_LOG("sql", get_thread_id() << ": ERROR: " << error.format());
  } else {
    DBUG_LOG("sql", get_thread_id() << ": ERROR: " << error.format()
                                    << "\n\twhile executing: " << context);
  }
  m_last_error.reset(new Error(error));
  throw error;
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

}  // namespace mysqlx
}  // namespace db
}  // namespace mysqlshdk
