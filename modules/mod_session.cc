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

#include "mod_session.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"

#include "shellcore/proxy_object.h"

#include "mod_db.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>

#include "mod_mysql.h"
#ifdef X_PROTOCOL_ENABLED
#include "mod_mysqlx.h"
#endif


#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;


#include <iostream>


Session::Session(Shell_core *shc)
: _shcore(shc), _show_warnings(false)
{
  _schema_proxy.reset(new Proxy_object(boost::bind(&Session::get_db, this, _1)));

  add_method("sql", boost::bind(&Session::sql, this, _1),
             "stmt", shcore::String,
             "*options", shcore::Map,
             NULL);

  add_method("sql_one", boost::bind(&Session::sql_one, this, _1),
    "stmt", shcore::String,
    NULL);
}


Session::~Session()
{
}


Value Session::connect(const Argument_list &args)
{
  args.ensure_count(1, 2, "Session::connect");

  std::string uri = args.string_at(0);

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

  if (!parse_mysql_connstring(uri, protocol, user, pass, host, port, sock, db, pwd_found))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  // If password is received as parameter, then it overwrites
  // Anything found on the URI
  if (2 == args.size())
    pwd_override = args.string_at(1).c_str();

  if (protocol.empty() || protocol == "mysql")
    _conn.reset(new Mysql_connection(uri, pwd_override));
#ifdef X_PROTOCOL_ENABLED
  else if (protocol == "mysqlx")
    _conn.reset(new X_connection(uri, pwd_override));
#endif
  else
    throw shcore::Exception::argument_error("Invalid protocol found in URI");

  return Value::Null();
}


Value Session::sql(const Argument_list &args)
{
  if (!_conn)
  {
    _shcore->print("Not connected\n");
    return Value::Null();
  }

  // Options are the statement and optionally options to modify
  // How the resultset is created.
  std::string statement = args.string_at(0);
  Value options;
  if (args.size() == 2)
    options = args[1];

  if (statement.empty())
    _shcore->print_error("No query specified\n");
  else
  {
    try
    {
      Value result = _conn->sql(statement, options);
      return result;
    }
    catch (shcore::Exception &exc)
    {
      print_exception(exc);
    }
  }
  
  return Value::Null();
}

Value Session::sql_one(const Argument_list &args)
{
  if (!_conn)
  {
    _shcore->print("Not connected\n");
    return Value::Null();
  }

  // Options are the statement and optionally options to modify
  // How the resultset is created.
  std::string statement = args.string_at(0);

  if (statement.empty())
    _shcore->print_error("No query specified\n");
  else
  {
    try
    {
      Value result = _conn->sql_one(statement);
      return result;
    }
    catch (shcore::Exception &exc)
    {
      print_exception(exc);
    }
  }

  return Value::Null();
}

void Session::print_exception(const shcore::Exception &e)
{
  // Removes the type, at this moment only type is MySQLError
  // Reason is that leaving it will print things like:
  // ERROR: MySQLError (code) message
  // Where ERROR is added on the called print_error routine
  //std::string message = (*e.error())["type"].as_string();
  
  std::string message;
  if ((*e.error()).has_key("code"))
  {
    //message.append(" ");
    message.append(((*e.error())["code"].repr()));
                   
    if ((*e.error()).has_key("state") && (*e.error())["state"])
      message.append((boost::format(" (%s)") % ((*e.error())["state"].as_string())).str());
  }
  
  message.append(": ");
  message.append(e.what());
  
  _shcore->print_error(message);
}


std::string Session::class_name() const
{
  return "Session";
}


std::string &Session::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  if (!_conn)
    s_out.append("<Session:disconnected>");
  else
    s_out.append("<Session:"+_conn->uri()+">");
  return s_out;
}


std::string &Session::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}


std::vector<std::string> Session::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("connection");
  members.push_back("conn");
  members.push_back("dbs");
  members.push_back("schemas");
  return members;
}


Value Session::get_member(const std::string &prop) const
{
  if (prop == "conn" || prop == "connection")
  {
    if (!_conn)
      return Value::Null();
    else
      return Value(boost::static_pointer_cast<Object_bridge>(_conn));
  }
  else if (prop == "dbs" || prop == "schemas")
  {
    return Value(boost::static_pointer_cast<Object_bridge>(_schema_proxy));
  }

  if (!_conn)
  {
    _shcore->print("Not connected. Use connect(<uri>) or \\connect <uri>\n");
    return Value::Null();
  }

  return Cpp_object_bridge::get_member(prop);
}


void Session::set_member(const std::string &prop, Value value)
{
  Cpp_object_bridge::set_member(prop, value);
}


bool Session::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}


Value Session::get_db(const std::string &schema)
{
  if (_conn)
  {
    boost::shared_ptr<Db> db(new Db(shared_from_this(), schema));
    db->cache_table_names();
    return Value(boost::static_pointer_cast<Object_bridge>(db));
  }
  throw Exception::runtime_error("Session not connected");
}


boost::shared_ptr<Db> Session::default_schema()
{
  if (_conn)
  {
    Value res = _conn->sql_one("select schema()");
    if (res && res.type == Map && res.as_map()->get_type("schema()") == String)
    {
      boost::shared_ptr<Db> db(new Db(shared_from_this(), res.as_map()->get_string("schema()")));
      db->cache_table_names();
      return db;
    }
  }
  return boost::shared_ptr<Db>();
}

