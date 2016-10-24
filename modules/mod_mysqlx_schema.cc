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

#include "mod_mysqlx_schema.h"
#include "mod_mysqlx_session.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"

#include "shellcore/proxy_object.h"

#include "mod_mysqlx_session.h"
#include "mod_mysqlx_table.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include "utils/utils_general.h"
#include "logger/logger.h"
#include "utils/utils_help.h"

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

// Documentation of Schema class
REGISTER_HELP(SCHEMA_INTERACTIVE_BRIEF,"Used to work with database schema objects.");
REGISTER_HELP(SCHEMA_BRIEF, "Represents a Schema as retrived from a session created using the X Protocol.");
REGISTER_HELP(SCHEMA_DETAIL, "<b> View Support </b>");
REGISTER_HELP(SCHEMA_DETAIL1, "MySQL Views are stored queries that when executed produce a result set.");
REGISTER_HELP(SCHEMA_DETAIL2, "MySQL supports the concept of Updatable Views: in specific conditions are met, "\
"Views can be used not only to retrieve data from them but also to update, add and delete records.");
REGISTER_HELP(SCHEMA_DETAIL3, "For the purpose of this API, Views behave similar to a Table, and so they are threated as Tables.");

Schema::Schema(std::shared_ptr<BaseSession> session, const std::string &schema)
  : DatabaseObject(session, std::shared_ptr<DatabaseObject>(), schema), _schema_impl(session->session_obj()->getSchema(schema)) {
  init();
}

Schema::Schema(std::shared_ptr<const BaseSession> session, const std::string &schema) :
DatabaseObject(std::const_pointer_cast<BaseSession>(session), std::shared_ptr<DatabaseObject>(), schema) {
  init();
}

Schema::~Schema() {}

void Schema::init() {
  add_method("getTables", std::bind(&Schema::get_tables, this, _1), NULL);
  add_method("getCollections", std::bind(&Schema::get_collections, this, _1), NULL);

  add_method("getTable", std::bind(&Schema::get_table, this, _1), "name", shcore::String, NULL);
  add_method("getCollection", std::bind(&Schema::get_collection, this, _1), "name", shcore::String, NULL);
  add_method("getCollectionAsTable", std::bind(&Schema::get_collection_as_table, this, _1), "name", shcore::String, NULL);

  add_method("createCollection", std::bind(&Schema::create_collection, this, _1), "name", shcore::String, NULL);

  _tables = Value::new_map().as_map();
  _views = Value::new_map().as_map();
  _collections = Value::new_map().as_map();

  // Setups the cache handlers
  auto table_generator = [this](const std::string& name) {return shcore::Value::wrap<Table>(new Table(shared_from_this(), name, false)); };
  auto view_generator = [this](const std::string& name) {return shcore::Value::wrap<Table>(new Table(shared_from_this(), name, true)); };
  auto collection_generator = [this](const std::string& name) {return shcore::Value::wrap<Collection>(new Collection(shared_from_this(), name)); };

  update_table_cache = [table_generator, this](const std::string &name, bool exists) {DatabaseObject::update_cache(name, table_generator, exists, _tables, this); };
  update_view_cache = [view_generator, this](const std::string &name, bool exists) {DatabaseObject::update_cache(name, view_generator, exists, _views, this); };
  update_collection_cache = [collection_generator, this](const std::string &name, bool exists) {DatabaseObject::update_cache(name, collection_generator, exists, _collections, this); };

  update_full_table_cache = [table_generator, this](const std::vector<std::string> &names) {DatabaseObject::update_cache(names, table_generator, _tables, this); };
  update_full_view_cache = [view_generator, this](const std::vector<std::string> &names) {DatabaseObject::update_cache(names, view_generator, _views, this); };
  update_full_collection_cache = [collection_generator, this](const std::vector<std::string> &names) {DatabaseObject::update_cache(names, collection_generator, _collections, this); };
}

