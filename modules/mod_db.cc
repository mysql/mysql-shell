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

#include "mod_db.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"

#include "shellcore/proxy_object.h"

#include "mod_session.h"
#include "mod_db_table.h"

#include "mod_connection.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>


using namespace mysh;
using namespace shcore;


#include <iostream>


Db::Db(boost::shared_ptr<Session> session, const std::string &schema)
: _session(session), _schema(schema)
{
}


Db::~Db()
{
}


void Db::cache_table_names()
{
  boost::shared_ptr<Session> sess(_session.lock());
  if (sess)
  {
    _tables = Value::new_map().as_map();
    {
      Value options(Value::new_map());
      (*options.as_map())["key_by_index"] = Value::True();
      Value result = sess->conn()->sql("show tables in `"+_schema+"`", options);
      Value doc;
      while ((doc = result.as_object<Base_resultset>()->next(Argument_list())) && doc.type != Null)
      {
        std::string table = (*doc.as_map())["0"].as_string();
        (*_tables)[table] = Value(Object_bridge_ref(new Db_table(shared_from_this(), table)));
      }
    }
  }
}


std::string Db::class_name() const
{
  return "Db";
}


std::string &Db::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  s_out.append("<Db:"+_schema+">");
  return s_out;
}


std::string &Db::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}


std::vector<std::string> Db::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("name");
  members.push_back("t");
  members.push_back("session");
  for (Value::Map_type::const_iterator iter = _tables->begin();
       iter != _tables->end(); ++iter)
  {
    members.push_back(iter->first);
  }
  return members;
}


bool Db::operator == (const Object_bridge &other) const
{
  if (class_name() == other.class_name())
  {
    return _session.lock() == ((Db*)&other)->_session.lock() && _schema == ((Db*)&other)->_schema;
  }
  return false;
}


Value Db::get_member(const std::string &prop) const
{
  if (prop == "name")
    return Value(_schema);
  else if (prop == "session")
    return Value(boost::static_pointer_cast<Object_bridge>(_session.lock()));
  else if (prop == "t")
    return Value(_tables);

  Value::Map_type::const_iterator iter = _tables->find(prop);
  if (iter == _tables->end())
  {
    return Value(boost::shared_ptr<Object_bridge>(new Db_table(const_cast<Db*>(this)->shared_from_this(), prop)));
  }
  else
  {
    return Value(boost::shared_ptr<Object_bridge>(iter->second.as_object()));
  }
}


void Db::set_member(const std::string &prop, Value value)
{
  Cpp_object_bridge::set_member(prop, value);
}


