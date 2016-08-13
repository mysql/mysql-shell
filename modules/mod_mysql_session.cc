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
#include "shellcore/server_registry.h"

#include "shellcore/proxy_object.h"
#include "mysqlxtest_utils.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <set>

#include "mysql_connection.h"
#include "mod_mysql_resultset.h"
#include "mod_mysql_schema.h"
#include "utils/utils_general.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::mysql;
using namespace shcore;

REGISTER_OBJECT(mysql, ClassicSession);

ClassicSession::ClassicSession()
{
  init();
}

ClassicSession::ClassicSession(const ClassicSession& session) :
ShellDevelopmentSession(session), _conn(session._conn)
{
  init();
}

void ClassicSession::init()
{
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
  auto generator = [this](const std::string& name){return shcore::Value::wrap<ClassicSchema>(new ClassicSchema(shared_from_this(), name)); };
  update_schema_cache = [generator, this](const std::string &name, bool exists){DatabaseObject::update_cache(name, generator, exists, _schemas); };
}

Connection *ClassicSession::connection()
{
  return _conn.get();
}

Value ClassicSession::connect(const Argument_list &args)
{
  std::string function = class_name() + '.' + "connect";
  args.ensure_count(1, 2, function.c_str());

  try
  {
    // Retrieves the connection data, whatever the source is
    load_connection_data(args);

    // Performs the connection
    _conn.reset(new Connection(_host, _port, _sock, _user, _password, _schema, _ssl_ca, _ssl_cert, _ssl_key));

    _default_schema = _retrieve_current_schema();
  }
  CATCH_AND_TRANSLATE();

  return Value::Null();
}

/**
* Closes the internal connection to the MySQL Server held on this session object.
*/
#if DOXYGEN_JS
Undefined ClassicSession::close(){}
#elif DOXYGEN_PY
None ClassicSession::close(){}
#endif
Value ClassicSession::close(const shcore::Argument_list &args)
{
  args.ensure_count(0, get_function_name("close").c_str());

  // Connection must be explicitly closed, we can't rely on the
  // automatic destruction because if shared across different objects
  // it may remain open
  if (_conn)
    _conn->close();

  _conn.reset();

  return shcore::Value();
}

//! Executes a query against the database and returns a  ClassicResult object wrapping the result.
#if DOXYGEN_CPP
//! \param args should contain the SQL query to execute against the database.
#else
//! \param query the SQL query to execute against the database.
#endif
/**
* \return A ClassicResult object.
* \exception An exception is thrown if an error occurs on the SQL execution.
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::runSql(String query){}
#elif DOXYGEN_PY
ClassicResult ClassicSession::run_sql(str query){}
#endif
Value ClassicSession::run_sql(const shcore::Argument_list &args) const
{
  args.ensure_count(1, get_function_name("runSql").c_str());
  // Will return the result of the SQL execution
  // In case of error will be Undefined
  Value ret_val;
  if (!_conn)
    throw Exception::logic_error("Not connected.");
  else
  {
    // Options are the statement and optionally options to modify
    // How the resultset is created.
    std::string statement = args.string_at(0);

    if (statement.empty())
      throw Exception::argument_error("No query specified.");
    else
      ret_val = Value::wrap(new ClassicResult(std::shared_ptr<Result>(_conn->run_sql(statement))));
  }

  return ret_val;
}

//! Creates a schema on the database and returns the corresponding object.
#if DOXYGEN_CPP
//! \param args should contain a string value indicating the schema name.
#else
//! \param name A string value indicating the schema name..
#endif
/**
* \return The created schema object.
* \exception An exception is thrown if an error occurs creating the Session.
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::createSchema(String name){}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::create_schema(str name){}
#endif
Value ClassicSession::create_schema(const shcore::Argument_list &args)
{
  args.ensure_count(1, get_function_name("createSchema").c_str());

  Value ret_val;
  if (!_conn)
    throw Exception::logic_error("Not connected.");
  else
  {
    // Options are the statement and optionally options to modify
    // How the resultset is created.
    std::string schema = args.string_at(0);

    if (schema.empty())
      throw Exception::argument_error("The schema name can not be empty.");
    else
    {
      std::string statement = sqlstring("create schema !", 0) << schema;
      ret_val = Value::wrap(new ClassicResult(std::shared_ptr<Result>(_conn->run_sql(statement))));

      std::shared_ptr<ClassicSchema> object(new ClassicSchema(shared_from_this(), schema));

      // If reached this point it indicates the schema was created successfully
      ret_val = shcore::Value(std::static_pointer_cast<Object_bridge>(object));
      (*_schemas)[schema] = ret_val;
    }
  }

  return ret_val;
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function. The content of the returned value depends on the property being requested. The next list shows the valid properties as well as the returned value for each of them:
 *
 * \li currentSchema: returns ClassicSchema object representing the active schema on the session. If none is active, returns Null.
 */
