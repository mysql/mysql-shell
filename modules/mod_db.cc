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

#include "mod_db.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

using namespace mysh;
using namespace shcore;


#include <iostream>


Db::Db(Shell_core *shc)
: _shcore(shc)
{
  add_method("sql", boost::bind(&Db::sql, this, _1),
             "stmt", shcore::String,
             "*options", shcore::Map,
             NULL);
}


Db::~Db()
{
}


Value Db::connect(const Argument_list &args)
{
  args.ensure_count(1, "Db::connect");

  if (!_conns.empty())
  {
    _shcore->print("Closing old connections...\n");
    _conns.clear();
  }

  Mysql_connection *conn;
  if (args[0].type == String)
  {
    std::string uri = args.string_at(0);
    _shcore->print("Connecting to "+uri+"...\n");
    conn = new Mysql_connection(uri);
    _conns.push_back(boost::shared_ptr<Mysql_connection>(conn));
    _shcore->print("Connection opened and assigned to variable db\n");
  }
  else if (args[0].type == Map)
  {
    boost::shared_ptr<Value::Map_type> object(args.map_at(0));

    std::string name = object->get_string("name", "");
    boost::shared_ptr<Value::Array_type> servers(object->get_array("servers"));
    _shcore->print("Connecting to server group "+name+"...\n");
    for (Value::Array_type::const_iterator iter = servers->begin(); iter != servers->end(); ++iter)
    {
      _shcore->print(iter->descr(false)+"...\n");
      conn = new Mysql_connection(iter->as_string());
      _conns.push_back(boost::shared_ptr<Mysql_connection>(conn));
    }
  }
  return Value::Null();
}


Value Db::sql(const Argument_list &args)
{
  if (_conns.empty())
  {
    _shcore->print("Not connected\n");
    return Value::Null();
  }
  for (std::vector<boost::shared_ptr<Mysql_connection> >::iterator c = _conns.begin();
       c != _conns.end(); ++c)
  {
    MYSQL_RES *result = (*c)->raw_sql(args.string_at(0));

    // print rows from result, with stats etc
    print_result(result);

    mysql_free_result(result);
  }
  return Value::Null();
}


void Db::print_result(MYSQL_RES *res)
{
  //XXX

  _shcore->print("%1% row in set (%2% sec)\n");
}


std::string Db::class_name() const
{
  return "Db";
}


std::string &Db::append_descr(std::string &s_out, bool pprint) const
{
  s_out.append("<Db>");
  return s_out;
}


std::string &Db::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}


std::vector<std::string> Db::get_members() const
{
  return Cpp_object_bridge::get_members();
}


bool Db::operator == (const Object_bridge &other) const
{
  return this == &other;
}


Value Db::get_member(const std::string &prop) const
{
  if (_conns.empty())
  {
    _shcore->print("Not connected\n");
    return Value::Null();
  }

  std::cout << "get "<<prop<<"\n";
  return Cpp_object_bridge::get_member(prop);
}


void Db::set_member(const std::string &prop, Value value)
{
}
