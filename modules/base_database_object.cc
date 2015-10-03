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

#include "base_database_object.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/common.h"
#include "shellcore/proxy_object.h"
#include "base_session.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>

using namespace mysh;
using namespace shcore;

DatabaseObject::DatabaseObject(boost::shared_ptr<ShellBaseSession> session, boost::shared_ptr<DatabaseObject> schema, const std::string &name)
: _session(session), _schema(schema), _name(name)
{
  add_method("getName", boost::bind(&DatabaseObject::get_member_method, this, _1, "getName", "name"), NULL);
  add_method("getSession", boost::bind(&DatabaseObject::get_member_method, this, _1, "getSession", "session"), NULL);
  add_method("getSchema", boost::bind(&DatabaseObject::get_member_method, this, _1, "getSchema", "schema"), NULL);
  add_method("existInDatabase", boost::bind(&DatabaseObject::existInDatabase, this, _1), "data");
}

DatabaseObject::~DatabaseObject()
{
}

std::string &DatabaseObject::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const
{
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

std::string &DatabaseObject::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}

void DatabaseObject::append_json(const shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  dumper.append_string("class", class_name());
  dumper.append_string("name", _name);

  dumper.end_object();
}

std::vector<std::string> DatabaseObject::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("name");
  members.push_back("session");
  members.push_back("schema");

  return members;
}

shcore::Value DatabaseObject::get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop)
{
  std::string function = class_name() + "." + method;
  args.ensure_count(0, function.c_str());

  return get_member(prop);
}

bool DatabaseObject::operator == (const Object_bridge &other) const
{
  if (class_name() == other.class_name())
  {
    return _session.lock() == ((DatabaseObject*)&other)->_session.lock() &&
           _schema.lock() == ((DatabaseObject*)&other)->_schema.lock() &&
           _name == ((DatabaseObject*)&other)->_name &&
           class_name() == other.class_name();
  }
  return false;
}

bool DatabaseObject::has_member(const std::string &prop) const
{
  return Cpp_object_bridge::has_member(prop) ||
    prop == "name" ||
    prop == "session" ||
    prop == "schema";
}

#ifdef DOXYGEN
/**
* Returns the name of this database object.
* \return the name as an String object.
*/
String DatabaseObject::getName(){}

/**
* Returns the Session object of this database object.
* \return the Session object used to get to this object.
*
* Note that the returned object can be any of:
* - XSession: if the object was created/retrieved using an XSession instance.
* - NodeSession: if the object was created/retrieved using an NodeSession instance.
* - ClassicSession: if the object was created/retrieved using an ClassicSession instance.
*/
Object DatabaseObject::getSession(){}

/**
* Returns the Schema object of this database object.
* \return the object for this schema of this database object.
*
* Note that the returned object can be any of:
* - Schema: if the object was created/retrieved using a Schema instance.
* - ClassicSchema: if the object was created/retrieved using an ClassicSchema instance.
*/
Object DatabaseObject::getSchema(){}
#endif
Value DatabaseObject::get_member(const std::string &prop) const
{
  Value ret_val;

  if (prop == "name")
    ret_val = Value(_name);
  else if (prop == "session")
  {
    if (_session._empty())
      ret_val = Value::Null();
    else
      ret_val = Value(boost::static_pointer_cast<Object_bridge>(_session.lock()));
  }
  else if (prop == "schema")
  {
    if (_schema._empty())
      ret_val = Value::Null();
    else
    ret_val = Value(boost::static_pointer_cast<Object_bridge>(_schema.lock()));
  }
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

#ifdef DOXYGEN
/**
* Verifies if this object exists in the database.
*/
Undefined DatabaseObject::existInDatabase(){}
#endif
shcore::Value DatabaseObject::existInDatabase(const shcore::Argument_list &args)
{
  std::string cname(class_name());
  return shcore::Value(_session.lock()->db_object_exists(cname, _name, _schema._empty() ? "" : _schema.lock()->get_member("name").as_string()));
}