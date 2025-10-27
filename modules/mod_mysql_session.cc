/*
 * Copyright (c) 2014, 2025, Oracle and/or its affiliates.
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

#include "modules/mod_mysql_session.h"

#include <string>
#include <vector>

#include "modules/mod_mysql_resultset.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/obj_date.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/db/utils/utils.h"
#include "mysqlshdk/libs/ssh/ssh_manager.h"
#include "mysqlshdk/libs/utils/option_tracker.h"
#include "shellcore/shell_notifications.h"
#include "shellcore/utils_help.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"
#include "utils/utils_sqlstring.h"

using namespace mysqlsh;
using namespace mysqlsh::mysql;
using namespace shcore;

// Documentation for ClassicSession class
REGISTER_HELP_CLASS(ClassicSession, mysql);
REGISTER_HELP(CLASSICSESSION_GLOBAL_BRIEF,
              "Represents the currently "
              "open MySQL session.");
REGISTER_HELP(CLASSICSESSION_BRIEF,
              "Enables interaction with a MySQL Server "
              "using the MySQL Protocol.");
REGISTER_HELP(CLASSICSESSION_DETAIL, "Provides facilities to execute queries.");
ClassicSession::ClassicSession() { init(); }

ClassicSession::ClassicSession(const ClassicSession &session)
    : ShellBaseSession(session),
      std::enable_shared_from_this<mysqlsh::mysql::ClassicSession>(session),
      _session(session._session) {
  init();
}

ClassicSession::ClassicSession(
    std::shared_ptr<mysqlshdk::db::mysql::Session> session)
    : ShellBaseSession(), _session(session) {
  init();

  // TODO(alfredo) maybe can remove _connection_options ivar
  _connection_options = session->get_connection_options();
}

ClassicSession::~ClassicSession() {
  if (is_open()) close();
}

void ClassicSession::init() {
  //_schema_proxy.reset(new Proxy_object(std::bind(&ClassicSession::get_db,
  // this, _1)));

  add_property("uri", "getUri");
  add_property("sshUri", "getSshUri");
  add_property("connectionId", "getConnectionId");

  expose("close", &ClassicSession::close);
  expose("runSql", &ClassicSession::run_sql, "query", "?args");
  expose("isOpen", &ClassicSession::is_open);
  expose("startTransaction", &ClassicSession::_start_transaction);
  expose("commit", &ClassicSession::_commit);
  expose("rollback", &ClassicSession::_rollback);
  expose("setQueryAttributes", &ClassicSession::set_query_attributes,
         "attributes");
  expose("setClientData", &ClassicSession::set_client_data, "key", "data");
  expose("getClientData", &ClassicSession::get_client_data, "key");
  expose("getSqlMode", &ClassicSession::get_sql_mode);
  expose("trackSystemVariable", &ClassicSession::track_system_variable,
         "variable");
  expose<void, const std::string &>(
      "setOptionTrackerFeatureId",
      &ClassicSession::set_option_tracker_feature_id, "feature_id");

  expose("_getSocketFd", &ClassicSession::_get_socket_fd);
}

void ClassicSession::connect(
    const mysqlshdk::db::Connection_options &connection_options) {
  using shcore::option_tracker::Shell_feature;

  _connection_options = connection_options;

  try {
    _session = mysqlshdk::db::mysql::Session::create();

    _session->connect(_connection_options);

    _session->set_option_tracker_feature_id(Shell_feature::SHELL_USE);
  }
  CATCH_AND_TRANSLATE();
}

std::string ClassicSession::get_ssl_cipher() const {
  const char *cipher = _session ? _session->get_ssl_cipher() : nullptr;
  return cipher ? cipher : "";
}

// Documentation of close function
REGISTER_HELP_FUNCTION(close, ClassicSession);
REGISTER_HELP(CLASSICSESSION_CLOSE_BRIEF,
              "Closes the internal connection to the MySQL Server held on this "
              "session object.");

/**
 * $(CLASSICSESSION_CLOSE_BRIEF)
 */
