/*
 * Copyright (c) 2014, 2022, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/devapi/mod_mysqlx_schema.h"
#include <mysqld_error.h>
#include <memory>
#include <string>
#include <vector>

#include "scripting/lang_base.h"
#include "scripting/object_factory.h"
#include "shellcore/base_shell.h"
#include "shellcore/shell_core.h"

#include "scripting/proxy_object.h"

#include "modules/devapi/mod_mysqlx_collection.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/devapi/mod_mysqlx_table.h"

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/db/mysqlx/xpl_error.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "shellcore/utils_help.h"
#include "utils/utils_general.h"
#include "utils/utils_sqlstring.h"
#include "utils/utils_string.h"

using namespace mysqlsh;
using namespace mysqlsh::mysqlx;
using namespace shcore;

// Documentation of Schema class
REGISTER_HELP_SUB_CLASS(Schema, mysqlx, DatabaseObject);
REGISTER_HELP(SCHEMA_GLOBAL_BRIEF,
              "Used to work with database schema objects.");
REGISTER_HELP(SCHEMA_BRIEF,
              "Represents a Schema as retrieved from a session created using "
              "the X Protocol.");
REGISTER_HELP(SCHEMA_DETAIL, "<b>View Support</b>");
REGISTER_HELP(
    SCHEMA_DETAIL1,
    "MySQL Views are stored queries that when executed produce a result set.");
REGISTER_HELP(SCHEMA_DETAIL2,
              "MySQL supports the concept of Updatable Views: in specific "
              "conditions are met, "
              "Views can be used not only to retrieve data from them but also "
              "to update, add and delete records.");
REGISTER_HELP(SCHEMA_DETAIL3,
              "For the purpose of this API, Views behave similar to a Table, "
              "and so they are treated as Tables.");
REGISTER_HELP(SCHEMA_DETAIL4, "<b>Tables and Collections as Properties</b>");
REGISTER_HELP(SCHEMA_DETAIL5, "${DYNAMIC_PROPERTIES}");

REGISTER_HELP_TOPIC(Dynamic Properties, TOPIC, DYNAMIC_PROPERTIES, Schema,
                    SCRIPTING);
REGISTER_HELP_TOPIC_TEXT(DYNAMIC_PROPERTIES, R"*(
A Schema object may expose tables and collections as properties, this way they
can be accessed as:

@li schema.@<collection_name@>
@li schema.@<table_name@>

This handy way of accessing tables and collections is available if they met the
following conditions:

@li They existed at the moment the Schema object was retrieved from the session.
@li The name is a valid identifier.
@li The name is different from any other property or function on the Schema
object. 

If any of the conditions is not met, the way to access the table or collection
is by using the standard DevAPI functions:
@li schema.<<<getTable>>>(@<name@>)
@li schema.<<<getCollection>>>(@<name@>)
)*");
REGISTER_HELP(SCHEMA_PROPERTIES_CLOSING_DESC,
              "Some tables and collections are also exposed as properties of "
              "the Schema object. For details look at 'Tables and Collections "
              "as Properties' on the DETAILS section.");

Schema::Schema(std::shared_ptr<Session> session, const std::string &schema)
    : DatabaseObject(session, std::shared_ptr<DatabaseObject>(), schema) {
  init();
}

Schema::~Schema() {}

void Schema::init() {
  add_method("getTables", std::bind(&Schema::get_tables, this, _1));
  add_method("getCollections", std::bind(&Schema::get_collections, this, _1));
  add_method("getTable", std::bind(&Schema::get_table, this, _1), "name",
             shcore::String);
  add_method("getCollection", std::bind(&Schema::get_collection, this, _1),
             "name", shcore::String);
  add_method("getCollectionAsTable",
             std::bind(&Schema::get_collection_as_table, this, _1), "name",
             shcore::String);

  expose("createCollection", &Schema::create_collection, "name", "?options");
  expose("modifyCollection", &Schema::modify_collection, "name", "options");
  add_method("dropCollection",
             std::bind(&Schema::drop_schema_object, this, _1, "Collection"),
             "name", shcore::String);

  // Note: If properties are added uncomment this
  // _base_property_count = _properties.size();

  _tables = Value::new_map().as_map();
  _views = Value::new_map().as_map();
  _collections = Value::new_map().as_map();

  // Setups the cache handlers
  auto table_generator = [this](const std::string &name) {
    return shcore::Value::wrap<Table>(
        std::make_shared<Table>(shared_from_this(), name, false));
  };
  auto view_generator = [this](const std::string &name) {
    return shcore::Value::wrap<Table>(
        std::make_shared<Table>(shared_from_this(), name, true));
  };
  auto collection_generator = [this](const std::string &name) {
    return shcore::Value::wrap<Collection>(
        std::make_shared<Collection>(shared_from_this(), name));
  };

  update_table_cache = [table_generator, this](const std::string &name,
                                               bool exists) {
    DatabaseObject::update_cache(name, table_generator, exists, _tables,
                                 use_object_handles() ? this : nullptr);
  };
  update_view_cache = [view_generator, this](const std::string &name,
                                             bool exists) {
    DatabaseObject::update_cache(name, view_generator, exists, _views,
                                 use_object_handles() ? this : nullptr);
  };
  update_collection_cache = [collection_generator, this](
                                const std::string &name, bool exists) {
    DatabaseObject::update_cache(name, collection_generator, exists,
                                 _collections,
                                 use_object_handles() ? this : nullptr);
  };

  update_full_table_cache = [table_generator,
                             this](const std::vector<std::string> &names) {
    _using_object_handles = true;
    DatabaseObject::update_cache(names, table_generator, _tables, this);
  };
  update_full_view_cache = [view_generator,
                            this](const std::vector<std::string> &names) {
    _using_object_handles = true;
    DatabaseObject::update_cache(names, view_generator, _views, this);
  };
  update_full_collection_cache = [collection_generator,
                                  this](const std::vector<std::string> &names) {
    _using_object_handles = true;
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
    if (_views->count(name)) _views->erase(name);
  } else if (type == "Table") {
    if (_tables->count(name)) _tables->erase(name);
  } else if (type == "Collection") {
    if (_collections->count(name)) _collections->erase(name);
  }
}

bool Schema::use_object_handles() const {
  return current_shell_options()->get().devapi_schema_object_handles;
}

std::vector<std::string> Schema::get_members() const {
  // Flush the cache if we have it populated but it got disabled
  if (!use_object_handles() && _using_object_handles) {
    _using_object_handles = false;
    flush_cache(_views, const_cast<Schema *>(this));
    flush_cache(_tables, const_cast<Schema *>(this));
    flush_cache(_collections, const_cast<Schema *>(this));
  }
  return DatabaseObject::get_members();
}

Value Schema::get_member(const std::string &prop) const {
  Value ret_val;

  // Only checks the cache if the requested member is not a base one
  if (!is_base_member(prop) && use_object_handles()) {
    // Searches prop as  a table
    ret_val = find_in_cache(prop, _tables);

    // Searches prop as a collection
    if (!ret_val) ret_val = find_in_cache(prop, _collections);

    // Searches prop as a view
    if (!ret_val) ret_val = find_in_cache(prop, _views);
  }

  if (!ret_val) {
    if (prop == "schema")
      ret_val = Value(std::const_pointer_cast<Schema>(shared_from_this()));
    else
      ret_val = DatabaseObject::get_member(prop);
  }

  return ret_val;
}

// Documentation of getTables function
REGISTER_HELP_FUNCTION(getTables, Schema);
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

  if (use_object_handles()) update_cache();

  shcore::Value::Array_type_ref list(new shcore::Value::Array_type);

  get_object_list(_tables, list);
  get_object_list(_views, list);

  return shcore::Value(list);
}

// Documentation of getCollections function
REGISTER_HELP_FUNCTION(getCollections, Schema);
REGISTER_HELP(SCHEMA_GETCOLLECTIONS_BRIEF,
              "Returns a list of Collections for this Schema.");
REGISTER_HELP(SCHEMA_GETCOLLECTIONS_RETURNS,
              "@returns A List containing the Collection objects available for "
              "the Schema.");
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

  if (use_object_handles()) update_cache();

  shcore::Value::Array_type_ref list(new shcore::Value::Array_type);

  get_object_list(_collections, list);

  return shcore::Value(list);
}

// Documentation of getTable function
REGISTER_HELP_FUNCTION(getTable, Schema);
REGISTER_HELP(SCHEMA_GETTABLE_BRIEF,
              "Returns the Table of the given name for this schema.");
REGISTER_HELP(SCHEMA_GETTABLE_PARAM,
              "@param name the name of the Table to look for.");
REGISTER_HELP(SCHEMA_GETTABLE_RETURNS,
              "@returns the Table object matching the name.");
REGISTER_HELP(SCHEMA_GETTABLE_DETAIL,
              "Verifies if the requested Table exist on the database, if "
              "exists, returns the corresponding Table object.");
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
REGISTER_HELP_FUNCTION(getCollection, Schema);
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
      if (!real_name.empty() && found_type == "COLLECTION") exists = true;

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
REGISTER_HELP_FUNCTION(getCollectionAsTable, Schema);
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
REGISTER_HELP_FUNCTION(createCollection, Schema);
REGISTER_HELP_FUNCTION_TEXT(SCHEMA_CREATECOLLECTION, R"*(
Creates in the current schema a new collection with the specified name and
retrieves an object representing the new collection created.

@param name the name of the collection.
@param options optional dictionary with options.

@returns the new created collection.

To specify a name for a collection, follow the naming conventions in MySQL.

The options dictionary may contain the following attributes:

@li reuseExistingObject: boolean, indicating if the call should succeed when
collection with the same name already exists.
@li validation: object defining JSON schema validation for the collection.

The validation object allows the following attributes:

@li level: which can be either 'strict' or 'off'.
@li schema: a JSON schema specification as described at json-schema.org. 
)*");

/**
 * $(SCHEMA_CREATECOLLECTION_BRIEF)
 *
 * $(SCHEMA_CREATECOLLECTION)
 */
