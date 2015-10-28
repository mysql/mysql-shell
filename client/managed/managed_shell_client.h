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

#ifndef _MANAGED_SHELL_CLIENT_H_
#define _MANAGED_SHELL_CLIENT_H_

#include "msclr\marshal_cppstd.h"
#include <vcclr.h>

#using <mscorlib.dll>

#include "shell_client.h"

using namespace System;
using namespace System::Collections::Generic;

namespace MySqlX
{
  namespace Shell
  {
    ref class ShellClient;

    class ManagedShellClient : public Shell_client
    {
    public:
      ManagedShellClient(gcroot<ShellClient^> shell /* managed reference to  ShellClient */) : Shell_client(), _shell(shell) { }
      virtual ~ManagedShellClient() { _shell = nullptr; }

    protected:
      virtual void print(const char *text);
      virtual void print_error(const char *text);
      virtual bool input(const char *text, std::string &ret);
      virtual bool password(const char *text, std::string &ret);
      virtual void source(const char* module);

    private:
      gcroot<ShellClient^> _shell;
    };

    public enum class Mode
    {
      None,
      SQL,
      JScript,
      Python
    };

    ref class ResultSet;
    ref class ResultSetMetadata;

    public ref class ShellClient
    {
    public:
      ShellClient();

      void MakeConnection(String ^connstr);
      void SwitchMode(Mode^ mode);
      ResultSet^ Execute(String^ query);

      !ShellClient();
      ~ShellClient();

      virtual void Print(String^ text);
      virtual void PrintError(String^ text);
      virtual bool Input(String^ text, String^% ret);
      virtual bool Password(String^ text, String^% ret);
      virtual void Source(String^ module);

    private:
      ManagedShellClient* _obj;
      Object^ wrap_value(const shcore::Value& val);
      List<Dictionary<String^, Object^>^>^ get_managed_doc_result(Document_result_set *doc);
      List<array<Object^>^>^ get_managed_table_result_set(Table_result_set* tbl);
      List<ResultSetMetadata^>^ get_managed_metadata(Table_result_set* tbl);
    };

    public ref class ResultSetMetadata
    {
    public:
      ResultSetMetadata(String^ catalog, String^ db, String^ table, String^ orig_table, String^ name, String^ orig_name,
        Int32^ charset, Int32^ length, Int32^ type, Int32^ flags, Int32^ decimal) : _catalog(catalog), _db(db), _table(table), _orig_table(orig_table), _name(name), _orig_name(orig_name),
        _charset(charset), _length(length), _type(type), _flags(flags), _decimal(decimal) { }

      !ResultSetMetadata() { }
      ~ResultSetMetadata() { }

      String^ GetCatalog() { return _catalog; }
      String^ GetDb() { return _db; }
      String^ GetTable() { return _table; }
      String^ GetOrigTable() { return _orig_table; }
      String^ GetName() { return _name; }
      String^ GetOrigName() { return _orig_name; }
      Int32^ GetCharset() { return _charset; }
      Int32^ GetLength() { return _length; }
      Int32^ GetType() { return _type; }
      Int32^ GetFlags() { return _flags; }
      Int32^ GetDecimal() { return _decimal; }
    private:
      String^ _catalog;
      String^ _db;
      String^ _table;
      String^ _orig_table;
      String^ _name;
      String^ _orig_name;
      Int32^ _charset;
      Int32^ _length;
      /* This is the enum enum_field_types from mysql_com.h  */
      Int32^ _type;
      Int32^ _flags;
      Int32^ _decimal;
    };

    public ref class ResultSet
    {
    public:
      ResultSet(Int64^ affected_rows, Int32^ warning_count, String^ execution_time) : _affected_rows(affected_rows), _warning_count(warning_count),
        _execution_time(execution_time)
      {}
      !ResultSet() { }
      virtual ~ResultSet() { }
      Int64^ GetAffectedRows() { return _affected_rows; }
      Int32^ GetWarningCount() { return _warning_count; }
      String^ GetExecutionTime() { return _execution_time; }
    protected:
      Int64^ _affected_rows;
      Int32^ _warning_count;
      String^ _execution_time;
    };

    public ref class TableResultSet : public ResultSet
    {
    public:
      TableResultSet(List<array<Object^>^>^ data, List<ResultSetMetadata^>^ metadata,
        Int64^ affected_rows, Int32^ warning_count, String^ execution_time) : ResultSet(affected_rows, warning_count, execution_time), _metadata(metadata), _data(data)
      {
      }
      List<ResultSetMetadata^>^ GetMetadata() { return _metadata; }
      List<array<Object^>^>^ GetData() { return _data; }
      !TableResultSet() { }
      virtual ~TableResultSet() { }
    protected:
      List<ResultSetMetadata^>^ _metadata;
      List<array<Object^>^>^ _data;
    };

    public ref class DocumentResultSet : public ResultSet
    {
    public:
      DocumentResultSet(List<Dictionary<String^, Object^>^>^ data, Int64^ affected_rows, Int32^ warning_count, String^ execution_time) :
        ResultSet(affected_rows, warning_count, execution_time), _data(data)
      {
      }
      !DocumentResultSet() { }
      virtual ~DocumentResultSet() { }
      List<Dictionary<String^, Object^>^>^ GetData() { return _data; }
    protected:
      List<Dictionary<String^, Object^>^>^ _data;
    };
  };
};
#endif
