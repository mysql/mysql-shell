/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/base_database_object.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "mysqlxtest_utils.h"
#include "scripting/common.h"
#include "scripting/lang_base.h"
#include "scripting/object_factory.h"
#include "scripting/proxy_object.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_core.h"
#include "utils/utils_general.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace shcore;

DatabaseObject::DatabaseObject(std::shared_ptr<ShellBaseSession> session,
                               std::shared_ptr<DatabaseObject> schema,
                               const std::string &name)
    : _session(session), _schema(schema), _name(name), _base_property_count(0) {
  init();
}

DatabaseObject::~DatabaseObject() {}

void DatabaseObject::init() {
  add_property("name", "getName");
  add_property("session", "getSession");
  add_property("schema", "getSchema");

  // Hold the number of base properties
  _base_property_count = _properties.size();

  add_method("existsInDatabase",
             std::bind(&DatabaseObject::existsInDatabase, this, _1), "data");
}

std::string &DatabaseObject::append_descr(std::string &s_out,
                                          int UNUSED(indent),
                                          int UNUSED(quote_strings)) const {
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

std::string &DatabaseObject::append_repr(std::string &s_out) const {
  return append_descr(s_out, false);
}

void DatabaseObject::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();

  dumper.append_string("class", class_name());
  dumper.append_string("name", _name);

  dumper.end_object();
}

bool DatabaseObject::operator==(const Object_bridge &other) const {
  if (class_name() == other.class_name()) {
    return _session.lock() == ((DatabaseObject *)&other)->_session.lock() &&
           _schema.lock() == ((DatabaseObject *)&other)->_schema.lock() &&
           _name == ((DatabaseObject *)&other)->_name &&
           class_name() == other.class_name();
  }
  return false;
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the
 * scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this
 * function. The content of the returned value depends on the property being
 * requested. The next list shows the valid properties as well as the returned
 * value for each of them:
 *
 * \li name: returns a String object with the name of this database object.
 * \li schema: returns the schema object that owns this DatabaseObject, so it
 * could be either an instance of Schema or ClassicSchema. If this
 * DatabaseObject is either an instance of Schema or ClassicSchema it returns
 * Null.
 * \li session: returns a session object under which the DatabaseObject was
 * created, it could be any of Session, ClassicSession.
 */
#else
/**
* Returns the name of this database object.
* \return the name as an String object.
*/
#if DOXYGEN_JS
String DatabaseObject::getName() {}
#elif DOXYGEN_PY
str DatabaseObject::get_name() {}
#endif
/**
* Returns the Session object of this database object.
* \return the Session object used to get to this object.
*
* Note that the returned object can be any of:
* - Session: if the object was created/retrieved using an Session instance.
* - ClassicSession: if the object was created/retrieved using an ClassicSession
* instance.
*/
#if DOXYGEN_JS
Object DatabaseObject::getSession() {}
#elif DOXYGEN_PY
object DatabaseObject::get_session() {}
#endif

/**
* Returns the Schema object of this database object.
* \return the object for this schema of this database object.
*
* Note that the returned object can be any of:
* - Schema: if the object was created/retrieved using a Schema instance.
* - ClassicSchema: if the object was created/retrieved using an ClassicSchema
* instance.
*/
#if DOXYGEN_JS
Object DatabaseObject::getSchema() {}
#elif DOXYGEN_PY
object DatabaseObject::get_schema() {}
#endif
#endif
Value DatabaseObject::get_member(const std::string &prop) const {
  Value ret_val;

  if (prop == "name")
    ret_val = Value(_name);
  else if (prop == "session") {
    if (_session.expired())
      ret_val = Value::Null();
    else {
      auto session = _session.lock();
      if (session)
        ret_val = Value(std::static_pointer_cast<Object_bridge>(session));
    }
  } else if (prop == "schema") {
    if (_schema.expired())
      ret_val = Value::Null();
    else {
      auto schema = _schema.lock();

      if (schema)
        ret_val =
            Value(std::static_pointer_cast<Object_bridge>(_schema.lock()));
    }
  } else {
    ret_val = Cpp_object_bridge::get_member(prop);
  }

  return ret_val;
}

//! Verifies if this object exists in the database.
#if DOXYGEN_JS
Bool DatabaseObject::existsInDatabase() {}
#elif DOXYGEN_PY
bool DatabaseObject::exists_in_database() {}
#endif
shcore::Value DatabaseObject::existsInDatabase(
    const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("existsInDatabase").c_str());

  shcore::Value ret_val;
  std::string type(get_object_type());

  auto session = _session.lock();
  auto schema = _schema.lock();

  try {
    if (session)
      ret_val = shcore::Value(
          !session
               ->db_object_exists(
                   type, _name,
                   schema ? schema->get_member("name").as_string() : "")
               .empty());
    else {
      std::string name = _name;
      if (schema)
        name = schema->get_member("name").as_string() + "." + _name;

      throw shcore::Exception::logic_error("Unable to verify existence of '" +
                                           name + "', no Session available");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("existsInDatabase"));

  return ret_val;
}

void DatabaseObject::update_cache(
    const std::vector<std::string> &names,
    const std::function<shcore::Value(const std::string &name)> &generator,
    Cache target_cache, DatabaseObject *target) {
  std::set<std::string> existing;

  // Backups the existing items in the collection
  shcore::Value::Map_type::iterator index, end = target_cache->end();

  for (auto index = target_cache->begin(); index != end; index++)
    existing.insert(index->first);

  // Ensures the existing items are on the cache
  for (auto name : names) {
    if (existing.find(name) == existing.end()) {
      (*target_cache)[name] = generator(name);

      if (target && shcore::is_valid_identifier(name)) {
        // Dynamic properties must keep the name as in the database
        // i.e. Name must not change to match the python naming style
        std::string names = name + "|" + name;
        target->add_property(names);
      }
    }

    else
      existing.erase(name);
  }

  // Removes no longer existing items
  for (auto name : existing) {
    target_cache->erase(name);

    if (target)
      target->delete_property(name);
  }
}

void DatabaseObject::update_cache(
    const std::string &name,
    const std::function<shcore::Value(const std::string &name)> &generator,
    bool exists, Cache target_cache, DatabaseObject *target) {
  if (exists && target_cache->find(name) == target_cache->end()) {
    (*target_cache)[name] = generator(name);

    if (target && shcore::is_valid_identifier(name))
      target->add_property(name);
  }

  if (!exists && target_cache->find(name) != target_cache->end()) {
    target_cache->erase(name);

    if (target)
      target->delete_property(name);
  }
}

void DatabaseObject::get_object_list(Cache target_cache,
                                     shcore::Value::Array_type_ref list) {
  for (auto entry : *target_cache)
    list->push_back(entry.second);
}

shcore::Value DatabaseObject::find_in_cache(const std::string &name,
                                            Cache target_cache) {
  Value::Map_type::const_iterator iter = target_cache->find(name);
  if (iter != target_cache->end())
    return Value(std::shared_ptr<Object_bridge>(iter->second.as_object()));
  else
    return Value();
}

bool DatabaseObject::is_base_member(const std::string &prop) const {
  auto style = naming_style;
  if (has_method_advanced(prop, style))
    return true;
  auto prop_index = std::find_if(
      _properties.begin(), _properties.begin() + (_base_property_count - 1),
      [prop, style](const Cpp_property_name &p) {
        return p.name(style) == prop;
      });

  return (prop_index != _properties.begin() + (_base_property_count - 1));
}
