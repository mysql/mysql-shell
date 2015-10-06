/*
 * Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
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
#include "mysql.h"

using namespace mysh;
using namespace shcore;
using namespace mysh::mysql;

ClassicResult::ClassicResult(boost::shared_ptr<Result> result)
: _result(result)
{
  add_method("fetchOne", boost::bind(&ClassicResult::fetch_one, this, _1), "nothing", shcore::String, NULL);
  add_method("fetchAll", boost::bind(&ClassicResult::fetch_all, this, _1), "nothing", shcore::String, NULL);
  add_method("nextDataSet", boost::bind(&ClassicResult::next_data_set, this, _1), "nothing", shcore::String, NULL);

  add_method("getColumns", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getColumns", "columns"), NULL);
  add_method("getColumnCount", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getColumnCount", "columnCount"), NULL);
  add_method("getColumnNames", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getColumnNames", "columnNames"), NULL);
  add_method("getAffectedRowCount", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getAffectedRowCount", "affectedRowCount"), NULL);
  add_method("getWarningCount", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getWarningCount", "warningCount"), NULL);
  add_method("getWarnings", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getWarnings", "warnings"), NULL);
  add_method("getExecutionTime", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getExecutionTime", "executionTime"), NULL);
  add_method("getLastInsertId", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getLastInsertId", "lastInsertId"), NULL);
  add_method("getInfo", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getInfo", "info"), NULL);
  add_method("getHasData", boost::bind(&ShellBaseResult::get_member_method, this, _1, "getHasData", "hasData"), NULL);
}

std::vector<std::string> ClassicResult::get_members() const
{
  std::vector<std::string> members(shcore::Cpp_object_bridge::get_members());
  members.push_back("columns");
  members.push_back("columnCount");
  members.push_back("columnNames");
  members.push_back("affectedRowCount");
  members.push_back("warningCount");
  members.push_back("executionTime");
  members.push_back("lastInsertId");
  members.push_back("info");
  members.push_back("hasData");
  return members;
}

shcore::Value ClassicResult::fetch_one(const shcore::Argument_list &args)
{
  args.ensure_count(0, "ClassicResult.fetchOne");
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
  args.ensure_count(0, "ClassicResult.nextDataSet");

  return shcore::Value(_result->next_data_set());
}

shcore::Value ClassicResult::fetch_all(const shcore::Argument_list &args)
{
  args.ensure_count(0, "ClassicResult.fetchAll");

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
* Returns true if the last statement execution has a result set.
*/
Bool ClassicResult::getHasData(){};

/**
* Returns the identifier for the last record inserted.
*
* Note that this value will only be set if the executed statement inserted a record in the database and an ID was automatically generated.
*/
Integer ClassicResult::getLastInsertId(){};

/**
* Returns the number of rows affected by the executed query.
*/
Integer ClassicResult::getAffectedRowCount(){};
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

  if (prop == "info")
    return shcore::Value(_result->info());

  if (prop == "executionTime")
    return shcore::Value(MySQL_timer::format_legacy(_result->execution_time(), true));

  if (prop == "lastInsertId")
    return shcore::Value((int)_result->last_insert_id());

  if (prop == "info")
    return shcore::Value(_result->info());

  if (prop == "hasData")
    return Value(_result->has_resultset());

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
      boost::shared_ptr<shcore::Value::Map_type> map(new shcore::Value::Map_type);

      (*map)["catalog"] = shcore::Value(metadata[i].catalog());
      (*map)["db"] = shcore::Value(metadata[i].db());
      (*map)["table"] = shcore::Value(metadata[i].table());
      (*map)["org_table"] = shcore::Value(metadata[i].org_table());
      (*map)["name"] = shcore::Value(metadata[i].name());
      (*map)["org_name"] = shcore::Value(metadata[i].org_name());
      (*map)["charset"] = shcore::Value(int(metadata[i].charset()));
      (*map)["length"] = shcore::Value(int(metadata[i].length()));
      (*map)["type"] = shcore::Value(int(metadata[i].type()));
      (*map)["flags"] = shcore::Value(int(metadata[i].flags()));
      (*map)["decimal"] = shcore::Value(int(metadata[i].decimals()));
      (*map)["max_length"] = shcore::Value(int(metadata[i].max_length()));
      (*map)["name_length"] = shcore::Value(int(metadata[i].name_length()));

      // Temporal hack to identify numeric values
      (*map)["is_numeric"] = shcore::Value(IS_NUM(metadata[i].type()));

      array->push_back(shcore::Value(map));
    }

    return shcore::Value(array);
  }

  return ShellBaseResult::get_member(prop);
}