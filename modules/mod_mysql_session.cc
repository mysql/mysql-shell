/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_mysql_session.h"

#include <set>
#include <thread>
#include <string>
#include <vector>

#include "scripting/object_factory.h"
#include "shellcore/shell_core.h"
#include "scripting/lang_base.h"
#include "shellcore/shell_notifications.h"

#include "scripting/proxy_object.h"
#include "modules/mysqlxtest_utils.h"

#include "modules/mod_mysql_resultset.h"
#include "modules/mod_mysql_schema.h"
#include "utils/utils_general.h"
#include "utils/utils_sqlstring.h"
#include "utils/utils_path.h"
#include "utils/utils_time.h"
#include "shellcore/utils_help.h"
#include "modules/mod_utils.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::mysql;
using namespace shcore;

REGISTER_OBJECT(mysql, ClassicSession);

// Documentation for ClassicSession class
REGISTER_HELP(CLASSICSESSION_INTERACTIVE_BRIEF, "Represents the currently "\
"open MySQL session.");
REGISTER_HELP(CLASSICSESSION_BRIEF, "Enables interaction with a MySQL Server "\
"using the MySQL Protocol.");
REGISTER_HELP(CLASSICSESSION_DETAIL, "Provides facilities to execute queries "\
"and retrieve database objects.");
REGISTER_HELP(CLASSICSESSION_PARENTS, "ShellDevelopmentSession,"\
"ShellBaseSession");
ClassicSession::ClassicSession() {
  init();
}

ClassicSession::ClassicSession(const ClassicSession &session)
    : ShellBaseSession(session),
      std::enable_shared_from_this<mysqlsh::mysql::ClassicSession>(session),
      _session(session._session) {
  init();
}

ClassicSession::~ClassicSession() {
  if (is_open())
    close();
}

void ClassicSession::init() {
  //_schema_proxy.reset(new Proxy_object(std::bind(&ClassicSession::get_db, this, _1)));

  add_property("uri", "getUri");
  add_property("defaultSchema", "getDefaultSchema");
  add_property("currentSchema", "getCurrentSchema");

  add_method("close", std::bind(&ClassicSession::_close, this, _1), "data");
  add_method("runSql", std::bind(&ClassicSession::run_sql, this, _1),
    "stmt", shcore::String,
    NULL);
  add_method("query", std::bind(&ClassicSession::query, this, _1),
    "stmt", shcore::String,
    NULL);

  add_method("isOpen", std::bind(&ClassicSession::_is_open, this, _1), NULL);
  add_method("createSchema", std::bind(&ClassicSession::_create_schema, this, _1), "name", shcore::String, NULL);
  add_method("getSchema", std::bind(&ClassicSession::_get_schema, this, _1), "name", shcore::String, NULL);
  add_method("getSchemas", std::bind(&ClassicSession::get_schemas, this, _1), NULL);
  add_method("setCurrentSchema", std::bind(&ClassicSession::_set_current_schema, this, _1), "name", shcore::String, NULL);
  add_method("startTransaction", std::bind(&ClassicSession::_start_transaction, this, _1), "data");
  add_method("commit", std::bind(&ClassicSession::_commit, this, _1), "data");
  add_method("rollback", std::bind(&ClassicSession::_rollback, this, _1), "data");
  add_method("dropSchema", std::bind(&ClassicSession::_drop_schema, this, _1), "data");
  add_method("dropTable", std::bind(&ClassicSession::drop_schema_object, this, _1, "Table"), "data");
  add_method("dropView", std::bind(&ClassicSession::drop_schema_object, this, _1, "View"), "data");

  _schemas.reset(new shcore::Value::Map_type);

  // Prepares the cache handling
  auto generator = [this](const std::string& name) {return shcore::Value::wrap<ClassicSchema>(new ClassicSchema(shared_from_this(), name)); };
  update_schema_cache = [generator, this](const std::string &name, bool exists) {DatabaseObject::update_cache(name, generator, exists, _schemas); };
}

void ClassicSession::connect(
    const mysqlshdk::db::Connection_options &connection_options) {
  _connection_options = connection_options;

  try {
    // All connections should use mode = VERIFY_CA if no ssl mode is specified
    // and either ssl-ca or ssl-capath are specified
    if (!_connection_options.has_value(mysqlshdk::db::kSslMode) &&
        (_connection_options.has_value(mysqlshdk::db::kSslCa) ||
         _connection_options.has_value(mysqlshdk::db::kSslCaPath))) {
      _connection_options.set(mysqlshdk::db::kSslMode,
                             {mysqlshdk::db::kSslModeVerifyCA});
    }

    _session = mysqlshdk::db::mysql::Session::create();

    _session->connect(_connection_options);

    if (_connection_options.has_schema())
      update_schema_cache(_connection_options.get_schema(), true);
  }
  CATCH_AND_TRANSLATE();
}

