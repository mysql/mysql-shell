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

namespace shcore {


class Registry
{
public:

private:
  Value _connections;
};


enum Interactive_input_state
{
  Input_ok,
  Input_continued
};


class Shell_core;

class Shell_language
{
public:
  Shell_language(Shell_core *owner) : _owner(owner) {}

  virtual Interactive_input_state handle_interactive_input(const std::string &code) = 0;

private:
  Shell_core *_owner;
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

  Mode interactive_mode() const { return _mode; }
  bool switch_mode(Mode mode);

public:
  Interactive_input_state handle_interactive_input(const std::string &code);

private:
  void init_sql();
  void init_js();
  void init_py();

private:
  Registry _registry;
  std::map<Mode, Shell_language*> _langs;

  Interpreter_delegate *_lang_delegate;

  Mode _mode;
};

};

#endif // _SHELLCORE_H_

