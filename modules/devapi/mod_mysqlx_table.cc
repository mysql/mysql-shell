/*
 * Copyright (c) 2015, 2020, Oracle and/or its affiliates.
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
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/utils_help.h"

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
  expose("count", &Table::count);
}

Table::~Table() {}

// Documentation of insert function
REGISTER_HELP_FUNCTION(insert, Table);
REGISTER_HELP(TABLE_INSERT_CHAINED, "TableInsert.insert.[values].execute");
REGISTER_HELP_FUNCTION_TEXT(TABLE_INSERT, R"*(
Creates <b>TableInsert</b> object to insert new records into the table.

The TableInsert class has several functions that allow specifying the way the
insertion occurs.

The insertion is done when the execute() method is called on the handler.
)*");

/**
 * $(TABLE_INSERT_BRIEF)
 *
 * ### Full Syntax
 *
 * <code>
 *   <table border = "0">
 *     <tr><td>Table</td><td>.insert(...)</td></tr>
 *     <tr><td></td><td>[.values(value[, value, ...])]</td></tr>
 *     <tr><td></td><td>.execute()</td></tr>
 *   </table>
 * </code>
 *
 * #### .insert(...)
 *
 * ##### Overloads
 *
 * @li insert$(TABLEINSERT_INSERT_SIGNATURE)
 * @li insert$(TABLEINSERT_INSERT_SIGNATURE1)
 * @li insert$(TABLEINSERT_INSERT_SIGNATURE2)
 * @li insert$(TABLEINSERT_INSERT_SIGNATURE3)
 *
 * $(TABLEINSERT_INSERT_DETAIL)
 *
 * $(TABLEINSERT_INSERT_DETAIL1)
 *
 * $(TABLEINSERT_INSERT_DETAIL2)
 * $(TABLEINSERT_INSERT_DETAIL3)
 * $(TABLEINSERT_INSERT_DETAIL4)
 * $(TABLEINSERT_INSERT_DETAIL5)
 *
 * $(TABLEINSERT_INSERT_DETAIL6)
 *
 * $(TABLEINSERT_INSERT_DETAIL7)
 *
 * $(TABLEINSERT_INSERT_DETAIL8)
 *
 * #### .values(value[, value, ...])
 *
 * $(TABLEINSERT_VALUES)
 *
 * #### .execute()
 *
 * $(TABLEINSERT_EXECUTE_BRIEF)
 *
 * \sa TableInsert
 *
 * ### Examples
 */
#if DOXYGEN_JS
/**
 * #### Inserting values without specifying the column names
 * \snippet mysqlx_table_insert.js TableInsert: insert()
 *
 * #### The column names given as a list of strings
 * \snippet mysqlx_table_insert.js TableInsert: insert(list)
 *
 * #### The column names given as a sequence of strings
 * \snippet mysqlx_table_insert.js TableInsert: insert(str...)
 *
 * #### The column names and corresponding values given as a JSON document
 * \snippet mysqlx_table_insert.js TableInsert: insert(JSON)
 */
TableInsert Table::insert(...) {}
#elif DOXYGEN_PY
/**
 * #### Inserting values without specifying the column names
 * \snippet mysqlx_table_insert.py TableInsert: insert()
 *
 * #### The column names given as a list of strings
 * \snippet mysqlx_table_insert.py TableInsert: insert(list)
 *
 * #### The column names given as a sequence of strings
 * \snippet mysqlx_table_insert.py TableInsert: insert(str...)
 *
 * #### The column names and corresponding values given as a JSON document
 * \snippet mysqlx_table_insert.py TableInsert: insert(JSON)
 */
TableInsert Table::insert(...) {}
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
    "TableUpdate.update.set.[where].[<<<orderBy>>>].[limit].[bind].execute");
REGISTER_HELP(TABLE_UPDATE_DETAIL,
              "This function creates a TableUpdate object which is a record "
              "update handler.");
REGISTER_HELP(
    TABLE_UPDATE_DETAIL1,
    "The TableUpdate class has several functions that allow specifying the way "
    "the update occurs.");
REGISTER_HELP(
    TABLE_UPDATE_DETAIL2,
    "The update is done when the execute() function is called on the handler.");

/**
 * $(TABLE_UPDATE_BRIEF)
 *
 * ### Full Syntax
 *
 * <code>
 *   <table border = "0">
 *     <tr><td>Table</td><td>.update()</td></tr>
 *     <tr><td></td><td>.set(attribute, value)</td></tr>
 *     <tr><td></td><td>[.where(expression)]</td></tr>
 */
