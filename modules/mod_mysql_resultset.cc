/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysql_resultset.h"
#include <iomanip>
#include <string>
#include "modules/devapi/base_constants.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "mysqlshdk/libs/db/charset.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace shcore;
using namespace mysqlsh::mysql;

// Documentation of the ClassicResult class
REGISTER_HELP_CLASS(ClassicResult, mysql);
REGISTER_HELP(CLASSICRESULT_BRIEF,
              "Allows browsing through the result information "
              "after performing an operation on the database through the MySQL "
              "Protocol.");
REGISTER_HELP(
    CLASSICRESULT_DETAIL,
    "This class allows access to the result set from "
    "the classic MySQL data model to be retrieved from Dev API queries.");

ClassicResult::ClassicResult(
    std::shared_ptr<mysqlshdk::db::mysql::Result> result)
    : _result(result) {
  add_property("columns", "getColumns");
  add_property("columnCount", "getColumnCount");
  add_property("columnNames", "getColumnNames");
  add_property("affectedItemsCount", "getAffectedItemsCount");
  add_property("affectedRowCount", "getAffectedRowCount");
  add_property("warningCount", "getWarningCount");
  add_property("warningsCount", "getWarningsCount");
  add_property("warnings", "getWarnings");
  add_property("executionTime", "getExecutionTime");
  add_property("autoIncrementValue", "getAutoIncrementValue");
  add_property("info", "getInfo");

  add_method("fetchOne", std::bind((shcore::Value(ClassicResult::*)(
                                       const shcore::Argument_list &) const) &
                                       ClassicResult::fetch_one,
                                   this, _1));
  expose("fetchOneObject", &ClassicResult::_fetch_one_object);
  add_method("fetchAll", std::bind((shcore::Value(ClassicResult::*)(
                                       const shcore::Argument_list &) const) &
                                       ClassicResult::fetch_all,
                                   this, _1));
  add_method("nextDataSet", std::bind(&ClassicResult::next_data_set, this, _1));
  add_method("nextResult", std::bind(&ClassicResult::next_result, this, _1));
  add_method("hasData", std::bind(&ClassicResult::has_data, this, _1));

  _column_names.reset(new std::vector<std::string>());
  for (auto &cmd : _result->get_metadata())
    _column_names->push_back(cmd.get_column_label());
}

// Documentation of the hasData function
REGISTER_HELP_FUNCTION(hasData, ClassicResult);
REGISTER_HELP(CLASSICRESULT_HASDATA_BRIEF,
              "Returns true if the last statement execution "
              "has a result set.");

/**
 * $(CLASSICRESULT_HASDATA_BRIEF)
 */
#if DOXYGEN_JS
Bool ClassicResult::hasData() {}
#elif DOXYGEN_PY
bool ClassicResult::has_data() {}
#endif
shcore::Value ClassicResult::has_data(const shcore::Argument_list &args) const {
  args.ensure_count(0, get_function_name("hasData").c_str());

  return Value(_result->has_resultset());
}

// Documentation of the fetchOne function
REGISTER_HELP_FUNCTION(fetchOne, ClassicResult);
REGISTER_HELP(CLASSICRESULT_FETCHONE_BRIEF,
              "Retrieves the next Row on the ClassicResult.");
REGISTER_HELP(
    CLASSICRESULT_FETCHONE_RETURNS,
    "@returns A Row object representing the next record in the result.");

/**
 * $(CLASSICRESULT_FETCHONE_BRIEF)
 *
 * $(CLASSICRESULT_FETCHONE_RETURNS)
 */
