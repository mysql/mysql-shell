/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_ENUMSET_H_
#define MYSQLSHDK_LIBS_UTILS_ENUMSET_H_

#include <cstdint>

namespace mysqlshdk {
namespace utils {

template <typename Enum, Enum last_value>
class Enum_set {
 public:
  Enum_set() : _value(0) {}

  explicit Enum_set(Enum value) : _value(ord(value)) {}

  static Enum_set any() {
    return all();
  }

  static Enum_set all() {
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

  bool is_set(Enum value) const {
    return _value & ord(value);
  }

  bool empty() const { return _value == 0; }

  bool matches_any(Enum_set set) const { return (_value & set._value) != 0; }

  Enum_set &operator|=(Enum value) { return set(value); }
  Enum_set &operator=(Enum value) { return clear().set(value); }

  operator bool() const { return !empty(); }

  Enum_set operator&(Enum value) const {
    return Enum_set(_value & ord(value));
  }

  Enum_set operator|(Enum value) const {
    return Enum_set(_value | ord(value));
  }

  bool operator==(Enum_set set) const { return _value == set._value; }

  bool operator==(Enum value) const {
    return _value == ord(value);
  }

  bool operator!=(Enum_set set) const { return _value != set._value; }

 private:
  explicit Enum_set(uint32_t v) : _value(v) {}

  inline uint32_t ord(Enum value) const {
    return 1 << static_cast<uint32_t>(value);
  }

  uint32_t _value;
};

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_ENUMSET_H_
