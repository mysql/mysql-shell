/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/devapi/mod_mysqlx_table_select.h"
#include <memory>
#include <string>
#include <vector>
#include "db/mysqlx/mysqlx_parser.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_table.h"
#include "modules/devapi/base_constants.h"
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
  register_dynamic_function(F::select,F::_empty);
  register_dynamic_function(F::where, F::select);
  register_dynamic_function(F::groupBy, F::select | F::where);
  register_dynamic_function(F::having, F::groupBy);
  register_dynamic_function(F::orderBy, F::select | F::where | F::groupBy | F::having);
  register_dynamic_function(F::limit, F::select | F::where | F::groupBy | F::having | F::orderBy);
  register_dynamic_function(F::offset, F::limit);
  register_dynamic_function(F::lockShared, F::select | F::where | F::groupBy | F::having | F::orderBy | F::offset | F::limit);
  register_dynamic_function(F::lockExclusive, F::select | F::where | F::groupBy | F::having | F::orderBy | F::offset | F::limit);
  register_dynamic_function(F::bind, F::select | F::where | F::groupBy | F::having | F::orderBy | F::offset | F::limit | F::lockShared | F::lockExclusive | F::bind);
  register_dynamic_function(F::execute, F::select | F::where | F::groupBy | F::having | F::orderBy | F::offset | F::limit | F::lockShared | F::lockExclusive | F::bind);
  register_dynamic_function(F::__shell_hook__, F::select | F::where | F::groupBy | F::having | F::orderBy | F::offset | F::limit | F::lockShared | F::lockExclusive | F::bind);

  // Initial function update
  update_functions(F::_empty);
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
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
*/
#endif
/**
 */
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
*/
#endif
/**
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
      update_functions(F::select);
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
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
*/
#endif
/**
 */
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
*/
#endif
/**
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

    update_functions(F::where);
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
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
*/
#endif
/**
 */
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
*/
#endif
/**
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

    update_functions(F::groupBy);
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
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
*/
#endif
/**
 */
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
*/
#endif
/**
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

    update_functions(F::having);
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
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
*/
#endif
/**
 */
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
*/
#endif
/**
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

    update_functions(F::orderBy);
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
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
*/
#endif
/**
 */
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
*/
#endif
/**
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

    update_functions(F::limit);
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

    update_functions(F::offset);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("offset"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

void TableSelect::set_lock_contention(const shcore::Argument_list &args) {
  std::string lock_contention;
  if (args.size() == 1) {
    if (args[0].type == shcore::Object) {
      std::shared_ptr<Constant> constant =
          std::dynamic_pointer_cast<Constant>(args.object_at(0));
      if (constant && constant->group() == "LockContention")
        lock_contention = constant->data().as_string();
    } else if (args[0].type == shcore::String) {
      lock_contention = args.string_at(0);
    }

    if (!shcore::str_casecmp(lock_contention.c_str(), "nowait")) {
      message_.set_locking_options(Mysqlx::Crud::Find_RowLockOptions_NOWAIT);
    } else if (!shcore::str_casecmp(lock_contention.c_str(), "skip_lock")) {
      message_.set_locking_options(Mysqlx::Crud::Find_RowLockOptions_SKIP_LOCKED);
    } else if (shcore::str_casecmp(lock_contention.c_str(), "default")) {
        throw shcore::Exception::argument_error(
            "Argument #1 is expected to be one of DEFAULT, NOWAIT or "
            "SKIP_LOCK");
    }
  }
}

REGISTER_HELP(TABLESELECT_LOCK_SHARED_BRIEF,
              "Instructs the server to acquire shared row locks in documents "
              "matched by this find operation.");
REGISTER_HELP(
    TABLESELECT_LOCK_SHARED_PARAM,
    "@param lockContention optional parameter to indicate how to handle rows "
    "that are already locked.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_RETURNS,
              "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL,
              "When this function is called, the selected rows will be"
              "locked for write operations, they may be retrieved on a "
              "different session, but no updates will be allowed.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL1,
              "The acquired locks will be released when the current "
              "transaction is commited or rolled back.");

REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL2,
              "The lockContention parameter defines the behavior of the "
              "operation if another session contains an exlusive lock to "
              "matching rows.");

REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL3,
              "The lockContention can be specified using the following "
              "constants:");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL4,
              "@li mysqlx.LockContention.DEFAULT");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL5,
              "@li mysqlx.LockContention.NOWAIT");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL6,
              "@li mysqlx.LockContention.SKIP_LOCK");

REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL7,
              "The lockContention can also be specified using the following "
              "string literals (no case sensitive):");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL8, "@li 'DEFAULT'");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL9, "@li 'NOWAIT'");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL10, "@li 'SKIP_LOCK'");

REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL11,
              "If no lockContention or the default is specified, the operation "
              "will block if another session already holds an exclusive lock "
              "on matching rows until the lock is released.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL12,
              "If lockContention is set to NOWAIT and another session "
              "already holds an exclusive lock on matching rows, the "
              "operation will not block and an error will be generated.");
REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL13,
              "If lockContention is set to SKIP_LOCK and another session "
              "already holds an exclusive lock on matching rows, the "
              "operation will not block and will return only those rows "
              "not having an exclusive lock.");

REGISTER_HELP(TABLESELECT_LOCK_SHARED_DETAIL14,
              "This operation only makes sense within a transaction.");

/**
 * $(TABLESELECT_LOCK_SHARED_BRIEF)
 *
 * $(TABLESELECT_LOCK_SHARED_PARAM)
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
 * $(TABLESELECT_LOCK_SHARED_DETAIL4)
 * $(TABLESELECT_LOCK_SHARED_DETAIL5)
 * $(TABLESELECT_LOCK_SHARED_DETAIL6)
 *
 * $(TABLESELECT_LOCK_SHARED_DETAIL7)
 * $(TABLESELECT_LOCK_SHARED_DETAIL8)
 * $(TABLESELECT_LOCK_SHARED_DETAIL9)
 * $(TABLESELECT_LOCK_SHARED_DETAIL10)
 *
 * $(TABLESELECT_LOCK_SHARED_DETAIL11)
 *
 * $(TABLESELECT_LOCK_SHARED_DETAIL12)
 *
 * $(TABLESELECT_LOCK_SHARED_DETAIL13)
 *
 * $(TABLESELECT_LOCK_SHARED_DETAIL14)
 *
 * #### Method Chaining
 *
 * This function can be invoked at any time before bind or execute are called.
 *
 * After this function invocation, the following functions can be invoked:
 *
 */
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
*/
#endif
/**
 * - bind(String name, Value value)
 * - execute()
 *
 * If lockExclusive() is called, it will override the lock type to be used on
 * on the selected documents.
 */