#else
/**
* Retrieves the ClassicSchema configured as default for the session.
* \return A ClassicSchema object or Null
*
* If the configured schema is not valid anymore Null wil be returned.
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::getDefaultSchema(){}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::get_default_schema(){}
#endif

/**
* Retrieves the ClassicSchema that is active as current on the session.
* \return A ClassicSchema object or Null
*
* The current schema is configured either throu setCurrentSchema(String name) or by using the USE statement.
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::getCurrentSchema(){}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::get_current_schema(){}
#endif

/**
* Returns the connection string passed to connect() method.
* \return A string representation of the connection data in URI format (excluding the password or the database).
*/
#if DOXYGEN_JS
String ClassicSession::getUri(){}
#elif DOXYGEN_PY
str ClassicSession::get_uri(){}
#endif
#endif
Value ClassicSession::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  if (prop == "currentSchema")
  {
    ClassicSession *session = const_cast<ClassicSession *>(this);
    std::string name = session->_retrieve_current_schema();

    if (!name.empty())
    {
      shcore::Argument_list args;
      args.push_back(shcore::Value(name));
      ret_val = get_schema(args);
    }
    else
      ret_val = Value::Null();
  }
  else
    ret_val = ShellDevelopmentSession::get_member(prop);

  return ret_val;
}

std::string ClassicSession::_retrieve_current_schema()
{
  std::string name;

  if (_conn)
  {
    shcore::Argument_list query;
    query.push_back(Value("select schema()"));

    Value res = run_sql(query);

    std::shared_ptr<ClassicResult> rset = res.as_object<ClassicResult>();
    Value next_row = rset->fetch_one(shcore::Argument_list());

    if (next_row)
    {
      std::shared_ptr<mysh::Row> row = next_row.as_object<mysh::Row>();
      shcore::Value schema = row->get_member("schema()");

      if (schema)
        name = schema.as_string();
    }
  }

  return name;
}

void ClassicSession::_remove_schema(const std::string& name)
{
  if (_schemas->find(name) != _schemas->end())
    _schemas->erase(name);
}

//! Retrieves a ClassicSchema object from the current session through it's name.
#if DOXYGEN_CPP
//! \param args should contain the name of the ClassicSchema object to be retrieved.
#else
//! \param name The name of the ClassicSchema object to be retrieved.
#endif
/**
* \return The ClassicSchema object with the given name.
* \exception An exception is thrown if the given name is not a valid schema on the Session.
* \sa ClassicSchema
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::getSchema(String name){}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::get_schema(str name){}
#endif
shcore::Value ClassicSession::get_schema(const shcore::Argument_list &args) const
{
  args.ensure_count(1, get_function_name("getSchema").c_str());
  shcore::Value ret_val;

  std::string type = "Schema";
  std::string search_name = args.string_at(0);
  std::string name = db_object_exists(type, search_name, "");

  if (!name.empty())
  {
    update_schema_cache(name, true);

    ret_val = (*_schemas)[name];

    ret_val.as_object<ClassicSchema>()->update_cache();
  }
  else
  {
    update_schema_cache(search_name, false);

    throw Exception::runtime_error("Unknown database '" + search_name + "'");
  }

  return ret_val;
}

/**
* Retrieves the Schemas available on the session.
* \return A List containing the ClassicSchema objects available o the session.
*/
#if DOXYGEN_JS
List ClassicSession::getSchemas(){}
#elif DOXYGEN_PY
list ClassicSession::get_schemas(){}
#endif
shcore::Value ClassicSession::get_schemas(const shcore::Argument_list &args) const
{
  shcore::Value::Array_type_ref schemas(new shcore::Value::Array_type);

  if (_conn)
  {
    shcore::Argument_list query;
    query.push_back(Value("show databases;"));

    Value res = run_sql(query);

    shcore::Argument_list args;
    std::shared_ptr<ClassicResult> rset = res.as_object<ClassicResult>();
    Value next_row = rset->fetch_one(args);
    std::shared_ptr<mysh::Row> row;

    while (next_row)
    {
      row = next_row.as_object<mysh::Row>();
      shcore::Value schema = row->get_member("Database");
      if (schema)
      {
        update_schema_cache(schema.as_string(), true);

        schemas->push_back((*_schemas)[schema.as_string()]);
      }

      next_row = rset->fetch_one(args);
    }
  }

  return shcore::Value(schemas);
}

