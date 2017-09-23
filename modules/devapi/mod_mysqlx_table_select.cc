/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/devapi/mod_mysqlx_table_select.h"
#include <memory>
#include <string>
#include <vector>
#include "db/mysqlx/mysqlx_parser.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_table.h"
#include "scripting/common.h"
#include "utils/utils_time.h"
#include "shellcore/utils_help.h"


using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

TableSelect::TableSelect(std::shared_ptr<Table> owner)
    : Table_crud_definition(std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::TABLE);

  // Exposes the methods available for chaining
  add_method("select", std::bind(&TableSelect::select, this, _1), "data");
  add_method("where", std::bind(&TableSelect::where, this, _1), "data");
  add_method("groupBy", std::bind(&TableSelect::group_by, this, _1), "data");
  add_method("having", std::bind(&TableSelect::having, this, _1), "data");
  add_method("orderBy", std::bind(&TableSelect::order_by, this, _1), "data");
  add_method("limit", std::bind(&TableSelect::limit, this, _1), "data");
  add_method("offset", std::bind(&TableSelect::offset, this, _1), "data");
  add_method("bind", std::bind(&TableSelect::bind, this, _1), "data");
  add_method("lockShared", std::bind(&TableSelect::lock_shared, this, _1),
             NULL);
  add_method("lockExclusive", std::bind(&TableSelect::lock_exclusive, this, _1),
             NULL);

  // Registers the dynamic function behavior
  register_dynamic_function("select", "");
  register_dynamic_function("where", "select");
  register_dynamic_function("groupBy", "select, where");
  register_dynamic_function("having", "groupBy");
  register_dynamic_function("orderBy", "select, where, groupBy, having");
  register_dynamic_function("limit", "select, where, groupBy, having, orderBy");
  register_dynamic_function("offset", "limit");
  register_dynamic_function(
      "lockShared", "select, where, groupBy, having, orderBy, offset, limit");
  register_dynamic_function(
      "lockExclusive",
      "select, where, groupBy, having, orderBy, offset, limit");
  register_dynamic_function(
      "bind",
      "select, where, groupBy, having, orderBy, offset, limit, "
      "lockShared, lockExclusive, bind");
  register_dynamic_function(
      "execute",
      "select, where, groupBy, having, orderBy, offset, limit, lockShared, "
      "lockExclusive, bind");
  register_dynamic_function(
      "__shell_hook__",
      "select, where, groupBy, having, orderBy, offset, limit, lockShared, "
      "lockExclusive, bind");

  // Initial function update
  update_functions("");
}