#if DOXYGEN_JS
Row ClassicResult::fetchOne() {}
#elif DOXYGEN_PY
Row ClassicResult::fetch_one() {}
#endif
shcore::Value ClassicResult::fetch_one(
    const shcore::Argument_list &args) const {
  shcore::Value ret_val = shcore::Value::Null();

  args.ensure_count(0, get_function_name("fetchOne").c_str());

  try {
    auto row = fetch_one_row();

    if (row) {
      ret_val = shcore::Value::wrap(row.release());
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("fetchOne"));

  return ret_val;
}

const mysqlshdk::db::IRow *ClassicResult::fetch_one() const {
  return _result->fetch_one();
}

REGISTER_HELP_FUNCTION(fetchOneObject, ClassicResult);
REGISTER_HELP(CLASSICRESULT_FETCHONEOBJECT_BRIEF,
              "${BASERESULT_FETCHONEOBJECT_BRIEF}");
REGISTER_HELP(CLASSICRESULT_FETCHONEOBJECT_RETURNS,
              "${BASERESULT_FETCHONEOBJECT_RETURNS}");
REGISTER_HELP(CLASSICRESULT_FETCHONEOBJECT_DETAIL,
              "${BASERESULT_FETCHONEOBJECT_DETAIL}");
/**
 * $(BASERESULT_FETCHONEOBJECT_BRIEF)
 *
 * $(BASERESULT_FETCHONEOBJECT)
 */
#if DOXYGEN_JS
Dictionary ClassicResult::fetchOneObject() {}
#elif DOXYGEN_PY
dict ClassicResult::fetch_one_object() {}
#endif
shcore::Dictionary_t ClassicResult::_fetch_one_object() {
  return ShellBaseResult::fetch_one_object();
}

// Documentation of nextDataSet function
REGISTER_HELP_FUNCTION(nextDataSet, ClassicResult);
REGISTER_HELP(CLASSICRESULT_NEXTDATASET_BRIEF,
              "Prepares the SqlResult to start reading data from the next "
              "Result (if many results were returned).");
REGISTER_HELP(CLASSICRESULT_NEXTDATASET_DETAIL,
              "${CLASSICRESULT_NEXTDATASET_DEPRECATED}");
REGISTER_HELP(CLASSICRESULT_NEXTDATASET_DEPRECATED,
              "@attention This function will be removed in a future release, "
              "use the <b><<<nextResult>>></b> function instead.");
REGISTER_HELP(CLASSICRESULT_NEXTDATASET_RETURNS,
              "@returns A boolean value indicating whether there is another "
              "result or not.");

/**
 * $(CLASSICRESULT_NEXTDATASET_BRIEF)
 *
 * $(CLASSICRESULT_NEXTDATASET_RETURNS)
 */
#if DOXYGEN_JS
/**
 * @attention This function will be removed in a future release, use the
 * &nbsp;<b>nextResult</b> function instead.
 */
#elif DOXYGEN_PY
/**
 * @attention This function will be removed in a future release, use the
 * &nbsp;<b>next_result</b> function instead.
 */
#endif

#if DOXYGEN_JS
Bool ClassicResult::nextDataSet() {}
#elif DOXYGEN_PY
bool ClassicResult::next_data_set() {}
#endif
shcore::Value ClassicResult::next_data_set(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("nextDataSet").c_str());

  log_warning("'%s' is deprecated, use '%s' instead.",
              get_function_name("nextDataSet").c_str(),
              get_function_name("nextResult").c_str());

  return shcore::Value(_result->next_resultset());
}

// Documentation of nextResult function
REGISTER_HELP_FUNCTION(nextResult, ClassicResult);
REGISTER_HELP(CLASSICRESULT_NEXTRESULT_BRIEF,
              "Prepares the SqlResult to start reading data from the next "
              "Result (if many results were returned).");
REGISTER_HELP(CLASSICRESULT_NEXTRESULT_RETURNS,
              "@returns A boolean value indicating whether there is another "
              "result or not.");

/**
 * $(CLASSICRESULT_NEXTRESULT_BRIEF)
 *
 * $(CLASSICRESULT_NEXTRESULT_RETURNS)
 */
#if DOXYGEN_JS
Bool ClassicResult::nextResult() {}
#elif DOXYGEN_PY
bool ClassicResult::next_result() {}
#endif
shcore::Value ClassicResult::next_result(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("nextResult").c_str());

  return shcore::Value(_result->next_resultset());
}

// Documentation of the fetchAll function
REGISTER_HELP_FUNCTION(fetchAll, ClassicResult);
REGISTER_HELP(CLASSICRESULT_FETCHALL_BRIEF,
              "Returns a list of Row objects which contains an element for "
              "every record left on the result.");
REGISTER_HELP(CLASSICRESULT_FETCHALL_RETURNS,
              "@returns A List of Row objects.");
REGISTER_HELP(CLASSICRESULT_FETCHALL_DETAIL,
              "If this function is called right after executing a query, "
              "it will return a Row for every record on the resultset.");
