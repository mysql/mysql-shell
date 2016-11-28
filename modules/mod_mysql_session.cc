/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "utils/utils_sqlstring.h"
#include "mod_mysql_session.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/shell_notifications.h"

#include "shellcore/proxy_object.h"
#include "mysqlxtest_utils.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <set>

#include "mysql_connection.h"
#include "mod_mysql_resultset.h"
#include "mod_mysql_schema.h"
#include "utils/utils_general.h"
#include "utils/utils_help.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::mysql;
using namespace shcore;

REGISTER_OBJECT(mysql, ClassicSession);

// Documentation for ClassicSession class
REGISTER_HELP(CLASSICSESSION_INTERACTIVE_BRIEF, "Represents the currently open MySQL session.");
REGISTER_HELP(CLASSICSESSION_BRIEF, "Enables interaction with a MySQL Server using the MySQL Protocol.");
REGISTER_HELP(CLASSICSESSION_DETAIL, "Provides facilities to execute queries and retrieve database objects.");

ClassicSession::ClassicSession() {
  init();
}

ClassicSession::ClassicSession(const ClassicSession& session) :
ShellDevelopmentSession(session), _conn(session._conn) {
  init();
}

void ClassicSession::init() {
  //_schema_proxy.reset(new Proxy_object(std::bind(&ClassicSession::get_db, this, _1)));

  add_property("currentSchema", "getCurrentSchema");

  add_method("close", std::bind(&ClassicSession::close, this, _1), "data");
  add_method("runSql", std::bind(&ClassicSession::run_sql, this, _1),
    "stmt", shcore::String,
    NULL);
  add_method("setCurrentSchema", std::bind(&ClassicSession::set_current_schema, this, _1), "name", shcore::String, NULL);
  add_method("startTransaction", std::bind(&ClassicSession::startTransaction, this, _1), "data");
  add_method("commit", std::bind(&ClassicSession::commit, this, _1), "data");
  add_method("rollback", std::bind(&ClassicSession::rollback, this, _1), "data");
  add_method("dropSchema", std::bind(&ClassicSession::drop_schema, this, _1), "data");
  add_method("dropTable", std::bind(&ClassicSession::drop_schema_object, this, _1, "Table"), "data");
  add_method("dropView", std::bind(&ClassicSession::drop_schema_object, this, _1, "View"), "data");

  _schemas.reset(new shcore::Value::Map_type);

  // Prepares the cache handling
  auto generator = [this](const std::string& name) {return shcore::Value::wrap<ClassicSchema>(new ClassicSchema(shared_from_this(), name)); };
  update_schema_cache = [generator, this](const std::string &name, bool exists) {DatabaseObject::update_cache(name, generator, exists, _schemas); };
}

Connection *ClassicSession::connection() {
  return _conn.get();
}

Value ClassicSession::connect(const Argument_list &args) {
  std::string function = class_name() + '.' + "connect";
  args.ensure_count(1, 2, function.c_str());

  try {
    // Retrieves the connection data, whatever the source is
    load_connection_data(args);

    // Performs the connection
    _conn.reset(new Connection(_host, _port, _sock, _user, _password, _schema, _ssl_ca, _ssl_cert, _ssl_key));

    _default_schema = _retrieve_current_schema();
  }
  CATCH_AND_TRANSLATE();

  return Value::Null();
}

// Documentation of close function
REGISTER_HELP(CLASSICSESSION_CLOSE_BRIEF, "Closes the internal connection to the MySQL Server held on this session object.");

/**
* $(CLASSICSESSION_CLOSE_BRIEF)
*/
#if DOXYGEN_JS
Undefined ClassicSession::close() {}
#elif DOXYGEN_PY
None ClassicSession::close() {}
#endif
Value ClassicSession::close(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("close").c_str());

  // Connection must be explicitly closed, we can't rely on the
  // automatic destruction because if shared across different objects
  // it may remain open
  if (_conn)
    _conn->close();

  _conn.reset();

  ShellNotifications::get()->notify("SN_SESSION_CLOSED", shared_from_this());

  return shcore::Value();
}

