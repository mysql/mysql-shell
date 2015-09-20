/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysqlx_session.h"
#include "mod_mysqlx_schema.h"
#include "mod_mysqlx_resultset.h"
#include "mod_mysqlx_expression.h"
#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "mod_mysqlx_session_sql.h"

#include "shellcore/proxy_object.h"

#include "mysqlxtest_utils.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/algorithm/string.hpp>

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;
using namespace mysh::mysqlx;

REGISTER_OBJECT(mysqlx, XSession);
REGISTER_OBJECT(mysqlx, NodeSession);
REGISTER_OBJECT(mysqlx, Expression);

#include <set>

#ifdef DOXYGEN
/**
* Creates a XSession instance using the provided connection data.
* \param connectionData the connection string used to connect to the database.
* \param password if provided, will override the password in the connection string.
* \return An XSession instance.
*
* A XSession object uses the X Protocol to allow executing operations on the connected MySQL Product.
*
* The format of the connection string can be as follows for connections using the TCP protocol:
*
* [user[:pass]\@]host[:port][/db]
*
* or as follows for connections using a socket or named pipe:
*
* [user[:pass\@]\::socket[/db]
*
* \sa XSession
*/
XSession getSession(String connectionData, String password){}

/**
* This function works the same as the function above, except that the connection data comes enclosed on a dictionary object.
* \param connectionData a map with the connection data as key value pairs, the following keys are recognized:
*  - host, the host to use for the connection (can be an IP or DNS name)
*  - port, the TCP port where the server is listening (default value is 33060).
*  - schema, the current database for the connection's session.
*  - dbUser, the user to authenticate against.
*  - dbPassword, the password of the user user to authenticate against.
* \param password if provided, will override the password in the connection string.
* \return An XSession instance.
*
* \sa XSession
*/
XSession getSession(Map connectionData, String password){}

/**
* This function works the same as getSession(String connectionData, String password), except that it will return a NodeSession object which allows SQL Execution.
*/
NodeSession getNodeSession(String connectionData, String password){}

/**
* This function works the same as getSession(Map connectionData, String password), except that it will return a NodeSession object which allows SQL Execution.
*/
NodeSession getNodeSession(Map connectionData, String password){}

#endif

BaseSession::BaseSession()
{
  _schemas.reset(new shcore::Value::Map_type);

  add_method("close", boost::bind(&BaseSession::close, this, _1), "data");
  add_method("setFetchWarnings", boost::bind(&BaseSession::set_fetch_warnings, this, _1), "data");
}

