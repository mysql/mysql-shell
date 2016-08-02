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
    BaseResult(std::shared_ptr<mysh::mysqlx::BaseResult> result);

    UInt64 GetWarningCount() { return (UInt64)_warningCount; }
    List<Dictionary<String^, Object^>^>^ GetWarnings(){ return _warnings; }
    String^ GetExecutionTime(){ return _executionTime; }

  private:
    UInt64^ _warningCount;
    List<Dictionary<String^, Object^>^>^ _warnings;
    String^ _executionTime;

  protected:
    // TODO: Improve this so the data is returned interactively
    //       only when requested from the managed app
    //std::shared_ptr<mysh::mysqlx::BaseResult> _inner;
  };

  public ref class Result : public BaseResult
  {
  public:
    Result(std::shared_ptr<mysh::mysqlx::Result> result);
    Int64 GetAffectedItemCount() { return (Int64)_affectedItemCount; }
    Int64 GetLastInsertId() { return (Int64)_lastInsertId; }
    String^ GetLastDocumentId() { return _lastDocumentId; }

  private:
    Int64^ _affectedItemCount;
    Int64^ _lastInsertId;
    String^ _lastDocumentId;
  };

  public ref class DocResult : public BaseResult
  {
  public:
    DocResult(std::shared_ptr<mysh::mysqlx::DocResult> result);

    Dictionary<String^, Object^>^ FetchOne();
    List<Dictionary<String^, Object^>^>^ FetchAll();

  private:
    List<Dictionary<String^, Object^>^>^ _documents;
    Int32 _index;
  };

  public ref class Column
  {
  public:
    Column(String^ db, String^ table, String^ orig_table, String^ name, String^ orig_name,
           UInt64^ length, String^ type, UInt64^ fractional, Boolean^ is_signed, String^ collation, String^ charset, Boolean^ padded) :
           _db(db), _table(table), _orig_table(orig_table), _name(name), _orig_name(orig_name),
           _length(length), _type(type), _fractional_digits(fractional), _signed(is_signed), _collation(collation),
           _charset(charset), _padded(padded){ }

    !Column() { }
    ~Column() { }

    String^ GetSchemaName() { return _db; }
    String^ GetTableLabel() { return _table; }
    String^ GetTableName() { return _orig_table; }
    String^ GetColumnLabel() { return _name; }
    String^ GetColumnName() { return _orig_name; }
    UInt64^ GetLength() { return _length; }
    String^ GetType() { return _type; }
    UInt64^ GetFractionalDigits() { return _fractional_digits; }
    Boolean^ IsNumberSigned() { return _signed; }
    String ^ GetCollationName() { return _collation; }
    String ^ GetCharsetName() { return _charset; }
    Boolean^ IsPadded() { return _padded; }
  private:
    String^ _db;
    String^ _table;
    String^ _orig_table;
    String^ _name;
    String^ _orig_name;
    UInt64^ _length;
    /* This is a string in the form of <Group.Constant>  */
    /* Available values include Type.XXX as defined in: */
    /* http://home.no.oracle.com/~jkneschk/devapi/internal/specifications/MY-130-spec.html */
    String^ _type;
    UInt64^ _fractional_digits;
    Boolean^ _signed;
    String^ _collation;
    String^ _charset;
    Boolean^ _padded;
  };

  public ref class RowResult : public BaseResult
  {
  public:
    RowResult(std::shared_ptr<mysh::mysqlx::RowResult> result);

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
    SqlResult(std::shared_ptr<mysh::mysqlx::SqlResult> result);

    Int64 GetAffectedRowCount(){ return (Int64)_affectedRowCount; }
    Int64 GetLastInsertId(){ return (Int64)_lastInsertId; }
    Boolean HasData() { return (Boolean)_hasData; }
    // Enable once we find the way to reference the unmanaged class from the managed one
    //Boolean NextDataSet();

  private:
    Int64^ _affectedRowCount;
    Int64^ _lastInsertId;
    Boolean^ _hasData;
  };
};
#endif