#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.orderBy(...)]</td></tr>
 */
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.order_by(...)]</td></tr>
 */
#endif
/**
 *     <tr><td></td><td>[.limit(numberOfRows)]</td></tr>
 *     <tr><td></td><td>[.bind(name, value)]</td></tr>
 *     <tr><td></td><td>.execute()</td></tr>
 *   </table>
 * </code>
 *
 * #### .update()
 *
 * $(TABLEUPDATE_UPDATE_BRIEF)
 *
 * #### .set(attribute, value)
 *
 * $(TABLEUPDATE_SET)
 *
 * The expression also can be used for \a [Parameter
 * Binding](param_binding.html).
 *
 * #### .where(expression)
 *
 * $(TABLEUPDATE_WHERE_DETAIL)
 *
 * The <b>expression</b> supports \a [Parameter Binding](param_binding.html).
 *
 */
#if DOXYGEN_JS
/**
 * #### .orderBy(...)
 *
 * ##### Overloads
 *
 * @li orderBy$(TABLEUPDATE_ORDERBY_SIGNATURE)
 * @li orderBy$(TABLEUPDATE_ORDERBY_SIGNATURE1)
 */
#elif DOXYGEN_PY
/**
 * #### .order_by(...)
 *
 * ##### Overloads
 *
 * @li order_by$(TABLEUPDATE_ORDERBY_SIGNATURE)
 * @li order_by$(TABLEUPDATE_ORDERBY_SIGNATURE1)
 */
#endif
/**
 *
 * $(TABLEUPDATE_ORDERBY_DETAIL)
 *
 * $(TABLEUPDATE_ORDERBY_DETAIL1)
 *
 * $(TABLEUPDATE_ORDERBY_DETAIL2)
 *
 * $(TABLEUPDATE_ORDERBY_DETAIL3)
 *
 * #### .limit(numberOfRows)
 *
 * $(TABLEUPDATE_LIMIT_DETAIL)
 *
 * $(TABLEUPDATE_LIMIT_DETAIL1)
 *
 * #### .bind(name, value)
 *
 * $(TABLEUPDATE_BIND_DETAIL)
 *
 * $(TABLEUPDATE_BIND_DETAIL1)
 *
 * $(TABLEUPDATE_BIND_DETAIL2)
 *
 * #### .execute()
 *
 * $(TABLEDELETE_EXECUTE_BRIEF)
 *
 * \sa TableUpdate
 *
 * ### Examples
 */
#if DOXYGEN_JS
/**
 * #### Updating a single field in a record
 * \snippet mysqlx_table_update.js TableUpdate: simple test
 *
 * #### Updating a single field using expressions
 * \snippet mysqlx_table_update.js TableUpdate: expression
 *
 * #### Updating a single field using expressions and parameter binding
 * \snippet mysqlx_table_update.js TableUpdate: limits
 *
 * #### Updating a view
 * \snippet mysqlx_table_update.js TableUpdate: view
 */
TableUpdate Table::update() {}
#elif DOXYGEN_PY
/**
 * #### Updating a single field in a record
 * \snippet mysqlx_table_update.py TableUpdate: simple test
 *
 * #### Updating a single field using expressions
 * \snippet mysqlx_table_update.py TableUpdate: expression
 *
 * #### Updating a single field using expressions and parameter binding
 * \snippet mysqlx_table_update.py TableUpdate: limits
 *
 * #### Updating a view
 * \snippet mysqlx_table_update.py TableUpdate: view
 */
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
    "TableDelete.delete.[where].[<<<orderBy>>>].[limit].[bind].execute");

REGISTER_HELP(TABLE_DELETE_DETAIL,
              "This function creates a TableDelete object which is a record "
              "deletion handler.");
REGISTER_HELP(
    TABLE_DELETE_DETAIL1,
    "The TableDelete class has several functions that allow specifying what "
    "should be deleted and how.");
REGISTER_HELP(TABLE_DELETE_DETAIL2,
              "The deletion is done when the execute() function is called on "
              "the handler.");

/**
 * $(TABLE_DELETE_BRIEF)
 *
 * ### Full Syntax
 *
 * <code>
 *   <table border = "0">
 *     <tr><td>Table</td><td>.delete()</td></tr>
 *     <tr><td></td><td>[.where(expression)]</td></tr>
 */
#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.orderBy(...)]</td></tr>
 */
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.order_by(...)]</td></tr>
 */
