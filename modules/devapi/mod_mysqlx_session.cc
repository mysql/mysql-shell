/*
 * Copyright (c) 2014, 2021, Oracle and/or its affiliates.
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

#include "mod_mysqlx_session.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "modules/devapi/mod_mysqlx_constants.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_schema.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/devapi/mod_mysqlx_session_sql.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/object_factory.h"
#include "mysqlshdk/include/scripting/proxy_object.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/base_shell.h"  // for options
#include "mysqlshdk/include/shellcore/shell_core.h"
#include "mysqlshdk/include/shellcore/shell_notifications.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_uuid.h"

#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define mystrdup _strdup
#else
#define mystrdup strdup
#endif

using namespace mysqlsh;
using namespace shcore;
using namespace mysqlsh::mysqlx;

#ifdef _WIN32
#define strcasecmp _stricmp
#endif
// Documentation of BaseSession class
REGISTER_HELP_CLASS(Session, mysqlx);
REGISTER_HELP(SESSION_BRIEF,
              "Enables interaction with a MySQL Server using the X Protocol.");
REGISTER_HELP(SESSION_DETAIL,
              "Document Store functionality can be used through this object, "
              "in addition to SQL.");
REGISTER_HELP(SESSION_DETAIL1,
              "This class allows performing database operations such as:");
REGISTER_HELP(SESSION_DETAIL2, "@li Schema management operations.");
REGISTER_HELP(SESSION_DETAIL3, "@li Access to relational tables.");
REGISTER_HELP(SESSION_DETAIL4, "@li Access to Document Store collections.");
REGISTER_HELP(SESSION_DETAIL5, "@li Enabling/disabling warning generation.");
REGISTER_HELP(SESSION_DETAIL6, "@li Retrieval of connection information.");

// Documentation of Session class
REGISTER_HELP(SESSION_GLOBAL_BRIEF,
              "Represents the currently open MySQL session.");

Session::Session() : _case_sensitive_table_names(false), _savepoint_counter(0) {
  init();
  _session = mysqlshdk::db::mysqlx::Session::create();
}

Session::Session(std::shared_ptr<mysqlshdk::db::mysqlx::Session> session)
    : _case_sensitive_table_names(false) {
  init();

  // TODO(alfredo) maybe can remove _connection_options ivar
  _connection_options = session->get_connection_options();
  _session = session;

  _connection_id = _session->get_connection_id();
}

Session::Session(const Session &s)
    : ShellBaseSession(s),
      std::enable_shared_from_this<Session>(s),
      _case_sensitive_table_names(false),
      _savepoint_counter(0) {
  init();
  _session = mysqlshdk::db::mysqlx::Session::create();
}

Session::~Session() {
  if (is_open()) close();
}

void Session::init() {
  expose("close", &Session::close);
  expose("setFetchWarnings", &Session::set_fetch_warnings, "enable");
  expose("startTransaction", &Session::_start_transaction);
  expose("commit", &Session::_commit);
  expose("rollback", &Session::_rollback);

  expose("createSchema", &Session::_create_schema, "name");
  expose("getSchema", &Session::get_schema, "name");
  expose("getSchemas", &Session::get_schemas);
  expose("dropSchema", &Session::drop_schema, "name");
  expose("setSavepoint", &Session::set_savepoint, "?name");
  expose("releaseSavepoint", &Session::release_savepoint, "name");
  expose("rollbackTo", &Session::rollback_to, "name");
  add_property("uri", "getUri");
  add_property("sshUri", "getSshUri");
  add_property("defaultSchema", "getDefaultSchema");
  add_property("currentSchema", "getCurrentSchema");

  expose("isOpen", &Session::is_open);
  expose("sql", &Session::sql, "statement");
  expose("setCurrentSchema", &Session::_set_current_schema, "name");
  expose("quoteName", &Session::quote_name, "id");

  expose("runSql", &Session::run_sql, "query", "?args");

  expose("_getSocketFd", &Session::_get_socket_fd);
  expose("_fetchNotice", &Session::_fetch_notice);
  expose("_enableNotices", &Session::_enable_notices, "noticeTypes");

  _schemas.reset(new shcore::Value::Map_type);

  // Prepares the cache handling
  auto generator = [this](const std::string &name) {
    return shcore::Value::wrap<Schema>(new Schema(shared_from_this(), name));
  };
  update_schema_cache = [generator, this](const std::string &name,
                                          bool /* exists */) {
    DatabaseObject::update_cache(name, generator, true, _schemas);
  };

  _session_uuid = shcore::Id_generator::new_document_id();
}

REGISTER_HELP_FUNCTION(isOpen, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_ISOPEN, R"*(
Returns true if session is known to be open.

@returns A boolean value indicating if the session is still open.

Returns true if the session is still open and false otherwise.

@note This function may return true if connection is lost.
)*");
/**
 * $(SESSION_ISOPEN_BRIEF)
 *
 * $(SESSION_ISOPEN)
 */
#if DOXYGEN_JS
Bool Session::isOpen() {}
#elif DOXYGEN_PY
bool Session::is_open() {}
#endif
bool Session::is_open() const { return _session->is_open(); }

