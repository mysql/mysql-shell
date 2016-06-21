/*
 * Copyright (c) 2014, 2016 Oracle and/or its affiliates. All rights reserved.
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

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <iomanip>
#include "mod_mysql_resultset.h"
#include "mysql_connection.h"
#include "shellcore/shell_core_options.h"

using namespace mysh;
using namespace shcore;
using namespace mysh::mysql;

ClassicResult::ClassicResult(boost::shared_ptr<Result> result)
  : _result(result)
{
  add_property("columns", "getColumns");
  add_property("columnCount", "getColumnCount");
  add_property("columnNames", "getColumnNames");
  add_property("affectedRowCount", "getAffectedRowCount");
  add_property("warningCount", "getWarningCount");
  add_property("warnings", "getWarnings");
  add_property("executionTime", "getExecutionTime");
  add_property("autoIncrementValue", "getAutoIncrementValue");
  add_property("info", "getInfo");

  add_method("fetchOne", boost::bind(&ClassicResult::fetch_one, this, _1), "nothing", shcore::String, NULL);
  add_method("fetchAll", boost::bind(&ClassicResult::fetch_all, this, _1), "nothing", shcore::String, NULL);
  add_method("nextDataSet", boost::bind(&ClassicResult::next_data_set, this, _1), "nothing", shcore::String, NULL);
  add_method("hasData", boost::bind(&ClassicResult::has_data, this, _1), "nothing", shcore::String, NULL);
}

#ifdef DOXYGEN

/**
* Returns true if the last statement execution has a result set.
*/
Bool ClassicResult::hasData(){}
#endif
shcore::Value ClassicResult::has_data(const shcore::Argument_list &args) const
{
  args.ensure_count(0, get_function_name("hasData").c_str());

  return Value(_result->has_resultset());
}

shcore::Value ClassicResult::fetch_one(const shcore::Argument_list &args) const
{
  args.ensure_count(0, get_function_name("fetchOne").c_str());
  Row *inner_row = _result->fetch_one();

  if (inner_row)
  {
    mysh::Row *value_row = new mysh::Row();

    std::vector<Field> metadata(_result->get_metadata());

    for (size_t index = 0; index < metadata.size(); index++)
      value_row->add_item(metadata[index].name(), inner_row->get_value(index));

    return shcore::Value::wrap(value_row);
  }

  return shcore::Value::Null();
}

shcore::Value ClassicResult::next_data_set(const shcore::Argument_list &args)
{
  args.ensure_count(0, get_function_name("nextDataSet").c_str());

  return shcore::Value(_result->next_data_set());
}

shcore::Value ClassicResult::fetch_all(const shcore::Argument_list &args) const
{
  args.ensure_count(0, get_function_name("fetchAll").c_str());

  boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

  shcore::Value record = fetch_one(args);

  while (record)
  {
    array->push_back(record);
    record = fetch_one(args);
  }

  return shcore::Value(array);
}

#ifdef DOXYGEN
/**
* Retrieves the next Row on the ClassicResult.
* \return A Row object representing the next record in the result.
*/
Row ClassicResult::fetchOne(){}

/**
* Returns a list of Row objects which contains an element for every record left on the result.
* \return A List of Row objects.
*
* If this function is called right after executing a query, it will return a Row for every record on the resultset.
*
* If fetchOne is called before this function, when this function is called it will return a Row for each of the remaining records on the resultset.
*/
List ClassicResult::fetchAll(){}

/**
* The the number of affected rows for the last operation.
* \return the number of affected rows.
* This is the value of the C API mysql_affected_rows(), see https://dev.mysql.com/doc/refman/5.7/en/mysql-affected-rows.html
*/
Integer ClassicResult::getAffectedRowCount(){}

/**
* Retrieves the number of columns on the current result.
* \return the number of columns on the current result.
*/
Integer ClassicResult::getColumnCount(){}

/**
* Gets the columns on the current result.
* \return A list with the names of the columns returned on the active result.
*/
List ClassicResult::getColumnNames(){}

/**
* Gets the column metadata for the columns on the active result.
* \return a list of column metadata objects containing information about the columns included on the active result.
*/
List ClassicResult::getColumns(){}

/**
* Retrieves a string value indicating the execution time of the executed operation.
*/
String ClassicResult::getExecutionTime(){}

