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

#include <string>

#include <boost/shared_ptr.hpp>

#include "shellcore/ishell_core.h"
#include "managed_shell_client.h"
#include "managed_mysqlx_result.h"
#include "modules/base_resultset.h"

using namespace MySqlX::Shell;
using namespace System::Runtime::InteropServices;
using namespace System::Reflection;

[assembly:AssemblyDelaySign(false)];
[assembly:AssemblyKeyName("MySQLXShell")];
[assembly:AssemblyVersion(MYSH_VERSION)];
[assembly:AssemblyInformationalVersion(MYSH_VERSION)];

void ManagedShellClient::print(const char *text)
{
  _shell->Print(msclr::interop::marshal_as<String^>(text));
}

void ManagedShellClient::print_error(const char *text)
{
  _shell->PrintError(msclr::interop::marshal_as<String^>(text));
}

bool ManagedShellClient::input(const char *text, std::string &ret)
{
  String^ m_ret = gcnew String("");
  bool result = _shell->Input(msclr::interop::marshal_as<String^>(text), m_ret);
  ret = msclr::interop::marshal_as<std::string>(m_ret);
  delete m_ret;
  return result;
}

bool ManagedShellClient::password(const char *text, std::string &ret)
{
  String^ m_ret = gcnew String("");
  bool result = _shell->Password(msclr::interop::marshal_as<String^>(text), m_ret);
  ret = msclr::interop::marshal_as<std::string>(m_ret);
  delete m_ret;
  return result;
}

void ManagedShellClient::source(const char* module)
{
  _shell->Source(msclr::interop::marshal_as<String^>(module));
}

ShellClient::ShellClient()
{
  gcroot< ShellClient^> _managed_obj(this);
  _obj = new ManagedShellClient(_managed_obj);
}

void ShellClient::MakeConnection(String ^connstr)
{
  std::string n_connstr = msclr::interop::marshal_as<std::string>(connstr);
  _obj->make_connection(n_connstr);
}

void ShellClient::SwitchMode(Mode^ mode)
{
  Int32^ p_mode = Convert::ToInt32(mode);
  int p_mode2 = safe_cast<int>(p_mode);
  shcore::IShell_core::Mode n_mode = static_cast<shcore::IShell_core::Mode>(p_mode2);
  _obj->switch_mode(n_mode);
}

Object^ ShellClient::Execute(String^ query)
{
  Object^ ret_val;
  std::string n_query = msclr::interop::marshal_as<std::string>(query);
  shcore::Value n_result = _obj->execute(n_query);

  if (n_result.type == shcore::Object)
  {
    std::string class_name = n_result.as_object()->class_name();
    if (class_name == "Result")
      ret_val = gcnew Result(boost::static_pointer_cast<mysh::mysqlx::Result>(n_result.as_object()));
    else if (class_name == "DocResult")
      ret_val = gcnew DocResult(boost::static_pointer_cast<mysh::mysqlx::DocResult>(n_result.as_object()));
    else if (class_name == "RowResult")
      ret_val = gcnew RowResult(boost::static_pointer_cast<mysh::mysqlx::RowResult>(n_result.as_object()));
    else if (class_name == "SqlResult")
      ret_val = gcnew SqlResult(boost::static_pointer_cast<mysh::mysqlx::SqlResult>(n_result.as_object()));
    else
      ret_val = msclr::interop::marshal_as<String^>(n_result.descr(true));
  }
  else
    ret_val = msclr::interop::marshal_as<String^>(n_result.descr(true));

  return ret_val;
}

ShellClient::!ShellClient()
{
  if (!_disposed)
  {
    // the Finalize method is invoked by the garbage collector, the standard pattern is for the Finalize to validate Dispose() has not be invoked and then invoke it.
    _disposed = true;
    this->~ShellClient();
  }
}

// The C++ CLI destructor becomes the IDisposable.Dispose method when this class is exposed to managed languages (C#).
// The standard IDisposable implementation is to release here the native resources associated with this class instance (managed resource), that is as soon as possible, 
// instead of waiting for it to be called by the GC thru the Finalize method.
ShellClient::~ShellClient()
{
  if (_obj != nullptr)
    delete _obj;
  _obj = nullptr;
  _disposed = true;
}

void ShellClient::Print(String^ text)
{
}

void ShellClient::PrintError(String^ text)
{
}

bool ShellClient::Input(String^ text, String^% ret)
{
  return false;
}

bool ShellClient::Password(String^ text, String^% ret)
{
  return false;
}

void ShellClient::Source(String^ module)
{
}
