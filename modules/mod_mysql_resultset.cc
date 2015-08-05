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

Resultset::Resultset(boost::shared_ptr< ::mysql::Result> result)
: _result(result)
{
}

shcore::Value Resultset::next(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset::next");
  Row *inner_row = _result->next();

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

shcore::Value Resultset::next_result(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset::nextDataSet");

  return shcore::Value(_result->next_result());
}

shcore::Value Resultset::all(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset::all");

  boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

  shcore::Value record = next(args);

  while (record)
  {
    array->push_back(record);
    record = next(args);
  }

  return shcore::Value(array);
}

shcore::Value Resultset::get_member(const std::string &prop) const
{
  if (prop == "fetchedRowCount")
    return shcore::Value((int64_t)_result->fetched_row_count());

  if (prop == "affectedRows")
    return shcore::Value((int64_t)((_result->affected_rows() == ~(my_ulonglong)0) ? 0 : _result->affected_rows()));

  if (prop == "warningCount")
    return shcore::Value(_result->warning_count());

  if (prop == "warnings")
  {
    Result* inner_warnings = _result->query_warnings();
    boost::shared_ptr<Resultset> warnings(new Resultset(boost::shared_ptr<Result>(inner_warnings)));
    return warnings->all(shcore::Argument_list());
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

  if (prop == "columnMetadata")
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

  return BaseResultset::get_member(prop);
}