/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/common.h"
#include "shellcore/types.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_core_options.h"
#include "shellcore/shell_notifications.h"
#include <boost/system/error_code.hpp>
#include <list>

#include <iostream>

using namespace std::placeholders;

// Helper to add commands in standard format to the dispatcher.
// Use it as:
// SET_SHELL_COMMAND("<name>", "<description>", "<help>", Class::method)
// Assumptions:
// - Caller class has a command handler defined as _shell_command_handler
// - Command method has the required signature: void class::method(const std::vector<std::string>& params)
// - Command methid is a method on the caller class
#define SET_SHELL_COMMAND(name,desc,help,function) _shell_command_handler.add_command(name,desc,help,std::bind(&function,this,_1))

// Helper to add commands in non standard format to the dispatcher.
// Use it as:
// SET_CUSTOM_SHELL_COMMAND("<name>", "<description>", "<help>", <bound function>)
// Assumption:
// - Caller class has a command handler defined as _shell_command_handler
// - Bound function receives const std::vector<std::string>& params
#define SET_CUSTOM_SHELL_COMMAND(name,desc,help,function) _shell_command_handler.add_command(name,desc,help,function)

namespace shcore {
class Object_registry;
class Shell_core;

// A command function should return true if handling was OK
// Returning false tells the caller to continue processing it
typedef std::function<bool(const std::vector<std::string>&) > Shell_command_function;

struct Shell_command {
  std::string triggers;
  std::string description;
  std::string help;
  Shell_command_function function;
};

class SHCORE_PUBLIC Shell_command_handler {
  typedef std::map <std::string, Shell_command*> Command_registry;
  typedef std::list <Shell_command> Command_list;
private:
  Command_registry _command_dict;
  Command_list _commands;

public:
  bool process(const std::string& command_line);
  void add_command(const std::string& triggers, const std::string& description, const std::string& help, Shell_command_function function);
  std::string get_commands(const std::string& title);
  bool get_command_help(const std::string& command, std::string& help);
};

class SHCORE_PUBLIC Shell_language {
public:
  Shell_language(IShell_core *owner) : _killed(false), _owner(owner) {}

  virtual ~Shell_language() {}

  virtual void set_global(const std::string &name, const Value &value) = 0;

  virtual std::string preprocess_input_line(const std::string &s) { return s; }
  virtual void handle_input(std::string &code, Input_state &state, std::function<void(shcore::Value)> result_processor) = 0;
  virtual bool handle_shell_command(const std::string &code) { return _shell_command_handler.process(code); }
  virtual std::string get_handled_input() { return _last_handled; }
  virtual std::string prompt() = 0;
  virtual bool print_help(const std::string&) { return false; }
  virtual void abort() = 0;
protected:
  bool _killed;
  IShell_core *_owner;
  std::string _last_handled;
  Shell_command_handler _shell_command_handler;
};

struct Interpreter_delegate;

#if DOXYGEN_JS_CPP
/**
* Class that encloses the shell functionality.
*/
#endif

class SHCORE_PUBLIC Shell_core : public shcore::IShell_core, public shcore::NotificationObserver {
public:

  Shell_core(Interpreter_delegate *shdelegate);
  virtual ~Shell_core();

  virtual Mode interactive_mode() const { return _mode; }
  virtual bool switch_mode(Mode mode, bool &lang_initialized);

  // sets a global variable, exposed to all supported scripting languages
  // the value is saved in a map, so that the exposing can be deferred in
  // case the context for some langauge is not yet created at the time this is called
  virtual void set_global(const std::string &name, const Value &value);
  virtual Value get_global(const std::string &name);
  std::vector<std::string> get_global_objects();

  void set_active_session(const Value &session);
  Value active_session() const { return _active_session; }

  virtual std::shared_ptr<mysqlsh::ShellDevelopmentSession> connect_dev_session(const Argument_list &args, mysqlsh::SessionType session_type);
  virtual std::shared_ptr<mysqlsh::ShellDevelopmentSession> set_dev_session(std::shared_ptr<mysqlsh::ShellDevelopmentSession> session);
  virtual std::shared_ptr<mysqlsh::ShellDevelopmentSession> get_dev_session();

  virtual shcore::Value set_current_schema(const std::string& name);

  virtual Object_registry *registry() { return _registry; }
public:
  virtual std::string preprocess_input_line(const std::string &s);
  virtual void handle_input(std::string &code, Input_state &state, std::function<void(shcore::Value)> result_processor);
  virtual bool handle_shell_command(const std::string &code);
  virtual std::string get_handled_input();
  virtual int process_stream(std::istream& stream, const std::string& source,
      std::function<void(shcore::Value)> result_processor,
      const std::vector<std::string> &argv);

  virtual std::string prompt();

  virtual Interpreter_delegate *get_delegate() { return &_delegate; }

  bool is_running_query() { return _running_query; }
  virtual void abort();

  // To be used to stop processing from caller
  void set_error_processing() { _global_return_code = 1; }
public:
  virtual void print(const std::string &s);
  void println(const std::string &s = "", const std::string& tag = "");
  void print_value(const shcore::Value &value, const std::string& tag);
  virtual void print_error(const std::string &s);
  virtual bool password(const std::string &s, std::string &ret_pass);
  bool prompt(const std::string &s, std::string &ret_val);
  virtual const std::string& get_input_source() { return _input_source; }
  virtual const std::vector<std::string>& get_input_args() { return _input_args; }
  virtual bool print_help(const std::string& topic);
  bool reconnect_if_needed();

private:
  static void deleg_print(void *self, const char *text);
  static void deleg_print_error(void *self, const char *text);
  static void deleg_print_value(void *self, const shcore::Value &value, const char *tag);
  static bool deleg_prompt(void *self, const char *text, std::string &ret);
  static bool deleg_password(void *self, const char *text, std::string &ret);
  static void deleg_source(void *self, const char *module);

private:
  std::string format_json_output(const std::string &info, const std::string& tag);
  std::string format_json_output(const shcore::Value &info, const std::string& tag);

  virtual void handle_notification(const std::string &name, shcore::Object_bridge_ref sender, shcore::Value::Map_type_ref data);
  void set_dba_global();
  void set_shell_global();
  void init_sql();
  void init_js();
  void init_py();

  bool reconnect();

private:
  Object_registry *_registry;
  std::map<std::string, Value> _globals;
  std::map<Mode, Shell_language*> _langs;
  Value _active_session;

  std::shared_ptr<mysqlsh::ShellDevelopmentSession> _global_dev_session;

  Interpreter_delegate *_client_delegate;
  Interpreter_delegate _delegate;
  std::string _input_source;
  std::vector<std::string> _input_args;
  Mode _mode;
  int _global_return_code;
  bool _running_query;
  bool _reconnect_session;
};
};

#endif // _SHELLCORE_H_
