/*
 * Copyright (c) 2014, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/devapi/base_database_object.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"

using namespace mysqlsh;
using namespace shcore;

REGISTER_HELP_CLASS(DatabaseObject, mysqlx);
REGISTER_HELP(DATABASEOBJECT_BRIEF,
              "Provides base functionality for database objects.");

DatabaseObject::DatabaseObject(std::shared_ptr<mysqlx::Session> session,
                               std::shared_ptr<DatabaseObject> schema,
                               const std::string &name)
    : _session(session), _schema(schema), _name(name), _base_property_count(0) {
  init();
}

DatabaseObject::~DatabaseObject() = default;

void DatabaseObject::init() {
  add_property("name", "getName");
  add_property("session", "getSession");
  add_property("schema", "getSchema");

  // Hold the number of base properties
  _base_property_count = _properties.size();

  add_method(
      "existsInDatabase",
      std::bind(&DatabaseObject::existsInDatabase, this, std::placeholders::_1),
      "data");
}

std::string &DatabaseObject::append_descr(
    std::string &s_out, [[maybe_unused]] int indent,
    [[maybe_unused]] int quote_strings) const {
  s_out.append(1, '<')
      .append(class_name())
      .append(1, ':')
      .append(_name)
      .append(1, '>');
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

REGISTER_HELP_PROPERTY(name, DatabaseObject);
REGISTER_HELP(DATABASEOBJECT_NAME_BRIEF, "The name of this database object.");
REGISTER_HELP_FUNCTION(getName, DatabaseObject);
REGISTER_HELP(DATABASEOBJECT_GETNAME_BRIEF,
              "Returns the name of this database object.");
REGISTER_HELP(DATABASEOBJECT_GETNAME_RETURNS,
              "@returns The name this database object.");

/**
 * $(DATABASEOBJECT_GETNAME_BRIEF)
 * $(DATABASEOBJECT_GETNAME_RETURNS)
 */
#if DOXYGEN_JS
String DatabaseObject::getName() {}
#elif DOXYGEN_PY
str DatabaseObject::get_name() {}
#endif

REGISTER_HELP_PROPERTY(session, DatabaseObject);
REGISTER_HELP(DATABASEOBJECT_SESSION_BRIEF,
              "The Session object of this database object.");
REGISTER_HELP_FUNCTION(getSession, DatabaseObject);
REGISTER_HELP(DATABASEOBJECT_GETSESSION_BRIEF,
              "Returns the Session object of this database object.");
REGISTER_HELP(DATABASEOBJECT_GETSESSION_RETURNS,
              "@returns The Session object used to get to this object.");
REGISTER_HELP(DATABASEOBJECT_GETSESSION_DETAIL,
              "Note that the returned object can be any of:");
REGISTER_HELP(DATABASEOBJECT_GETSESSION_DETAIL1,
              "@li Session: if the object was created/retrieved using an "
              "Session instance.");
REGISTER_HELP(DATABASEOBJECT_GETSESSION_DETAIL2,
              "@li ClassicSession: if the object was created/retrieved using "
              "an ClassicSession.");

/**
 * $(DATABASEOBJECT_GETSESSION_BRIEF)
 * $(DATABASEOBJECT_GETSESSION_RETURNS)
 *
 * $(DATABASEOBJECT_GETSESSION_DETAIL)
 * $(DATABASEOBJECT_GETSESSION_DETAIL1)
 * $(DATABASEOBJECT_GETSESSION_DETAIL2)
 */
#if DOXYGEN_JS
Object DatabaseObject::getSession() {}
#elif DOXYGEN_PY
object DatabaseObject::get_session() {}
#endif

REGISTER_HELP_PROPERTY(schema, DatabaseObject);
REGISTER_HELP(DATABASEOBJECT_SCHEMA_BRIEF,
              "The Schema object of this database object.");
REGISTER_HELP_FUNCTION(getSchema, DatabaseObject);
REGISTER_HELP(DATABASEOBJECT_GETSCHEMA_BRIEF,
              "Returns the Schema object of this database object.");
REGISTER_HELP(DATABASEOBJECT_GETSCHEMA_RETURNS,
              "@returns The Schema object used to get to this object.");
REGISTER_HELP(DATABASEOBJECT_GETSCHEMA_DETAIL,
              "Note that the returned object can be any of:");
REGISTER_HELP(
    DATABASEOBJECT_GETSCHEMA_DETAIL1,
    "@li Schema: if the object was created/retrieved using a Schema instance.");
REGISTER_HELP(DATABASEOBJECT_GETSCHEMA_DETAIL2,
              "@li ClassicSchema: if the object was created/retrieved using an "
              "ClassicSchema.");
REGISTER_HELP(
    DATABASEOBJECT_GETSCHEMA_DETAIL3,
    "@li Null: if this database object is a Schema or ClassicSchema.");

/**
 * $(DATABASEOBJECT_GETSCHEMA_BRIEF)
 * $(DATABASEOBJECT_GETSCHEMA_RETURNS)
 *
 * $(DATABASEOBJECT_GETSCHEMA_DETAIL)
 * $(DATABASEOBJECT_GETSCHEMA_DETAIL1)
 * $(DATABASEOBJECT_GETSCHEMA_DETAIL2)
 * $(DATABASEOBJECT_GETSCHEMA_DETAIL3)
 */
