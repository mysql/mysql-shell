/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_shell_options.h"
#include "shellcore/shell_notifications.h"
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
  throw Exception::logic_error("There's only one shell object!");
  return false;
}

Value Mod_shell_options::get_member(const std::string &prop) const {
  if (has_member(prop))
    return options->get(prop);
  return Value();
}

void Mod_shell_options::set_member(const std::string &prop, Value value) {
  if (options->has_key(prop)) {
    options->set(prop, value);
    shcore::Value::Map_type_ref info = shcore::Value::new_map().as_map();
    (*info)["option"] = shcore::Value(prop);
    (*info)["value"] = value;
    shcore::ShellNotifications::get()->notify(SN_SHELL_OPTION_CHANGED, nullptr,
                                              info);
  } else {
    throw shcore::Exception::attrib_error("Unable to set the property " + prop +
                                          " on the shell object.");
  }
}

Mod_shell_options::Mod_shell_options(
    std::shared_ptr<mysqlsh::Shell_options> options)
    : options(options) {
  for (const auto &opt : options->get_named_options())
    add_property(opt + "|" + opt);
}
}  // namespace shcore
