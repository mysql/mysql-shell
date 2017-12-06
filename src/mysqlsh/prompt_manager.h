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

#ifndef SRC_MYSQLSH_PROMPT_MANAGER_H_
#define SRC_MYSQLSH_PROMPT_MANAGER_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "mysqlsh/prompt_renderer.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "scripting/types.h"

namespace mysqlsh {

class Prompt_manager {
 public:
  typedef std::map<std::string, std::string> Variables_map;

  enum Dynamic_variable_type {
    Mysql_system_variable,
    Mysql_session_variable,
    Mysql_status,
    Mysql_session_status
  };

  typedef std::function<std::string(const std::string &, Dynamic_variable_type)>
      Dynamic_variable_callback;

  Prompt_manager();
  ~Prompt_manager();

  void set_is_continuing(bool flag) { renderer_.set_is_continuing(flag); }

  void set_theme(const shcore::Value &theme);

  std::string get_prompt(Variables_map *vars,
                         Dynamic_variable_callback query_var);

 public:
  class Custom_variable {
   public:
    const std::string &name() const { return name_; }
    virtual std::string evaluate(
        const std::function<std::string(const std::string &)> &apply_vars) = 0;
    virtual ~Custom_variable() {}

   protected:
    std::string name_;
    std::string true_value_;
    std::string false_value_;
  };

 private:
  struct Attributes {
    mysqlshdk::textui::Style style;
    std::string text;
    std::unique_ptr<std::string> sep;
    int min_width = 2;
    int padding = 0;
    Prompt_renderer::Shrinker_type shrink =
        Prompt_renderer::Shrinker_type::No_shrink;

    void load(const shcore::Value::Map_type_ref &attribs);
  };

  shcore::Value::Map_type_ref theme_;
  Prompt_renderer renderer_;
  std::map<std::string, std::unique_ptr<Custom_variable>> custom_variables_;

  std::string do_apply_vars(
      const std::string &s, Prompt_manager::Variables_map *vars,
      Prompt_manager::Dynamic_variable_callback query_var,
      int recursion_depth = 0);

  void update(
      const std::function<std::string(const std::string &)> &apply_vars);

  void apply_classes(
      shcore::Value::Array_type_ref classes, Attributes *attributes,
      const std::function<std::string(const std::string &)> &apply_vars);

  void load_variables(const shcore::Value::Map_type_ref &vars);

#ifdef FRIEND_TEST
  FRIEND_TEST(Shell_prompt_manager, attributes_attr);
  FRIEND_TEST(Shell_prompt_manager, attributes_other);
  FRIEND_TEST(Shell_prompt_manager, variables);
#endif
};

}  // namespace mysqlsh
#endif  // SRC_MYSQLSH_PROMPT_MANAGER_H_