std::string Session::get_uuid() {
  std::string new_uuid = shcore::Id_generator::new_document_id();

  // To guarantee the same Node id us used
  // (in case a random number is generated when MAC address is not available)
  // We copy the _session_uuid on top of the new uuid node
  std::copy(_session_uuid.begin(), _session_uuid.begin() + 12,
            new_uuid.begin());

  return new_uuid;
}

void Session::connect(const mysqlshdk::db::Connection_options &data) {
  try {
    _connection_options = data;

    _session->connect(_connection_options);

    _connection_id = _session->get_connection_id();

    if (_connection_options.has_schema())
      update_schema_cache(_connection_options.get_schema(), true);
  }
  CATCH_AND_TRANSLATE();
}

void Session::set_option(const char *option, int value) {
  if (strcmp(option, "trace_protocol") == 0) {
    if (is_open()) _session->enable_protocol_trace(value != 0);
  } else {
    throw shcore::Exception::argument_error(
        std::string("Unknown option ").append(option));
  }
}

uint64_t Session::get_connection_id() const {
  return _session->get_connection_id();
}

bool Session::table_name_compare(const std::string &n1, const std::string &n2) {
  if (_case_sensitive_table_names)
    return n1 == n2;
  else
    return strcasecmp(n1.c_str(), n2.c_str()) == 0;
}

// Documentation of close function
REGISTER_HELP_FUNCTION(close, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_CLOSE, R"*(
Closes the session.

After closing the session it is still possible to make read only operations to
gather metadata info, like <<<getTable>>>(name) or <<<getSchemas>>>().
)*");
/**
 * $(SESSION_CLOSE_BRIEF)
 *
 * $(SESSION_CLOSE)
 */
#if DOXYGEN_JS
Undefined Session::close() {}
#elif DOXYGEN_PY
None Session::close() {}
#endif
void Session::close() {
  try {
    // Connection must be explicitly closed, we can't rely on the
    // automatic destruction because if shared across different objects
    // it may remain open

    log_debug(
        "Closing session: %s",
        uri(mysqlshdk::db::uri::formats::scheme_user_transport()).c_str());

    if (_session->is_open()) {
      _session->close();
    }
  } catch (const std::exception &e) {
    log_warning("Error occurred closing session: %s", e.what());
  }

  _session = mysqlshdk::db::mysqlx::Session::create();
}

// Documentation of createSchema function
REGISTER_HELP_FUNCTION(createSchema, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_CREATESCHEMA, R"*(
Creates a schema on the database and returns the corresponding object.

@param name A string value indicating the schema name.

@returns The created schema object.

@throw A MySQL error is thrown if fails creating the schema.
)*");

/**
 * $(SESSION_CREATESCHEMA_BRIEF)
 *
 * $(SESSION_CREATESCHEMA)
 */
#if DOXYGEN_JS
Schema Session::createSchema(String name) {}
#elif DOXYGEN_PY
Schema Session::create_schema(str name) {}
#endif
std::shared_ptr<Schema> Session::_create_schema(const std::string &name) {
  create_schema(name);

  return std::dynamic_pointer_cast<Schema>(_schemas->at(name).as_object());
}

void Session::create_schema(const std::string &name) {
  std::string statement = sqlstring("create schema ! charset='utf8mb4'", 0)
                          << name;
  execute_sql(statement);

  // if reached this point it indicates that there were no errors
  update_schema_cache(name, true);
}

void Session::set_current_schema(const std::string &name) {
  execute_sql(sqlstring("use !", 0) << name);
}

REGISTER_HELP_FUNCTION(setSavepoint, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_SETSAVEPOINT, R"*(
Creates or replaces a transaction savepoint with the given name.

@param name Optional string with the name to be assigned to the transaction
save point.

@returns The name of the transaction savepoint.

When working with transactions, using savepoints allows rolling back operations
executed after the savepoint without terminating the transaction.

Use this function to set a savepoint within a transaction.

If this function is called with the same name of another savepoint set
previously, the original savepoint will be deleted and a new one will be
created.

If the name is not provided an auto-generated name as 'TXSP#' will be assigned,
where # is a consecutive number that guarantees uniqueness of the savepoint at
Session level.
)*");
/**
 * $(SESSION_SETSAVEPOINT_BRIEF)
 *
 * $(SESSION_SETSAVEPOINT)
 */
#if DOXYGEN_JS
String Session::setSavepoint(String name) {}
#elif DOXYGEN_PY
str Session::set_savepoint(str name) {}
#endif
std::string Session::set_savepoint(const std::string &name) {
  std::string new_name(name);

  if (new_name.empty())
    new_name = "TXSP" + std::to_string(++_savepoint_counter);

  _session->execute(sqlstring("savepoint !", 0) << new_name);

  return new_name;
}

REGISTER_HELP_FUNCTION(releaseSavepoint, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_RELEASESAVEPOINT, R"*(
Removes a savepoint defined on a transaction.

@param name string with the name of the savepoint to be removed.

Removes a named savepoint from the set of savepoints defined on the current
transaction. This does not affect the operations executed on the transaction
since no commit or rollback occurs.

It is an error trying to remove a savepoint that does not exist.
)*");
/**
 * $(SESSION_RELEASESAVEPOINT_BRIEF)
 *
 * $(SESSION_RELEASESAVEPOINT)
 */