Value BaseSession::connect(const Argument_list &args)
{
  std::string function_name = class_name() + ".connect";
  args.ensure_count(1, 2, function_name.c_str());

  std::string pwd_override;

  // If password is received as parameter, then it overwrites
  // Anything found on the URI
  if (2 == args.size())
    pwd_override = args.string_at(1).c_str();

  try
  {
    if (args[0].type == String)
    {
      std::string uri_ = args.string_at(0);
      _uri = mysh::strip_password(uri_);
      _session = ::mysqlx::openSession(uri_, pwd_override, ::mysqlx::Ssl_config(), true);
    }
    else if (args[0].type == Map)
    {
      std::string host;
      int port = 33060;
      std::string schema;
      std::string user;
      std::string password;

      shcore::Value::Map_type_ref options = args[0].as_map();

      if (options->has_key("host"))
        host = (*options)["host"].as_string();

      if (options->has_key("port"))
        port = (*options)["port"].as_int();

      if (options->has_key("schema"))
        schema = (*options)["schema"].as_string();

      if (options->has_key("dbUser"))
        user = (*options)["dbUser"].as_string();

      if (options->has_key("dbPassword"))
        password = (*options)["dbPassword"].as_string();

      if (!pwd_override.empty())
        password = pwd_override;

      _session = ::mysqlx::openSession(host, port, schema, user, password, ::mysqlx::Ssl_config());

      std::stringstream str;
      str << user << "@" << host << ":" << port;
      _uri = str.str();
    }
    else
      throw shcore::Exception::argument_error("Unexpected argument on connection data.");
  }
  CATCH_AND_TRANSLATE();

  _load_schemas();
  _default_schema = _retrieve_current_schema();

  return Value::Null();
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

  _session.reset();

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
    ret_val = executeSql(args.string_at(0), shcore::Argument_list());
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
Value BaseSession::createSchema(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".createSchema";
  args.ensure_count(1, function_name.c_str());

  Value ret_val;
  try
  {
    std::string schema = args.string_at(0);
    std::string statement = "create schema " + schema;
    ret_val = executeStmt("sql", statement, shcore::Argument_list());

    // if reached this point it indicates that there were no errors
    boost::shared_ptr<Schema> object(new Schema(_get_shared_this(), schema));
    ret_val = shcore::Value(boost::static_pointer_cast<Object_bridge>(object));

    (*_schemas)[schema] = ret_val;
  }
  CATCH_AND_TRANSLATE();

  return ret_val;
}

::mysqlx::ArgumentValue BaseSession::get_argument_value(shcore::Value source)
{
  ::mysqlx::ArgumentValue ret_val;
  switch (source.type)
  {
    case shcore::Bool:
    case shcore::UInteger:
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

Value BaseSession::executeSql(const std::string& statement, const Argument_list &args)
{
  return executeStmt("sql", statement, args);
}

Value BaseSession::executeAdminCommand(const std::string& command, const Argument_list &args)
{
  std::string function_name = class_name() + ".executeAdminCommand";
  args.ensure_at_least(1, function_name.c_str());

  return executeStmt("xplugin", command, args);
}

Value BaseSession::executeStmt(const std::string &domain, const std::string& command, const Argument_list &args)
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
      _last_result = _session->executeStmt(domain, command, arguments);

      // Calls wait so any error is properly triggered at execution time
      _last_result->wait();

      ret_val = shcore::Value::wrap(new Resultset(_last_result));
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
    set.insert(iter->first);

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
  if (prop == "uri" || prop == "schemas" || prop == "defaultSchema")
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
  else if (prop == "defaultSchema")
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
  Value ret_val;

  args.ensure_count(1, (class_name() + ".setFetchWarnings").c_str());

  bool enable = args.bool_at(0);
  std::string command = enable ? "enable_notices" : "disable_notices";

  shcore::Argument_list command_args;
  command_args.push_back(Value("warnings"));

  executeAdminCommand(command, command_args);

  return ret_val;
}

void BaseSession::drop_db_object(const std::string &type, const std::string &name, const std::string& owner)
{
  if (type == "Schema")
    executeStmt("sql", "drop schema `" + name + "`", shcore::Argument_list());
  else if (type == "View")
    executeStmt("sql", "drop view `" + owner + "`.`" + name + "`", shcore::Argument_list());
  else
  {
    shcore::Argument_list command_args;
    command_args.push_back(Value(owner));
    command_args.push_back(Value(name));

    executeAdminCommand("drop_collection", command_args);
  }
}

/*
* This function verifies if the given object exist in the database, works for schemas, tables, views and collections.
* The check for tables, views and collections is done is done based on the type.
* If type is not specified and an object with the name is found, the type will be returned.
*/
bool BaseSession::db_object_exists(std::string &type, const std::string &name, const std::string& owner)
{
  std::string statement;
  bool ret_val = false;

  if (type == "Schema")
  {
    shcore::Value res = executeStmt("sql", "show databases like \"" + name + "\"", shcore::Argument_list());
    boost::shared_ptr<Resultset> res_obj = res.as_object<Resultset>();

    ret_val = res_obj->all(shcore::Argument_list()).as_array()->size() > 0;
  }
  else
  {
    shcore::Argument_list args;
    args.push_back(Value(owner));
    args.push_back(Value(name));

    Value myres = executeAdminCommand("list_objects", args);
    boost::shared_ptr<mysh::mysqlx::Resultset> my_res = myres.as_object<mysh::mysqlx::Resultset>();

    Value raw_entry = my_res->next(shcore::Argument_list());

    if (raw_entry)
    {
      boost::shared_ptr<mysh::Row> row = raw_entry.as_object<mysh::Row>();
      std::string object_name = row->get_member("name").as_string();
      std::string object_type = row->get_member("type").as_string();

      if (type.empty())
      {
        type = object_type;
        ret_val = true;
      }
      else
      {
        boost::algorithm::to_upper(type);

        if (type == object_type)
          ret_val = true;
      }
    }

    my_res->all(shcore::Argument_list());
  }

  return ret_val;
}

boost::shared_ptr<BaseSession> XSession::_get_shared_this() const
{
  boost::shared_ptr<const XSession> shared = shared_from_this();

  return boost::const_pointer_cast<XSession>(shared);
}

boost::shared_ptr<shcore::Object_bridge> XSession::create(const shcore::Argument_list &args)
{
  boost::shared_ptr<XSession> session(new XSession());

  session->connect(args);

  return boost::static_pointer_cast<Object_bridge>(session);
}

NodeSession::NodeSession() : BaseSession()
{
  add_method("sql", boost::bind(&NodeSession::sql, this, _1), "sql", shcore::String, NULL);
  add_method("setCurrentSchema", boost::bind(&NodeSession::set_current_schema, this, _1), "name", shcore::String, NULL);
  add_method("getCurrentSchema", boost::bind(&ShellBaseSession::get_member_method, this, _1, "getCurrentSchema", "currentSchema"), NULL);
}

boost::shared_ptr<BaseSession> NodeSession::_get_shared_this() const
{
  boost::shared_ptr<const NodeSession> shared = shared_from_this();

  return boost::const_pointer_cast<NodeSession>(shared);
}

boost::shared_ptr<shcore::Object_bridge> NodeSession::create(const shcore::Argument_list &args)
{
  boost::shared_ptr<NodeSession> session(new NodeSession());

  session->connect(args);

  return boost::static_pointer_cast<Object_bridge>(session);
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
Resultset NodeSession::sql(String sql){}
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
shcore::Value NodeSession::set_current_schema(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".setCurrentSchema";
  args.ensure_count(1, function_name.c_str());

  if (_session)
  {
    std::string name = args[0].as_string();

    boost::shared_ptr< ::mysqlx::Result> result = _session->executeSql("use " + name + ";");
    result->flush();
  }
  else
    throw Exception::runtime_error("NodeSession not connected");

  return get_member("currentSchema");
}

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
  if (BaseSession::has_member(prop))
    ret_val = BaseSession::get_member(prop);
  else if (prop == "currentSchema")
  {
    NodeSession *session = const_cast<NodeSession *>(this);
    std::string name = session->_retrieve_current_schema();

    if (!name.empty())
      ret_val = get_member(name);
    else
      ret_val = Value::Null();
  }

  return ret_val;
}