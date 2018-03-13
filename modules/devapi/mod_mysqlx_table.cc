/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/mod_mysqlx_table.h"
#include <memory>
#include <string>
#include "modules/devapi/mod_mysqlx_schema.h"

#include "modules/devapi/mod_mysqlx_table_delete.h"
#include "modules/devapi/mod_mysqlx_table_insert.h"
#include "modules/devapi/mod_mysqlx_table_select.h"
#include "modules/devapi/mod_mysqlx_table_update.h"

#include "shellcore/utils_help.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::mysqlx;
using namespace shcore;

// Documentation of Table class
REGISTER_HELP_SUB_CLASS(Table, mysqlx, DatabaseObject);
REGISTER_HELP(TABLE_BRIEF,
              "Represents a Table on an Schema, retrieved with a session "
              "created using mysqlx module.");

Table::Table(std::shared_ptr<Schema> owner, const std::string &name,
             bool is_view)
    : DatabaseObject(owner->_session.lock(),
                     std::static_pointer_cast<DatabaseObject>(owner), name),
      _is_view(is_view) {
  init();
}

Table::Table(std::shared_ptr<const Schema> owner, const std::string &name,
             bool is_view)
    : DatabaseObject(owner->_session.lock(),
                     std::const_pointer_cast<Schema>(owner), name),
      _is_view(is_view) {
  init();
}

void Table::init() {
  add_method("insert", std::bind(&Table::insert_, this, _1));
  add_method("update", std::bind(&Table::update_, this, _1));
  add_method("select", std::bind(&Table::select_, this, _1), "searchCriteria",
             shcore::Array);
  add_method("delete", std::bind(&Table::delete_, this, _1), "tableFields",
             shcore::Array);
  add_method("isView", std::bind(&Table::is_view_, this, _1));
}

Table::~Table() {}

// Documentation of insert function
REGISTER_HELP_FUNCTION(insert, Table);
REGISTER_HELP(
    TABLE_INSERT_BRIEF,
    "Creates <b>TableInsert</b> object to insert new records into the table.");
REGISTER_HELP(TABLE_INSERT_RETURNS, "@returns A TableInsert object.");
REGISTER_HELP(TABLE_INSERT_CHAINED, "TableInsert.insert.[values].[execute]");
REGISTER_HELP(TABLE_INSERT_DETAIL,
              "The TableInsert class has other functions that allow specifying "
              "the way the insertion occurs.");
REGISTER_HELP(
    TABLE_INSERT_DETAIL1,
    "The insertion is done when the execute method is called on the handler.");

