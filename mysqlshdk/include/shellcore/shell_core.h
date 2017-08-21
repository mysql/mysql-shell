/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <atomic>
#include <iostream>
#include <list>
#include <utility>

#include "scripting/common.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_core_options.h"
#include "shellcore/shell_notifications.h"

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
  std::vector<std::string> split_command_line(const std::string &command_line);
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
  explicit Shell_language(IShell_core *owner) : _owner(owner) {}

  virtual ~Shell_language() {}

  virtual void set_global(const std::string &name, const Value &value) = 0;

  virtual std::string preprocess_input_line(const std::string &s) { return s; }
  virtual void handle_input(std::string &code, Input_state &state, std::function<void(shcore::Value)> result_processor) = 0;
  virtual bool handle_shell_command(const std::string &code) { return _shell_command_handler.process(code); }
  virtual std::string get_handled_input() { return _last_handled; }

  virtual void clear_input() {}
  virtual std::string get_continued_input_context() { return ""; }

  virtual bool print_help(const std::string&) { return false; }
  virtual bool is_module(const std::string& UNUSED(file_name)) { return false; }
  virtual void execute_module(const std::string& UNUSED(file_name), std::function<void(shcore::Value)> UNUSED(result_processor)) { /* Does Nothing by default*/ }
protected:
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
  virtual void set_global(const std::string &name, const Value &value,
                          Mode_mask mode = Mode_mask::any());
  virtual Value get_global(const std::string &name);
  std::vector<std::string> get_global_objects(Mode mode);

  virtual std::shared_ptr<mysqlsh::ShellBaseSession> set_dev_session(const std::shared_ptr<mysqlsh::ShellBaseSession>& session);
  virtual std::shared_ptr<mysqlsh::ShellBaseSession> get_dev_session();

  virtual Object_registry *registry() { return _registry; }
public:
  virtual std::string preprocess_input_line(const std::string &s);
  virtual void handle_input(std::string &code, Input_state &state, std::function<void(shcore::Value)> result_processor);
  virtual bool handle_shell_command(const std::string &code);
  virtual std::string get_handled_input();
  virtual int process_stream(std::istream& stream, const std::string& source,
      std::function<void(shcore::Value)> result_processor,
      const std::vector<std::string> &argv);
  virtual bool is_module(const std::string &file_name) { return _langs[_mode]->is_module(file_name); }
  virtual void execute_module(const std::string &file_name, std::function<void(shcore::Value)> result_processor, const std::vector<std::string> &argv);

  virtual void clear_input();

  virtual Interpreter_delegate *get_delegate() { return &_delegate; }

  // To be used to stop processing from caller
  void set_error_processing() { _global_return_code = 1; }
public:
  virtual void print(const std::string &s);
  void println(const std::string &s = "", const std::string& tag = "");
  void print_value(const shcore::Value &value, const std::string& tag);
  virtual void print_error(const std::string &s);
  virtual bool password(const std::string &s, std::string &ret_pass);
  bool prompt(const std::string &s, std::string &ret_val);
  void cancel_input();
  virtual const std::string& get_input_source() { return _input_source; }
  virtual const std::vector<std::string>& get_input_args() { return _input_args; }
  virtual bool print_help(const std::string& topic);
  bool reconnect_if_needed();
  std::string get_main_delimiter() const;

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

  virtual void handle_notification(const std::string &name, const shcore::Object_bridge_ref& sender, shcore::Value::Map_type_ref data);
  void init_sql();
  void init_js();
  void init_py();

  bool reconnect();

private:
  Object_registry *_registry;
  std::map<std::string, std::pair<Mode_mask, Value> > _globals;
  std::map<Mode, Shell_language*> _langs;

  std::shared_ptr<mysqlsh::ShellBaseSession> _global_dev_session;

  Interpreter_delegate *_client_delegate;
  Interpreter_delegate _delegate;
  std::string _input_source;
  std::vector<std::string> _input_args;
  Mode _mode;
  int _global_return_code;
  bool _reconnect_session;
};
};

#endif // _SHELLCORE_H_
