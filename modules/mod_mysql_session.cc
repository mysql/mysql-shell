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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "mod_mysql_session.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/server_registry.h"

#include "shellcore/proxy_object.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>
#include <set>

#include "mysql.h"
#include "mod_mysql_resultset.h"
#include "mod_mysql_schema.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh::mysql;
using namespace shcore;

REGISTER_OBJECT(mysql, ClassicSession);

#ifdef DOXYGEN
/**
* Creates a ClassicSession instance using the provided connection data.
* \param connectionData the connection string used to connect to the database.
* \param password if provided, will override the password in the connection string.
* \return An ClassicSession instance.
*
* A ClassicSession object uses the traditional MySQL Protocol to allow executing operations on the connected MySQL Server.
*
* The format of the connection string can be as follows for connections using the TCP protocol:
*
* [user[:pass]\@]host[:port][/db]
*
* or as follows for connections using a socket or named pipe:
*
* [user[:pass\@]\::socket[/db]
*
* \sa ClassicSession
*/
ClassicSession getClassicSession(String connectionData, String password){}

/**
* This function works the same as the function above, except that the connection data comes enclosed on a dictionary object.
* \param connectionData a map with the connection data as key value pairs, the following keys are recognized:
*  - host, the host to use for the connection (can be an IP or DNS name)
*  - port, the TCP port where the server is listening (default value is 3306).
*  - schema, the current database for the connection's session.
*  - dbUser, the user to authenticate against.
*  - dbPassword, the password of the user user to authenticate against.
* \param password if provided, will override the password in the connection string.
* \return An ClassicSession instance.
*
* \sa ClassicSession
*/
ClassicSession getClassicSession(Map connectionData, String password){}
#endif

ClassicSession::ClassicSession()
{
  //_schema_proxy.reset(new Proxy_object(boost::bind(&ClassicSession::get_db, this, _1)));

  add_method("close", boost::bind(&ClassicSession::close, this, _1), "data");
  add_method("sql", boost::bind(&ClassicSession::sql, this, _1),
    "stmt", shcore::String,
    NULL);
  add_method("setCurrentSchema", boost::bind(&ClassicSession::set_current_schema, this, _1), "name", shcore::String, NULL);
  add_method("getCurrentSchema", boost::bind(&ShellBaseSession::get_member_method, this, _1, "getCurrentSchema", "currentSchema"), NULL);
  add_method("startTransaction", boost::bind(&ClassicSession::startTransaction, this, _1), "data");
  add_method("commit", boost::bind(&ClassicSession::commit, this, _1), "data");
  add_method("rollback", boost::bind(&ClassicSession::rollback, this, _1), "data");

  _schemas.reset(new shcore::Value::Map_type);
}

Connection *ClassicSession::connection()
{
  return _conn.get();
}

Value ClassicSession::connect(const Argument_list &args)
{
  args.ensure_count(1, 2, "ClassicSession.connect");

  std::string protocol;
  std::string user;
  std::string pass;
  const char *pwd_override = NULL;
  std::string host;
  std::string sock;
  std::string db;
  std::string uri_stripped;
  std::string ssl_ca;
  std::string ssl_cert;
  std::string ssl_key;

  int pwd_found;
  int port = 3306;

  // If password is received as parameter, then it overwrites
  // Anything found on the URI
  if (2 == args.size())
    pwd_override = args.string_at(1).c_str();

  if (args[0].type == String)
  {
    std::string uri_ = args.string_at(0);

    if (!parse_mysql_connstring(uri_, protocol, user, pass, host, port, sock, db, pwd_found, ssl_ca, ssl_cert, ssl_key))
      throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

    _conn.reset(new Connection(uri_, pwd_override));
  }
  else if (args[0].type == Map)
  {
    // TODO: Take into account datasource / app args
    std::string data_source_file;
    std::string app;
    shcore::Value::Map_type_ref options = args[0].as_map();

    if (options->has_key("dataSourceFile"))
    {
      data_source_file = (*options)["dataSourceFile"].as_string();
      app = (*options)["app"].as_string();

      shcore::Server_registry sr(data_source_file);
      shcore::Connection_options& conn = sr.get_connection_by_name(app);

      host = conn.get_server();
      port = boost::lexical_cast<int>(conn.get_port());
      user = conn.get_user();
      pass = conn.get_password();
      if (pwd_override)
        pass = pwd_override;

      ssl_ca = conn.get_value_if_exists("ssl_ca");
      ssl_cert = conn.get_value_if_exists("ssl_cert");
      ssl_key = conn.get_value_if_exists("ssl_key");
    }
    else
    {
      if (options->has_key("host"))
        host = (*options)["host"].as_string();

      if (options->has_key("port"))
        port = (*options)["port"].as_int();

      if (options->has_key("schema"))
        db = (*options)["schema"].as_string();

      if (options->has_key("dbUser"))
        user = (*options)["dbUser"].as_string();

      if (options->has_key("dbPassword"))
        pass = (*options)["dbPassword"].as_string();

      if (options->has_key("ssl_ca"))
        ssl_ca = (*options)["ssl_ca"].as_string();

      if (options->has_key("ssl_cert"))
        ssl_cert = (*options)["ssl_cert"].as_string();

      if (options->has_key("ssl_key"))
        ssl_key = (*options)["ssl_key"].as_string();

      if (pwd_override)
        pass.assign(pwd_override);
    }

    _conn.reset(new Connection(host, port, sock, user, pass, db, ssl_ca, ssl_cert, ssl_key));
  }
  else
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  _load_schemas();
  _default_schema = _retrieve_current_schema();

  return Value::Null();
}

