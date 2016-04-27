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

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>
#include "utils/utils_general.h"
#include "logger/logger.h"

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

Schema::Schema(boost::shared_ptr<BaseSession> session, const std::string &schema)
  : DatabaseObject(session, boost::shared_ptr<DatabaseObject>(), schema), _schema_impl(session->session_obj()->getSchema(schema))
{
  init();
}

Schema::Schema(boost::shared_ptr<const BaseSession> session, const std::string &schema) :
DatabaseObject(boost::const_pointer_cast<BaseSession>(session), boost::shared_ptr<DatabaseObject>(), schema)
{
  init();
}

Schema::~Schema()
{
}

void Schema::init()
{
  add_method("getTables", boost::bind(&Schema::get_tables, this, _1), NULL);
  add_method("getCollections", boost::bind(&Schema::get_collections, this, _1), NULL);

  add_method("getTable", boost::bind(&Schema::get_table, this, _1), "name", shcore::String, NULL);
  add_method("getCollection", boost::bind(&Schema::get_collection, this, _1), "name", shcore::String, NULL);
  add_method("getCollectionAsTable", boost::bind(&Schema::get_collection_as_table, this, _1), "name", shcore::String, NULL);

  add_method("createCollection", boost::bind(&Schema::create_collection, this, _1), "name", shcore::String, NULL);

  _tables = Value::new_map().as_map();
  _views = Value::new_map().as_map();
  _collections = Value::new_map().as_map();

  // Setups the cache handlers
  auto table_generator = [this](const std::string& name){return shcore::Value::wrap<Table>(new Table(shared_from_this(), name, false)); };
  auto view_generator = [this](const std::string& name){return shcore::Value::wrap<Table>(new Table(shared_from_this(), name, true)); };
  auto collection_generator = [this](const std::string& name){return shcore::Value::wrap<Collection>(new Collection(shared_from_this(), name)); };

  update_table_cache = [table_generator, this](const std::string &name, bool exists){DatabaseObject::update_cache(name, table_generator, exists, _tables); };
  update_view_cache = [view_generator, this](const std::string &name, bool exists){DatabaseObject::update_cache(name, view_generator, exists, _views); };
  update_collection_cache = [collection_generator, this](const std::string &name, bool exists){DatabaseObject::update_cache(name, collection_generator, exists, _collections); };

  update_full_table_cache = [table_generator, this](const std::vector<std::string> &names){DatabaseObject::update_cache(names, table_generator, _tables); };
  update_full_view_cache = [view_generator, this](const std::vector<std::string> &names){DatabaseObject::update_cache(names, view_generator, _views); };
  update_full_collection_cache = [collection_generator, this](const std::vector<std::string> &names){DatabaseObject::update_cache(names, collection_generator, _collections); };
}

void Schema::update_cache()
{
  try
  {
    boost::shared_ptr<BaseSession> sess(boost::static_pointer_cast<BaseSession>(_session.lock()));
    if (sess)
    {
      std::vector<std::string> tables;
      std::vector<std::string> collections;
      std::vector<std::string> views;
      std::vector<std::string> others;

      {
        shcore::Argument_list args;
        args.push_back(Value(_name));
        args.push_back(Value(""));

        Value myres = sess->executeAdminCommand("list_objects", true, args);
        boost::shared_ptr<mysh::mysqlx::SqlResult> my_res = myres.as_object<mysh::mysqlx::SqlResult>();

        Value raw_entry;

        while ((raw_entry = my_res->fetch_one(shcore::Argument_list())))
        {
          boost::shared_ptr<mysh::Row> row = raw_entry.as_object<mysh::Row>();
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
        if (others.size())
        {
          for (size_t index = 0; index < others.size(); index++)
            log_error("%s", others[index].c_str());
        }
      }
    }
  }
  CATCH_AND_TRANSLATE();
}

void Schema::_remove_object(const std::string& name, const std::string& type)
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
  else if (type == "Collection")
  {
    if (_collections->count(name))
      _collections->erase(name);
  }
}

std::vector<std::string> Schema::get_members() const
{
  std::vector<std::string> members(DatabaseObject::get_members());

  for (Value::Map_type::const_iterator iter = _tables->begin();
       iter != _tables->end(); ++iter)
  {
    if (shcore::is_valid_identifier(iter->first))
      members.push_back(iter->first);
  }

  for (Value::Map_type::const_iterator iter = _collections->begin();
       iter != _collections->end(); ++iter)
  {
    if (shcore::is_valid_identifier(iter->first))
      members.push_back(iter->first);
  }

  for (Value::Map_type::const_iterator iter = _views->begin();
       iter != _views->end(); ++iter)
  {
    if (shcore::is_valid_identifier(iter->first))
      members.push_back(iter->first);
  }

  return members;
}

Value Schema::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is must be either a table, collection or view and attempt loading it as such
  if (DatabaseObject::has_member(prop))
    ret_val = DatabaseObject::get_member(prop);
  else
  {
    // At this point the property should be one of table
    // collection or view
    ret_val = find_in_cache(prop, _tables);

    if (!ret_val)
      ret_val = find_in_cache(prop, _collections);

    if (!ret_val)
      ret_val = find_in_cache(prop, _views);

    if (!ret_val)
      throw Exception::attrib_error("Invalid object member " + prop);
  }

  return ret_val;
}

