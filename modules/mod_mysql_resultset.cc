/*
 * Copyright (c) 2014, 2021, Oracle and/or its affiliates.
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

#include "modules/mod_mysql_resultset.h"

#include <iomanip>
#include <string>

#include "modules/devapi/base_constants.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/charset.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_json.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace shcore;
using namespace mysqlsh::mysql;

// Documentation of the ClassicResult class
REGISTER_HELP_CLASS(ClassicResult, mysql);
REGISTER_HELP_CLASS_TEXT(CLASSICRESULT, R"*(
Allows browsing through the result information after performing an operation
on the database through the MySQL Protocol.

This class allows access to the result set from the classic MySQL data model
to be retrieved from Dev API queries.
)*");
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

  expose("fetchOne", &ClassicResult::fetch_one);
  expose("fetchOneObject", &ClassicResult::_fetch_one_object);
  expose("fetchAll", &ClassicResult::fetch_all);
  expose("nextDataSet", &ClassicResult::next_data_set);
  expose("nextResult", &ClassicResult::next_result);
  expose("hasData", &ClassicResult::has_data);
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
bool ClassicResult::has_data() const { return _result->has_resultset(); }

// Documentation of the fetchOne function
REGISTER_HELP_FUNCTION(fetchOne, ClassicResult);
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_FETCHONE, R"*(
Retrieves the next Row on the ClassicResult.

@returns A Row object representing the next record in the result.
)*");
/**
 * $(CLASSICRESULT_FETCHONE_BRIEF)
 *
 * $(CLASSICRESULT_FETCHONE)
 */
#if DOXYGEN_JS
Row ClassicResult::fetchOne() {}
#elif DOXYGEN_PY
Row ClassicResult::fetch_one() {}
#endif
std::shared_ptr<mysqlsh::Row> ClassicResult::fetch_one() const {
  return fetch_one_row();
}

REGISTER_HELP_FUNCTION(fetchOneObject, ClassicResult);
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_FETCHONEOBJECT, R"*(
Retrieves the next Row on the result and returns it as an object.

@returns A dictionary containing the row information.

The column names will be used as keys in the returned dictionary and the column
data will be used as the key values.

If a column is a valid identifier it will be accessible as an object attribute
as @<dict@>.@<column@>.

If a column is not a valid identifier, it will be accessible as a dictionary
key as @<dict@>[@<column@>].
)*");
/**
 * $(CLASSICRESULT_FETCHONEOBJECT_BRIEF)
 *
 * $(CLASSICRESULT_FETCHONEOBJECT)
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
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_NEXTDATASET, R"*(
Prepares the SqlResult to start reading data from the next Result (if many
results were returned).

@returns A boolean value indicating whether there is another result or not.

@attention This function will be removed in a future release, use the
<b><<<nextResult>>></b> function instead.
)*");
/**
 * $(CLASSICRESULT_NEXTDATASET_BRIEF)
 *
 * $(CLASSICRESULT_NEXTDATASET)
 */
#if DOXYGEN_JS
Bool ClassicResult::nextDataSet() {}
#elif DOXYGEN_PY
bool ClassicResult::next_data_set() {}
#endif
bool ClassicResult::next_data_set() {
  log_warning("'%s' is deprecated, use '%s' instead.",
              get_function_name("nextDataSet").c_str(),
              get_function_name("nextResult").c_str());

  return next_result();
}

// Documentation of nextResult function
REGISTER_HELP_FUNCTION(nextResult, ClassicResult);
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_NEXTRESULT, R"*(
Prepares the SqlResult to start reading data from the next Result (if many
results were returned).

@returns A boolean value indicating whether there is another esult or not.
)*");
/**
 * $(CLASSICRESULT_NEXTRESULT_BRIEF)
 *
 * $(CLASSICRESULT_NEXTRESULT)
 */
#if DOXYGEN_JS
Bool ClassicResult::nextResult() {}
#elif DOXYGEN_PY
bool ClassicResult::next_result() {}
#endif
bool ClassicResult::next_result() {
  reset_column_cache();
  return _result->next_resultset();
}

// Documentation of the fetchAll function
REGISTER_HELP_FUNCTION(fetchAll, ClassicResult);
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_FETCHALL, R"*(
Returns a list of Row objects which contains an element for every record left
on the result.

@returns A List of Row objects.

If this function is called right after executing a query, it will return a Row
for every record on the resultset.