#ifdef DOXYGEN
/**
* Closes the internal connection to the MySQL Server held on this session object.
*/
Undefined ClassicSession::close(){}
#endif
Value ClassicSession::close(const shcore::Argument_list &args)
{
  args.ensure_count(0, "ClassicSession.close");

  _conn.reset();

  return shcore::Value();
}

#ifdef DOXYGEN
/**
* Executes a query against the database and returns a  ClassicResultset object wrapping the result.
* \param query the SQL query to execute against the database.
* \return A ClassicResultset object.
* \exception An exception is thrown if an error occurs on the SQL execution.
*/
ClassicResultset ClassicSession::sql(String query){}
#endif
Value ClassicSession::sql(const shcore::Argument_list &args)
{
  args.ensure_count(1, "ClassicSession.sql");
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
      ret_val = Value::wrap(new ClassicResultset(boost::shared_ptr<Result>(_conn->executeSql(statement))));
  }

  return ret_val;
}

#ifdef DOXYGEN
/**
* Creates a schema on the database and returns the corresponding object.
* \param name A string value indicating the schema name.
* \return The created schema object.
* \exception An exception is thrown if an error occurs creating the Session.
*/
ClassicSchema ClassicSession::createSchema(String name){}
#endif
Value ClassicSession::createSchema(const shcore::Argument_list &args)
{
  args.ensure_count(1, "ClassicSession.createSchema");

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
      std::string statement = "create schema " + schema;
      ret_val = Value::wrap(new ClassicResultset(boost::shared_ptr<Result>(_conn->executeSql(statement))));

      boost::shared_ptr<ClassicSchema> object(new ClassicSchema(shared_from_this(), schema));

      // If reached this point it indicates the schema was created successfully
      ret_val = shcore::Value(boost::static_pointer_cast<Object_bridge>(object));
      (*_schemas)[schema] = ret_val;
    }
  }

  return ret_val;
}

#ifdef DOXYGEN
/**
* Returns the connection string passed to connect() method.
* \return A string representation of the connection data in URI format (excluding the password or the database).
* \sa connect
*/
String ClassicSession::getUri(){}
#endif
std::string ClassicSession::uri() const
{
  return _conn->uri();
}

std::vector<std::string> ClassicSession::get_members() const
{
  std::vector<std::string> members(ShellBaseSession::get_members());

  members.push_back("currentSchema");

  // This function is here only to append the schemas as direct members
  // Will use a set to prevent duplicates
  std::set<std::string> set(members.begin(), members.end());
  for (Value::Map_type::const_iterator iter = _schemas->begin(); iter != _schemas->end(); ++iter)
    set.insert(iter->first);

  for (std::vector<std::string>::iterator index = members.begin(); index != members.end(); ++index)
    set.erase(*index);

  for (std::set<std::string>::iterator index = set.begin(); index != set.end(); ++index)
    members.push_back(*index);

  return members;
}

