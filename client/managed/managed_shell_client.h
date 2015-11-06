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
      Object^ Execute(String^ query);

      !ShellClient();
      ~ShellClient();

      virtual void Print(String^ text);
      virtual void PrintError(String^ text);
      virtual bool Input(String^ text, String^% ret);
      virtual bool Password(String^ text, String^% ret);
      virtual void Source(String^ module);

    private:
      bool _disposed;
      ManagedShellClient* _obj;
    };
  };
};
#endif
