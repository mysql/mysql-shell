/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_GENERIC_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_GENERIC_H_

#include <string>
#include <vector>

#include "mysqlshdk/include/scripting/type_info.h"
#include "mysqlshdk/include/scripting/types.h"

namespace shcore {
namespace detail {

template <>
struct Type_info<void> {
  static Value_type vtype() { return shcore::Null; }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<int64_t> {
  static int64_t to_native(const shcore::Value &in) { return in.as_int(); }
  static Value_type vtype() { return shcore::Integer; }
  static const char *code() { return "i"; }
  static int64_t default_value() { return 0; }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<uint64_t> {
  static uint64_t to_native(const shcore::Value &in) { return in.as_uint(); }
  static Value_type vtype() { return shcore::UInteger; }
  static const char *code() { return "u"; }
  static uint64_t default_value() { return 0; }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<int> {
  static int to_native(const shcore::Value &in) {
    return static_cast<int>(in.as_int());
  }
  static Value_type vtype() { return shcore::Integer; }
  static const char *code() { return "i"; }
  static int default_value() { return 0; }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<unsigned int> {
  static unsigned int to_native(const shcore::Value &in) {
    return static_cast<unsigned int>(in.as_uint());
  }
  static Value_type vtype() { return shcore::UInteger; }
  static const char *code() { return "u"; }
  static unsigned int default_value() { return 0; }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<double> {
  static double to_native(const shcore::Value &in) { return in.as_double(); }
  static Value_type vtype() { return shcore::Float; }
  static const char *code() { return "f"; }
  static double default_value() { return 0.0; }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<float> {
  static float to_native(const shcore::Value &in) {
    return static_cast<float>(in.as_double());
  }
  static Value_type vtype() { return shcore::Float; }
  static const char *code() { return "f"; }
  static float default_value() { return 0.0f; }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<bool> {
  static bool to_native(const shcore::Value &in) { return in.as_bool(); }
  static Value_type vtype() { return shcore::Bool; }
  static const char *code() { return "b"; }
  static bool default_value() { return false; }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<std::string> {
  static const std::string &to_native(const shcore::Value &in) {
    return in.get_string();
  }
  static Value_type vtype() { return shcore::String; }
  static const char *code() { return "s"; }
  static std::string default_value() { return std::string(); }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<const char *> {
  static const char *to_native(const shcore::Value &in) {
    return in.get_string().c_str();
  }
  static Value_type vtype() { return shcore::String; }
  static const char *code() { return "s"; }
  static const char *default_value() { return nullptr; }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<std::vector<std::string>> {
  static std::vector<std::string> to_native(const shcore::Value &in) {
    std::vector<std::string> strs;
    shcore::Array_t array(in.as_array());
    for (size_t i = 0; i < array->size(); ++i) {
      strs.push_back(array->at(i).get_string());
    }
    return strs;
  }
  static Value_type vtype() { return shcore::Array; }
  static const char *code() { return "A"; }
  static std::vector<std::string> default_value() { return {}; }
  static std::string desc() { return type_description(vtype()); }
};

}  // namespace detail
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_GENERIC_H_
