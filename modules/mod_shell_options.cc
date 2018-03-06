/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_shell_options.h"
#include "modules/mysqlxtest_utils.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace shcore {

std::string &Mod_shell_options::append_descr(std::string &s_out, int indent,
                                             int quote_strings) const {
  shcore::Value::Map_type_ref ops = std::make_shared<shcore::Value::Map_type>();
  for (const auto &name : options->get_named_options())
    (*ops)[name] = Value(options->get(name));
  Value(ops).append_descr(s_out, indent, quote_strings);
  return s_out;
}

bool Mod_shell_options::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name();
}

Value Mod_shell_options::get_member(const std::string &prop) const {
  if (options->has_key(prop))
    return options->get(prop);
  return Cpp_object_bridge::get_member(prop);
}

void Mod_shell_options::set_member(const std::string &prop, Value value) {
  if (!options->has_key(prop))
    return Cpp_object_bridge::set_member(prop, value);
  options->set_and_notify(prop, value, true);
}

Mod_shell_options::Mod_shell_options(
    std::shared_ptr<mysqlsh::Shell_options> options)
    : options(options) {
  for (const auto &opt : options->get_named_options())
    add_property(opt + "|" + opt);
  add_method("unset",
             std::bind(&Mod_shell_options::unset, this, std::placeholders::_1),
             "option_name", shcore::String);
}

shcore::Value Mod_shell_options::unset(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("unset").c_str());

  try {
    std::string option_name = args.string_at(0);
    options->unset(option_name);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("unset"));
  return Value();
}

}  // namespace shcore
