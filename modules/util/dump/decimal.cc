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

#include "modules/util/dump/decimal.h"

#include <stdexcept>

namespace mysqlsh {
namespace dump {

using shcore::Bignum;

Decimal::Decimal(const std::string &d) {
  const auto pos = d.find('.');

  if (std::string::npos != pos) {
    m_fractional_digits = d.length() - pos - 1;
    set_decimal_scale();

    auto dec = d;
    dec.erase(pos, 1);

    m_decimal = Bignum(dec);
  } else {
    m_decimal = Bignum(d);
  }
}

std::string Decimal::to_string() const {
  auto str = m_decimal.to_string();

  if (m_fractional_digits) {
    auto digits = str.length();
    decltype(digits) first_digit = 0;

    if ('-' == str[0]) {
      ++first_digit;
      --digits;
    }

    // if there are not enough digits to hold all the fractional digits, fill
    // with zeros
    if (digits <= m_fractional_digits) {
      str.insert(first_digit, m_fractional_digits - digits + 1, '0');
    }

    str.insert(str.size() - m_fractional_digits, 1, '.');
  }

  return str;
}

Decimal &Decimal::operator++() {
  ++m_decimal;
  return *this;
}

Decimal Decimal::operator++(int) {
  auto copy = *this;
  operator++();
  return copy;
}

Decimal &Decimal::operator--() {
  --m_decimal;
  return *this;
}

Decimal Decimal::operator--(int) {
  auto copy = *this;
  operator--();
  return copy;
}

Decimal &Decimal::operator+=(const Decimal &rhs) {
  validate_fractional_digits(rhs);
  m_decimal += rhs.m_decimal;
  return *this;
}

Decimal &Decimal::operator-=(const Decimal &rhs) {
  validate_fractional_digits(rhs);
  m_decimal -= rhs.m_decimal;
  return *this;
}

Decimal &Decimal::operator*=(const Decimal &rhs) {
  validate_fractional_digits(rhs);

  m_decimal *= rhs.m_decimal;

  if (m_fractional_digits) {
    // we did a multiplication and doubled the number of fractional digits
    // now we need to round the result, since we don't care about the precision
    // we simply "cut" the excessive digits
    m_decimal /= m_decimal_scale;
  }

  return *this;
}

Decimal &Decimal::operator/=(const Decimal &rhs) {
  validate_fractional_digits(rhs);

  auto lhs = m_decimal;

  // division will eat some fractional digits, need to multiply first in order
  // to maintain precision
  if (m_fractional_digits) {
    lhs *= m_decimal_scale;
  }

  m_decimal = lhs / rhs.m_decimal;

  return *this;
}

int Decimal::compare(const Decimal &rhs) const {
  validate_fractional_digits(rhs);
  return m_decimal.compare(rhs.m_decimal);
}

bool Decimal::operator==(const Decimal &rhs) const { return compare(rhs) == 0; }

bool Decimal::operator!=(const Decimal &rhs) const { return compare(rhs) != 0; }

bool Decimal::operator<(const Decimal &rhs) const { return compare(rhs) < 0; }

bool Decimal::operator>(const Decimal &rhs) const { return compare(rhs) > 0; }

bool Decimal::operator<=(const Decimal &rhs) const { return compare(rhs) <= 0; }

bool Decimal::operator>=(const Decimal &rhs) const { return compare(rhs) >= 0; }

Decimal::Decimal(shcore::Bignum &&d, std::size_t fractional)
    : m_decimal(std::move(d)) {
  set_fractional_digits(fractional);
}

void Decimal::validate_fractional_digits(const Decimal &other) const {
  if (m_fractional_digits != other.m_fractional_digits) {
    throw std::logic_error("Mismatch of fractional digits");
  }
}

void Decimal::set_fractional_digits(std::size_t digits) {
  if (digits < m_fractional_digits) {
    throw std::logic_error("Cannot decrease the number of fractional digits");
  }

  if (digits > m_fractional_digits) {
    m_decimal *= Bignum{10}.exp(digits - m_fractional_digits);
    m_fractional_digits = digits;
    set_decimal_scale();
  }
}

void Decimal::set_decimal_scale() {
  m_decimal_scale = Bignum{10}.exp(m_fractional_digits);
}

}  // namespace dump
}  // namespace mysqlsh