#ifdef DOXYGEN
/**
* Returns a list of Tables for this Schema.
* \sa Table
* \return A List containing the Table objects available for the Schema.
*
* Pulls from the database the available Tables, Views and Collections.
*
* Does a full refresh of the Tables, Views and Collections cache.
*
* Returns a List of available Table objects.
*/
List Schema::getTables(){}
#endif
shcore::Value Schema::get_tables(const shcore::Argument_list &args)
{
  args.ensure_count(0, (class_name() + ".getTables").c_str());

  update_cache();

  shcore::Value::Array_type_ref list(new shcore::Value::Array_type);

  get_object_list(_tables, list);
  get_object_list(_views, list);

  return shcore::Value(list);
}

#ifdef DOXYGEN
/**
* Returns a list of Collections for this Schema.
* \sa Collection
* \return A List containing the Collection objects available for the Schema.
*
* Pulls from the database the available Tables, Views and Collections.
*
* Does a full refresh of the Tables, Views and Collections cache.
*
* Returns a List of available Collection objects.
*/
List Schema::getCollections(){}
#endif
shcore::Value Schema::get_collections(const shcore::Argument_list &args)
{
  args.ensure_count(0, (class_name() + ".getCollections").c_str());

  update_cache();

  shcore::Value::Array_type_ref list(new shcore::Value::Array_type);

  get_object_list(_collections, list);

  return shcore::Value(list);
}

#ifdef DOXYGEN
/**
* Returns the Table of the given name for this schema.
* \sa Table
* \param name the name of the Table to look for.
* \return the Table object matching the name.
*
* Verifies if the requested Table exist on the database, if exists, returns the corresponding Table object.
*
* Updates the Tables cache.
*/
View Schema::getTable(String name){}
#endif
shcore::Value Schema::get_table(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + ".getTable").c_str());

  std::string name = args.string_at(0);
  shcore::Value ret_val;

  if (!name.empty())
  {
    std::string found_type;
    std::string real_name = _session.lock()->db_object_exists(found_type, name, _name);
    bool exists = false;

    if (!real_name.empty())
    {
      if (found_type == "TABLE")
      {
        exists = true;

        // Updates the cache
        update_table_cache(real_name, exists);

        ret_val = (*_tables)[real_name];
      }
      else if (found_type == "VIEW")
      {
        exists = true;

        // Updates the cache
        update_view_cache(real_name, exists);

        ret_val = (*_views)[real_name];
      }
    }

    if (!exists)
      throw shcore::Exception::runtime_error("The table " + _name + "." + name + " does not exist");
  }
  else
    throw shcore::Exception::argument_error("An empty name is invalid for a table");

  return ret_val;
}

#ifdef DOXYGEN
/**
* Returns the Collection of the given name for this schema.
* \sa Collection
* \param name the name of the Collection to look for.
* \return the Collection object matching the name.
*
* Verifies if the requested Collection exist on the database, if exists, returns the corresponding Collection object.
*
* Updates the Collections cache.
*/
View Schema::getCollection(String name){}
#endif
shcore::Value Schema::get_collection(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + ".getCollection").c_str());

  std::string name = args.string_at(0);
  shcore::Value ret_val;

  if (!name.empty())
  {
    std::string found_type;
    std::string real_name = _session.lock()->db_object_exists(found_type, name, _name);
    bool exists = false;
    if (!real_name.empty() && found_type == "COLLECTION")
      exists = true;

    // Updates the cache
    update_collection_cache(real_name, exists);

    if (exists)
      ret_val = (*_collections)[real_name];
    else
      throw shcore::Exception::runtime_error("The collection " + _name + "." + name + " does not exist");
  }
  else
    throw shcore::Exception::argument_error("An empty name is invalid for a collection");

  return ret_val;
  }

#ifdef DOXYGEN
/**
* Returns a Table object representing a Collection on the database.
* \param name the name of the collection to be retrieved as a table.
* \return the Table object representing the collection or undefined.
*/
Collection Schema::getCollectionAsTable(String name){}
#endif
shcore::Value Schema::get_collection_as_table(const shcore::Argument_list &args)
{
  args.ensure_count(1, "Schema.getCollectionAsTable");

  Value ret_val = get_collection(args);

  if (ret_val)
  {
    boost::shared_ptr<Table> table(new Table(shared_from_this(), args.string_at(0)));
    ret_val = Value(boost::static_pointer_cast<Object_bridge>(table));
  }

  return ret_val;
}

#ifdef DOXYGEN
/**
* Creates in the current schema a new collection with the specified name and retrieves an object representing the new collection created.
* \param name the name of the collection.
* \return the new created collection.
*
* To specify a name for a collection, follow the naming conventions in MySQL.
* \sa getCollections(), getCollection()
*/
Collection Schema::createCollection(String name){}
#endif
shcore::Value Schema::create_collection(const shcore::Argument_list &args)
{
  Value ret_val;

  args.ensure_count(1, (class_name() + ".createCollection").c_str());

  // Creates the collection on the server
  shcore::Argument_list command_args;
  command_args.push_back(Value(_name));
  command_args.push_back(args[0]);

  boost::shared_ptr<BaseSession> sess(boost::static_pointer_cast<BaseSession>(_session.lock()));
  sess->executeAdminCommand("create_collection", false, command_args);

  // If this is reached it implies all went OK on the previous operation
  std::string name = args.string_at(0);
  boost::shared_ptr<Collection> collection(new Collection(shared_from_this(), name));
  ret_val = Value(boost::static_pointer_cast<Object_bridge>(collection));
  (*_collections)[name] = ret_val;

  return ret_val;
}