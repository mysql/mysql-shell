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
#include <list>

#include <iostream>

// Helper to add commands in standard format to the dispatcher. 
// Use it as:
// SET_SHELL_COMMAND("<name>", "<description>", "<help>", Class::method)
// Assumptions:
// - Caller class has a command handler defined as _shell_command_handler
// - Command method has the required signature: void class::method(const std::vector<std::string>& params)
// - Command methid is a method on the caller class
#define SET_SHELL_COMMAND(name,desc,help,function) _shell_command_handler.add_command(name,desc,help,boost::bind(&function,this,_1))

// Helper to add commands in non standard format to the dispatcher. 
// Use it as:
// SET_CUSTOM_SHELL_COMMAND("<name>", "<description>", "<help>", <bound function>)
// Assumption:
// - Caller class has a command handler defined as _shell_command_handler
// - Bound function receives const std::vector<std::string>& params
#define SET_CUSTOM_SHELL_COMMAND(name,desc,help,function) _shell_command_handler.add_command(name,desc,help,function)


namespace shcore {

enum Interactive_input_state
{
  Input_ok,
  Input_continued
};

class Object_registry;
class Shell_core;

typedef boost::function<void(const std::vector<std::string>&) > Shell_command_function;

struct Shell_command
{
  std::string triggers;
  std::string description;
  std::string help;
  Shell_command_function function;
};

class Shell_command_handler
{
  typedef std::map <std::string, Shell_command*> Command_registry;
  typedef std::list <Shell_command> Command_list;
private:
  Command_registry _command_dict;
  Command_list _commands;

public:
  bool process(const std::string& command_line);
  void add_command(const std::string& triggers, const std::string& description, const std::string& help, Shell_command_function function);
  void print_commands(const std::string& title);
  bool print_command_help(const std::string& command);
};


class Shell_language
{
public:
  Shell_language(Shell_core *owner): _owner(owner) {}

  virtual void set_global(const std::string &name, const Value &value) = 0;

  virtual Value handle_input(std::string &code, Interactive_input_state &state, bool interactive = true) = 0;
  virtual bool handle_shell_command(const std::string &code) { return _shell_command_handler.process(code); }
  virtual std::string get_handled_input() { return _last_handled; }
  virtual std::string prompt() = 0;
  virtual bool print_help(const std::string& topic) { return true;  }
protected:
  Shell_core *_owner;
  std::string _last_handled;
  Shell_command_handler _shell_command_handler;
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
  bool switch_mode(Mode mode, bool &lang_initialized);

  // sets a global variable, exposed to all supported scripting languages
  // the value is saved in a map, so that the exposing can be deferred in
  // case the context for some langauge is not yet created at the time this is called
  void set_global(const std::string &name, const Value &value);
  Value get_global(const std::string &name);

  Object_registry *registry() { return _registry; }
public:
  Value handle_input(std::string &code, Interactive_input_state &state, bool interactive = true);
  virtual bool handle_shell_command(const std::string &code);
  std::string get_handled_input();
  int process_stream(std::istream& stream = std::cin, const std::string& source = "(shcore)");

  std::string prompt();

  Interpreter_delegate *lang_delegate() { return _lang_delegate; }

public:
  void print(const std::string &s);
  void print_error(const std::string &s);
  bool password(const std::string &s, std::string &ret_pass);
  const std::string& get_input_source() { return _input_source; }
  bool print_help(const std::string& topic);
private:
  void init_sql();
  void init_js();
  void init_py();

private:
  Object_registry *_registry;
  std::map<std::string, Value> _globals;
  std::map<Mode, Shell_language*> _langs;

  Interpreter_delegate *_lang_delegate;
  std::string _input_source;
  Mode _mode;
};

};

#endif // _SHELLCORE_H_