//! Initializes this record selection handler.
#if DOXYGEN_CPP
//! \param args should contain an optional list of string expressions
//! identifying the columns to be retrieved, alias support is enabled on these
//! fields.
#else
//! \param searchExprStr: An optional list of string expressions identifying the
//! columns to be retrieved, alias support is enabled on these fields.
#endif
/**
* \return This TableSelect object.
*
* The TableSelect handler will only retrieve the columns that were included on
* the filter, if no filter was set then all the columns will be included.
*
* Calling this function is allowed only for the first time and it is done
* automatically when Table.select() is called.
*
* #### Method Chaining
*
* After this function invocation, the following functions can be invoked:
*
* - where(String searchCriteria)
* - groupBy(List searchExprStr)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableSelect TableSelect::select(List searchExprStr) {}
#elif DOXYGEN_PY
TableSelect TableSelect::select(list searchExprStr) {}
#endif
shcore::Value TableSelect::select(const shcore::Argument_list &args) {
  std::shared_ptr<Table> table(std::static_pointer_cast<Table>(_owner));

  if (table) {
    try {
      std::vector<std::string> fields;

      if (args.size()) {
        parse_string_list(args, fields);

        if (fields.size() == 0)
          throw shcore::Exception::argument_error(
              "Field selection criteria can not be empty");
      }

      for (auto &field : fields) {
        ::mysqlx::parser::parse_table_column_list_with_alias(
            *message_.mutable_projection(), field);
      }

      // Updates the exposed functions
      update_functions("select");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("select"));
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets the search condition to filter the records to be retrieved from the
//! owner Table.
#if DOXYGEN_CPP
//! \param args should contain an optional expression to filter the records to
//! be retrieved.
#else
//! \param searchCondition: An optional expression to filter the records to be
//! retrieved.
#endif
/**
* if not specified all the records will be retrieved from the table unless a
* limit is set.
* \return This TableSelect object.
*
* The searchCondition supports \a [Parameter Binding](param_binding.html).
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - select(List columns)
*
* After this function invocation, the following functions can be invoked:
*
* - groupBy(List searchExprStr)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableSelect TableSelect::where(String searchCondition) {}
#elif DOXYGEN_PY
TableSelect TableSelect::where(str searchCondition) {}
#endif
shcore::Value TableSelect::where(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("where").c_str());

  try {
    message_.set_allocated_criteria(::mysqlx::parser::parse_table_filter(
        args.string_at(0), &_placeholders));

    update_functions("where");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("where"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets a grouping criteria for the resultset.
#if DOXYGEN_CPP
//! \param args should contain a list of string expressions identifying the
//! grouping criteria.
#else
//! \param searchExprStr: A list of string expressions identifying the grouping
//! criteria.
#endif
/**
* \return This TableSelect object.
*
* If used, the TableSelect handler will group the records using the stablished
* criteria.
*
* #### Method Chaining
*
* This function can be invoked only once after:
* - select(List projectedSearchExprStr)
* - where(String searchCondition)
*
* After this function invocation the following functions can be invoked:
*
* - having(String searchCondition)
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableSelect TableSelect::groupBy(List searchExprStr) {}
#elif DOXYGEN_PY
TableSelect TableSelect::group_by(list searchExprStr) {}
#endif
shcore::Value TableSelect::group_by(const shcore::Argument_list &args) {
  args.ensure_at_least(1, get_function_name("groupBy").c_str());

  try {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error(
          "Grouping criteria can not be empty");

    for (auto &field : fields) {
      message_.mutable_grouping()->AddAllocated(
          ::mysqlx::parser::parse_table_filter(field));
    }

    update_functions("groupBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("groupBy"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets a condition for records to be considered in agregate function
//! operations.
#if DOXYGEN_CPP
//! \param args should contain a condition on the agregate functions used on the
//! grouping criteria.
#else
//! \param searchCondition: A condition on the agregate functions used on the
//! grouping criteria.
#endif
/**
* \return This TableSelect object.
*
* If used the TableSelect operation will only consider the records matching the
* stablished criteria.
*
* The searchCondition supports \a [Parameter Binding](param_binding.html).
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - groupBy(List searchExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - orderBy(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableSelect TableSelect::having(String searchCondition) {}
#elif DOXYGEN_PY
TableSelect TableSelect::having(str searchCondition) {}
#endif
shcore::Value TableSelect::having(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("having").c_str());

  try {
    message_.set_allocated_grouping_criteria(
        ::mysqlx::parser::parse_table_filter(args.string_at(0),
                                             &_placeholders));

    update_functions("having");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("having"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets the sorting criteria to be used on the RowResult.
#if DOXYGEN_CPP
//! \param args should contain a list of expression strings defining the sort
//! criteria for the returned records.
#else
//! \param sortExprStr: A list of expression strings defining the sort criteria
//! for the returned records.
#endif
/**
* \return This TableSelect object.
*
* If used the TableSelect handler will return the records sorted with the
* defined criteria.
*
* The elements of sortExprStr list are strings defining the column name on which
* the sorting will be based in the form of "columnIdentifier [ ASC | DESC ]".
* If no order criteria is specified, ascending will be used by default.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - select(List projectedSearchExprStr)
* - where(String searchCondition)
* - groupBy(List searchExprStr)
* - having(String searchCondition)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableSelect TableSelect::orderBy(List sortExprStr) {}
#elif DOXYGEN_PY
TableSelect TableSelect::order_by(list sortExprStr) {}
#endif
shcore::Value TableSelect::order_by(const shcore::Argument_list &args) {
  args.ensure_at_least(1, get_function_name("orderBy").c_str());

  try {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error(
          "Order criteria can not be empty");

    for (auto &field : fields) {
      ::mysqlx::parser::parse_table_sort_column(*message_.mutable_order(),
                                                field);
    }

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("orderBy"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets the maximum number of records to be returned on the select operation.
#if DOXYGEN_CPP
//! \param args should contain the maximum number of records to be retrieved.
#else
//! \param numberOfRows: The maximum number of records to be retrieved.
#endif
/**
* \return This TableSelect object.
*
* If used, the TableSelect operation will return at most numberOfRows records.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - select(List projectedSearchExprStr)
* - where(String searchCondition)
* - groupBy(List searchExprStr)
* - having(String searchCondition)
* - orderBy(List sortExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - offset(Integer limitOffset)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableSelect TableSelect::limit(Integer numberOfRows) {}
#elif DOXYGEN_PY
TableSelect TableSelect::limit(int numberOfRows) {}
#endif
shcore::Value TableSelect::limit(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("limit").c_str());

  try {
    message_.mutable_limit()->set_row_count(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("limit"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Sets number of records to skip on the RowResult when a limit has been
//! defined.
#if DOXYGEN_CPP
//! \param args should contain the number of records to skip before start
//! including them on the Resultset.
#else
//! \param limitOffset: The number of records to skip before start including
//! them on the Resultset.
#endif
/**
* \return This TableSelect object.
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - limit(Integer numberOfRows)
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableSelect TableSelect::offset(Integer limitOffset) {}
#elif DOXYGEN_PY
TableSelect TableSelect::offset(int limitOffset) {}
#endif
shcore::Value TableSelect::offset(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("offset").c_str());

  try {
    message_.mutable_limit()->set_offset(args.uint_at(0));

    update_functions("offset");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("offset"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(TABLESELECT_LOCK_SHARED_BRIEF,
              "Instructs the server to acquire shared row locks in documents "
              "matched by this find operation.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_RETURNS,
              "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL,
              "When this function is called, the selected documents will be"
              "locked for write operations, they may be retrieved on a "
              "different session, but no updates will be allowed.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL1,
              "The acquired locks will be released when the current "
              "transaction is commited or rolled back.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL2,
              "If another session already holds an exclusive lock on the "
              "matching documents, the find will block until the lock is "
              "released.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL3,
              "This operation only makes sense within a transaction.");

/**
 * $(TABLESELECT_LOCK_SHARED_BRIEF)
 *
 * $(TABLESELECT_LOCK_SHARED_RETURNS)
 *
 * $(TABLESELECT_LOCK_SHARED_DETAIL)
 *
 * $(TABLESELECT_LOCK_SHARED_DETAIL1)
 *
 * $(TABLESELECT_LOCK_SHARED_DETAIL2)
 *
 * $(TABLESELECT_LOCK_SHARED_DETAIL3)
 *
 * #### Method Chaining
 *
 * This function can be invoked at any time before bind or execute are called.
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - lockExclusive()
 * - bind(String name, Value value)
 * - execute()
 *
 * If lockExclusive() is called, it will override the lock type to be used on
 * on the selected documents.
 */
