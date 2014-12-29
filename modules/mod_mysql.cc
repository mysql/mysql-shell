/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysql.h"

#include <boost/bind.hpp>

using namespace mysh;


#include <iostream>

class mysh::Mysql_resultset : public shcore::Cpp_object_bridge
{
public:
  Mysql_resultset(MYSQL_RES *res)
  : _result(res)
  {
  }

  virtual ~Mysql_resultset()
  {
    mysql_free_result(_result);
  }

  virtual std::string class_name() const
  {
    return "mysql_resultset";
  }

  virtual std::string &append_descr(std::string &s_out, bool pprint) const
  {
    s_out.append("<mysql_resultset>");
    return s_out;
  }

  virtual std::string &append_repr(std::string &s_out) const
  {
    s_out.append("<mysql_resultset>");
    return s_out;
  }

  virtual std::vector<std::string> get_members() const
  {
    std::vector<std::string> members(shcore::Cpp_object_bridge::get_members());
    members.push_back("count");
    return members;
  }

  virtual bool operator == (const Object_bridge &other) const
  {
    return this == &other;
  }

  virtual shcore::Value get_member(const std::string &prop) const
  {
    if (prop == "count")
      return shcore::Value((int64_t)mysql_num_rows(_result));
    return shcore::Value();
  }

  virtual void set_member(const std::string &prop, shcore::Value value)
  {
  }

private:
  MYSQL_RES *_result;
};


//-----------


static bool parse_mysql_connstring(const std::string &connstring,
                                   std::string &user, std::string &password,
                                   std::string &host, int &port, std::string &sock)
{
  // format is [user[:pass]]@host[:port] or user[:pass]@::socket, like what cmdline utilities use
  std::string::size_type p = connstring.rfind('@');
  std::string user_part = (p == std::string::npos) ? "root" : connstring.substr(0, p);
  std::string server_part = (p == std::string::npos) ? connstring : connstring.substr(p+1);

  if ((p = user_part.find(':')) != std::string::npos)
  {
    user = user_part.substr(0, p);
    password = user_part.substr(p+1);
  }
  else
    user = user_part;

  p = server_part.find(':');
  if (p != std::string::npos)
  {
    host = server_part.substr(0, p);
    server_part = server_part.substr(p+1);
    p = server_part.find(':');
    if (p != std::string::npos)
      sock = server_part.substr(p+1);
    else
      if (!sscanf(server_part.substr(0, p).c_str(), "%i", &port))
        return false;
  }
  else
    host = server_part;
  return true;
}





Mysql_connection::Mysql_connection(const std::string &uri)
: _mysql(NULL)
{
  add_function(boost::shared_ptr<shcore::Cpp_function>(new shcore::Cpp_function("close", boost::bind(&Mysql_connection::close, this, _1), NULL)));
  add_function(boost::shared_ptr<shcore::Cpp_function>(new shcore::Cpp_function("query", boost::bind(&Mysql_connection::query, this, _1),
                                                                        "sql", shcore::String, NULL)));

  std::string user;
  std::string pass;
  std::string host;
  int port;
  std::string sock;
  long flags = 0;

  _mysql = mysql_init(NULL);

  if (!parse_mysql_connstring(uri, user, pass, host, port, sock))
    throw std::invalid_argument("Could not parse URI for MySQL connection");

  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(), pass.c_str(), NULL, port, sock.empty() ? NULL : sock.c_str(), flags))
  {
    // use error class
    throw std::runtime_error(mysql_error(_mysql));
  }

  //TODO strip password from uri?
  _uri = uri;
}


shcore::Value Mysql_connection::close(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Mysql_connection::close");

  mysql_close(_mysql);
  _mysql = NULL;
  return shcore::Value(shcore::Null);
}


shcore::Value Mysql_connection::query(const shcore::Argument_list &args)
{
  MYSQL_RES *res;
  std::string query = args.string_at(0);
  args.ensure_count(1, "Mysql_connection::query");

  if (mysql_real_query(_mysql, query.c_str(), query.length()) < 0)
  {
    throw std::runtime_error(mysql_error(_mysql));
  }

  res = mysql_use_result(_mysql);
  if (!res)
  {
    throw std::runtime_error(mysql_error(_mysql));
  }

  return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(new Mysql_resultset(res)));
}


std::string Mysql_connection::class_name() const
{
  return "mysql_connection";
}


std::string &Mysql_connection::append_descr(std::string &s_out, bool pprint) const
{
  s_out.append("<mysql_connection:"+_uri+">");
  return s_out;
}


std::string &Mysql_connection::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}


std::vector<std::string> Mysql_connection::get_members() const
{
  return Cpp_object_bridge::get_members();
}


bool Mysql_connection::operator == (const Object_bridge &other) const
{
  return this == &other;
}


shcore::Value Mysql_connection::get_member(const std::string &prop) const
{
  return Cpp_object_bridge::get_member(prop);
}


void Mysql_connection::set_member(const std::string &prop, shcore::Value value)
{
}


class Mysql_connection_factory : public shcore::Object_factory
{
public:
  Mysql_connection_factory()
  {
  }

  virtual std::string name() const
  {
    return "Connection";
  }

  virtual boost::shared_ptr<shcore::Object_bridge> construct(const shcore::Argument_list &args)
  {
    args.ensure_count(1, "Connection constructor");

    return boost::shared_ptr<shcore::Object_bridge>(new Mysql_connection(args.string_at(0)));
  }
};


static struct Auto_register {
  Auto_register()
  {
    shcore::Object_factory::register_factory("mysql", new Mysql_connection_factory);
  }
} Mysql_connection_register;

