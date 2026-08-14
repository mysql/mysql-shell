/*
 * Copyright (c) 2014, 2026, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_LANG_BASE_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_LANG_BASE_H_

#include <string>
#include <system_error>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/shellcore/shell_prompt_options.h"

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

class Interpreter_print_handler {
 public:
  // true -> print request was handled and other handlers should not be called
  using Print_function = bool (*)(void *, const char *, const Dictionary_t &);

  using Print = bool (Interpreter_print_handler::*)(const char *,
                                                    const Dictionary_t &) const;

  Interpreter_print_handler(void *user_data, Print_function print_,
                            Print_function error, Print_function diag)
      : m_user_data(user_data),
        m_print(print_),
        m_print_error(error),
        m_print_diag(diag) {}

  virtual ~Interpreter_print_handler() = default;

  bool print(const char *msg, const Dictionary_t &attributes = {}) const {
    return delegate(msg, attributes, m_print);
  }

  bool print_error(const char *msg, const Dictionary_t &attributes = {}) const {
    return delegate(msg, attributes, m_print_error);
  }

  bool print_diag(const char *msg, const Dictionary_t &attributes = {}) const {
    return delegate(msg, attributes, m_print_diag);
  }

 protected:
  void *m_user_data = nullptr;

 private:
  bool delegate(const char *msg, const Dictionary_t &attributes,
                Print_function func) const {
    if (func) {
      const auto normalized_attributes = attributes ? attributes : make_dict();
      return func(m_user_data, msg, normalized_attributes);
    } else {
      return false;
    }
  }

  Print_function m_print = nullptr;
  Print_function m_print_error = nullptr;
  Print_function m_print_diag = nullptr;
};

class Interpreter_delegate : public Interpreter_print_handler {
 public:
  using Prompt_function = Prompt_result (*)(
      void *, const char *, const prompt::Prompt_options &options,
      const Dictionary_t &, std::string *);

  using Prompt = Prompt_result (Interpreter_delegate ::*)(
      const char *, const prompt::Prompt_options &options, const Dictionary_t &,
      std::string *) const;

  Interpreter_delegate(void *user_data, Print_function print_,
                       Prompt_function prompt_, Print_function error,
                       Print_function diag)
      : Interpreter_print_handler(user_data, print_, error, diag),
        m_prompt(prompt_) {}

  Prompt_result prompt(const char *msg, const prompt::Prompt_options &options,
                       const Dictionary_t &attributes,
                       std::string *result) const {
    if (m_prompt) {
      const auto normalized_attributes = attributes ? attributes : make_dict();
      return m_prompt(m_user_data, msg, options, normalized_attributes, result);
    }
    return Prompt_result::CTRL_D;
  }

 private:
  Prompt_function m_prompt = nullptr;
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_LANG_BASE_H_
