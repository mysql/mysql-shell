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

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"

#include "shellcore/proxy_object.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>
#include "utils/utils_general.h"
#include "logger/logger.h"

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
  add_method("getTables", boost::bind(&ClassicSchema::get_tables, this, _1), NULL);
  add_method("getTable", boost::bind(&ClassicSchema::get_table, this, _1), "name", shcore::String, NULL);

  _tables = Value::new_map().as_map();
  _views = Value::new_map().as_map();

  // Setups the cache handlers
  auto table_generator = [this](const std::string& name){return shcore::Value::wrap<ClassicTable>(new ClassicTable(shared_from_this(), name, false)); };
  auto view_generator = [this](const std::string& name){return shcore::Value::wrap<ClassicTable>(new ClassicTable(shared_from_this(), name, true)); };

  update_table_cache = [table_generator, this](const std::string &name, bool exists){DatabaseObject::update_cache(name, table_generator, exists, _tables, this); };
  update_view_cache = [view_generator, this](const std::string &name, bool exists){DatabaseObject::update_cache(name, view_generator, exists, _views, this); };

  update_full_table_cache = [table_generator, this](const std::vector<std::string> &names){DatabaseObject::update_cache(names, table_generator, _tables, this); };
  update_full_view_cache = [view_generator, this](const std::vector<std::string> &names){DatabaseObject::update_cache(names, view_generator, _views, this); };
}

ClassicSchema::~ClassicSchema()
{
}

void ClassicSchema::update_cache()
{
  boost::shared_ptr<ClassicSession> sess(boost::dynamic_pointer_cast<ClassicSession>(_session.lock()));
  if (sess)
  {
    std::vector<std::string> tables;
    std::vector<std::string> views;
    std::vector<std::string> others;

    Result *result = sess->connection()->run_sql(sqlstring("show full tables in !", 0) << _name);
    Row *row = result->fetch_one();
    while (row)
    {
      std::string object_name = row->get_value_as_string(0);
      std::string object_type = row->get_value_as_string(1);

      if (object_type == "BASE TABLE" || object_type == "LOCAL TEMPORARY")
        tables.push_back(object_name);
      else if (object_type == "VIEW" || object_type == "SYSTEM VIEW")
        views.push_back(object_name);
      else
        others.push_back((boost::format("Unexpected Object Retrieved from Database: %s% of type %s%") % object_name % object_type).str());;

      row = result->fetch_one();
    }

    // Updates the cache
    update_full_table_cache(tables);
    update_full_view_cache(views);

    // Log errors about unexpected object type
    if (others.size())
    {
      for (size_t index = 0; index < others.size(); index++)
        log_error("%s", others[index].c_str());
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

Value ClassicSchema::get_member(const std::string &prop) const
{
  // Searches the property in tables
  Value ret_val = find_in_cache(prop, _tables);

  // Search the property in views
  if (!ret_val)
    ret_val = find_in_cache(prop, _views);

  // Search the rest of the properties
  if (!ret_val)
    ret_val = DatabaseObject::get_member(prop);

  return ret_val;
}

#ifdef DOXYGEN
/**
* Returns the table of the given name for this schema.
* \sa ClassicTable
* \param name the name of the table to look for.
* \return the ClassicTable object matching the name.
*
* Verifies if the requested Table exist on the database, if exists, returns the corresponding ClassicTable object.
*
* Updates the Tables cache.
*/
ClassicTable ClassicSchema::getTable(String name)
{}
#endif
shcore::Value ClassicSchema::get_table(const shcore::Argument_list &args)
{
  args.ensure_count(1, get_function_name("getTable").c_str());
  std::string name = args.string_at(0);
  shcore::Value ret_val;

  if (!name.empty())
  {
    std::string found_type;
    std::string real_name = _session.lock()->db_object_exists(found_type, name, _name);
    bool exists = false;
    if (!real_name.empty())
    {
      if (found_type == "BASE TABLE" || found_type == "LOCAL TEMPORARY")
      {
        exists = true;

        // Updates the cache
        update_table_cache(real_name, exists);

        ret_val = (*_tables)[real_name];
      }
      else if (found_type == "VIEW" || found_type == "SYSTEM VIEW")
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
* Returns a list of Tables for this Schema.
* \sa ClassicTable
* \return A List containing the Table objects available for the Schema.
*
* Pulls from the database the available Tables and Views.
*
* Does a full refresh of the Tables and Views cache.
*
* Returns a List of available Table objects.
*/
List ClassicSchema::getTables(){}
#endif
shcore::Value ClassicSchema::get_tables(const shcore::Argument_list &args)
{
  args.ensure_count(0, get_function_name("getTables").c_str());

  update_cache();

  shcore::Value::Array_type_ref list(new shcore::Value::Array_type);

  get_object_list(_tables, list);
  get_object_list(_views, list);

  return shcore::Value(list);
}