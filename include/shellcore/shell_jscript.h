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

#ifndef _SHELLCORE_JS_H_
#define _SHELLCORE_JS_H_

#include "shellcore/shell_core.h"



namespace shcore {

class JScript_context;

class Shell_javascript : public Shell_language
{
public:
  Shell_javascript(Shell_core *shcore);

  virtual void set_global(const std::string &name, const Value &value);

  virtual Value handle_interactive_input(std::string &code, Interactive_input_state &state);
  virtual int run_script(const std::string &path, boost::system::error_code &err);

  virtual std::string prompt();
private:
  boost::shared_ptr<JScript_context> _js;
};

};

#endif