#endif
/**
 *     <tr><td></td><td>[.limit(numberOfRows)]</td></tr>
 *     <tr><td></td><td>[.bind(name, value)]</td></tr>
 *     <tr><td></td><td>.execute()</td></tr>
 *   </table>
 * </code>
 *
 * #### .delete()
 *
 * $(TABLEDELETE_DELETE_BRIEF)
 *
 * #### .where(expression)
 *
 * $(TABLEDELETE_WHERE_DETAIL)
 *
 * The <b>expression</b> supports \a [Parameter Binding](param_binding.html).
 *
 */
#if DOXYGEN_JS
/**
 * #### .orderBy(...)
 *
 * ##### Overloads
 *
 * @li orderBy$(TABLEDELETE_ORDERBY_SIGNATURE)
 * @li orderBy$(TABLEDELETE_ORDERBY_SIGNATURE1)
 */
#elif DOXYGEN_PY
/**
 * #### .order_by(...)
 *
 * ##### Overloads
 *
 * @li order_by$(TABLEDELETE_ORDERBY_SIGNATURE)
 * @li order_by$(TABLEDELETE_ORDERBY_SIGNATURE1)
 */
#endif
/**
 *
 * $(TABLEDELETE_ORDERBY_DETAIL)
 *
 * $(TABLEDELETE_ORDERBY_DETAIL1)
 *
 * $(TABLEDELETE_ORDERBY_DETAIL2)
 *
 * $(TABLEDELETE_ORDERBY_DETAIL3)
 *
 * #### .limit(numberOfRows)
 *
 * $(TABLEDELETE_LIMIT_DETAIL)
 *
 * $(TABLEDELETE_LIMIT_DETAIL1)
 *
 * #### .bind(name, value)
 *
 * $(TABLEDELETE_BIND_DETAIL)
 *
 * $(TABLEDELETE_BIND_DETAIL1)
 *
 * $(TABLEDELETE_BIND_DETAIL2)
 *
 * #### .execute()
 *
 * $(TABLEDELETE_EXECUTE_BRIEF)
 *
 * \sa TableDelete
 *
 * ### Examples
 */
#if DOXYGEN_JS
/**
 * #### Deleting records with a condition
 * \snippet mysqlx_table_delete.js TableDelete: delete under condition
 *
 * #### Deleting records with a condition and parameter binding
 * \snippet mysqlx_table_delete.js TableDelete: delete with binding
 *
 * #### Deleting all records using a view
 * \snippet mysqlx_table_delete.js TableDelete: full delete
 *
 * #### Deleting records with a limit
 * \snippet mysqlx_table_delete.js TableDelete: with limit
 */
TableDelete Table::delete () {}
#elif DOXYGEN_PY
/**
 * #### Deleting records with a condition
 * \snippet mysqlx_table_delete.py TableDelete: delete under condition
 *
 * #### Deleting records with a condition and parameter binding
 * \snippet mysqlx_table_delete.py TableDelete: delete with binding
 *
 * #### Deleting all records using a view
 * \snippet mysqlx_table_delete.py TableDelete: full delete
 *
 * #### Deleting records with a limit
 * \snippet mysqlx_table_delete.py TableDelete: with limit
 */
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
    "execute");

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
REGISTER_HELP(TABLE_SELECT_DETAIL1,
              "This handler will retrieve the selected columns for each "
              "included record.");
REGISTER_HELP(TABLE_SELECT_DETAIL2,
              "The TableSelect class has several functions that allow "
              "specifying what records should be retrieved from the table.");
REGISTER_HELP(TABLE_SELECT_DETAIL3,
              "The selection will be returned when the execute() function is "
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
 * <tr><td></td><td>[.groupBy(...)[.having(condition)]]</td></tr>
 */
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.group_by(...)[.having(condition)]]</td></tr>
 */
#endif
#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.orderBy(...)]</td></tr>
 */
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.order_by(...)]</td></tr>
 */
#endif
/**
 *     <tr><td></td><td>[.limit(numberOfRows)[.offset(numberOfRows)]]</td></tr>
 */
#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.lockShared(lockContention)|.lockExclusive(lockContention)]</td></tr>
 */
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.lock_shared(lockContention)|.lock_exclusive(lockContention)]</td></tr>
 */