/**
* Sets the selected schema for this session's connection.
* \return The new schema.
*/
#if DOXYGEN_JS
ClassicSchema ClassicSession::setCurrentSchema(String schema){}
#elif DOXYGEN_PY
ClassicSchema ClassicSession::set_current_schema(str schema){}
#endif
shcore::Value ClassicSession::set_current_schema(const shcore::Argument_list &args)
{
  args.ensure_count(1, get_function_name("setCurrentSchema").c_str());

  if (_conn)
  {
    std::string name = args[0].as_string();

    shcore::Argument_list query;
    query.push_back(Value(sqlstring("use !", 0) << name));

    Value res = run_sql(query);
  }
  else
    throw Exception::runtime_error("ClassicSession not connected");

  return get_member("currentSchema");
}

std::shared_ptr<shcore::Object_bridge> ClassicSession::create(const shcore::Argument_list &args)
{
  return connect_session(args, mysh::Classic);
}

/**
* Drops the schema with the specified name.
* \return A ClassicResult object if succeeded.
* \exception An error is raised if the schema did not exist.
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::dropSchema(String name){}
#elif DOXYGEN_PY
ClassicResult ClassicSession::drop_schema(str name){}
#endif
shcore::Value ClassicSession::drop_schema(const shcore::Argument_list &args)
{
  std::string function = get_function_name("dropSchema");

  args.ensure_count(1, function.c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #1 is expected to be a string");

  std::string name = args[0].as_string();

  Value ret_val = Value::wrap(new ClassicResult(std::shared_ptr<Result>(_conn->run_sql(sqlstring("drop schema !", 0) << name))));

  _remove_schema(name);

  return ret_val;
}

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
* Drops a table from the specified schema.
* \return A ClassicResult object if succeeded.
* \exception An error is raised if the table did not exist.
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::dropTable(String schema, String name){}
#elif DOXYGEN_PY
ClassicResult ClassicSession::drop_table(str schema, str name){}
#endif

/**
* Drops a view from the specified schema.
* \return A ClassicResult object if succeeded.
* \exception An error is raised if the view did not exist.
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::dropView(String schema, String name){}
#elif DOXYGEN_PY
ClassicResult ClassicSession::drop_view(str schema, str name){}
#endif
#endif
shcore::Value ClassicSession::drop_schema_object(const shcore::Argument_list &args, const std::string& type)
{
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

  Value ret_val = Value::wrap(new ClassicResult(std::shared_ptr<Result>(_conn->run_sql(statement))));

  if (_schemas->count(schema))
  {
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

std::string ClassicSession::db_object_exists(std::string &type, const std::string &name, const std::string& owner) const
{
  std::string statement;
  std::string ret_val;

  if (type == "Schema")
  {
    statement = sqlstring("show databases like ?", 0) << name;
    Result *res = _conn->run_sql(statement);
    if (res->has_resultset())
    {
      Row *row = res->fetch_one();
      if (row)
        ret_val = row->get_value(0).as_string();
    }
  }
  else
  {
    statement = sqlstring("show full tables from ! like ?", 0) << owner << name;
    Result *res = _conn->run_sql(statement);

    if (res->has_resultset())
    {
      Row *row = res->fetch_one();

      if (row)
      {
        std::string db_type = row->get_value(1).as_string();

        if (type == "Table" && (db_type == "BASE TABLE" || db_type == "LOCAL TEMPORARY"))
          ret_val = row->get_value(0).as_string();
        else if (type == "View" && (db_type == "VIEW" || db_type == "SYSTEM VIEW"))
          ret_val = row->get_value(0).as_string();
        else if (type.empty())
        {
          ret_val = row->get_value(0).as_string();
          type = db_type;
        }
      }
    }
  }

  return ret_val;
}

/**
* Starts a transaction context on the server.
* \return A ClassicResult object.
* Calling this function will turn off the autocommit mode on the server.
*
* All the operations executed after calling this function will take place only when commit() is called.
*
* All the operations executed after calling this function, will be discarded is rollback() is called.
*
* When commit() or rollback() are called, the server autocommit mode will return back to it's state before calling startTransaction().
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::startTransaction(){}
#elif DOXYGEN_PY
ClassicResult ClassicSession::start_transaction(){}
#endif
shcore::Value ClassicSession::startTransaction(const shcore::Argument_list &args)
{
  args.ensure_count(0, get_function_name("startTransaction").c_str());

  return Value::wrap(new ClassicResult(std::shared_ptr<Result>(_conn->run_sql("start transaction"))));
}

/**
* Commits all the operations executed after a call to startTransaction().
* \return A ClassicResult object.
*
* All the operations executed after calling startTransaction() will take place when this function is called.
*
* The server autocommit mode will return back to it's state before calling startTransaction().
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::commit(){}
#elif DOXYGEN_PY
ClassicResult ClassicSession::commit(){}
#endif
shcore::Value ClassicSession::commit(const shcore::Argument_list &args)
{
  args.ensure_count(0, get_function_name("commit").c_str());

  return Value::wrap(new ClassicResult(std::shared_ptr<Result>(_conn->run_sql("commit"))));
}

/**
* Discards all the operations executed after a call to startTransaction().
* \return A ClassicResult object.
*
* All the operations executed after calling startTransaction() will be discarded when this function is called.
*
* The server autocommit mode will return back to it's state before calling startTransaction().
*/
#if DOXYGEN_JS
ClassicResult ClassicSession::rollback(){}
#elif DOXYGEN_PY
ClassicResult ClassicSession::rollback(){}
#endif
shcore::Value ClassicSession::rollback(const shcore::Argument_list &args)
{
  args.ensure_count(0, get_function_name("rollback").c_str());

  return Value::wrap(new ClassicResult(std::shared_ptr<Result>(_conn->run_sql("rollback"))));
}