#if DOXYGEN_JS
Undefined ClassicSession::close() {}
#elif DOXYGEN_PY
None ClassicSession::close() {}
#endif
void ClassicSession::close() {
  // Connection must be explicitly closed, we can't rely on the
  // automatic destruction because if shared across different objects
  // it may remain open
  if (_session) _session->close();

  _session.reset();
}

REGISTER_HELP_FUNCTION(setQueryAttributes, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_SETQUERYATTRIBUTES, R"*(
Defines query attributes that apply to the next statement sent to the server
for execution.

It is possible to define up to 32 pairs of attribute and values for the next
executed SQL statement.

To access query attributes within SQL statements for which attributes have been
defined, the <b>query_attributes</b> component must be installed, this
component implements a <b>mysql_query_attribute_string()</b> loadable function
that takes an attribute name argument and returns the attribute value as a
string, or NULL if the attribute does not exist.

To install the <b>query_attributes</b> component execute the following
statement:


@code
   session.<<<runSql>>>('INSTALL COMPONENT "file://component_query_attributes"');
@endcode
)*");

/**
 * $(CLASSICSESSION_SETQUERYATTRIBUTES_BRIEF)
 *
 * $(CLASSICSESSION_SETQUERYATTRIBUTES)
 */
#if DOXYGEN_JS
Undefined ClassicSession::setQueryAttributes(Dictionary attributes) {}
#elif DOXYGEN_PY
None ClassicSession::set_query_attributes(dict attributes) {}
#endif

std::vector<mysqlshdk::db::Query_attribute> ClassicSession::query_attributes()
    const {
  using mysqlshdk::db::mysql::Classic_query_attribute;
  return m_query_attributes.get_query_attributes(
      [](const shcore::Value &att) -> std::unique_ptr<Classic_query_attribute> {
        switch (att.get_type()) {
          case shcore::Value_type::String:
            return std::make_unique<Classic_query_attribute>(att.get_string());
          case shcore::Value_type::Bool:
            return std::make_unique<Classic_query_attribute>(att.as_int());
          case shcore::Value_type::Integer:
            return std::make_unique<Classic_query_attribute>(att.as_int());
          case shcore::Value_type::Float:
            return std::make_unique<Classic_query_attribute>(att.as_double());
          case shcore::Value_type::UInteger:
            return std::make_unique<Classic_query_attribute>(att.as_uint());
          case shcore::Value_type::Null:
            return std::make_unique<Classic_query_attribute>();
          case shcore::Value_type::Object: {
            // Only the date object is supported
            if (auto date = att.as_object<Date>(); date) {
              MYSQL_TIME time;
              std::memset(&time, 0, sizeof(time));

              time.year = date->get_year();
              time.month = date->get_month();
              time.day = date->get_day();
              time.hour = date->get_hour();
              time.minute = date->get_min();
              time.second = date->get_sec();
              time.second_part = date->get_usec();

              enum_field_types type = MYSQL_TYPE_TIMESTAMP;
              time.time_type = MYSQL_TIMESTAMP_DATETIME;
              if (date->has_date()) {
                if (!date->has_time()) {
                  type = MYSQL_TYPE_DATE;
                  time.time_type = MYSQL_TIMESTAMP_DATE;
                }
              } else {
                type = MYSQL_TYPE_TIME;
                time.time_type = MYSQL_TIMESTAMP_TIME;
              }
              return std::make_unique<Classic_query_attribute>(time, type);
            }
            break;
          }
          default:
            // This should never happen
            assert(false);
            break;
        }

        return {};
      });
}

REGISTER_HELP_FUNCTION(runSql, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_RUNSQL, R"*(
Executes a query and returns the corresponding ClassicResult object.

@param query the SQL query to execute against the database.
@param args Optional list of literals to use when replacing ? placeholders in
the query string.

@returns A ClassicResult object.

@throw LogicError if there's no open session.
@throw ArgumentError if the parameters are invalid.
)*");
/**
 * $(CLASSICSESSION_RUNSQL_BRIEF)
 *
 * $(CLASSICSESSION_RUNSQL)
 */