REGISTER_HELP(CLASSICRESULT_FETCHALL_DETAIL1,
              "If fetchOne is called before this function, when this function "
              "is called it will return a Row for each of the remaining "
              "records on the resultset.");

/**
 * $(CLASSICRESULT_FETCHALL_BRIEF)
 *
 * $(CLASSICRESULT_FETCHALL_RETURNS)
 *
 * $(CLASSICRESULT_FETCHALL_DETAIL)
 *
 * $(CLASSICRESULT_FETCHALL_DETAIL1)
 */
#if DOXYGEN_JS
List ClassicResult::fetchAll() {}
#elif DOXYGEN_PY
list ClassicResult::fetch_all() {}
#endif
shcore::Value ClassicResult::fetch_all(
    const shcore::Argument_list &args) const {
  args.ensure_count(0, get_function_name("fetchAll").c_str());

  std::shared_ptr<shcore::Value::Array_type> array(
      new shcore::Value::Array_type);

  shcore::Value record = fetch_one(args);

  while (record) {
    array->push_back(record);
    record = fetch_one(args);
  }

  return shcore::Value(array);
}

// Documentation of getAffectedRowCount function
REGISTER_HELP_PROPERTY(affectedRowCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_AFFECTEDROWCOUNT_BRIEF,
              "Same as <<<getAffectedRowCount>>>");
REGISTER_HELP(CLASSICRESULT_AFFECTEDROWCOUNT_DETAIL,
              "${CLASSICRESULT_AFFECTEDROWCOUNT_DEPRECATED}");
REGISTER_HELP(CLASSICRESULT_AFFECTEDROWCOUNT_DEPRECATED,
              "@attention This property will be removed in a future release, "
              "use the <b><<<affectedItemsCount>>></b> property instead.");

REGISTER_HELP_FUNCTION(getAffectedRowCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETAFFECTEDROWCOUNT_BRIEF,
              "The number of affected rows for the last operation.");
REGISTER_HELP(CLASSICRESULT_GETAFFECTEDROWCOUNT_RETURNS,
              "@returns the number of affected rows.");
REGISTER_HELP(CLASSICRESULT_GETAFFECTEDROWCOUNT_DETAIL,
              "This is the value of the C API mysql_affected_rows(), see "
              "https://dev.mysql.com/doc/refman/en/"
              "mysql-affected-rows.html");
REGISTER_HELP(CLASSICRESULT_GETAFFECTEDROWCOUNT_DETAIL1,
              "${CLASSICRESULT_GETAFFECTEDROWCOUNT_DEPRECATED}");
REGISTER_HELP(CLASSICRESULT_GETAFFECTEDROWCOUNT_DEPRECATED,
              "@attention This function will be removed in a future release, "
              "use the <b><<<getAffectedItemsCount>>></b> function instead.");

/**
 * $(CLASSICRESULT_GETAFFECTEDROWCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETAFFECTEDROWCOUNT_RETURNS)
 *
 * $(CLASSICRESULT_GETAFFECTEDROWCOUNT_DETAIL)
 *
 */
#if DOXYGEN_JS
/**
 * @attention This function will be removed in a future release, use the
 * &nbsp;<b>getAffectedItemsCount</b> function instead.
 */
#elif DOXYGEN_PY
/**
 * @attention This function will be removed in a future release, use the
 * &nbsp;<b>get_affected_items_count</b> function instead.
 */
#endif
#if DOXYGEN_JS
Integer ClassicResult::getAffectedRowCount() {}
#elif DOXYGEN_PY
int ClassicResult::get_affected_row_count() {}
#endif

// Documentation of getAffectedItemsCount function
REGISTER_HELP_PROPERTY(affectedItemsCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_AFFECTEDITEMSCOUNT_BRIEF,
              "Same as <<<getAffectedItemsCount>>>");

REGISTER_HELP_FUNCTION(getAffectedItemsCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETAFFECTEDITEMSCOUNT_BRIEF,
              "The the number of affected items for the last operation.");

REGISTER_HELP(CLASSICRESULT_GETAFFECTEDITEMSCOUNT_RETURNS,
              "@returns the number of affected items.");
REGISTER_HELP(CLASSICRESULT_GETAFFECTEDITEMSCOUNT_DETAIL,
              "Returns the number of records affected by the executed "
              "operation");

