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

Shell_core::Shell_core(Mode default_mode)
{
  _mode = default_mode;
  switch (_mode)
  {
  case Mode_SQL:
    _langs[_mode] = new Shell_sql(this);
    break;
  case Mode_JScript:
    _langs[_mode] = new Shell_javascript(this);
    break;
  }
}


Interactive_input_state Shell_core::handle_interactive_input_line(std::string &line)
{
  return _langs[_mode]->handle_interactive_input_line(line);
}


bool Shell_core::switch_mode(Mode mode)
{
  if (_mode != mode)
  {
    _mode = mode;
    return true;
  }
  return false;
}
