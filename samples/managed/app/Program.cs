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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using MySqlX.Shell;

namespace SimpleShellClientSharp
{
  class MySimpleClientShell : SimpleClientShell
  {
    public override void Print(string text)
    {
      // 'body' of the intrinsic 'print' function in Js mode
      Console.WriteLine(text);
    }

    public override void PrintError(string text)
    {
      // The error message may actually go to a log window, or a popup.
      Console.WriteLine("***ERROR***{0}", text);
    }

    public override bool Input(string text, ref string ret)
    {
      // 'body' of the instrinsic 'input' function in Js mode
      Console.WriteLine(text);
      ret = Console.ReadLine();
      return ret != null;
    }

    public override bool Password(string text, ref string ret)
    {
      // Intrinsic 'password' function? Ups, in a real application it should mask the input
      return Input(text, ref ret);
    }
  }

  class Program
  {

    static void Main(string[] args)
    {
      MySimpleClientShell shell = new MySimpleClientShell();
      shell.MakeConnection("root:123@localhost:33060");
      shell.SwitchMode(Mode.SQL);
      string query;
      while ((query = ReadLineFromPrompt("sql")) != null)
      {
        if (query == @"\quit") break;
        ResultSet rs = shell.Execute(query);
        TableResultSet tbl = rs as TableResultSet;
        if (tbl != null)
          PrintTableResulSet(tbl);
      }

      shell.SwitchMode(Mode.JScript);
      while ((query = ReadLineFromPrompt("js")) != null)
      {
        if (query == @"\quit") break;
        ResultSet rs = shell.Execute(query);
        DocumentResultSet doc = rs as DocumentResultSet;
        TableResultSet tbl = rs as TableResultSet;
        if (tbl != null)
          PrintTableResulSet(tbl);
        else if (doc != null)
          PrintDocumentResulSet(doc);
      }

      shell.SwitchMode(Mode.Python);
      while ((query = ReadLineFromPrompt("py")) != null)
      {
        if (query == @"\quit") break;
        ResultSet rs = shell.Execute(query);
        DocumentResultSet doc = rs as DocumentResultSet;
        if (doc != null)
          PrintDocumentResulSet(doc);
      }

      shell.Dispose();
    }

    static public string ReadLineFromPrompt(string prompt)
    {
      Console.Write("{0}> ", prompt);
      return Console.ReadLine();
    }

    static void PrintTableResulSet(TableResultSet tbl)
    {
      List<object[]> data = tbl.GetData();
      List<ResultSetMetadata> meta = tbl.GetMetadata();
      foreach(ResultSetMetadata col in meta)
      {
        Console.Write("{0}\t", col.GetName());
      }
      Console.WriteLine();
      foreach(object[] row in data)
      {
        foreach(object o in row)
        {
          string s = "";
          if (o is string)
            s = o as string;
          else if (o is int)
            s = o.ToString();
          else if (o is double)
            s = o.ToString();
          else if (o is bool)
            s = o.ToString();
          Console.Write("{0}\t", s);
        }
        Console.WriteLine();
      }
      Console.WriteLine();
    }

    static void PrintDocumentResulSet(DocumentResultSet doc)
    {
      List<Dictionary<string, object>> data = doc.GetData();
      foreach(Dictionary<string, object> row in data)
      {
        StringBuilder sb = new StringBuilder();
        sb.AppendLine("{");
        int i = 0;
        foreach(KeyValuePair<string,object> kvp in row)
        {
          sb.AppendFormat("\t\"{0}\" : \"{1}\"{2}\n", kvp.Key, kvp.Value, (i == row.Count - 1)? "" : ",");
          i++;
        }
        sb.AppendLine("},");
        Console.Write(sb);
      }
    }
  }
}