#ifdef DOXYGEN
/**
* Retrieves the ClassicSchema configured as default for the session.
* \return A ClassicSchema object or Null
*
* If the configured schema is not valid anymore Null wil be returned.
*/
ClassicSchema ClassicSession::getDefaultSchema(){}

/**
* Retrieves the ClassicSchema that is active as current on the session.
* \return A ClassicSchema object or Null
*
* The current schema is configured either throu setCurrentSchema(String name) or by using the USE statement.
*/
ClassicSchema ClassicSession::getCurrentSchema(){}

/**
* Retrieves the Schemas available on the session.
* \return A Map containing the ClassicSchema objects available o the session.
*/
Map ClassicSession::getSchemas(){}

/**
* Retrieves the connection data for this session in string format.
* \return A string representing the connection data.
*/
String ClassicSession::getUri(){}
#endif
Value ClassicSession::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is a schema and attempt loading it as such
  if (ShellBaseSession::has_member(prop))
    ret_val = ShellBaseSession::get_member(prop);
  else if (prop == "schemas")
    ret_val = Value(_schemas);
  else if (prop == "uri")
    ret_val = Value(_conn->uri());
  else if (prop == "defaultSchema")
  {
    if (!_default_schema.empty())
      return get_member(_default_schema);
    else
      ret_val = Value::Null();
  }
  else if (prop == "currentSchema")
  {
    ClassicSession *session = const_cast<ClassicSession *>(this);
    std::string name = session->_retrieve_current_schema();

    if (!name.empty())
      ret_val = get_member(name);
    else
      ret_val = Value::Null();
  }
  else
  {
    if (_schemas->has_key(prop))
    {
      boost::shared_ptr<ClassicSchema> schema = (*_schemas)[prop].as_object<ClassicSchema>();

      // This will validate the schema continues valid
      schema->cache_table_objects();

      ret_val = (*_schemas)[prop];
    }
  }

  return ret_val;
}

std::string ClassicSession::_retrieve_current_schema()
{
  std::string name;

  if (_conn)
  {
    shcore::Argument_list query;
    query.push_back(Value("select schema()"));

    Value res = sql(query);

    boost::shared_ptr<ClassicResultset> rset = res.as_object<ClassicResultset>();
    Value next_row = rset->next(shcore::Argument_list());

    if (next_row)
    {
      boost::shared_ptr<mysh::Row> row = next_row.as_object<mysh::Row>();
      shcore::Value schema = row->get_member("schema()");

      if (schema)
        name = schema.as_string();
    }
  }

  return name;
}

void ClassicSession::_load_schemas()
{
  if (_conn)
  {
    shcore::Argument_list query;
    query.push_back(Value("show databases;"));

    Value res = sql(query);

    shcore::Argument_list args;
    boost::shared_ptr<ClassicResultset> rset = res.as_object<ClassicResultset>();
    Value next_row = rset->next(args);
    boost::shared_ptr<mysh::Row> row;

    while (next_row)
    {
      row = next_row.as_object<mysh::Row>();
      shcore::Value schema = row->get_member("Database");
      if (schema)
      {
        boost::shared_ptr<ClassicSchema> object(new ClassicSchema(shared_from_this(), schema.as_string()));
        (*_schemas)[schema.as_string()] = shcore::Value(boost::static_pointer_cast<Object_bridge>(object));
      }

      next_row = rset->next(args);
    }
  }
}

void ClassicSession::_remove_schema(const std::string& name)
{
  if (_schemas->count(name))
    _schemas->erase(name);
}

#ifdef DOXYGEN
/**
* Retrieves a ClassicSchema object from the current session through it's name.
* \param name The name of the ClassicSchema object to be retrieved.
* \return The ClassicSchema object with the given name.
* \exception An exception is thrown if the given name is not a valid schema on the Session.
* \sa ClassicSchema
*/
ClassicSchema ClassicSession::getSchema(String name){}
#endif
shcore::Value ClassicSession::get_schema(const shcore::Argument_list &args) const
{
  std::string function_name = class_name() + ".getSchema";
  args.ensure_count(1, function_name.c_str());

  std::string name = args[0].as_string();

  if (_schemas->has_key(name))
  {
    boost::shared_ptr<ClassicSchema> schema = (*_schemas)[name].as_object<ClassicSchema>();

    // This will validate the schema continues valid
    schema->cache_table_objects();
  }
  else
  {
    if (_conn)
    {
      boost::shared_ptr<ClassicSchema> schema(new ClassicSchema(shared_from_this(), name));

      // Here this call will also validate the schema is valid
      schema->cache_table_objects();

      (*_schemas)[name] = Value(boost::static_pointer_cast<Object_bridge>(schema));
    }
    else
      throw Exception::runtime_error("ClassicSession not connected");
  }

  // If this point is reached, the schema will be there!
  return (*_schemas)[name];
}

