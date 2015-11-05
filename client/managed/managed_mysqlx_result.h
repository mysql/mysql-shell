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

#ifndef _MANAGED_MYSQLX_RESULT_H_
#define _MANAGED_MYSQLX_RESULT_H_

#include "msclr\marshal_cppstd.h"
#include <vcclr.h>
#include "mod_mysqlx_resultset.h"

#using <mscorlib.dll>

using namespace System;
using namespace System::Collections::Generic;

namespace MySqlX
{
  public ref class BaseResult
  {
  public:
    BaseResult(boost::shared_ptr<mysh::mysqlx::BaseResult> result);

    UInt64^ GetWarningCount() { return _warningCount; }
    List<Dictionary<String^, Object^>^>^ GetWarnings(){ return _warnings; }
    String^ GetExecutionTime(){ return _executionTime; }

  private:
    UInt64^ _warningCount;
    List<Dictionary<String^, Object^>^>^ _warnings;
    String^ _executionTime;

  protected:
    // TODO: Improve this so the data is returned interactively
    //       only when requested from the managed app
    //boost::shared_ptr<mysh::mysqlx::BaseResult> *_inner;
  };

  public ref class Result : public BaseResult
  {
  public:
    Result(boost::shared_ptr<mysh::mysqlx::Result> result);
    Int64^ GetAffectedItemCount() { return _affectedItemCount; }
    Int64^ GetLastInsertId() { return _lastInsertId; }
    String^ GetLastDocumentId() { return _lastDocumentId; }

  private:
    Int64^ _affectedItemCount;
    Int64^ _lastInsertId;
    String^ _lastDocumentId;
  };

  public ref class DocResult : public BaseResult
  {
  public:
    DocResult(boost::shared_ptr<mysh::mysqlx::DocResult> result);

    Dictionary<String^, Object^>^ FetchOne();
    List<Dictionary<String^, Object^>^>^ FetchAll();

  private:
    List<Dictionary<String^, Object^>^>^ _documents;
    Int32 _index;
  };

  public ref class Column
  {
  public:
    Column(String^ catalog, String^ db, String^ table, String^ orig_table, String^ name, String^ orig_name,
           UInt64^ collation, UInt64^ length, UInt64^ type, UInt64^ flags) : _catalog(catalog), _db(db), _table(table), _orig_table(orig_table), _name(name), _orig_name(orig_name),
           _collation(collation), _length(length), _type(type), _flags(flags){ }

    !Column() { }
    ~Column() { }

    String^ GetCatalog() { return _catalog; }
    String^ GetDb() { return _db; }
    String^ GetTable() { return _table; }
    String^ GetOrigTable() { return _orig_table; }
    String^ GetName() { return _name; }
    String^ GetOrigName() { return _orig_name; }
    UInt64^ GetCollation() { return _collation; }
    UInt64^ GetLength() { return _length; }
    UInt64^ GetType() { return _type; }
    UInt64^ GetFlags() { return _flags; }
  private:
    String^ _catalog;
    String^ _db;
    String^ _table;
    String^ _orig_table;
    String^ _name;
    String^ _orig_name;
    UInt64^ _collation;
    UInt64^ _length;
    /* This is the enum enum_field_types from mysql_com.h  */
    UInt64^ _type;
    UInt64^ _flags;
  };

  public ref class RowResult : public BaseResult
  {
  public:
    RowResult(boost::shared_ptr<mysh::mysqlx::RowResult> result);

    array<Object^>^ FetchOne();
    List<array<Object^>^>^ FetchAll();

    Int64^ GetColumnCount() { return _columnCount; }
    List<String^>^ GetColumnNames(){ return _columnNames; }
    List<Column^>^ GetColumns(){ return _columns; }

  private:
    Int64^ _columnCount;
    List<String^>^ _columnNames;
    List<Column^>^ _columns;

    List<array<Object^>^>^ _records;
    Int32 _index;
  };

  public ref class SqlResult : public RowResult
  {
  public:
    SqlResult(boost::shared_ptr<mysh::mysqlx::SqlResult> result);

    Int64^ GetAffectedRowCount(){ return _affectedRowCount; }
    Int64^ GetLastInsertId(){ return _lastInsertId; }
    Boolean^ HasData() { return _hasData; }
    // Enable once we find the way to reference the unmanaged class from the managed one
    //Boolean^ NextDataSet();

  private:
    Int64^ _affectedRowCount;
    Int64^ _lastInsertId;
    Boolean^ _hasData;
  };
};
#endif
