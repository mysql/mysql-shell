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

#include <iostream>
#include <set>

ApiBaseSession::ApiBaseSession()
: _show_warnings(false)
{
  _schemas.reset(new shcore::Value::Map_type);
}

Value ApiBaseSession::connect(const Argument_list &args)
{
  std::string function_name = class_name() + ".connect";
  args.ensure_count(1, 2, function_name.c_str());

  std::string uri = args.string_at(0);

  std::string pwd_override;

  // If password is received as parameter, then it overwrites
  // Anything found on the URI
  if (2 == args.size())
    pwd_override = args.string_at(1).c_str();

  _uri = mysh::strip_password(uri);

  try
  {
    _session = ::mysqlx::openSession(uri, pwd_override);
  }
  CATCH_AND_TRANSLATE();

  _load_schemas();
  _load_default_schema();

  return Value::Null();
}

Value ApiBaseSession::executeSql(const Argument_list &args)
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
        ret_val = shcore::Value::wrap(new Resultset(_last_result));
      }
      CATCH_AND_TRANSLATE();
    }
  }

  return ret_val;
}

std::vector<std::string> ApiBaseSession::get_members() const
{
  std::vector<std::string> members(BaseSession::get_members());

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

Value ApiBaseSession::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is a schema and attempt loading it as such
  if (BaseSession::has_member(prop))
    ret_val = BaseSession::get_member(prop);
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

void ApiBaseSession::flush_last_result()
{
  if (_last_result)
    _last_result->discardData();
}

void ApiBaseSession::_load_default_schema()
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

void ApiBaseSession::_load_schemas()
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

shcore::Value ApiBaseSession::get_schema(const shcore::Argument_list &args) const
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

shcore::Value ApiBaseSession::set_default_schema(const shcore::Argument_list &args)
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

void ApiBaseSession::_update_default_schema(const std::string& name)
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

boost::shared_ptr<ApiBaseSession> Session::_get_shared_this() const
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

NodeSession::NodeSession() :ApiBaseSession()
{
  add_method("executeSql", boost::bind(&Session::executeSql, this, _1),
             "stmt", shcore::String,
             NULL);
}

boost::shared_ptr<ApiBaseSession> NodeSession::_get_shared_this() const
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