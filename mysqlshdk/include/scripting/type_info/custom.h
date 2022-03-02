/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_CUSTOM_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_CUSTOM_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/type_info.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/nullable.h"

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
  static shcore::Value to_native(const shcore::Value &in) { return in; }
  static Value_type vtype() { return shcore::Undefined; }
  static const char *code() { return "V"; }
  static shcore::Value default_value() { return shcore::Value(); }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<shcore::Dictionary_t> {
  static shcore::Dictionary_t to_native(const shcore::Value &in) {
    return in.as_map();
  }
  static Value_type vtype() { return shcore::Map; }
  static const char *code() { return "D"; }
  static shcore::Dictionary_t default_value() { return shcore::Dictionary_t(); }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<shcore::Array_t> {
  static shcore::Array_t to_native(const shcore::Value &in) {
    return in.as_array();
  }
  static Value_type vtype() { return shcore::Array; }
  static const char *code() { return "A"; }
  static shcore::Array_t default_value() { return shcore::Array_t(); }
  static std::string desc() { return type_description(vtype()); }
};

template <>
struct Type_info<shcore::Function_base_ref> {
  static shcore::Function_base_ref to_native(const shcore::Value &in) {
    return in.as_function();
  }
  static Value_type vtype() { return shcore::Function; }
  static const char *code() { return "F"; }
  static shcore::Function_base_ref default_value() {
    return shcore::Function_base_ref();
  }
  static std::string desc() { return type_description(vtype()); }
};

template <typename Bridge_class>
struct Type_info<std::shared_ptr<Bridge_class>> {
  static std::shared_ptr<Bridge_class> to_native(const shcore::Value &in) {
    if (in.as_object()) {
      auto tmp = in.as_object<Bridge_class>();
      if (!tmp) throw std::invalid_argument("Object type mismatch");
      return tmp;
    }
    return std::shared_ptr<Bridge_class>();
  }
  static Value_type vtype() { return shcore::Object; }
  static const char *code() { return "O"; }
  static std::shared_ptr<Bridge_class> default_value() {
    return std::shared_ptr<Bridge_class>();
  }
  static std::string desc() { return type_description(vtype()); }
};

template <typename T>
struct Type_info<mysqlshdk::utils::nullable<T>> {
  static mysqlshdk::utils::nullable<T> to_native(const shcore::Value &in) {
    if (Value_type::Null == in.type) {
      return {};
    } else {
      return {Type_info<T>::to_native(in)};
    }
  }
  static Value_type vtype() { return Type_info<T>::vtype(); }
  static const char *code() { return Type_info<T>::code(); }
  static mysqlshdk::utils::nullable<T> default_value() { return {}; }
  static std::string desc() { return Type_info<T>::desc(); }
};

template <typename T>
struct Type_info<std::optional<T>> {
  static std::optional<T> to_native(const shcore::Value &in) {
    if (Value_type::Null == in.type) {
      return {};
    } else {
      return {Type_info<T>::to_native(in)};
    }
  }
  static Value_type vtype() { return Type_info<T>::vtype(); }
  static const char *code() { return Type_info<T>::code(); }
  static std::optional<T> default_value() { return {}; }
  static std::string desc() { return Type_info<T>::desc(); }
};

std::unique_ptr<Parameter_validator> get_nullable_validator();

template <typename T>
struct Validator_for<mysqlshdk::utils::nullable<T>> {
  static std::unique_ptr<Parameter_validator> get() {
    return get_nullable_validator();
  }
};

template <typename T>
struct Validator_for<std::optional<T>> {
  static std::unique_ptr<Parameter_validator> get() {
    return get_nullable_validator();
  }
};

template <>
struct Type_info<mysqlshdk::db::Connection_options> {
  // implementation is in mod_utils.cc
  static mysqlshdk::db::Connection_options to_native(const shcore::Value &in);
  static mysqlshdk::db::Connection_options default_value();
  static Value_type vtype() {
    // using undefined allows to postpone validation until to_native() is called
    return shcore::Value_type::Undefined;
  }
  static const char *code() { return "C"; }
  static std::string desc() {
    return "either a URI or a Connection Options Dictionary";
  }
};

template <>
struct Type_info<std::shared_ptr<mysqlsh::Extensible_object>> {
  // implementation is in mod_extensible_object.cc
  static std::shared_ptr<mysqlsh::Extensible_object> to_native(
      const shcore::Value &in);
  static Value_type vtype() { return shcore::Object; }
  static const char *code() { return "O"; }
  static std::shared_ptr<mysqlsh::Extensible_object> default_value() {
    return std::shared_ptr<mysqlsh::Extensible_object>();
  }
  static std::string desc() { return "an extension object"; }
};

template <typename T>
struct Type_info<Option_pack_ref<T>> {
  static Option_pack_ref<T> to_native(const shcore::Value &in) {
    Option_pack_ref<T> ret_val;

    // Null or Undefined get interpreted as a default option pack
    if (in.type != shcore::Null && in.type != shcore::Undefined) {
      ret_val.unpack(in.as_map());
    }

    return ret_val;
  }
  static Value_type vtype() { return shcore::Map; }
  static const char *code() { return "D"; }
  static Option_pack_ref<T> default_value() { return Option_pack_ref<T>(); }
  static std::string desc() { return "an options dictionary"; }
};

template <typename T>
struct Validator_for<Option_pack_ref<T>> {
  static std::unique_ptr<Parameter_validator> get() {
    auto validator = std::make_unique<Option_validator>();
    validator->set_allowed(&T::options().options());
    // Option validators should be disabled as validation is done on unpacking
    validator->set_enabled(false);
    return validator;
  }
};

}  // namespace detail
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_CUSTOM_H_
