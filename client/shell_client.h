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

#ifndef SHELL_CLIENT_H_
#define SHELL_CLIENT_H_

#ifdef _WIN32
# if _DLL
#  ifdef SHELL_CLIENT_NATIVE_EXPORTS
#   define SHELL_CLIENT_NATIVE_PUBLIC __declspec(dllexport)
#  else
#   define SHELL_CLIENT_NATIVE_PUBLIC __declspec(dllimport)
#  endif
# else
#  define SHELL_CLIENT_NATIVE_PUBLIC
# endif
#else
# define SHELL_CLIENT_NATIVE_PUBLIC
#endif

#include <string>
#include <vector>
#include <stdexcept>
#include <map>

#include "shellcore/types.h"
#include "shellcore/shell_core.h"
#include "modules/base_session.h"
#include "shellcore/lang_base.h"

class SHELL_CLIENT_NATIVE_PUBLIC Shell_client
{
public:
  Shell_client();
  virtual ~Shell_client();
  // Makes a connection, throws an std::runtime_error in case of error.
  void make_connection(const std::string& connstr);
  void switch_mode(shcore::Shell_core::Mode mode);
  shcore::Value execute(const std::string query);
protected:
  virtual void print(const char *text);
  virtual void print_error(const char *text);
  virtual bool input(const char *text, std::string &ret);
  virtual bool password(const char *text, std::string &ret);
  virtual void source(const char* module);
private:
  shcore::Interpreter_delegate _delegate;
  std::shared_ptr<shcore::Shell_core> _shell;
  shcore::Value _last_result;

  static void deleg_print(void *self, const char *text);
  static void deleg_print_error(void *self, const char *text);
  static bool deleg_input(void *self, const char *text, std::string &ret);
  static bool deleg_password(void *self, const char *text, std::string &ret);
  static void deleg_source(void *self, const char *module);

  shcore::Value connect_session(const shcore::Argument_list &args);
  shcore::Value process_line(const std::string &line);
  void process_result(shcore::Value result);
  bool do_shell_command(const std::string &line);
  bool connect(const std::string &uri);
};

#endif
