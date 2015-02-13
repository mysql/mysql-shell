/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SHELL_SQL_H_
#define _SHELL_SQL_H_

#include "shellcore/shell_core.h"
#include "../modules/mod_session.h"
#include <boost/system/error_code.hpp>

namespace shcore
{

class Shell_sql : public Shell_language
{
public:
  Shell_sql(Shell_core *owner);

  virtual void set_global(const std::string &name, const Value &value) {}

  virtual Value handle_interactive_input(std::string &code, Interactive_input_state &state);
  virtual int run_script(const std::string &path, boost::system::error_code &err);

  void print_warnings(boost::shared_ptr<mysh::Session> session);
  
  virtual std::string prompt();

private:
  std::string _delimiter;
  int _continuing_line;
  
};

};

#endif