std::string ClassicSession::get_ssl_cipher() const {
  const char *cipher = _session->get_ssl_cipher();
  return cipher ? cipher : "";
}

// Documentation of close function
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
  if (_session)
    _session->close();

  _session.reset();

  try {
    // this shouldn't be getting clled from teh d-tor...
    ShellNotifications::get()->notify("SN_SESSION_CLOSED", shared_from_this());
  } catch (std::bad_weak_ptr) {
    ShellNotifications::get()->notify("SN_SESSION_CLOSED", {});
  }
}

Value ClassicSession::_close(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("close").c_str());

  close();

  return shcore::Value();
}

REGISTER_HELP(CLASSICSESSION_ISOPEN_BRIEF, "Returns true if session is "\
  "known to be open.");
REGISTER_HELP(CLASSICSESSION_ISOPEN_RETURNS, "@returns A boolean value "\
  "indicating if the session is still open.");
REGISTER_HELP(CLASSICSESSION_ISOPEN_DETAIL, "Returns true if the session is "\
    "still open and false otherwise. Note: may return true if connection "\
    "is lost.");

shcore::Value ClassicSession::_is_open(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("isOpen").c_str());

  return shcore::Value(is_open());
}


// Documentation of runSql function
REGISTER_HELP(CLASSICSESSION_RUNSQL_BRIEF, "Executes a query and returns the "
"corresponding ClassicResult object.");
REGISTER_HELP(CLASSICSESSION_RUNSQL_PARAM1, "@param query the SQL query to "
"execute against the database.");
REGISTER_HELP(CLASSICSESSION_RUNSQL_PARAM2, "@param args list of literals to "
"use when replacing ? placeholders in the query string.");
REGISTER_HELP(CLASSICSESSION_RUNSQL_RETURNS, "@returns A ClassicResult "
"object.");
REGISTER_HELP(CLASSICSESSION_RUNSQL_EXCEPTION, "@exception An exception is "
"thrown if an error occurs on the SQL execution.");