#ifdef DOXYGEN
/**
* Sets the selected schema for this session's connection.
* \return The new schema.
*/
ClassicSchema ClassicSession::setCurrentSchema(String schema){}
#endif
shcore::Value ClassicSession::set_current_schema(const shcore::Argument_list &args)
{
  args.ensure_count(1, "ClassicSession.setCurrentSchema");

  if (_conn)
  {
    std::string name = args[0].as_string();

    shcore::Argument_list query;
    query.push_back(Value("use " + name + ";"));

    Value res = sql(query);
  }
  else
    throw Exception::runtime_error("ClassicSession not connected");

  return get_member("currentSchema");
}

boost::shared_ptr<shcore::Object_bridge> ClassicSession::create(const shcore::Argument_list &args)
{
  boost::shared_ptr<ClassicSession> session(new ClassicSession());

  session->connect(args);

  return session;
}

void ClassicSession::drop_db_object(const std::string &type, const std::string &name, const std::string& owner)
{
  std::string statement;

  if (type == "ClassicSchema")
    statement = "drop schema `" + name + "`";
  else if (type == "ClassicView")
    statement = "drop view `" + owner + "`.`" + name + "`";
  else
    statement = "drop table `" + owner + "`.`" + name + "`";

  // We execute the statement, any error will be reported properly
  _conn->executeSql(statement);

  if (type == "ClassicSchema")
    _remove_schema(name);
  else
  {
    if (_schemas->count(owner))
    {
      boost::shared_ptr<ClassicSchema> schema = boost::static_pointer_cast<ClassicSchema>((*_schemas)[owner].as_object());
      if (schema)
        schema->_remove_object(name, type);
    }
  }
}

/*
* This function verifies if the given object exist in the database, works for schemas, tables and views.
* The check for tables and views is done is done based on the type.
* If type is not specified and an object with the name is found, the type will be returned.
*/

bool ClassicSession::db_object_exists(std::string &type, const std::string &name, const std::string& owner)
{
  std::string statement;
  bool ret_val = false;

  if (type == "ClassicSchema")
  {
    statement = "show databases like \"" + name + "\"";
    Result *res = _conn->executeSql(statement);
    if (res->has_resultset())
    {
      Row *row = res->next();
      if (row)
        ret_val = true;
    }
  }
  else
  {
    statement = "show full tables from `" + owner + "` like \"" + name + "\"";
    Result *res = _conn->executeSql(statement);

    if (res->has_resultset())
    {
      Row *row = res->next();

      if (row)
      {
        std::string db_type = row->get_value(1).as_string();

        if (type == "ClassicTable" && (db_type == "BASE TABLE" || db_type == "LOCAL TEMPORARY"))
          ret_val = true;
        else if (type == "ClassicView" && (db_type == "VIEW" || db_type == "SYSTEM VIEW"))
          ret_val = true;
        else if (type.empty())
        {
          ret_val = true;
          type = db_type;
        }
      }
    }
  }

  return ret_val;
}

shcore::Value ClassicSession::startTransaction(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".startTransaction";
  args.ensure_count(0, function_name.c_str());

  return Value::wrap(new ClassicResultset(boost::shared_ptr<Result>(_conn->executeSql("start transaction"))));
}

shcore::Value ClassicSession::commit(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".startTransaction";
  args.ensure_count(0, function_name.c_str());

  return Value::wrap(new ClassicResultset(boost::shared_ptr<Result>(_conn->executeSql("commit"))));
}

shcore::Value ClassicSession::rollback(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".startTransaction";
  args.ensure_count(0, function_name.c_str());

  return Value::wrap(new ClassicResultset(boost::shared_ptr<Result>(_conn->executeSql("rollback"))));
}