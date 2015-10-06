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

#include "mod_mysql_schema.h"
#include "mod_mysql_table.h"
#include "mod_mysql_view.h"
#include "mod_mysql_session.h"
#include "mod_mysql_resultset.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"

#include "shellcore/proxy_object.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>

using namespace mysh::mysql;
using namespace shcore;

ClassicSchema::ClassicSchema(boost::shared_ptr<ClassicSession> session, const std::string &schema)
: DatabaseObject(boost::dynamic_pointer_cast<ShellBaseSession>(session), boost::shared_ptr<DatabaseObject>(), schema)
{
  init();
}

ClassicSchema::ClassicSchema(boost::shared_ptr<const ClassicSession> session, const std::string &schema) :
DatabaseObject(boost::const_pointer_cast<ClassicSession>(session), boost::shared_ptr<DatabaseObject>(), schema)
{
  init();
}

void ClassicSchema::init()
{
  add_method("getTables", boost::bind(&DatabaseObject::get_member_method, this, _1, "getTables", "tables"), "name", shcore::String, NULL);
  add_method("getViews", boost::bind(&DatabaseObject::get_member_method, this, _1, "getViews", "views"), "name", shcore::String, NULL);

  add_method("getTable", boost::bind(&ClassicSchema::getTable, this, _1), "name", shcore::String, NULL);
  add_method("getView", boost::bind(&ClassicSchema::getView, this, _1), "name", shcore::String, NULL);

  _tables = Value::new_map().as_map();
  _views = Value::new_map().as_map();
}

ClassicSchema::~ClassicSchema()
{
}

void ClassicSchema::cache_table_objects()
{
  boost::shared_ptr<ClassicSession> sess(boost::dynamic_pointer_cast<ClassicSession>(_session.lock()));
  if (sess)
  {
    Result *result = sess->connection()->run_sql("show full tables in `" + _name + "`");
    Row *row = result->fetch_one();
    while (row)
    {
      std::string object_name = row->get_value_as_string(0);
      std::string object_type = row->get_value_as_string(1);

      if (object_type == "BASE TABLE" || object_type == "LOCAL TEMPORARY")
        (*_tables)[object_name] = Value::wrap(new ClassicTable(shared_from_this(), object_name));
      else if (object_type == "VIEW" || object_type == "SYSTEM VIEW")
        (*_views)[object_name] = Value::wrap(new ClassicView(shared_from_this(), object_name));

      row = result->fetch_one();
    }
  }
}

void ClassicSchema::_remove_object(const std::string& name, const std::string& type)
{
  if (type == "View")
  {
    if (_views->count(name))
      _views->erase(name);
  }
  else if (type == "Table")
  {
    if (_tables->count(name))
      _tables->erase(name);
  }
}

std::vector<std::string> ClassicSchema::get_members() const
{
  std::vector<std::string> members(DatabaseObject::get_members());
  members.push_back("tables");
  members.push_back("views");

  for (Value::Map_type::const_iterator iter = _tables->begin();
       iter != _tables->end(); ++iter)
  {
    members.push_back(iter->first);
  }

  for (Value::Map_type::const_iterator iter = _views->begin();
       iter != _views->end(); ++iter)
  {
    members.push_back(iter->first);
  }

  return members;
}

Value ClassicSchema::get_member(const std::string &prop) const
{
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is must be either a table, collection or view and attempt loading it as such
  if (DatabaseObject::has_member(prop))
    ret_val = DatabaseObject::get_member(prop);
  else if (prop == "tables")
    ret_val = Value(_tables);
  else if (prop == "views")
    ret_val = Value(_views);
  else
  {
    // At this point the property should be one of table
    // collection or view
    ret_val = find_in_collection(prop, _tables);

    if (!ret_val)
      ret_val = find_in_collection(prop, _views);

    if (!ret_val)
      ret_val = _load_object(prop);

    if (!ret_val)
      throw Exception::attrib_error("Invalid object member " + prop);
  }

  return ret_val;
}

#ifdef DOXYGEN
/**
* Returns the table of the given name for this schema.
* \sa ClassicTable
* \param name the name of the table to look for.
* \return the ClassicTable object matching the name.
*/
ClassicTable ClassicSchema::getTable(String name)
{}
#endif
shcore::Value ClassicSchema::getTable(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + ".getTable").c_str());
  std::string name = args.string_at(0);

  Value::Map_type::const_iterator iter = _tables->find(name);
  if (iter != _tables->end())
    return Value(boost::shared_ptr<Object_bridge>(iter->second.as_object()));
  else
    return Value();

  //return shcore::Value::wrap(new ClassicTable(shared_from_this(), name));
}

#ifdef DOXYGEN
/**
* Returns the view of the given name for this schema.
* \sa ClassicView
* \param name the name of the view to look for.
* \return the ClassicView object matching the name.
*/
ClassicView ClassicSchema::getView(String name)
{}
#endif
shcore::Value ClassicSchema::getView(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + ".getCollection").c_str());
  std::string name = args.string_at(0);

  Value::Map_type::const_iterator iter = _views->find(name);
  if (iter != _views->end())
    return Value(boost::shared_ptr<Object_bridge>(iter->second.as_object()));
  else
    return Value();

  //return shcore::Value::wrap(new ClassicView(shared_from_this(), name));
}

shcore::Value ClassicSchema::find_in_collection(const std::string& name, boost::shared_ptr<shcore::Value::Map_type>source) const
{
  Value::Map_type::const_iterator iter = source->find(name);
  if (iter != source->end())
    return Value(boost::shared_ptr<Object_bridge>(iter->second.as_object()));
  else
    return Value();
}

Value ClassicSchema::_load_object(const std::string& name, const std::string& type) const
{
  Value ret_val;

  std::string found_type(type);
  if (_session.lock()->db_object_exists(found_type, name, _name))
  {
    if (found_type == "BASE TABLE" || found_type == "LOCAL TEMPORARY")
    {
      ret_val = Value::wrap(new ClassicTable(shared_from_this(), name));
      (*_tables)[name] = ret_val;
    }
    else if (found_type == "VIEW" || found_type == "SYSTEM VIEW")
    {
      ret_val = Value::wrap(new ClassicView(shared_from_this(), name));
      (*_views)[name] = ret_val;
    }
  }

  return ret_val;
}