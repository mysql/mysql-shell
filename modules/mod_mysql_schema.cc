/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "utils/utils_sqlstring.h"
#include "mod_mysql_schema.h"
#include "mod_mysql_table.h"
#include "mod_mysql_session.h"
#include "mod_mysql_resultset.h"

#include "scripting/object_factory.h"
#include "shellcore/shell_core.h"
#include "scripting/lang_base.h"

#include "scripting/proxy_object.h"

#include "utils/utils_general.h"
#include "utils/utils_string.h"
#include "logger/logger.h"
#include "shellcore/utils_help.h"
#include "mysqlxtest_utils.h"

using namespace mysqlsh::mysql;
using namespace shcore;

// Documentation of the ClassicSchema class
REGISTER_HELP(CLASSICSCHEMA_INTERACTIVE_BRIEF, "Used to work with database schema objects.");
REGISTER_HELP(CLASSICSCHEMA_BRIEF, "Represents a Schema retrieved with a session created using the MySQL Protocol.");
REGISTER_HELP(CLASSICSCHEMA_DETAIL, "<b>Dynamic Properties</b>");
REGISTER_HELP(CLASSICSCHEMA_DETAIL1, "In addition to the properties documented above, when a schema object is retrieved from the session, "\
"its Tables are loaded from the database and a cache is filled with the corresponding objects");
REGISTER_HELP(CLASSICSCHEMA_DETAIL2, "This cache is used to allow the user accessing the Tables as Schema properties. "\
"These Dynamic Properties are named as the object name, so, if a Schema has a table named *customers* this table "\
"can be accessed in two forms:");
REGISTER_HELP(CLASSICSCHEMA_DETAIL3, "Note that dynamic properties for Tables are available only if the next conditions are met:");
REGISTER_HELP(CLASSICSCHEMA_DETAIL4, "@li The object name is a valid identifier.");
REGISTER_HELP(CLASSICSCHEMA_DETAIL5, "@li The object name is different from any member of the ClassicSchema class.");
REGISTER_HELP(CLASSICSCHEMA_DETAIL6, "@li The object is in the cache.");
REGISTER_HELP(CLASSICSCHEMA_DETAIL7, "The object cache is updated every time getTables() is called.");
REGISTER_HELP(CLASSICSCHEMA_DETAIL8, "To retrieve an object that is not available through a Dynamic Property use getTable(name).");
REGISTER_HELP(CLASSICSCHEMA_DETAIL9, "<b>View Support</b>");
REGISTER_HELP(CLASSICSCHEMA_DETAIL10, "MySQL Views are stored queries that when executed produce a result set.");
REGISTER_HELP(CLASSICSCHEMA_DETAIL11, "MySQL supports the concept of Updatable Views: in specific conditions are met, "\
"Views can be used not only to retrieve data from them but also to update, add and delete records.");
REGISTER_HELP(CLASSICSCHEMA_DETAIL12, "For the purpose of this API, Views behave similar to a Table, and so they are threated as Tables.");

ClassicSchema::ClassicSchema(std::shared_ptr<ClassicSession> session, const std::string &schema)
  : DatabaseObject(std::dynamic_pointer_cast<ShellBaseSession>(session), std::shared_ptr<DatabaseObject>(), schema) {
  init();
}

ClassicSchema::ClassicSchema(std::shared_ptr<const ClassicSession> session, const std::string &schema) :
DatabaseObject(std::const_pointer_cast<ClassicSession>(session), std::shared_ptr<DatabaseObject>(), schema) {
  init();
}

