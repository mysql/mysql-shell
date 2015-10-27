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

using MySqlX.Shell;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace SimpleShellClientSharp
{
  internal class MySimpleClientShell : ShellClient
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

  internal class Program
  {
    private const int READLINE_BUFFER_SIZE = 1024;

    private static void Main(string[] args)
    {
      MySimpleClientShell shell = new MySimpleClientShell();
      shell.MakeConnection("root:123@localhost:33060");
      //shell.MakeConnection("root:123@localhost:33060?ssl_ca=C:\\MySQL\\xplugin-16293675.mysql-advanced-5.7.9-winx64\\data\\ca.pem&ssl_cert=C:\\MySQL\\xplugin-16293675.mysql-advanced-5.7.9-winx64\\data\\server-cert.pem&ssl_key=C:\\MySQL\\xplugin-16293675.mysql-advanced-5.7.9-winx64\\data\\server-key.pem");
      shell.SwitchMode(Mode.SQL);

      // By default the stdin buffer is too short to allow long inputs like a connection + SSL args.
      Console.SetBufferSize(128, 1024);

      string query;
      while ((query = ReadLineFromPrompt("sql")) != null)
      {
        if (query == "\\quit\r\n") break;
        ResultSet rs = shell.Execute(query);
        TableResultSet tbl = rs as TableResultSet;
        if (tbl != null)
          PrintTableResulSet(tbl);
      }

      shell.SwitchMode(Mode.JScript);
      while ((query = ReadLineFromPrompt("js")) != null)
      {
        if (query == "\\quit\r\n") break;
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
        if (query == "\\quit\r\n") break;
        ResultSet rs = shell.Execute(query);
        DocumentResultSet doc = rs as DocumentResultSet;
        TableResultSet tbl = rs as TableResultSet;
        if (tbl != null)
          PrintTableResulSet(tbl);
        if (doc != null)
          PrintDocumentResulSet(doc);
      }

      shell.Dispose();
    }

    static public string ReadLine()
    {
      Stream inputStream = Console.OpenStandardInput(READLINE_BUFFER_SIZE);
      byte[] bytes = new byte[READLINE_BUFFER_SIZE];
      int outputLength = inputStream.Read(bytes, 0, READLINE_BUFFER_SIZE);
      //Console.WriteLine(outputLength);
      char[] chars = Encoding.UTF7.GetChars(bytes, 0, outputLength);
      return new string(chars);
    }

    static public string ReadLineFromPrompt(string prompt)
    {
      Console.Write("{0}> ", prompt);
      return ReadLine();
    }

    private static void PrintTableResulSet(TableResultSet tbl)
    {
      List<object[]> data = tbl.GetData();
      List<ResultSetMetadata> meta = tbl.GetMetadata();
      foreach (ResultSetMetadata col in meta)
      {
        Console.Write("{0}\t", col.GetName());
      }
      Console.WriteLine();
      foreach (object[] row in data)
      {
        foreach (object o in row)
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

    private static void PrintDocumentResulSet(DocumentResultSet doc)
    {
      List<Dictionary<string, object>> data = doc.GetData();
      foreach (Dictionary<string, object> row in data)
      {
        StringBuilder sb = new StringBuilder();
        sb.AppendLine("{");
        int i = 0;
        foreach (KeyValuePair<string, object> kvp in row)
        {
          sb.AppendFormat("\t\"{0}\" : \"{1}\"{2}\n", kvp.Key, kvp.Value, (i == row.Count - 1) ? "" : ",");
          i++;
        }
        sb.AppendLine("},");
        Console.Write(sb);
      }
    }
  }
}