/**
 * $(TABLE_INSERT_BRIEF)
 *
 * $(TABLE_INSERT_RETURNS)
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
 * $(TABLE_INSERT_RETURNS)
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
 * $(TABLE_INSERT_RETURNS)
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
REGISTER_HELP_FUNCTION(update, Table);
REGISTER_HELP(TABLE_UPDATE_BRIEF, "Creates a record update handler.");
REGISTER_HELP(TABLE_UPDATE_RETURNS, "@returns A TableUpdate object.");
REGISTER_HELP(
    TABLE_UPDATE_CHAINED,
    "TableUpdate.update.set.[where].[<<<orderBy>>>].[limit].[bind].[execute]");
REGISTER_HELP(TABLE_UPDATE_DETAIL,
              "This function creates a TableUpdate object which is a record "
              "update handler.");
REGISTER_HELP(
    TABLE_UPDATE_DETAIL1,
    "The TableUpdate class has several functions that allow specifying the way "
    "the update occurs, "
    "if a searchCondition was specified, it will be set on the handler.");
REGISTER_HELP(
    TABLE_UPDATE_DETAIL2,
    "The update is done when the execute function is called on the handler.");

/**
 * $(TABLE_UPDATE_BRIEF)
 *
 * $(TABLE_UPDATE_RETURNS)
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
REGISTER_HELP_FUNCTION(delete, Table);
REGISTER_HELP(TABLE_DELETE_BRIEF, "Creates a record deletion handler.");
REGISTER_HELP(TABLE_DELETE_RETURNS, "@returns A TableDelete object.");
REGISTER_HELP(
    TABLE_DELETE_CHAINED,
    "TableDelete.delete.[where].[<<<orderBy>>>].[limit].[bind].[execute]");

REGISTER_HELP(TABLE_DELETE_DETAIL,
              "This function creates a TableDelete object which is a record "
              "deletion handler.");
REGISTER_HELP(
    TABLE_DELETE_DETAIL1,
    "The TableDelete class has several functions that allow specifying what "
    "should be deleted and how, "
    "if a searchCondition was specified, it will be set on the handler.");
REGISTER_HELP(
    TABLE_DELETE_DETAIL2,
    "The deletion is done when the execute function is called on the handler.");

/**
 * $(TABLE_DELETE_BRIEF)
 *
 * $(TABLE_DELETE_RETURNS)
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
TableDelete Table::delete () {}
#elif DOXYGEN_PY
TableDelete Table::delete () {}
#endif
shcore::Value Table::delete_(const shcore::Argument_list &args) {
  std::shared_ptr<TableDelete> tableDelete(new TableDelete(shared_from_this()));

  return tableDelete->remove(args);
}

// Documentation of select function
REGISTER_HELP_FUNCTION(select, Table);
REGISTER_HELP(
    TABLE_SELECT_CHAINED,
    "TableSelect.select.[where].[<<<groupBy>>>->[having]].[<<<orderBy>>>]."
    "[limit->[offset]].[<<<lockShared>>>].[<<<lockExclusive>>>].[bind]."
    "[execute]");

REGISTER_HELP(
    TABLE_SELECT_BRIEF,
    "Creates a <b>TableSelect</b> object to retrieve rows from the table.");
REGISTER_HELP(
    TABLE_SELECT_PARAM,
    "@param columns A list of strings defining the columns to be retrieved.");
REGISTER_HELP(TABLE_SELECT_RETURNS, "@returns A TableSelect object.");
REGISTER_HELP(TABLE_SELECT_DETAIL,
              "This function creates a TableSelect object which is a record "
              "selection handler.");
REGISTER_HELP(
    TABLE_SELECT_DETAIL1,
    "This handler will retrieve all the columns for each included record.");
REGISTER_HELP(TABLE_SELECT_DETAIL2,
              "The TableSelect class has several functions that allow "
              "specifying what records should be retrieved "
              "from the table, if a searchCondition was specified, it will be "
              "set on the handler.");
REGISTER_HELP(TABLE_SELECT_DETAIL3,
              "The selection will be returned when the execute function is "
              "called on the handler.");
REGISTER_HELP(TABLE_SELECT_DETAIL4,
              "This handler will retrieve only the columns specified on the "
              "columns list for each included record.");
REGISTER_HELP(TABLE_SELECT_DETAIL5,
              "Each column on the list should be a string identifying the "
              "column name, alias is supported.");

/**
 * $(TABLE_SELECT_BRIEF)
 *
 * ### Full Syntax
 *
 * <code>
 *   <table border = "0">
 *     <tr><td>Table</td><td>.select(...)</td></tr>
 *     <tr><td></td><td>[.where(expression)]</td></tr>
 */
#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.groupBy(...)[.having(condition)]]</td></tr>*/
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.group_by(...)[.having(condition)]]</td></tr>*/
#endif
#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.orderBy(...)]</td></tr>*/
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.order_by(...)]</td></tr>*/
#endif
/**
 *     <tr><td></td><td>[.limit(numberOfRows)[.offset(numberOfRows)]]</td></tr>
 */
#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.lockShared(lockContention)|.lockExclusive(lockContention)]</td></tr>*/
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.lock_shared(lockContention)|.lock_exclusive(lockContention)]</td></tr>*/
#endif
/**
 *     <tr><td></td><td>[.bind(name, value)]</td></tr>
 *     <tr><td></td><td>.execute()</td></tr>
 *   </table>
 * </code>
 *
 * #### .select()
 *
 * $(TABLESELECT_SELECT_DETAIL)
 *
 * $(TABLESELECT_SELECT_DETAIL1)
 *
 * #### .where(...)
 *
 * $(TABLESELECT_WHERE_DETAIL)
 */

#if DOXYGEN_JS
/**
 *
 * #### .groupBy(...)
 *
 */
#elif DOXYGEN_PY
/**
 *
 * #### .group_by(...)
 *
 */
