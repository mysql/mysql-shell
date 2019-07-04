/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __CORELIBS_UTILS_NULLABLE_H__
#define __CORELIBS_UTILS_NULLABLE_H__

#include <stdexcept>

namespace mysqlshdk {
namespace utils {
template <class C>
class nullable {
 public:
  nullable() : _value(C()), _is_null(true) {}

  explicit nullable(std::nullptr_t) : _value(C()), _is_null(true) {}

  nullable(const nullable<C> &other) {
    _value = other._value;
    _is_null = other._is_null;
  }

  nullable(const C &value) {
    _value = value;
    _is_null = false;
  }

  void operator=(std::nullptr_t) { _is_null = true; }

  nullable<C> &operator=(const C &value) {
    _value = value;
    _is_null = false;
    return *this;
  }

  nullable<C> &operator=(const nullable<C> &value) = default;

  operator bool() const { return !_is_null; }

  const C &get_safe(const C &defval = C()) const {
    if (_is_null) return defval;
    return _value;
  }

  const C &operator*() const {
    if (_is_null) throw std::logic_error("Attempt to read null value");

    return _value;
  }

  C *operator->() {
    if (_is_null) throw std::logic_error("Attempt to read null value");

    return &_value;
  }

  const C *operator->() const {
    if (_is_null) throw std::logic_error("Attempt to read null value");

    return &_value;
  }

  bool operator==(const C &value) const { return !_is_null && _value == value; }

  bool operator==(std::nullptr_t) const { return _is_null; }

  bool operator==(const nullable<C> &value) const {
    return is_null() == value.is_null() &&
           (is_null() || _value == value._value);
  }

  bool is_null() const { return _is_null; }

  void reset() { _is_null = true; }

 private:
  C _value;
  bool _is_null;
};

}  // namespace utils

using null_string = utils::nullable<std::string>;
using null_bool = utils::nullable<bool>;

}  // namespace mysqlshdk
#endif /* __CORELIBS_UTILS_NULLABLE_H__ */
