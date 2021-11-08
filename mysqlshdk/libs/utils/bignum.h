/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_BIGNUM_H_
#define MYSQLSHDK_LIBS_UTILS_BIGNUM_H_

#include <cstdint>
#include <memory>
#include <string>

namespace shcore {

class Bignum final {
 public:
  Bignum() : Bignum(UINT64_C(0)) {}

  Bignum(uint64_t d);

  Bignum(uint32_t d) : Bignum(static_cast<uint64_t>(d)) {}

  Bignum(uint16_t d) : Bignum(static_cast<uint64_t>(d)) {}

  Bignum(uint8_t d) : Bignum(static_cast<uint64_t>(d)) {}

  Bignum(int64_t d);

  Bignum(int32_t d) : Bignum(static_cast<int64_t>(d)) {}

  Bignum(int16_t d) : Bignum(static_cast<int64_t>(d)) {}

  Bignum(int8_t d) : Bignum(static_cast<int64_t>(d)) {}

  explicit Bignum(const std::string &d);

  Bignum(const Bignum &other);
  Bignum(Bignum &&other);

  Bignum &operator=(const Bignum &other);
  Bignum &operator=(Bignum &&other);

  ~Bignum();

  std::string to_string() const;

  Bignum &exp(uint64_t p);

  Bignum operator+() const;
  Bignum operator-() const;

  Bignum &operator++();
  Bignum operator++(int);

  Bignum &operator--();
  Bignum operator--(int);

  Bignum &operator+=(const Bignum &rhs);
  Bignum &operator-=(const Bignum &rhs);
  Bignum &operator*=(const Bignum &rhs);
  Bignum &operator/=(const Bignum &rhs);
  Bignum &operator%=(const Bignum &rhs);
  Bignum &operator<<=(int n);
  Bignum &operator>>=(int n);

  friend Bignum operator+(Bignum lhs, const Bignum &rhs) {
    lhs += rhs;
    return lhs;
  }

  friend Bignum operator-(Bignum lhs, const Bignum &rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend Bignum operator*(Bignum lhs, const Bignum &rhs) {
    lhs *= rhs;
    return lhs;
  }

  friend Bignum operator/(Bignum lhs, const Bignum &rhs) {
    lhs /= rhs;
    return lhs;
  }

  friend Bignum operator%(Bignum lhs, const Bignum &rhs) {
    lhs %= rhs;
    return lhs;
  }

  friend Bignum operator<<(Bignum lhs, int n) {
    lhs <<= n;
    return lhs;
  }

  friend Bignum operator>>(Bignum lhs, int n) {
    lhs >>= n;
    return lhs;
  }

  int compare(const Bignum &rhs) const;

  friend bool operator==(const Bignum &lhs, const Bignum &rhs) {
    return lhs.compare(rhs) == 0;
  }

  friend bool operator!=(const Bignum &lhs, const Bignum &rhs) {
    return lhs.compare(rhs) != 0;
  }

  friend bool operator<(const Bignum &lhs, const Bignum &rhs) {
    return lhs.compare(rhs) < 0;
  }

  friend bool operator>(const Bignum &lhs, const Bignum &rhs) {
    return lhs.compare(rhs) > 0;
  }

  friend bool operator<=(const Bignum &lhs, const Bignum &rhs) {
    return lhs.compare(rhs) <= 0;
  }

  friend bool operator>=(const Bignum &lhs, const Bignum &rhs) {
    return lhs.compare(rhs) >= 0;
  }

  void swap(Bignum &rhs);

  friend void swap(Bignum &lhs, Bignum &rhs) { lhs.swap(rhs); }

 private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_BIGNUM_H_
