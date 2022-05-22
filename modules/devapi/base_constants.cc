/*
 * Copyright (c) 2014, 2022, Oracle and/or its affiliates.
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

#include "modules/devapi/base_constants.h"
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "scripting/object_factory.h"

using namespace shcore;
using namespace mysqlsh;

namespace {
// Retrieves the internal constant value based on a Group and ID
// An exception is produced if invalid group.id data is provided
// But this should not happen as it is used internally only
Value get_constant_value(std::string_view module, std::string_view group,
                         std::string_view id) {
  if (module != "mysqlx" && module != "mysql")
    throw shcore::Exception::logic_error(
        "Invalid module on constant definition:" + std::string{group} + "." +
        std::string{id});

  if (group == "Type") {
    if (id == "BIT") return Value("BIT");
    if (id == "TINYINT") return Value("TINYINT");
    if (id == "SMALLINT") return Value("SMALLINT");
    if (id == "MEDIUMINT") return Value("MEDIUMINT");
    // These 3 are treated as integer as the difference is determined by the
    // flags on the column metadata
    if (id == "INT" || id == "INTEGER" || id == "UINTEGER") return Value("INT");
    if (id == "BIGINT") return Value("BIGINT");
    if (id == "FLOAT") return Value("FLOAT");
    if (id == "DECIMAL") return Value("DECIMAL");
    if (id == "DOUBLE") return Value("DOUBLE");

    // These are new
    if (id == "JSON") return Value("JSON");
    if (id == "STRING") return Value("STRING");
    if (id == "BYTES") return Value("BYTES");
    if (id == "TIME") return Value("TIME");
    if (id == "DATE") return Value("DATE");
    if (id == "DATETIME") return Value("DATETIME");
    if (id == "SET") return Value("SET");
    if (id == "ENUM") return Value("ENUM");
    if (id == "GEOMETRY") return Value("GEOMETRY");
    // NULL is only registered for mysql module
    if (id == "NULL" && module == "mysql") return Value("NULL");

    return {};
  }

  if (group == "LockContention") {
    if (id == "NOWAIT") return Value("NOWAIT");
    if (id == "SKIP_LOCKED") return Value("SKIP_LOCKED");
    if (id == "DEFAULT") return Value("DEFAULT");
    return {};
  }

  throw shcore::Exception::logic_error(
      "Invalid group on constant definition:" + std::string{group} + "." +
      std::string{id});
}
}  // namespace

std::map<std::string, Constant::Module_constants> Constant::_constants;

Constant::Constant(std::string module, std::string group, std::string id)
    : _module(std::move(module)), _group(std::move(group)), _id(std::move(id)) {
  _data = get_constant_value(_module, _group, _id);

  add_property("data");
}

Value Constant::get_member(const std::string &prop) const {
  // Retrieves the member first from the parent
  if (prop == "data") return _data;
  return Cpp_object_bridge::get_member(prop);
}

bool Constant::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

Value Constant::get_constant(const std::string &module,
                             const std::string &group, const std::string &id,
                             const shcore::Argument_list &) {
  Value ret_val;

  if (_constants.find(module) != _constants.end()) {
    if (_constants.at(module).find(group) != _constants.at(module).end()) {
      if (_constants.at(module).at(group).find(id) !=
          _constants.at(module).at(group).end())
        ret_val = shcore::Value(std::static_pointer_cast<shcore::Object_bridge>(
            _constants.at(module).at(group).at(id)));
    }
  }

  if (!ret_val) {
    std::shared_ptr<Constant> constant(new Constant(module, group, id));

    if (constant->data()) {
      if (_constants.find(module) == _constants.end()) {
        Module_constants new_module;
        _constants.insert({module, new_module});
      }

      if (_constants.at(module).find(group) == _constants.at(module).end()) {
        Group_constants new_group;
        _constants.at(module).insert({group, new_group});
      }

      if (_constants.at(module).at(group).find(id) ==
          _constants.at(module).at(group).end())
        _constants.at(module).at(group).insert({id, constant});

      ret_val = shcore::Value(
          std::static_pointer_cast<shcore::Object_bridge>(constant));
    }
  }

  return ret_val;
}

std::string &Constant::append_descr(std::string &s_out, int UNUSED(indent),
                                    int UNUSED(quote_strings)) const {
  s_out.append("<" + _group + "." + _id);

  if (_data.type == shcore::String) {
    const std::string &data = _data.get_string();
    size_t pos = data.find("(");
    if (pos != std::string::npos) s_out.append(data.substr(pos));
  }

  s_out.append(">");

  return s_out;
}

void Constant::set_param(const std::string &data) {
  std::string temp = _data.get_string();
  temp += "(" + data + ")";
  _data = Value(temp);
}