#if DOXYGEN_JS
Undefined Session::releaseSavepoint(String name) {}
#elif DOXYGEN_PY
None Session::release_savepoint(str name) {}
#endif
void Session::release_savepoint(const std::string &name) {
  _session->execute(sqlstring("release savepoint !", 0) << name);
}

REGISTER_HELP_FUNCTION(rollbackTo, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_ROLLBACKTO, R"*(
Rolls back the transaction to the named savepoint without terminating the
transaction.

@param name string with the name of the savepoint for the rollback operation.

Modifications that the current transaction made to rows after the savepoint was
defined will be rolled back.

The given savepoint will not be removed, but any savepoint defined after the
given savepoint was defined will be removed.

It is an error calling this operation with an unexisting savepoint.
)*");
/**
 * $(SESSION_ROLLBACKTO_BRIEF)
 *
 * $(SESSION_ROLLBACKTO)
 */

#if DOXYGEN_JS
Undefined Session::rollbackTo(String name) {}
#elif DOXYGEN_PY
None Session::rollback_to(str name) {}
#endif
void Session::rollback_to(const std::string &name) {
  _session->execute(sqlstring("rollback to !", 0) << name);
}

// Documentation of getDefaultSchema function
REGISTER_HELP_PROPERTY(defaultSchema, Session);
REGISTER_HELP(SESSION_DEFAULTSCHEMA_BRIEF,
              "Retrieves the Schema configured as default for the session.");
REGISTER_HELP_FUNCTION(getDefaultSchema, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_GETDEFAULTSCHEMA, R"*(
Retrieves the Schema configured as default for the session.

@returns A Schema object or Null
)*");
#if DOXYGEN_JS || DOXYGEN_PY
/**
 * $(SESSION_GETDEFAULTSCHEMA_BRIEF)
 *
 * $(SESSION_GETDEFAULTSCHEMA)
 */
#if DOXYGEN_JS
Schema Session::getDefaultSchema() {}
#elif DOXYGEN_PY
Schema Session::get_default_schema() {}
#endif

REGISTER_HELP_PROPERTY(uri, Session);
REGISTER_HELP(SESSION_URI_BRIEF, "Retrieves the URI for the current session.");
REGISTER_HELP_FUNCTION(getUri, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_GETURI, R"*(
Retrieves the URI for the current session.

@return A string representing the connection data.
)*");
/**
 * $(SESSION_GETURI_BRIEF)
 *
 * $(SESSION_GETURI)
 */
#if DOXYGEN_JS
String Session::getUri() {}
#elif DOXYGEN_PY
str Session::get_uri() {}
#endif
#endif

// Documentation of getCurrentSchema function
REGISTER_HELP_PROPERTY(currentSchema, Session);
REGISTER_HELP(SESSION_CURRENTSCHEMA_BRIEF, "${SESSION_GETCURRENTSCHEMA_BRIEF}");
REGISTER_HELP_FUNCTION(getCurrentSchema, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_GETCURRENTSCHEMA, R"*(
Retrieves the active schema on the session.

@return A Schema object if a schema is active on the session.
)*");
/**
 * $(SESSION_GETCURRENTSCHEMA_BRIEF)
 *
 * $(SESSION_GETCURRENTSCHEMA)
 */
#if DOXYGEN_JS
Schema Session::getCurrentSchema() {}
#elif DOXYGEN_PY
Schema Session::get_current_schema() {}
#endif

std::string Session::get_current_schema() {
  if (is_open()) {
    auto result = execute_sql("select schema()");
    if (auto row = result->fetch_one()) {
      if (row && !row->is_null(0)) return row->get_string(0);
    }
  } else {
    throw std::logic_error("Not connected");
  }

  return "";
}

// Documentation of getSchema function
REGISTER_HELP_FUNCTION(getSchema, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_GETSCHEMA, R"*(
Retrieves a Schema object from the current session through it's name.

@param name The name of the Schema object to be retrieved.

@returns The Schema object with the given name.

@throw RuntimeError If the given name is not a valid schema.
)*");
/**
 * $(SESSION_GETSCHEMA_BRIEF)
 *
 * $(SESSION_GETSCHEMA)
 * \sa Schema
 */
#if DOXYGEN_JS
Schema Session::getSchema(String name) {}
#elif DOXYGEN_PY
Schema Session::get_schema(str name) {}
#endif
std::shared_ptr<Schema> Session::get_schema(const std::string &name) {
  std::shared_ptr<Schema> ret_val;
  std::string type = "Schema";

  if (name.empty())
    throw Exception::runtime_error("Schema name must be specified");

  std::string found_name = db_object_exists(type, name, "");

  if (!found_name.empty()) {
    update_schema_cache(found_name, true);

    ret_val =
        std::dynamic_pointer_cast<Schema>(_schemas->at(found_name).as_object());
  } else {
    update_schema_cache(name, false);

    throw Exception::runtime_error("Unknown database '" + name + "'");
  }

  if (current_shell_options()->get().devapi_schema_object_handles)
    ret_val->update_cache();

  return ret_val;
}

