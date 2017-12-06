/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _LANG_BASE_H_
#define _LANG_BASE_H_

#include <string>
#include <system_error>

#include "scripting/common.h"
#include "scripting/types.h"
#include "scripting/types_common.h"

namespace shcore {

enum class Input_state {
  Ok,
  ContinuedSingle,
  ContinuedBlock,
};

enum class Prompt_result {
  Cancel = 0,  // ^C
  Ok = 1,
  CTRL_D = -1  // EOF / Abort / Cancel
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
  }

  Interpreter_delegate(
      void *user_data, void (*print)(void *user_data, const char *text),
      Prompt_result (*prompt)(void *user_data, const char *prompt,
                              std::string *ret_input),
      Prompt_result (*password)(void *user_data, const char *prompt,
                                std::string *ret_password),
      void (*source)(void *user_data, const char *module),
      void (*print_value)(void *user_data, const shcore::Value &value,
                          const char *tag),
      void (*print_error)(void *user_data, const char *text)) {
    this->user_data = user_data;
    this->print = print;
    this->prompt = prompt;
    this->password = password;
    this->source = source;
    this->print_value = print_value;
    this->print_error = print_error;
  }

  void *user_data;
  void (*print)(void *user_data, const char *text);
  Prompt_result (*prompt)(void *user_data, const char *prompt,
                          std::string *ret_input);
  Prompt_result (*password)(void *user_data, const char *prompt,
                            std::string *ret_password);
  void (*source)(void *user_data, const char *module);
  void (*print_value)(void *user_data, const shcore::Value &value,
                      const char *tag);

  void (*print_error)(void *user_data, const char *text);
  void (*print_error_code)(void *user_data, const char *message,
                           const std::error_code &error);
};
};  // namespace shcore

#endif