#if DOXYGEN_JS
ClassicResult ClassicSession::runSql(String query, Array args) {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::run_sql(str query, list args) {}
#endif
std::shared_ptr<ClassicResult> ClassicSession::run_sql(
    const std::string &query, const shcore::Array_t &args) {
  std::shared_ptr<ClassicResult> ret_val;

  // Will return the result of the SQL execution
  // In case of error will be Undefined
  if (!_session || !_session->is_open()) {
    throw Exception::logic_error("Not connected.");
  } else {
    shcore::Scoped_callback finally(
        [this]() { query_attribute_store().clear(); });

    ret_val = std::make_shared<ClassicResult>(
        execute_sql(query, args, query_attributes(), true));
  }

  return ret_val;
}

std::shared_ptr<mysqlshdk::db::IResult> ClassicSession::do_execute_sql(
    std::string_view query, const shcore::Array_t &args,
    const std::vector<mysqlshdk::db::Query_attribute> &query_attributes) {
  std::shared_ptr<mysqlshdk::db::IResult> result;
  if (!_session || !_session->is_open()) {
    throw Exception::logic_error("Not connected.");
  } else {
    if (query.empty()) {
      throw Exception::argument_error("No query specified.");
    } else {
      shcore::Interrupt_handler intr(
          []() { return true; },
          [weak_session = std::weak_ptr{get_core_session()}]() {
            kill_query(weak_session);
          });

      try {
        result = _session->query(sub_query_placeholders(query, args), false,
                                 query_attributes);
      } catch (const mysqlshdk::db::Error &error) {
        throw shcore::Exception::mysql_error_with_code_and_state(
            error.what(), error.code(), error.sqlstate());
      }
    }
  }

  return result;
}

// Documentation of getCurrentSchema function
REGISTER_HELP_PROPERTY(uri, ClassicSession);
REGISTER_HELP(CLASSICSESSION_URI_BRIEF, "${CLASSICSESSION_GETURI_BRIEF}");
REGISTER_HELP_FUNCTION(getUri, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_GETURI, R"*(
Retrieves the URI for the current session.

@returns A string representing the connection data.
)*");
/**
 * $(CLASSICSESSION_GETURI_BRIEF)
 *
 * $(CLASSICSESSION_GETURI)
 */
#if DOXYGEN_JS
String ClassicSession::getUri() {}
#elif DOXYGEN_PY
str ClassicSession::get_uri() {}
#endif

REGISTER_HELP_PROPERTY(sshUri, ClassicSession);
REGISTER_HELP_FUNCTION(getSshUri, ClassicSession);
REGISTER_HELP(CLASSICSESSION_SSHURI_BRIEF, "${CLASSICSESSION_GETSSHURI_BRIEF}");
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_GETSSHURI, R"*(
Retrieves the SSH URI for the current session.

@returns A string representing the SSH connection data.
)*");
/**
 * $(CLASSICSESSION_GETSSHURI_BRIEF)
 *
 * $(CLASSICSESSION_GETSSHURI)
 */
#if DOXYGEN_JS
String ClassicSession::getSshUri() {}
#elif DOXYGEN_PY
str ClassicSession::get_ssh_uri() {}
#endif

REGISTER_HELP_FUNCTION(getSqlMode, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_GETSQLMODE, R"*(
Retrieves the SQL_MODE for the current session.

@returns Value of the SQL_MODE session variable.

Queries the value of the SQL_MODE session variable. If session tracking of
SQL_MODE is enabled, it will fetch its cached value.
)*");
/**
 * $(CLASSICSESSION_GETSQLMODE_BRIEF)
 *
 * $(CLASSICSESSION_GETSQLMODE)
 */
#if DOXYGEN_JS
String ClassicSession::getSqlMode() {}
#elif DOXYGEN_PY
str ClassicSession::get_sql_mode() {}
#endif

