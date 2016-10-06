/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysqlx_table_insert.h"
#include "mod_mysqlx_table_delete.h"
#include "mod_mysqlx_table_update.h"
#include "mod_mysqlx_table_select.h"

#include "utils/utils_help.h"

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

// Documentation of Table class
REGISTER_HELP(TABLE_BRIEF, "Represents a Table on an Schema, retrieved with a session created using mysqlx module.");

Table::Table(std::shared_ptr<Schema> owner, const std::string &name, bool is_view)
  : DatabaseObject(owner->_session.lock(), std::static_pointer_cast<DatabaseObject>(owner), name), _is_view(is_view) {
  _table_impl = owner->_schema_impl->getTable(name);
  init();
}

Table::Table(std::shared_ptr<const Schema> owner, const std::string &name, bool is_view) :
DatabaseObject(owner->_session.lock(), std::const_pointer_cast<Schema>(owner), name), _is_view(is_view) {
  _table_impl = owner->_schema_impl->getTable(name);
  init();
}

void Table::init() {
  add_method("insert", std::bind(&Table::insert_, this, _1), NULL);
  add_method("update", std::bind(&Table::update_, this, _1), NULL);
  add_method("select", std::bind(&Table::select_, this, _1), "searchCriteria", shcore::Array, NULL);
  add_method("delete", std::bind(&Table::delete_, this, _1), "tableFields", shcore::Array, NULL);
  add_method("isView", std::bind(&Table::is_view_, this, _1), NULL);
}

Table::~Table() {}

// Documentation of insert function
REGISTER_HELP(TABLE_INSERT_BRIEF, "Creates a record insertion handler.");
REGISTER_HELP(TABLE_INSERT_BRIEF1, "Creates a record insertion handler using a column list to insert records.");
REGISTER_HELP(TABLE_INSERT_PARAM, "@param columns The column list that will determine the order of the values to be inserted on the table.");
REGISTER_HELP(TABLE_INSERT_PARAM1, "@param col1 The first column name.");
REGISTER_HELP(TABLE_INSERT_PARAM2, "@param col2 The second column name.");
REGISTER_HELP(TABLE_INSERT_RETURN, "@return A TableInsert object.");
REGISTER_HELP(TABLE_INSERT_DETAIL, "This function creates a TableInsert object which is a record insertion handler.");
REGISTER_HELP(TABLE_INSERT_DETAIL1, "The TableInsert class has other functions that allow specifying the way the insertion occurs.");
REGISTER_HELP(TABLE_INSERT_DETAIL2, "The insertion is done when the execute method is called on the handler.");

/**
 * $(TABLE_INSERT_BRIEF)
 *
 * $(TABLE_INSERT_RETURN)
 *
 * $(TABLE_INSERT_DETAIL)
 *
 * $(TABLE_INSERT_DETAIL1)
 *
 * $(TABLE_INSERT_DETAIL2)
 *
 * \sa TableInsert
 */
#if DOXYGEN_JS
TableInsert Table::insert() {}
#elif DOXYGEN_PY
TableInsert Table::insert() {}
#endif

// Documentation of function
REGISTER_HELP(TABLE__BRIEF, "");

/**
 * $(TABLE_INSERT_BRIEF1)
 *
 * $(TABLE_INSERT_PARAM)
 * $(TABLE_INSERT_RETURN)
 *
 * $(TABLE_INSERT_DETAIL)
 *
 * $(TABLE_INSERT_DETAIL1)
 *
 * $(TABLE_INSERT_DETAIL2)
 *
 * \sa TableInsert
 */
#if DOXYGEN_JS
TableInsert Table::insert(List columns) {}
#elif DOXYGEN_PY
TableInsert Table::insert(list columns) {}
#endif

/**
 * $(TABLE_INSERT_BRIEF1)
 *
 * $(TABLE_INSERT_PARAM1)
 * $(TABLE_INSERT_PARAM2)
 * $(TABLE_INSERT_RETURN)
 *
 * $(TABLE_INSERT_DETAIL)
 *
 * $(TABLE_INSERT_DETAIL1)
 *
 * $(TABLE_INSERT_DETAIL2)
 *
 * \sa TableInsert
 */
#if DOXYGEN_JS
TableInsert Table::insert(String col1, String col2, ...) {}
#elif DOXYGEN_PY
TableInsert Table::insert(str col1, str col2, ...) {}
#endif

shcore::Value Table::insert_(const shcore::Argument_list &args) {
  std::shared_ptr<TableInsert> tableInsert(new TableInsert(shared_from_this()));

  return tableInsert->insert(args);
}

// Documentation of update function
REGISTER_HELP(TABLE_UPDATE_BRIEF, "Creates a record update handler.");
REGISTER_HELP(TABLE_UPDATE_RETURN, "@return A TableUpdate object.");
REGISTER_HELP(TABLE_UPDATE_DETAIL, "This function creates a TableUpdate object which is a record update handler.");
REGISTER_HELP(TABLE_UPDATE_DETAIL1, "The TableUpdate class has several functions that allow specifying the way the update occurs, "\
"if a searchCondition was specified, it will be set on the handler.");
REGISTER_HELP(TABLE_UPDATE_DETAIL2, "The update is done when the execute function is called on the handler.");

