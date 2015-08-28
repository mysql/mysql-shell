/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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
#include "mod_mysqlx_table.h"
#include <boost/bind.hpp>

#include "mod_mysqlx_table_insert.h"
#include "mod_mysqlx_table_delete.h"
#include "mod_mysqlx_table_update.h"
#include "mod_mysqlx_table_select.h"

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

Table::Table(boost::shared_ptr<Schema> owner, const std::string &name)
: DatabaseObject(owner->_session.lock(), boost::static_pointer_cast<DatabaseObject>(owner), name)
{
  _table_impl = owner->_schema_impl->getTable(name);
  init();
}

Table::Table(boost::shared_ptr<const Schema> owner, const std::string &name) :
DatabaseObject(owner->_session.lock(), boost::const_pointer_cast<Schema>(owner), name)
{
  _table_impl = owner->_schema_impl->getTable(name);
  init();
}

void Table::init()
{
  add_method("insert", boost::bind(&Table::insert_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("update", boost::bind(&Table::update_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("select", boost::bind(&Table::select_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("delete", boost::bind(&Table::delete_, this, _1), "searchCriteria", shcore::String, NULL);
}

Table::~Table()
{
}

#ifdef DOXYGEN
/**
* Creates an insert object with column mappings to be defined.
* \sa update, delete, select, TableInsert
* \return a new TableInsert object.
*/
TableInsert Table::insert()
{}

/**
* Creates an insert object with a list of column names.
* \sa update, delete, select, TableInsert
* \param [field, field, ...] one of several field names passed as an array parameter.
* \return a new TableInsert object.
*/
TableInsert Table::insert([field, field, ...])
{}

/**
* Creates an insert object with a set of mappings (field name, value of field).
* \sa update, delete, select, TableInsert
* \param { field : value, field : value, ... } a map with a set of key value pair mappings where the key is a field name and the field value is the value.
* \return a new TableInsert object.
*/
TableInsert Table::insert({ field : value, field : value, ... })
{}
#endif
shcore::Value Table::insert_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableInsert> tableInsert(new TableInsert(shared_from_this()));

  return tableInsert->insert(args);
}

#ifdef DOXYGEN
/**
* Creates an Update object and returns it.
* \sa insert, delete, select, TableUpdate
* \return a new TableUpdate object.
*/
TableUpdate Table::update()
{}
#endif
shcore::Value Table::update_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableUpdate> tableUpdate(new TableUpdate(shared_from_this()));

  return tableUpdate->update(args);
}

#ifdef DOXYGEN
/**
* Creates a Delete object and returns it.
* \sa insert, update, select, TableDelete
* \return a new TableDelete object.
*/
TableDelete Table::delete()
{}
#endif
shcore::Value Table::delete_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableDelete> tableDelete(new TableDelete(shared_from_this()));

  return tableDelete->remove(args);
}

#ifdef DOXYGEN
/**
* Creates a Select object and returns it.
* \sa insert, delete, update, TableSelect
* \return a new TableSelect object.
*/
TableSelect Table::select()
{}

/**
* Creates a Select object and returns it with the specified projection (list of fields to return).
* \sa insert, delete, update, TableSelect
* \param [field, field, ...] one or more Strings each with the name of a field to retrieve in this select statement.
*   each field can be of the form "identifier [[AS] alias]", that is, after the column name optionally include the AS keyword and then an ALIAS
* \return a new TableSelect object.
*/
TableSelect Table::select([field, field, ...])
{}
#endif
shcore::Value Table::select_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableSelect> tableSelect(new TableSelect(shared_from_this()));

  return tableSelect->select(args);
}
