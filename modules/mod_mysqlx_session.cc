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

#include "utils/utils_sqlstring.h"
#include "mod_mysqlx_session.h"
#include "mod_mysqlx_schema.h"
#include "mod_mysqlx_resultset.h"
#include "mod_mysqlx_expression.h"
#include "mod_mysqlx_constants.h"
#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "mod_mysqlx_session_sql.h"
#include "shellcore/server_registry.h"
#include "utils/utils_general.h"
#include "utils/utils_time.h"
#include "utils/utils_file.h"
#include "shellcore/proxy_object.h"

#include "mysqlxtest_utils.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "mysqlx_connection.h"

#include <stdlib.h>
#include <string.h>

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define mystrdup _strdup
#else
#define mystrdup strdup
#endif

using namespace mysh;
using namespace shcore;
using namespace mysh::mysqlx;

REGISTER_OBJECT(mysqlx, XSession);
REGISTER_OBJECT(mysqlx, NodeSession);
REGISTER_OBJECT(mysqlx, Expression);
REGISTER_OBJECT(mysqlx, Type);
REGISTER_OBJECT(mysqlx, IndexType);

#include <set>

#ifdef WIN32
#define strcasecmp _stricmp
#endif

BaseSession::BaseSession()
  : _case_sensitive_table_names(false)
{
  _schemas.reset(new shcore::Value::Map_type);

  add_method("close", boost::bind(&BaseSession::close, this, _1), "data");
  add_method("setFetchWarnings", boost::bind(&BaseSession::set_fetch_warnings, this, _1), "data");
  add_method("startTransaction", boost::bind(&BaseSession::startTransaction, this, _1), "data");
  add_method("commit", boost::bind(&BaseSession::commit, this, _1), "data");
  add_method("rollback", boost::bind(&BaseSession::rollback, this, _1), "data");

  add_method("dropSchema", boost::bind(&BaseSession::drop_schema, this, _1), "data");
  add_method("dropTable", boost::bind(&BaseSession::drop_schema_object, this, _1, "Table"), "data");
  add_method("dropCollection", boost::bind(&BaseSession::drop_schema_object, this, _1, "Collection"), "data");
  add_method("dropView", boost::bind(&BaseSession::drop_schema_object, this, _1, "View"), "data");
}

Value BaseSession::connect(const Argument_list &args)
{
  std::string function_name = class_name() + ".connect";
  args.ensure_count(1, 2, function_name.c_str());

  try
  {
    // Retrieves the connection data, whatever the source is
    load_connection_data(args);

    ::mysqlx::Ssl_config ssl;
    memset(&ssl, 0, sizeof(ssl));

    ssl.ca = _ssl_ca.c_str();
    ssl.cert = _ssl_cert.c_str();
    ssl.key = _ssl_key.c_str();

    // TODO: Define a proper timeout for the session creation
    _session = ::mysqlx::openSession(_host, _port, _schema, _user, _password, ssl, 10000, _auth_method, true);

    _load_schemas();

    int case_sesitive_table_names = 0;
    _retrieve_session_info(_default_schema, case_sesitive_table_names);

    _case_sensitive_table_names = (case_sesitive_table_names == 0);
  }
  CATCH_AND_TRANSLATE();

  return Value::Null();
}

void BaseSession::set_option(const char *option, int value)
{
  if (strcmp(option, "trace_protocol") == 0 && _session)
    _session->connection()->set_trace_protocol(value != 0);
  else
    throw shcore::Exception::argument_error(std::string("Unknown option ").append(option));
}

bool BaseSession::table_name_compare(const std::string &n1, const std::string &n2)
{
  if (_case_sensitive_table_names)
    return n1 == n2;
  else
    return strcasecmp(n1.c_str(), n2.c_str()) == 0;
}

#ifdef DOXYGEN
/**
* \brief Closes the session.
* After closing the session it is still possible to make read only operation to gather metadata info, like getTable(name) or getSchemas().
*/
Undefined BaseSession::close(){}
#endif
Value BaseSession::close(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".close";

  args.ensure_count(0, function_name.c_str());

  // Connection must be explicitly closed, we can't rely on the
  // automatic destruction because if shared across different objects
  // it may remain open

  if (_session)
  {
    _session->close();

    _session.reset();
  }

  return shcore::Value();
}

