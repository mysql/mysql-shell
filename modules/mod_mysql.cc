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

#include "shellcore/object_factory.h"


#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

using namespace mysh;


#include <iostream>

class mysh::Mysql_resultset : public shcore::Cpp_object_bridge
{
public:
  Mysql_resultset(MYSQL_RES *res)
  : _result(res)
  {
    add_method("next", boost::bind(&Mysql_resultset::next, this, _1), NULL);

    _fields = mysql_fetch_fields(res);
    _num_fields = mysql_num_fields(res);
  }

  virtual ~Mysql_resultset()
  {
    mysql_free_result(_result);
  }

  virtual std::string class_name() const
  {
    return "mysql_resultset";
  }

  virtual std::string &append_descr(std::string &s_out, int indent=-1, bool quote_strings=false) const
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
    return shcore::Cpp_object_bridge::get_member(prop);
  }

  virtual void set_member(const std::string &prop, shcore::Value value)
  {
  }

private:
  shcore::Value next(const shcore::Argument_list &args)
  {
    args.ensure_count(0, "Mysql_resultset::next");

    MYSQL_ROW row = mysql_fetch_row(_result);
    if (!row)
      return shcore::Value::Null();

    unsigned long *lengths;
    lengths = mysql_fetch_lengths(_result);

    return row_to_doc(row, lengths);
  }

  shcore::Value row_to_doc(const MYSQL_ROW &row, unsigned long *lengths)
  {
    boost::shared_ptr<shcore::Value::Map_type> map(new shcore::Value::Map_type);

    for (int i = 0; i < _num_fields; i++)
    {
      if (row[i] == NULL)
        map->insert(std::make_pair(_fields[i].name, shcore::Value::Null()));
      else
      {
        switch (_fields[i].type)
        {
          case MYSQL_TYPE_STRING:
          case MYSQL_TYPE_VARCHAR:
          case MYSQL_TYPE_VAR_STRING:
            map->insert(std::make_pair(_fields[i].name, shcore::Value(std::string(row[i], lengths[i]))));
            break;

          case MYSQL_TYPE_TINY:
          case MYSQL_TYPE_SHORT:
          case MYSQL_TYPE_INT24:
          case MYSQL_TYPE_LONG:
          case MYSQL_TYPE_LONGLONG:
            map->insert(std::make_pair(_fields[i].name, shcore::Value(boost::lexical_cast<int64_t>(row[i]))));
            break;

          case MYSQL_TYPE_DOUBLE:
            map->insert(std::make_pair(_fields[i].name, shcore::Value(boost::lexical_cast<double>(row[i]))));
            break;
        }
      }
    }
    return shcore::Value(map);
  }

private:
  MYSQL_RES *_result;
  MYSQL_FIELD *_fields;
  int _num_fields;
};


//-----------


static bool parse_mysql_connstring(const std::string &connstring,
                                   std::string &user, std::string &password,
                                   std::string &host, int &port, std::string &sock,
                                   std::string &db)
{
  // format is [user[:pass]]@host[:port][/db] or user[:pass]@::socket[/db], like what cmdline utilities use
  std::string s = connstring;
  std::string::size_type p = connstring.find('/');
  if (p != std::string::npos)
  {
    db = connstring.substr(p+1);
    s = connstring.substr(0, p);
  }
  p = s.rfind('@');
  std::string user_part = (p == std::string::npos) ? "root" : s.substr(0, p);
  std::string server_part = (p == std::string::npos) ? s : s.substr(p+1);

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
  add_method("close", boost::bind(&Mysql_connection::close, this, _1), NULL);
  add_method("sql", boost::bind(&Mysql_connection::sql, this, _1),
             "stmt", shcore::String,
             "*options", shcore::Map,
             NULL);

  std::string user;
  std::string pass;
  std::string host;
  int port;
  std::string sock;
  std::string db;
  long flags = 0;

  _mysql = mysql_init(NULL);

  if (!parse_mysql_connstring(uri, user, pass, host, port, sock, db))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  if (!mysql_real_connect(_mysql, host.c_str(), user.c_str(), pass.c_str(), db.empty() ? NULL : db.c_str(), port, sock.empty() ? NULL : sock.c_str(), flags | CLIENT_MULTI_STATEMENTS))
  {
    throw shcore::Exception::error_with_code("MySQLError", mysql_error(_mysql), mysql_errno(_mysql));
  }

  //TODO strip password from uri?
  _uri = uri;
}


shcore::Value Mysql_connection::uri(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Mysql_connection::uri");
  return shcore::Value(_uri);
}


shcore::Value Mysql_connection::close(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Mysql_connection::close");

  std::cout << "disconnect\n";
  if (_mysql)
    mysql_close(_mysql);
  _mysql = NULL;
  return shcore::Value(shcore::Null);
}


shcore::Value Mysql_connection::sql(const shcore::Argument_list &args)
{
  MYSQL_RES *res;
  std::string query = args.string_at(0);

  args.ensure_count(1, 2, "Mysql_connection::sql");

  if (mysql_real_query(_mysql, query.c_str(), query.length()) < 0)
  {
    throw shcore::Exception::error_with_code("MySQLError", mysql_error(_mysql), mysql_errno(_mysql));
  }

  res = mysql_store_result(_mysql);
  if (!res)
  {
    throw shcore::Exception::error_with_code("MySQLError", mysql_error(_mysql), mysql_errno(_mysql));
  }
  return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(new Mysql_resultset(res)));
}


MYSQL_RES *Mysql_connection::raw_sql(const std::string &query)
{
  if (mysql_real_query(_mysql, query.c_str(), query.length()) != 0)
  {
    throw shcore::Exception::error_with_code("MySQLError", mysql_error(_mysql), mysql_errno(_mysql));
  }

  return mysql_store_result(_mysql);
}

MYSQL_RES *Mysql_connection::next_result()
{
  MYSQL_RES* next_result = NULL;

  if(mysql_more_results(_mysql))
  {
    // mysql_next_result has the next return values:
    //  0: success and there are more results
    // -1: succeess and this is the last one
    // >1: in case of error
    // So we assign the result on the first two cases
    if (mysql_next_result(_mysql) < 1)
      next_result = mysql_store_result(_mysql);
  }
  
  return next_result;
}

my_ulonglong Mysql_connection::affected_rows()
{
  return mysql_affected_rows(_mysql);
}


Mysql_connection::~Mysql_connection()
{
  close(shcore::Argument_list());
}

/*
shcore::Value Mysql_connection::stats(const shcore::Argument_list &args)
{
  return shcore::Value();
}
*/

std::string Mysql_connection::class_name() const
{
  return "mysql_connection";
}


std::string &Mysql_connection::append_descr(std::string &s_out, int indent, bool quote_strings) const
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
  std::cout << "get "<<prop<<"\n";
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