/**
 * $(CLASSICRESULT_GETAFFECTEDITEMSCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETAFFECTEDITEMSCOUNT_RETURNS)
 *
 * $(CLASSICRESULT_GETAFFECTEDITEMSCOUNT_DETAIL)
 */
#if DOXYGEN_JS
Integer ClassicResult::getAffectedItemsCount() {}
#elif DOXYGEN_PY
int ClassicResult::get_affected_items_count() {}
#endif

// Documentation of the getColumnCount function
REGISTER_HELP_FUNCTION(getColumnCount, ClassicResult);
REGISTER_HELP_PROPERTY(columnCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETCOLUMNCOUNT_BRIEF,
              "Retrieves the number of columns on the current result.");
REGISTER_HELP(CLASSICRESULT_COLUMNCOUNT_BRIEF,
              "${CLASSICRESULT_GETCOLUMNCOUNT_BRIEF}");
REGISTER_HELP(CLASSICRESULT_GETCOLUMNCOUNT_RETURNS,
              "@returns the number of columns on the current result.");

/**
 * $(CLASSICRESULT_GETCOLUMNCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETCOLUMNCOUNT_RETURNS)
 */
#if DOXYGEN_JS
Integer ClassicResult::getColumnCount() {}
#elif DOXYGEN_PY
int ClassicResult::get_column_count() {}
#endif

// Documentation of the getColumnCount function
REGISTER_HELP_FUNCTION(getColumnNames, ClassicResult);
REGISTER_HELP_PROPERTY(columnNames, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETCOLUMNNAMES_BRIEF,
              "Gets the columns on the current result.");
REGISTER_HELP(CLASSICRESULT_COLUMNNAMES_BRIEF,
              "${CLASSICRESULT_GETCOLUMNNAMES_BRIEF}");
REGISTER_HELP(CLASSICRESULT_GETCOLUMNNAMES_RETURNS,
              "@returns A list with the names of the columns returned on the "
              "active result.");

/**
 * $(CLASSICRESULT_GETCOLUMNNAMES_BRIEF)
 *
 * $(CLASSICRESULT_GETCOLUMNNAMES_RETURNS)
 */
#if DOXYGEN_JS
List ClassicResult::getColumnNames() {}
#elif DOXYGEN_PY
list ClassicResult::get_column_names() {}
#endif

// Documentation of the getColumns function
REGISTER_HELP_FUNCTION(getColumns, ClassicResult);
REGISTER_HELP_PROPERTY(columns, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETCOLUMNS_BRIEF,
              "Gets the column metadata for the columns on the active result.");
REGISTER_HELP(CLASSICRESULT_COLUMNS_BRIEF, "${CLASSICRESULT_GETCOLUMNS_BRIEF}");
REGISTER_HELP(
    CLASSICRESULT_GETCOLUMNS_RETURNS,
    "@returns a list of column metadata objects "
    "containing information about the columns included on the active result.");

/**
 * $(CLASSICRESULT_GETCOLUMNS_BRIEF)
 *
 * $(CLASSICRESULT_GETCOLUMNS_RETURNS)
 */
#if DOXYGEN_JS
List ClassicResult::getColumns() {}
#elif DOXYGEN_PY
list ClassicResult::get_columns() {}
#endif

// Documentation of the getExecutionTime function
REGISTER_HELP_FUNCTION(getExecutionTime, ClassicResult);
REGISTER_HELP_PROPERTY(executionTime, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETEXECUTIONTIME_BRIEF,
              "Retrieves a string value indicating the execution time of the "
              "executed operation.");
REGISTER_HELP(CLASSICRESULT_EXECUTIONTIME_BRIEF,
              "${CLASSICRESULT_GETEXECUTIONTIME_BRIEF}");

/**
 * $(CLASSICRESULT_GETEXECUTIONTIME_BRIEF)
 */
#if DOXYGEN_JS
String ClassicResult::getExecutionTime() {}
#elif DOXYGEN_PY
str ClassicResult::get_execution_time() {}
#endif

// Documentation of the getInfo function
REGISTER_HELP_FUNCTION(getInfo, ClassicResult);
REGISTER_HELP_PROPERTY(info, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETINFO_BRIEF,
              "Retrieves a string providing information about the most "
              "recently executed statement.");