REGISTER_HELP_PROPERTY(connectionId, ClassicSession);
REGISTER_HELP_FUNCTION(getConnectionId, ClassicSession);
REGISTER_HELP(CLASSICSESSION_CONNECTIONID_BRIEF,
              "${CLASSICSESSION_GETCONNECTIONID_BRIEF}");
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_GETCONNECTIONID, R"*(
Retrieves the connection id for the current session.

@returns An integer value representing the connection id.
)*");
/**
 * $(CLASSICSESSION_GETCONNECTIONID_BRIEF)
 *
 * $(CLASSICSESSION_GETCONNECTIONID)
 */
#if DOXYGEN_JS
Integer ClassicSession::getConnectionId() {}
#elif DOXYGEN_PY
int ClassicSession::get_connection_id() {}
#endif

Value ClassicSession::get_member(const std::string &prop) const {
  // Retrieves the member first from the parent
  Value ret_val;

  if (prop == "__connection_info") {
    // FIXME: temporary code until ISession refactoring
    const char *i = _session ? _session->get_connection_info() : nullptr;
    return Value(i ? i : "");
  }

  if (prop == "uri") {
    ret_val = shcore::Value(uri());
  } else if (prop == "sshUri") {
    ret_val = shcore::Value(ssh_uri());
  } else if (prop == "connectionId") {
    ret_val = shcore::Value(_session->get_connection_id());
  } else {
    ret_val = ShellBaseSession::get_member(prop);
  }
  return ret_val;
}

std::string ClassicSession::get_current_schema() {
  std::string name;

  if (_session && _session->is_open()) {
    auto result = execute_sql("select schema()");
    auto next_row = result->fetch_one();

    if (next_row && !next_row->is_null(0)) {
      name = next_row->get_string(0);
    }
  }

  return name;
}

void ClassicSession::set_current_schema(const std::string &name) {
  const auto query = sqlstring("use !", 0) << name;
  get_core_session()->execute(query.str_view());
}

REGISTER_HELP_FUNCTION(setClientData, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_SETCLIENTDATA, R"*(
Associates a value with the session for the given key.

@param key (string) A string to identify the stored value.
@param value JSON-like value to be stored.

Saves a value in the session data structure associated to the given key.
The value can be retrieved later using <<<getClientData>>>().
)*");
/**
 * $(CLASSICSESSION_SETCLIENTDATA_BRIEF)
 *
 * $(CLASSICSESSION_SETCLIENTDATA)
 */
#if DOXYGEN_JS
Undefined ClassicSession::setClientData(String key, Any value) {}
#elif DOXYGEN_PY
None ClassicSession::set_client_data(str key, Any value) {}
#endif

REGISTER_HELP_FUNCTION(getClientData, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_GETCLIENTDATA, R"*(
Returns value associated with the session for the given key.

@param key (string) A string to identify the stored value.

@returns JSON-like value stored for the key.

Returns a value previously stored in the session data structure associated
with <<<setClientData>>>().
)*");
/**
 * $(CLASSICSESSION_GETCLIENTDATA_BRIEF)
 *
 * $(CLASSICSESSION_GETCLIENTDATA)
 */
#if DOXYGEN_JS
Any ClassicSession::getClientData(String key) {}
#elif DOXYGEN_PY
Any ClassicSession::get_client_data(str key) {}
#endif