//Documentation of runSql function
REGISTER_HELP(CLASSICSESSION_RUNSQL_BRIEF, "Executes a query against the database and returns a  ClassicResult object wrapping the result.");
REGISTER_HELP(CLASSICSESSION_RUNSQL_PARAM, "@param query the SQL query to execute against the database.");
REGISTER_HELP(CLASSICSESSION_RUNSQL_RETURN, "@return A ClassicResult object.");
REGISTER_HELP(CLASSICSESSION_RUNSQL_EXCEPTION, "@exception An exception is thrown if an error occurs on the SQL execution.");

//! $(CLASSICSESSION_RUNSQL_BRIEF)
#if DOXYGEN_CPP
//! \param args should contain the SQL query to execute against the database.
#else
//! $(CLASSICSESSION_RUNSQL_PARAM)
#endif
/**
* $(CLASSICSESSION_RUNSQL_RETURN)
*
* $(CLASSICSESSION_RUNSQL_EXCEPTION)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::runSql(String query) {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::run_sql(str query) {}
#endif
Value ClassicSession::run_sql(const shcore::Argument_list &args) const {
  args.ensure_count(1, get_function_name("runSql").c_str());
  // Will return the result of the SQL execution
  // In case of error will be Undefined
  Value ret_val;
  if (!_conn)
    throw Exception::logic_error("Not connected.");
  else {
    try {
      ret_val = execute_sql(args.string_at(0), shcore::Argument_list());
    } catch (shcore::Exception & e) {
      // Connection lost, sends a notification
      //if (CR_SERVER_GONE_ERROR == e.code() || ER_X_BAD_PIPE == e.code())
      std::shared_ptr<ClassicSession> myself = std::const_pointer_cast<ClassicSession>(shared_from_this());

      if (e.code() == 2006 || e.code() == 5166 || e.code() == 2013)
        ShellNotifications::get()->notify("SN_SESSION_CONNECTION_LOST", std::dynamic_pointer_cast<Cpp_object_bridge>(myself));

      // Rethrows the exception for normal flow
      throw;
    }
  }

  return ret_val;
}

shcore::Value ClassicSession::execute_sql(const std::string& query, const shcore::Argument_list &UNUSED(args)) const {
  Value ret_val;
  if (!_conn)
    throw Exception::logic_error("Not connected.");
  else {
    if (query.empty())
      throw Exception::argument_error("No query specified.");
    else
      ret_val = Value::wrap(new ClassicResult(std::shared_ptr<Result>(_conn->run_sql(query))));
  }
  return ret_val;
}

// We need to hide this from doxygen to avoif warnings
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
std::shared_ptr<ClassicResult> ClassicSession::execute_sql(const std::string& query) const {
  if (!_conn)
    throw Exception::logic_error("Not connected.");
  else {
    if (query.empty())
      throw Exception::argument_error("No query specified.");
    else
      return std::shared_ptr<ClassicResult>(new ClassicResult(std::shared_ptr<Result>(_conn->run_sql(query))));
  }
}
#endif

//Documentation of createSchema function
REGISTER_HELP(CLASSICSESSION_CREATESCHEMA_BRIEF, "Creates a schema on the database and returns the corresponding object.");
REGISTER_HELP(CLASSICSESSION_CREATESCHEMA_PARAM, "@param name A string value indicating the schema name.");
REGISTER_HELP(CLASSICSESSION_CREATESCHEMA_RETURN, "@return The created schema object.");
REGISTER_HELP(CLASSICSESSION_CREATESCHEMA_EXCEPTION, "@exception An exception is thrown if an error occurs creating the Session.");

/**
* $(CLASSICSESSION_CREATESCHEMA_BRIEF)
*
* $(CLASSICSESSION_CREATESCHEMA_PARAM)
*
* $(CLASSICSESSION_CREATESCHEMA_RETURN)
*
* $(CLASSICSESSION_CREATESCHEMA_EXCEPTION)
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::createSchema(String name) {}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::create_schema(str name) {}
#endif
Value ClassicSession::create_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("createSchema").c_str());

  Value ret_val;
  if (!_conn)
    throw Exception::logic_error("Not connected.");
  else {
    // Options are the statement and optionally options to modify
    // How the resultset is created.
    std::string schema = args.string_at(0);

    if (schema.empty())
      throw Exception::argument_error("The schema name can not be empty.");
    else {
      std::string statement = sqlstring("create schema !", 0) << schema;
      ret_val = execute_sql(statement, shcore::Argument_list());

      std::shared_ptr<ClassicSchema> object(new ClassicSchema(shared_from_this(), schema));

      // If reached this point it indicates the schema was created successfully
      ret_val = shcore::Value(std::static_pointer_cast<Object_bridge>(object));
      (*_schemas)[schema] = ret_val;
    }
  }

  return ret_val;
}

// Documentation of getDefaultSchema function
REGISTER_HELP(CLASSICSESSION_GETDEFAULTSCHEMA_BRIEF, "Retrieves the ClassicSchema configured as default for the session.");
REGISTER_HELP(CLASSICSESSION_GETDEFAULTSCHEMA_RETURN, "@return A ClassicSchema object or Null");
REGISTER_HELP(CLASSICSESSION_GETDEFAULTSCHEMA_DETAIL, "If the configured schema is not valid anymore Null wil be returned.");

/**
* $(CLASSICSESSION_GETDEFAULTSCHEMA_BRIEF)
*
* $(CLASSICSESSION_GETDEFAULTSCHEMA_RETURN)
*
* $(CLASSICSESSION_GETDEFAULTSCHEMA_DETAIL)
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::getDefaultSchema() {}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::get_default_schema() {}
#endif

// Documentation of getCurrentSchema function
REGISTER_HELP(CLASSICSESSION_GETCURRENTSCHEMA_BRIEF, "Retrieves the ClassicSchema that is active as current on the session.");
REGISTER_HELP(CLASSICSESSION_GETCURRENTSCHEMA_RETURN, "@return A ClassicSchema object or Null");
REGISTER_HELP(CLASSICSESSION_GETCURRENTSCHEMA_DETAIL, "The current schema is configured either throu setCurrentSchema(String name) or by using the USE statement.");

/**
* $(CLASSICSESSION_GETCURRENTSCHEMA_BRIEF)
*
* $(CLASSICSESSION_GETCURRENTSCHEMA_RETURN)
*
* $(CLASSICSESSION_GETCURRENTSCHEMA_BRIEF)
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::getCurrentSchema() {}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::get_current_schema() {}
#endif

// Documentation of getCurrentSchema function
REGISTER_HELP(CLASSICSESSION_GETURI_BRIEF, "Returns the connection string passed to connect() method.");
REGISTER_HELP(CLASSICSESSION_GETURI_RETURN, "@return A string representation of the connection data in URI format (excluding the password or the database).");

/**
* $(CLASSICSESSION_GETURI_BRIEF)
*
* $(CLASSICSESSION_GETURI_RETURN)
*/
#if DOXYGEN_JS
String ClassicSession::getUri() {}
#elif DOXYGEN_PY
str ClassicSession::get_uri() {}
#endif