void Schema::update_cache() {
  try {
    std::shared_ptr<BaseSession> sess(std::static_pointer_cast<BaseSession>(_session.lock()));
    if (sess) {
      std::vector<std::string> tables;
      std::vector<std::string> collections;
      std::vector<std::string> views;
      std::vector<std::string> others;

      {
        shcore::Argument_list args;
        args.push_back(Value(_name));
        args.push_back(Value(""));

        Value myres = sess->executeAdminCommand("list_objects", true, args);
        std::shared_ptr<mysh::mysqlx::SqlResult> my_res = myres.as_object<mysh::mysqlx::SqlResult>();

        Value raw_entry;

        while ((raw_entry = my_res->fetch_one(shcore::Argument_list()))) {
          std::shared_ptr<mysh::Row> row = raw_entry.as_object<mysh::Row>();
          std::string object_name = row->get_member("name").as_string();
          std::string object_type = row->get_member("type").as_string();

          if (object_type == "TABLE")
            tables.push_back(object_name);
          else if (object_type == "VIEW")
            views.push_back(object_name);
          else if (object_type == "COLLECTION")
            collections.push_back(object_name);
          else
            others.push_back((boost::format("Unexpected Object Retrieved from Database: %s% of type %s%") % object_name % object_type).str());;
        }

        // Updates the cache
        update_full_table_cache(tables);
        update_full_view_cache(views);
        update_full_collection_cache(collections);

        // Log errors about unexpected object type
        if (others.size()) {
          for (size_t index = 0; index < others.size(); index++)
            log_error("%s", others[index].c_str());
        }
      }
    }
  }
  CATCH_AND_TRANSLATE();
}

void Schema::_remove_object(const std::string& name, const std::string& type) {
  if (type == "View") {
    if (_views->count(name))
      _views->erase(name);
  } else if (type == "Table") {
    if (_tables->count(name))
      _tables->erase(name);
  } else if (type == "Collection") {
    if (_collections->count(name))
      _collections->erase(name);
  }
}

Value Schema::get_member(const std::string &prop) const {
  // Searches prop as  a table
  Value ret_val = find_in_cache(prop, _tables);

  // Searches prop as a collection
  if (!ret_val)
    ret_val = find_in_cache(prop, _collections);

  // Searches prop as a view
  if (!ret_val)
    ret_val = find_in_cache(prop, _views);

  if (!ret_val)
    ret_val = DatabaseObject::get_member(prop);

  return ret_val;
}

// Documentation of getTables function
REGISTER_HELP(SCHEMA_GETTABLES_BRIEF, "Returns a list of Tables for this Schema.");
REGISTER_HELP(SCHEMA_GETTABLES_RETURN, "@return A List containing the Table objects available for the Schema.");
REGISTER_HELP(SCHEMA_GETTABLES_DETAIL, "Pulls from the database the available Tables, Views and Collections.");
REGISTER_HELP(SCHEMA_GETTABLES_DETAIL1, "Does a full refresh of the Tables, Views and Collections cache.");
REGISTER_HELP(SCHEMA_GETTABLES_DETAIL2, "Returns a List of available Table objects.");

/**
* $(SCHEMA_GETTABLES_BRIEF)
*
* \sa Table
*
* $(SCHEMA_GETTABLES_RETURN)
*
* $(SCHEMA_GETTABLES_DETAIL)
*
* $(SCHEMA_GETTABLES_DETAIL1)
*
* $(SCHEMA_GETTABLES_DETAIL2)
*/
#if DOXYGEN_JS
List Schema::getTables() {}
#elif DOXYGEN_PY
list Schema::get_tables() {}
#endif
shcore::Value Schema::get_tables(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("getTables").c_str());

  update_cache();

  shcore::Value::Array_type_ref list(new shcore::Value::Array_type);

  get_object_list(_tables, list);
  get_object_list(_views, list);

  return shcore::Value(list);
}

// Documentation of getCollections function
REGISTER_HELP(SCHEMA_GETCOLLECTIONS_BRIEF, "Returns a list of Collections for this Schema.");
REGISTER_HELP(SCHEMA_GETCOLLECTIONS_RETURN, "@return A List containing the Collection objects available for the Schema.");
REGISTER_HELP(SCHEMA_GETCOLLECTIONS_DETAIL, "Pulls from the database the available Tables, Views and Collections.");
REGISTER_HELP(SCHEMA_GETCOLLECTIONS_DETAIL1, "Does a full refresh of the Tables, Views and Collections cache.");
REGISTER_HELP(SCHEMA_GETCOLLECTIONS_DETAIL2, "Returns a List of available Collection objects.");

