/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

namespace shcore {
namespace utils {

template <typename Enum, Enum last_value>
class Enum_set {
 public:
  Enum_set() : _value(0) {}

  explicit Enum_set(Enum value) : _value(1 << static_cast<uint32_t>(value)) {}

  static Enum_set any() {
    Enum_set tmp;
    tmp._value = (1 << (static_cast<uint32_t>(last_value) + 1)) - 1;
    return tmp;
  }

  Enum_set &set(Enum value) {
    _value = _value | (1 << static_cast<uint32_t>(value));
    return *this;
  }

  Enum_set &unset(Enum value) {
    _value = _value & ~(1 << static_cast<uint32_t>(value));
    return *this;
  }

  bool is_set(Enum value) const {
    return _value & (1 << static_cast<uint32_t>(value));
  }

  bool empty() const { return _value == 0; }

  bool matches_any(Enum_set set) const {
    return (_value & set._value) != 0;
  }

  bool operator==(Enum_set set) const { return _value == set._value; }

 private:
  uint32_t _value;
};

}  // namespace utils
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_ENUMSET_H_