std::shared_ptr<shcore::Object_bridge> ClassicSession::create(
    const mysqlshdk::db::Connection_options &co_) {
  auto co = co_;
  // DevAPI getClassicSession uses ssl-mode = REQUIRED by default if no
  // ssl-ca or ssl-capath are specified
  if (!co.get_ssl_options().has_mode() &&
      !co.has_value(mysqlshdk::db::kSslCa) &&
      !co.has_value(mysqlshdk::db::kSslCaPath)) {
    co.get_ssl_options().set_mode(mysqlshdk::db::Ssl_mode::Required);
  }

  co.set_default_data();
  co.get_ssh_options().set_fallback_remote_port(
      mysqlshdk::db::k_default_mysql_port);

  mysqlshdk::ssh::Ssh_tunnel tunnel;

  if (auto &ssh = co.get_ssh_options(); ssh.has_data()) {
    // before creating a normal session we need to establish ssh if needed:
    tunnel = mysqlshdk::ssh::current_ssh_manager()->create_tunnel(&ssh);
  }

  const auto session = std::make_shared<ClassicSession>();
  session->connect(co);

  shcore::ShellNotifications::get()->notify("SN_SESSION_CONNECTED", session);

  return session;
}

shcore::Value::Map_type_ref ClassicSession::get_status() {
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  try {
    auto result = _session->query("select DATABASE(), USER() limit 1");
    auto row = result->fetch_one();
    if (row) {
      (*status)["SESSION_TYPE"] = shcore::Value("Classic");
      //        (*status)["DEFAULT_SCHEMA"] =
      //          shcore::Value(_connection_options.has_schema() ?
      //                        _connection_options.get_schema() : "");

      std::string current_schema;
      if (!row->is_null(0)) current_schema = row->get_string(0);

      (*status)["CURRENT_SCHEMA"] = shcore::Value(current_schema);
      (*status)["CURRENT_USER"] = shcore::Value(row->get_string(1));
      (*status)["CONNECTION_ID"] =
          shcore::Value(uint64_t(_session->get_connection_id()));

      //(*status)["SKIP_UPDATES"] = shcore::Value(???);

      (*status)["SERVER_INFO"] = shcore::Value(_session->get_server_info());

      (*status)["PROTOCOL_VERSION"] = shcore::Value(
          (_session->is_compression_enabled() ? "Compressed " : "Classic ") +
          std::to_string(_session->get_protocol_info()));

      if (_session->is_compression_enabled()) {
        std::string compr("Enabled");
        auto res = _session->query("show status like 'Compression_algorithm';");
        auto status_row = res->fetch_one();
        if (status_row && !status_row->is_null(1))
          compr += " (" + status_row->get_as_string(1) + ")";

        (*status)["COMPRESSION"] = shcore::Value(compr);
      } else {
        (*status)["COMPRESSION"] = shcore::Value(std::string("Disabled"));
      }

      (*status)["CONNECTION"] = shcore::Value(_session->get_connection_info());
      //(*status)["INSERT_ID"] = shcore::Value(???);
    }

    const char *cipher = _session->get_ssl_cipher();
    if (cipher != NULL) {
      result = _session->query("show session status like 'ssl_version';");
      row = result->fetch_one();
      std::string version;
      if (row) {
        version = " " + row->get_string(1);
      }
      (*status)["SSL_CIPHER"] = shcore::Value(cipher + version);
    }

    result = _session->query(
        "select @@character_set_client, @@character_set_connection, "
        "@@character_set_server, @@character_set_database, "
        "concat(@@version, ' ', @@version_comment) as version, "
        "@@socket, @@port, @@datadir, @@character_set_results "
        "limit 1");

    row = result->fetch_one();

    if (row) {
      (*status)["CLIENT_CHARSET"] = shcore::Value(row->get_string(0));
      (*status)["CONNECTION_CHARSET"] = shcore::Value(row->get_string(1));
      (*status)["SERVER_CHARSET"] = shcore::Value(row->get_string(2));
      (*status)["SCHEMA_CHARSET"] = shcore::Value(row->get_string(3));
      (*status)["SERVER_VERSION"] = shcore::Value(row->get_string(4));
      (*status)["RESULTS_CHARSET"] = shcore::Value(row->get_string(8));

      mysqlshdk::db::Transport_type transport_type =
          mysqlshdk::db::Transport_type::Socket;
      if (_connection_options.has_transport_type()) {
        transport_type = _connection_options.get_transport_type();
      }
      if (transport_type == mysqlshdk::db::Transport_type::Tcp) {
        (*status)["TCP_PORT"] = shcore::Value(row->get_int(6));
      } else if (transport_type == mysqlshdk::db::Transport_type::Socket) {
        const std::string datadir = row->get_string(7);
        const std::string socket = row->get_string(5);
        const std::string socket_abs_path = shcore::path::normalize(
            shcore::path::join_path(std::vector<std::string>{datadir, socket}));
        (*status)["UNIX_SOCKET"] = shcore::Value(socket_abs_path);
      }

      unsigned long ver = mysql_get_client_version();
      std::stringstream sv;
      sv << ver / 10000 << "." << (ver % 10000) / 100 << "." << ver % 100;
      (*status)["CLIENT_LIBRARY"] = shcore::Value(sv.str());

      if (_session->get_stats())
        (*status)["SERVER_STATS"] = shcore::Value(_session->get_stats());

      try {
        if (_connection_options.get_transport_type() ==
            mysqlshdk::db::Transport_type::Tcp)
          (*status)["TCP_PORT"] = shcore::Value(_connection_options.get_port());
      } catch (...) {
      }
      //(*status)["PROTOCOL_COMPRESSED"] = row->get_value(3);

      // STATUS

      // SAFE UPDATES
    }
  } catch (const shcore::Exception &e) {
    (*status)["STATUS_ERROR"] = shcore::Value(e.format());
  }

  return status;
}

