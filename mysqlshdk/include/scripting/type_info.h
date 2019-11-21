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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/std.h"

namespace shcore {

struct Parameter_validator;

namespace detail {

template <typename T>
struct Validator_for {
  static std::unique_ptr<Parameter_validator> get() { return {}; }
};

// Helper type traits for automatic method wrapping
template <typename T>
struct Type_info {};

template <typename R>
struct Result_wrapper_ {
  template <typename F>
  static inline shcore::Value call(F f) {
    return shcore::Value(f());
  }
};

template <>
struct Result_wrapper_<void> {
  template <typename F>
  static inline shcore::Value call(F f) {
    f();
    return shcore::Value();
  }
};

template <typename R>
struct Result_wrapper : public Result_wrapper_<std20::remove_cvref_t<R>> {};

}  // namespace detail

template <typename T, typename Type = std20::remove_cvref_t<T>>
struct Type_info : detail::Type_info<Type> {
  using type = Type;

  static std::unique_ptr<Parameter_validator> validator() {
    return detail::Validator_for<type>::get();
  }
};

template <typename T>
using Type_info_t = typename Type_info<T>::type;

/**
 * This class is used in the expose__() method to validate conversion
 * of string values to numeric values.
 *
 * In case of failure the standard error message is generated.
 */
template <typename T>
struct Arg_handler {
  static Type_info_t<T> get(uint64_t position,
                            const shcore::Argument_list &args) {
    try {
      return Type_info<T>::to_native(args.at(position));
    } catch (const std::invalid_argument &ex) {
      throw Exception::argument_error(
          "Argument #" + std::to_string(position + 1) + ": " + ex.what());
    } catch (...) {
      std::string error = "Argument #";
      error.append(std::to_string(position + 1));
      error.append(" is expected to be ")
          .append(Type_info<T>::desc().append("."));
      throw Exception::argument_error(error);
    }
  }
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_H_
