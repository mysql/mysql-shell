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

#include "shellcore/proxy_object.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;
using namespace mysh::mysqlx;

REGISTER_OBJECT(mysqlx, Session);
REGISTER_OBJECT(mysqlx, NodeSession);
REGISTER_OBJECT(mysqlx, Expression);

#include <set>

BaseSession::BaseSession()
: _show_warnings(false)
{
  _schemas.reset(new shcore::Value::Map_type);

  add_method("close", boost::bind(&BaseSession::close, this, _1), "data");
  add_method("setFetchWarnings", boost::bind(&BaseSession::set_fetch_warnings, this, _1), "data");
}

#ifdef DOXYGEN
/**
* \brief Connects to a X Protocol enabled MySQL Product.
* \param connectionData: string representation of the connection data in URI format.
* \param password: optional parameter, the password could be specified through this parameter or embedded on the connectionData string.
*
* \return Null
*
* The URI format is as follows:
* [user[:password]]\@localhost[:port][/schema]
*
* JavaScript Example
* \code{.js}
* // Using password.
* var pwd = "mypwd";
* session.connect("rennox\@localhost/sakila", pwd);
*
* // With password embedded on the URI.
* session.connect("rennox:mypwd\@localhost:33460/sakila");
* \endcode
*/
Undefined BaseSession::connect(String connectionData, String password)
{}

/**
* \brief Connects to a X Protocol enabled MySQL Product.
* \param connectionData: Dictionary containing the connection data.
* \param password: optional parameter, the password could be specified through this parameter or embedded on the connectionData dictionary.
*
* The connection data is specified on the connectionData dictionary with the next entries:
* - host: the host for the target MySQL Product.
* - port: the port where the target product is listening.
* - schema: the schema to be set as default for this session.
* - dbUser: the user name to be used on the connection.
* - dbPassword: the user password.
*
* \return undefined
*
* JavaScript Example
* \code{.js}
* // Using password
* var pwd = "mypwd";
* session.connect({dbUser:"rennox", host:"localhost", schema:"sakila"}, mypwd);
*
* // With password embedded on the connectionData
* session.connect({dbUser:"rennox", host:"localhost", port:3307, dbPassword: pwd});
* \endcode
*/
Undefined BaseSession::connect(Map connectionData, String password)
{}
#endif
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
      _session = ::mysqlx::openSession(uri_, pwd_override);
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

      _session = ::mysqlx::openSession(host, port, schema, user, password);

      std::stringstream str;
      str << user << "@" << host << ":" << port;
      _uri = str.str();
    }
    else
      throw shcore::Exception::argument_error("Unexpected argument on connection data.");
  }
  CATCH_AND_TRANSLATE();

  _load_schemas();
  _load_default_schema();

  return Value::Null();
}

#ifdef DOXYGEN
/**
* \brief Closes the session.
*/
Undefined BaseSession::close()
{}
#endif

Value BaseSession::close(const Argument_list &args)
{
  std::string function_name = class_name() + ".close";

  args.ensure_count(0, function_name.c_str());

  _session.reset();

  return shcore::Value();
}

Value BaseSession::executeSql(const Argument_list &args)
{
  std::string function_name = class_name() + ".executeSql";
  args.ensure_count(1, function_name.c_str());
  // Will return the result of the SQL execution
  // In case of error will be Undefined
  Value ret_val;
  if (!_session)
    throw Exception::logic_error("Not connected.");
  else
  {
    // Options are the statement and optionally options to modify
    // How the resultset is created.
    std::string statement = args.string_at(0);
    Value options;
    if (args.size() == 2)
      options = args[1];

    if (statement.empty())
      throw Exception::argument_error("No query specified.");
    else
    {
      try
      {
        // Cleans out the comm buffer
        flush_last_result();

        _last_result.reset(_session->executeSql(statement));

        // Calls wait so any error is properly triggered at execution time
        _last_result->wait();

        ret_val = shcore::Value::wrap(new Resultset(_last_result));
      }
      CATCH_AND_TRANSLATE();
    }
  }

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

Value BaseSession::executeAdminCommand(const std::string& command, const Argument_list &args)
{
  std::string function_name = class_name() + ".executeAdminCommand";
  args.ensure_at_least(1, function_name.c_str());

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
      flush_last_result();

      _last_result.reset(_session->executeStmt("xplugin", command, arguments));

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
  return false;
}

#ifdef DOXYGEN
/**
* \brief Retrieves the Schema configured as default for the session, if none, returns Null.
*/
Schema BaseSession::getDefaultSchema()
{}

/**
* \brief Retrieves the List of Schemas available on the session.
*/
List BaseSession::getSchemas()
{}

/**
* \brief Retrieves the connectionData in string format.
*/
String BaseSession::getUri()
{}
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
    if (_default_schema)
      ret_val = Value(boost::static_pointer_cast<Object_bridge>(_default_schema));
    else
      ret_val = Value::Null();
  }
  else
  {
    // Since the property was not satisfied, we assume it is a schema and
    // proceed to retrieve it
    shcore::Argument_list args;
    args.push_back(Value(prop));

    ret_val = get_schema(args);
  }

  return ret_val;
}