/**
* Retrieves a string providing information about the most recently executed statement.
* \return a string with the execution information
*
* For more details, see: https://dev.mysql.com/doc/refman/5.7/en/mysql-info.html
*/
String ClassicResult::getInfo(){}

/**
* Returns the last insert id auto generated (from an insert operation)
* \return the integer representing the last insert id
*
* For more details, see https://dev.mysql.com/doc/refman/5.7/en/information-functions.html#function_last-insert-id
*/
Integer ClassicResult::getAutoIncrementValue(){}

/**
* The number of warnings produced by the last statement execution.
* \return the number of warnings.
*
* This is the same value than C API mysql_warning_count, see https://dev.mysql.com/doc/refman/5.7/en/mysql-warning-count.html
*
* \sa getWarnings()
*/
Integer ClassicResult::getWarningCount(){}

/**
* Retrieves the warnings generated by the executed operation.
* \return A list containing a warning object for each generated warning.
* This is the same value than C API mysql_warning_count, see https://dev.mysql.com/doc/refman/5.7/en/mysql-warning-count.html
*
* Each warning object contains a key/value pair describing the information related to a specific warning.
* This information includes: Level, Code and Message.
*/
List ClassicResult::getWarnings(){}

/**
* Prepares the SqlResult to start reading data from the next Result (if many results were returned).
* \return A boolean value indicating whether there is another result or not.
*/
Bool ClassicResult::nextDataSet(){}
#endif
shcore::Value ClassicResult::get_member(const std::string &prop) const
{
  if (prop == "affectedRowCount")
    return shcore::Value((int64_t)((_result->affected_rows() == ~(my_ulonglong)0) ? 0 : _result->affected_rows()));

  if (prop == "warningCount")
    return shcore::Value(_result->warning_count());

  if (prop == "warnings")
  {
    Result* inner_warnings = _result->query_warnings();
    boost::shared_ptr<ClassicResult> warnings(new ClassicResult(boost::shared_ptr<Result>(inner_warnings)));
    return warnings->fetch_all(shcore::Argument_list());
  }

  if (prop == "executionTime")
    return shcore::Value(MySQL_timer::format_legacy(_result->execution_time(), 2));

  if (prop == "autoIncrementValue")
    return shcore::Value((int)_result->last_insert_id());

  if (prop == "info")
    return shcore::Value(_result->info());

  if (prop == "columnCount")
  {
    size_t count = _result->get_metadata().size();

    return shcore::Value((uint64_t)count);
  }

  if (prop == "columnNames")
  {
    std::vector<Field> metadata(_result->get_metadata());

    boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

    int num_fields = metadata.size();

    for (int i = 0; i < num_fields; i++)
      array->push_back(shcore::Value(metadata[i].name()));

    return shcore::Value(array);
  }

  if (prop == "columns")
  {
    std::vector<Field> metadata(_result->get_metadata());

    boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

    int num_fields = metadata.size();

    for (int i = 0; i < num_fields; i++)
    {
      bool numeric = IS_NUM(metadata[i].type());
      boost::shared_ptr<mysh::Column> column(new mysh::Column(
        metadata[i].db(),
        metadata[i].org_table(),
        metadata[i].table(),
        metadata[i].org_name(),
        metadata[i].name(),
  shcore::Value(), //type
        metadata[i].length(),
  numeric,
  metadata[i].decimals(),
  false, // signed
  Charset::item[metadata[i].charset()].collation,
  Charset::item[metadata[i].charset()].name,
        false //padded
      ));

      array->push_back(shcore::Value(boost::static_pointer_cast<Object_bridge>(column)));
    }

    return shcore::Value(array);
  }

  return ShellBaseResult::get_member(prop);
}

void ClassicResult::append_json(shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  dumper.append_value("executionTime", get_member("executionTime"));

  dumper.append_value("info", get_member("info"));
  dumper.append_value("rows", fetch_all(shcore::Argument_list()));

  if (Shell_core_options::get()->get_bool(SHCORE_SHOW_WARNINGS))
  {
    dumper.append_value("warningCount", get_member("warningCount"));
    dumper.append_value("warnings", get_member("warnings"));
  }

  dumper.append_value("hasData", has_data(shcore::Argument_list()));
  dumper.append_value("affectedRowCount", get_member("affectedRowCount"));
  dumper.append_value("autoIncrementValue", get_member("autoIncrementValue"));

  dumper.end_object();
}