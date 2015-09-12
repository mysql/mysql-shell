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

#include "shellcore/common.h"
#include "shellcore/types.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_core_options.h"
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

namespace shcore
{
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

  class SHCORE_PUBLIC Shell_command_handler
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

  class SHCORE_PUBLIC Shell_language
  {
  public:
    Shell_language(IShell_core *owner) : _owner(owner) {}
    virtual ~Shell_language(){}

    virtual void set_global(const std::string &name, const Value &value) = 0;

    virtual void handle_input(std::string &code, Interactive_input_state &state, boost::function<void(shcore::Value)> result_processor) = 0;
    virtual bool handle_shell_command(const std::string &code) { return _shell_command_handler.process(code); }
    virtual std::string get_handled_input() { return _last_handled; }
    virtual std::string prompt() = 0;
    virtual bool print_help(const std::string&) { return false; }
  protected:
    IShell_core *_owner;
    std::string _last_handled;
    Shell_command_handler _shell_command_handler;
  };

  struct Interpreter_delegate;

  class SHCORE_PUBLIC Shell_core : public shcore::IShell_core
  {
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

    virtual Object_registry *registry() { return _registry; }
  public:
    virtual void handle_input(std::string &code, Interactive_input_state &state, boost::function<void(shcore::Value)> result_processor);
    virtual bool handle_shell_command(const std::string &code);
    virtual std::string get_handled_input();
    virtual int process_stream(std::istream& stream = std::cin, const std::string& source = "(shcore)");

    virtual std::string prompt();

    virtual Interpreter_delegate *lang_delegate() { return _lang_delegate; }
  public:
    virtual void print(const std::string &s);
    virtual void print_error(const std::string &s);
    virtual bool password(const std::string &s, std::string &ret_pass);
    virtual const std::string& get_input_source() { return _input_source; }
    virtual bool print_help(const std::string& topic);
  private:
    void init_sql();
    void init_js();
    void init_py();

    void process_result(shcore::Value result);

  private:
    Object_registry *_registry;
    std::map<std::string, Value> _globals;
    std::map<Mode, Shell_language*> _langs;

    Interpreter_delegate *_lang_delegate;
    std::string _input_source;
    Mode _mode;
    int _global_return_code;
  };
};

#endif // _SHELLCORE_H_
