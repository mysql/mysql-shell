/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/devapi/base_constants.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_table.h"
#include "scripting/common.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

REGISTER_HELP_CLASS(TableSelect, mysqlx);
REGISTER_HELP(TABLESELECT_BRIEF, "Operation to retrieve rows from a table.");
REGISTER_HELP(TABLESELECT_DETAIL,
              "A TableSelect represents a query to retrieve rows from a Table. "
              "It is is created through the <b>select</b> function on the "
              "<b>Table</b> class.");
TableSelect::TableSelect(std::shared_ptr<Table> owner)
    : Table_crud_definition(std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::TABLE);

  auto limit_id = F::limit;
  auto offset_id = F::offset;
  auto bind_id = F::bind;
  // Exposes the methods available for chaining
  add_method("select", std::bind(&TableSelect::select, this, _1), "data");
  add_method("where", std::bind(&TableSelect::where, this, _1), "data");
  add_method("groupBy", std::bind(&TableSelect::group_by, this, _1), "data");
  add_method("having", std::bind(&TableSelect::having, this, _1), "data");
  add_method("orderBy", std::bind(&TableSelect::order_by, this, _1), "data");
  add_method("limit", std::bind(&TableSelect::limit, this, _1, limit_id, true),
             "data");
  add_method("offset",
             std::bind(&TableSelect::offset, this, _1, offset_id, "offset"),
             "data");
  add_method("bind", std::bind(&TableSelect::bind_, this, _1, bind_id), "data");
  add_method("lockShared", std::bind(&TableSelect::lock_shared, this, _1));
  add_method("lockExclusive",
             std::bind(&TableSelect::lock_exclusive, this, _1));

  // Registers the dynamic function behavior
  register_dynamic_function(F::select, F::where | F::groupBy | F::orderBy |
                                           F::limit | F::lockShared |
                                           F::lockExclusive | F::bind |
                                           F::execute | F::__shell_hook__);
  register_dynamic_function(F::where);
  register_dynamic_function(F::groupBy, F::having, F::where);
  register_dynamic_function(F::having);
  register_dynamic_function(F::orderBy, K_ENABLE_NONE,
                            F::where | F::groupBy | F::having);
  register_dynamic_function(F::limit, F::offset,
                            F::where | F::groupBy | F::having | F::orderBy);
  register_dynamic_function(F::offset);
  register_dynamic_function(
      F::lockShared, K_ENABLE_NONE,
      F::lockExclusive | F::where | F::groupBy | F::limit | F::orderBy);
  register_dynamic_function(
      F::lockExclusive, K_ENABLE_NONE,
      F::lockShared | F::where | F::groupBy | F::limit | F::orderBy);
  register_dynamic_function(F::bind, K_ENABLE_NONE,
                            F::groupBy | F::having | F::where | F::orderBy |
                                F::limit | F::lockShared | F::lockExclusive,
                            K_ALLOW_REUSE);
  register_dynamic_function(F::execute, F::limit, K_DISABLE_NONE,
                            K_ALLOW_REUSE);

  // Initial function update
  enable_function(F::select);
}

