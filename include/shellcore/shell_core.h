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

namespace shcore {

enum Interactive_input_state
{
  Input_ok,
  Input_continued
};

class Object_registry;
class Shell_core;

class Shell_language
{
public:
  Shell_language(Shell_core *owner) {}//: _owner(owner) {}

  virtual void set_global(const std::string &name, const Value &value) = 0;

  virtual Interactive_input_state handle_interactive_input(const std::string &code) = 0;
  virtual int run_script(const std::string &path, boost::system::error_code &err) = 0;
private:
//  Shell_core *_owner;
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

  void set_global(const std::string &name, const Value &value);

  Object_registry *registry() { return _registry; }
public:
  Interactive_input_state handle_interactive_input(const std::string &code);
  int run_script(const std::string &path, boost::system::error_code &err);

  Interpreter_delegate *lang_delegate() { return _lang_delegate; }

public:
  void print(const std::string &s);
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