//! $(CLASSICSESSION_RUNSQL_BRIEF)
#if DOXYGEN_CPP
//! \param args should contain the SQL query to execute against the database.
#else
//! $(CLASSICSESSION_RUNSQL_PARAM1)
//! $(CLASSICSESSION_RUNSQL_PARAM2)
#endif
/**
* $(CLASSICSESSION_RUNSQL_RETURNS)
*
* $(CLASSICSESSION_RUNSQL_EXCEPTION)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::runSql(String query, Array args) {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::run_sql(str query, list args) {}
#endif
Value ClassicSession::run_sql(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("runSql").c_str());

  if (args.size() > 1 && args[1].type != shcore::Array) {
    throw Exception::argument_error(
        "Argument #2 to runSql() must be a list of values");
  }
  // Will return the result of the SQL execution
  // In case of error will be Undefined
  Value ret_val;
  if (!_session || !_session->is_open()) {
    throw Exception::logic_error("Not connected.");
  } else {
    try {
      Interruptible intr(this);
      ret_val =
          execute_sql(args.string_at(0),
                      args.size() > 1 ? args.array_at(1) : shcore::Array_t());
    } catch (shcore::Exception & e) {
      // Connection lost, sends a notification
      std::shared_ptr<ClassicSession> myself =
          std::const_pointer_cast<ClassicSession>(shared_from_this());

      if (e.code() == CR_SERVER_GONE_ERROR || e.code() == CR_SERVER_LOST)
        ShellNotifications::get()->notify(
            "SN_SESSION_CONNECTION_LOST",
            std::dynamic_pointer_cast<Cpp_object_bridge>(myself));

      // Rethrows the exception for normal flow
      throw;
    }
  }

  return ret_val;
}


REGISTER_HELP(CLASSICSESSION_QUERY_BRIEF, "Executes a query and returns the "
  "corresponding ClassicResult object.");
REGISTER_HELP(CLASSICSESSION_QUERY_PARAM1, "@param query the SQL query string "
  "to execute, with optional ? placeholders");
REGISTER_HELP(CLASSICSESSION_QUERY_PARAM2, "@param args list of literals to "
"use when replacing ? placeholders in the query string.");
REGISTER_HELP(CLASSICSESSION_QUERY_RETURNS, "@returns A ClassicResult "
  "object.");
REGISTER_HELP(CLASSICSESSION_QUERY_EXCEPTION, "@exception An exception is "
  "thrown if an error occurs on the SQL execution.");

//! $(CLASSICSESSION_QUERY_BRIEF)
#if DOXYGEN_CPP
//! \param args should contain the SQL query to execute against the database.
#else
//! $(CLASSICSESSION_QUERY_PARAM1)
//! $(CLASSICSESSION_QUERY_PARAM2)
#endif
/**
* $(CLASSICSESSION_QUERY_RETURNS)
*
* $(CLASSICSESSION_QUERY_EXCEPTION)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::query(String query, Array args = []) {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::query(str query, list args = []) {}
#endif
Value ClassicSession::query(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("query").c_str());

  if (args.size() > 1 && args[1].type != shcore::Array) {
    throw Exception::argument_error(
        "Argument #2 to query() must be a list of values");
  }

  // Will return the result of the SQL execution
  // In case of error will be Undefined
  Value ret_val;
  if (!_session || !_session->is_open()) {
    throw Exception::logic_error("Not connected.");
  } else {
    try {
      Interruptible intr(this);
      ret_val =
          execute_sql(args.string_at(0),
                      args.size() > 1 ? args.array_at(1) : shcore::Array_t());
    } catch (shcore::Exception & e) {
      // Connection lost, sends a notification
      std::shared_ptr<ClassicSession> myself =
          std::const_pointer_cast<ClassicSession>(shared_from_this());

      // TODO(alfredo) this notification isn't really needed, the
      // REPL loop should check the state of the global session and reconnect
      // if needed
      if (e.code() == CR_SERVER_GONE_ERROR || e.code() == CR_SERVER_LOST)
        ShellNotifications::get()->notify(
            "SN_SESSION_CONNECTION_LOST",
            std::dynamic_pointer_cast<Cpp_object_bridge>(myself));

      // Rethrows the exception for normal flow
      throw;
    }
  }
  return ret_val;
}

shcore::Object_bridge_ref ClassicSession::raw_execute_sql(
    const std::string &query) {
  return execute_sql(query, {}).as_object();
}

shcore::Value ClassicSession::execute_sql(
    const std::string &query, const shcore::Array_t &args) {
  Value ret_val;
  if (!_session || !_session->is_open()) {
    throw Exception::logic_error("Not connected.");
  } else {
    if (query.empty()) {
      throw Exception::argument_error("No query specified.");
    } else {
      Interruptible intr(this);
      try {
        MySQL_timer timer;
        timer.start();
        ClassicResult *result;
        ret_val = Value::wrap(result = new ClassicResult(
            std::dynamic_pointer_cast<mysqlshdk::db::mysql::Result>(
                _session->query(sub_query_placeholders(query, args)))));
        timer.end();
        result->set_execution_time(timer.raw_duration());
      } catch (const shcore::database_error &error) {
        throw shcore::Exception::mysql_error_with_code_and_state(
            error.error(), error.code(), error.sqlstate().c_str());
      }
    }
  }
  return ret_val;
}

// We need to hide this from doxygen to avoif warnings
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
std::shared_ptr<ClassicResult> ClassicSession::execute_sql(
    const std::string &query) {
  if (!_session || !_session->is_open()) {
    throw Exception::logic_error("Not connected.");
  } else {
    if (query.empty()) {
      throw Exception::argument_error("No query specified.");
    } else {
      MySQL_timer timer;
      timer.start();
      auto result = std::shared_ptr<ClassicResult>(new ClassicResult(
          std::dynamic_pointer_cast<mysqlshdk::db::mysql::Result>(
              _session->query(query))));
      timer.end();
      result->set_execution_time(timer.raw_duration());
      return result;
    }
  }
}
#endif

//Documentation of createSchema function
REGISTER_HELP(CLASSICSESSION_CREATESCHEMA_BRIEF, "Creates a schema on the database and returns the corresponding object.");
REGISTER_HELP(CLASSICSESSION_CREATESCHEMA_PARAM, "@param name A string value indicating the schema name.");
REGISTER_HELP(CLASSICSESSION_CREATESCHEMA_RETURNS, "@returns The created schema object.");
REGISTER_HELP(CLASSICSESSION_CREATESCHEMA_EXCEPTION, "@exception An exception is thrown if an error occurs creating the Session.");

/**
* $(CLASSICSESSION_CREATESCHEMA_BRIEF)
*
* $(CLASSICSESSION_CREATESCHEMA_PARAM)
*
* $(CLASSICSESSION_CREATESCHEMA_RETURNS)
*
* $(CLASSICSESSION_CREATESCHEMA_EXCEPTION)
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::createSchema(String name) {}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::create_schema(str name) {}
#endif
Value ClassicSession::_create_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("createSchema").c_str());
  std::string name;

  try {
    name = args.string_at(0);

    create_schema(name);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createSchema"));

  return (*_schemas)[name];
}

void ClassicSession::create_schema(const std::string &name) {
  if (!_session || !_session->is_open()) {
    throw Exception::logic_error("Not connected.");
  } else {
    // Options are the statement and optionally options to modify
    // How the resultset is created.

    if (name.empty()) {
      throw Exception::argument_error("The schema name can not be empty.");
    } else {
      std::string statement = sqlstring("create schema !", 0) << name;
      execute_sql(statement, shcore::Array_t());

      (*_schemas)[name] = shcore::Value::wrap<ClassicSchema>(new ClassicSchema(shared_from_this(), name));
    }
  }
}


// Documentation of getDefaultSchema function
REGISTER_HELP(CLASSICSESSION_DEFAULTSCHEMA_BRIEF, "Retrieves the "\
"ClassicSchema configured as default for the session.");
REGISTER_HELP(CLASSICSESSION_GETDEFAULTSCHEMA_BRIEF, "Retrieves the "\
"ClassicSchema configured as default for the session.");
REGISTER_HELP(CLASSICSESSION_GETDEFAULTSCHEMA_RETURNS, "@returns A "\
"ClassicSchema object or Null");
REGISTER_HELP(CLASSICSESSION_GETDEFAULTSCHEMA_DETAIL, "If the configured "\
"schema is not valid anymore Null wil be returned.");

/**
* $(CLASSICSESSION_GETDEFAULTSCHEMA_BRIEF)
*
* $(CLASSICSESSION_GETDEFAULTSCHEMA_RETURNS)
*
* $(CLASSICSESSION_GETDEFAULTSCHEMA_DETAIL)
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::getDefaultSchema() {}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::get_default_schema() {}
#endif

// Documentation of getCurrentSchema function
REGISTER_HELP(CLASSICSESSION_CURRENTSCHEMA_BRIEF, "Retrieves the "\
"ClassicSchema that is active as current on the session.");
REGISTER_HELP(CLASSICSESSION_GETCURRENTSCHEMA_BRIEF, "Retrieves the "\
"ClassicSchema that is active as current on the session.");
REGISTER_HELP(CLASSICSESSION_GETCURRENTSCHEMA_RETURN, "@return A "\
"ClassicSchema object or Null");
REGISTER_HELP(CLASSICSESSION_GETCURRENTSCHEMA_DETAIL, "The current schema is "\
"configured either throug setCurrentSchema(String name) or by using the USE "\
"statement.");

/**
* $(CLASSICSESSION_GETCURRENTSCHEMA_BRIEF)
*
* $(CLASSICSESSION_GETCURRENTSCHEMA_RETURNS)
*
* $(CLASSICSESSION_GETCURRENTSCHEMA_BRIEF)
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::getCurrentSchema() {}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::get_current_schema() {}
#endif

// Documentation of getCurrentSchema function
REGISTER_HELP(CLASSICSESSION_URI_BRIEF, "Retrieves the URI for the current "\
"session.");
REGISTER_HELP(CLASSICSESSION_GETURI_BRIEF, "Retrieves the URI for the current "\
"session.");
REGISTER_HELP(CLASSICSESSION_GETURI_RETURNS, "@return A string representing "\
"the connection data.");

/**
* $(SHELLBASESESSION_GETURI_BRIEF)
*
* $(CLASSICSESSION_GETURI_RETURNS)
*/
#if DOXYGEN_JS
String ClassicSession::getUri() {}
#elif DOXYGEN_PY
str ClassicSession::get_uri() {}
#endif