REGISTER_HELP_FUNCTION(getSchemas, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_GETSCHEMAS, R"*(
Retrieves the Schemas available on the session.

@returns A List containing the Schema objects available on the session.
)*");
/**
 * $(SESSION_GETSCHEMAS_BRIEF)
 *
 * $(SESSION_GETSCHEMAS)
 */
#if DOXYGEN_JS
List Session::getSchemas() {}
#elif DOXYGEN_PY
list Session::get_schemas() {}
#endif
shcore::Array_t Session::get_schemas() {
  auto schemas = shcore::make_array();

  if (is_open()) {
    auto result = execute_sql("show databases;");

    while (const mysqlshdk::db::IRow *row = result->fetch_one()) {
      std::string schema_name;
      if (!row->is_null(0)) schema_name = row->get_string(0);
      // TODO(alfredo) review whether caching of all schemas shouldn't be off
      if (!schema_name.empty()) {
        update_schema_cache(schema_name, true);

        schemas->push_back((*_schemas)[schema_name]);
      }
    }
  } else {
    throw std::logic_error("Not connected");
  }

  return schemas;
}

REGISTER_HELP_FUNCTION(setFetchWarnings, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_SETFETCHWARNINGS, R"*(
Enables or disables warning generation.

@param enable Boolean value to enable or disable the warnings.

Warnings are generated sometimes when database operations are executed, such as
SQL statements.

On a Node session the warning generation is disabled by default. This function
can be used to enable or disable the warning generation based on the received
parameter.

When warning generation is enabled, the warnings will be available through the
result object returned on the executed operation.
)*");
/**
 * $(SESSION_SETFETCHWARNINGS_BRIEF)
 *
 * $(SESSION_SETFETCHWARNINGS)
 */
#if DOXYGEN_JS
Result Session::setFetchWarnings(Boolean enable) {}
#elif DOXYGEN_PY
Result Session::set_fetch_warnings(bool enable) {}
#endif
std::shared_ptr<Result> Session::set_fetch_warnings(bool enable) {
  std::string command = enable ? "enable_notices" : "disable_notices";

  auto notices = shcore::make_array();
  notices->emplace_back("warnings");

  auto command_args = shcore::make_dict();
  command_args->emplace("notice", std::move(notices));

  return std::make_shared<Result>(execute_mysqlx_stmt(command, command_args));
}

REGISTER_HELP_FUNCTION(dropSchema, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_DROPSCHEMA, R"*(
Drops the schema with the specified name.

@param name The name of the schema to be dropped.

@returns Nothing.
)*");
/**
 * $(SESSION_DROPSCHEMA_BRIEF)
 *
 * $(SESSION_DROPSCHEMA)
 */
#if DOXYGEN_JS
Undefined Session::dropSchema(String name) {}
#elif DOXYGEN_PY
None Session::drop_schema(str name) {}
#endif
void Session::drop_schema(const std::string &name) {
  execute_sql(sqlstring("drop schema if exists !", 0) << name);
  if (_schemas->find(name) != _schemas->end()) _schemas->erase(name);
}

/*
 * This function verifies if the given object exist in the database, works for
 * schemas, tables, views and collections.
 * The check for tables, views and collections is done is done based on the
 * type. If type is not specified and an object with the name is found, the type
 * will be returned.
 *
 * Returns the name of the object as exists in the database.
 */
std::string Session::db_object_exists(std::string &type,
                                      const std::string &name,
                                      const std::string &owner) {
  Interruptible intr(this);
  // match must be exact, since both branches below use LIKE and both escape
  // their arguments it's enough to just escape the wildcards
  std::string escaped_name = escape_wildcards(name);

  if (type == "Schema") {
    auto result =
        execute_sql(sqlstring("show databases like ?", 0) << escaped_name);
    if (auto row = result->fetch_one()) {
      return row->get_string(0);
    }
  } else {
    shcore::Dictionary_t args = shcore::make_dict();
    args->set("schema", shcore::Value(owner));
    args->set("pattern", shcore::Value(escaped_name));
    auto result = execute_mysqlx_stmt("list_objects", args);
    if (auto row = result->fetch_one()) {
      std::string object_name = row->get_string(0);
      std::string object_type = row->get_string(1);

      if (type.empty()) {
        type = object_type;
        return object_name;
      } else {
        type = shcore::str_upper(type);
        if (type == object_type) return object_name;
      }
    }
  }
  return "";
}