REGISTER_HELP(CLASSICRESULT_INFO_BRIEF, "${CLASSICRESULT_GETINFO_BRIEF}");
REGISTER_HELP(CLASSICRESULT_GETINFO_RETURNS,
              "@returns a string with the execution information");

/**
 * $(CLASSICRESULT_GETINFO_BRIEF)
 *
 * $(CLASSICRESULT_GETINFO_RETURNS)
 *
 * For more details, see:
 * https://dev.mysql.com/doc/refman/en/mysql-info.html
 */
#if DOXYGEN_JS
String ClassicResult::getInfo() {}
#elif DOXYGEN_PY
str ClassicResult::get_info() {}
#endif

// Documentation of the getAutoIncrementValue function
REGISTER_HELP_FUNCTION(getAutoIncrementValue, ClassicResult);
REGISTER_HELP_PROPERTY(autoIncrementValue, ClassicResult);
REGISTER_HELP(
    CLASSICRESULT_GETAUTOINCREMENTVALUE_BRIEF,
    "Returns the last insert id auto generated (from an insert operation)");
REGISTER_HELP(CLASSICRESULT_AUTOINCREMENTVALUE_BRIEF,
              "${CLASSICRESULT_GETAUTOINCREMENTVALUE_BRIEF}");
REGISTER_HELP(CLASSICRESULT_GETAUTOINCREMENTVALUE_RETURNS,
              "@returns the integer "
              "representing the last insert id");

/**
 * $(CLASSICRESULT_GETAUTOINCREMENTVALUE_BRIEF)
 *
 * $(CLASSICRESULT_GETAUTOINCREMENTVALUE_RETURNS)
 *
 * For more details, see
 * https://dev.mysql.com/doc/refman/en/information-functions.html#function_last-insert-id
 */
#if DOXYGEN_JS
Integer ClassicResult::getAutoIncrementValue() {}
#elif DOXYGEN_PY
int ClassicResult::get_auto_increment_value() {}
#endif

// Documentation of getWarningCount function
REGISTER_HELP_PROPERTY(warningCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_WARNINGCOUNT_BRIEF,
              "Same as <<<getWarningCount>>>");
REGISTER_HELP(CLASSICRESULT_WARNINGCOUNT_DETAIL,
              "${CLASSICRESULT_WARNINGCOUNT_DEPRECATED}");
REGISTER_HELP(CLASSICRESULT_WARNINGCOUNT_DEPRECATED,
              "@attention This property will be removed in a future release, "
              "use the <b><<<warningsCount>>></b> property instead.");

REGISTER_HELP_FUNCTION(getWarningCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETWARNINGCOUNT_BRIEF,
              "The number of warnings produced by the last "
              "statement execution.");
REGISTER_HELP(CLASSICRESULT_GETWARNINGCOUNT_RETURNS,
              "@returns the number of warnings.");
REGISTER_HELP(CLASSICRESULT_GETWARNINGCOUNT_DETAIL,
              "This is the same value than C API mysql_warning_count, see "
              "https://dev.mysql.com/doc/refman/en/"
              "mysql-warning-count.html");
REGISTER_HELP(CLASSICRESULT_GETWARNINGCOUNT_DETAIL1,
              "See <<<getWarnings>>>() for more details.");
REGISTER_HELP(CLASSICRESULT_GETWARNINGCOUNT_DETAIL2,
              "${CLASSICRESULT_GETWARNINGCOUNT_DEPRECATED}");
REGISTER_HELP(CLASSICRESULT_GETWARNINGCOUNT_DEPRECATED,
              "@attention This function will be removed in a future release, "
              "use the <b><<<getWarningsCount>>></b> function instead.");

/**
 * $(CLASSICRESULT_GETWARNINGCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETWARNINGCOUNT_RETURNS)
 *
 * $(CLASSICRESULT_GETWARNINGCOUNT_DETAIL)
 *
 */
#if DOXYGEN_JS
/**
 * @attention This function will be removed in a future release, use the&nbsp;
 * <b>getWarningsCount</b> function instead.
 */
#elif DOXYGEN_PY
/**
 * @attention This function will be removed in a future release, use the&nbsp;
 * <b>get_warnings_count</b> function instead.
 */
#endif
/**
 * \sa warnings
 */
