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

#include "modules/devapi/mod_mysqlx_schema.h"
#include <memory>
#include <string>
#include <vector>
#include <mysqld_error.h>

#include "scripting/lang_base.h"
#include "scripting/object_factory.h"
#include "shellcore/shell_core.h"

#include "scripting/proxy_object.h"

#include "modules/devapi/mod_mysqlx_collection.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/devapi/mod_mysqlx_table.h"

#include "mysqlshdk/libs/utils/logger.h"
#include "shellcore/utils_help.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"
#include "utils/utils_sqlstring.h"


using namespace mysqlsh;
using namespace mysqlsh::mysqlx;
using namespace shcore;

// Documentation of Schema class
REGISTER_HELP(SCHEMA_INTERACTIVE_BRIEF,
              "Used to work with database schema objects.");
REGISTER_HELP(
    SCHEMA_BRIEF,
    "Represents a Schema as retrived from a session created using the X Protocol.");
REGISTER_HELP(SCHEMA_DETAIL, "<b> View Support </b>");
REGISTER_HELP(
    SCHEMA_DETAIL1,
    "MySQL Views are stored queries that when executed produce a result set.");
REGISTER_HELP(
    SCHEMA_DETAIL2,
    "MySQL supports the concept of Updatable Views: in specific conditions are met, "
    "Views can be used not only to retrieve data from them but also to update, add and delete records.");
REGISTER_HELP(
    SCHEMA_DETAIL3,
    "For the purpose of this API, Views behave similar to a Table, and so they are threated as Tables.");

Schema::Schema(std::shared_ptr<Session> session, const std::string &schema)
    : DatabaseObject(session, std::shared_ptr<DatabaseObject>(), schema) {
  init();
}

Schema::~Schema() {}

void Schema::init() {
  add_method("getTables", std::bind(&Schema::get_tables, this, _1), NULL);
  add_method("getCollections", std::bind(&Schema::get_collections, this, _1),
             NULL);

  add_method("getTable", std::bind(&Schema::get_table, this, _1), "name",
             shcore::String, NULL);
  add_method("getCollection", std::bind(&Schema::get_collection, this, _1),
             "name", shcore::String, NULL);
  add_method("getCollectionAsTable",
             std::bind(&Schema::get_collection_as_table, this, _1), "name",
             shcore::String, NULL);

  add_method("createCollection",
             std::bind(&Schema::create_collection, this, _1), "name",
             shcore::String, NULL);

  add_method("dropTable",
             std::bind(&Schema::drop_schema_object, this, _1, "Table"),
             "name", shcore::String, NULL);
  add_method(
      "dropCollection",
      std::bind(&Schema::drop_schema_object, this, _1, "Collection"),
      "name", shcore::String, NULL);
  add_method("dropView",
             std::bind(&Schema::drop_schema_object, this, _1, "View"),
             "name", shcore::String, NULL);

  // Note: If properties are added uncomment this
  // _base_property_count = _properties.size();

  _tables = Value::new_map().as_map();
  _views = Value::new_map().as_map();
  _collections = Value::new_map().as_map();

  // Setups the cache handlers
  auto table_generator = [this](const std::string &name) {
    return shcore::Value::wrap<Table>(
        new Table(shared_from_this(), name, false));
  };
  auto view_generator = [this](const std::string &name) {
    return shcore::Value::wrap<Table>(
        new Table(shared_from_this(), name, true));
  };
  auto collection_generator = [this](const std::string &name) {
    return shcore::Value::wrap<Collection>(
        new Collection(shared_from_this(), name));
  };

  update_table_cache = [table_generator, this](const std::string &name,
                                               bool exists) {
    DatabaseObject::update_cache(name, table_generator, exists, _tables, this);
  };
  update_view_cache = [view_generator, this](const std::string &name,
                                             bool exists) {
    DatabaseObject::update_cache(name, view_generator, exists, _views, this);
  };
  update_collection_cache = [collection_generator, this](
      const std::string &name, bool exists) {
    DatabaseObject::update_cache(name, collection_generator, exists,
                                 _collections, this);
  };

  update_full_table_cache = [table_generator,
                             this](const std::vector<std::string> &names) {
    DatabaseObject::update_cache(names, table_generator, _tables, this);
  };
  update_full_view_cache = [view_generator,
                            this](const std::vector<std::string> &names) {
    DatabaseObject::update_cache(names, view_generator, _views, this);
  };
  update_full_collection_cache = [collection_generator,
                                  this](const std::vector<std::string> &names) {
    DatabaseObject::update_cache(names, collection_generator, _collections,
                                 this);
  };
}