Value BaseSession::sql(const Argument_list &args)
{
  std::string function_name = class_name() + ".sql";
  args.ensure_count(1, function_name.c_str());

  // NOTE: This function is no longer exposed as part of the API but it kept
  //       since it is used internally.
  //       until further cleanup is done let it live here.
  Value ret_val;
  try
  {
    ret_val = execute_sql(args.string_at(0), shcore::Argument_list());
  }
  CATCH_AND_TRANSLATE();

  return ret_val;
}

#ifdef DOXYGEN
/**
* Creates a schema on the database and returns the corresponding object.
* \param name A string value indicating the schema name.
* \return The created schema object.
* \exception An exception is thrown if an error occurs creating the XSession.
*/
Schema BaseSession::createSchema(String name){}
#endif
Value BaseSession::create_schema(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".createSchema";
  args.ensure_count(1, function_name.c_str());

  Value ret_val;
  try
  {
    std::string schema = args.string_at(0);
    std::string statement = sqlstring("create schema !", 0) << schema;
    ret_val = executeStmt("sql", statement, false, shcore::Argument_list());

    // if reached this point it indicates that there were no errors
    boost::shared_ptr<Schema> object(new Schema(_get_shared_this(), schema));
    ret_val = shcore::Value(boost::static_pointer_cast<Object_bridge>(object));

    (*_schemas)[schema] = ret_val;
  }
  CATCH_AND_TRANSLATE();

  return ret_val;
}

#ifdef DOXYGEN
/**
* Starts a transaction context on the server.
* \return A SqlResult object.
* Calling this function will turn off the autocommit mode on the server.
*
* All the operations executed after calling this function will take place only when commit() is called.
*
* All the operations executed after calling this function, will be discarded is rollback() is called.
*
* When commit() or rollback() are called, the server autocommit mode will return back to it's state before calling startTransaction().
*/
Result BaseSession::startTransaction(){}
#endif
shcore::Value BaseSession::startTransaction(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".startTransaction";
  args.ensure_count(0, function_name.c_str());

  return executeStmt("sql", "start transaction", false, shcore::Argument_list());
}

#ifdef DOXYGEN
/**
* Commits all the operations executed after a call to startTransaction().
* \return A SqlResult object.
*
* All the operations executed after calling startTransaction() will take place when this function is called.
*
* The server autocommit mode will return back to it's state before calling startTransaction().
*/
Result BaseSession::commit(){}
#endif
shcore::Value BaseSession::commit(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".startTransaction";
  args.ensure_count(0, function_name.c_str());

  return executeStmt("sql", "commit", false, shcore::Argument_list());
}

#ifdef DOXYGEN
/**
* Discards all the operations executed after a call to startTransaction().
* \return A SqlResult object.
*
* All the operations executed after calling startTransaction() will be discarded when this function is called.
*
* The server autocommit mode will return back to it's state before calling startTransaction().
*/
Result BaseSession::rollback(){}
#endif
shcore::Value BaseSession::rollback(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".startTransaction";
  args.ensure_count(0, function_name.c_str());

  return executeStmt("sql", "rollback", false, shcore::Argument_list());
}

::mysqlx::ArgumentValue BaseSession::get_argument_value(shcore::Value source)
{
  ::mysqlx::ArgumentValue ret_val;
  switch (source.type)
  {
    case shcore::Bool:
      ret_val = ::mysqlx::ArgumentValue(source.as_bool());
      break;
    case shcore::UInteger:
      ret_val = ::mysqlx::ArgumentValue(source.as_uint());
      break;
    case shcore::Integer:
      ret_val = ::mysqlx::ArgumentValue(source.as_int());
      break;
    case shcore::String:
      ret_val = ::mysqlx::ArgumentValue(source.as_string());
      break;
    case shcore::Float:
      ret_val = ::mysqlx::ArgumentValue(source.as_double());
      break;
    case shcore::Object:
    case shcore::Null:
    case shcore::Array:
    case shcore::Map:
    case shcore::MapRef:
    case shcore::Function:
    case shcore::Undefined:
      std::stringstream str;
      str << "Unsupported value received: " << source.descr();
      throw shcore::Exception::argument_error(str.str());
      break;
  }

  return ret_val;
}

