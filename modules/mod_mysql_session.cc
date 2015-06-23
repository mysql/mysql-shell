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

#include "mod_mysql_session.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"

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

REGISTER_OBJECT(mysql, Session);

#include <iostream>

Session::Session()
: _show_warnings(false)
{
  //_schema_proxy.reset(new Proxy_object(boost::bind(&Session::get_db, this, _1)));

  add_method("executeSql", boost::bind(&Session::executeSql, this, _1),
    "stmt", shcore::String,
    NULL);

  _schemas.reset(new shcore::Value::Map_type);
}

Connection *Session::connection()
{
  return _conn.get();
}

Value Session::connect(const Argument_list &args)
{
  args.ensure_count(1, 2, "Session::connect");

  std::string uri_ = args.string_at(0);

  std::string protocol;
  std::string user;
  std::string pass;
  const char *pwd_override = NULL;
  std::string host;
  std::string sock;
  std::string db;
  std::string uri_stripped;

  int pwd_found;
  int port = 0;

  if (!parse_mysql_connstring(uri_, protocol, user, pass, host, port, sock, db, pwd_found))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  // If password is received as parameter, then it overwrites
  // Anything found on the URI
  if (2 == args.size())
    pwd_override = args.string_at(1).c_str();

  _conn.reset(new Connection(uri_, pwd_override));

  _load_schemas();
  _load_default_schema();

  return Value::Null();
}

Value Session::executeSql(const Argument_list &args)
{
  args.ensure_count(1, "Session::sql");
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
      ret_val = Value::wrap(new Resultset(boost::shared_ptr<Result>(_conn->executeSql(statement))));
  }

  return ret_val;
}

std::string Session::uri() const
{
  return _conn->uri();
}

std::vector<std::string> Session::get_members() const
{
  std::vector<std::string> members(BaseSession::get_members());

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

Value Session::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is a schema and attempt loading it as such
  if (BaseSession::has_member(prop))
    ret_val = BaseSession::get_member(prop);
  else if (prop == "schemas")
    ret_val = Value(_schemas);
  else if (prop == "uri")
    ret_val = Value(_conn->uri());
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

void Session::_load_default_schema()
{
  _default_schema.reset();

  if (_conn)
  {
    shcore::Argument_list query;
    query.push_back(Value("select schema()"));

    Value res = executeSql(query);

    boost::shared_ptr<Resultset> rset = res.as_object<Resultset>();
    Value next_row = rset->next(shcore::Argument_list());

    if (next_row)
    {
      boost::shared_ptr<mysh::Row> row = next_row.as_object<mysh::Row>();
      shcore::Value schema = row->get_member("schema()");

      if (schema)
      {
        std::string name = schema.as_string();
        _update_default_schema(name);
      }
    }
  }
}

void Session::_load_schemas()
{
  if (_conn)
  {
    shcore::Argument_list query;
    query.push_back(Value("show databases;"));

    Value res = executeSql(query);

    shcore::Argument_list args;
    boost::shared_ptr<Resultset> rset = res.as_object<Resultset>();
    Value next_row = rset->next(args);
    boost::shared_ptr<mysh::Row> row;

    while (next_row)
    {
      row = next_row.as_object<mysh::Row>();
      shcore::Value schema = row->get_member("Database");
      if (schema)
      {
        boost::shared_ptr<Schema> object(new Schema(shared_from_this(), schema.as_string()));
        (*_schemas)[schema.as_string()] = shcore::Value(boost::static_pointer_cast<Object_bridge>(object));
      }

      next_row = rset->next(args);
    }
  }
}

shcore::Value Session::get_schema(const shcore::Argument_list &args) const
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
    if (_conn)
    {
      boost::shared_ptr<Schema> schema(new Schema(shared_from_this(), name));

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

shcore::Value Session::set_default_schema(const shcore::Argument_list &args)
{
  args.ensure_count(1, "Session.setDefaultSchema");

  if (_conn)
  {
    std::string name = args[0].as_string();

    shcore::Argument_list query;
    query.push_back(Value("use " + name + ";"));

    Value res = executeSql(query);

    _update_default_schema(name);
  }
  else
    throw Exception::runtime_error("Session not connected");

  return get_member("defaultSchema");
}

void Session::_update_default_schema(const std::string& name)
{
  if (!name.empty())
  {
    if (_schemas->has_key(name))
      _default_schema = (*_schemas)[name].as_object<Schema>();
    else
    {
      _default_schema.reset(new Schema(shared_from_this(), name));
      _default_schema->cache_table_objects();
      (*_schemas)[name] = Value(boost::static_pointer_cast<Object_bridge>(_default_schema));
    }
  }
}

boost::shared_ptr<shcore::Object_bridge> Session::create(const shcore::Argument_list &args)
{
  boost::shared_ptr<Session> session(new Session());

  session->connect(args);

  return session;
}