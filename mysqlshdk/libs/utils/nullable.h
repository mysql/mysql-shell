/*
* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#ifndef __CORELIBS_UTILS_NULLABLE_H__
#define __CORELIBS_UTILS_NULLABLE_H__

#include <stdexcept>

namespace mysqlshdk {
namespace utils {
template<class C>
class nullable {
public:
  nullable() : _is_null(true) {};
  nullable(const nullable<C>& other) {
    _value = other._value;
    _is_null = other._is_null;
  }

  nullable(const C& value) {
    _value = value;
    _is_null = false;
  }

  C& operator=(const C &value) {
    _value = value;
    _is_null = false;

    return _value;
  }

  operator bool() const { return !_is_null; }

  operator C&() {
    if (_is_null)
      throw std::logic_error("Attempt to read null value");

    return _value;
  }

  const C& operator *() const {
    if (_is_null)
      throw std::logic_error("Attempt to read null value");

    return _value;
  }

  bool is_null() const { return _is_null; }

  void reset() {
    _is_null = true;
  }

private:
  C _value;
  bool _is_null;
};
}
}
#endif /* __CORELIBS_UTILS_NULLABLE_H__ */
