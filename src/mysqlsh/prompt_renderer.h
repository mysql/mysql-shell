/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SRC_MYSQLSH_PROMPT_RENDERER_H_
#define SRC_MYSQLSH_PROMPT_RENDERER_H_

#include <stdint.h>
#include <cstdio>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/textui/textui.h"

namespace mysqlsh {

class Prompt_segment;

class Prompt_renderer {
 public:
  static const char* k_symbol_sep_right_pl;
  static const char* k_symbol_sep_right_hollow_pl;
  static const char* k_symbol_ellipsis_pl;

  explicit Prompt_renderer(int min_empty_space = 20);
  ~Prompt_renderer();

  //< How to shrink text if something doesn't fit
  enum class Shrinker_type {
    No_shrink,
    Ellipsize_on_char,
    Truncate_on_dot_from_right
  };

  void set_is_continuing(bool flag);

  void set_prompt(const std::string &text, const std::string &cont_text,
                  const mysqlshdk::textui::Style &style);

  void clear();
  void add_segment(
      const std::string &text,
      const mysqlshdk::textui::Style &style = mysqlshdk::textui::Style(),
      int priority = 10, int min_width = -1, int padding = 0,
      Shrinker_type type = Shrinker_type::No_shrink,
      const std::string *separator = nullptr);

  std::string render();

  void set_width(int width);

  enum Symbol { Ellipsis = 1, Last };

  void set_symbols(const std::vector<std::string> &s) { symbols_ = s; }
  void set_separator(const std::string &sep, const std::string &sep_alt);

 private:
  std::string render_prompt_line();
  void shrink_to_fit();

  std::list<Prompt_segment *> segments_;

  std::string sep_;
  std::string sep_alt_;
  std::vector<std::string> symbols_;

  Prompt_segment *last_segment_ = nullptr;
  std::unique_ptr<Prompt_segment> prompt_;
  std::string cont_text_;
  mysqlshdk::textui::Style cont_style_;
  bool continued_prompt_ = false;

 private:
  int width_ = 80;
  int height_ = 25;
  int min_empty_space_ = 20;
  int last_prompt_width_ = 0;
  bool width_override_ = false;
};

}  // namespace mysqlsh
#endif  // SRC_MYSQLSH_PROMPT_RENDERER_H_