#if DOXYGEN_JS
Integer ClassicResult::getWarningCount() {}
#elif DOXYGEN_PY
int ClassicResult::get_warning_count() {}
#endif

// Documentation of getWarningsCount function
REGISTER_HELP_PROPERTY(warningsCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_WARNINGSCOUNT_BRIEF,
              "Same as <<<getWarningsCount>>>");

REGISTER_HELP_FUNCTION(getWarningsCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETWARNINGSCOUNT_BRIEF,
              "The number of warnings produced by the last statement "
              "execution.");
REGISTER_HELP(CLASSICRESULT_GETWARNINGSCOUNT_RETURNS,
              "@returns the number of warnings.");
REGISTER_HELP(CLASSICRESULT_GETWARNINGSCOUNT_DETAIL,
              "This is the same value than C API mysql_warning_count, see "
              "https://dev.mysql.com/doc/refman/en/"
              "mysql-warning-count.html");
REGISTER_HELP(CLASSICRESULT_GETWARNINGSCOUNT_DETAIL1,
              "See <<<getWarnings>>>() for more details.");

/**
 * $(CLASSICRESULT_GETWARNINGSCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETWARNINGSCOUNT_RETURNS)
 *
 * $(CLASSICRESULT_GETWARNINGSCOUNT_DETAIL)
 *
 * \sa warnings
 */
#if DOXYGEN_JS
Integer ClassicResult::getWarningsCount() {}
#elif DOXYGEN_PY
int ClassicResult::get_warnings_count() {}
#endif

// Documentation of the getWarnings function
REGISTER_HELP_FUNCTION(getWarnings, ClassicResult);
REGISTER_HELP_PROPERTY(warnings, ClassicResult);
REGISTER_HELP(CLASSICRESULT_GETWARNINGS_BRIEF,
              "Retrieves the warnings generated by the executed operation.");
REGISTER_HELP(CLASSICRESULT_WARNINGS_BRIEF,
              "${CLASSICRESULT_GETWARNINGS_BRIEF}");
REGISTER_HELP(CLASSICRESULT_GETWARNINGS_RETURNS,
              "@returns a list containing a warning object "
              "for each generated warning.");
REGISTER_HELP(
    CLASSICRESULT_GETWARNINGS_DETAIL,
    "Each warning object contains a "
    "key/value pair describing the information related to a specific warning.");
REGISTER_HELP(CLASSICRESULT_GETWARNINGS_DETAIL1,
              "This information includes: "
              "level, code and message.");

/**
 * $(CLASSICRESULT_GETWARNINGS_BRIEF)
 *
 * $(CLASSICRESULT_GETWARNINGS_RETURNS)
 *
 * This is the same value than C API mysql_warning_count, see
 * https://dev.mysql.com/doc/refman/en/mysql-warning-count.html
 *
 * $(CLASSICRESULT_GETWARNINGS_DETAIL)
 * $(CLASSICRESULT_GETWARNINGS_DETAIL1)
 */
#if DOXYGEN_JS
List ClassicResult::getWarnings() {}
#elif DOXYGEN_PY
list ClassicResult::get_warnings() {}
#endif

/*static shcore::Value get_field_type(Field &meta) {
  std::string type_name;
  switch (meta.type()) {
    case MYSQL_TYPE_NULL:
      type_name = "NULL";
      break;
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
      type_name = "DECIMAL";
      break;
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_NEWDATE:
      type_name = "DATE";
      break;
    case MYSQL_TYPE_TIME2:
    case MYSQL_TYPE_TIME:
      type_name = "TIME";
      break;
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
      if (meta.flags() & ENUM_FLAG)
        type_name = "ENUM";
      else if (meta.flags() & SET_FLAG)
        type_name = "SET";
      else
        type_name = "STRING";
      break;
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      type_name = "BYTES";
      break;
    case MYSQL_TYPE_GEOMETRY:
      type_name = "GEOMETRY";
      break;
    case MYSQL_TYPE_JSON:
      type_name = "JSON";
      break;
    case MYSQL_TYPE_YEAR:
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONG:
      type_name = "INT";
      break;
    case MYSQL_TYPE_LONGLONG:
      type_name = "BIGINT";
      break;
    case MYSQL_TYPE_FLOAT:
      type_name = "FLOAT";
      break;
    case MYSQL_TYPE_DOUBLE:
      type_name = "DOUBLE";
      break;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME2:
    case MYSQL_TYPE_TIMESTAMP2:
      // The difference between TIMESTAMP and DATETIME is entirely in terms
      // of internal representation at the server side. At the client side,
      // there is no difference.
      // TIMESTAMP is the number of seconds since epoch, so it cannot store
      // dates before 1970. DATETIME is an arbitrary date and time value,
      // so it does not have that limitation.
      type_name = "DATETIME";
      break;
    case MYSQL_TYPE_BIT:
      type_name = "BIT";
      break;
    case MYSQL_TYPE_ENUM:
      type_name = "ENUM";
      break;
    case MYSQL_TYPE_SET:
      type_name = "SET";
      break;
  }

  assert(!type_name.empty());
  return mysqlsh::Constant::get_constant("mysqlx", "Type", type_name,
                                         shcore::Argument_list());
}*/

