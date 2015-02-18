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
#include "shellcore/object_registry.h"
#include <boost/algorithm/string.hpp>

#include "shellcore/lang_base.h"

using namespace shcore;

Shell_core::Shell_core(Interpreter_delegate *shdelegate)
: _lang_delegate(shdelegate)
{
  _mode = Mode_None;
  _registry = new Object_registry();
}


Shell_core::~Shell_core()
{
  delete _registry;
}


void Shell_core::print(const std::string &s)
{
  _lang_delegate->print(_lang_delegate->user_data, s.c_str());
}

void Shell_core::print_error(const std::string &s)
{
  _lang_delegate->print_error(_lang_delegate->user_data, s.c_str());
}

bool Shell_core::password(const std::string &s, std::string &ret_pass)
{
  return _lang_delegate->password(_lang_delegate->user_data, s.c_str(), ret_pass);
}

Value Shell_core::handle_interactive_input(std::string &line, Interactive_input_state &state)
{
  return _langs[_mode]->handle_interactive_input(line, state);
}

std::string Shell_core::get_handled_input()
{
  return _langs[_mode]->get_handled_input();
}


int Shell_core::run_script(const std::string &path, boost::system::error_code &err)
{
  return _langs[_mode]->run_script(path, err);
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
  Shell_javascript *js;
  _langs[Mode_JScript] = js = new Shell_javascript(this);

  for (std::map<std::string, Value>::const_iterator iter = _globals.begin();
       iter != _globals.end(); ++iter)
    js->set_global(iter->first, iter->second);
}


void Shell_core::init_py()
{

}



void Shell_core::set_global(const std::string &name, const Value &value)
{
  _globals[name] = value;

  for (std::map<Mode, Shell_language*>::const_iterator iter = _langs.begin();
       iter != _langs.end(); ++iter)
    iter->second->set_global(name, value);
}

Value Shell_core::get_global(const std::string &name)
{
  return (_globals.count(name) > 0) ? _globals[name] : Value();
}


std::string Shell_core::prompt()
{
  return _langs[interactive_mode()]->prompt();
}


bool Shell_core::handle_shell_command(const std::string &line)
{
  return _langs[_mode]->handle_shell_command(line);
}


//------------------ COMMAND HANDLER FUNCTIONS ------------------//
bool Shell_command_handler::process(const std::string& command_line)
{
  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, command_line, boost::is_any_of(" "), boost::token_compress_on);

  Command_registry::iterator item = commands.find(tokens[0]);
  if (item != commands.end())
  {
    tokens.erase(tokens.begin());
    std::string command_params = boost::algorithm::join(tokens, " ");

    item->second(command_params);
  }

  return item != commands.end();
}

void Shell_command_handler::add_command(const std::string& triggers, boost::function<void(const std::string&)> function)
{
  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, triggers, boost::is_any_of("|"), boost::token_compress_on);

  std::vector<std::string>::iterator index= tokens.begin(), end = tokens.end();

  // Inserts a mapping for each given token
  while (index != end)
  {
    commands.insert(std::pair < const std::string&, boost::function<void(const std::string&)>>(*index, function));
    index++;
  }
}