Value BaseSession::execute_sql(const std::string& statement, const Argument_list &args)
{
  return executeStmt("sql", statement, true, args);
}

Value BaseSession::executeAdminCommand(const std::string& command, bool expect_data, const Argument_list &args)
{
  std::string function_name = class_name() + ".executeAdminCommand";
  args.ensure_at_least(1, function_name.c_str());

  return executeStmt("xplugin", command, expect_data, args);
}

Value BaseSession::executeStmt(const std::string &domain, const std::string& command, bool expect_data, const Argument_list &args)
{
  // Will return the result of the SQL execution
  // In case of error will be Undefined
  Value ret_val;
  if (!_session)
    throw Exception::logic_error("Not connected.");
  else
  {
    // Converts the arguments from shcore to mysqlxtest format
    std::vector< ::mysqlx::ArgumentValue> arguments;
    for (size_t index = 0; index < args.size(); index++)
      arguments.push_back(get_argument_value(args[index]));

    try
    {
      MySQL_timer timer;

      timer.start();

      _last_result = _session->executeStmt(domain, command, arguments);

      // Calls wait so any error is properly triggered at execution time
      _last_result->wait();

      timer.end();

      if (expect_data)
      {
        SqlResult *result;
        ret_val = shcore::Value::wrap(result = new SqlResult(_last_result));
        result->set_execution_time(timer.raw_duration());
      }
      else
      {
        Result *result;
        ret_val = shcore::Value::wrap(result = new Result(_last_result));
        result->set_execution_time(timer.raw_duration());
      }
    }
    CATCH_AND_TRANSLATE();
  }

  return ret_val;
}

std::vector<std::string> BaseSession::get_members() const
{
  std::vector<std::string> members(ShellBaseSession::get_members());

  // This function is here only to append the schemas as direct members
  // Using a set to prevent duplicates
  std::set<std::string> set(members.begin(), members.end());
  for (Value::Map_type::const_iterator iter = _schemas->begin(); iter != _schemas->end(); ++iter)
  {
    if (shcore::is_valid_identifier(iter->first))
      set.insert(iter->first);
  }

  for (std::vector<std::string>::iterator index = members.begin(); index != members.end(); ++index)
    set.erase(*index);

  for (std::set<std::string>::iterator index = set.begin(); index != set.end(); ++index)
    members.push_back(*index);

  return members;
}

bool BaseSession::has_member(const std::string &prop) const
{
  if (ShellBaseSession::has_member(prop))
    return true;
  if (prop == "currentSchema" || prop == "uri" || prop == "schemas" || prop == "defaultSchema")
    return true;
  if (_schemas->has_key(prop))
    return true;

  return false;
}

#ifdef DOXYGEN
/**
* Retrieves the Schema configured as default for the session.
* \return A Schema object or Null
*/
Schema BaseSession::getDefaultSchema(){}

/**
* Retrieves the Schemas available on the session.
* \return A Map containing the Schema objects available o the session.
*/
Map BaseSession::getSchemas(){}

/**
* Retrieves the connection data for this session in string format.
* \return A string representing the connection data.
*/
String BaseSession::getUri(){}
#endif
Value BaseSession::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is a schema and attempt loading it as such
  if (ShellBaseSession::has_member(prop))
    ret_val = ShellBaseSession::get_member(prop);
  else if (prop == "uri")
    ret_val = Value(_uri);
  else if (prop == "schemas")
    ret_val = Value(_schemas);
  else if (prop == "defaultSchema" || prop == "currentSchema")
  {
    if (!_default_schema.empty())
      ret_val = get_member(_default_schema);
    else
      ret_val = Value::Null();
  }
  else
  {
    if (_schemas->has_key(prop))
    {
      boost::shared_ptr<Schema> schema = (*_schemas)[prop].as_object<Schema>();

      // This will validate the schema continues valid
      schema->cache_table_objects();

      ret_val = (*_schemas)[prop];
    }
  }

  return ret_val;
}

