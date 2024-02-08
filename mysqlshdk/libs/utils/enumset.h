/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_ENUMSET_H_
#define MYSQLSHDK_LIBS_UTILS_ENUMSET_H_

#include <cstdint>
#include <limits>
#include <set>
#include <type_traits>

namespace mysqlshdk {
namespace utils {

template <typename Enum, Enum last_value>
class Enum_set {
 public:
  constexpr Enum_set() noexcept : _value(0) {}

  template <typename... T>
  constexpr explicit Enum_set(Enum value, T &&... values) noexcept
      : _value(ord(value)) {
    if constexpr (sizeof...(values) > 0)
      _value = ((_value | ord(values)) | ...);
  }

  constexpr static Enum_set any() { return all(); }

  constexpr static Enum_set all() {
    Enum_set tmp;
    tmp._value = (1 << (static_cast<uint32_t>(last_value) + 1)) - 1;
    return tmp;
  }

  Enum_set &set(Enum value) {
    _value = _value | ord(value);
    return *this;
  }

  Enum_set &set(Enum_set values) {
    _value = _value | values._value;
    return *this;
  }

  Enum_set &unset(Enum value) {
    _value = _value & ~ord(value);
    return *this;
  }

  Enum_set &clear() {
    _value = 0;
    return *this;
  }

  constexpr bool is_set(Enum value) const { return _value & ord(value); }

  constexpr bool empty() const { return _value == 0; }

  constexpr bool matches_any(Enum_set set) const {
    return (_value & set._value) != 0;
  }

  Enum_set &operator|=(Enum value) { return set(value); }
  Enum_set &operator=(Enum value) { return clear().set(value); }

  constexpr operator bool() const { return !empty(); }

  constexpr Enum_set operator&(Enum value) const {
    return Enum_set(_value & ord(value));
  }

  constexpr Enum_set operator|(Enum value) const {
    return Enum_set(_value | ord(value));
  }

  constexpr bool operator==(Enum_set set) const { return _value == set._value; }

  constexpr bool operator==(Enum value) const { return _value == ord(value); }

  constexpr bool operator!=(Enum_set set) const { return _value != set._value; }

  std::set<Enum> values() const {
    std::set<Enum> enums;
    auto v = _value;
    std::underlying_type_t<Enum> e{};

    while (v) {
      if (v & UINT32_C(1)) {
        enums.emplace(static_cast<Enum>(e));
      }

      v >>= 1;
      ++e;
    }

    return enums;
  }

 private:
  static_assert(
      static_cast<int>(last_value) <= std::numeric_limits<uint32_t>::digits,
      "The last enum value should not exceed the number of available bits");

  constexpr explicit Enum_set(uint32_t v) : _value(v) {}

  constexpr inline uint32_t ord(Enum value) const {
    return 1 << static_cast<uint32_t>(value);
  }

  uint32_t _value;
};

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_ENUMSET_H_