/**
* $(SCHEMA_GETCOLLECTIONS_BRIEF)
*
* \sa Collection
*
* $(SCHEMA_GETCOLLECTIONS_RETURN)
*
* $(SCHEMA_GETCOLLECTIONS_DETAIL)
*
* $(SCHEMA_GETCOLLECTIONS_DETAIL1)
*
* $(SCHEMA_GETCOLLECTIONS_DETAIL2)
*/
#if DOXYGEN_JS
List Schema::getCollections() {}
#elif DOXYGEN_PY
list Schema::get_collections() {}
#endif
shcore::Value Schema::get_collections(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("getCollections").c_str());

  update_cache();

  shcore::Value::Array_type_ref list(new shcore::Value::Array_type);

  get_object_list(_collections, list);

  return shcore::Value(list);
}

// Documentation of getTable function
REGISTER_HELP(SCHEMA_GETTABLE_BRIEF, "Returns the Table of the given name for this schema.");
REGISTER_HELP(SCHEMA_GETTABLE_PARAM, "@param name the name of the Table to look for.");
REGISTER_HELP(SCHEMA_GETTABLE_RETURN, "@return the Table object matching the name.");
REGISTER_HELP(SCHEMA_GETTABLE_DETAIL, "Verifies if the requested Table exist on the database, if exists, returns the corresponding Table object.");
REGISTER_HELP(SCHEMA_GETTABLE_DETAIL1, "Updates the Tables cache.");

