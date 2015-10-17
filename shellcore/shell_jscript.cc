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

void Shell_javascript::handle_input(std::string &code, Interactive_input_state &state, boost::function<void(shcore::Value)> result_processor)
{
  // Undefined to be returned in case of errors
  Value result;

  state = Input_ok;
  if ((*Shell_core_options::get())[SHCORE_INTERACTIVE].as_bool())
  {
    bool continued;
    result = _js->execute_interactive(code, continued);
    if (continued)
      state = Input_continued;
  }
  else
  {
    try
    {
      result = _js->execute(code, _owner->get_input_source());
    }
    catch (std::exception &exc)
    {
      _owner->print_error(exc.what());
    }
  }

  _last_handled = code;

  result_processor(result);
}

std::string Shell_javascript::prompt()
{
  try
  {
    shcore::Value value = _js->execute("shell.ps ? shell.ps() : null", "shell.ps");
    if (value && value.type == String)
      return value.as_string();
  }
  catch (std::exception &exc)
  {
    _owner->print_error(std::string("Exception in JS ps function: ") + exc.what());
  }
  return "mysql-js> ";
}

void Shell_javascript::set_global(const std::string &name, const Value &value)
{
  _js->set_global(name, value);
}