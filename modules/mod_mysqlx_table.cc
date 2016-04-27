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

Table::Table(boost::shared_ptr<Schema> owner, const std::string &name, bool is_view)
  : DatabaseObject(owner->_session.lock(), boost::static_pointer_cast<DatabaseObject>(owner), name), _is_view(is_view)
{
  _table_impl = owner->_schema_impl->getTable(name);
  init();
}

Table::Table(boost::shared_ptr<const Schema> owner, const std::string &name, bool is_view) :
DatabaseObject(owner->_session.lock(), boost::const_pointer_cast<Schema>(owner), name), _is_view(is_view)
{
  _table_impl = owner->_schema_impl->getTable(name);
  init();
}

void Table::init()
{
  add_method("insert", boost::bind(&Table::insert_, this, _1), NULL);
  add_method("update", boost::bind(&Table::update_, this, _1), NULL);
  add_method("select", boost::bind(&Table::select_, this, _1), "searchCriteria", shcore::Array, NULL);
  add_method("delete", boost::bind(&Table::delete_, this, _1), "tableFields", shcore::Array, NULL);
  add_method("isView", boost::bind(&Table::is_view, this, _1), NULL);
}

Table::~Table()
{
}

#ifdef DOXYGEN
/**
* Creates a record insertion handler.
* \return A TableInsert object.
*
* This function creates a TableInsert object which is a record insertion handler.
*
* The TableInsert class has other functions that allow specifying the way the insertion occurs.
*
* The insertion is done when the execute method is called on the handler.
*
* \sa TableInsert
*/
TableInsert Table::insert(){}

/**
* Creates a record insertion handler using a column list to insert records.
* \param columns The column list that will determine the order of the values to be inserted on the table.
* \return A TableInsert object.
*
* This function creates a TableInsert object which is a record insertion handler.
*
* The TableInsert class has other functions that allow specifying the way the insertion occurs.
*
* The insertion is done when the execute method is called on the handler.
*
* \sa TableInsert
*/
TableInsert Table::insert(List columns){}

/**
* Creates a record insertion handler using a column list to insert records.
* \param col1 The first column name.
* \param col2 The second column name.
* \return A TableInsert object.
*
* This function creates a TableInsert object which is a record insertion handler.
*
* The TableInsert class has other functions that allow specifying the way the insertion occurs.
*
* The insertion is done when the execute method is called on the handler.
*
* \sa TableInsert
*/
TableInsert Table::insert(String col1, String col2, ...){}
#endif
shcore::Value Table::insert_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableInsert> tableInsert(new TableInsert(shared_from_this()));

  return tableInsert->insert(args);
}

#ifdef DOXYGEN
/**
* Creates a record update handler.
* \return A TableUpdate object.
*
* This function creates a TableUpdate object which is a record update handler.
*
* The TableUpdate class has several functions that allow specifying the way the update occurs, if a searchCondition was specified, it will be set on the handler.
*
* The update is done when the execute function is called on the handler.
*
* \sa TableUpdate
*/
TableUpdate Table::update(){}
#endif
shcore::Value Table::update_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableUpdate> tableUpdate(new TableUpdate(shared_from_this()));

  return tableUpdate->update(args);
}

#ifdef DOXYGEN
/**
* Creates a record deletion handler.
* \return A TableDelete object.
*
* This function creates a TableDelete object which is a record deletion handler.
*
* The TableDelete class has several functions that allow specifying what should be deleted and how, if a searchCondition was specified, it will be set on the handler.
*
* The deletion is done when the execute function is called on the handler.
*
* \sa TableDelete
*/
TableDelete Table::delete(){}
#endif
shcore::Value Table::delete_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableDelete> tableDelete(new TableDelete(shared_from_this()));

  return tableDelete->remove(args);
}

#ifdef DOXYGEN
/**
* Creates a full record retrieval handler.
* \return A TableSelect object.
*
* This function creates a TableSelect object which is a record selection handler.
*
* This handler will retrieve all the columns for each included record.
*
* The TableSelect class has several functions that allow specifying what records should be retrieved from the table, if a searchCondition was specified, it will be set on the handler.
*
* The selection will be returned when the execute function is called on the handler.
*
* \sa TableSelect
*/
TableSelect Table::select(){}

/**
* Creates a partial record retrieval handler.
* \param columns A list of strings defining the columns to be retrieved.
* \return A TableSelect object.
*
* This function creates a TableSelect object which is a record selection handler.
*
* This handler will retrieve only the columns specified on the columns list for each included record.
*
* Each column on the list should be a string identifying the column name, alias is supported.
*
* The TableSelect class has several functions that allow specifying what records should be retrieved from the table, if a searchCondition was specified, it will be set on the handler.
*
* The selection will be returned when the execute function is called on the handler.
*
* \sa TableSelect
*/
TableSelect Table::select(List columns){}
#endif
shcore::Value Table::select_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableSelect> tableSelect(new TableSelect(shared_from_this()));

  return tableSelect->select(args);
}

#ifdef DOXYGEN
/**
* Indicates whether this Table object represents a View on the database.
* \return True if the Table represents a View on the database, False if represents a Table.
*/
Bool Table::isView(){}
#endif
shcore::Value Table::is_view(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Table.isView");

  return Value(_is_view);
}