shcore::Value ClassicSession::get_status(const shcore::Argument_list &args)
{
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  Result *result;
  Row *row;

  try
  {
    result = _conn->run_sql("select DATABASE(), USER() limit 1");
    row = result->fetch_one();

    (*status)["SESSION_TYPE"] = shcore::Value("Classic");
    (*status)["DEFAULT_SCHEMA"] = shcore::Value(_default_schema);

    std::string current_schema = row->get_value(0).descr(true);
    if (current_schema == "null")
      current_schema = "";

    (*status)["CURRENT_SCHEMA"] = shcore::Value(current_schema);
    (*status)["CURRENT_USER"] = row->get_value(1);
    (*status)["CONNECTION_ID"] = shcore::Value(uint64_t(_conn->get_thread_id()));
    (*status)["SSL_CIPHER"] = shcore::Value(_conn->get_ssl_cipher());
    //(*status)["SKIP_UPDATES"] = shcore::Value(???);
    //(*status)["DELIMITER"] = shcore::Value(???);

    (*status)["SERVER_INFO"] = shcore::Value(_conn->get_server_info());

    (*status)["PROTOCOL_VERSION"] = shcore::Value(uint64_t(_conn->get_protocol_info()));
    (*status)["CONNECTION"] = shcore::Value(_conn->get_connection_info());
    //(*status)["INSERT_ID"] = shcore::Value(???);

    result = _conn->run_sql("select @@character_set_client, @@character_set_connection, @@character_set_server, @@character_set_database, @@version_comment limit 1");
    row = result->fetch_one();
    (*status)["CLIENT_CHARSET"] = row->get_value(0);
    (*status)["CONNECTION_CHARSET"] = row->get_value(1);
    (*status)["SERVER_CHARSET"] = row->get_value(2);
    (*status)["SCHEMA_CHARSET"] = row->get_value(3);
    (*status)["SERVER_VERSION"] = row->get_value(4);

    (*status)["SERVER_STATS"] = shcore::Value(_conn->get_stats());

    // TODO: Review retrieval from charset_info, mysql connection

    // TODO: Embedded library stuff
    //(*status)["TCP_PORT"] = row->get_value(1);
    //(*status)["UNIX_SOCKET"] = row->get_value(2);
    //(*status)["PROTOCOL_COMPRESSED"] = row->get_value(3);

    // STATUS

    // SAFE UPDATES
  }
  catch(shcore::Exception& e)
  {
    (*status)["STATUS_ERROR"] = shcore::Value(e.format());
  }

  return shcore::Value(status);
}