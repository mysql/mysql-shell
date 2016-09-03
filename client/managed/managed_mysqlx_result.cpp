/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "msclr\marshal.h"
#include "msclr\marshal_cppstd.h"
#include "managed_mysqlx_result.h"

#include <string>

using namespace MySqlX;
using namespace System::Runtime::InteropServices;
using namespace System::Reflection;

[assembly:AssemblyDelaySign(false)];
[assembly:AssemblyKeyName("MySQLShell")];

// AssemblyVersion does not allow slashes. Example: 1.0.5-labs
#ifdef _WIN32
[assembly:AssemblyVersion(MYSH_VERSION_WIN)];
#else
[assembly:AssemblyVersion(MYSH_VERSION)];
#endif

[assembly:AssemblyInformationalVersion(MYSH_VERSION)];

Object^ wrap_value(const shcore::Value& val)
{
  Object^ o;
  Dictionary<String^, Object^>^ document;
  std::string class_name;
  switch (val.type)
  {
    case shcore::Object:
      class_name = val.as_object()->class_name();
      if (class_name == "Result")
        o = gcnew Result(std::static_pointer_cast<mysh::mysqlx::Result>(val.as_object()));
      else if (class_name == "DocResult")
        o = gcnew DocResult(std::static_pointer_cast<mysh::mysqlx::DocResult>(val.as_object()));
      else if (class_name == "RowResult")
        o = gcnew DocResult(std::static_pointer_cast<mysh::mysqlx::DocResult>(val.as_object()));
      else if (class_name == "SqlResult")
        o = gcnew DocResult(std::static_pointer_cast<mysh::mysqlx::DocResult>(val.as_object()));
      break;
    case shcore::Integer:
      o = gcnew Int32(val.as_int());
      break;
    case shcore::String:
      o = msclr::interop::marshal_as<String^>(val.as_string());
      break;
    case shcore::Bool:
      o = gcnew Boolean(val.as_bool());
      break;
    case shcore::Float:
      o = gcnew Double(val.as_double());
      break;
    case shcore::Map:
    {
      shcore::Value::Map_type_ref map = val.as_map();
      shcore::Value::Map_type::const_iterator index, end = map->end();

      Dictionary<String^, Object^>^ document = gcnew Dictionary<String^, Object^>();
      for (index = map->begin(); index != end; index++)
        document->Add(msclr::interop::marshal_as<String^>(index->first.c_str()), wrap_value(index->second));

      o = document;
    }
    break;
    case shcore::Array:
    {
      shcore::Value::Array_type_ref array = val.as_array();
      shcore::Value::Array_type::const_iterator index, end = array->end();

      List<Object^>^ list = gcnew List<Object^>();
      for (index = array->begin(); index != end; index++)
        list->Add(wrap_value(*index));

      o = list;
    }
    break;
    default:
      o = msclr::interop::marshal_as<String^>(val.descr());
  }
  return o;
}

BaseResult::BaseResult(std::shared_ptr<mysh::mysqlx::BaseResult> result)
{
  _warningCount = gcnew UInt64(result->get_warning_count());
  _executionTime = msclr::interop::marshal_as<String^>(result->get_execution_time());

  // Fills the warnings
  _warnings = gcnew List<Dictionary<String^, Object^>^>();

  shcore::Value::Array_type_ref warnings = result->get_member("warnings").as_array();

  for (size_t index = 0; index < warnings->size(); index++)
  {
    std::shared_ptr<mysh::Row> row = std::static_pointer_cast<mysh::Row>(warnings->at(index).as_object());

    Dictionary<String^, Object^>^ warning = gcnew Dictionary<String^, Object^>();

    warning->Add(gcnew String("Level"), wrap_value(row->get_member("Level")));
    warning->Add(gcnew String("Code"), wrap_value(row->get_member("Code")));
    warning->Add(gcnew String("Message"), wrap_value(row->get_member("Message")));

    _warnings->Add(warning);
  }
}

