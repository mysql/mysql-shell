/*
 * Copyright (c) 2019, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_CUSTOM_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_CUSTOM_H_

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "mysqlshdk/include/scripting/type_info.h"
#include "mysqlshdk/include/scripting/types.h"

namespace mysqlshdk {
namespace db {

class Connection_options;

}  // namespace db
}  // namespace mysqlshdk

namespace mysqlsh {

class Extensible_object;

}  // namespace mysqlsh

namespace shcore {
namespace detail {

// This mapping allows exposed functions to receive a Value parameter
// On this case the expected type is Undefined and any value can be
// mapped to it since the coming Value will simply by passed to the
// function without any transformation
template <>
struct Type_info<shcore::Value> {
  static constexpr auto vtype = shcore::Undefined;
  static constexpr auto code = 'V';

  static shcore::Value to_native(const shcore::Value &in) { return in; }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<shcore::Dictionary_t> {
  static constexpr auto vtype = shcore::Map;
  static constexpr auto code = 'D';

  static shcore::Dictionary_t to_native(const shcore::Value &in) {
    return in.as_map();
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<shcore::Array_t> {
  static constexpr auto vtype = shcore::Array;
  static constexpr auto code = 'A';

  static shcore::Array_t to_native(const shcore::Value &in) {
    return in.as_array();
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<shcore::Function_base_ref> {
  static constexpr auto vtype = shcore::Function;
  static constexpr auto code = 'F';

  static shcore::Function_base_ref to_native(const shcore::Value &in) {
    return in.as_function();
  }
  static std::string desc() { return type_description(vtype); }
};

template <typename Bridge_class>
struct Type_info<std::shared_ptr<Bridge_class>> {
  static constexpr auto vtype = shcore::Object;
  static constexpr auto code = 'O';

  static std::shared_ptr<Bridge_class> to_native(const shcore::Value &in) {
    if (in.as_object()) {
      auto tmp = in.as_object<Bridge_class>();
      if (!tmp) throw std::invalid_argument("Object type mismatch");
      return tmp;
    }
    return std::shared_ptr<Bridge_class>();
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<mysqlshdk::db::Connection_options> {
  // using undefined allows to postpone validation until to_native() is called
  static constexpr auto vtype = shcore::Undefined;
  static constexpr auto code = 'C';

  // implementation is in mod_utils.cc
  static mysqlshdk::db::Connection_options to_native(const shcore::Value &in);
  static std::string desc() {
    return "either a URI or a Connection Options Dictionary";
  }
};

template <>
struct Type_info<std::shared_ptr<mysqlsh::Extensible_object>> {
  static constexpr auto vtype = shcore::Object;
  static constexpr auto code = 'O';

  // implementation is in mod_extensible_object.cc
  static std::shared_ptr<mysqlsh::Extensible_object> to_native(
      const shcore::Value &in);
  static std::string desc() { return "an extension object"; }
};

}  // namespace detail
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_CUSTOM_H_