shcore::Value::Map_type_ref Session::get_status() {
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  (*status)["SESSION_TYPE"] = shcore::Value("X");

  (*status)["NODE_TYPE"] = shcore::Value(get_node_type());

  (*status)["DEFAULT_SCHEMA"] = shcore::Value(
      _connection_options.has_schema() ? _connection_options.get_schema() : "");

  try {
    auto result = execute_sql("select DATABASE(), USER() limit 1");
    const mysqlshdk::db::IRow *row = result->fetch_one();

    (*status)["CURRENT_SCHEMA"] =
        shcore::Value(row->is_null(0) ? "" : row->get_string(0));
    (*status)["CURRENT_USER"] =
        shcore::Value(row->is_null(1) ? "" : row->get_string(1));
    (*status)["CONNECTION_ID"] = shcore::Value(_session->get_connection_id());
    const char *cipher = _session->get_ssl_cipher();
    if (cipher) {
      result = execute_sql("show session status like 'mysqlx_ssl_version';");
      row = result->fetch_one();
      (*status)["SSL_CIPHER"] =
          shcore::Value(std::string(cipher) + " " + row->get_string(1));
    }
    //(*status)["SKIP_UPDATES"] = shcore::Value(???);

    // (*status)["SERVER_INFO"] = shcore::Value(_conn->get_server_info());

    (*status)["PROTOCOL_VERSION"] = shcore::Value("X protocol");
    unsigned long ver = mysql_get_client_version();
    std::stringstream sv;
    sv << ver / 10000 << "." << (ver % 10000) / 100 << "." << ver % 100;
    (*status)["CLIENT_LIBRARY"] = shcore::Value(sv.str());
    // (*status)["INSERT_ID"] = shcore::Value(???);

    result = execute_sql(
        "select @@character_set_client, @@character_set_connection, "
        "@@character_set_server, @@character_set_database, "
        "concat(@@version, \" \", @@version_comment) as version, "
        "@@mysqlx_socket, @@mysqlx_port, @@datadir, @@character_set_results "
        "limit 1");
    row = result->fetch_one();
    if (row) {
      (*status)["CLIENT_CHARSET"] =
          shcore::Value(row->is_null(0) ? "" : row->get_string(0));
      (*status)["CONNECTION_CHARSET"] =
          shcore::Value(row->is_null(1) ? "" : row->get_string(1));
      (*status)["SERVER_CHARSET"] =
          shcore::Value(row->is_null(2) ? "" : row->get_string(2));
      (*status)["SCHEMA_CHARSET"] =
          shcore::Value(row->is_null(3) ? "" : row->get_string(3));
      (*status)["SERVER_VERSION"] =
          shcore::Value(row->is_null(4) ? "" : row->get_string(4));
      (*status)["RESULTS_CHARSET"] =
          shcore::Value(row->is_null(8) ? "" : row->get_string(8));

      mysqlshdk::db::Transport_type transport_type =
          mysqlshdk::db::Transport_type::Tcp;
      if (_connection_options.has_transport_type()) {
        transport_type = _connection_options.get_transport_type();
        std::stringstream ss;
        if (_connection_options.has_host())
          ss << _connection_options.get_host() << " via ";
        else
          ss << "localhost via ";
        ss << to_string(transport_type);
        (*status)["CONNECTION"] = shcore::Value(ss.str());
      } else {
        (*status)["CONNECTION"] = shcore::Value("localhost via TCP/IP");
      }
      if (transport_type == mysqlshdk::db::Transport_type::Tcp) {
        (*status)["TCP_PORT"] =
            shcore::Value(row->is_null(6) ? "" : row->get_as_string(6));
      } else if (transport_type == mysqlshdk::db::Transport_type::Socket) {
        const std::string datadir = (row->is_null(5) ? "" : row->get_string(7));
        const std::string socket = (row->is_null(5) ? "" : row->get_string(5));
        const std::string socket_abs_path = shcore::path::normalize(
            shcore::path::join_path(std::vector<std::string>{datadir, socket}));
        (*status)["UNIX_SOCKET"] = shcore::Value(socket_abs_path);
      }
    }

    result = execute_sql(
        "show status where variable_name in "
        "('Mysqlx_bytes_sent_compressed_payload', "
        "'Mysqlx_compression_algorithm');");
    row = result->fetch_one();
    if (row && !row->is_null(1) &&
        shcore::lexical_cast<int>(row->get_string(1), 0) > 0) {
      std::string compr_status = "Enabled";
      row = result->fetch_one();
      if (row && !row->is_null(1) && !row->get_string(1).empty())
        compr_status += " (" + row->get_string(1) + ")";
      (*status)["COMPRESSION"] = shcore::Value(compr_status);
    } else {
      (*status)["COMPRESSION"] = shcore::Value(std::string("Disabled"));
    }

    result = execute_sql("SHOW GLOBAL STATUS LIKE 'Uptime';");
    row = result->fetch_one();
    std::stringstream su;
    su << "Uptime: " << row->get_string(1) << " \n";
    (*status)["SERVER_STATS"] = shcore::Value(su.str());
    // (*status)["PROTOCOL_COMPRESSED"] = row->get_value(3);

    // SAFE UPDATES
  } catch (const shcore::Exception &e) {
    (*status)["STATUS_ERROR"] = shcore::Value(e.format());
  }

  return status;
}

// Documentation of startTransaction function
REGISTER_HELP_FUNCTION(startTransaction, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_STARTTRANSACTION, R"*(
Starts a transaction context on the server.

@returns A SqlResult object.

Calling this function will turn off the autocommit mode on the server.

All the operations executed after calling this function will take place only
when commit() is called.

All the operations executed after calling this function, will be discarded if
rollback() is called.

When commit() or rollback() are called, the server autocommit mode will return
back to it's state before calling <<<startTransaction>>>().
)*");
/**
 * $(SESSION_STARTTRANSACTION_BRIEF)
 *
 * $(SESSION_STARTTRANSACTION)
 */