If fetchOne is called before this function, when this function is called it
will return a Row for each of the remaining records on the resultset.
)*");
/**
 * $(CLASSICRESULT_FETCHALL_BRIEF)
 *
 * $(CLASSICRESULT_FETCHALL)
 */
#if DOXYGEN_JS
List ClassicResult::fetchAll() {}
#elif DOXYGEN_PY
list ClassicResult::fetch_all() {}
#endif
shcore::Array_t ClassicResult::fetch_all() const {
  auto array = shcore::make_array();

  while (const auto record = fetch_one()) {
    array->push_back(shcore::Value(record));
  }

  return array;
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
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETAFFECTEDROWCOUNT, R"*(
The number of affected rows for the last operation.

@returns the number of affected rows.

@attention This function will be removed in a future release, use the
<b><<<getAffectedItemsCount>>></b> function instead.

This is the value of the C API mysql_affected_rows(), see
https://dev.mysql.com/doc/refman/en/mysql-affected-rows.html
)*");
/**
 * $(CLASSICRESULT_GETAFFECTEDROWCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETAFFECTEDROWCOUNT)
 */
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
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETAFFECTEDITEMSCOUNT, R"*(
The the number of affected items for the last operation.

@returns the number of affected items.
)*");
/**
 * $(CLASSICRESULT_GETAFFECTEDITEMSCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETAFFECTEDITEMSCOUNT)
 */
#if DOXYGEN_JS
Integer ClassicResult::getAffectedItemsCount() {}
#elif DOXYGEN_PY
int ClassicResult::get_affected_items_count() {}
#endif

// Documentation of the getColumnCount function
REGISTER_HELP_FUNCTION(getColumnCount, ClassicResult);
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETCOLUMNCOUNT, R"*(
Retrieves the number of columns on the current result.

@returns the number of columns on the current result.
)*");
REGISTER_HELP_PROPERTY(columnCount, ClassicResult);
REGISTER_HELP(CLASSICRESULT_COLUMNCOUNT_BRIEF,
              "${CLASSICRESULT_GETCOLUMNCOUNT_BRIEF}");
/**
 * $(CLASSICRESULT_GETCOLUMNCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETCOLUMNCOUNT)
 */
#if DOXYGEN_JS
Integer ClassicResult::getColumnCount() {}
#elif DOXYGEN_PY
int ClassicResult::get_column_count() {}
#endif

// Documentation of the getColumnCount function
REGISTER_HELP_FUNCTION(getColumnNames, ClassicResult);

REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETCOLUMNNAMES, R"*(
Gets the columns on the current result.

@returns A list with the names of the columns returned on the active result.
)*");
REGISTER_HELP_PROPERTY(columnNames, ClassicResult);
REGISTER_HELP(CLASSICRESULT_COLUMNNAMES_BRIEF,
              "${CLASSICRESULT_GETCOLUMNNAMES_BRIEF}");

/**
 * $(CLASSICRESULT_GETCOLUMNNAMES_BRIEF)
 *
 * $(CLASSICRESULT_GETCOLUMNNAMES)
 */
#if DOXYGEN_JS
List ClassicResult::getColumnNames() {}
#elif DOXYGEN_PY
list ClassicResult::get_column_names() {}
#endif

// Documentation of the getColumns function
REGISTER_HELP_FUNCTION(getColumns, ClassicResult);

REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETCOLUMNS, R"*(
Gets the column metadata for the columns on the active result.

