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

#ifndef _LANG_BASE_H_
#define _LANG_BASE_H_

#include <string>

#include "scripting/types.h"
#include "scripting/types_common.h"
#include "scripting/common.h"

namespace shcore {

enum class Input_state {
  Ok,
  ContinuedSingle,
  ContinuedBlock,
};

struct TYPES_COMMON_PUBLIC Interpreter_delegate {
  Interpreter_delegate() {
    user_data = nullptr;
    print = nullptr;
    prompt = nullptr;
    password = nullptr;
    source = nullptr;
    print_value = nullptr;
    print_error = nullptr;
    print_error_code = nullptr;
  }

  void *user_data;
  void(*print)(void *user_data, const char *text);
  bool(*prompt)(void *user_data, const char *prompt, std::string &ret_input);
  bool(*password)(void *user_data, const char *prompt, std::string &ret_password);
  void(*source)(void *user_data, const char *module);
  void(*print_value)(void *user_data, const shcore::Value &value, const char *tag);

  void(*print_error)(void *user_data, const char *text);
  void(*print_error_code)(void *user_data, const char *message, const std::error_code &error);
};
};

#endif