/**
* $(TABLE_UPDATE_BRIEF)
*
* $(TABLE_UPDATE_RETURN)
*
* $(TABLE_UPDATE_DETAIL)
*
* $(TABLE_UPDATE_DETAIL1)
*
* $(TABLE_UPDATE_DETAIL2)
*
* \sa TableUpdate
*/
#if DOXYGEN_JS
TableUpdate Table::update() {}
#elif DOXYGEN_PY
TableUpdate Table::update() {}
#endif
shcore::Value Table::update_(const shcore::Argument_list &args) {
  std::shared_ptr<TableUpdate> tableUpdate(new TableUpdate(shared_from_this()));

  return tableUpdate->update(args);
}

// Documentation of delete function
REGISTER_HELP(TABLE_DELETE_BRIEF, "Creates a record deletion handler.");
REGISTER_HELP(TABLE_DELETE_RETURN, "@return A TableDelete object.");
REGISTER_HELP(TABLE_DELETE_DETAIL, "This function creates a TableDelete object which is a record deletion handler.");
REGISTER_HELP(TABLE_DELETE_DETAIL1, "The TableDelete class has several functions that allow specifying what should be deleted and how, "\
"if a searchCondition was specified, it will be set on the handler.");
REGISTER_HELP(TABLE_DELETE_DETAIL2, "The deletion is done when the execute function is called on the handler.");

/**
* $(TABLE_DELETE_BRIEF)
*
* $(TABLE_DELETE_RETURN)
*
* $(TABLE_DELETE_DETAIL)
*
* $(TABLE_DELETE_DETAIL1)
*
* $(TABLE_DELETE_DETAIL2)
*
* \sa TableDelete
*/
#if DOXYGEN_JS
TableDelete Table::delete() {}
#elif DOXYGEN_PY
TableDelete Table::delete() {}
#endif
shcore::Value Table::delete_(const shcore::Argument_list &args) {
  std::shared_ptr<TableDelete> tableDelete(new TableDelete(shared_from_this()));

  return tableDelete->remove(args);
}

// Documentation of select function
REGISTER_HELP(TABLE_SELECT_BRIEF, "Creates a full record retrieval handler.");
REGISTER_HELP(TABLE_SELECT_BRIEF1, "Creates a partial record retrieval handler.");
REGISTER_HELP(TABLE_SELECT_PARAM, "@param columns A list of strings defining the columns to be retrieved.");
REGISTER_HELP(TABLE_SELECT_RETURN, "@return A TableSelect object.");
REGISTER_HELP(TABLE_SELECT_DETAIL, "This function creates a TableSelect object which is a record selection handler.");
REGISTER_HELP(TABLE_SELECT_DETAIL1, "This handler will retrieve all the columns for each included record.");
REGISTER_HELP(TABLE_SELECT_DETAIL2, "The TableSelect class has several functions that allow specifying what records should be retrieved "\
"from the table, if a searchCondition was specified, it will be set on the handler.");
REGISTER_HELP(TABLE_SELECT_DETAIL3, "The selection will be returned when the execute function is called on the handler.");
REGISTER_HELP(TABLE_SELECT_DETAIL4, "This handler will retrieve only the columns specified on the columns list for each included record.");
REGISTER_HELP(TABLE_SELECT_DETAIL5, "Each column on the list should be a string identifying the column name, alias is supported.");

/**
* $(TABLE_SELECT_BRIEF)
*
* $(TABLE_SELECT_RETURN)
*
* $(TABLE_SELECT_DETAIL)
*
* $(TABLE_SELECT_DETAIL1)
*
* $(TABLE_SELECT_DETAIL2)
*
* $(TABLE_SELECT_DETAIL3)
*
* \sa TableSelect
*/
#if DOXYGEN_JS
TableSelect Table::select() {}
#elif DOXYGEN_PY
TableSelect Table::select() {}
#endif

/**
* $(TABLE_SELECT_BRIEF1)
*
* $(TABLE_SELECT_PARAM)
* $(TABLE_SELECT_RETURN)
*
* $(TABLE_SELECT_DETAIL)
*
* $(TABLE_SELECT_DETAIL4)
*
* $(TABLE_SELECT_DETAIL5)
*
* $(TABLE_SELECT_DETAIL2)
*
* $(TABLE_SELECT_DETAIL3)
*
* \sa TableSelect
*/
#if DOXYGEN_JS
TableSelect Table::select(List columns) {}
#elif DOXYGEN_PY
TableSelect Table::select(list columns) {}
#endif

shcore::Value Table::select_(const shcore::Argument_list &args) {
  std::shared_ptr<TableSelect> tableSelect(new TableSelect(shared_from_this()));

  return tableSelect->select(args);
}

// Documentation of isView function
REGISTER_HELP(TABLE_ISVIEW_BRIEF, "Indicates whether this Table object represents a View on the database.");
REGISTER_HELP(TABLE_ISVIEW_RETURN, "@return True if the Table represents a View on the database, False if represents a Table.");

/**
* $(TABLE_ISVIEW_BRIEF)
*
* $(TABLE_ISVIEW_RETURN)
*/
#if DOXYGEN_JS
Bool Table::isView() {}
#elif DOXYGEN_PY
Bool Table::is_view() {}
#endif
shcore::Value Table::is_view_(const shcore::Argument_list &args) {
  args.ensure_count(0, "Table.isView");

  return Value(_is_view);
}
