/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "utils/debug.h"
#include "utils/utils_general.h"

namespace mysqlshdk {
namespace db {
namespace mysqlx {

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

XSession_impl::XSession_impl() {
  _enable_trace = false;

  DEBUG_OBJ_ALLOC(db_mysqlx_Session);
}

void XSession_impl::connect(const mysqlshdk::db::Connection_options &data) {
  _mysql.reset(::xcl::create_session().release());
  if (_enable_trace) _trace_handler = do_enable_trace(_mysql.get());

  _connection_options = data;
  if (!_connection_options.has_scheme())
    _connection_options.set_scheme("mysqlx");

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

  // All connections should use mode = VERIFY_CA if no ssl mode is specified
  // and either ssl-ca or ssl-capath are specified
  if (!_connection_options.has_value(mysqlshdk::db::kSslMode) &&
      (_connection_options.has_value(mysqlshdk::db::kSslCa) ||
       _connection_options.has_value(mysqlshdk::db::kSslCaPath))) {
    _connection_options.set(mysqlshdk::db::kSslMode,
                            {mysqlshdk::db::kSslModeVerifyCA});
  }
  auto &ssl_options(_connection_options.get_ssl_options());
  if (ssl_options.has_data()) {
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
  if (ssl_options.has_mode())
    _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_mode,
                             ssl_options.get_mode_name());
  else
    _mysql->set_mysql_option(xcl::XSession::Mysqlx_option::Ssl_mode,
                             "PREFERRED");
  _mysql->set_capability(xcl::XSession::Capability_can_handle_expired_password,
                         true);

  // In 8.0.4, trying to connect without SSL to a caching_sha2_password account
  // will not work. The error message that's given is also confusing, because
  // there's no hint the error is because of no SSL instead of wrong password
  // So as a workaround, we force the PLAIN auth type to be always attempted
  // at last, at least until libmysqlxclient is fixed to produce a specific
  // error msg.
  // In addition, in servers < 8.0.4 the plugin kicks the user after the frst
  // authentication attempt, so it is required that MYSQL41 is used as the first
  // authentication attempt in order the connection to suceed on those servers.
  _mysql->set_mysql_option(
      xcl::XSession::Mysqlx_option::Authentication_method,
      std::vector<std::string>{"MYSQL41", "SHA256_MEMORY", "PLAIN"});
#if LIBMYSQL_VERSION_ID > 80012
#error Check whether libmysqlx already fixed error for caching_sha2_password
  // if libmysqlx already fixed the error, the code above to set auth
  // methods can be removed. If not, update the version check above to error out
  // again on 8.0.12
#endif

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

  if ((host.empty() || host == "localhost") && data.has_socket()) {
    err =
        _mysql->connect(data.has_socket() ? data.get_socket().c_str() : nullptr,
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
        _mysql->connect(host.c_str(), data.has_port() ? data.get_port() : 33060,
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
      throw mysqlshdk::db::Error(message.c_str(), CR_MALFORMED_PACKET);
    } else {
      if (!xproto_errors.empty()) {
        auto e = xproto_errors.back();
        throw mysqlshdk::db::Error(e.msg().c_str(), e.code());
      }
      throw mysqlshdk::db::Error(err.what(), err.error());
    }
  }

  // If the account is not expired, retrieves additional session information
  if (!_expired_account) load_session_info();
}

void XSession_impl::close() {
  // This should be logged, for now commenting to
  // avoid having unneeded output on the script mode
  if (auto result = _prev_result.lock()) {
    while (result->next_resultset()) {
    }
  }

  if (_enable_trace) {
    enable_trace(false);
    _enable_trace = true;
  }

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
  std::shared_ptr<IResult> result(
      query("select @@lower_case_table_names, @@version, connection_id(), "
            "variable_value from performance_schema.session_status where "
            "variable_name = 'mysqlx_ssl_cipher'"));

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

  if (buffered) res->pre_fetch_rows(false);

  return std::static_pointer_cast<IResult>(res);
}

std::shared_ptr<IResult> XSession_impl::query(const std::string &sql,
                                              bool buffered) {
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(_mysql->execute_sql(sql, &error));
  if (error) throw mysqlshdk::db::Error(error.what(), error.error());
  return after_query(std::move(xresult), buffered);
}

void XSession_impl::execute(const std::string &sql) {
  std::shared_ptr<IResult> result = query(sql, false);
  while (result->next_resultset()) {
  }
}

std::shared_ptr<IResult> XSession_impl::execute_stmt(
    const std::string &ns, const std::string &stmt,
    const xcl::Arguments &args) {
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->execute_stmt(ns, stmt, args, &error));
  if (error) throw mysqlshdk::db::Error(error.what(), error.error());
  return after_query(std::move(xresult));
}

std::shared_ptr<IResult> XSession_impl::execute_crud(
    const ::Mysqlx::Crud::Insert &msg) {
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->get_protocol().execute_insert(msg, &error));
  if (error) throw mysqlshdk::db::Error(error.what(), error.error());
  return after_query(std::move(xresult));
}

std::shared_ptr<IResult> XSession_impl::execute_crud(
    const ::Mysqlx::Crud::Update &msg) {
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->get_protocol().execute_update(msg, &error));
  if (error) throw mysqlshdk::db::Error(error.what(), error.error());
  return after_query(std::move(xresult));
}

std::shared_ptr<IResult> XSession_impl::execute_crud(
    const ::Mysqlx::Crud::Delete &msg) {
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->get_protocol().execute_delete(msg, &error));
  if (error) throw mysqlshdk::db::Error(error.what(), error.error());
  return after_query(std::move(xresult));
}

std::shared_ptr<IResult> XSession_impl::execute_crud(
    const ::Mysqlx::Crud::Find &msg) {
  before_query();
  xcl::XError error;
  std::unique_ptr<xcl::XQuery_result> xresult(
      _mysql->get_protocol().execute_find(msg, &error));
  if (error) throw mysqlshdk::db::Error(error.what(), error.error());
  return after_query(std::move(xresult));
}

std::function<std::shared_ptr<Session>()> g_session_factory;

void Session::set_factory_function(
    std::function<std::shared_ptr<Session>()> factory) {
  g_session_factory = factory;
}

std::shared_ptr<Session> Session::create() {
  if (g_session_factory) return g_session_factory();
  return std::shared_ptr<Session>(new Session());
}

}  // namespace mysqlx
}  // namespace db
}  // namespace mysqlshdk