#endif
/**
 *     <tr><td></td><td>[.bind(name, value)]</td></tr>
 *     <tr><td></td><td>.execute()</td></tr>
 *   </table>
 * </code>
 *
 * #### .select(...)
 *
 * ##### Overloads
 *
 * @li select$(TABLESELECT_SELECT_SIGNATURE)
 * @li select$(TABLESELECT_SELECT_SIGNATURE1)
 * @li select$(TABLESELECT_SELECT_SIGNATURE2)
 *
 * $(TABLESELECT_SELECT_DETAIL)
 *
 * $(TABLESELECT_SELECT_DETAIL1)
 *
 * $(TABLESELECT_SELECT_DETAIL2)
 *
 * #### .where(expression)
 *
 * $(TABLESELECT_WHERE_DETAIL)
 *
 * The <b>expression</b> supports \a [Parameter Binding](param_binding.html).
 *
 */
#if DOXYGEN_JS
/**
 * #### .groupBy(...)
 *
 * ##### Overloads
 *
 * @li groupBy$(TABLESELECT_GROUPBY_SIGNATURE)
 * @li groupBy$(TABLESELECT_GROUPBY_SIGNATURE1)
 */
#elif DOXYGEN_PY
/**
 * #### .group_by(...)
 *
 * ##### Overloads
 *
 * @li group_by$(TABLESELECT_GROUPBY_SIGNATURE)
 * @li group_by$(TABLESELECT_GROUPBY_SIGNATURE1)
 */
#endif
/**
 *
 * $(TABLESELECT_GROUPBY_BRIEF)
 *
 * #### .having(condition)
 *
 * $(TABLESELECT_HAVING_DETAIL)
 *
 * The <b>condition</b> supports \a [Parameter Binding](param_binding.html).
 *
 */
#if DOXYGEN_JS
/**
 * #### .orderBy(...)
 *
 * ##### Overloads
 *
 * @li orderBy$(TABLESELECT_ORDERBY_SIGNATURE)
 * @li orderBy$(TABLESELECT_ORDERBY_SIGNATURE1)
 */
#elif DOXYGEN_PY
/**
 * #### .order_by(...)
 *
 * ##### Overloads
 *
 * @li order_by$(TABLESELECT_ORDERBY_SIGNATURE)
 * @li order_by$(TABLESELECT_ORDERBY_SIGNATURE1)
 */
#endif
/**
 *
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
 * $(TABLESELECT_LIMIT_DETAIL1)
 *
 * #### .offset(numberOfRows)
 *
 * $(TABLESELECT_OFFSET_DETAIL)
 *
 */
#if DOXYGEN_JS
/**
 * #### .lockShared(lockContention)
 */
#elif DOXYGEN_PY
/**
 * #### .lock_shared(lockContention)
 */
#endif
/**
 *
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
 *
 */
#if DOXYGEN_JS
/**
 * #### .lockExclusive(lockContention)
 */
#elif DOXYGEN_PY
/**
 * #### .lock_exclusive(lockContention)
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
 *
 * \sa TableSelect
 *
 * ### Examples
 */
#if DOXYGEN_JS
/**
 * #### Fetching all the records
 * \snippet mysqlx_table_select.js Table.Select All
 *
 * #### Fetching records matching specified criteria
 * \snippet mysqlx_table_select.js Table.Select Filtering
 *
 * #### Selecting which columns to fetch
 * \snippet mysqlx_table_select.js Table.Select Field Selection
 *
 * #### Sorting the results
 * \snippet mysqlx_table_select.js Table.Select Sorting
 *
 * #### Setting limit and offset
 * \snippet mysqlx_table_select.js Table.Select Limit and Offset
 *
 * #### Using parameter binding
 * \snippet mysqlx_table_select.js Table.Select Parameter Binding
 */
TableSelect Table::select(...) {}
#elif DOXYGEN_PY
/**
 * #### Fetching all the records
 * \snippet mysqlx_table_select.py Table.Select All
 *
 * #### Fetching records matching specified criteria
 * \snippet mysqlx_table_select.py Table.Select Filtering
 *
 * #### Selecting which columns to fetch
 * \snippet mysqlx_table_select.py Table.Select Field Selection
 *
 * #### Sorting the results
 * \snippet mysqlx_table_select.py Table.Select Sorting
 *
 * #### Setting limit and offset
 * \snippet mysqlx_table_select.py Table.Select Limit and Offset
 *
 * #### Using parameter binding
 * \snippet mysqlx_table_select.py Table.Select Parameter Binding
 */
TableSelect Table::select(...) {}
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

// Documentation of isView function
REGISTER_HELP_FUNCTION(count, Table);
REGISTER_HELP(TABLE_COUNT_BRIEF, "Returns the number of records in the table.");
/**
 * $(TABLE_COUNT_BRIEF)
 */
#if DOXYGEN_JS
Bool Table::count() {}
#elif DOXYGEN_PY
Bool Table::count() {}
#endif
