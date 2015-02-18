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

#ifndef _SHELLCORE_H_
#define _SHELLCORE_H_

#include "shellcore/types.h"
#include <boost/system/error_code.hpp>

// Helper to add commands easily to the dispatcher. Use it as:
// SET_SHELL_COMMAND("<name>", Class::method)
// Assumption is the method has the required signature: void class::method(const std::string& params)
#define SET_SHELL_COMMAND(x,y) _shell_command_handler.add_command(x,boost::bind(&y,this,_1))


namespace shcore {

enum Interactive_input_state
{
  Input_ok,
  Input_continued
};

class Object_registry;
class Shell_core;

class Shell_command_handler
{
  typedef std::map <std::string, boost::function<void(const std::string &)>> Command_registry;
private:
  Command_registry commands;

public:
  bool process(const std::string& command_line);
  void add_command(const std::string& triggers, boost::function<void(const std::string&)> function);
};


class Shell_language
{
public:
  Shell_language(Shell_core *owner): _owner(owner) {}

  virtual void set_global(const std::string &name, const Value &value) = 0;

  virtual Value handle_interactive_input(std::string &code, Interactive_input_state &state) = 0;
  virtual bool handle_shell_command(const std::string &code) { return _shell_command_handler.process(code); }
  virtual std::string get_handled_input() { return _last_handled; }
  virtual int run_script(const std::string &path, boost::system::error_code &err) = 0;
  virtual std::string prompt() = 0;
protected:
  Shell_core *_owner;
  std::string _last_handled;
  Shell_command_handler _shell_command_handler;

  // The command index will match a command name to a specific command index:
  // - A command may have many names (i.e. full name and shortcut)
  // - The names for the same command should be mapped to a unique index
  // - That unique index will be used to actually process the command
  std::map<std::string, int> _command_index;

};


struct Interpreter_delegate;

class Shell_core
{
public:
  enum Mode
  {
    Mode_None,
    Mode_SQL,
    Mode_JScript,
    Mode_Python
  };

  Shell_core(Interpreter_delegate *shdelegate);
  ~Shell_core();

  Mode interactive_mode() const { return _mode; }
  bool switch_mode(Mode mode);

  // sets a global variable, exposed to all supported scripting languages
  // the value is saved in a map, so that the exposing can be deferred in
  // case the context for some langauge is not yet created at the time this is called
  void set_global(const std::string &name, const Value &value);
  Value get_global(const std::string &name);

  Object_registry *registry() { return _registry; }
public:
  Value handle_interactive_input(std::string &code, Interactive_input_state &state);
  virtual bool handle_shell_command(const std::string &code);
  std::string get_handled_input();
  int run_script(const std::string &path, boost::system::error_code &err);

  std::string prompt();

  Interpreter_delegate *lang_delegate() { return _lang_delegate; }

public:
  void print(const std::string &s);
  void print_error(const std::string &s);
  bool password(const std::string &s, std::string &ret_pass);
private:
  void init_sql();
  void init_js();
  void init_py();

private:
  Object_registry *_registry;
  std::map<std::string, Value> _globals;
  std::map<Mode, Shell_language*> _langs;

  Interpreter_delegate *_lang_delegate;

  Mode _mode;
};

};

#endif // _SHELLCORE_H_

