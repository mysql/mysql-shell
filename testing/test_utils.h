/* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

   
#include "shellcore/lang_base.h"

class Shell_test_output_handler
{

public:
  // You can define per-test set-up and tear-down logic as usual.
  Shell_test_output_handler()
  {
    deleg.user_data = this;
    deleg.print = &Shell_test_output_handler::deleg_print;
    deleg.print_error = &Shell_test_output_handler::deleg_print_error;
  }

  virtual void TearDown()
  {
  }

  static void deleg_print(void *user_data, const char *text)
  {
    Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
    target->std_out.append(text);
  }

  static void deleg_print_error(void *user_data, const char *text)
  {
    Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
    target->std_err.append(text);
  }

  static bool password(void *user_data, const char *prompt, std::string &ret)
  {
    Shell_test_output_handler* target = (Shell_test_output_handler*)(user_data);
    ret = target->ret_pwd;
    return true;
  }

  
  void wipe_out() {std_out.clear(); }
  void wipe_err() {std_err.clear(); }

  shcore::Interpreter_delegate deleg;
  std::string std_err;
  std::string std_out;
  std::string ret_pwd;
};
