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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_UUID_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_UUID_H_

#include <string>
namespace shcore {

class Id_generator {
 public:
  static std::string new_uuid();
  static std::string new_document_id();

 private:
  // return size of an array
  template <class T, std::size_t N>
  constexpr static std::size_t size(const T (&)[N]) noexcept {
    return N;
  }

  template <typename T>
  static std::string hexify(const T &data) {
    std::string s(2 * size(data), 'x');

    std::string::iterator k = s.begin();

    for (const auto &i : data) {
      *k++ = "0123456789ABCDEF"[i >> 4];
      *k++ = "0123456789ABCDEF"[i & 0x0F];
    }

    return s;
  }
};
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_UUID_H_
