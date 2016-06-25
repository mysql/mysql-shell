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
#include "utils/utils_general.h"
#include "base_session.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>
#include <set>

using namespace mysh;
using namespace shcore;

DatabaseObject::DatabaseObject(boost::shared_ptr<ShellBaseSession> session, boost::shared_ptr<DatabaseObject> schema, const std::string &name)
  : _session(session), _schema(schema), _name(name)
{
  init();
}

DatabaseObject::~DatabaseObject()
{
}

void DatabaseObject::init()
{
  add_property("name", "getName");
  add_property("session", "getSession");
  add_property("schema", "getSchema");

  add_method("existsInDatabase", boost::bind(&DatabaseObject::existsInDatabase, this, _1), "data");
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

void DatabaseObject::append_json(shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  dumper.append_string("class", class_name());
  dumper.append_string("name", _name);

  dumper.end_object();
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


#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function. The content of the returned value depends on the property being requested. The next list shows the valid properties as well as the returned value for each of them:
 *
 * \li name: returns a String object with the name of this database object.
 * \li schema: returns the schema object that owns this DatabaseObject, so it could be either an instance of Schema or ClassicSchema. If this DatabaseObject is either an instance of Schema or ClassicSchema it returns Null.
 * \li session: returns a session object under which the DatabaseObject was created, it could be any of XSession, NodeSession or ClassicSession.
 */
#else
/**
* Returns the name of this database object.
* \return the name as an String object.
*/
#if DOXYGEN_JS
String DatabaseObject::getName(){}
#elif DOXYGEN_PY
str DatabaseObject::get_name(){}
#endif
/**
* Returns the Session object of this database object.
* \return the Session object used to get to this object.
*
* Note that the returned object can be any of:
* - XSession: if the object was created/retrieved using an XSession instance.
* - NodeSession: if the object was created/retrieved using an NodeSession instance.
* - ClassicSession: if the object was created/retrieved using an ClassicSession instance.
*/
#if DOXYGEN_JS
Object DatabaseObject::getSession(){}
#elif DOXYGEN_PY
object DatabaseObject::get_session(){}
#endif

/**
* Returns the Schema object of this database object.
* \return the object for this schema of this database object.
*
* Note that the returned object can be any of:
* - Schema: if the object was created/retrieved using a Schema instance.
* - ClassicSchema: if the object was created/retrieved using an ClassicSchema instance.
*/
#if DOXYGEN_JS
Object DatabaseObject::getSchema(){}
#elif DOXYGEN_PY
object DatabaseObject::get_schema(){}
#endif
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

//! Verifies if this object exists in the database.
#if DOXYGEN_JS
Bool DatabaseObject::existsInDatabase(){}
#elif DOXYGEN_PY
bool DatabaseObject::exists_in_database(){}
#endif
shcore::Value DatabaseObject::existsInDatabase(const shcore::Argument_list &args)
{
  std::string type(get_object_type());
  return shcore::Value(!_session.lock()->db_object_exists(type, _name, _schema._empty() ? "" : _schema.lock()->get_member("name").as_string()).empty());
}

void DatabaseObject::update_cache(const std::vector<std::string>& names, const std::function<shcore::Value(const std::string &name)>& generator, Cache target_cache, DatabaseObject* target)
{
  std::set<std::string> existing;

  // Backups the existing items in the collection
  shcore::Value::Map_type::iterator index, end = target_cache->end();

  for (auto index = target_cache->begin(); index != end; index++)
    existing.insert(index->first);

  // Ensures the existing items are on the cache
  for (auto name : names)
  {
    if (existing.find(name) == existing.end())
    {
      (*target_cache)[name] = generator(name);

      if (target && shcore::is_valid_identifier(name))
        target->add_property(name);
    }

    else
      existing.erase(name);
  }

  // Removes no longer existing items
  for (auto name : existing)
  {
    target_cache->erase(name);

    if (target)
      target->delete_property(name);
  }
}

void DatabaseObject::update_cache(const std::string& name, const std::function<shcore::Value(const std::string &name)>& generator, bool exists, Cache target_cache, DatabaseObject* target)
{
  if (exists && target_cache->find(name) == target_cache->end())
  {
    (*target_cache)[name] = generator(name);

    if (target && shcore::is_valid_identifier(name))
      target->add_property(name);
  }

  if (!exists && target_cache->find(name) != target_cache->end())
  {
    target_cache->erase(name);

    if (target)
      target->delete_property(name);
  }
}

void DatabaseObject::get_object_list(Cache target_cache, shcore::Value::Array_type_ref list)
{
  for (auto entry : *target_cache)
    list->push_back(entry.second);
}

shcore::Value DatabaseObject::find_in_cache(const std::string& name, Cache target_cache)
{
  Value::Map_type::const_iterator iter = target_cache->find(name);
  if (iter != target_cache->end())
    return Value(boost::shared_ptr<Object_bridge>(iter->second.as_object()));
  else
    return Value();
}