@returns a list of column metadata objects containing information about the
columns included on the active result.
)*");
REGISTER_HELP_PROPERTY(columns, ClassicResult);
REGISTER_HELP(CLASSICRESULT_COLUMNS_BRIEF, "${CLASSICRESULT_GETCOLUMNS_BRIEF}");
/**
 * $(CLASSICRESULT_GETCOLUMNS_BRIEF)
 *
 * $(CLASSICRESULT_GETCOLUMNS)
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
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETINFO, R"*(
Retrieves a string providing information about the most recently executed
statement.

@returns a string with the execution information.
)*");
REGISTER_HELP_PROPERTY(info, ClassicResult);
REGISTER_HELP(CLASSICRESULT_INFO_BRIEF, "${CLASSICRESULT_GETINFO_BRIEF}");
/**
 * $(CLASSICRESULT_GETINFO_BRIEF)
 *
 * $(CLASSICRESULT_GETINFO)
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
REGISTER_HELP_PROPERTY(autoIncrementValue, ClassicResult);
REGISTER_HELP(CLASSICRESULT_AUTOINCREMENTVALUE_BRIEF,
              "${CLASSICRESULT_GETAUTOINCREMENTVALUE_BRIEF}");
REGISTER_HELP_FUNCTION(getAutoIncrementValue, ClassicResult);
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETAUTOINCREMENTVALUE, R"*(
Returns the last insert id auto generated (from an insert operation).

@returns the integer representing the last insert id.
)*");
/**
 * $(CLASSICRESULT_GETAUTOINCREMENTVALUE_BRIEF)
 *
 * $(CLASSICRESULT_GETAUTOINCREMENTVALUE)
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
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETWARNINGCOUNT, R"*(
The number of warnings produced by the last statement execution.

@returns the number of warnings.

@attention This function will be removed in a future release, use the
<b><<<getWarningsCount>>></b> function instead.

This is the same value than C API mysql_warning_count, see
https://dev.mysql.com/doc/refman/en/mysql-warning-count.html

See <<<getWarnings>>>() for more details.
)*");
/**
 * $(CLASSICRESULT_GETWARNINGCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETWARNINGCOUNT)
 *
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
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETWARNINGSCOUNT, R"*(
The number of warnings produced by the last statement execution.

@returns the number of warnings.

This is the same value than C API mysql_warning_count, see
https://dev.mysql.com/doc/refman/en/mysql-warning-count.html

See <<<getWarnings>>>() for more details.
)*");
/**
 * $(CLASSICRESULT_GETWARNINGSCOUNT_BRIEF)
 *
 * $(CLASSICRESULT_GETWARNINGSCOUNT)
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
REGISTER_HELP_FUNCTION_TEXT(CLASSICRESULT_GETWARNINGS, R"*(
Retrieves the warnings generated by the executed operation.

@returns a list containing a warning object for each generated warning.

Each warning object contains a key/value pair describing the information
related to a specific warning.

This information includes: level, code and message.
)*");
REGISTER_HELP_PROPERTY(warnings, ClassicResult);
REGISTER_HELP(CLASSICRESULT_WARNINGS_BRIEF,
              "${CLASSICRESULT_GETWARNINGS_BRIEF}");
/**
 * $(CLASSICRESULT_GETWARNINGS_BRIEF)
 *
 * $(CLASSICRESULT_GETWARNINGS)
 *
 * This is the same value than C API mysql_warning_count, see
 * https://dev.mysql.com/doc/refman/en/mysql-warning-count.html
 */
#if DOXYGEN_JS
List ClassicResult::getWarnings() {}
#elif DOXYGEN_PY
list ClassicResult::get_warnings() {}
#endif

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
  }

  if (prop == "executionTime")
    return shcore::Value(
        mysqlshdk::utils::format_seconds(_result->get_execution_time()));

  if (prop == "autoIncrementValue")
    return shcore::Value(_result->get_auto_increment_value());

  if (prop == "info") return shcore::Value(_result->get_info());

  if (prop == "columnCount") {
    size_t count = _result->get_metadata().size();

    return shcore::Value((uint64_t)count);
  }

  if (prop == "columnNames") {
    auto array = shcore::make_array();

    update_column_cache();

    if (m_column_names) {
      for (auto &column : *m_column_names)
        array->push_back(shcore::Value(column));
    }

    return shcore::Value(array);
  }

  if (prop == "columns") {
    update_column_cache();

    if (m_columns) {
      return shcore::Value(m_columns);
    } else {
      return shcore::Value(shcore::make_array());
    }
  }

  return ShellBaseResult::get_member(prop);
}

void ClassicResult::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();

  dumper.append_value("executionTime", get_member("executionTime"));

  dumper.append_value("info", get_member("info"));
  dumper.append_value("rows", shcore::Value(fetch_all()));

  if (mysqlsh::current_shell_options()->get().show_warnings) {
    dumper.append_value("warningCount", get_member("warningsCount"));
    dumper.append_value("warningsCount", get_member("warningsCount"));
    dumper.append_value("warnings", get_member("warnings"));
  }

  dumper.append_value("hasData", shcore::Value(has_data()));
  dumper.append_value("affectedRowCount", get_member("affectedItemsCount"));
  dumper.append_value("affectedItemsCount", get_member("affectedItemsCount"));
  dumper.append_value("autoIncrementValue", get_member("autoIncrementValue"));

  dumper.end_object();
}
