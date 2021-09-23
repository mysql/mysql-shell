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

#include "unittest/gprod_clean.h"

#include "mysqlshdk/libs/utils/bignum.h"

#include <limits>

#include "unittest/gtest_clean.h"

namespace shcore {

namespace {

void EXPECT_BIGNUM(const std::string &expected, const Bignum &bn,
                   const char *file, int line) {
  const auto actual = bn.to_string();
  SCOPED_TRACE("expected: " + expected + ", actual: " + actual + "\n" + file +
               ":" + std::to_string(line) + ": EXPECT_BIGNUM()");
  EXPECT_EQ(expected, actual);
}

#define EXPECT_BIGNUM(expected, bn) \
  EXPECT_BIGNUM(expected, bn, __FILE__, __LINE__)

}  // namespace

TEST(Bignum_test, default_constructor) {
  Bignum a;
  EXPECT_BIGNUM("0", a);
}

TEST(Bignum_test, integer_constructors) {
  const auto EXPECT_VALUE = [](auto value) {
    EXPECT_BIGNUM(std::to_string(value), Bignum(value));
  };

  {
    SCOPED_TRACE("uint64_t");
    EXPECT_VALUE(std::numeric_limits<uint64_t>::min());
    EXPECT_VALUE(std::numeric_limits<uint64_t>::max());
  }

  {
    SCOPED_TRACE("uint32_t");
    EXPECT_VALUE(std::numeric_limits<uint32_t>::min());
    EXPECT_VALUE(std::numeric_limits<uint32_t>::max());
  }

  {
    SCOPED_TRACE("uint16_t");
    EXPECT_VALUE(std::numeric_limits<uint16_t>::min());
    EXPECT_VALUE(std::numeric_limits<uint16_t>::max());
  }

  {
    SCOPED_TRACE("uint8_t");
    EXPECT_VALUE(std::numeric_limits<uint8_t>::min());
    EXPECT_VALUE(std::numeric_limits<uint8_t>::max());
  }

  {
    SCOPED_TRACE("int64_t");
    EXPECT_VALUE(std::numeric_limits<int64_t>::min());
    EXPECT_VALUE(std::numeric_limits<int64_t>::max());
  }

  {
    SCOPED_TRACE("int32_t");
    EXPECT_VALUE(std::numeric_limits<int32_t>::min());
    EXPECT_VALUE(std::numeric_limits<int32_t>::max());
  }

  {
    SCOPED_TRACE("int16_t");
    EXPECT_VALUE(std::numeric_limits<int16_t>::min());
    EXPECT_VALUE(std::numeric_limits<int16_t>::max());
  }

  {
    SCOPED_TRACE("int8_t");
    EXPECT_VALUE(std::numeric_limits<int8_t>::min());
    EXPECT_VALUE(std::numeric_limits<int8_t>::max());
  }
}

TEST(Bignum_test, string_constructor) {
  EXPECT_BIGNUM("1234567890", Bignum("1234567890"));
  EXPECT_BIGNUM("-1234567890", Bignum("-1234567890"));
}

TEST(Bignum_test, copy_constructor) {
  Bignum a{"1234567890"};
  Bignum b{a};

  EXPECT_BIGNUM("1234567890", b);
}

TEST(Bignum_test, move_constructor) {
  Bignum a{"1234567890"};
  Bignum b{std::move(a)};

  EXPECT_BIGNUM("1234567890", b);
}

TEST(Bignum_test, copy_assignment_operator) {
  Bignum a{"1234567890"};
  Bignum b{"9876543210"};

  b = a;

  EXPECT_BIGNUM("1234567890", b);
}

TEST(Bignum_test, move_assignment_operator) {
  Bignum a{"1234567890"};
  Bignum b{"9876543210"};

  b = std::move(a);

  EXPECT_BIGNUM("1234567890", b);
}

TEST(Bignum_test, to_string) {
  for (const auto &value : {
           "1",
           "0",
           "-1",
           "18446744073709551615",
           "18446744073709551616",
           "-18446744073709551615",
           "-18446744073709551616",
       }) {
    EXPECT_BIGNUM(value, Bignum(value));
  }
}

TEST(Bignum_test, exp) {
  EXPECT_BIGNUM("1024", Bignum(2).exp(10));
  EXPECT_BIGNUM("1", Bignum(10).exp(0));
  EXPECT_BIGNUM("10", Bignum(10).exp(1));
  EXPECT_BIGNUM("100", Bignum(10).exp(2));
}

TEST(Bignum_test, unary_plus_operator) {
  Bignum a{"-1"};
  auto b = +a;
  EXPECT_BIGNUM("-1", a);
  EXPECT_BIGNUM("-1", b);

  ++a;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("-1", b);
}

TEST(Bignum_test, unary_minus_operator) {
  Bignum a{"-1"};
  auto b = -a;
  EXPECT_BIGNUM("-1", a);
  EXPECT_BIGNUM("1", b);

  ++a;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("1", b);

  b = -a;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("0", b);

  a = 1;
  b = -a;
  EXPECT_BIGNUM("1", a);
  EXPECT_BIGNUM("-1", b);
}

TEST(Bignum_test, prefix_increment_operator) {
  Bignum a{"-1"};
  EXPECT_BIGNUM("-1", a);

  ++a;
  EXPECT_BIGNUM("0", a);

  ++a;
  EXPECT_BIGNUM("1", a);
}

TEST(Bignum_test, postfix_increment_operator) {
  Bignum a{"-1"};
  auto b = a;
  EXPECT_BIGNUM("-1", a);
  EXPECT_BIGNUM("-1", b);

  b = a++;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("-1", b);

  b = a++;
  EXPECT_BIGNUM("1", a);
  EXPECT_BIGNUM("0", b);
}

TEST(Bignum_test, prefix_decrement_operator) {
  Bignum a{"1"};
  EXPECT_BIGNUM("1", a);

  --a;
  EXPECT_BIGNUM("0", a);

  --a;
  EXPECT_BIGNUM("-1", a);
}

TEST(Bignum_test, postfix_decrement_operator) {
  Bignum a{"1"};
  auto b = a;
  EXPECT_BIGNUM("1", a);
  EXPECT_BIGNUM("1", b);

  b = a--;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("1", b);

  b = a--;
  EXPECT_BIGNUM("-1", a);
  EXPECT_BIGNUM("0", b);
}

TEST(Bignum_test, addition_operators) {
  Bignum a{"1"};
  a += a;
  EXPECT_BIGNUM("2", a);

  Bignum b{"0"};
  a += b;
  EXPECT_BIGNUM("2", a);
  EXPECT_BIGNUM("0", b);

  b = 1;
  a += b;
  EXPECT_BIGNUM("3", a);
  EXPECT_BIGNUM("1", b);

  b = -1;
  a += b;
  EXPECT_BIGNUM("2", a);
  EXPECT_BIGNUM("-1", b);

  b = Bignum{"18446744073709551615"};
  a += b;
  EXPECT_BIGNUM("18446744073709551617", a);
  EXPECT_BIGNUM("18446744073709551615", b);

  EXPECT_BIGNUM("36893488147419103232", a + b);
  EXPECT_BIGNUM("18446744073709551618", a + 1);
  EXPECT_BIGNUM("18446744073709551616", a + (-1));
  EXPECT_BIGNUM("18446744073709551618", 1 + a);
  EXPECT_BIGNUM("18446744073709551616", -1 + a);
}

TEST(Bignum_test, subtraction_operators) {
  Bignum a{"-1"};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif  // __clang__
  a -= a;
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
  EXPECT_BIGNUM("0", a);

  Bignum b{"0"};
  a -= b;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("0", b);

  b = -1;
  a -= b;
  EXPECT_BIGNUM("1", a);
  EXPECT_BIGNUM("-1", b);

  b = 1;
  a -= b;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("1", b);

  a = -2;
  b = Bignum{"18446744073709551615"};
  a -= b;
  EXPECT_BIGNUM("-18446744073709551617", a);
  EXPECT_BIGNUM("18446744073709551615", b);

  EXPECT_BIGNUM("-36893488147419103232", a - b);
  EXPECT_BIGNUM("-18446744073709551618", a - 1);
  EXPECT_BIGNUM("-18446744073709551616", a - (-1));
  EXPECT_BIGNUM("18446744073709551618", 1 - a);
  EXPECT_BIGNUM("18446744073709551616", -1 - a);
}

TEST(Bignum_test, multiplication_operators) {
  Bignum a{"2"};
  a *= a;
  EXPECT_BIGNUM("4", a);

  Bignum b{"0"};
  a *= b;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("0", b);

  a = 2;
  b = 1;
  a *= b;
  EXPECT_BIGNUM("2", a);
  EXPECT_BIGNUM("1", b);

  b = -1;
  a *= b;
  EXPECT_BIGNUM("-2", a);
  EXPECT_BIGNUM("-1", b);

  b = Bignum{"18446744073709551615"};
  a *= b;
  EXPECT_BIGNUM("-36893488147419103230", a);
  EXPECT_BIGNUM("18446744073709551615", b);

  EXPECT_BIGNUM("-680564733841876926852962238568698216450", a * b);
  EXPECT_BIGNUM("-36893488147419103230", a * 1);
  EXPECT_BIGNUM("36893488147419103230", a * (-1));
  EXPECT_BIGNUM("-36893488147419103230", 1 * a);
  EXPECT_BIGNUM("36893488147419103230", -1 * a);
}

TEST(Bignum_test, division_operators) {
  Bignum a{"2"};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif  // __clang__
  a /= a;
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
  EXPECT_BIGNUM("1", a);

  Bignum b{"0"};
  EXPECT_THROW_MSG_CONTAINS(a /= b, std::runtime_error, "div by zero");

  b = -1;
  a /= b;
  EXPECT_BIGNUM("-1", a);
  EXPECT_BIGNUM("-1", b);

  b = 1;
  a /= b;
  EXPECT_BIGNUM("-1", a);
  EXPECT_BIGNUM("1", b);

  a = Bignum{"-680564733841876926852962238568698216450"};
  b = Bignum{"18446744073709551615"};
  a /= b;
  EXPECT_BIGNUM("-36893488147419103230", a);
  EXPECT_BIGNUM("18446744073709551615", b);

  EXPECT_BIGNUM("-2", a / b);
  EXPECT_BIGNUM("-36893488147419103230", a / 1);
  EXPECT_BIGNUM("36893488147419103230", a / (-1));
  EXPECT_BIGNUM("0", 1 / a);
  EXPECT_BIGNUM("0", -1 / a);
}

TEST(Bignum_test, modulo_operators) {
  Bignum a{"2"};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif  // __clang__
  a %= a;
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
  EXPECT_BIGNUM("0", a);

  Bignum b{"0"};
  EXPECT_THROW_MSG_CONTAINS(a %= b, std::runtime_error, "div by zero");

  a = 7;
  b = 1;
  a %= b;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("1", b);

  a = 7;
  b = -1;
  a %= b;
  EXPECT_BIGNUM("0", a);
  EXPECT_BIGNUM("-1", b);

  a = 7;
  b = 2;
  a %= b;
  EXPECT_BIGNUM("1", a);
  EXPECT_BIGNUM("2", b);

  a = 7;
  b = 3;
  EXPECT_BIGNUM("1", a % b);
  EXPECT_BIGNUM("3", a % 4);
  EXPECT_BIGNUM("3", a % (-4));
  EXPECT_BIGNUM("4", 123 % a);
  EXPECT_BIGNUM("-4", -123 % a);
}

TEST(Bignum_test, left_shift_operators) {
  Bignum a{"2"};

  EXPECT_THROW_MSG_CONTAINS(a <<= -1, std::runtime_error, "invalid shift");

  a <<= 0;
  EXPECT_BIGNUM("2", a);

  a <<= 1;
  EXPECT_BIGNUM("4", a);

  a <<= 8;
  EXPECT_BIGNUM("1024", a);

  EXPECT_BIGNUM("649037107316853453566312041152512", a << 99);
  EXPECT_BIGNUM("1298074214633706907132624082305024", a << 100);
}

TEST(Bignum_test, right_shift_operators) {
  Bignum a{"1298074214633706907132624082305024"};

  EXPECT_THROW_MSG_CONTAINS(a >>= -1, std::runtime_error, "invalid shift");

  a >>= 0;
  EXPECT_BIGNUM("1298074214633706907132624082305024", a);

  a >>= 1;
  EXPECT_BIGNUM("649037107316853453566312041152512", a);

  a >>= 9;
  EXPECT_BIGNUM("1267650600228229401496703205376", a);

  EXPECT_BIGNUM("2", a >> 99);
  EXPECT_BIGNUM("1", a >> 100);
}

TEST(Bignum_test, compare) {
  Bignum a{"0"};

  EXPECT_LT(0, a.compare(-1));
  EXPECT_EQ(0, a.compare(0));
  EXPECT_GT(0, a.compare(1));
}

TEST(Bignum_test, equality_operators) {
  Bignum a{"1234567890"};
  EXPECT_TRUE(a == a);
  EXPECT_TRUE(a == UINT64_C(1234567890));
  EXPECT_TRUE(UINT64_C(1234567890) == a);
  EXPECT_FALSE(a == 1);
  EXPECT_FALSE(1 == a);

  Bignum b{"1234567890"};
  EXPECT_TRUE(a == b);
  EXPECT_TRUE(b == a);

  b = 1;
  EXPECT_FALSE(a == b);
  EXPECT_FALSE(b == a);
}

TEST(Bignum_test, inequality_operators) {
  Bignum a{"1234567890"};
  EXPECT_FALSE(a != a);
  EXPECT_FALSE(a != UINT64_C(1234567890));
  EXPECT_FALSE(UINT64_C(1234567890) != a);
  EXPECT_TRUE(a != 1);
  EXPECT_TRUE(1 != a);

  Bignum b{"1234567890"};
  EXPECT_FALSE(a != b);
  EXPECT_FALSE(b != a);

  b = 1;
  EXPECT_TRUE(a != b);
  EXPECT_TRUE(b != a);
}

TEST(Bignum_test, less_than_operators) {
  Bignum a{"1234567890"};
  EXPECT_FALSE(a < a);
  EXPECT_FALSE(a < UINT64_C(1234567890));
  EXPECT_FALSE(UINT64_C(1234567890) < a);
  EXPECT_FALSE(a < 1);
  EXPECT_TRUE(1 < a);

  Bignum b{"1234567890"};
  EXPECT_FALSE(a < b);
  EXPECT_FALSE(b < a);

  b = 1;
  EXPECT_FALSE(a < b);
  EXPECT_TRUE(b < a);
}

TEST(Bignum_test, greater_than_operators) {
  Bignum a{"1234567890"};
  EXPECT_FALSE(a > a);
  EXPECT_FALSE(a > UINT64_C(1234567890));
  EXPECT_FALSE(UINT64_C(1234567890) > a);
  EXPECT_TRUE(a > 1);
  EXPECT_FALSE(1 > a);

  Bignum b{"1234567890"};
  EXPECT_FALSE(a > b);
  EXPECT_FALSE(b > a);

  b = 1;
  EXPECT_TRUE(a > b);
  EXPECT_FALSE(b > a);
}

TEST(Bignum_test, less_than_or_equal_operators) {
  Bignum a{"1234567890"};
  EXPECT_TRUE(a <= a);
  EXPECT_TRUE(a <= UINT64_C(1234567890));
  EXPECT_TRUE(UINT64_C(1234567890) <= a);
  EXPECT_FALSE(a <= 1);
  EXPECT_TRUE(1 <= a);

  Bignum b{"1234567890"};
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(b <= a);

  b = 1;
  EXPECT_FALSE(a <= b);
  EXPECT_TRUE(b <= a);
}

TEST(Bignum_test, greater_than_or_equal_operators) {
  Bignum a{"1234567890"};
  EXPECT_TRUE(a >= a);
  EXPECT_TRUE(a >= UINT64_C(1234567890));
  EXPECT_TRUE(UINT64_C(1234567890) >= a);
  EXPECT_TRUE(a >= 1);
  EXPECT_FALSE(1 >= a);

  Bignum b{"1234567890"};
  EXPECT_TRUE(a >= b);
  EXPECT_TRUE(b >= a);

  b = 1;
  EXPECT_TRUE(a >= b);
  EXPECT_FALSE(b >= a);
}

TEST(Bignum_test, swap) {
  Bignum a{"1234567890"};
  Bignum b{"-987654321"};
  EXPECT_BIGNUM("1234567890", a);
  EXPECT_BIGNUM("-987654321", b);

  a.swap(b);
  EXPECT_BIGNUM("-987654321", a);
  EXPECT_BIGNUM("1234567890", b);

  b.swap(a);
  EXPECT_BIGNUM("1234567890", a);
  EXPECT_BIGNUM("-987654321", b);

  swap(a, b);
  EXPECT_BIGNUM("-987654321", a);
  EXPECT_BIGNUM("1234567890", b);

  swap(b, a);
  EXPECT_BIGNUM("1234567890", a);
  EXPECT_BIGNUM("-987654321", b);
}

}  // namespace shcore