#endif

/**
 *
 * $(TABLESELECT_GROUPBY_DETAIL)
 *
 * #### .having(condition)
 *
 * $(TABLESELECT_HAVING_DETAIL)
 */

#if DOXYGEN_JS
/**
 *
 * #### .orderBy(...)
 *
 */
#elif DOXYGEN_PY
/**
 *
 * #### .order_by(...)
 *
 */
#endif

/**
 * ##### Overloads
 */
#if DOXYGEN_JS
/**
 *
 * @li orderBy(sortCriterion[, sortCriterion, ...])");
 * @li orderBy(sortCriteria)");
 *
 */
#elif DOXYGEN_PY
/**
 *
 * @li order_by(sortCriterion[, sortCriterion, ...])");
 * @li order_by(sortCriteria)");
 *
 */
#endif
/**
 * $(TABLESELECT_ORDERBY_DETAIL)
 *
 * $(TABLESELECT_ORDERBY_DETAIL1)
 *
 * $(TABLESELECT_ORDERBY_DETAIL2)
 *
 * $(TABLESELECT_ORDERBY_DETAIL3)
 *
 * #### .limit(numberOfRows)
 *
 * $(TABLESELECT_LIMIT_DETAIL)
 *
 * #### .offset(numberOfRows)
 *
 * $(TABLESELECT_SKIP_DETAIL)
 */

#if DOXYGEN_JS
/**
 *
 * #### .lockShared(lockContention)
 *
 */
#elif DOXYGEN_PY
/**
 *
 * #### .lock_shared(lockContention)
 *
 */
#endif
/**
 * $(TABLESELECT_LOCKSHARED_DETAIL)
 *
 * $(TABLESELECT_LOCKSHARED_DETAIL1)
 *
 * $(TABLESELECT_LOCKSHARED_DETAIL2)
 *
 * $(TABLESELECT_LOCKSHARED_DETAIL3)
 * $(TABLESELECT_LOCKSHARED_DETAIL4)
 * $(TABLESELECT_LOCKSHARED_DETAIL5)
 * $(TABLESELECT_LOCKSHARED_DETAIL6)
 *
 * $(TABLESELECT_LOCKSHARED_DETAIL7)
 * $(TABLESELECT_LOCKSHARED_DETAIL8)
 * $(TABLESELECT_LOCKSHARED_DETAIL9)
 * $(TABLESELECT_LOCKSHARED_DETAIL10)
 *
 * $(TABLESELECT_LOCKSHARED_DETAIL11)
 *
 * $(TABLESELECT_LOCKSHARED_DETAIL12)
 *
 * $(TABLESELECT_LOCKSHARED_DETAIL13)
 *
 * $(TABLESELECT_LOCKSHARED_DETAIL14)
 */
#if DOXYGEN_JS
/**
 *
 * #### .lockExclusive(lockContention)
 *
 */
#elif DOXYGEN_PY
/**
 *
 * #### .lock_exclusive(lockContention)
 *
 */
#endif

/**
 *
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL)
 *
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL1)
 *
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL2)
 *
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL3)
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL4)
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL5)
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL6)
 *
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL7)
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL8)
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL9)
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL10)
 *
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL11)
 *
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL12)
 *
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL13)
 *
 * $(TABLESELECT_LOCKEXCLUSIVE_DETAIL14)
 *
 * #### .bind(name, value)
 *
 * $(TABLESELECT_BIND_DETAIL)
 *
 * $(TABLESELECT_BIND_DETAIL1)
 *
 * $(TABLESELECT_BIND_DETAIL2)
 *
 * #### .execute()
 *
 * $(TABLESELECT_EXECUTE_BRIEF)
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
 * $(TABLE_SELECT_RETURNS)
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
REGISTER_HELP_FUNCTION(isView, Table);
REGISTER_HELP(
    TABLE_ISVIEW_BRIEF,
    "Indicates whether this Table object represents a View on the database.");
REGISTER_HELP(TABLE_ISVIEW_RETURNS,
              "@returns True if the Table represents a View on the database, "
              "False if represents a Table.");

/**
 * $(TABLE_ISVIEW_BRIEF)
 *
 * $(TABLE_ISVIEW_RETURNS)
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