Value ClassicSession::get_member(const std::string &prop) const {
  // Retrieves the member first from the parent
  Value ret_val;

  if (prop == "currentSchema") {
    ClassicSession *session = const_cast<ClassicSession *>(this);
    std::string name = session->_retrieve_current_schema();

    if (!name.empty()) {
      shcore::Argument_list args;
      args.push_back(shcore::Value(name));
      ret_val = get_schema(args);
    } else
      ret_val = Value::Null();
  } else
    ret_val = ShellDevelopmentSession::get_member(prop);

  return ret_val;
}

std::string ClassicSession::_retrieve_current_schema() {
  std::string name;

  if (_conn) {
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
REGISTER_HELP(CLASSICSESSION_GETSCHEMA_RETURN, "@return The ClassicSchema object with the given name.");
REGISTER_HELP(CLASSICSESSION_GETSCHEMA_EXCEPTION, "@exception An exception is thrown if the given name is not a valid schema on the Session.");

//! $(CLASSICSESSION_GETSCHEMA_BRIEF)
#if DOXYGEN_CPP
//! \param args should contain the name of the ClassicSchema object to be retrieved.
#else
//! $(CLASSICSESSION_GETSCHEMA_PARAM)
#endif
/**
* $(CLASSICSESSION_GETSCHEMA_RETURN)
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
shcore::Value ClassicSession::get_schema(const shcore::Argument_list &args) const {
  args.ensure_count(1, get_function_name("getSchema").c_str());
  shcore::Value ret_val;

  try {
    std::string type = "Schema";
    std::string search_name = args.string_at(0);
    std::string name = db_object_exists(type, search_name, "");

    if (!name.empty()) {
      update_schema_cache(name, true);

      ret_val = (*_schemas)[name];

      ret_val.as_object<ClassicSchema>()->update_cache();
    } else {
      update_schema_cache(search_name, false);

      throw Exception::runtime_error("Unknown database '" + search_name + "'");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getSchema"));

  return ret_val;
}

// Documentation of getSchemas function
REGISTER_HELP(CLASSICSESSION_GETSCHEMAS_BRIEF, "Retrieves the Schemas available on the session.");
REGISTER_HELP(CLASSICSESSION_GETSCHEMAS_RETURN, "@return A List containing the ClassicSchema objects available on the session.");

/**
* $(CLASSICSESSION_GETSCHEMAS_BRIEF)
*
* $(CLASSICSESSION_GETSCHEMAS_RETURN)
*/
#if DOXYGEN_JS
List ClassicSession::getSchemas() {}
#elif DOXYGEN_PY
list ClassicSession::get_schemas() {}
#endif
shcore::Value ClassicSession::get_schemas(const shcore::Argument_list &args) const {
  shcore::Value::Array_type_ref schemas(new shcore::Value::Array_type);

  if (_conn) {
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
REGISTER_HELP(CLASSICSESSION_SETCURRENTSCHEMA_RETURN, "@return The new schema.");

/**
* $(CLASSICSESSION_SETCURRENTSCHEMA_BRIEF)
*
* $(CLASSICSESSION_SETCURRENTSCHEMA_RETURN)
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::setCurrentSchema(String schema) {}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::set_current_schema(str schema) {}
#endif
shcore::Value ClassicSession::set_current_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("setCurrentSchema").c_str());

  if (_conn) {
    std::string name = args[0].as_string();

    shcore::Argument_list query;
    query.push_back(Value(sqlstring("use !", 0) << name));

    Value res = run_sql(query);
  } else
    throw Exception::runtime_error("ClassicSession not connected");

  return get_member("currentSchema");
}

std::shared_ptr<shcore::Object_bridge> ClassicSession::create(const shcore::Argument_list &args) {
  return connect_session(args, mysqlsh::SessionType::Classic);
}

// Documentation of dropSchema function
REGISTER_HELP(CLASSICSESSION_DROPSCHEMA_BRIEF, "Drops the schema with the specified name.");
REGISTER_HELP(CLASSICSESSION_DROPSCHEMA_RETURN, "@return A ClassicResult object if succeeded.");
REGISTER_HELP(CLASSICSESSION_DROPSCHEMA_EXCEPTION, "@exception An error is raised if the schema did not exist.");

/**
* $(CLASSICSESSION_DROPSCHEMA_BRIEF)
*
* $(CLASSICSESSION_DROPSCHEMA_RETURN)
*
* $(CLASSICSESSION_DROPSCHEMA_EXCEPTION)
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::dropSchema(String name) {}
#elif DOXYGEN_PY
ClassicResult ClassicSession::drop_schema(str name) {}
#endif
shcore::Value ClassicSession::drop_schema(const shcore::Argument_list &args) {
  std::string function = get_function_name("dropSchema");

  args.ensure_count(1, function.c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #1 is expected to be a string");

  std::string name = args[0].as_string();

  Value ret_val = execute_sql(sqlstring("drop schema !", 0) << name, shcore::Argument_list());

  _remove_schema(name);

  return ret_val;
}

// Documentation of dropTable function
REGISTER_HELP(CLASSICSESSION_DROPTABLE_BRIEF, "Drops a table from the specified schema.");
REGISTER_HELP(CLASSICSESSION_DROPTABLE_RETURN, "@return A ClassicResult object if succeeded.");
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
* $(CLASSICSESSION_DROPTABLE_RETURN)
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
REGISTER_HELP(CLASSICSESSION_DROPVIEW_RETURN, "@return A ClassicResult object if succeeded.");
REGISTER_HELP(CLASSICSESSION_DROPVIEW_EXCEPTION, "@exception An error is raised if the view did not exist.");

/**
* $(CLASSICSESSION_DROPVIEW_BRIEF)
*
* $(CLASSICSESSION_DROPVIEW_RETURN)
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

  Value ret_val = execute_sql(statement, shcore::Argument_list());

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

std::string ClassicSession::db_object_exists(std::string &type, const std::string &name, const std::string& owner) const {
  std::string statement;
  std::string ret_val;

  if (type == "Schema") {
    statement = sqlstring("show databases like ?", 0) << name;
    auto val_result = execute_sql(statement, shcore::Argument_list());
    auto result = val_result.as_object<ClassicResult>();
    auto val_row = result->fetch_one(shcore::Argument_list());

    if (val_row) {
      auto row = val_row.as_object<mysqlsh::Row>();
      if (row)
        ret_val = row->get_member(0).as_string();
    }
  } else {
    statement = sqlstring("show full tables from ! like ?", 0) << owner << name;
    auto val_result = execute_sql(statement, shcore::Argument_list());
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
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_RETURN, "@return A ClassicResult object.");
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_DETAIL, "Calling this function will turn off the autocommit mode on the server.");
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_DETAIL1, "All the operations executed after calling this function will take place only when commit() is called.");
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_DETAIL2, "All the operations executed after calling this function, will be discarded is rollback() is called.");
REGISTER_HELP(CLASSICSESSION_STARTTRANSACTION_DETAIL3, "When commit() or rollback() are called, the server autocommit mode will return back to it's state before calling startTransaction().");

/**
* $(CLASSICSESSION_STARTTRANSACTION_BRIEF)
*
* $(CLASSICSESSION_STARTTRANSACTION_RETURN)
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
shcore::Value ClassicSession::startTransaction(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("startTransaction").c_str());

  return execute_sql("start transaction", shcore::Argument_list());
}

// Documentation of commit function
REGISTER_HELP(CLASSICSESSION_COMMIT_BRIEF, "Commits all the operations executed after a call to startTransaction().");
REGISTER_HELP(CLASSICSESSION_COMMIT_RETURN, "@return A ClassicResult object.");
REGISTER_HELP(CLASSICSESSION_COMMIT_DETAIL, "All the operations executed after calling startTransaction() will take place when this function is called.");
REGISTER_HELP(CLASSICSESSION_COMMIT_DETAIL1, "The server autocommit mode will return back to it's state before calling startTransaction().");

/**
* $(CLASSICSESSION_COMMIT_BRIEF)
*
* $(CLASSICSESSION_COMMIT_RETURN)
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
shcore::Value ClassicSession::commit(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("commit").c_str());

  return execute_sql("commit", shcore::Argument_list());
}

// Documentation of rollback function
REGISTER_HELP(CLASSICSESSION_ROLLBACK_BRIEF, "Discards all the operations executed after a call to startTransaction().");
REGISTER_HELP(CLASSICSESSION_ROLLBACK_RETURN, "@return A ClassicResult object.");
REGISTER_HELP(CLASSICSESSION_ROLLBACK_DETAIL, "All the operations executed after calling startTransaction() will be discarded when this function is called.");
REGISTER_HELP(CLASSICSESSION_ROLLBACK_DETAIL1, "The server autocommit mode will return back to it's state before calling startTransaction().");

/**
* $(CLASSICSESSION_ROLLBACK_BRIEF)
*
* $(CLASSICSESSION_ROLLBACK_RETURN)
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
shcore::Value ClassicSession::rollback(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("rollback").c_str());

  return execute_sql("rollback", shcore::Argument_list());
}

shcore::Value ClassicSession::get_status(const shcore::Argument_list &args) {
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  try {
    auto val_result = execute_sql("select DATABASE(), USER() limit 1", shcore::Argument_list());
    auto result = val_result.as_object<ClassicResult>();
    auto val_row = result->fetch_one(shcore::Argument_list());
    if (val_row) {
      auto row = val_row.as_object<mysqlsh::Row>();

      if (row) {
        (*status)["SESSION_TYPE"] = shcore::Value("Classic");
        (*status)["DEFAULT_SCHEMA"] = shcore::Value(_default_schema);

        std::string current_schema = row->get_member(0).descr(true);
        if (current_schema == "null")
          current_schema = "";

        (*status)["CURRENT_SCHEMA"] = shcore::Value(current_schema);
        (*status)["CURRENT_USER"] = row->get_member(1);
        (*status)["CONNECTION_ID"] = shcore::Value(uint64_t(_conn->get_thread_id()));
        (*status)["SSL_CIPHER"] = shcore::Value(_conn->get_ssl_cipher());
        //(*status)["SKIP_UPDATES"] = shcore::Value(???);
        //(*status)["DELIMITER"] = shcore::Value(???);

        (*status)["SERVER_INFO"] = shcore::Value(_conn->get_server_info());

        (*status)["PROTOCOL_VERSION"] = shcore::Value(uint64_t(_conn->get_protocol_info()));
        (*status)["CONNECTION"] = shcore::Value(_conn->get_connection_info());
        //(*status)["INSERT_ID"] = shcore::Value(???);
      }
    }

    val_result = execute_sql("select @@character_set_client, @@character_set_connection, @@character_set_server, @@character_set_database, @@version_comment limit 1", shcore::Argument_list());
    result = val_result.as_object<ClassicResult>();
    val_row = result->fetch_one(shcore::Argument_list());

    if (val_row) {
      auto row = val_row.as_object<mysqlsh::Row>();

      if (row) {
        (*status)["CLIENT_CHARSET"] = row->get_member(0);
        (*status)["CONNECTION_CHARSET"] = row->get_member(1);
        (*status)["SERVER_CHARSET"] = row->get_member(2);
        (*status)["SCHEMA_CHARSET"] = row->get_member(3);
        (*status)["SERVER_VERSION"] = row->get_member(4);

        (*status)["SERVER_STATS"] = shcore::Value(_conn->get_stats());

        // TODO: Review retrieval from charset_info, mysql connection

        // TODO: Embedded library stuff
        //(*status)["TCP_PORT"] = row->get_value(1);
        //(*status)["UNIX_SOCKET"] = row->get_value(2);
        //(*status)["PROTOCOL_COMPRESSED"] = row->get_value(3);

        // STATUS

        // SAFE UPDATES
      }
    }
  } catch (shcore::Exception& e) {
    (*status)["STATUS_ERROR"] = shcore::Value(e.format());
  }

  return shcore::Value(status);
}