//@{
#if DOXYGEN_JS
TableSelect TableSelect::lockShared(String lockContention) {
}
#elif DOXYGEN_PY
TableSelect TableSelect::lock_shared(str lockContention) {
}
#endif
//@}
shcore::Value TableSelect::lock_shared(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("lockShared").c_str());

  try {
    message_.set_locking(Mysqlx::Crud::Find_RowLock_SHARED_LOCK);

    set_lock_contention(args);

    update_functions(F::lockShared);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("lockShared"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_BRIEF,
              "Instructs the server to acquire an exclusive lock on rows "
              "matched by this find operation.");
REGISTER_HELP(
    TABLESELECT_LOCK_EXCLUSIVE_PARAM,
    "@param lockContention optional parameter to indicate how to handle "
    "rows that are already locked.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_RETURNS,
              "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL,
              "When this function is called, the selected rows will be"
              "locked for read operations, they will not be retrievable by "
              "other session.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL1,
              "The acquired locks will be released when the current "
              "transaction is commited or rolled back.");

REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL2,
              "The lockContention parameter defines the behavior of the "
              "operation if another session contains a lock to matching "
              "rows.");

REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL3,
              "The lockContention can be specified using the following "
              "constants:");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL4,
              "@li mysqlx.LockContention.DEFAULT");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL5,
              "@li mysqlx.LockContention.NOWAIT");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL6,
              "@li mysqlx.LockContention.SKIP_LOCK");

REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL7,
              "The lockContention can also be specified using the following "
              "string literals (no case sensitive):");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL8, "@li 'DEFAULT'");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL9, "@li 'NOWAIT'");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL10, "@li 'SKIP_LOCK'");

REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL11,
              "If no lockContention or the default is specified, the operation "
              "will block if another session already holds a lock on matching "
              "rows until the lock is released.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL12,
              "If lockContention is set to NOWAIT and another session "
              "already holds a lock on matching rows, the operation will not "
              "block and an error will be generated.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL13,
              "If lockContention is set to SKIP_LOCK and another session "
              "already holds a lock on matching rows, the operation will not "
              "block and will return only those rows not having an exclusive "
              "lock.");
REGISTER_HELP(TABLESELECT_LOCK_EXCLUSIVE_DETAIL14,
              "This operation only makes sense within a transaction.");

/**
 * $(TABLESELECT_LOCK_EXCLUSIVE_BRIEF)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_PARAM)
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
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL4)
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL5)
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL6)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL7)
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL8)
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL9)
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL10)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL11)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL12)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL13)
 *
 * $(TABLESELECT_LOCK_EXCLUSIVE_DETAIL14)
 *
 * #### Method Chaining
 *
 * This function can be invoked at any time before bind or execute are called.
 *
 * After this function invocation, the following functions can be invoked:
 *
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
*/
#endif
/**
 * - bind(String name, Value value)
 * - execute()
 *
 * If lockShared() is called, it will override the lock type to be used on
 * on the selected documents.
 */
//@{
#if DOXYGEN_JS
TableSelect TableSelect::lockExclusive(String lockContention) {
}
#elif DOXYGEN_PY
TableSelect TableSelect::lock_exclusive(str lockContention) {
}
#endif
//@}
shcore::Value TableSelect::lock_exclusive(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("lockExclusive").c_str());

  try {
    message_.set_locking(Mysqlx::Crud::Find_RowLock_EXCLUSIVE_LOCK);

    set_lock_contention(args);

    update_functions(F::lockExclusive);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("lockExclusive"));

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

    update_functions(F::bind);
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