//@{
#if DOXYGEN_JS
TableSelect TableSelect::lockShared() {
}
#elif DOXYGEN_PY
TableSelect TableSelect::lock_shared() {
}
#endif
//@}
shcore::Value TableSelect::lock_shared(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("lockShared").c_str());

  try {
    message_.set_locking(Mysqlx::Crud::Find_RowLock_SHARED_LOCK);

    update_functions("lockShared");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.lockShared");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_BRIEF,
              "Instructs the server to acquire an exclusive lock on documents "
              "matched by this find operation.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_RETURNS,
              "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL,
              "When this function is called, the selected documents will be"
              "locked for read operations, they will not be retrievable by "
              "other session.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL1,
              "The acquired locks will be released when the current "
              "transaction is commited or rolled back.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL2,
              "The operation will block if another session already holds a "
              "lock on matching documents (either shared and exclusive).");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL3,
              "This operation only makes sense within a transaction.");

/**
 * $(TABLESELECT_LOCK_EXCLUSIVE_BRIEF)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_RETURNS)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL1)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL2)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL3)
 *
 * #### Method Chaining
 *
 * This function can be invoked at any time before bind or execute are called.
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - lockShared()
 * - bind(String name, Value value)
 * - execute()
 *
 * If lockShared() is called, it will override the lock type to be used on
 * on the selected documents.
 */