#if DOXYGEN_JS
Collection Schema::createCollection(String name, Dictionary options) {}
#elif DOXYGEN_PY
Collection Schema::create_collection(str name, dict options) {}
#endif
shcore::Value Schema::create_collection(const std::string &name,
                                        const shcore::Dictionary_t &opts) {
  Value ret_val;

  // Creates the collection on the server
  shcore::Dictionary_t options = shcore::make_dict();
  options->set("schema", Value(_name));
  options->set("name", Value(name));
  if (opts && !opts->empty()) {
    shcore::Dictionary_t validation;
    mysqlshdk::utils::nullable<bool> reuse;
    shcore::Option_unpacker{opts}
        .optional("validation", &validation)
        .optional("reuseExistingObject", &reuse)
        .end("in create collection options dictionary");

    if (reuse) {
      opts->erase("reuseExistingObject");
      opts->emplace("reuse_existing", *reuse);
    }

    options->set("options", Value(opts));
  }

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
  } catch (const mysqlshdk::db::Error &e) {
    if (opts && !opts->empty()) {
      std::string err_msg;
      switch (e.code()) {
        case ER_X_CMD_NUM_ARGUMENTS:
          if (strstr(e.what(), "Invalid number of arguments") != nullptr)
            err_msg =
                "The target MySQL server does not support options in the "
                "<<<createCollection>>> function";
          break;
        case ER_X_CMD_INVALID_ARGUMENT:
          if (strstr(e.what(), "reuse_existing") != nullptr)
            err_msg =
                "The target MySQL server does not support the "
                "reuseExistingObject attribute";
          else if (strstr(e.what(), "field for create_collection command"))
            err_msg = shcore::str_replace(
                e.what(), "field for create_collection command", "option");
          break;
        case ER_X_INVALID_VALIDATION_SCHEMA:
          err_msg = std::string("The provided validation schema is invalid (") +
                    e.what() + ").";
          break;
      }
      if (!err_msg.empty()) {
        log_error("%s", e.what());
        throw mysqlshdk::db::Error(err_msg, e.code());
      }
    }
    throw;
  }

  return ret_val;
}

