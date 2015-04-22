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

#include "shellcore/shell_python.h"
#include "shellcore/python_context.h"

#include <Python.h>

using namespace shcore;

void Python_context_init() {
  Py_InitializeEx(0);
}

void Python_context_deinit() {
  Py_Finalize();
}

Shell_python::Shell_python(Shell_core *shcore)
: Shell_language(shcore)
{
  _py = boost::shared_ptr<Python_context>(new Python_context());
}

Value Shell_python::handle_input(std::string &code, Interactive_input_state &state, bool interactive)
{
  Value result;

  result = _py->execute(code);

  _last_handled = code;

  state = Input_ok;

  return result;
}

std::string Shell_python::prompt()
{
  return "mysql-py> ";
}

void Shell_python::set_global(const std::string &name, const Value &value)
{
  _py->set_global(name, value);
}