//@{
#if DOXYGEN_JS
TableSelect TableSelect::lockExclusive() {
}
#elif DOXYGEN_PY
TableSelect TableSelect::lock_exclusive() {
}
#endif
//@}
shcore::Value TableSelect::lock_exclusive(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("lockExclusive").c_str());

  try {
    message_.set_locking(Mysqlx::Crud::Find_RowLock_EXCLUSIVE_LOCK);

    update_functions("lockExclusive");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect.lockExclusive");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

//! Binds a value to a specific placeholder used on this TableSelect object.
#if DOXYGEN_CPP
//! \param args should contain the next elements:
//! \li The name of the placeholder to which the value will be bound.
//! \li The value to be bound on the placeholder.
#else
//! \param name: The name of the placeholder to which the value will be bound.
//! \param value: The value to be bound on the placeholder.
#endif
/**
* \return This TableSelect object.
*
* #### Method Chaining
*
* This function can be invoked multiple times right before calling execute:
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute()
*
* An error will be raised if the placeholder indicated by name does not exist.
*
* This function must be called once for each used placeohlder or an error will
* be
* raised when the execute method is called.
*
* \sa Usage examples at execute().
*/
#if DOXYGEN_JS
TableSelect TableSelect::bind(String name, Value value) {}
#elif DOXYGEN_PY
TableSelect TableSelect::bind(str name, Value value) {}
#endif

shcore::Value TableSelect::bind(const shcore::Argument_list &args) {
  args.ensure_count(2, get_function_name("bind").c_str());

  try {
    bind_value(args.string_at(0), args[1]);

    update_functions("bind");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("bind"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

/**
* Executes the Find operation with all the configured options and returns.
* \return RowResult A Row result object that can be used to traverse the records
* returned by rge select operation.
*
* #### Method Chaining
*
* This function can be invoked after any other function on this class.
*/
#if DOXYGEN_JS
/**
*
* #### Examples
* \dontinclude "js_devapi/scripts/mysqlx_table_select.js"
* \skip //@ Table.Select All
* \until print('Select Binding Name:', records[0].my_name, '\n');
*/
RowResult TableSelect::execute() {}
#elif DOXYGEN_PY
/**
*
* #### Examples
* \dontinclude "py_devapi/scripts/mysqlx_table_select.py"
* \skip #@ Table.Select All
* \until print 'Select Binding Name:', records[0].name, '\n'
*/
RowResult TableSelect::execute() {}
#endif
shcore::Value TableSelect::execute(const shcore::Argument_list &args) {
  std::unique_ptr<mysqlx::RowResult> result;
  args.ensure_count(0, get_function_name("execute").c_str());
  try {
    MySQL_timer timer;
    insert_bound_values(message_.mutable_args());
    timer.start();
    result.reset(new mysqlx::RowResult(safe_exec(
        [this]() { return session()->session()->execute_crud(message_); })));
    timer.end();
    result->set_execution_time(timer.raw_duration());
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}