REGISTER_HELP_FUNCTION(modifyCollection, Schema);
REGISTER_HELP_FUNCTION_TEXT(SCHEMA_MODIFYCOLLECTION, R"*(
Modifies the schema validation of a collection.

@param name the name of the collection.
@param options dictionary with options.

The options dictionary may contain the following attributes:

@li validation: object defining JSON schema validation for the collection. 

The validation object allows the following attributes:

@li level: which can be either 'strict' or 'off'.
@li schema: a JSON schema specification as described at json-schema.org.
)*");

/**
 * $(SCHEMA_MODIFYCOLLECTION_BRIEF)
 *
 * $(SCHEMA_MODIFYCOLLECTION)
 */
#if DOXYGEN_JS
Undefined Schema::modifyCollection(String name, Dictionary options) {}
#elif DOXYGEN_PY
None Schema::modify_collection(str name, dict options) {}
#endif
void Schema::modify_collection(const std::string &name,
                               const shcore::Dictionary_t &opts) {
  if (!opts)
    throw Exception::argument_error(
        "The options dictionary needs to be specified");

  // Modifies collection on the server
  shcore::Dictionary_t options = shcore::make_dict();
  options->set("schema", Value(_name));
  options->set("name", Value(name));
  shcore::Argument_map(*opts).ensure_keys({"validation"}, {},
                                          "modify collection options", true);
  if (opts->get_type("validation") != Value_type::Map ||
      opts->get_map("validation")->size() == 0)
    throw Exception::argument_error(
        "validation option's value needs to be a non empty map");
  options->set("options", Value(opts));

  std::shared_ptr<Session> sess(
      std::static_pointer_cast<Session>(_session.lock()));

  try {
    if (sess)
      sess->execute_mysqlx_stmt("modify_collection_options", options);
    else
      throw shcore::Exception::logic_error("Unable to create collection '" +
                                           name + "', no Session available");
  } catch (const mysqlshdk::db::Error &e) {
    std::string err_msg;
    switch (e.code()) {
      case ER_X_INVALID_ADMIN_COMMAND:
        err_msg =
            "The target MySQL server does not support the requested operation.";
        break;
      case ER_X_CMD_INVALID_ARGUMENT:
        if (strstr(e.what(), "field for modify_collection_options command"))
          err_msg = shcore::str_replace(
              e.what(), "field for modify_collection_options command",
              "option");
        break;
      case ER_X_INVALID_VALIDATION_SCHEMA:
        err_msg = std::string("The provided validation schema is invalid (") +
                  e.what() + ").";
        break;
    }
    if (!err_msg.empty()) {
      log_error("%s", e.what());
      throw mysqlshdk::db::Error(err_msg, e.code());
    }
    throw;
  }
}

