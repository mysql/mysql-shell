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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_GENERIC_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_GENERIC_H_

#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/include/scripting/type_info.h"
#include "mysqlshdk/include/scripting/types.h"

namespace shcore {
namespace detail {

template <>
struct Type_info<void> {
  static constexpr auto vtype = shcore::Null;

  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<int> {
  static constexpr auto vtype = shcore::Integer;
  static constexpr auto code = 'i';

  static int to_native(const shcore::Value &in) {
    return static_cast<int>(in.as_int());
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<unsigned int> {
  static constexpr auto vtype = shcore::UInteger;
  static constexpr auto code = 'u';

  static unsigned int to_native(const shcore::Value &in) {
    return static_cast<unsigned int>(in.as_uint());
  }
  static std::string desc() { return type_description(vtype); }
};

// NOLINTBEGIN(runtime/int)

template <>
struct Type_info<long int> {
  static constexpr auto vtype = shcore::Integer;
  static constexpr auto code = 'i';

  static long int to_native(const shcore::Value &in) {
    return static_cast<long int>(in.as_int());
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<unsigned long int> {
  static constexpr auto vtype = shcore::UInteger;
  static constexpr auto code = 'u';

  static unsigned long int to_native(const shcore::Value &in) {
    return static_cast<unsigned long int>(in.as_uint());
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<long long int> {
  static constexpr auto vtype = shcore::Integer;
  static constexpr auto code = 'i';

  static long long int to_native(const shcore::Value &in) {
    return static_cast<long long int>(in.as_int());
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<unsigned long long int> {
  static constexpr auto vtype = shcore::UInteger;
  static constexpr auto code = 'u';

  static unsigned long long int to_native(const shcore::Value &in) {
    return static_cast<unsigned long long int>(in.as_uint());
  }
  static std::string desc() { return type_description(vtype); }
};

// NOLINTEND

template <>
struct Type_info<double> {
  static constexpr auto vtype = shcore::Float;
  static constexpr auto code = 'f';

  static double to_native(const shcore::Value &in) { return in.as_double(); }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<float> {
  static constexpr auto vtype = shcore::Float;
  static constexpr auto code = 'f';

  static float to_native(const shcore::Value &in) {
    return static_cast<float>(in.as_double());
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<bool> {
  static constexpr auto vtype = shcore::Bool;
  static constexpr auto code = 'b';

  static bool to_native(const shcore::Value &in) { return in.as_bool(); }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<std::string> {
  static constexpr auto vtype = shcore::String;
  static constexpr auto code = 's';

  static const std::string &to_native(const shcore::Value &in) {
    return in.get_string();
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<std::string_view> {
  static constexpr auto vtype = shcore::String;
  static constexpr auto code = 's';

  static const std::string &to_native(const shcore::Value &in) {
    return in.get_string();
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<const char *> {
  static constexpr auto vtype = shcore::String;
  static constexpr auto code = 's';

  static const char *to_native(const shcore::Value &in) {
    return in.get_string().c_str();
  }
  static std::string desc() { return type_description(vtype); }
};

template <>
struct Type_info<std::vector<std::string>> {
  static constexpr auto vtype = shcore::Array;
  static constexpr auto code = 'A';

  static std::vector<std::string> to_native(const shcore::Value &in) {
    if (const auto array = in.as_array()) {
      try {
        return in.to_string_container<std::vector<std::string>>();
      } catch (...) {
      }
    }

    throw shcore::Exception::type_error("is expected to be " + desc());
  }

  static std::string desc() { return type_description(vtype) + " of strings"; }
};

template <>
struct Type_info<std::unordered_set<std::string>> {
  static constexpr auto vtype = shcore::Array;
  static constexpr auto code = 'A';

  static std::unordered_set<std::string> to_native(const shcore::Value &in) {
    if (const auto array = in.as_array()) {
      try {
        return in.to_string_container<std::unordered_set<std::string>>();
      } catch (...) {
      }
    }

    throw shcore::Exception::type_error("is expected to be " + desc());
  }

  static std::string desc() { return type_description(vtype) + " of strings"; }
};

template <>
struct Type_info<std::map<std::string, std::string>> {
  static constexpr auto vtype = shcore::Map;
  static constexpr auto code = 'D';

  static std::map<std::string, std::string> to_native(const shcore::Value &in) {
    if (const auto map = in.as_map()) {
      try {
        return in.to_string_map();
      } catch (...) {
      }
    }

    throw shcore::Exception::type_error("is expected to be " + desc());
  }

  static std::string desc() { return type_description(vtype) + " of strings"; }
};

template <class C>
struct Type_info<std::map<std::string, C>> {
  static constexpr auto vtype = shcore::Map;
  static constexpr auto code = 'D';

  static std::map<std::string, C> to_native(const shcore::Value &in) {
    if (const auto map = in.as_map()) {
      try {
        return in.to_container_map<C>();
      } catch (...) {
      }
    }

    throw shcore::Exception::type_error("is expected to be " + desc());
  }

  static std::string desc() {
    const auto plural = [](std::string_view s) {
      if (shcore::str_ibeginswith(s, "a ")) {
        s.remove_prefix(2);
      } else if (shcore::str_ibeginswith(s, "an ")) {
        s.remove_prefix(3);
      }

      std::string p{s};

      if ('y' == p.back()) {
        p.pop_back();

        const auto v = p.back();

        if ('a' == v || 'e' == v || 'i' == v || 'o' == v || 'u' == v) {
          p.append("ys");
        } else {
          p.append("ies");
        }

      } else {
        p.append(1, 's');
      }

      return p;
    };

    auto type = type_description(vtype);
    type += " of ";
    type += plural(type_description(Type_info<C>::vtype));
    type += " of ";
    type += plural(type_description(Type_info<typename C::value_type>::vtype));

    return type;
  }
};

template <typename T>
struct Type_info<std::optional<T>> {
  static constexpr auto vtype = Type_info<T>::vtype;
  static constexpr auto code = Type_info<T>::code;

  static std::optional<T> to_native(const shcore::Value &in) {
    if (Value_type::Null == in.get_type()) return {};
    return {Type_info<T>::to_native(in)};
  }
  static std::string desc() { return Type_info<T>::desc(); }
};

template <typename... Types>
struct Type_info<std::variant<Types...>> {
  // using undefined allows to postpone validation until to_native() is called
  static constexpr auto vtype = shcore::Undefined;
  static constexpr auto code = 'V';

  static std::variant<Types...> to_native(const shcore::Value &in) {
    std::variant<Types...> result;

    if ((to_native<Types>(in, &result) || ...)) {
      return result;
    } else {
      throw Exception::type_error("is expected to be " + desc());
    }
  }

  static std::string desc() {
    return "either " + shcore::str_join(
                           std::vector<std::string>{
                               type_description(Type_info<Types>::vtype)...},
                           " or ");
  }

 private:
  template <typename T>
  static bool to_native(const Value &v, std::variant<Types...> *out) {
    try {
      *out = Type_info<T>::to_native(v);
      return true;
    } catch (...) {
      return false;
    }
  }
};

}  // namespace detail
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_GENERIC_H_
