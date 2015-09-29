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
#include <boost/bind.hpp>
#include "shellcore/shell_core_options.h"

using namespace mysh;
using namespace shcore;
using namespace mysh::mysqlx;

Resultset::Resultset(boost::shared_ptr< ::mysqlx::Result> result)
: _result(result)
{
  add_method("buffer", boost::bind(&Resultset::buffer, this, _1), NULL);
  add_method("flush", boost::bind(&Resultset::flush, this, _1), NULL);
  add_method("rewind", boost::bind(&Resultset::rewind, this, _1), NULL);
}

Resultset::~Resultset(){}

#ifdef DOXYGEN
/**
* Returns the metadata for the result set.
* \return the metadata Map[]
* The metadata returned its an array of map each one per field the map accepts the following data:
*
* Map key     | Meaning                        |
* ----------: | :----------------------------: |
* catalog     | the catalog name               |
* table       | the table name                 |
* org_table   | original table name            |
* name        | the column name                |
* org_name    | the original column name       |
* collation   | collation (from charset)       |
* length      | the column length              |
* type        | the column type                |
* flags       | flags (to be documented)       |
* decimal     | decimal precision              |
* max_length  | max length allowed             |
* name_length | length of column name          |
* is_numeric  | bool, true if type is numeric  |
*/
Schema Resultset::getColumnMetadata(){};

/**
* The last insert id auto generated (from an insert operation)
* \return the integer representing the last insert id
* For more details, see https://dev.mysql.com/doc/refman/5.7/en/information-functions.html#function_last-insert-id
* \sa getAffectedRows(), getWarnings(), getExcutionTime(), getInfo()
*/
Integer Resultset::getLastInsertId(){};

/**
* The the number of affected rows for the last operation.
* \return the number of affected rows.
* This is the value of the C API mysql_affected_rows(), see https://dev.mysql.com/doc/refman/5.7/en/mysql-affected-rows.html
* \sa getLastInsertId(), getWarnings(), getExcutionTime(), getInfo()
*/
Integer Resultset::getAffectedRows(){};

/**
* The number of warnings produced by the last statement execution. See getWarnings() for more details.
* \return the number of warnings.
* This is the same value than C API mysql_warning_count, see https://dev.mysql.com/doc/refman/5.7/en/mysql-warning-count.html
* \sa warnings
*/
Integer Resultset::getWarnings(){};

/**
* Gets an string with information on the last statement executed. See for details getInfo().
* This is the same value then C API mysql_info, see https://dev.mysql.com/doc/refman/5.7/en/mysql-info.html
* \sa getInfo().
*/
String Resultset::getInfo(){};

/**
* Gets a string representation of the time spent time by the last statement executed.
* \return the execution time as an String.
* \sa executionTime()
*/
String Resultset::getExecutionTime(){};

/**
* Returns true if the last statement execution has a result set. As opposite as having only the basic info of affected rows, info, exectution time, warnings, last insert id.
* \sa getLastInsertId(), getInfo(), getExecutionTime(), getWarnings(), getAffectedRows().
* \sa getHasData()
*/
Bool Resultset::getHasData(){};

/**
* Returns the next result set.
* \return true if more data available, otherwise false.
* Before returning the next result set, this operation takes care of flushing an ongoing result set stream reading.
* After reading (and discarding) all the packages of the current result set stream, it will attempt to read the metadata of the next result set or the OK msg.
*/
Bool Resultset::nextDataSet(){};

Row Resultset::next(){};
List Resultset::all(){};
Resultset Resultset::buffer(){};
Undefined Resultset::flush(){};
Undefined Resultset::rewind(){};
#endif

shcore::Value Resultset::get_member(const std::string &prop) const
{
  Value ret_val;
  if (prop == "fetchedRowCount")
    ret_val = Value(0);
  else if (prop == "affectedRows")
    ret_val = Value(_result->affectedRows());
  else if (prop == "warningCount")
    ret_val = Value(uint64_t(_result->getWarnings().size()));
  else if (prop == "executionTime")
    ret_val = Value("0"); // TODO: Execution time not being provided on X Protocol
  else if (prop == "lastInsertId")
    ret_val = Value(_result->lastInsertId());
  else if (prop == "info")
    ret_val = Value(""); // TODO: Info not being provided on X Protocol
  else if (prop == "hasData")
    ret_val = Value(_result->columnMetadata() && (_result->columnMetadata()->size() > 0));
  else if (prop == "columnMetadata")
  {
    boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

    if (!_result->columnMetadata()) return ret_val = shcore::Value(array);

    size_t num_fields = _result->columnMetadata()->size();

    for (size_t i = 0; i < num_fields; i++)
    {
      boost::shared_ptr<shcore::Value::Map_type> map(new shcore::Value::Map_type);

      (*map)["catalog"] = shcore::Value(_result->columnMetadata()->at(i).catalog);
      //(*map)["db"] = shcore::Value(_result->columnMetadata()->at(i).db);
      (*map)["table"] = shcore::Value(_result->columnMetadata()->at(i).table);
      (*map)["org_table"] = shcore::Value(_result->columnMetadata()->at(i).original_table);
      (*map)["name"] = shcore::Value(_result->columnMetadata()->at(i).name);
      (*map)["org_name"] = shcore::Value(_result->columnMetadata()->at(i).original_name);
      (*map)["collation"] = shcore::Value(_result->columnMetadata()->at(i).collation);
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
    }

    ret_val = shcore::Value(array);
  }
  else if (prop == "warnings")
  {
    boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

    std::vector< ::mysqlx::Result::Warning> warnings = _result->getWarnings();

    if (warnings.size())
    {
      for (size_t index = 0; index < warnings.size(); index++)
      {
        mysh::Row *warning_row = new mysh::Row();

        warning_row->add_item("Level", shcore::Value(warnings[index].is_note ? "Note" : "Warning"));
        warning_row->add_item("Code", shcore::Value(warnings[index].code));
        warning_row->add_item("Message", shcore::Value(warnings[index].text));

        array->push_back(shcore::Value::wrap(warning_row));
      }
    }

    ret_val = shcore::Value(array);
  }
  else
    ret_val = BaseResultset::get_member(prop);

  return ret_val;
}