REGISTER_HELP_FUNCTION(startTransaction, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_STARTTRANSACTION, R"*(
Starts a transaction context on the server.

Calling this function will turn off the autocommit mode on the server.

All the operations executed after calling this function will take place only
when commit() is called.

All the operations executed after calling this function, will be discarded if
rollback() is called.

When commit() or rollback() are called, the server autocommit mode will return
back to it's state before calling <<<startTransaction>>>().
)*");
/**
 * $(CLASSICSESSION_STARTTRANSACTION_BRIEF)
 *
 * $(CLASSICSESSION_STARTTRANSACTION)
 */
#if DOXYGEN_JS
Undefined ClassicSession::startTransaction() {}
#elif DOXYGEN_PY
None ClassicSession::start_transaction() {}
#endif
void ClassicSession::start_transaction() {
  if (_tx_deep == 0) execute_sql("start transaction", shcore::Array_t());

  _tx_deep++;
}

std::shared_ptr<ClassicResult> ClassicSession::_start_transaction() {
  auto result = std::dynamic_pointer_cast<mysqlshdk::db::mysql::Result>(
      execute_sql("start transaction"));

  return std::make_shared<ClassicResult>(result);
}

REGISTER_HELP_FUNCTION(commit, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_COMMIT, R"*(
Commits all the operations executed after a call to <<<startTransaction>>>().

All the operations executed after calling <<<startTransaction>>>() will take place
when this function is called.

The server autocommit mode will return back to it's state before calling
<<<startTransaction>>>().
)*");
/**
 * $(CLASSICSESSION_COMMIT_BRIEF)
 *
 * $(CLASSICSESSION_COMMIT)
 */
#if DOXYGEN_JS
Undefined ClassicSession::commit() {}
#elif DOXYGEN_PY
None ClassicSession::commit() {}
#endif

void ClassicSession::commit() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0) execute_sql("commit", shcore::Array_t());
}

std::shared_ptr<ClassicResult> ClassicSession::_commit() {
  auto result = std::dynamic_pointer_cast<mysqlshdk::db::mysql::Result>(
      execute_sql("commit"));

  return std::make_shared<ClassicResult>(result);
}

// Documentation of rollback function
REGISTER_HELP_FUNCTION(rollback, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_ROLLBACK, R"*(
Discards all the operations executed after a call to <<<startTransaction>>>().

All the operations executed after calling <<<startTransaction>>>() will be discarded
when this function is called.

The server autocommit mode will return back to it's state before calling
<<<startTransaction>>>().
)*");
/**
 * $(CLASSICSESSION_ROLLBACK_BRIEF)
 *
 * $(CLASSICSESSION_ROLLBACK)
 */