// Documentation of dropCollection function
REGISTER_HELP_FUNCTION(dropCollection, Schema);
REGISTER_HELP(SCHEMA_DROPCOLLECTION_BRIEF, "Drops the specified collection.");
REGISTER_HELP(SCHEMA_DROPCOLLECTION_RETURNS, "@returns Nothing.");

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
    throw shcore::Exception::type_error(
        function + ": Argument #1 is expected to be a string");

  Value schema = this->get_member("name");
  std::string name = args[0].get_string();

  try {
    if (type == "View") {
      std::shared_ptr<Session> sess(
          std::static_pointer_cast<Session>(_session.lock()));
      sess->execute_sql(sqlstring("drop view if exists !.!", 0)
                        << schema.get_string() << name);
    } else {
      if ((type == "Table") || (type == "Collection")) {
        auto command_args = shcore::make_dict();
        command_args->emplace("name", name);
        command_args->emplace("schema", schema);

        std::shared_ptr<Session> sess(
            std::static_pointer_cast<Session>(_session.lock()));
        try {
          sess->execute_mysqlx_stmt("drop_collection", command_args);
        } catch (const mysqlshdk::db::Error &e) {
          if (e.code() != ER_BAD_TABLE_ERROR) throw;
        }
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("drop" + type));

  this->_remove_object(name, type);

  return shcore::Value();
}
