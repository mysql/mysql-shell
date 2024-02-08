/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

#include "scripting/object_registry.h"
#include <algorithm>

using namespace shcore;

Object_registry::Object_registry() {
  _registry = std::shared_ptr<Value::Map_type>(new Value::Map_type);
}

void Object_registry::set_reg(const std::string &name, const Value &value) {
  (*_registry)[name] = value;
}

Value &Object_registry::get_reg(const std::string &name) {
  Value::Map_type::iterator iter = _registry->find(name);
  if (iter != _registry->end()) return iter->second;
  throw std::invalid_argument("Registry value " + name + " does not exist");
}

void Object_registry::add_to_reg_list(
    const std::string &list_name,
    const std::shared_ptr<Object_bridge> &object) {
  Value::Map_type::iterator liter = _registry->find(list_name);

  if (liter == _registry->end())
    (*_registry)[list_name] =
        Value(std::shared_ptr<Value::Array_type>(new Value::Array_type()));
  else if (liter->second.get_type() != Array)
    throw std::invalid_argument("Registry " + list_name + " is not a list");

  liter->second.as_array()->push_back(Value(object));
}

void Object_registry::add_to_reg_list(const std::string &list_name,
                                      const Value &value) {
  Value::Map_type::iterator liter = _registry->find(list_name);

  if (liter == _registry->end())
    (*_registry)[list_name] =
        Value(std::shared_ptr<Value::Array_type>(new Value::Array_type()));
  else if (liter->second.get_type() != Array)
    throw std::invalid_argument("Registry " + list_name + " is not a list");

  liter->second.as_array()->push_back(value);
}

void Object_registry::remove_from_reg_list(
    const std::string &list_name,
    const std::shared_ptr<Object_bridge> &object) {
  Value::Map_type::iterator liter = _registry->find(list_name);
  if (liter != _registry->end() || liter->second.get_type() != Array)
    throw std::invalid_argument("Registry " + list_name + " is not a list");

  auto list_array = liter->second.as_array();

  auto iter = std::find(list_array->begin(), list_array->end(), Value(object));
  if (iter != list_array->end()) list_array->erase(iter);
}

void Object_registry::remove_from_reg_list(
    const std::string &list_name, Value::Array_type::iterator iterator) {
  Value::Map_type::iterator liter = _registry->find(list_name);
  if (liter != _registry->end() || liter->second.get_type() != Array)
    throw std::invalid_argument("Registry " + list_name + " is not a list");

  liter->second.as_array()->erase(iterator);
}

std::shared_ptr<Value::Array_type> Object_registry::get_reg_list(
    const std::string &list_name) {
  Value::Map_type::iterator liter = _registry->find(list_name);
  if (liter != _registry->end() || liter->second.get_type() != Array)
    throw std::invalid_argument("Registry " + list_name + " is not a list");

  return liter->second.as_array();
}