#if DOXYGEN_JS
Result Session::startTransaction() {}
#elif DOXYGEN_PY
Result Session::start_transaction() {}
#endif
void Session::start_transaction() {
  if (_tx_deep == 0) execute_sql("start transaction");

  _tx_deep++;
}

std::shared_ptr<SqlResult> Session::_start_transaction() {
  auto result = std::dynamic_pointer_cast<mysqlshdk::db::mysqlx::Result>(
      execute_sql("start transaction"));

  return std::make_shared<SqlResult>(result);
}

// Documentation of commit function
REGISTER_HELP_FUNCTION(commit, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_COMMIT, R"*(
Commits all the operations executed after a call to <<<startTransaction>>>().

@returns A SqlResult object.

All the operations executed after calling <<<startTransaction>>>() will take
place when this function is called.

The server autocommit mode will return back to it's state before calling
<<<startTransaction>>>().
)*");
/**
 * $(SESSION_COMMIT_BRIEF)
 *
 * $(SESSION_COMMIT)
 */
#if DOXYGEN_JS
Result Session::commit() {}
#elif DOXYGEN_PY
Result Session::commit() {}
#endif
void Session::commit() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0) execute_sql("commit");
}

std::shared_ptr<SqlResult> Session::_commit() {
  auto result = std::dynamic_pointer_cast<mysqlshdk::db::mysqlx::Result>(
      execute_sql("commit"));

  return std::make_shared<SqlResult>(result);
}

// Documentation of rollback function
REGISTER_HELP_FUNCTION(rollback, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_ROLLBACK, R"*(
Discards all the operations executed after a call to <<<startTransaction>>>().

@returns A SqlResult object.

All the operations executed after calling <<<startTransaction>>>() will be
discarded when this function is called.

The server autocommit mode will return back to it's state before calling
<<<startTransaction>>>().
)*");
/**
 * $(SESSION_ROLLBACK_BRIEF)
 *
 * $(SESSION_ROLLBACK)
 */
#if DOXYGEN_JS
Result Session::rollback() {}
#elif DOXYGEN_PY
Result Session::rollback() {}
#endif
void Session::rollback() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0) execute_sql("rollback");
}

std::shared_ptr<SqlResult> Session::_rollback() {
  auto result = std::dynamic_pointer_cast<mysqlshdk::db::mysqlx::Result>(
      execute_sql("rollback"));

  return std::make_shared<SqlResult>(result);
}

std::string Session::query_one_string(const std::string &query, int field) {
  auto result = execute_sql(query);
  if (auto row = result->fetch_one()) {
    if (!row->is_null(field)) {
      return row->get_string(field);
    }
  }
  return "";
}

void Session::kill_query() {
  uint64_t cid = get_connection_id();

  auto session = mysqlshdk::db::mysqlx::Session::create();
  try {
    session->connect(_connection_options);
    session->query(shcore::sqlstring("kill query ?", 0) << cid);
  } catch (const std::exception &e) {
    log_warning("Error cancelling SQL query: %s", e.what());
  }
}

static ::xcl::Argument_value convert(const shcore::Value &value);

static ::xcl::Argument_object convert_map(const shcore::Dictionary_t &args) {
  ::xcl::Argument_object object;
  for (const auto &iter : *args) {
    object[iter.first] = convert(iter.second);
  }
  return object;
}

static ::xcl::Argument_array convert_array(const shcore::Array_t &args) {
  ::xcl::Argument_array object;
  for (const auto &iter : *args) {
    object.push_back(convert(iter));
  }
  return object;
}

static ::xcl::Argument_value convert(const shcore::Value &value) {
  switch (value.type) {
    case shcore::Bool:
      return xcl::Argument_value(value.as_bool());
    case shcore::UInteger:
      return xcl::Argument_value(value.as_uint());
    case shcore::Integer:
      return xcl::Argument_value(value.as_int());
    case shcore::String:
      return xcl::Argument_value(value.get_string());
    case shcore::Float:
      return xcl::Argument_value(value.as_double());
    case shcore::Map:
      return xcl::Argument_value(convert_map(value.as_map()));
    case shcore::Array:
      return xcl::Argument_value(convert_array(value.as_array()));
    case shcore::Null:
      return xcl::Argument_value();
    case shcore::Object:
    case shcore::MapRef:
    case shcore::Function:
    case shcore::Undefined:
      break;
  }
  throw std::invalid_argument("Invalid argument value: " + value.descr());
}

static ::xcl::Argument_array convert_args(const shcore::Dictionary_t &args) {
  ::xcl::Argument_array cargs;
  cargs.push_back(xcl::Argument_value(convert_map(args)));
  return cargs;
}

static ::xcl::Argument_array convert_args(const shcore::Array_t &args) {
  ::xcl::Argument_array cargs;

  if (args) {
    for (const shcore::Value &arg : *args) {
      cargs.push_back(convert(arg));
    }
  }
  return cargs;
}

shcore::Value Session::_execute_mysqlx_stmt(const std::string &command,
                                            const shcore::Dictionary_t &args) {
  return _execute_stmt("mysqlx", command, convert_args(args), true);
}

