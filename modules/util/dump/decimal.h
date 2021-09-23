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

#ifndef MODULES_UTIL_DUMP_DECIMAL_H_
#define MODULES_UTIL_DUMP_DECIMAL_H_

#include <cstdint>
#include <string>
#include <type_traits>

#include "mysqlshdk/libs/utils/bignum.h"

#include "mysqlshdk/libs/utils/std.h"

namespace mysqlsh {
namespace dump {

/**
 * Representation of a decimal number.
 *
 * Warning: The arithmetic is not always in line with the MySQL implementation,
 *          i.e. rounding simply removes excessive fractional digits instead of
 *          using "round half away from zero".
 *
 * Note: All operators are taking Bignum by value, as most of the time arguments
 *       are going to be created in place from integer values by converting
 *       constructors. In cases where Bignum is actually used, copy is needed
 *       anyway, when Bignum is converted to Decimal.
 */
class Decimal final {
 public:
  explicit Decimal(const std::string &d);

  Decimal(const Decimal &other) = default;
  Decimal(Decimal &&other) = default;

  Decimal &operator=(const Decimal &other) = default;
  Decimal &operator=(Decimal &&other) = default;

  ~Decimal() = default;

  std::string to_string() const;

  /**
   * Increment/decrement operators provide next decimal number, i.e.:
   *  - ++Decimal(1234) == 1235
   *  - ++Decimal(1234.5) == 1234.6
   */
  Decimal &operator++();
  Decimal operator++(int);

  Decimal &operator--();
  Decimal operator--(int);

  Decimal &operator+=(const Decimal &rhs);
  Decimal &operator-=(const Decimal &rhs);
  Decimal &operator*=(const Decimal &rhs);
  Decimal &operator/=(const Decimal &rhs);

  inline Decimal &operator+=(shcore::Bignum rhs) {
    *this += convert(std::move(rhs));
    return *this;
  }

  inline Decimal &operator-=(shcore::Bignum rhs) {
    *this -= convert(std::move(rhs));
    return *this;
  }

  inline Decimal &operator*=(shcore::Bignum rhs) {
    *this *= convert(std::move(rhs));
    return *this;
  }

  inline Decimal &operator/=(shcore::Bignum rhs) {
    *this /= convert(std::move(rhs));
    return *this;
  }

  friend Decimal operator+(Decimal lhs, const Decimal &rhs) {
    lhs += rhs;
    return lhs;
  }

  friend Decimal operator+(Decimal lhs, shcore::Bignum rhs) {
    lhs.operator+=(std::move(rhs));
    return lhs;
  }

  friend Decimal operator+(shcore::Bignum lhs, const Decimal &rhs) {
    return operator+(rhs.convert(std::move(lhs)), rhs);
  }

  friend Decimal operator-(Decimal lhs, const Decimal &rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend Decimal operator-(Decimal lhs, shcore::Bignum rhs) {
    lhs.operator-=(std::move(rhs));
    return lhs;
  }

  friend Decimal operator-(shcore::Bignum lhs, const Decimal &rhs) {
    return operator-(rhs.convert(std::move(lhs)), rhs);
  }

  friend Decimal operator*(Decimal lhs, const Decimal &rhs) {
    lhs *= rhs;
    return lhs;
  }

  friend Decimal operator*(Decimal lhs, shcore::Bignum rhs) {
    lhs.operator*=(std::move(rhs));
    return lhs;
  }

  friend Decimal operator*(shcore::Bignum lhs, const Decimal &rhs) {
    return operator*(rhs.convert(std::move(lhs)), rhs);
  }

  friend Decimal operator/(Decimal lhs, const Decimal &rhs) {
    lhs /= rhs;
    return lhs;
  }

  friend Decimal operator/(Decimal lhs, shcore::Bignum rhs) {
    lhs.operator/=(std::move(rhs));
    return lhs;
  }

  friend Decimal operator/(shcore::Bignum lhs, const Decimal &rhs) {
    return operator/(rhs.convert(std::move(lhs)), rhs);
  }

  int compare(const Decimal &rhs) const;

  bool operator==(const Decimal &rhs) const;
  bool operator!=(const Decimal &rhs) const;
  bool operator<(const Decimal &rhs) const;
  bool operator>(const Decimal &rhs) const;
  bool operator<=(const Decimal &rhs) const;
  bool operator>=(const Decimal &rhs) const;

  inline int compare(shcore::Bignum rhs) const {
    return compare(convert(std::move(rhs)));
  }

  inline bool operator==(shcore::Bignum rhs) const {
    return compare(std::move(rhs)) == 0;
  }

  inline bool operator!=(shcore::Bignum rhs) const {
    return compare(std::move(rhs)) != 0;
  }

  inline bool operator<(shcore::Bignum rhs) const {
    return compare(std::move(rhs)) < 0;
  }

  inline bool operator>(shcore::Bignum rhs) const {
    return compare(std::move(rhs)) > 0;
  }

  inline bool operator<=(shcore::Bignum rhs) const {
    return compare(std::move(rhs)) <= 0;
  }

  inline bool operator>=(shcore::Bignum rhs) const {
    return compare(std::move(rhs)) >= 0;
  }

  friend bool operator==(shcore::Bignum lhs, const Decimal &rhs) {
    return rhs.operator==(std::move(lhs));
  }

  friend bool operator!=(shcore::Bignum lhs, const Decimal &rhs) {
    return rhs.operator!=(std::move(lhs));
  }

  friend bool operator<(shcore::Bignum lhs, const Decimal &rhs) {
    return rhs.operator>(std::move(lhs));
  }

  friend bool operator>(shcore::Bignum lhs, const Decimal &rhs) {
    return rhs.operator<(std::move(lhs));
  }

  friend bool operator<=(shcore::Bignum lhs, const Decimal &rhs) {
    return rhs.operator>=(std::move(lhs));
  }

  friend bool operator>=(shcore::Bignum lhs, const Decimal &rhs) {
    return rhs.operator<=(std::move(lhs));
  }

 private:
  Decimal(shcore::Bignum &&d, std::size_t fractional);

  void validate_fractional_digits(const Decimal &other) const;

  void set_fractional_digits(std::size_t digits);

  void set_decimal_scale();

  inline Decimal convert(shcore::Bignum &&d) const {
    return Decimal{std::move(d), m_fractional_digits};
  }

  shcore::Bignum m_decimal;

  shcore::Bignum m_decimal_scale = 1;

  std::size_t m_fractional_digits = 0;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DECIMAL_H_