shcore::Value TableSelect::this_object() {
  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(select, TableSelect);
REGISTER_HELP(TABLESELECT_SELECT_BRIEF,
              "Defines the columns to be retrieved from the table.");
REGISTER_HELP(TABLESELECT_SELECT_SIGNATURE, "()");
REGISTER_HELP(TABLESELECT_SELECT_SIGNATURE1, "(colDefArray)");
REGISTER_HELP(TABLESELECT_SELECT_SIGNATURE2, "(colDef, colDef, ...)");
REGISTER_HELP(TABLESELECT_SELECT_RETURNS, "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_SELECT_DETAIL,
              "Defines the columns that will be retrieved from the Table.");
REGISTER_HELP(TABLESELECT_SELECT_DETAIL1,
              "To define the column list either use a list object containing "
              "the column definition or pass each column definition on a "
              "separate parameter");
REGISTER_HELP(TABLESELECT_SELECT_DETAIL2,
              "If the function is called without specifying any column "
              "definition, all the columns in the table will be retrieved.");
/**
 * $(TABLESELECT_SELECT_BRIEF)
 *
 * $(TABLESELECT_SELECT_RETURNS)
 *
 * $(TABLESELECT_SELECT_DETAIL)
 *
 * $(TABLESELECT_SELECT_DETAIL1)
 *
 * $(TABLESELECT_SELECT_DETAIL2)
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
      reset_prepared_statement();
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("select"));
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(where, TableSelect);
REGISTER_HELP(TABLESELECT_WHERE_BRIEF,
              "Sets the search condition to filter the records to be retrieved "
              "from the Table.");
REGISTER_HELP(TABLESELECT_WHERE_PARAM,
              "@param expression Optional condition to filter the records to "
              "be retrieved.");
REGISTER_HELP(TABLESELECT_WHERE_RETURNS, "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_WHERE_DETAIL,
              "If used, only those rows satisfying the <b>expression</b> will "
              "be retrieved");
REGISTER_HELP(TABLESELECT_WHERE_DETAIL1,
              "The <b>expression</b> supports parameter binding.");

/**
 * $(TABLESELECT_WHERE_BRIEF)
 *
 * $(TABLESELECT_WHERE_PARAM)
 *
 * $(TABLESELECT_WHERE_RETURNS)
 *
 * $(TABLESELECT_WHERE_DETAIL)
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
TableSelect TableSelect::where(String expression) {}
#elif DOXYGEN_PY
TableSelect TableSelect::where(str expression) {}
#endif
shcore::Value TableSelect::where(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("where").c_str());

  try {
    message_.set_allocated_criteria(::mysqlx::parser::parse_table_filter(
        args.string_at(0), &_placeholders));

    update_functions(F::where);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("where"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(groupBy, TableSelect);
REGISTER_HELP(TABLESELECT_GROUPBY_BRIEF,
              "Sets a grouping criteria for the retrieved rows.");
REGISTER_HELP(TABLESELECT_GROUPBY_SIGNATURE, "(searchExprStrList)");
REGISTER_HELP(TABLESELECT_GROUPBY_SIGNATURE1,
              "(searchExprStr, searchExprStr, ...)");
REGISTER_HELP(TABLESELECT_GROUPBY_RETURNS, "@returns This TableSelect object.");
/**
 * $(TABLESELECT_GROUPBY_BRIEF)
 *
 * $(TABLESELECT_GROUPBY_RETURNS)
 *
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
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("groupBy"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(having, TableSelect);
REGISTER_HELP(TABLESELECT_HAVING_BRIEF,
              "Sets a condition for records to be considered in aggregate "
              "function operations.");
REGISTER_HELP(
    TABLESELECT_HAVING_PARAM,
    "@param condition A condition to be used with aggregate functions.");
REGISTER_HELP(TABLESELECT_HAVING_RETURNS, "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_HAVING_DETAIL,
              "If used the TableSelect operation will only consider the "
              "records matching the established criteria.");
/**
 * $(TABLESELECT_HAVING_BRIEF)
 *
 * $(TABLESELECT_HAVING_PARAM)
 *
 * $(TABLESELECT_HAVING_RETURNS)
 *
 * $(TABLESELECT_HAVING_DETAIL)
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
TableSelect TableSelect::having(String condition) {}
#elif DOXYGEN_PY
TableSelect TableSelect::having(str condition) {}
#endif
shcore::Value TableSelect::having(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("having").c_str());

  try {
    message_.set_allocated_grouping_criteria(
        ::mysqlx::parser::parse_table_filter(args.string_at(0),
                                             &_placeholders));

    update_functions(F::having);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("having"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(orderBy, TableSelect);
REGISTER_HELP(TABLESELECT_ORDERBY_BRIEF,
              "Sets the order in which the records will be retrieved.");
REGISTER_HELP(TABLESELECT_ORDERBY_SIGNATURE,
              "(sortCriterion[, sortCriterion, ...])");
REGISTER_HELP(TABLESELECT_ORDERBY_SIGNATURE1, "(sortCriteria)");
REGISTER_HELP(TABLESELECT_ORDERBY_RETURNS, "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_ORDERBY_DETAIL,
              "If used the records will be sorted with the defined criteria.");
REGISTER_HELP(TABLESELECT_ORDERBY_DETAIL1,
              "The elements of <b>sortExprStr</b> list are strings defining "
              "the column name on which the sorting will be based.");
REGISTER_HELP(TABLESELECT_ORDERBY_DETAIL2,
              "The format is as follows: columnIdentifier [ ASC | DESC ]");
REGISTER_HELP(
    TABLESELECT_ORDERBY_DETAIL3,
    "If no order criteria is specified, ascending will be used by default.");
/**
 * $(TABLESELECT_ORDERBY_BRIEF)
 *
 * $(TABLESELECT_ORDERBY_RETURNS)
 *
 * $(TABLESELECT_ORDERBY_DETAIL)
 *
 * $(TABLESELECT_ORDERBY_DETAIL1)
 *
 * $(TABLESELECT_ORDERBY_DETAIL2)
 *
 * $(TABLESELECT_ORDERBY_DETAIL3)
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
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("orderBy"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(limit, TableSelect);
REGISTER_HELP(
    TABLESELECT_LIMIT_BRIEF,
    "Sets the maximum number of rows to be returned on the select operation.");
REGISTER_HELP(
    TABLESELECT_LIMIT_PARAM,
    "@param numberOfRows The maximum number of rows to be retrieved.");
REGISTER_HELP(TABLESELECT_LIMIT_RETURNS, "@returns This TableSelect object.");
REGISTER_HELP(
    TABLESELECT_LIMIT_DETAIL,
    "If used, the operation will return at most <b>numberOfRows</b> rows.");
REGISTER_HELP(TABLESELECT_LIMIT_DETAIL1, "${LIMIT_EXECUTION_MODE}");

/**
 * $(TABLESELECT_LIMIT_BRIEF)
 *
 * $(TABLESELECT_LIMIT_PARAM)
 *
 * $(TABLESELECT_LIMIT_RETURNS)
 *
 * $(TABLESELECT_LIMIT_DETAIL)
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
 * $(LIMIT_EXECUTION_MODE)
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

REGISTER_HELP_FUNCTION(offset, TableSelect);
REGISTER_HELP(TABLESELECT_OFFSET_BRIEF,
              "Sets number of rows to skip on the resultset when a limit has "
              "been defined.");
REGISTER_HELP(TABLESELECT_OFFSET_PARAM,
              "@param numberOfRows The number of rows to skip before start "
              "including them on the RowResult.");
REGISTER_HELP(TABLESELECT_OFFSET_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(TABLESELECT_OFFSET_DETAIL,
              "If used, the first <b>numberOfRows</b> records will not be "
              "included on the result.");
/**
 * $(TABLESELECT_OFFSET_BRIEF)
 *
 * $(TABLESELECT_OFFSET_PARAM)
 *
 * $(TABLESELECT_OFFSET_RETURNS)
 *
 * $(TABLESELECT_OFFSET_DETAIL)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 *
 * - limit(Integer numberOfRows)
 *
 * After this function invocation, the following functions can be invoked:
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
TableSelect TableSelect::offset(Integer numberOfRows) {}
#elif DOXYGEN_PY
TableSelect TableSelect::offset(int numberOfRows) {}
#endif

void TableSelect::set_lock_contention(const shcore::Argument_list &args) {
  std::string lock_contention;
  if (args.size() == 1) {
    if (args[0].type == shcore::Object) {
      std::shared_ptr<Constant> constant =
          std::dynamic_pointer_cast<Constant>(args.object_at(0));
      if (constant && constant->group() == "LockContention")
        lock_contention = constant->data().get_string();
    } else if (args[0].type == shcore::String) {
      lock_contention = args.string_at(0);
    }

    if (!shcore::str_casecmp(lock_contention.c_str(), "nowait")) {
      message_.set_locking_options(Mysqlx::Crud::Find_RowLockOptions_NOWAIT);
    } else if (!shcore::str_casecmp(lock_contention.c_str(), "skip_locked")) {
      message_.set_locking_options(
          Mysqlx::Crud::Find_RowLockOptions_SKIP_LOCKED);
    } else if (shcore::str_casecmp(lock_contention.c_str(), "default")) {
      throw shcore::Exception::argument_error(
          "Argument #1 is expected to be one of DEFAULT, NOWAIT or "
          "SKIP_LOCKED");
    }
  }
}

REGISTER_HELP_FUNCTION(lockShared, TableSelect);
REGISTER_HELP(TABLESELECT_LOCKSHARED_BRIEF,
              "Instructs the server to acquire shared row locks in documents "
              "matched by this find operation.");
REGISTER_HELP(
    TABLESELECT_LOCKSHARED_PARAM,
    "@param lockContention optional parameter to indicate how to handle rows "
    "that are already locked.");
REGISTER_HELP(TABLESELECT_LOCKSHARED_RETURNS,
              "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL,
              "When this function is called, the selected rows will be "
              "locked for write operations, they may be retrieved on a "
              "different session, but no updates will be allowed.");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL1,
              "The acquired locks will be released when the current "
              "transaction is committed or rolled back.");

REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL2,
              "The lockContention parameter defines the behavior of the "
              "operation if another session contains an exclusive lock to "
              "matching rows.");

REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL3,
              "The lockContention can be specified using the following "
              "constants:");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL4,
              "@li mysqlx.LockContention.DEFAULT");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL5,
              "@li mysqlx.LockContention.NOWAIT");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL6,
              "@li mysqlx.LockContention.SKIP_LOCKED");

REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL7,
              "The lockContention can also be specified using the following "
              "string literals (no case sensitive):");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL8, "@li 'DEFAULT'");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL9, "@li 'NOWAIT'");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL10, "@li 'SKIP_LOCKED'");

REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL11,
              "If no lockContention or the default is specified, the operation "
              "will block if another session already holds an exclusive lock "
              "on matching rows until the lock is released.");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL12,
              "If lockContention is set to NOWAIT and another session "
              "already holds an exclusive lock on matching rows, the "
              "operation will not block and an error will be generated.");
REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL13,
              "If lockContention is set to SKIP_LOCKED and another session "
              "already holds an exclusive lock on matching rows, the "
              "operation will not block and will return only those rows "
              "not having an exclusive lock.");

REGISTER_HELP(TABLESELECT_LOCKSHARED_DETAIL14,
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
TableSelect TableSelect::lockShared(String lockContention) {}
#elif DOXYGEN_PY
TableSelect TableSelect::lock_shared(str lockContention) {}
#endif
//@}
shcore::Value TableSelect::lock_shared(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("lockShared").c_str());

  try {
    message_.set_locking(Mysqlx::Crud::Find_RowLock_SHARED_LOCK);

    set_lock_contention(args);

    update_functions(F::lockShared);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("lockShared"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(lockExclusive, TableSelect);
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_BRIEF,
              "Instructs the server to acquire an exclusive lock on rows "
              "matched by this find operation.");
REGISTER_HELP(
    TABLESELECT_LOCKEXCLUSIVE_PARAM,
    "@param lockContention optional parameter to indicate how to handle "
    "rows that are already locked.");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_RETURNS,
              "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL,
              "When this function is called, the selected rows will be "
              "locked for read operations, they will not be retrievable by "
              "other session.");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL1,
              "The acquired locks will be released when the current "
              "transaction is committed or rolled back.");

REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL2,
              "The lockContention parameter defines the behavior of the "
              "operation if another session contains a lock to matching "
              "rows.");

REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL3,
              "The lockContention can be specified using the following "
              "constants:");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL4,
              "@li mysqlx.LockContention.DEFAULT");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL5,
              "@li mysqlx.LockContention.NOWAIT");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL6,
              "@li mysqlx.LockContention.SKIP_LOCKED");

REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL7,
              "The lockContention can also be specified using the following "
              "string literals (no case sensitive):");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL8, "@li 'DEFAULT'");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL9, "@li 'NOWAIT'");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL10, "@li 'SKIP_LOCKED'");

REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL11,
              "If no lockContention or the default is specified, the operation "
              "will block if another session already holds a lock on matching "
              "rows until the lock is released.");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL12,
              "If lockContention is set to NOWAIT and another session "
              "already holds a lock on matching rows, the operation will not "
              "block and an error will be generated.");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL13,
              "If lockContention is set to SKIP_LOCKED and another session "
              "already holds a lock on matching rows, the operation will not "
              "block and will return only those rows not having an exclusive "
              "lock.");
REGISTER_HELP(TABLESELECT_LOCKEXCLUSIVE_DETAIL14,
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
TableSelect TableSelect::lockExclusive(String lockContention) {}
#elif DOXYGEN_PY
TableSelect TableSelect::lock_exclusive(str lockContention) {}
#endif
//@}
shcore::Value TableSelect::lock_exclusive(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("lockExclusive").c_str());

  try {
    message_.set_locking(Mysqlx::Crud::Find_RowLock_EXCLUSIVE_LOCK);

    set_lock_contention(args);

    update_functions(F::lockExclusive);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("lockExclusive"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(bind, TableSelect);
REGISTER_HELP(
    TABLESELECT_BIND_BRIEF,
    "Binds a value to a specific placeholder used on this operation.");
REGISTER_HELP(TABLESELECT_BIND_PARAM,
              "@param name The name of the placeholder to which the value will "
              "be bound.");
REGISTER_HELP(TABLESELECT_BIND_PARAM1,
              "@param value The value to be bound on the placeholder.");
REGISTER_HELP(TABLESELECT_BIND_RETURNS, "@returns This TableSelect object.");
REGISTER_HELP(TABLESELECT_BIND_DETAIL, "${TABLESELECT_BIND_BRIEF}");
REGISTER_HELP(TABLESELECT_BIND_DETAIL1,
              "An error will be raised if the placeholder indicated by name "
              "does not exist.");
REGISTER_HELP(TABLESELECT_BIND_DETAIL2,
              "This function must be called once for each used placeholder or "
              "an error will be raised when the execute method is called.");

/**
 * $(TABLESELECT_BIND_BRIEF)
 *
 * $(TABLESELECT_BIND_PARAM)
 * $(TABLESELECT_BIND_PARAM1)
 *
 * $(TABLESELECT_BIND_RETURNS)
 *
 * $(TABLESELECT_BIND_DETAIL1)
 *
 * $(TABLESELECT_BIND_DETAIL2)
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
 * This function must be called once for each used placeholder or an error will
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

REGISTER_HELP_FUNCTION(execute, TableSelect);
REGISTER_HELP(TABLESELECT_EXECUTE_BRIEF,
              "Executes the select operation with all the configured options.");
REGISTER_HELP(TABLESELECT_EXECUTE_RETURNS,
              "@returns A RowResult object that can be used to traverse the "
              "rows returned by this operation.");
/**
 * $(TABLESELECT_EXECUTE_BRIEF)
 *
 * $(TABLESELECT_EXECUTE_RETURNS)
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
    result.reset(new mysqlx::RowResult(safe_exec([this]() {
      update_limits();
      insert_bound_values(message_.mutable_args());
      return session()->session()->execute_crud(message_);
    })));

    update_functions(F::execute);
    if (!m_limit.is_null()) enable_function(F::offset);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}

void TableSelect::set_prepared_stmt() {
  m_prep_stmt.mutable_stmt()->set_type(
      Mysqlx::Prepare::Prepare_OneOfMessage_Type_FIND);
  message_.clear_args();
  update_limits();
  *m_prep_stmt.mutable_stmt()->mutable_find() = message_;
}