void ClassicSchema::init() {
  add_method("getTables", std::bind(&ClassicSchema::get_tables, this, _1), NULL);
  add_method("getTable", std::bind(&ClassicSchema::get_table, this, _1), "name", shcore::String, NULL);

  // Note: If properties are added uncomment this
  //_base_property_count = _properties.size();

  _tables = Value::new_map().as_map();
  _views = Value::new_map().as_map();

  // Setups the cache handlers
  auto table_generator = [this](const std::string& name) {return shcore::Value::wrap<ClassicTable>(new ClassicTable(shared_from_this(), name, false)); };
  auto view_generator = [this](const std::string& name) {return shcore::Value::wrap<ClassicTable>(new ClassicTable(shared_from_this(), name, true)); };

  update_table_cache = [table_generator, this](const std::string &name, bool exists) {DatabaseObject::update_cache(name, table_generator, exists, _tables, this); };
  update_view_cache = [view_generator, this](const std::string &name, bool exists) {DatabaseObject::update_cache(name, view_generator, exists, _views, this); };

  update_full_table_cache = [table_generator, this](const std::vector<std::string> &names) {DatabaseObject::update_cache(names, table_generator, _tables, this); };
  update_full_view_cache = [view_generator, this](const std::vector<std::string> &names) {DatabaseObject::update_cache(names, view_generator, _views, this); };
}

ClassicSchema::~ClassicSchema() {}

void ClassicSchema::update_cache() {
  std::shared_ptr<ClassicSession> sess(std::dynamic_pointer_cast<ClassicSession>(_session.lock()));
  if (sess) {
    std::vector<std::string> tables;
    std::vector<std::string> views;
    std::vector<std::string> others;

    auto val_result = sess->execute_sql(sqlstring("show full tables in !", 0) << _name, shcore::Argument_list());
    auto result = val_result.as_object<ClassicResult>();
    auto val_row = result->fetch_one(shcore::Argument_list());

    if (val_row) {
      auto row = val_row.as_object<mysqlsh::Row>();
      while (row) {
        std::string object_name = row->get_member(0).as_string();
        std::string object_type = row->get_member(1).as_string();

        if (object_type == "BASE TABLE" || object_type == "LOCAL TEMPORARY")
          tables.push_back(object_name);
        else if (object_type == "VIEW" || object_type == "SYSTEM VIEW")
          views.push_back(object_name);
        else
          others.push_back(str_format("Unexpected Object Retrieved from Database: %s of type %s", object_name.c_str(), object_type.c_str()));

        row.reset();
        val_row = result->fetch_one(shcore::Argument_list());

        if (val_row)
          row = val_row.as_object<mysqlsh::Row>();
      }
    }

    // Updates the cache
    update_full_table_cache(tables);
    update_full_view_cache(views);

    // Log errors about unexpected object type
    if (others.size()) {
      for (size_t index = 0; index < others.size(); index++)
        log_error("%s", others[index].c_str());
    }
  }
}