void BaseSession::flush_last_result()
{
  if (_last_result)
    _last_result->discardData();
}

void BaseSession::_load_default_schema()
{
  try
  {
    _default_schema.reset();

    if (_session)
    {
      // Cleans out the comm buffer
      flush_last_result();

      // TODO: update this logic properly
      ::mysqlx::Result* result = _session->executeSql("select schema()");
      ::mysqlx::Row *row = result->next();

      std::string name;
      if (!row->isNullField(0))
        name = row->stringField(0);

      result->discardData();

      _update_default_schema(name);
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
      // Cleans out the comm buffer
      flush_last_result();

      ::mysqlx::Result* result = _session->executeSql("show databases;");
      ::mysqlx::Row *row = result->next();

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

      result->discardData();
    }
  }
  CATCH_AND_TRANSLATE();
}

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
      throw Exception::runtime_error("Session not connected");
  }

  // If this point is reached, the schema will be there!
  return (*_schemas)[name];
}

Schema setDefaultSchema();

shcore::Value BaseSession::set_default_schema(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".setDefaultSchema";
  args.ensure_count(1, function_name.c_str());

  if (_session)
  {
    // Cleans out the comm buffer
    flush_last_result();

    std::string name = args[0].as_string();

    ::mysqlx::Result* result = _session->executeSql("use " + name + ";");
    result->discardData();

    _update_default_schema(name);
  }
  else
    throw Exception::runtime_error("Session not connected");

  return get_member("defaultSchema");
}

//Undefined setFetchWarnings(Bool value);

shcore::Value BaseSession::set_fetch_warnings(const shcore::Argument_list &args)
{
  Value ret_val;

  args.ensure_count(1, (class_name() + "::setFetchWarnings").c_str());

  bool enable = args.bool_at(0);
  std::string command = enable ? "enable_notices" : "disable_notices";

  shcore::Argument_list command_args;
  command_args.push_back(Value("warnings"));

  executeAdminCommand(command, command_args);

  return ret_val;
}

void BaseSession::_update_default_schema(const std::string& name)
{
  if (!name.empty())
  {
    if (_schemas->has_key(name))
      _default_schema = (*_schemas)[name].as_object<Schema>();
    else
    {
      _default_schema.reset(new Schema(_get_shared_this(), name));
      _default_schema->cache_table_objects();
      (*_schemas)[name] = Value(boost::static_pointer_cast<Object_bridge>(_default_schema));
    }
  }
}

boost::shared_ptr<BaseSession> Session::_get_shared_this() const
{
  boost::shared_ptr<const Session> shared = shared_from_this();

  return boost::const_pointer_cast<Session>(shared);
}

boost::shared_ptr<shcore::Object_bridge> Session::create(const shcore::Argument_list &args)
{
  boost::shared_ptr<Session> session(new Session());

  session->connect(args);

  return boost::static_pointer_cast<Object_bridge>(session);
}

NodeSession::NodeSession() : BaseSession()
{
  add_method("executeSql", boost::bind(&Session::executeSql, this, _1),
             "stmt", shcore::String,
             NULL);
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
* Executes an sql statement and returns a Resultset object
* \param sql: The statement to be executed
*
* \return Resultset object
*
* JavaScript Example
* \code{.js}
* var result = session.executeSql("show databases");
* \endcode
*/
Resultset NodeSession::executeSql(String sql)
{}
#endif