std::shared_ptr<mysqlshdk::db::mysqlx::Result> Session::execute_mysqlx_stmt(
    const std::string &command, const shcore::Dictionary_t &args) {
  return execute_stmt("mysqlx", command, convert_args(args));
}

shcore::Value Session::_execute_stmt(const std::string &ns,
                                     const std::string &command,
                                     const ::xcl::Argument_array &args,
                                     bool expect_data) {
  if (expect_data) {
    SqlResult *result = new SqlResult(execute_stmt(ns, command, args));
    return shcore::Value::wrap(result);
  } else {
    Result *result = new Result(execute_stmt(ns, command, args));
    return shcore::Value::wrap(result);
  }
  return {};
}

std::shared_ptr<mysqlshdk::db::mysqlx::Result> Session::execute_stmt(
    const std::string &ns, const std::string &command,
    const ::xcl::Argument_array &args) {
  Interruptible intr(this);
  auto result = std::static_pointer_cast<mysqlshdk::db::mysqlx::Result>(
      _session->execute_stmt(ns, command, args));
  result->pre_fetch_rows();
  return result;
}

std::shared_ptr<mysqlshdk::db::IResult> Session::execute_sql(
    const std::string &statement, const shcore::Array_t &args) {
  try {
    return execute_stmt("sql", statement, convert_args(args));
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

std::shared_ptr<shcore::Object_bridge> Session::create(
    const mysqlshdk::db::Connection_options &co_) {
  auto co = co_;
  // DevAPI getSession uses ssl-mode = REQUIRED by default if no
  // ssl-ca or ssl-capath are specified
  if (!co.get_ssl_options().has_mode() &&
      !co.has_value(mysqlshdk::db::kSslCa) &&
      !co.has_value(mysqlshdk::db::kSslCaPath)) {
    co.get_ssl_options().set_default(mysqlshdk::db::kSslMode,
                                     mysqlshdk::db::kSslModeRequired);
  }

  // before creating a normal session we need to establish ssh if needed:
  co.set_default_data();
  auto &ssh = co.get_ssh_options_handle(mysqlshdk::db::k_default_mysql_x_port);
  if (ssh.has_data()) {
    mysqlshdk::ssh::current_ssh_manager()->create_tunnel(&ssh);
  }

  const auto session = std::make_shared<Session>();
  session->connect(co);

  shcore::ShellNotifications::get()->notify("SN_SESSION_CONNECTED", session);

  return session;
}

// Documentation of sql function
REGISTER_HELP_FUNCTION(sql, Session);
REGISTER_HELP(SESSION_SQL_CHAINED, "SqlExecute.sql.[bind].[execute]");
REGISTER_HELP_FUNCTION_TEXT(SESSION_SQL, R"*(
Creates a SqlExecute object to allow running the received SQL statement on the
target MySQL Server.

@param sql A string containing the SQL statement to be executed.

@return A SqlExecute object.

This method creates an SqlExecute object which is a SQL execution handler.

The SqlExecute class has functions that allow defining the way the statement
will be executed and allows doing parameter binding.

The received SQL is set on the execution handler.
)*");
/**
 * $(SESSION_SQL_BRIEF)
 *
 * $(SESSION_SQL)
 *
 * JavaScript Example
 * \code{.js}
 * var sql = session.sql("select * from mydb.students where  age > ?");
 * var result = sql.bind(18).execute();
 * \endcode
 * \sa SqlExecute
 */
#if DOXYGEN_JS
SqlExecute Session::sql(String sql) {}
#elif DOXYGEN_PY
SqlExecute Session::sql(str sql) {}
#endif
std::shared_ptr<SqlExecute> Session::sql(const std::string &statement) {
  auto sql_execute = std::make_shared<SqlExecute>(
      std::static_pointer_cast<Session>(shared_from_this()));

  return sql_execute->sql(statement);
}

REGISTER_HELP_FUNCTION(runSql, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_RUNSQL, R"*(
Executes a query and returns the corresponding SqlResult object.

@param query the SQL query to execute against the database.
@param args Optional list of literals to use when replacing ? placeholders in
the query string.

@returns An SqlResult object.

@throw LogicError if there's no open session.
@throw ArgumentError if the parameters are invalid.
)*");
/**
 * $(SESSION_RUNSQL_BRIEF)
 *
 * $(SESSION_RUNSQL)
 */

#if DOXYGEN_JS
SqlResult Session::runSql(String query, Array args) {}
#elif DOXYGEN_PY
SqlResult Session::run_sql(str query, list args) {}
#endif
std::shared_ptr<SqlResult> Session::run_sql(const std::string sql,
                                            const shcore::Array_t &args) {
  // Will return the result of the SQL execution
  // In case of error will be Undefined
  if (!_session || !_session->is_open())
    throw Exception::logic_error("Not connected.");

  Interruptible intr(this);
  auto sql_execute = std::make_shared<SqlExecute>(
      std::static_pointer_cast<Session>(shared_from_this()));
  sql_execute->set_sql(sql);
  if (args) {
    for (const auto &value : *args) {
      sql_execute->add_bind(value);
    }
  }

  return sql_execute->execute();
}

REGISTER_HELP_PROPERTY(uri, Session);
REGISTER_HELP(SESSION_URI_BRIEF, "${SESSION_GETURI_BRIEF}");
REGISTER_HELP_FUNCTION(getUri, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_GETURI, R"*(
Retrieves the URI for the current session.

@return A string representing the connection data.
)*");
/**
 * $(SESSION_GETURI_BRIEF)
 *
 * $(SESSION_GETURI)
 */
#if DOXYGEN_JS
String Session::getSshUri() {}
#elif DOXYGEN_PY
str Session::get_ssh_uri() {}
#endif

REGISTER_HELP_PROPERTY(sshUri, Session);
REGISTER_HELP(SESSION_SSHURI_BRIEF, "${SESSION_GETSSHURI_BRIEF}");
REGISTER_HELP_FUNCTION(getSshUri, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_GETSSHURI, R"*(
Retrieves the SSH URI for the current session.

@return A string representing the SSH connection data.
)*");
/**
 * $(SESSION_GETSSHURI_BRIEF)
 *
 * $(SESSION_GETSSHURI)
 */
#if DOXYGEN_JS
String Session::getSshUri() {}
#elif DOXYGEN_PY
str Session::get_ssh_uri() {}
#endif

Value Session::get_member(const std::string &prop) const {
  Value ret_val;
  Session *session = const_cast<Session *>(this);

  if (prop == "__connection_info") {
    // FIXME: temporary code until ISession refactoring
    std::string info = _session->get_connection_info();
    return Value(info);
  }

  if (prop == "uri") {
    ret_val = shcore::Value(uri());
  } else if (prop == "sshUri") {
    ret_val = shcore::Value(ssh_uri());
  } else if (prop == "defaultSchema") {
    if (_connection_options.has_schema()) {
      ret_val =
          shcore::Value(session->get_schema(_connection_options.get_schema()));
    } else {
      ret_val = Value::Null();
    }
  } else if (prop == "currentSchema") {
    std::string name = session->get_current_schema();

    if (!name.empty()) {
      ret_val = shcore::Value(session->get_schema(name));
    } else {
      ret_val = Value::Null();
    }
  } else {
    ret_val = ShellBaseSession::get_member(prop);
  }
  return ret_val;
}

// Documentation of quoteName function
REGISTER_HELP_FUNCTION(quoteName, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_QUOTENAME, R"*(
Escapes the passed identifier.

@param id The identifier to be quoted.

@return A String containing the escaped identifier.
)*");
/**
 * $(SESSION_QUOTENAME_BRIEF)
 *
 * $(SESSION_QUOTENAME)
 */
#if DOXYGEN_JS
String Session::quoteName(String id) {}
#elif DOXYGEN_PY
str Session::quote_name(str id) {}
#endif
std::string Session::quote_name(const std::string &id) {
  return get_quoted_name(id);
}

// Documentation of setCurrentSchema function
REGISTER_HELP_FUNCTION(setCurrentSchema, Session);
REGISTER_HELP_FUNCTION_TEXT(SESSION_SETCURRENTSCHEMA, R"*(
Sets the current schema for this session, and returns the schema object for it.

@param name the name of the new schema to switch to.

@return the Schema object for the new schema.

At the database level, this is equivalent at issuing the following SQL query:

  use <new-default-schema>;
)*");
/**
 * $(SESSION_SETCURRENTSCHEMA_BRIEF)
 *
 * $(SESSION_SETCURRENTSCHEMA)
 */
#if DOXYGEN_JS
Schema Session::setCurrentSchema(String name) {}
#elif DOXYGEN_PY
Schema Session::set_current_schema(str name) {}
#endif
std::shared_ptr<Schema> Session::_set_current_schema(const std::string &name) {
  if (is_open()) {
    set_current_schema(name);
  } else {
    throw std::logic_error("Not connected");
  }

  return get_schema(name);
}

socket_t Session::_get_socket_fd() const {
  if (!_session || !_session->is_open())
    throw std::invalid_argument("Session is not open");
  return _session->get_socket_fd();
}

// This is a temporary API that may change
void Session::_enable_notices(const std::vector<std::string> &notices) {
  std::vector<mysqlshdk::db::mysqlx::GlobalNotice::Type> types;

  for (const auto &n : notices) {
    if (n == "GRViewChanged") {
      types.push_back(mysqlshdk::db::mysqlx::GlobalNotice::GRViewChanged);
    } else {
      throw std::invalid_argument("Unknown notice type " + n);
    }
  }

  if (!types.empty()) {
    _session->enable_notices(types);

    if (!m_notices_enabled) {
      m_notices_enabled = true;
      _session->add_notice_listener(
          [this](const mysqlshdk::db::mysqlx::GlobalNotice &notice) {
            if (notice.type ==
                mysqlshdk::db::mysqlx::GlobalNotice::GRViewChanged) {
              m_notices.push_back(shcore::make_dict(
                  "type", shcore::Value("GRViewChanged"), "view_id",
                  shcore::Value(notice.info.gr_view_change.view_id)));
            }
            return true;
          });
    }
  }
}

shcore::Dictionary_t Session::_fetch_notice() {
  if (!m_notices.empty()) {
    auto tmp = m_notices.front();
    m_notices.pop_front();
    return tmp;
  }
  return nullptr;
}