Value ClassicSession::get_member(const std::string &prop) const {
  // Retrieves the member first from the parent
  Value ret_val;

  if (prop == "__connection_info") {
    // FIXME: temporary code until ISession refactoring
    const char *i = _session->get_connection_info();
    return Value(i ? i : "");
  }

  if (prop == "uri") {
    ret_val = shcore::Value(uri());
  } else if (prop == "defaultSchema") {
    if (_connection_options.has_schema()) {
      ret_val = shcore::Value(const_cast<ClassicSession *>(this)->get_schema(
          _connection_options.get_schema()));
    } else {
      ret_val = Value::Null();
    }
  } else if (prop == "currentSchema") {
    ClassicSession *session = const_cast<ClassicSession *>(this);
    std::string name = session->_retrieve_current_schema();

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

std::string ClassicSession::_retrieve_current_schema() {
  std::string name;

  if (_session && _session->is_open()) {
    shcore::Argument_list query;
    query.push_back(Value("select schema()"));

    Value res = run_sql(query);

    std::shared_ptr<ClassicResult> rset = res.as_object<ClassicResult>();
    Value next_row = rset->fetch_one(shcore::Argument_list());

    if (next_row) {
      std::shared_ptr<mysqlsh::Row> row = next_row.as_object<mysqlsh::Row>();
      shcore::Value schema = row->get_member(0);

      if (schema)
        name = schema.as_string();
    }
  }

  return name;
}

void ClassicSession::_remove_schema(const std::string& name) {
  if (_schemas->find(name) != _schemas->end())
    _schemas->erase(name);
}

// Documentation of getSchema function
REGISTER_HELP(CLASSICSESSION_GETSCHEMA_BRIEF, "Retrieves a ClassicSchema object from the current session through it's name.");
REGISTER_HELP(CLASSICSESSION_GETSCHEMA_PARAM, "@param name The name of the ClassicSchema object to be retrieved.");
REGISTER_HELP(CLASSICSESSION_GETSCHEMA_RETURNS, "@returns The ClassicSchema object with the given name.");
REGISTER_HELP(CLASSICSESSION_GETSCHEMA_EXCEPTION, "@exception An exception is thrown if the given name is not a valid schema on the Session.");

//! $(CLASSICSESSION_GETSCHEMA_BRIEF)
#if DOXYGEN_CPP
//! \param args should contain the name of the ClassicSchema object to be retrieved.
#else
//! $(CLASSICSESSION_GETSCHEMA_PARAM)
#endif
/**
* $(CLASSICSESSION_GETSCHEMA_RETURNS)
*
* $(CLASSICSESSION_GETSCHEMA_EXCEPTION)
*
* \sa ClassicSchema
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::getSchema(String name) {}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::get_schema(str name) {}
#endif
shcore::Object_bridge_ref ClassicSession::get_schema(const std::string &name) {
  auto ret_val = ShellBaseSession::get_schema(name);

  auto dbobject = std::dynamic_pointer_cast<DatabaseObject>(ret_val);
  dbobject->update_cache();

  return ret_val;
}

shcore::Value ClassicSession::_get_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getSchema").c_str());
  shcore::Value ret_val;

  try {
    ret_val = shcore::Value(get_schema(args.string_at(0)));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getSchema"));

  return ret_val;
}

// Documentation of getSchemas function
REGISTER_HELP(CLASSICSESSION_GETSCHEMAS_BRIEF, "Retrieves the Schemas available on the session.");
REGISTER_HELP(CLASSICSESSION_GETSCHEMAS_RETURNS, "@returns A List containing the ClassicSchema objects available on the session.");

/**
* $(CLASSICSESSION_GETSCHEMAS_BRIEF)
*
* $(CLASSICSESSION_GETSCHEMAS_RETURNS)
*/
#if DOXYGEN_JS
List ClassicSession::getSchemas() {}
#elif DOXYGEN_PY
list ClassicSession::get_schemas() {}
#endif
shcore::Value ClassicSession::get_schemas(const shcore::Argument_list &args) {
  shcore::Value::Array_type_ref schemas(new shcore::Value::Array_type);

  if (_session && _session->is_open()) {
    shcore::Argument_list query;
    query.push_back(Value("show databases;"));

    Value res = run_sql(query);

    shcore::Argument_list args;
    std::shared_ptr<ClassicResult> rset = res.as_object<ClassicResult>();
    Value next_row = rset->fetch_one(args);
    std::shared_ptr<mysqlsh::Row> row;

    while (next_row) {
      row = next_row.as_object<mysqlsh::Row>();
      shcore::Value schema = row->get_member("Database");
      if (schema) {
        update_schema_cache(schema.as_string(), true);

        schemas->push_back((*_schemas)[schema.as_string()]);
      }

      next_row = rset->fetch_one(args);
    }
  }

  return shcore::Value(schemas);
}

// Documentation of setCurrentSchema function
REGISTER_HELP(CLASSICSESSION_SETCURRENTSCHEMA_BRIEF, "Sets the selected schema for this session's connection.");
REGISTER_HELP(CLASSICSESSION_SETCURRENTSCHEMA_RETURNS, "@returns The new schema.");

/**
* $(CLASSICSESSION_SETCURRENTSCHEMA_BRIEF)
*
* $(CLASSICSESSION_SETCURRENTSCHEMA_RETURNS)
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::setCurrentSchema(String schema) {}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::set_current_schema(str schema) {}
#endif
void ClassicSession::set_current_schema(const std::string &name) {
  shcore::Argument_list query;
  query.push_back(Value(sqlstring("use !", 0) << name));

  run_sql(query);
}

shcore::Value ClassicSession::_set_current_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("setCurrentSchema").c_str());

  if (_session && _session->is_open()) {
    std::string name = args[0].as_string();

    set_current_schema(name);
  } else
    throw Exception::runtime_error("ClassicSession not connected");

  return get_member("currentSchema");
}

std::shared_ptr<shcore::Object_bridge> ClassicSession::create(
    const shcore::Argument_list &args) {
  std::shared_ptr<ClassicSession> session(new ClassicSession());

  auto connection_options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::STRING);

  // DevAPI getClassicSession uses ssl-mode = REQUIRED by default if no
  // ssl-ca or ssl-capath are specified
  if (!connection_options.get_ssl_options().has_mode() &&
      !connection_options.has_value(mysqlshdk::db::kSslCa) &&
      !connection_options.has_value(mysqlshdk::db::kSslCaPath)) {
    connection_options.get_ssl_options().set_mode(
        mysqlshdk::db::Ssl_mode::Required);
  }

  session->connect(connection_options);

  shcore::ShellNotifications::get()->notify("SN_SESSION_CONNECTED", session);

  return std::dynamic_pointer_cast<shcore::Object_bridge>(session);
}

// Documentation of dropSchema function
REGISTER_HELP(CLASSICSESSION_DROPSCHEMA_BRIEF, "Drops the schema with the specified name.");
REGISTER_HELP(CLASSICSESSION_DROPSCHEMA_RETURNS, "@returns A ClassicResult object if succeeded.");
REGISTER_HELP(CLASSICSESSION_DROPSCHEMA_EXCEPTION, "@exception An error is raised if the schema did not exist.");

/**
* $(CLASSICSESSION_DROPSCHEMA_BRIEF)
*
* $(CLASSICSESSION_DROPSCHEMA_RETURNS)
*
* $(CLASSICSESSION_DROPSCHEMA_EXCEPTION)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::dropSchema(String name) {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::drop_schema(str name) {}
#endif
void ClassicSession::drop_schema(const std::string &name) {
  execute_sql(sqlstring("drop schema !", 0) << name, shcore::Array_t());

  _remove_schema(name);
}

shcore::Value ClassicSession::_drop_schema(const shcore::Argument_list &args) {
  std::string function = get_function_name("dropSchema");

  args.ensure_count(1, function.c_str());

  try {
    drop_schema(args[0].as_string());
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(function);


  return shcore::Value();
}

// Documentation of dropTable function
REGISTER_HELP(CLASSICSESSION_DROPTABLE_BRIEF, "Drops a table from the specified schema.");
REGISTER_HELP(CLASSICSESSION_DROPTABLE_RETURNS, "@returns A ClassicResult object if succeeded.");
REGISTER_HELP(CLASSICSESSION_DROPTABLE_EXCEPTION, "@exception An error is raised if the table did not exist.");

#if DOXYGEN_CPP
/**
 * Drops a table or a view from a specific ClassicSchema.
 * \param args contains the identification data for the object to be deleted.
 * \param type indicates the object type to be deleted
 *
 * args must contain two string entries: schema and table/view name.
 *
 * type must be either "Table" or "View"
 */
#else
/**
* $(CLASSICSESSION_DROPTABLE_BRIEF)
*
* $(CLASSICSESSION_DROPTABLE_RETURNS)
*
* $(CLASSICSESSION_DROPTABLE_EXCEPTION)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::dropTable(String schema, String name) {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::drop_table(str schema, str name) {}
#endif

// Documentation of dropView function
REGISTER_HELP(CLASSICSESSION_DROPVIEW_BRIEF, "Drops a view from the specified schema.");
REGISTER_HELP(CLASSICSESSION_DROPVIEW_RETURNS, "@returns A ClassicResult object if succeeded.");
REGISTER_HELP(CLASSICSESSION_DROPVIEW_EXCEPTION, "@exception An error is raised if the view did not exist.");

/**
* $(CLASSICSESSION_DROPVIEW_BRIEF)
*
* $(CLASSICSESSION_DROPVIEW_RETURNS)
*
* $(CLASSICSESSION_DROPVIEW_EXCEPTION)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::dropView(String schema, String name) {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::drop_view(str schema, str name) {}
#endif
#endif
shcore::Value ClassicSession::drop_schema_object(const shcore::Argument_list &args, const std::string& type) {
  std::string function = get_function_name("drop" + type);

  args.ensure_count(2, function.c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #1 is expected to be a string");

  if (args[1].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #2 is expected to be a string");

  std::string schema = args[0].as_string();
  std::string name = args[1].as_string();

  std::string statement;
  if (type == "Table")
    statement = "drop table !.!";
  else
    statement = "drop view !.!";

  statement = sqlstring(statement.c_str(), 0) << schema << name;

  Value ret_val = execute_sql(statement, shcore::Array_t());

  if (_schemas->count(schema)) {
    std::shared_ptr<ClassicSchema> schema_obj = std::static_pointer_cast<ClassicSchema>((*_schemas)[schema].as_object());
    if (schema_obj)
      schema_obj->_remove_object(name, type);
  }

  return ret_val;
}

/*
* This function verifies if the given object exist in the database, works for schemas, tables and views.
* The check for tables and views is done is done based on the type.
* If type is not specified and an object with the name is found, the type will be returned.
*/

std::string ClassicSession::db_object_exists(std::string &type, const std::string &name, const std::string& owner) {
  std::string statement;
  std::string ret_val;

  if (type == "Schema") {
    statement = sqlstring("show databases like ?", 0) << name;
    auto val_result = execute_sql(statement, shcore::Array_t());
    auto result = val_result.as_object<ClassicResult>();
    auto val_row = result->fetch_one(shcore::Argument_list());

    if (val_row) {
      auto row = val_row.as_object<mysqlsh::Row>();
      if (row)
        ret_val = row->get_member(0).as_string();
    }
  } else {
    statement = sqlstring("show full tables from ! like ?", 0) << owner << name;
    auto val_result = execute_sql(statement, shcore::Array_t());
    auto result = val_result.as_object<ClassicResult>();
    auto val_row = result->fetch_one(shcore::Argument_list());

    if (val_row) {
      auto row = val_row.as_object<mysqlsh::Row>();

      if (row) {
        std::string db_type = row->get_member(1).as_string();

        if (type == "Table" && (db_type == "BASE TABLE" || db_type == "LOCAL TEMPORARY"))
          ret_val = row->get_member(0).as_string();
        else if (type == "View" && (db_type == "VIEW" || db_type == "SYSTEM VIEW"))
          ret_val = row->get_member(0).as_string();
        else if (type.empty()) {
          ret_val = row->get_member(0).as_string();
          type = db_type;
        }
      }
    }
  }

  return ret_val;
}

// Documentation of startTransaction function
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_BRIEF, "Starts a transaction context on the server.");
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_RETURNS, "@returns A ClassicResult object.");
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_DETAIL, "Calling this function will turn off the autocommit mode on the server.");
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_DETAIL1, "All the operations executed after calling this function will take place only when commit() is called.");
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_DETAIL2, "All the operations executed after calling this function, will be discarded is rollback() is called.");
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_DETAIL3, "When commit() or rollback() are called, the server autocommit mode will return back to it's state before calling startTransaction().");