Result::Result(std::shared_ptr<mysh::mysqlx::Result> result) :
BaseResult(result)
{
  _affectedItemCount = gcnew Int64(result->get_affected_item_count());
  _lastInsertId = gcnew Int64(result->get_auto_increment_value());
  _lastDocumentId = msclr::interop::marshal_as<String^>(result->get_last_document_id());
}

DocResult::DocResult(std::shared_ptr<mysh::mysqlx::DocResult> result) :
BaseResult(result)
{
  shcore::Argument_list args;
  shcore::Value raw_doc;

  _documents = gcnew List<Dictionary<String^, Object^>^>();
  while (raw_doc = result->fetch_one(args))
  {
    Object^ obj = wrap_value(raw_doc);
    Dictionary<String^, Object^>^ document = (Dictionary<String^, Object^>^)obj;

    _documents->Add(document);
  }
}

Dictionary<String^, Object^>^ DocResult::FetchOne()
{
  Dictionary<String^, Object^>^ ret_val;

  if (_index < _documents->Count)
    ret_val = _documents[_index++];

  return ret_val;
}

List<Dictionary<String^, Object^>^>^ DocResult::FetchAll()
{
  return _documents;
}

RowResult::RowResult(std::shared_ptr<mysh::mysqlx::RowResult> result) :
BaseResult(result)
{
  _columnCount = gcnew Int64(result->get_column_count());

  shcore::Value::Array_type_ref columns = result->get_columns();

  _columns = gcnew List<Column^>();
  _columnNames = gcnew List<String^>();

  for (size_t index = 0; index < columns->size(); index++)
  {
    std::shared_ptr<mysh::Column> column = std::static_pointer_cast<mysh::Column>(columns->at(index).as_object());

    _columnNames->Add(msclr::interop::marshal_as<String^>(column->get_column_label()));

    _columns->Add(gcnew Column(msclr::interop::marshal_as<String^>(column->get_schema_name()),
                               msclr::interop::marshal_as<String^>(column->get_table_label()),
                               msclr::interop::marshal_as<String^>(column->get_table_name()),
                               msclr::interop::marshal_as<String^>(column->get_column_label()),
                               msclr::interop::marshal_as<String^>(column->get_column_name()),
                               gcnew UInt64(column->get_length()),
                               msclr::interop::marshal_as<String^>(column->get_type().descr()),
                               gcnew UInt64(column->get_fractional_digits()),
                               gcnew Boolean(column->is_number_signed()),
                               msclr::interop::marshal_as<String^>(column->get_collation_name()),
                               msclr::interop::marshal_as<String^>(column->get_character_set_name()),
                               gcnew Boolean(column->is_padded())));
  }

  _records = gcnew List<array<Object^>^>();

  shcore::Value raw_record;
  shcore::Argument_list args;
  while (raw_record = result->fetch_one(args))
  {
    std::shared_ptr<mysh::Row> row = std::static_pointer_cast<mysh::Row>(raw_record.as_object());

    if (row)
    {
      array<Object^>^ fields;

      size_t length = row->get_length();

      fields = gcnew array<Object^>(length);

      for (size_t index = 0; index < length; index++)
        fields->SetValue(wrap_value(row->get_member(index)), int(index));

      _records->Add(fields);
    }
  }
}

array<Object^>^ RowResult::FetchOne()
{
  array<Object^>^ ret_val;

  if (_index < _records->Count)
    ret_val = _records[_index++];

  return ret_val;
}

List<array<Object^>^>^ RowResult::FetchAll()
{
  return _records;
}

SqlResult::SqlResult(std::shared_ptr<mysh::mysqlx::SqlResult> result) :
RowResult(result)
{
  _affectedRowCount = gcnew Int64(result->get_affected_row_count());
  _lastInsertId = gcnew Int64(result->get_auto_increment_value());
  _hasData = gcnew Boolean(result->has_data(shcore::Argument_list()));
}

//Boolean^ SqlResult::NextDataSet()
//{
//  std::shared_ptr<mysh::mysqlx::SqlResult> result = std::static_pointer_cast<mysh::mysqlx::SqlResult>(_inner);
//
//  return gcnew Boolean(result->next_data_set());
//}