void ClassicSchema::_remove_object(const std::string& name, const std::string& type) {
  if (type == "View") {
    if (_views->count(name))
      _views->erase(name);
  } else if (type == "Table") {
    if (_tables->count(name))
      _tables->erase(name);
  }
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function. The the content of the returned value depends on the property being requested. The next list shows the valid properties as well as the returned value for each of them:
 *
 * The Schema tables and views are exposed as members of the Schema object so:
 *
 * \li If prop is the name of a valid Table or View on the Schema, the corresponding ClassicTable object will be returned.
 *
 * See the implementation of DatabaseObject for additional valid members.
 */
#endif
Value ClassicSchema::get_member(const std::string &prop) const {
  Value ret_val;

  // Only checks the cache if the requested member is not a base one
  if (!is_base_member(prop)) {
    // Searches the property in tables
    ret_val = find_in_cache(prop, _tables);

    // Search the property in views
    if (!ret_val)
      ret_val = find_in_cache(prop, _views);
  }

  // Search the rest of the properties
  if (!ret_val)
    ret_val = DatabaseObject::get_member(prop);

  return ret_val;
}

// Documentation of the getTable function
REGISTER_HELP(CLASSICSCHEMA_GETTABLE_BRIEF, "Returns the table of the given name for this schema.");
REGISTER_HELP(CLASSICSCHEMA_GETTABLE_PARAM, "@param name the name of the table to look for.");
REGISTER_HELP(CLASSICSCHEMA_GETTABLE_RETURNS, "@returns the ClassicTable object matching the name.");
REGISTER_HELP(CLASSICSCHEMA_GETTABLE_DETAIL, "Verifies if the requested Table exist on the database, "\
"if exists, returns the corresponding ClassicTable object.");
REGISTER_HELP(CLASSICSCHEMA_GETTABLE_DETAIL1, "Updates the Tables cache.");

//! $(CLASSICSCHEMA_GETTABLE_BRIEF)
#if DOXYGEN_CPP
//! \param args should contain the name of the table to look for.
#else
//! $(CLASSICSCHEMA_GETTABLE_PARAM)
#endif
/**
* $(CLASSICSCHEMA_GETTABLE_RETURNS)
*
* $(CLASSICSCHEMA_GETTABLE_DETAIL)
*
* $(CLASSICSCHEMA_GETTABLE_DETAIL1)
* \sa ClassicTable
*/
#if DOXYGEN_JS
ClassicTable ClassicSchema::getTable(String name) {}
#elif DOXYGEN_PY
ClassicTable ClassicSchema::get_table(str name) {}
#endif
shcore::Value ClassicSchema::get_table(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getTable").c_str());
  std::string name = args.string_at(0);
  shcore::Value ret_val;

  try {
    if (!name.empty()) {
      std::string found_type;
      auto session = _session.lock();
      std::string real_name;
      if (session)
        real_name = session->db_object_exists(found_type, name, _name);
      else
        throw shcore::Exception::logic_error("Unable to retrieve table '" + name + "', no Session available");

      bool exists = false;
      if (!real_name.empty()) {
        if (found_type == "BASE TABLE" || found_type == "LOCAL TEMPORARY") {
          exists = true;

          // Updates the cache
          update_table_cache(real_name, exists);

          ret_val = (*_tables)[real_name];
        } else if (found_type == "VIEW" || found_type == "SYSTEM VIEW") {
          exists = true;

          // Updates the cache
          update_view_cache(real_name, exists);

          ret_val = (*_views)[real_name];
        }
      }

      if (!exists)
        throw shcore::Exception::runtime_error("The table " + _name + "." + name + " does not exist");
    } else
      throw shcore::Exception::argument_error("An empty name is invalid for a table");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getTable"));

  return ret_val;
}

// Documentation of the getTables function
REGISTER_HELP(CLASSICSCHEMA_GETTABLES_BRIEF, "Returns a list of Tables for this Schema.");
REGISTER_HELP(CLASSICSCHEMA_GETTABLES_RETURNS, "@returns A List containing the Table objects available for the Schema.");
REGISTER_HELP(CLASSICSCHEMA_GETTABLES_DETAIL, "Pulls from the database the available Tables and Views.");
REGISTER_HELP(CLASSICSCHEMA_GETTABLES_DETAIL1, "Does a full refresh of the Tables and Views cache.");
REGISTER_HELP(CLASSICSCHEMA_GETTABLES_DETAIL2, "Returns a List of available Table objects.");

/**
* $(CLASSICSCHEMA_GETTABLES_BRIEF)
* \sa ClassicTable
* $(CLASSICSCHEMA_GETTABLES_RETURNS)
*
* $(CLASSICSCHEMA_GETTABLES_DETAIL)
*
* $(CLASSICSCHEMA_GETTABLES_DETAIL1)
*
* $(CLASSICSCHEMA_GETTABLES_DETAIL2)
*/
#if DOXYGEN_JS
List ClassicSchema::getTables() {}
#elif DOXYGEN_PY
list ClassicSchema::get_tables() {}
#endif
shcore::Value ClassicSchema::get_tables(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("getTables").c_str());

  update_cache();

  shcore::Value::Array_type_ref list(new shcore::Value::Array_type);

  get_object_list(_tables, list);
  get_object_list(_views, list);

  return shcore::Value(list);
}