std::string BaseSession::_retrieve_current_schema()
{
  std::string name;
  try
  {
    if (_session)
    {
      // TODO: update this logic properly
      boost::shared_ptr< ::mysqlx::Result> result = _session->executeSql("select schema()");
      boost::shared_ptr< ::mysqlx::Row>row = result->next();

      if (!row->isNullField(0))
        name = row->stringField(0);

      result->flush();
    }
  }
  CATCH_AND_TRANSLATE();

  return name;
}

void BaseSession::_retrieve_session_info(std::string &current_schema,
                                        int &case_sensitive_table_names)
{
  try
  {
    if (_session)
    {
      // TODO: update this logic properly
      boost::shared_ptr< ::mysqlx::Result> result = _session->executeSql("select schema(), @@lower_case_table_names");
      boost::shared_ptr< ::mysqlx::Row>row = result->next();

      if (!row->isNullField(0))
        current_schema = row->stringField(0);
      case_sensitive_table_names = (int)row->uInt64Field(1);

      result->flush();
    }
  }
  CATCH_AND_TRANSLATE();
}

void BaseSession::_load_schemas()
{
  try
  {
    if (_session)
    {
      boost::shared_ptr< ::mysqlx::Result> result = _session->executeSql("show databases;");
      boost::shared_ptr< ::mysqlx::Row> row = result->next();

      while (row)
      {
        std::string schema;
        if (!row->isNullField(0))
          schema = row->stringField(0);

        if (!schema.empty())
        {
          boost::shared_ptr<Schema> object(new Schema(_get_shared_this(), schema));
          (*_schemas)[schema] = shcore::Value(boost::static_pointer_cast<Object_bridge>(object));
        }

        row = result->next();
      }

      result->flush();
    }
  }
  CATCH_AND_TRANSLATE();
}

void BaseSession::_remove_schema(const std::string& name)
{
  if (_schemas->count(name))
    _schemas->erase(name);
}

#ifdef DOXYGEN
/**
* Retrieves a Schema object from the current session through it's name.
* \param name The name of the Schema object to be retrieved.
* \return The Schema object with the given name.
* \exception An exception is thrown if the given name is not a valid schema on the XSession.
* \sa Schema
*/
Schema BaseSession::getSchema(String name){}
#endif

shcore::Value BaseSession::get_schema(const shcore::Argument_list &args) const
{
  std::string function_name = class_name() + ".getSchema";
  args.ensure_count(1, function_name.c_str());

  std::string name = args[0].as_string();

  if (_schemas->has_key(name))
  {
    boost::shared_ptr<Schema> schema = (*_schemas)[name].as_object<Schema>();

    // This will validate the schema continues valid
    schema->cache_table_objects();
  }
  else
  {
    if (_session)
    {
      boost::shared_ptr<Schema> schema(new Schema(_get_shared_this(), name));

      // Here this call will also validate the schema is valid
      schema->cache_table_objects();

      (*_schemas)[name] = Value(boost::static_pointer_cast<Object_bridge>(schema));
    }
    else
      throw Exception::runtime_error("XSession not connected");
  }

  // If this point is reached, the schema will be there!
  return (*_schemas)[name];
}

shcore::Value BaseSession::set_fetch_warnings(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + ".setFetchWarnings").c_str());

  bool enable = args.bool_at(0);
  std::string command = enable ? "enable_notices" : "disable_notices";

  shcore::Argument_list command_args;
  command_args.push_back(Value("warnings"));

  return executeAdminCommand(command, false, command_args);
}

