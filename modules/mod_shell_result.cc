/*
 * Copyright (c) 2023, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/mod_shell_result.h"

#include <utility>

#include "mysqlshdk/include/shellcore/shell_resultset_dumper.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/strformat.h"

namespace mysqlsh {

// Documentation of the ShellResult class
REGISTER_HELP_CLASS(ShellResult, mysql);
REGISTER_HELP_CLASS_TEXT(SHELLRESULT, R"*(
Encapsulates custom query result and metadata.

This class allows access to a custom result set created through shell.<<<createResult>>>.
)*");

ShellResult::ShellResult(std::shared_ptr<mysqlshdk::db::IResult> result)
    : ShellBaseResult(), m_result(std::move(result)) {
  add_property("affectedItemsCount", "getAffectedItemsCount");
  add_property("autoIncrementValue", "getAutoIncrementValue");
  add_property("columns", "getColumns");
  add_property("columnCount", "getColumnCount");
  add_property("columnNames", "getColumnNames");
  add_property("executionTime", "getExecutionTime");
  add_property("info", "getInfo");
  add_property("warnings", "getWarnings");
  add_property("warningsCount", "getWarningsCount");

  expose("fetchOne", &ShellResult::fetch_one);
  expose("fetchAll", &ShellResult::fetch_all);
  expose("hasData", &ShellResult::has_data);
  expose("nextResult", &ShellResult::next_result);
}

// Documentation of the hasData function
REGISTER_HELP_FUNCTION(hasData, ShellResult);
REGISTER_HELP(SHELLRESULT_HASDATA_BRIEF,
              "Returns true if there are records in the result.");
/**
 * $(SHELLRESULT_HASDATA_BRIEF)
 */
#if DOXYGEN_JS
Bool ShellResult::hasData() {}
#elif DOXYGEN_PY
bool ShellResult::has_data() {}
#endif
bool ShellResult::has_data() const { return m_result->has_resultset(); }

REGISTER_HELP_FUNCTION(fetchOne, ShellResult);
REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_FETCHONE, R"*(
Retrieves the next Row on the ShellResult.

@returns A Row object representing the next record in the result.
)*");
/**
 * $(SHELLRESULT_FETCHONE_BRIEF)
 *
 * $(SHELLRESULT_FETCHONE)
 */
#if DOXYGEN_JS
Row ShellResult::fetchOne() {}
#elif DOXYGEN_PY
Row ShellResult::fetch_one() {}
#endif
std::shared_ptr<Row> ShellResult::fetch_one() const {
  std::shared_ptr<Row> ret_val;

  auto result = get_result();
  auto columns = get_column_names();
  if (result && columns) {
    if (const auto row = result->fetch_one()) {
      ret_val = std::make_shared<Row>(columns, *row);
    }
  }

  return ret_val;
}

REGISTER_HELP_FUNCTION(fetchAll, ShellResult);
REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_FETCHALL, R"*(
Returns a list of Row objects which contains an element for every record left
on the result.

@returns A List of Row objects.

This function will return a Row for every record on the resultset unless 
<<<fetchOne>>> is called before, in such case it will return a Row for each 
of the remaining records on the resultset.
)*");
/**
 * $(SHELLRESULT_FETCHALL_BRIEF)
 *
 * $(SHELLRESULT_FETCHALL)
 */
#if DOXYGEN_JS
List ShellResult::fetchAll() {}
#elif DOXYGEN_PY
list ShellResult::fetch_all() {}
#endif
shcore::Array_t ShellResult::fetch_all() const {
  auto array = shcore::make_array();

  while (auto record = fetch_one()) {
    array->push_back(shcore::Value(std::move(record)));
  }

  return array;
}

// Documentation of nextResult function
REGISTER_HELP_FUNCTION(nextResult, ShellResult);
REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_NEXTRESULT, R"*(
Prepares the ShellResult to start reading data from the next Result (if many
results were returned).

@returns A boolean value indicating whether there is another result or not.
)*");
/**
 * $(SHELLRESULT_NEXTRESULT_BRIEF)
 *
 * $(SHELLRESULT_NEXTRESULT)
 */
#if DOXYGEN_JS
Bool ShellResult::nextResult() {}
#elif DOXYGEN_PY
bool ShellResult::next_result() {}
#endif
bool ShellResult::next_result() {
  reset_column_cache();
  return m_result->next_resultset();
}

// Documentation of the getColumnCount function
REGISTER_HELP_FUNCTION(getColumnCount, ShellResult);
REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_GETCOLUMNCOUNT, R"*(
Retrieves the number of columns on the current result.

