/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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
#ifndef SRC_MYSQLSH_PROMPT_HANDLER_H_
#define SRC_MYSQLSH_PROMPT_HANDLER_H_

#include <string>

#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/shellcore/shell_prompt_options.h"

namespace mysqlsh {

/**
 * @brief Shell implementation for prompts
 *
 * This class implements the different types of prompts as used in the Shell:
 * - Text Prompts (Text, Directory, fileOpen, fileSave)
 * - Password Prompts
 * - Confirm Prompts
 * - Select Prompts
 */
class Prompt_handler {
 public:
  Prompt_handler(const std::function<shcore::Prompt_result(
                     bool, const char *, std::string *)> &do_prompt_cb);

  shcore::Prompt_result handle_prompt(
      const std::string &prompt, const shcore::prompt::Prompt_options &options,
      std::string *response);

 private:
  shcore::Prompt_result handle_text_prompt(
      const std::string &prompt, const shcore::prompt::Prompt_options &options,
      std::string *response);

  shcore::Prompt_result handle_confirm_prompt(
      const std::string &prompt, const shcore::prompt::Prompt_options &options,
      std::string *response);

  shcore::Prompt_result handle_select_prompt(
      const std::string &prompt, const shcore::prompt::Prompt_options &options,
      std::string *response);

  shcore::Prompt_result do_prompt(
      const std::function<char *(const char *)> &get_text, bool is_password,
      const char *text, std::string *ret);

  const std::function<shcore::Prompt_result(bool, const char *, std::string *)>
      &m_do_prompt_cb;
};

std::string render_text_prompt(const std::string &prompt,
                               const shcore::prompt::Prompt_options &options);

std::string render_confirm_prompt(const std::string &prompt,
                                  const shcore::prompt::Prompt_options &options,
                                  std::string *out_valid_answers = nullptr);

std::string render_select_prompt(const std::string &prompt,
                                 const shcore::prompt::Prompt_options &options);
}  // namespace mysqlsh

#endif  // SRC_MYSQLSH_PROMPT_HANDLER_H_
