/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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
#ifndef MODULES_SHELL_PROMPT_OPTIONS_H_
#define MODULES_SHELL_PROMPT_OPTIONS_H_

#include <map>
#include <string>
#include <vector>

#include "mysqlshdk/include/scripting/types_cpp.h"

namespace shcore {
namespace prompt {

constexpr const char k_title[] = "title";
constexpr const char k_description[] = "description";
constexpr const char k_type[] = "type";
constexpr const char k_yes_label[] = "yes";
constexpr const char k_no_label[] = "no";
constexpr const char k_alt_label[] = "alt";
constexpr const char k_options[] = "options";
constexpr const char k_default_value[] = "defaultValue";

enum class Prompt_type {
  CONFIRM,
  DIRECTORY,
  FILEOPEN,
  FILESAVE,
  PASSWORD,
  SELECT,
  TEXT
};

std::string to_string(Prompt_type type);
Prompt_type to_prompt_type(const std::string &type);
char process_label(const std::string &s, std::string *out_display,
                   std::string *out_clean_text);

struct Prompt_options final {
 public:
  Prompt_options();

  Prompt_options(const Prompt_options &) = default;
  Prompt_options(Prompt_options &&) = default;

  Prompt_options &operator=(const Prompt_options &) = default;
  Prompt_options &operator=(Prompt_options &&) = default;

  ~Prompt_options() = default;

  static const shcore::Option_pack_def<Prompt_options> &options();

  std::string title;
  std::vector<std::string> description;
  std::vector<std::string> select_items;
  Prompt_type type = Prompt_type::TEXT;
  std::string yes_label;
  std::string no_label;
  std::string alt_label;
  shcore::Value default_value;
  bool allow_custom;

  bool is_valid_answer(const std::string &answer,
                       std::string *real_answer = nullptr) const;

  // To be called for the complete initialization of the options when they are
  // filled by hand
  void done() { on_unpacked_options(); }

 private:
  void on_unpacked_options();
  void set_type(const std::string &type);

  static bool icomp(const std::string &lhs, const std::string &rhs);
  std::map<std::string, std::string,
           bool (*)(const std::string &, const std::string &)>
      m_allowed_answers_map;
};
}  // namespace prompt
}  // namespace shcore

#endif  // MODULES_SHELL_PROMPT_OPTIONS_H_