@returns the number of columns on the current result.
)*");
REGISTER_HELP_PROPERTY(columnCount, ShellResult);
REGISTER_HELP(SHELLRESULT_COLUMNCOUNT_BRIEF,
              "${SHELLRESULT_GETCOLUMNCOUNT_BRIEF}");
/**
 * $(SHELLRESULT_GETCOLUMNCOUNT_BRIEF)
 *
 * $(SHELLRESULT_GETCOLUMNCOUNT)
 */
#if DOXYGEN_JS
Integer ShellResult::getColumnCount() {}
#elif DOXYGEN_PY
int ShellResult::get_column_count() {}
#endif

// Documentation of the getColumnCount function
REGISTER_HELP_FUNCTION(getColumnNames, ShellResult);

REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_GETCOLUMNNAMES, R"*(
Gets the columns on the current result.

@returns A list with the names of the columns returned on the active result.
)*");
REGISTER_HELP_PROPERTY(columnNames, ShellResult);
REGISTER_HELP(SHELLRESULT_COLUMNNAMES_BRIEF,
              "${SHELLRESULT_GETCOLUMNNAMES_BRIEF}");

/**
 * $(SHELLRESULT_GETCOLUMNNAMES_BRIEF)
 *
 * $(SHELLRESULT_GETCOLUMNNAMES)
 */
#if DOXYGEN_JS
List ShellResult::getColumnNames() {}
#elif DOXYGEN_PY
list ShellResult::get_column_names() {}
#endif

// Documentation of the getColumns function
REGISTER_HELP_FUNCTION(getColumns, ShellResult);

REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_GETCOLUMNS, R"*(
Gets the column metadata for the columns on the current result.

@returns a list of column metadata objects containing information about the
columns included on the current result.
)*");
REGISTER_HELP_PROPERTY(columns, ShellResult);
REGISTER_HELP(SHELLRESULT_COLUMNS_BRIEF, "${SHELLRESULT_GETCOLUMNS_BRIEF}");
/**
 * $(SHELLRESULT_GETCOLUMNS_BRIEF)
 *
 * $(SHELLRESULT_GETCOLUMNS)
 */
#if DOXYGEN_JS
List ShellResult::getColumns() {}
#elif DOXYGEN_PY
list ShellResult::get_columns() {}
#endif

// Documentation of the getInfo function
REGISTER_HELP_FUNCTION(getInfo, ShellResult);
REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_GETINFO, R"*(
Retrieves a string providing additional information about the result.

@returns a string with the additional information.
)*");
REGISTER_HELP_PROPERTY(info, ShellResult);
REGISTER_HELP(SHELLRESULT_INFO_BRIEF, "${SHELLRESULT_GETINFO_BRIEF}");
/**
 * $(SHELLRESULT_GETINFO_BRIEF)
 *
 * $(SHELLRESULT_GETINFO)
 */
#if DOXYGEN_JS
String ShellResult::getInfo() {}
#elif DOXYGEN_PY
str ShellResult::get_info() {}
#endif

// Documentation of getWarningsCount function
REGISTER_HELP_FUNCTION(getWarningsCount, ShellResult);
REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_GETWARNINGSCOUNT, R"*(
The number of warnings produced by the operation that generated this result.

@returns the number of warnings.

See <<<getWarnings>>>() for more details.
)*");
REGISTER_HELP_PROPERTY(warningsCount, ShellResult);
REGISTER_HELP(SHELLRESULT_WARNINGSCOUNT_BRIEF,
              "${SHELLRESULT_GETWARNINGSCOUNT_BRIEF}");

/**
 * $(SHELLRESULT_GETWARNINGSCOUNT_BRIEF)
 *
 * $(SHELLRESULT_GETWARNINGSCOUNT)
 *
 * \sa warnings
 */
#if DOXYGEN_JS
Integer ShellResult::getWarningsCount() {}
#elif DOXYGEN_PY
int ShellResult::get_warnings_count() {}
#endif

// Documentation of the getWarnings function
REGISTER_HELP_FUNCTION(getWarnings, ShellResult);
REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_GETWARNINGS, R"*(
Retrieves the warnings generated by operation that generated this result.

@returns a list containing a warning object for each generated warning.

Each warning object contains a key/value pair describing the information
related to a specific warning.

This information includes: level, code and message.
)*");
REGISTER_HELP_PROPERTY(warnings, ShellResult);
REGISTER_HELP(SHELLRESULT_WARNINGS_BRIEF, "${SHELLRESULT_GETWARNINGS_BRIEF}");
/**
 * $(SHELLRESULT_GETWARNINGS_BRIEF)
 *
 * $(SHELLRESULT_GETWARNINGS)
 *
 */
