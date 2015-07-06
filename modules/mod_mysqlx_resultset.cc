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

#include "mod_mysqlx_resultset.h"
#include "mysqlx.h"
#include "shellcore/common.h"

using namespace mysh;
using namespace shcore;
using namespace mysh::mysqlx;

Resultset::Resultset(boost::shared_ptr< ::mysqlx::Result> result)
: _result(result)
{
}

Resultset::~Resultset(){}

shcore::Value Resultset::get_member(const std::string &prop) const
{
  Value ret_val;
  if (prop == "fetchedRowCount")
    ret_val = Value(0);
  else if (prop == "affectedRows")
    ret_val = Value(_result->affectedRows());
  else if (prop == "warningCount")
    ret_val = Value(0); // TODO: Warning count not being received on X Protocol
  else if (prop == "executionTime")
    ret_val = Value("0"); // TODO: Execution time not being provided on X Protocol
  else if (prop == "lastInsertId")
    ret_val = Value(_result->lastInsertId());
  else if (prop == "info")
    ret_val = Value(""); // TODO: Info not being provided on X Protocol
  else if (prop == "hasData")
    ret_val = Value(_result->columnMetadata()->size() > 0);
  else if (prop == "columnMetadata")
  {
    boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

    int num_fields = _result->columnMetadata()->size();

    for (int i = 0; i < num_fields; i++)
    {
      boost::shared_ptr<shcore::Value::Map_type> map(new shcore::Value::Map_type);

      (*map)["catalog"] = shcore::Value(_result->columnMetadata()->at(i).catalog);
      //(*map)["db"] = shcore::Value(_result->columnMetadata()->at(i).db);
      (*map)["table"] = shcore::Value(_result->columnMetadata()->at(i).table);
      (*map)["org_table"] = shcore::Value(_result->columnMetadata()->at(i).original_table);
      (*map)["name"] = shcore::Value(_result->columnMetadata()->at(i).name);
      (*map)["org_name"] = shcore::Value(_result->columnMetadata()->at(i).original_name);
      (*map)["charset"] = shcore::Value(_result->columnMetadata()->at(i).charset);
      (*map)["length"] = shcore::Value(int(_result->columnMetadata()->at(i).length));
      (*map)["type"] = shcore::Value(int(_result->columnMetadata()->at(i).type)); // TODO: Translate to MySQL Type
      (*map)["flags"] = shcore::Value(int(_result->columnMetadata()->at(i).flags));
      (*map)["decimal"] = shcore::Value(int(_result->columnMetadata()->at(i).fractional_digits)); // TODO: decimals??
      (*map)["max_length"] = shcore::Value(0);
      (*map)["name_length"] = shcore::Value(int(_result->columnMetadata()->at(i).name.length()));

      // Temporal hack to identify numeric values
      ::mysqlx::FieldType type = _result->columnMetadata()->at(i).type;
      (*map)["is_numeric"] = shcore::Value(type == ::mysqlx::SINT ||
                                           type == ::mysqlx::UINT ||
                                           type == ::mysqlx::DOUBLE ||
                                           type == ::mysqlx::FLOAT ||
                                           type == ::mysqlx::DECIMAL);

      array->push_back(shcore::Value(map));

      return shcore::Value(array);
    }
  }
  else
    ret_val = BaseResultset::get_member(prop);

  return ret_val;
}

shcore::Value Resultset::next(const shcore::Argument_list &UNUSED(args))
{
  boost::shared_ptr<std::vector< ::mysqlx::ColumnMetadata> > metadata = _result->columnMetadata();
  if (metadata->size() > 0)
  {
    ::mysqlx::Row *row = _result->next();
    if (row)
    {
      mysh::Row *value_row = new mysh::Row();

      for (size_t index = 0; index < metadata->size(); index++)
      {
        Value field_value;
        switch (metadata->at(index).type)
        {
          case ::mysqlx::SINT:
            field_value = Value(row->sInt64Field(index));
            break;
          case ::mysqlx::UINT:
            field_value = Value(row->uInt64Field(index));
            break;
          case ::mysqlx::DOUBLE:
            field_value = Value(row->doubleField(index));
            break;
          case ::mysqlx::FLOAT:
            field_value = Value(row->floatField(index));
            break;
          case ::mysqlx::BYTES:
            field_value = Value(row->stringField(index));
            break;
          case ::mysqlx::TIME:
          case ::mysqlx::DATETIME:
          case ::mysqlx::SET:
          case ::mysqlx::ENUM:
          case ::mysqlx::BIT:
          case ::mysqlx::DECIMAL:
            //XXX TODO
            break;
        }
        value_row->add_item(metadata->at(index).name, field_value);
      }

      return shcore::Value::wrap(value_row);
    }
  }
  return shcore::Value();
}

shcore::Value Resultset::all(const shcore::Argument_list &args)
{
  Value::Array_type_ref array(new Value::Array_type());

  std::string function = class_name() + "::all";

  args.ensure_count(0, function.c_str());

  // Gets the next row
  Value record = next(args);
  while (record)
  {
    array->push_back(record);
    record = next(args);
  }

  return Value(array);
}

shcore::Value Resultset::next_result(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset::nextResult");

  return shcore::Value(_result->nextResult());
}

Collection_resultset::Collection_resultset(boost::shared_ptr< ::mysqlx::Result> result)
: Resultset(result)
{
}

shcore::Value Collection_resultset::next(const shcore::Argument_list &args)
{
  Value ret_val = Value::Null();

  std::string function = class_name() + "::next";

  args.ensure_count(0, function.c_str());

  if (_result->columnMetadata()->size())
  {
    std::auto_ptr< ::mysqlx::Row> r(_result->next());
    if (r.get())
      return Value::parse(r->stringField(0));
  }
  return shcore::Value();
}