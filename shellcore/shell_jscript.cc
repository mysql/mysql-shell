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

#include "shellcore/shell_jscript.h"
#include "shellcore/jscript_context.h"

using namespace shcore;


Shell_javascript::Shell_javascript(Shell_core *shcore)
: Shell_language(shcore)
{
  _js = boost::shared_ptr<JScript_context>(new JScript_context(shcore->registry(), shcore->lang_delegate()));
}


Interactive_input_state Shell_javascript::handle_interactive_input(std::string &code)
{
  Value result = _js->execute_interactive(code);

  // last result
  _owner->set_global("_LR", result);

  _last_handled = code;

  return Input_ok;
}


std::string Shell_javascript::prompt()
{
  return "js> ";
}


int Shell_javascript::run_script(const std::string &path, boost::system::error_code &err)
{
  return _js->run_script(path, err);
}


void Shell_javascript::set_global(const std::string &name, const Value &value)
{
  _js->set_global(name, value);
}

