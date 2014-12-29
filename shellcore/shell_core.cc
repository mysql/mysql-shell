/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/shell_core.h"
#include "shellcore/shell_sql.h"
#include "shellcore/shell_jscript.h"
#include "shellcore/shell_python.h"

using namespace shcore;

Shell_core::Shell_core(Interpreter_delegate *shdelegate)
: _lang_delegate(shdelegate)
{
  _mode = Mode_None;
}


Interactive_input_state Shell_core::handle_interactive_input(const std::string &line)
{
  return _langs[_mode]->handle_interactive_input(line);
}


bool Shell_core::switch_mode(Mode mode)
{
  if (_mode != mode)
  {
    _mode = mode;
    if (!_langs[_mode])
    {
      switch (_mode)
      {
      case Mode_None:
        break;
      case Mode_SQL:
        init_sql();
        break;
      case Mode_JScript:
        init_js();
        break;
      case Mode_Python:
        init_py();
        break;
      }
    }
    return true;
  }
  return false;
}


void Shell_core::init_sql()
{
  _langs[Mode_SQL] = new Shell_sql(this);
}


void Shell_core::init_js()
{
  _langs[Mode_JScript] = new Shell_javascript(this, _lang_delegate);
}


void Shell_core::init_py()
{

}