shcore::Value ClassicResult::get_member(const std::string &prop) const {
  if (prop == "affectedRowCount" || prop == "affectedItemsCount") {
    if (prop == "affectedRowCount") {
      log_warning("'%s' is deprecated, use '%s' instead.",
                  get_function_name("affectedRowCount").c_str(),
                  get_function_name("affectedItemsCount").c_str());
    }

    return shcore::Value(_result->get_affected_row_count());
  }

  if (prop == "warningCount" || prop == "warningsCount") {
    if (prop == "warningCount") {
      log_warning("'%s' is deprecated, use '%s' instead.",
                  get_function_name("warningCount").c_str(),
                  get_function_name("warningsCount").c_str());
    }

    return shcore::Value(_result->get_warning_count());
  }

  if (prop == "warnings") {
    std::shared_ptr<shcore::Value::Array_type> array(
        new shcore::Value::Array_type);
    if (_result) {
      while (std::unique_ptr<mysqlshdk::db::Warning> warning =
                 _result->fetch_one_warning()) {
        mysqlsh::Row *warning_row = new mysqlsh::Row();
        switch (warning->level) {
          case mysqlshdk::db::Warning::Level::Note:
            warning_row->add_item("level", shcore::Value("Note"));
            break;
          case mysqlshdk::db::Warning::Level::Warn:
            warning_row->add_item("level", shcore::Value("Warning"));
            break;
          case mysqlshdk::db::Warning::Level::Error:
            warning_row->add_item("level", shcore::Value("Error"));
            break;
        }
        warning_row->add_item("code", shcore::Value(warning->code));
        warning_row->add_item("message", shcore::Value(warning->msg));

        array->push_back(shcore::Value::wrap(warning_row));
      }
    }

    return shcore::Value(array);

    /*    auto inner_warnings = _result->query_warnings().release();
        std::shared_ptr<ClassicResult> warnings(
            new ClassicResult(std::shared_ptr<Result>(inner_warnings)));
        return warnings->fetch_all(shcore::Argument_list());*/
  }

  if (prop == "executionTime")
    return shcore::Value(
        mysqlshdk::utils::format_seconds(_result->get_execution_time()));

  if (prop == "autoIncrementValue")
    return shcore::Value((int)_result->get_auto_increment_value());

  if (prop == "info") return shcore::Value(_result->get_info());

  if (prop == "columnCount") {
    size_t count = _result->get_metadata().size();

    return shcore::Value((uint64_t)count);
  }

  if (prop == "columnNames") {
    std::shared_ptr<shcore::Value::Array_type> array(
        new shcore::Value::Array_type);
    for (auto &cmd : _result->get_metadata())
      array->push_back(shcore::Value(cmd.get_column_label()));

    return shcore::Value(array);

    /*std::vector<Field> metadata(_result->get_metadata());

    std::shared_ptr<shcore::Value::Array_type> array(
        new shcore::Value::Array_type);

    int num_fields = metadata.size();

    for (int i = 0; i < num_fields; i++)
      array->push_back(shcore::Value(metadata[i].name()));

    return shcore::Value(array);*/
  }

  if (prop == "columns") {
    return shcore::Value(get_columns());
    /*std::vector<Field> metadata(_result->get_metadata());

    std::shared_ptr<shcore::Value::Array_type> array(
        new shcore::Value::Array_type);

    int num_fields = metadata.size();

    for (int i = 0; i < num_fields; i++) {
      bool numeric = IS_NUM(metadata[i].type());

      std::shared_ptr<mysqlsh::Column> column(new mysqlsh::Column(
          metadata[i].db(), metadata[i].org_table(), metadata[i].table(),
          metadata[i].org_name(), metadata[i].name(),
          get_field_type(metadata[i]),  // type
          metadata[i].length(), metadata[i].decimals(),
          (metadata[i].flags() & UNSIGNED_FLAG) == 0,  // signed
          mysqlshdk::db::charset::collation_name_from_collation_id(
              metadata[i].charset()),
          mysqlshdk::db::charset::charset_name_from_collation_id(
              metadata[i].charset()),
          numeric ? metadata[i].flags() & ZEROFILL_FLAG : false));  // zerofill

      array->push_back(
          shcore::Value(std::static_pointer_cast<Object_bridge>(column)));
    }

    return shcore::Value(array);*/
  }

  return ShellBaseResult::get_member(prop);
}