/**
* $(CLASSICSESSION_STARTTRANSACTION_BRIEF)
*
* $(CLASSICSESSION_STARTTRANSACTION_RETURNS)
*
* $(CLASSICSESSION_STARTTRANSACTION_DETAIL)
*
* $(CLASSICSESSION_STARTTRANSACTION_DETAIL1)
*
* $(CLASSICSESSION_STARTTRANSACTION_DETAIL2)
*
* $(CLASSICSESSION_STARTTRANSACTION_DETAIL3)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::startTransaction() {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::start_transaction() {}
#endif
shcore::Value ClassicSession::_start_transaction(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("startTransaction").c_str());

  return execute_sql("start transaction", shcore::Array_t());
}

// Documentation of commit function
REGISTER_HELP(CLASSICSESSION_COMMIT_BRIEF, "Commits all the operations executed after a call to startTransaction().");
REGISTER_HELP(CLASSICSESSION_COMMIT_RETURNS, "@returns A ClassicResult object.");
REGISTER_HELP(CLASSICSESSION_COMMIT_DETAIL, "All the operations executed after calling startTransaction() will take place when this function is called.");
REGISTER_HELP(CLASSICSESSION_COMMIT_DETAIL1, "The server autocommit mode will return back to it's state before calling startTransaction().");

/**
* $(CLASSICSESSION_COMMIT_BRIEF)
*
* $(CLASSICSESSION_COMMIT_RETURNS)
*
* $(CLASSICSESSION_COMMIT_DETAIL)
*
* $(CLASSICSESSION_COMMIT_DETAIL1)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::commit() {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::commit() {}
#endif
shcore::Value ClassicSession::_commit(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("commit").c_str());

  return execute_sql("commit", shcore::Array_t());
}

// Documentation of rollback function
REGISTER_HELP(CLASSICSESSION_ROLLBACK_BRIEF, "Discards all the operations executed after a call to startTransaction().");
REGISTER_HELP(CLASSICSESSION_ROLLBACK_RETURNS, "@returns A ClassicResult object.");
REGISTER_HELP(CLASSICSESSION_ROLLBACK_DETAIL, "All the operations executed after calling startTransaction() will be discarded when this function is called.");
REGISTER_HELP(CLASSICSESSION_ROLLBACK_DETAIL1, "The server autocommit mode will return back to it's state before calling startTransaction().");

/**
* $(CLASSICSESSION_ROLLBACK_BRIEF)
*
* $(CLASSICSESSION_ROLLBACK_RETURNS)
*
* $(CLASSICSESSION_ROLLBACK_DETAIL)
*
* $(CLASSICSESSION_ROLLBACK_DETAIL1)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::rollback() {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::rollback() {}
#endif
shcore::Value ClassicSession::_rollback(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("rollback").c_str());

  return execute_sql("rollback", shcore::Array_t());
}

shcore::Value::Map_type_ref ClassicSession::get_status() {
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  try {
    auto result = _session->query("select DATABASE(), USER() limit 1");
    auto row = result->fetch_one();
    if (row) {
      (*status)["SESSION_TYPE"] = shcore::Value("Classic");
      (*status)["NODE_TYPE"] = shcore::Value(get_node_type());
//        (*status)["DEFAULT_SCHEMA"] =
//          shcore::Value(_connection_options.has_schema() ?
//                        _connection_options.get_schema() : "");

      std::string current_schema;
      if (!row->is_null(0))
        current_schema = row->get_string(0);

      (*status)["CURRENT_SCHEMA"] = shcore::Value(current_schema);
      (*status)["CURRENT_USER"] = shcore::Value(row->get_string(1));
      (*status)["CONNECTION_ID"] = shcore::Value(uint64_t(_session->get_connection_id()));

      //(*status)["SKIP_UPDATES"] = shcore::Value(???);

      (*status)["SERVER_INFO"] = shcore::Value(_session->get_server_info());

      (*status)["PROTOCOL_VERSION"] =
          shcore::Value(std::string("classic ") +
                        std::to_string(_session->get_protocol_info()));
      (*status)["CONNECTION"] = shcore::Value(_session->get_connection_info());
      //(*status)["INSERT_ID"] = shcore::Value(???);
    }

    const char *cipher = _session->get_ssl_cipher();
    if (cipher != NULL) {
      result = _session->query("show session status like 'ssl_version';");
      row = result->fetch_one();
      std::string version;
      if (row) {
        version =  " " + row->get_string(1);
      }
      (*status)["SSL_CIPHER"] = shcore::Value(cipher + version);
      (*status)["SERVER_STATS"] = shcore::Value(_session->get_stats());
    }


    result = _session->query(
        "select @@character_set_client, @@character_set_connection, "
        "@@character_set_server, @@character_set_database, "
        "concat(@@version, \" \", @@version_comment) as version, "
        "@@socket, @@port, @@datadir "
        "limit 1");

    row = result->fetch_one();

    if (row) {
      (*status)["CLIENT_CHARSET"] = shcore::Value(row->get_string(0));
      (*status)["CONNECTION_CHARSET"] = shcore::Value(row->get_string(1));
      (*status)["SERVER_CHARSET"] = shcore::Value(row->get_string(2));
      (*status)["SCHEMA_CHARSET"] = shcore::Value(row->get_string(3));
      (*status)["SERVER_VERSION"] = shcore::Value(row->get_string(4));

      if (!_connection_options.has_transport_type() ||
          _connection_options.get_transport_type() ==
              mysqlshdk::db::Transport_type::Tcp) {
        (*status)["TCP_PORT"] = shcore::Value(row->get_int(6));
      } else if (_connection_options.get_transport_type() ==
                 mysqlshdk::db::Transport_type::Socket) {
          const std::string datadir = row->get_string(7);
          const std::string socket = row->get_string(5);
          const std::string socket_abs_path =
              shcore::path::normalize(shcore::path::join_path(
                  std::vector<std::string>{datadir, socket}));
          (*status)["UNIX_SOCKET"] = shcore::Value(socket_abs_path);
      }

      unsigned long ver = mysql_get_client_version();
      std::stringstream sv;
      sv << ver/10000 << "." << (ver%10000)/100 << "." << ver % 100;
      (*status)["CLIENT_LIBRARY"] = shcore::Value(sv.str());

      (*status)["SERVER_STATS"] = shcore::Value(_session->get_stats());

      try {
        if (_connection_options.get_transport_type() ==
            mysqlshdk::db::Transport_type::Tcp)
          (*status)["TCP_PORT"] =
              shcore::Value(_connection_options.get_port());
      } catch (...) {
      }
      //(*status)["PROTOCOL_COMPRESSED"] = row->get_value(3);

      // STATUS

      // SAFE UPDATES
    }
  } catch (shcore::Exception& e) {
    (*status)["STATUS_ERROR"] = shcore::Value(e.format());
  }

  return status;
}

void ClassicSession::start_transaction() {
  if (_tx_deep == 0)
    execute_sql("start transaction", shcore::Array_t());

  _tx_deep++;
}

void ClassicSession::commit() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute_sql("commit", shcore::Array_t());
}

void ClassicSession::rollback() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute_sql("rollback", shcore::Array_t());
}

uint64_t ClassicSession::get_connection_id() const {
  return _session->get_connection_id();
}

bool ClassicSession::is_open() const {
  return _session && _session->is_open();
}

std::string ClassicSession::query_one_string(const std::string &query,
                                             int field) {
  shcore::Value val_result = execute_sql(query, shcore::Array_t());
  auto result = val_result.as_object<mysql::ClassicResult>();
  shcore::Value val_row = result->fetch_one(shcore::Argument_list());
  if (val_row) {
    auto row = val_row.as_object<mysqlsh::Row>();
    if (row) {
      return row->get_member(field).as_string();
    }
  }
  return "";
}

void ClassicSession::kill_query() {
  uint64_t cid = get_connection_id();
  try {
    auto kill_session = mysqlshdk::db::mysql::Session::create();

    kill_session->connect(_session->get_connection_options());

    kill_session->execute("kill query " + std::to_string(cid));

    kill_session->close();
  } catch (std::exception &e) {
    log_warning("Error cancelling SQL query: %s", e.what());
  }
}
