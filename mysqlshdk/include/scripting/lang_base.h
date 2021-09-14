/*
 * Copyright (c) 2014, 2021, Oracle and/or its affiliates.
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

class TYPES_COMMON_PUBLIC Interpreter_print_handler {
 public:
  // true -> print request was handled and other handlers should not be called
  using Print_function = bool (*)(void *, const char *);

  using Print = bool (Interpreter_print_handler::*)(const char *) const;

  Interpreter_print_handler(void *user_data, Print_function print_,
                            Print_function error, Print_function diag)
      : m_user_data(user_data),
        m_print(print_),
        m_print_error(error),
        m_print_diag(diag) {}

  virtual ~Interpreter_print_handler() = default;

  bool print(const char *msg) const { return delegate(msg, m_print); }

  bool print_error(const char *msg) const {
    return delegate(msg, m_print_error);
  }

  bool print_diag(const char *msg) const { return delegate(msg, m_print_diag); }

 protected:
  void *m_user_data = nullptr;

 private:
  bool delegate(const char *msg, Print_function func) const {
    if (func) {
      return func(m_user_data, msg);
    } else {
      return false;
    }
  }

  Print_function m_print = nullptr;
  Print_function m_print_error = nullptr;
  Print_function m_print_diag = nullptr;
};

class TYPES_COMMON_PUBLIC Interpreter_delegate
    : public Interpreter_print_handler {
 public:
  using Prompt_function = Prompt_result (*)(void *, const char *,
                                            std::string *);

  using Prompt = Prompt_result (Interpreter_delegate ::*)(const char *,
                                                          std::string *) const;

  Interpreter_delegate(void *user_data, Print_function print_,
                       Prompt_function prompt_, Prompt_function password_,
                       Print_function error, Print_function diag)
      : Interpreter_print_handler(user_data, print_, error, diag),
        m_prompt(prompt_),
        m_password(password_) {}

  Prompt_result prompt(const char *msg, std::string *result) const {
    return delegate(msg, result, m_prompt);
  }

  Prompt_result password(const char *msg, std::string *result) const {
    return delegate(msg, result, m_password);
  }

 private:
  Prompt_result delegate(const char *msg, std::string *result,
                         Prompt_function func) const {
    if (func) {
      return func(m_user_data, msg, result);
    } else {
      return Prompt_result::CTRL_D;
    }
  }

  Prompt_function m_prompt = nullptr;
  Prompt_function m_password = nullptr;
};

}  // namespace shcore

#endif