#if DOXYGEN_JS
Undefined ClassicSession::rollback() {}
#elif DOXYGEN_PY
None ClassicSession::rollback() {}
#endif
void ClassicSession::rollback() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0) execute_sql("rollback", shcore::Array_t());
}

uint64_t ClassicSession::get_connection_id() const {
  return _session->get_connection_id();
}

std::shared_ptr<ClassicResult> ClassicSession::_rollback() {
  auto result = std::dynamic_pointer_cast<mysqlshdk::db::mysql::Result>(
      execute_sql("rollback"));

  return std::make_shared<ClassicResult>(result);
}

REGISTER_HELP_FUNCTION(isOpen, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_ISOPEN, R"*(
Returns true if session is known to be open.

@returns A boolean value indicating if the session is still open.

Returns true if the session is still open and false otherwise.

@note This function may return true if connection is lost.
)*");
/**
 * $(CLASSICSESSION_ISOPEN_BRIEF)
 *
 * $(CLASSICSESSION_ISOPEN)
 */
#if DOXYGEN_JS
Bool ClassicSession::isOpen() {}
#elif DOXYGEN_PY
bool ClassicSession::is_open() {}
#endif
bool ClassicSession::is_open() const { return _session && _session->is_open(); }

std::string ClassicSession::query_one_string(const std::string &query,
                                             int field) {
  auto result = execute_sql(query);
  auto row = result->fetch_one();
  if (row) {
    return row->get_string(field);
  }
  return "";
}

socket_t ClassicSession::_get_socket_fd() const {
  if (!_session || !_session->is_open())
    throw std::invalid_argument("Session is not open");
  return _session->get_socket_fd();
}

REGISTER_HELP_FUNCTION(trackSystemVariable, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_TRACKSYSTEMVARIABLE, R"*(
Enables session tracking of the given system variable.

@param variable Name of the system variable to track.
@returns The new value of the session_track_system_variables session variable.

Appends the given system variable name to the session_track_system_variables,
session variable enabling session tracking of changes to it.
Currently supported variables:
@li SQL_MODE
)*");
/**
 * $(CLASSICSESSION_TRACKSYSTEMVARIABLE_BRIEF)
 *
 * $(CLASSICSESSION_TRACKSYSTEMVARIABLE)
 */
#if DOXYGEN_JS
String ClassicSession::trackSystemVariable(String variable) {}
#elif DOXYGEN_PY
str ClassicSession::track_system_variable(str variable) {}
#endif
std::string ClassicSession::track_system_variable(const std::string &variable) {
  if (!_session || !_session->is_open())
    throw std::invalid_argument("Session is not open");

  return ShellBaseSession::track_system_variable(variable);
}

REGISTER_HELP_FUNCTION(setOptionTrackerFeatureId, ClassicSession);
REGISTER_HELP_FUNCTION_TEXT(CLASSICSESSION_SETOPTIONTRACKERFEATUREID, R"*(
Defines the id of an option to be reported as used by the option tracker.

@param feature_id The id of the option to be marked as used.
)*");
/**
 * $(CLASSICSESSION_SETOPTIONTRACKERFEATUREID_BRIEF)
 *
 * $(CLASSICSESSION_SETOPTIONTRACKERFEATUREID)
 */
#if DOXYGEN_JS
Undefined ClassicSession::setOptionTrackerFeatureId(String feature_id) {}
#elif DOXYGEN_PY
None ClassicSession::set_option_tracker_feature_id(str feature_id) {}
#endif

void ClassicSession::set_option_tracker_feature_id(
    const std::string &feature_id) {
  _session->set_option_tracker_feature_id(feature_id);
}

void ClassicSession::set_option_tracker_feature_id(
    shcore::option_tracker::Shell_feature feature_id) {
  _session->set_option_tracker_feature_id(feature_id);
}