#if DOXYGEN_JS
List ShellResult::getWarnings() {}
#elif DOXYGEN_PY
list ShellResult::get_warnings() {}
#endif

// Documentation of getAffectedItemsCount function
REGISTER_HELP_FUNCTION(getAffectedItemsCount, ShellResult);
REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_GETAFFECTEDITEMSCOUNT, R"*(
The the number of affected items for the last operation.

@returns the number of affected items.
)*");

REGISTER_HELP_PROPERTY(affectedItemsCount, ShellResult);
REGISTER_HELP(SHELLRESULT_AFFECTEDITEMSCOUNT_BRIEF,
              "${SHELLRESULT_GETAFFECTEDITEMSCOUNT_BRIEF}");

/**
 * $(SHELLRESULT_GETAFFECTEDITEMSCOUNT_BRIEF)
 *
 * $(SHELLRESULT_GETAFFECTEDITEMSCOUNT)
 */
#if DOXYGEN_JS
Integer ShellResult::getAffectedItemsCount() {}
#elif DOXYGEN_PY
int ShellResult::get_affected_items_count() {}
#endif

// Documentation of the getAutoIncrementValue function
REGISTER_HELP_PROPERTY(autoIncrementValue, ShellResult);
REGISTER_HELP(SHELLRESULT_AUTOINCREMENTVALUE_BRIEF,
              "${SHELLRESULT_GETAUTOINCREMENTVALUE_BRIEF}");
REGISTER_HELP_FUNCTION(getAutoIncrementValue, ShellResult);
REGISTER_HELP_FUNCTION_TEXT(SHELLRESULT_GETAUTOINCREMENTVALUE, R"*(
Returns the last insert id auto generated (from an insert operation).

@returns the integer representing the last insert id.
)*");
/**
 * $(SHELLRESULT_GETAUTOINCREMENTVALUE_BRIEF)
 *
 * $(SHELLRESULT_GETAUTOINCREMENTVALUE)
 */
#if DOXYGEN_JS
Integer ShellResult::getAutoIncrementValue() {}
#elif DOXYGEN_PY
int ShellResult::get_auto_increment_value() {}
#endif

// Documentation of the getExecutionTime function
REGISTER_HELP_FUNCTION(getExecutionTime, ShellResult);
REGISTER_HELP(SHELLRESULT_GETEXECUTIONTIME_BRIEF,
              "Retrieves a string value indicating the execution time of the "
              "operation that generated this result.");
REGISTER_HELP_PROPERTY(executionTime, ShellResult);
REGISTER_HELP(SHELLRESULT_EXECUTIONTIME_BRIEF,
              "${SHELLRESULT_GETEXECUTIONTIME_BRIEF}");
/**
 * $(SHELLRESULT_EXECUTIONTIME_BRIEF)
 */
#if DOXYGEN_JS
String ShellResult::getExecutionTime() {}
#elif DOXYGEN_PY
str ShellResult::get_execution_time() {}
#endif

shcore::Value ShellResult::get_member(const std::string &prop) const {
  if (prop == "affectedItemsCount") {
    return shcore::Value(m_result->get_affected_row_count());
  }

  if (prop == "autoIncrementValue") {
    return shcore::Value(m_result->get_auto_increment_value());
  }

  if (prop == "warningsCount") {
    return shcore::Value(m_result->get_warning_count());
  }

  if (prop == "warnings") {
    auto array = shcore::make_array();
    if (m_result) {
      while (auto warning = m_result->fetch_one_warning()) {
        auto warning_row = std::make_shared<mysqlsh::Row>();
        warning_row->add_item(
            "level", shcore::Value(mysqlshdk::db::to_string(warning->level)));
        warning_row->add_item("code", shcore::Value(warning->code));
        warning_row->add_item("message", shcore::Value(warning->msg));

        array->push_back(shcore::Value::wrap(std::move(warning_row)));
      }
    }

    return shcore::Value(std::move(array));
  }

  if (prop == "executionTime")
    return shcore::Value(
        mysqlshdk::utils::format_seconds(m_result->get_execution_time()));

  if (prop == "info") return shcore::Value(m_result->get_info());

  if (prop == "columnCount") {
    size_t count = m_result->get_metadata().size();

    return shcore::Value((uint64_t)count);
  }

  if (prop == "columnNames") {
    update_column_cache();

    auto array = shcore::make_array();

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

void ShellResult::rewind() { m_result->rewind(); }

}  // namespace mysqlsh