shcore::Value::Array_type_ref ClassicResult::get_columns() const {
  if (!_columns) {
    _columns.reset(new shcore::Value::Array_type);

    for (auto &column_meta : _result->get_metadata()) {
      std::string type_name;
      switch (column_meta.get_type()) {
        case mysqlshdk::db::Type::Null:
          type_name = "NULL";
          break;
        case mysqlshdk::db::Type::String:
          type_name = "STRING";
          break;
        case mysqlshdk::db::Type::Integer:
        case mysqlshdk::db::Type::UInteger:
          type_name = "INT";
          switch (column_meta.get_length()) {
            case 3:
            case 4:
              type_name = "TINYINT";
              break;
            case 5:
            case 6:
              type_name = "SMALLINT";
              break;
            case 8:
            case 9:
              type_name = "MEDIUMINT";
              break;
            case 10:
            case 11:
              type_name = "INT";
              break;
            case 20:
              type_name = "BIGINT";
              break;
          }
          break;
        case mysqlshdk::db::Type::Float:
          type_name = "FLOAT";
          break;
        case mysqlshdk::db::Type::Double:
          type_name = "DOUBLE";
          break;
        case mysqlshdk::db::Type::Decimal:
          type_name = "DECIMAL";
          break;
        case mysqlshdk::db::Type::Bytes:
          type_name = "BYTES";
          break;
        case mysqlshdk::db::Type::Geometry:
          type_name = "GEOMETRY";
          break;
        case mysqlshdk::db::Type::Json:
          type_name = "JSON";
          break;
        case mysqlshdk::db::Type::DateTime:
          type_name = "DATETIME";
          break;
        case mysqlshdk::db::Type::Date:
          type_name = "DATE";
          break;
        case mysqlshdk::db::Type::Time:
          type_name = "TIME";
          break;
        case mysqlshdk::db::Type::Bit:
          type_name = "BIT";
          break;
        case mysqlshdk::db::Type::Enum:
          type_name = "ENUM";
          break;
        case mysqlshdk::db::Type::Set:
          type_name = "SET";
          break;
      }
      assert(!type_name.empty());
      shcore::Value data_type = mysqlsh::Constant::get_constant(
          "mysql", "Type", type_name, shcore::Argument_list());

      assert(data_type);

      _columns->push_back(
          shcore::Value::wrap(new mysqlsh::Column(column_meta, data_type)));
    }
  }

  return _columns;
}

void ClassicResult::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();

  dumper.append_value("executionTime", get_member("executionTime"));

  dumper.append_value("info", get_member("info"));
  dumper.append_value("rows", fetch_all(shcore::Argument_list()));

  if (mysqlsh::current_shell_options()->get().show_warnings) {
    dumper.append_value("warningCount", get_member("warningsCount"));
    dumper.append_value("warningsCount", get_member("warningsCount"));
    dumper.append_value("warnings", get_member("warnings"));
  }

  dumper.append_value("hasData", has_data(shcore::Argument_list()));
  dumper.append_value("affectedRowCount", get_member("affectedItemsCount"));
  dumper.append_value("affectedItemsCount", get_member("affectedItemsCount"));
  dumper.append_value("autoIncrementValue", get_member("autoIncrementValue"));

  dumper.end_object();
}