/**
* $(SCHEMA_GETTABLE_BRIEF)
*
* $(SCHEMA_GETTABLE_PARAM)
*
* $(SCHEMA_GETTABLE_RETURN)
*
* $(SCHEMA_GETTABLE_DETAIL)
*
* $(SCHEMA_GETTABLE_DETAIL1)
*
* \sa Table
*/
#if DOXYGEN_JS
Table Schema::getTable(String name) {}
#elif DOXYGEN_PY
Table Schema::get_table(str name) {}
#endif
shcore::Value Schema::get_table(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getTable").c_str());

  std::string name = args.string_at(0);
  shcore::Value ret_val;

  if (!name.empty()) {
    std::string found_type;
    std::string real_name;

    auto session = _session.lock();
    if (session)
      real_name = session->db_object_exists(found_type, name, _name);
    else
      throw shcore::Exception::logic_error("Unable to get table '" + name + "', no Session available");

    bool exists = false;

    if (!real_name.empty()) {
      if (found_type == "TABLE") {
        exists = true;

        // Updates the cache
        update_table_cache(real_name, exists);

        ret_val = (*_tables)[real_name];
      } else if (found_type == "VIEW") {
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

  return ret_val;
}

// Documentation of getCollection function
REGISTER_HELP(SCHEMA_GETCOLLECTION_BRIEF, "Returns the Collection of the given name for this schema.");
REGISTER_HELP(SCHEMA_GETCOLLECTION_PARAM, "@param name the name of the Collection to look for.");
REGISTER_HELP(SCHEMA_GETCOLLECTION_RETURN, "@return the Collection object matching the name.");
REGISTER_HELP(SCHEMA_GETCOLLECTION_DETAIL, "Verifies if the requested Collection exist on the database, if exists, "\
"returns the corresponding Collection object.");
REGISTER_HELP(SCHEMA_GETCOLLECTION_DETAIL1, "Updates the Collections cache.");

/**
* $(SCHEMA_GETCOLLECTION_BRIEF)
*
* $(SCHEMA_GETCOLLECTION_PARAM)
*
* $(SCHEMA_GETCOLLECTION_RETURN)
*
* $(SCHEMA_GETCOLLECTION_DETAIL)
*
* $(SCHEMA_GETCOLLECTION_DETAIL1)
*
* \sa Collection
*/
#if DOXYGEN_JS
Collection Schema::getCollection(String name) {}
#elif DOXYGEN_PY
Collection Schema::get_collection(str name) {}
#endif
shcore::Value Schema::get_collection(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getCollection").c_str());

  std::string name = args.string_at(0);
  shcore::Value ret_val;

  if (!name.empty()) {
    std::string found_type;
    std::string real_name;

    auto session = _session.lock();
    if (session)
      real_name = session->db_object_exists(found_type, name, _name);
    else
      throw shcore::Exception::logic_error("Unable to retrieve collection '" + name + "', no Session available");

    bool exists = false;
    if (!real_name.empty() && found_type == "COLLECTION")
      exists = true;

    // Updates the cache
    update_collection_cache(real_name, exists);

    if (exists)
      ret_val = (*_collections)[real_name];
    else
      throw shcore::Exception::runtime_error("The collection " + _name + "." + name + " does not exist");
  } else
    throw shcore::Exception::argument_error("An empty name is invalid for a collection");

  return ret_val;
}

// Documentation of getCollectionAsTable function
REGISTER_HELP(SCHEMA_GETCOLLECTIONASTABLE_BRIEF, "Returns a Table object representing a Collection on the database.");
REGISTER_HELP(SCHEMA_GETCOLLECTIONASTABLE_PARAM, "@param name the name of the collection to be retrieved as a table.");
REGISTER_HELP(SCHEMA_GETCOLLECTIONASTABLE_RETURN, "@return the Table object representing the collection or undefined.");

/**
* $(SCHEMA_GETCOLLECTIONASTABLE_BRIEF)
*
* $(SCHEMA_GETCOLLECTIONASTABLE_PARAM)
*
* $(SCHEMA_GETCOLLECTIONASTABLE_RETURN)
*/
#if DOXYGEN_JS
Collection Schema::getCollectionAsTable(String name) {}
#elif DOXYGEN_PY
Collection Schema::get_collection_as_table(str name) {}
#endif
shcore::Value Schema::get_collection_as_table(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getCollectionAsTable").c_str());

  Value ret_val = get_collection(args);

  if (ret_val) {
    std::shared_ptr<Table> table(new Table(shared_from_this(), args.string_at(0)));
    ret_val = Value(std::static_pointer_cast<Object_bridge>(table));
  }

  return ret_val;
}

// Documentation of createCollection function
REGISTER_HELP(SCHEMA_CREATECOLLECTION_BRIEF, "Creates in the current schema a new collection with the specified name and "\
"retrieves an object representing the new collection created.");
REGISTER_HELP(SCHEMA_CREATECOLLECTION_PARAM, "@param name the name of the collection.");
REGISTER_HELP(SCHEMA_CREATECOLLECTION_RETURN, "@return the new created collection.");
REGISTER_HELP(SCHEMA_CREATECOLLECTION_DETAIL, "To specify a name for a collection, follow the naming conventions in MySQL.");

/**
* $(SCHEMA_CREATECOLLECTION_BRIEF)
*
* $(SCHEMA_CREATECOLLECTION_PARAM)
*
* $(SCHEMA_CREATECOLLECTION_RETURN)
*
* $(SCHEMA_CREATECOLLECTION_DETAIL)
*/
#if DOXYGEN_JS
Collection Schema::createCollection(String name) {}
#elif DOXYGEN_PY
Collection Schema::create_collection(str name) {}
#endif
shcore::Value Schema::create_collection(const shcore::Argument_list &args) {
  Value ret_val;

  args.ensure_count(1, get_function_name("createCollection").c_str());

  // Creates the collection on the server
  shcore::Argument_list command_args;
  std::string name = args.string_at(0);
  command_args.push_back(Value(_name));
  command_args.push_back(args[0]);

  std::shared_ptr<BaseSession> sess(std::static_pointer_cast<BaseSession>(_session.lock()));

  if (sess) {
    sess->executeAdminCommand("create_collection", false, command_args);

    // If this is reached it implies all went OK on the previous operation
    update_collection_cache(name, true);
    ret_val = (*_collections)[name];
  } else
    throw shcore::Exception::logic_error("Unable to create collection '" + name + "', no Session available");

  return ret_val;
}