#ifdef DOXYGEN
/**
* Retrieves the next record on the resultset.
* \return A Row object representing the next record.
*/
Row Resultset::next(){};
#endif
shcore::Value Resultset::next(const shcore::Argument_list &args)
{
  std::string function = class_name() + ".next";

  args.ensure_count(0, function.c_str());

  boost::shared_ptr<std::vector< ::mysqlx::ColumnMetadata> > metadata = _result->columnMetadata();
  if (metadata->size() > 0)
  {
    boost::shared_ptr< ::mysqlx::Row>row = _result->next();
    if (row)
    {
      mysh::Row *value_row = new mysh::Row();

      for (int index = 0; index < int(metadata->size()); index++)
      {
        Value field_value;

        if (row->isNullField(index))
          field_value = Value::Null();
        else
        {
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
            case ::mysqlx::DECIMAL:
              field_value = Value(row->decimalField(index));
              break;
            case ::mysqlx::TIME:
              field_value = Value(row->timeField(index));
              break;
            case ::mysqlx::DATETIME:
              field_value = Value(row->dateTimeField(index));
              break;
            case ::mysqlx::ENUM:
              field_value = Value(row->enumField(index));
              break;
            case ::mysqlx::BIT:
              field_value = Value(row->bitField(index));
              break;
              //TODO: Fix the handling of SET
            case ::mysqlx::SET:
              //field_value = Value(row->setField(int(index)));
              break;
          }
        }
        value_row->add_item(metadata->at(index).name, field_value);
      }

      return shcore::Value::wrap(value_row);
    }
  }
  return shcore::Value();
}

#ifdef DOXYGEN
/**
* Returns a list of Row objects which contains an element for every unread row on the Resultset.
* \return A List of Row objects.
*/
List Resultset::all(){};
#endif
shcore::Value Resultset::all(const shcore::Argument_list &args)
{
  Value::Array_type_ref array(new Value::Array_type());

  std::string function = class_name() + ".all";

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

#ifdef DOXYGEN
/**
* Prepares the Resultset to start reading data from the next Result (if many results were returned).
* \return A boolean value indicating whether there is another result or not.
*/
Bool Resultset::nextDataSet(){};
#endif
shcore::Value Resultset::next_result(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset.nextDataSet");

  return shcore::Value(_result->nextDataSet());
}

#ifdef DOXYGEN
/**
* Loads all the remaining records and results for this resultset.
* \return This resultset object.
*
* Only the remaining records will be loaded into an internal cache.
*/
Resultset Resultset::buffer(){};
#endif
shcore::Value Resultset::buffer(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset.buffer");

  _result->buffer();

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Discards all the remaining records and results for this resultset.
*/
Undefined Resultset::flush(){};
#endif
shcore::Value Resultset::flush(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset.flush");

  _result->flush();

  return shcore::Value();
}

#ifdef DOXYGEN
/**
* Moves the internal read index so the next call to next() returns the first cached record of the current Result.
*/
Undefined Resultset::rewind(){};
#endif
shcore::Value Resultset::rewind(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset.rewind");

  _result->rewind();

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

CollectionResultset::CollectionResultset(boost::shared_ptr< ::mysqlx::Result> result)
: Resultset(result)
{
}

#ifdef DOXYGEN
/**
* Retrieves the next record on the resultset.
* \return A Row object representing the next record.
*/
Document CollectionResultset::next(){};
#endif
shcore::Value CollectionResultset::next(const shcore::Argument_list &args)
{
  Value ret_val = Value::Null();

  std::string function = class_name() + ".next";

  args.ensure_count(0, function.c_str());

  if (_result->columnMetadata() && _result->columnMetadata()->size())
  {
    boost::shared_ptr< ::mysqlx::Row> r(_result->next());
    if (r.get())
      return Value::parse(r->stringField(0));
  }
  return shcore::Value();
}