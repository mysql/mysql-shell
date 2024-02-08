/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_H_

#include <memory>
#include <string>
#include <type_traits>

#include "mysqlshdk/include/scripting/types.h"

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
struct Result_wrapper : public Result_wrapper_<std::remove_cvref_t<R>> {};

}  // namespace detail

template <typename T, typename Type = std::remove_cvref_t<T>>
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
    // Prepends the Arg# to the given msg.
    // If the error starts with uppercase, the final error is in the format of:
    // Argument #<number>: <error>
    //
    // i.e. Argument #2: Option X is expected to be String
    //
    // If the error starts with lowercase then the final error is in the format
    // of:
    // Argument # <number> <error>
    // i.e. Argument #2 is expected to be String.
    auto prepend_arg_index = [](uint64_t index,
                                const char *msg) -> std::string {
      std::string error{"Argument #"};
      error.append(std::to_string(index + 1));
      if (std::isupper(*msg)) error.append(":");
      error.append(" ");
      error.append(msg);
      return error;
    };

    try {
      return Type_info<T>::to_native(args.at(position));
    } catch (const shcore::Exception &ex) {
      throw Exception(ex.type(), prepend_arg_index(position, ex.what()),
                      ex.code());
    } catch (const std::runtime_error &ex) {
      throw Exception::runtime_error(prepend_arg_index(position, ex.what()));
    } catch (const std::exception &ex) {
      throw Exception::argument_error(prepend_arg_index(position, ex.what()));
    }
  }
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_H_