void Schema::update_cache() {
  try {
    std::shared_ptr<Session> sess(
        std::static_pointer_cast<Session>(_session.lock()));
    if (sess) {
      std::vector<std::string> tables;
      std::vector<std::string> collections;
      std::vector<std::string> views;
      std::vector<std::string> others;

      {
        shcore::Dictionary_t args = shcore::make_dict();
        (*args)["schema"] = Value(_name);

        auto result = sess->execute_mysqlx_stmt("list_objects", args);

        while (auto row = result->fetch_one()) {
          std::string object_name = row->get_string(0);
          std::string object_type = row->get_string(1);

          if (object_type == "TABLE")
            tables.push_back(object_name);
          else if (object_type == "VIEW")
            views.push_back(object_name);
          else if (object_type == "COLLECTION")
            collections.push_back(object_name);
          else
            others.push_back(str_format(
                "Unexpected Object Retrieved from Database: %s of type %s",
                object_name.c_str(), object_type.c_str()));
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

void Schema::_remove_object(const std::string &name, const std::string &type) {
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
  Value ret_val;

  // Only checks the cache if the requested member is not a base one
  if (!is_base_member(prop)) {
    // Searches prop as  a table
    ret_val = find_in_cache(prop, _tables);

    // Searches prop as a collection
    if (!ret_val)
      ret_val = find_in_cache(prop, _collections);

    // Searches prop as a view
    if (!ret_val)
      ret_val = find_in_cache(prop, _views);
  }

  if (!ret_val)
    ret_val = DatabaseObject::get_member(prop);

  return ret_val;
}

// Documentation of getTables function
REGISTER_HELP(SCHEMA_GETTABLES_BRIEF,
              "Returns a list of Tables for this Schema.");
REGISTER_HELP(
    SCHEMA_GETTABLES_RETURNS,
    "@returns A List containing the Table objects available for the Schema.");
REGISTER_HELP(
    SCHEMA_GETTABLES_DETAIL,
    "Pulls from the database the available Tables, Views and Collections.");
REGISTER_HELP(
    SCHEMA_GETTABLES_DETAIL1,
    "Does a full refresh of the Tables, Views and Collections cache.");
REGISTER_HELP(SCHEMA_GETTABLES_DETAIL2,
              "Returns a List of available Table objects.");

/**
* $(SCHEMA_GETTABLES_BRIEF)
*
* \sa Table
*
* $(SCHEMA_GETTABLES_RETURNS)
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
REGISTER_HELP(SCHEMA_GETCOLLECTIONS_BRIEF,
              "Returns a list of Collections for this Schema.");
REGISTER_HELP(
    SCHEMA_GETCOLLECTIONS_RETURNS,
    "@returns A List containing the Collection objects available for the Schema.");
REGISTER_HELP(
    SCHEMA_GETCOLLECTIONS_DETAIL,
    "Pulls from the database the available Tables, Views and Collections.");
REGISTER_HELP(
    SCHEMA_GETCOLLECTIONS_DETAIL1,
    "Does a full refresh of the Tables, Views and Collections cache.");
REGISTER_HELP(SCHEMA_GETCOLLECTIONS_DETAIL2,
              "Returns a List of available Collection objects.");

/**
* $(SCHEMA_GETCOLLECTIONS_BRIEF)
*
* \sa Collection
*
* $(SCHEMA_GETCOLLECTIONS_RETURNS)
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
REGISTER_HELP(SCHEMA_GETTABLE_BRIEF,
              "Returns the Table of the given name for this schema.");
REGISTER_HELP(SCHEMA_GETTABLE_PARAM,
              "@param name the name of the Table to look for.");
REGISTER_HELP(SCHEMA_GETTABLE_RETURNS,
              "@returns the Table object matching the name.");
REGISTER_HELP(
    SCHEMA_GETTABLE_DETAIL,
    "Verifies if the requested Table exist on the database, if exists, returns the corresponding Table object.");
REGISTER_HELP(SCHEMA_GETTABLE_DETAIL1, "Updates the Tables cache.");

/**
* $(SCHEMA_GETTABLE_BRIEF)
*
* $(SCHEMA_GETTABLE_PARAM)
*
* $(SCHEMA_GETTABLE_RETURNS)
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

  try {
    if (!name.empty()) {
      std::string found_type;
      std::string real_name;

      auto session = _session.lock();
      if (session)
        real_name = session->db_object_exists(found_type, name, _name);
      else
        throw shcore::Exception::logic_error("Unable to get table '" + name +
                                             "', no Session available");

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
        throw shcore::Exception::runtime_error("The table " + _name + "." +
                                               name + " does not exist");
    } else
      throw shcore::Exception::argument_error(
          "An empty name is invalid for a table");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getTable"));

  return ret_val;
}

// Documentation of getCollection function
REGISTER_HELP(SCHEMA_GETCOLLECTION_BRIEF,
              "Returns the Collection of the given name for this schema.");
REGISTER_HELP(SCHEMA_GETCOLLECTION_PARAM,
              "@param name the name of the Collection to look for.");
REGISTER_HELP(SCHEMA_GETCOLLECTION_RETURNS,
              "@returns the Collection object matching the name.");
REGISTER_HELP(
    SCHEMA_GETCOLLECTION_DETAIL,
    "Verifies if the requested Collection exist on the database, if exists, "
    "returns the corresponding Collection object.");
REGISTER_HELP(SCHEMA_GETCOLLECTION_DETAIL1, "Updates the Collections cache.");

/**
* $(SCHEMA_GETCOLLECTION_BRIEF)
*
* $(SCHEMA_GETCOLLECTION_PARAM)
*
* $(SCHEMA_GETCOLLECTION_RETURNS)
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

  try {
    if (!name.empty()) {
      std::string found_type;
      std::string real_name;

      auto session = _session.lock();
      if (session)
        real_name = session->db_object_exists(found_type, name, _name);
      else
        throw shcore::Exception::logic_error("Unable to retrieve collection '" +
                                             name + "', no Session available");

      bool exists = false;
      if (!real_name.empty() && found_type == "COLLECTION")
        exists = true;

      // Updates the cache
      update_collection_cache(real_name, exists);

      if (exists)
        ret_val = (*_collections)[real_name];
      else
        throw shcore::Exception::runtime_error("The collection " + _name + "." +
                                               name + " does not exist");
    } else
      throw shcore::Exception::argument_error(
          "An empty name is invalid for a collection");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getCollection"));

  return ret_val;
}

// Documentation of getCollectionAsTable function
REGISTER_HELP(
    SCHEMA_GETCOLLECTIONASTABLE_BRIEF,
    "Returns a Table object representing a Collection on the database.");
REGISTER_HELP(
    SCHEMA_GETCOLLECTIONASTABLE_PARAM,
    "@param name the name of the collection to be retrieved as a table.");
REGISTER_HELP(
    SCHEMA_GETCOLLECTIONASTABLE_RETURNS,
    "@returns the Table object representing the collection or undefined.");

/**
* $(SCHEMA_GETCOLLECTIONASTABLE_BRIEF)
*
* $(SCHEMA_GETCOLLECTIONASTABLE_PARAM)
*
* $(SCHEMA_GETCOLLECTIONASTABLE_RETURNS)
*/
#if DOXYGEN_JS
Collection Schema::getCollectionAsTable(String name) {}
#elif DOXYGEN_PY
Collection Schema::get_collection_as_table(str name) {}
#endif
shcore::Value Schema::get_collection_as_table(
    const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getCollectionAsTable").c_str());

  Value ret_val = get_collection(args);

  if (ret_val) {
    std::shared_ptr<Table> table(
        new Table(shared_from_this(), args.string_at(0)));
    ret_val = Value(std::static_pointer_cast<Object_bridge>(table));
  }

  return ret_val;
}

// Documentation of createCollection function
REGISTER_HELP(
    SCHEMA_CREATECOLLECTION_BRIEF,
    "Creates in the current schema a new collection with the specified name and "
    "retrieves an object representing the new collection created.");
REGISTER_HELP(SCHEMA_CREATECOLLECTION_PARAM,
              "@param name the name of the collection.");
REGISTER_HELP(SCHEMA_CREATECOLLECTION_RETURNS,
              "@returns the new created collection.");
REGISTER_HELP(
    SCHEMA_CREATECOLLECTION_DETAIL,
    "To specify a name for a collection, follow the naming conventions in MySQL.");

/**
* $(SCHEMA_CREATECOLLECTION_BRIEF)
*
* $(SCHEMA_CREATECOLLECTION_PARAM)
*
* $(SCHEMA_CREATECOLLECTION_RETURNS)
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
  shcore::Dictionary_t options = shcore::make_dict();
  std::string name = args.string_at(0);
  options->set("schema", Value(_name));
  options->set("name", args[0]);

  std::shared_ptr<Session> sess(
      std::static_pointer_cast<Session>(_session.lock()));

  try {
    if (sess) {
      sess->execute_mysqlx_stmt("create_collection", options);

      // If this is reached it implies all went OK on the previous operation
      update_collection_cache(name, true);
      ret_val = (*_collections)[name];
    } else {
      throw shcore::Exception::logic_error("Unable to create collection '" +
                                           name + "', no Session available");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createCollection"));

  return ret_val;
}

// Documentation of dropTable function
REGISTER_HELP(SCHEMA_DROPTABLE_BRIEF,
              "Drops the specified table.");
REGISTER_HELP(SCHEMA_DROPTABLE_RETURNS,
              "@returns nothing.");

/**
 * $(SCHEMA_DROPTABLE_BRIEF)
 *
 * $(SCHEMA_DROPTABLE_RETURNS)
 */
#if DOXYGEN_JS
Undefined Schema::dropTable(String name) {}
#elif DOXYGEN_PY
None Schema::drop_table(str name) {}
#endif

// Documentation of dropView function
REGISTER_HELP(SCHEMA_DROPVIEW_BRIEF,
              "Drops the specified view");
REGISTER_HELP(SCHEMA_DROPVIEW_RETURNS,
              "@returns nothing.");

/**
 * $(SCHEMA_DROPVIEW_BRIEF)
 *
 * $(SCHEMA_DROPVIEW_RETURNS)
 */
#if DOXYGEN_JS
Undefined Schema::dropView(String name) {}
#elif DOXYGEN_PY
None Schema::drop_view(str name) {}
#endif

// Documentation of dropCollection function
REGISTER_HELP(SCHEMA_DROPCOLLECTION_BRIEF,
              "Drops the specified collection.");
REGISTER_HELP(SCHEMA_DROPCOLLECTION_RETURNS,
              "@returns nothing.");

/**
 * $(SCHEMA_DROPCOLLECTION_BRIEF)
 *
 * $(SCHEMA_DROPCOLLECTION_RETURNS)
 */
#if DOXYGEN_JS
Undefined Schema::dropCollection(String name) {}
#elif DOXYGEN_PY
None Schema::drop_collection(str name) {}
#endif
shcore::Value Schema::drop_schema_object(const shcore::Argument_list &args,
                                              const std::string &type) {
  std::string function = get_function_name("drop" + type);
  args.ensure_count(1, function.c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(
        function + ": Argument #1 is expected to be a string");

  Value schema = this->get_member("name");
  std::string name = args[0].as_string();

  try {
    if (type == "View") {
        std::shared_ptr<Session> sess(
          std::static_pointer_cast<Session>(_session.lock()));
        sess->_execute_sql(sqlstring("drop view if exists !.!", 0) << schema.as_string() << name + "",
                       shcore::Argument_list());
    } else {
      if ((type == "Table") ||
          (type == "Collection")) {
        shcore::Argument_list command_args;
        command_args.push_back(schema);
        command_args.push_back(Value(name));

        std::shared_ptr<Session> sess(
          std::static_pointer_cast<Session>(_session.lock()));
        try {
          sess->executeAdminCommand("drop_collection", false, command_args);
        }
        catch (const mysqlshdk::db::Error e) {
          if (e.code() != ER_BAD_TABLE_ERROR)
            throw;
        }
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("drop" + type));

  this->_remove_object(name, type);

  return shcore::Value();
}