#ifdef DOXYGEN
/**
* Drops the schema with the specified name.
* \return A SqlResult object if succeeded.
* \exception An error is raised if the schema did not exist.
*/
Result BaseSession::dropSchema(String name){}
#endif
shcore::Value BaseSession::drop_schema(const shcore::Argument_list &args)
{
  std::string function = class_name() + ".dropSchema";

  args.ensure_count(1, function.c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #1 is expected to be a string");

  std::string name = args[0].as_string();

  Value ret_val = executeStmt("sql", sqlstring("drop schema !", 0) << name, false, shcore::Argument_list());

  _remove_schema(name);

  return ret_val;
}

#ifdef DOXYGEN
/**
* Drops a table from the specified schema.
* \return A SqlResult object if succeeded.
* \exception An error is raised if the table did not exist.
*/
Result BaseSession::dropTable(String schema, String name){}

/**
* Drops a collection from the specified schema.
* \return A SqlResult object if succeeded.
* \exception An error is raised if the collection did not exist.
*/
Result BaseSession::dropCollection(String schema, String name){}

/**
* Drops a view from the specified schema.
* \return A SqlResult object if succeeded.
* \exception An error is raised if the view did not exist.
*/
Result BaseSession::dropView(String schema, String name){}

#endif
shcore::Value BaseSession::drop_schema_object(const shcore::Argument_list &args, const std::string& type)
{
  std::string function = class_name() + ".drop" + type;

  args.ensure_count(2, function.c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #1 is expected to be a string");

  if (args[1].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #2 is expected to be a string");

  std::string schema = args[0].as_string();
  std::string name = args[1].as_string();

  shcore::Value ret_val;
  if (type == "View")
    ret_val = executeStmt("sql", sqlstring("drop view !.!", 0) << schema << name + "", false, shcore::Argument_list());
  else
  {
    shcore::Argument_list command_args;
    command_args.push_back(Value(schema));
    command_args.push_back(Value(name));

    ret_val = executeAdminCommand("drop_collection", false, command_args);
  }

  if (_schemas->count(schema))
  {
    boost::shared_ptr<Schema> schema_obj = boost::static_pointer_cast<Schema>((*_schemas)[schema].as_object());
    if (schema_obj)
      schema_obj->_remove_object(name, type);
  }

  return ret_val;
}

/*
* This function verifies if the given object exist in the database, works for schemas, tables, views and collections.
* The check for tables, views and collections is done is done based on the type.
* If type is not specified and an object with the name is found, the type will be returned.
*
* Returns the name of the object as exists in the database.
*/
std::string BaseSession::db_object_exists(std::string &type, const std::string &name, const std::string& owner)
{
  std::string statement;
  std::string ret_val;

  if (type == "Schema")
  {
    shcore::Value res = executeStmt("sql", sqlstring("show databases like ?", 0) << name, true, shcore::Argument_list());
    boost::shared_ptr<SqlResult> my_res = res.as_object<SqlResult>();

    Value raw_entry = my_res->fetch_one(shcore::Argument_list());

    if (raw_entry)
    {
      boost::shared_ptr<mysh::Row> row = raw_entry.as_object<mysh::Row>();

      ret_val = row->get_member(0).as_string();
    }

    my_res->fetch_all(shcore::Argument_list());
  }
  else
  {
    shcore::Argument_list args;
    args.push_back(Value(owner));
    args.push_back(Value(name));

    Value myres = executeAdminCommand("list_objects", true, args);
    boost::shared_ptr<mysh::mysqlx::SqlResult> my_res = myres.as_object<mysh::mysqlx::SqlResult>();

    Value raw_entry = my_res->fetch_one(shcore::Argument_list());

    if (raw_entry)
    {
      boost::shared_ptr<mysh::Row> row = raw_entry.as_object<mysh::Row>();
      std::string object_name = row->get_member("name").as_string();
      std::string object_type = row->get_member("type").as_string();

      if (type.empty())
      {
        type = object_type;
        ret_val = object_name;
      }
      else
      {
        boost::algorithm::to_upper(type);

        if (type == object_type)
          ret_val = object_name;
      }
    }
    my_res->fetch_all(shcore::Argument_list());
  }

  return ret_val;
}

shcore::Value BaseSession::get_capability(const std::string& name)
{
  shcore::Value ret_val;

  if (_session)
  {
    const Mysqlx::Connection::Capabilities &caps(_session->connection()->capabilities());
    for (int c = caps.capabilities_size(), i = 0; i < c; i++)
    {
      if (caps.capabilities(i).name() == name)
      {
        const Mysqlx::Connection::Capability &cap(caps.capabilities(i));
        if (cap.value().type() == Mysqlx::Datatypes::Any::SCALAR &&
            cap.value().scalar().type() == Mysqlx::Datatypes::Scalar::V_STRING)
          ret_val = shcore::Value(cap.value().scalar().v_string().value());
        else if (cap.value().type() == Mysqlx::Datatypes::Any::SCALAR &&
                 cap.value().scalar().type() == Mysqlx::Datatypes::Scalar::V_OCTETS)
          ret_val = shcore::Value(cap.value().scalar().v_octets().value());
      }
    }
  }

  return ret_val;
}

shcore::Value BaseSession::get_status(const shcore::Argument_list &args)
{
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  if (class_name() == "XSession")
    (*status)["SESSION_TYPE"] = shcore::Value("X");
  else
    (*status)["SESSION_TYPE"] = shcore::Value("Node");

  shcore::Value node_type = get_capability("node_type");
  if (node_type)
    (*status)["NODE_TYPE"] = node_type;

  (*status)["DEFAULT_SCHEMA"] = shcore::Value(_default_schema);

  boost::shared_ptr< ::mysqlx::Result> result;
  boost::shared_ptr< ::mysqlx::Row>row;
  result = _session->executeSql("select DATABASE(), USER() limit 1");
  row = result->next();

  std::string current_schema = row->isNullField(0) ? "" : row->stringField(0);
  if (current_schema == "null")
    current_schema = "";

  (*status)["CURRENT_SCHEMA"] = shcore::Value(current_schema);
  (*status)["CURRENT_USER"] = shcore::Value(row->isNullField(1) ? "" : row->stringField(1));
  (*status)["CONNECTION_ID"] = shcore::Value(_session->connection()->client_id());
  //(*status)["SSL_CIPHER"] = shcore::Value(_conn->get_ssl_cipher());
  //(*status)["SKIP_UPDATES"] = shcore::Value(???);
  //(*status)["DELIMITER"] = shcore::Value(???);

  //(*status)["SERVER_INFO"] = shcore::Value(_conn->get_server_info());

  //(*status)["PROTOCOL_VERSION"] = shcore::Value(_conn->get_protocol_info());
  //(*status)["CONNECTION"] = shcore::Value(_conn->get_connection_info());
  //(*status)["INSERT_ID"] = shcore::Value(???);

  result = _session->executeSql("select @@character_set_client, @@character_set_connection, @@character_set_server, @@character_set_database, @@version_comment limit 1");
  row = result->next();
  (*status)["CLIENT_CHARSET"] = shcore::Value(row->isNullField(0) ? "" : row->stringField(0));
  (*status)["CONNECTION_CHARSET"] = shcore::Value(row->isNullField(1) ? "" : row->stringField(1));
  (*status)["SERVER_CHARSET"] = shcore::Value(row->isNullField(2) ? "" : row->stringField(2));
  (*status)["SCHEMA_CHARSET"] = shcore::Value(row->isNullField(3) ? "" : row->stringField(3));
  (*status)["SERVER_VERSION"] = shcore::Value(row->isNullField(4) ? "" : row->stringField(4));

  //(*status)["SERVER_STATS"] = shcore::Value(_conn->get_stats());

  // TODO: Review retrieval from charset_info, mysql connection

  // TODO: Embedded library stuff
  //(*status)["TCP_PORT"] = row->get_value(1);
  //(*status)["UNIX_SOCKET"] = row->get_value(2);
  //(*status)["PROTOCOL_COMPRESSED"] = row->get_value(3);

  // STATUS

  // SAFE UPDATES

  return shcore::Value(status);
}

shcore::Value BaseSession::set_current_schema(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".setCurrentSchema";
  args.ensure_count(1, function_name.c_str());

  if (_session)
  {
    std::string name = args[0].as_string();

    boost::shared_ptr< ::mysqlx::Result> result = _session->executeSql(sqlstring("use !", 0) << name);
    result->flush();
  }
  else
    throw Exception::runtime_error(class_name() + " not connected");

  return get_member("currentSchema");
}

boost::shared_ptr<BaseSession> XSession::_get_shared_this() const
{
  boost::shared_ptr<const XSession> shared = shared_from_this();

  return boost::const_pointer_cast<XSession>(shared);
}

boost::shared_ptr<shcore::Object_bridge> XSession::create(const shcore::Argument_list &args)
{
  return connect_session(args, mysh::Application);
}

NodeSession::NodeSession() : BaseSession()
{
  add_method("sql", boost::bind(&NodeSession::sql, this, _1), "sql", shcore::String, NULL);
  add_method("setCurrentSchema", boost::bind(&BaseSession::set_current_schema, this, _1), "name", shcore::String, NULL);
  add_method("getCurrentSchema", boost::bind(&ShellBaseSession::get_member_method, this, _1, "getCurrentSchema", "currentSchema"), NULL);
  add_method("quoteName", boost::bind(&NodeSession::quote_name, this, _1), "name", shcore::String, NULL);
}

boost::shared_ptr<BaseSession> NodeSession::_get_shared_this() const
{
  boost::shared_ptr<const NodeSession> shared = shared_from_this();

  return boost::const_pointer_cast<NodeSession>(shared);
}

boost::shared_ptr<shcore::Object_bridge> NodeSession::create(const shcore::Argument_list &args)
{
  return connect_session(args, mysh::Node);
}

#ifdef DOXYGEN
/**
* Creates a SqlExecute object to allow running the received SQL statement on the target MySQL Server.
* \param sql A string containing the SQL statement to be executed.
* \return A SqlExecute object.
* \sa SqlExecute
*
* This method creates an SqlExecute object which is a SQL execution handler.
*
* The SqlExecute class has functions that allow defining the way the statement will be executed and allows doing parameter binding.
*
* The received SQL is set on the execution handler.
*
* JavaScript Example
* \code{.js}
* var sql = session.sql("select * from mydb.students where  age > ?");
* var result = sql.bind(18).execute();
* \endcode
* \sa SqlExecute
*/
SqlExecute NodeSession::sql(String sql){}
#endif
shcore::Value NodeSession::sql(const shcore::Argument_list &args)
{
  boost::shared_ptr<SqlExecute> sql_execute(new SqlExecute(shared_from_this()));

  return sql_execute->sql(args);
}

#ifdef DOXYGEN
/**
* Sets the current schema for this session, and returns the schema object for it.
* At the database level, this is equivalent at issuing the following SQL query:
*   use <new-default-schema>;
*
* \sa getSchemas(), getSchema()
* \param name the name of the new schema to switch to.
* \return the Schema object for the new schema.
*/
Schema NodeSession::setCurrentSchema(String name){}
#endif

std::vector<std::string> NodeSession::get_members() const
{
  std::vector<std::string> members(BaseSession::get_members());
  members.push_back("currentSchema");
  return members;
}

#ifdef DOXYGEN
/**
* Retrieves the Schema set as active on the session.
* \return A Schema object or Null
*/
Schema NodeSession::getCurrentSchema(){}
#endif
Value NodeSession::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is a schema and attempt loading it as such
  if (prop == "currentSchema")
  {
    NodeSession *session = const_cast<NodeSession *>(this);
    std::string name = session->_retrieve_current_schema();

    if (!name.empty())
      ret_val = get_member(name);
    else
      ret_val = Value::Null();
  }
  else if (BaseSession::has_member(prop))
    ret_val = BaseSession::get_member(prop);
  return ret_val;
}

#ifdef DOXYGEN
/**
* Escapes the passed identifier.
* \return A String containing the escaped identifier.
*/
String NodeSession::quoteName(String id){}
#endif
shcore::Value NodeSession::quote_name(const shcore::Argument_list &args)
{
  args.ensure_count(1, "NodeSession.quoteName");

  if (args[0].type != shcore::String)
    throw shcore::Exception::type_error("Argument #1 is expected to be a string");

  std::string id = args[0].as_string();

  return shcore::Value(get_quoted_name(id));
}