#if DOXYGEN_JS
Object DatabaseObject::getSchema() {}
#elif DOXYGEN_PY
object DatabaseObject::get_schema() {}
#endif

Value DatabaseObject::get_member(const std::string &prop) const {
  if (prop == "name") {
    return Value(_name);
  }

  if (prop == "session") {
    if (!_session.expired()) {
      if (auto session = _session.lock(); session) return Value(session);
    }
    return Value::Null();
  }

  if (prop == "schema") {
    if (!_schema.expired()) {
      if (auto schema = _schema.lock(); schema) return Value(schema);
    }
    return Value::Null();
  }

  return Cpp_object_bridge::get_member(prop);
}

REGISTER_HELP_FUNCTION(existsInDatabase, DatabaseObject);
REGISTER_HELP(DATABASEOBJECT_EXISTSINDATABASE_BRIEF,
              "Verifies if this object exists in the database.");
REGISTER_HELP(DATABASEOBJECT_EXISTSINDATABASE_RETURNS,
              "@returns A boolean indicating if the object still exists on the "
              "database.");
/**
 * $(DATABASEOBJECT_EXISTSINDATABASE_BRIEF)
 * $(DATABASEOBJECT_EXISTSINDATABASE_RETURNS)
 */
#if DOXYGEN_JS
Bool DatabaseObject::existsInDatabase() {}
#elif DOXYGEN_PY
bool DatabaseObject::exists_in_database() {}
#endif
shcore::Value DatabaseObject::existsInDatabase(
    const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("existsInDatabase").c_str());

  auto session = _session.lock();
  auto schema = _schema.lock();

  try {
    if (session) {
      auto type = get_object_type();
      return shcore::Value(
          !session
               ->db_object_exists(
                   type, _name,
                   schema ? schema->get_member("name").get_string() : "")
               .empty());
    }

    auto name =
        !schema ? _name
                : shcore::str_format(
                      "%s.%s", schema->get_member("name").get_string().c_str(),
                      _name.c_str());

    throw shcore::Exception::logic_error(shcore::str_format(
        "Unable to verify existence of '%s', no Session available",
        name.c_str()));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("existsInDatabase"));
}

void DatabaseObject::update_cache(
    const std::vector<std::string> &names,
    const std::function<shcore::Value(const std::string &name)> &generator,
    Cache target_cache, DatabaseObject *target) {
  // Backups the existing items in the collection
  std::set<std::string> existing;
  for (const auto &index : *target_cache) {
    existing.insert(index.first);
  }

  // Ensures the existing items are on the cache
  for (const auto &name : names) {
    if (existing.find(name) == existing.end()) {
      (*target_cache)[name] = generator(name);

      if (target && shcore::is_valid_identifier(name)) {
        // Dynamic properties must keep the name as in the database
        // i.e. Name must not change to match the python naming style
        target->add_property(name + "|" + name);
      }
    } else
      existing.erase(name);
  }

  // Removes no longer existing items
  for (const auto &name : existing) {
    target_cache->erase(name);

    if (target) target->delete_property(name);
  }
}

void DatabaseObject::update_cache(
    const std::string &name,
    const std::function<shcore::Value(const std::string &name)> &generator,
    bool exists, Cache target_cache, DatabaseObject *target) {
  if (exists && target_cache->find(name) == target_cache->end()) {
    (*target_cache)[name] = generator(name);

    if (target && shcore::is_valid_identifier(name)) target->add_property(name);
  }

  if (!exists && target_cache->find(name) != target_cache->end()) {
    target_cache->erase(name);

    if (target) target->delete_property(name);
  }
}

void DatabaseObject::flush_cache(Cache target_cache, DatabaseObject *target) {
  if (target) {
    for (const auto &iter : *target_cache) {
      target->delete_property(iter.first);
    }
  }
  target_cache->clear();
}

void DatabaseObject::get_object_list(Cache target_cache,
                                     shcore::Value::Array_type_ref list) {
  for (const auto &entry : *target_cache) list->push_back(entry.second);
}

shcore::Value DatabaseObject::find_in_cache(const std::string &name,
                                            Cache target_cache) {
  if (auto iter = target_cache->find(name); iter != target_cache->end())
    return Value(std::shared_ptr<Object_bridge>(iter->second.as_object()));

  return Value();
}

bool DatabaseObject::is_base_member(const std::string &prop) const {
  if (has_method_advanced(prop)) return true;
  auto prop_index = std::find_if(
      _properties.begin(), _properties.begin() + (_base_property_count - 1),
      [prop](const Cpp_property_name &p) {
        return p.name(shcore::current_naming_style()) == prop;
      });

  return (prop_index != _properties.begin() + (_base_property_count - 1));
}

uint64_t DatabaseObject::count() {
  // We make sure count is being called in an object that makes sense
  // ATM only table and collection support this function
  assert(has_count());

  if (auto session_wrapper = session(); session_wrapper) {
    auto query = "select count(*) from !.!"_sql << schema()->name() << name();

    auto session = session_wrapper->get_core_session();
    auto result = session->query(query.str(), true);
    return result->fetch_one()->get_uint(0);
  }

  throw shcore::Exception::logic_error("No session available.");
}
