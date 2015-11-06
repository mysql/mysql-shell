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

using MySqlX;
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

      // By default the stdin buffer is too short to allow long inputs like a connection + SSL args.
      Console.SetBufferSize(128, 1024);

      List<Mode> modes = new List<Mode>();
      List<string> prompts = new List<string>();

      modes.Add(Mode.SQL);
      modes.Add(Mode.JScript);
      modes.Add(Mode.Python);

      prompts.Add("sql");
      prompts.Add("js");
      prompts.Add("py");

      int index = 0;

      string query;
      while (index < 3)
      {
        shell.SwitchMode(modes[index]);
        while ((query = ReadLineFromPrompt(prompts[index])) != null)
        {
          if (query == "\\quit\r\n")
          {
            index++;
            break;
          }

          Object rs = shell.Execute(query);

          printResult(rs);
        }
      }

      shell.Dispose();
    }

    static public void printResult(Object result)
    {
      Result res = result as Result;
      DocResult doc = result as DocResult;
      RowResult row = result as RowResult;
      SqlResult sql = result as SqlResult;
      if (res != null)
        PrintResult(res);
      else if (doc != null)
        PrintDocResult(doc);
      else if (row != null)
        PrintRowResult(row);
      else if (sql != null)
        PrintSqlResult(sql);
      else if (result == null)
        Console.Write("null");
      else
        Console.Write(result.ToString());
    }

    static public string ReadLine()
    {
      Stream inputStream = Console.OpenStandardInput(READLINE_BUFFER_SIZE);
      byte[] bytes = new byte[READLINE_BUFFER_SIZE];
      int outputLength = inputStream.Read(bytes, 0, READLINE_BUFFER_SIZE);
      char[] chars = Encoding.UTF7.GetChars(bytes, 0, outputLength);
      return new string(chars);
    }

    static public string ReadLineFromPrompt(string prompt)
    {
      Console.Write("{0}> ", prompt);
      return ReadLine();
    }

    private static void PrintBaseResult(BaseResult res)
    {
      Console.Write("Execution Time: {0}\n", res.GetExecutionTime());
      Console.Write("Warning Count: {0}\n", res.GetWarningCount());

      if (Convert.ToUInt64(res.GetWarningCount()) > 0)
      {
        List<Dictionary<String, Object>> warnings = res.GetWarnings();

        foreach (Dictionary<String, Object> warning in warnings)
          Console.Write("{0} ({1}): (2)\n", warning["Level"], warning["Code"], warning["Message"]);
      }
    }

    private static void PrintResult(Result res)
    {
      Console.Write("Affected Items: {0}\n", res.GetAffectedItemCount());
      Console.Write("Last Insert Id: {0}\n", res.GetLastInsertId());
      Console.Write("Last Document Id: {0}\n", res.GetLastDocumentId());

      PrintBaseResult(res);
    }

    private static void PrintRowResult(RowResult res)
    {
      foreach (Column col in res.GetColumns())
      {
        Console.Write("{0}\t", col.GetName());
      }
      Console.WriteLine();

      object[] record = res.FetchOne();

      while (record != null)
      {
        foreach (object o in record)
        {
          string s = "";
          if (o == null)
            s = "null";
          else
            s = o.ToString();

          Console.Write("{0}\t", s);
        }
        Console.WriteLine();

        record = res.FetchOne();
      }
    }

    private static void PrintDocResult(DocResult doc)
    {
      List<Dictionary<string, object>> data = doc.FetchAll();
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

    private static void PrintSqlResult(SqlResult res)
    {
      if ((bool)res.HasData())
        PrintRowResult(res);
      else
        PrintBaseResult(res);

      Console.Write("Affected Rows: {0}\n", res.GetAffectedRowCount());
      Console.Write("Last Insert Id: {0}\n", res.GetLastInsertId());
    }
  }
}