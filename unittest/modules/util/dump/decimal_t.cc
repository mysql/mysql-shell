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

#include "modules/util/dump/decimal.h"

#include "unittest/gtest_clean.h"

namespace mysqlsh {
namespace dump {

namespace {

const std::string k_fractional_digits_error = "Mismatch of fractional digits";

void EXPECT_DECIMAL(const std::string &expected, const Decimal &d,
                    const char *file, int line) {
  const auto actual = d.to_string();
  SCOPED_TRACE("expected: " + expected + ", actual: " + actual + "\n" + file +
               ":" + std::to_string(line) + ": EXPECT_DECIMAL()");
  EXPECT_EQ(expected, actual);
}

#define EXPECT_DECIMAL(expected, bn) \
  EXPECT_DECIMAL(expected, bn, __FILE__, __LINE__)

}  // namespace

TEST(Decimal_test, string_constructor) {
  EXPECT_DECIMAL("1234567890", Decimal("1234567890"));
  EXPECT_DECIMAL("123456789.0", Decimal("123456789.0"));
  EXPECT_DECIMAL("1.234567890", Decimal("1.234567890"));
  EXPECT_DECIMAL("-1234567890", Decimal("-1234567890"));
  EXPECT_DECIMAL("-123456789.0", Decimal("-123456789.0"));
  EXPECT_DECIMAL("-1.234567890", Decimal("-1.234567890"));
}

TEST(Decimal_test, copy_constructor) {
  Decimal a{"1234567890"};
  Decimal b{a};

  EXPECT_DECIMAL("1234567890", b);
}

TEST(Decimal_test, move_constructor) {
  Decimal a{"123456789.0"};
  Decimal b{std::move(a)};

  EXPECT_DECIMAL("123456789.0", b);
}

TEST(Decimal_test, copy_assignment_operator) {
  Decimal a{"-1234567890"};
  Decimal b{"9876543210"};

  b = a;

  EXPECT_DECIMAL("-1234567890", b);
}

TEST(Decimal_test, move_assignment_operator) {
  Decimal a{"-12.34567890"};
  Decimal b{"9876543210"};

  b = std::move(a);

  EXPECT_DECIMAL("-12.34567890", b);
}

TEST(Decimal_test, to_string) {
  for (const auto &value : {
           "1",
           "0.5",
           "0.05",
           "0.005",
           "-0.005",
           "-0.05",
           "-0.5",
           "-1",
           "18446744073709551615",
           "18446744073709551616.123",
           "-18446744073709551615",
           "-18446744073709551616.987654321",
       }) {
    EXPECT_DECIMAL(value, Decimal(value));
  }
}

TEST(Decimal_test, prefix_increment_operator) {
  {
    Decimal a{"-1"};
    EXPECT_DECIMAL("-1", a);

    ++a;
    EXPECT_DECIMAL("0", a);

    ++a;
    EXPECT_DECIMAL("1", a);
  }

  {
    Decimal a{"-1.9876"};
    EXPECT_DECIMAL("-1.9876", a);

    ++a;
    EXPECT_DECIMAL("-1.9875", a);

    ++a;
    EXPECT_DECIMAL("-1.9874", a);
  }
}

TEST(Decimal_test, postfix_increment_operator) {
  {
    Decimal a{"-1"};
    auto b = a;
    EXPECT_DECIMAL("-1", a);
    EXPECT_DECIMAL("-1", b);

    b = a++;
    EXPECT_DECIMAL("0", a);
    EXPECT_DECIMAL("-1", b);

    b = a++;
    EXPECT_DECIMAL("1", a);
    EXPECT_DECIMAL("0", b);
  }

  {
    Decimal a{"-0.8"};
    auto b = a;
    EXPECT_DECIMAL("-0.8", a);
    EXPECT_DECIMAL("-0.8", b);

    b = a++;
    EXPECT_DECIMAL("-0.7", a);
    EXPECT_DECIMAL("-0.8", b);

    b = a++;
    EXPECT_DECIMAL("-0.6", a);
    EXPECT_DECIMAL("-0.7", b);
  }
}

TEST(Decimal_test, prefix_decrement_operator) {
  {
    Decimal a{"1"};
    EXPECT_DECIMAL("1", a);

    --a;
    EXPECT_DECIMAL("0", a);

    --a;
    EXPECT_DECIMAL("-1", a);
  }

  {
    Decimal a{"0.1"};
    EXPECT_DECIMAL("0.1", a);

    --a;
    EXPECT_DECIMAL("0.0", a);

    --a;
    EXPECT_DECIMAL("-0.1", a);
  }
}

TEST(Decimal_test, postfix_decrement_operator) {
  {
    Decimal a{"1"};
    auto b = a;
    EXPECT_DECIMAL("1", a);
    EXPECT_DECIMAL("1", b);

    b = a--;
    EXPECT_DECIMAL("0", a);
    EXPECT_DECIMAL("1", b);

    b = a--;
    EXPECT_DECIMAL("-1", a);
    EXPECT_DECIMAL("0", b);
  }

  {
    Decimal a{"1.45"};
    auto b = a;
    EXPECT_DECIMAL("1.45", a);
    EXPECT_DECIMAL("1.45", b);

    b = a--;
    EXPECT_DECIMAL("1.44", a);
    EXPECT_DECIMAL("1.45", b);

    b = a--;
    EXPECT_DECIMAL("1.43", a);
    EXPECT_DECIMAL("1.44", b);
  }
}

TEST(Decimal_test, addition_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(a += b, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(b += a, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(a + b, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(b + a, std::logic_error, k_fractional_digits_error);
  }

  {
    Decimal a{"1"};
    a += a;
    EXPECT_DECIMAL("2", a);

    a += 1;
    EXPECT_DECIMAL("3", a);

    a += (-1);
    EXPECT_DECIMAL("2", a);

    Decimal b{"1"};
    a += b;
    EXPECT_DECIMAL("3", a);
    EXPECT_DECIMAL("1", b);

    b = Decimal{"-1"};
    a += b;
    EXPECT_DECIMAL("2", a);
    EXPECT_DECIMAL("-1", b);

    b = Decimal{"18446744073709551615"};
    a += b;
    EXPECT_DECIMAL("18446744073709551617", a);
    EXPECT_DECIMAL("18446744073709551615", b);

    EXPECT_DECIMAL("36893488147419103232", a + b);
    EXPECT_DECIMAL("18446744073709551618", a + 1);
    EXPECT_DECIMAL("18446744073709551616", a + (-1));
    EXPECT_DECIMAL("18446744073709551618", 1 + a);
    EXPECT_DECIMAL("18446744073709551616", -1 + a);
  }

  {
    Decimal a{"1.23"};
    a += a;
    EXPECT_DECIMAL("2.46", a);

    a += 1;
    EXPECT_DECIMAL("3.46", a);

    a += (-1);
    EXPECT_DECIMAL("2.46", a);

    Decimal b{"1.01"};
    a += b;
    EXPECT_DECIMAL("3.47", a);
    EXPECT_DECIMAL("1.01", b);

    b = Decimal{"-1.23"};
    a += b;
    EXPECT_DECIMAL("2.24", a);
    EXPECT_DECIMAL("-1.23", b);

    b = Decimal{"18446744073709551615.56"};
    a += b;
    EXPECT_DECIMAL("18446744073709551617.80", a);
    EXPECT_DECIMAL("18446744073709551615.56", b);

    EXPECT_DECIMAL("36893488147419103233.36", a + b);
    EXPECT_DECIMAL("18446744073709551618.80", a + 1);
    EXPECT_DECIMAL("18446744073709551616.80", a + (-1));
    EXPECT_DECIMAL("18446744073709551618.80", 1 + a);
    EXPECT_DECIMAL("18446744073709551616.80", -1 + a);
  }
}

TEST(Decimal_test, subtraction_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(a -= b, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(b -= a, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(a - b, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(b - a, std::logic_error, k_fractional_digits_error);
  }

  {
    Decimal a{"1"};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif  // __clang__
    a -= a;
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
    EXPECT_DECIMAL("0", a);

    a = Decimal{"2"};
    a -= 1;
    EXPECT_DECIMAL("1", a);

    a -= (-1);
    EXPECT_DECIMAL("2", a);

    Decimal b{"1"};
    a -= b;
    EXPECT_DECIMAL("1", a);
    EXPECT_DECIMAL("1", b);

    b = Decimal{"-1"};
    a -= b;
    EXPECT_DECIMAL("2", a);
    EXPECT_DECIMAL("-1", b);

    b = Decimal{"18446744073709551615"};
    a -= b;
    EXPECT_DECIMAL("-18446744073709551613", a);
    EXPECT_DECIMAL("18446744073709551615", b);

    EXPECT_DECIMAL("-36893488147419103228", a - b);
    EXPECT_DECIMAL("-18446744073709551614", a - 1);
    EXPECT_DECIMAL("-18446744073709551612", a - (-1));
    EXPECT_DECIMAL("18446744073709551614", 1 - a);
    EXPECT_DECIMAL("18446744073709551612", -1 - a);
  }

  {
    Decimal a{"1.23"};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif  // __clang__
    a -= a;
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
    EXPECT_DECIMAL("0.00", a);

    a = Decimal{"2.46"};
    a -= 1;
    EXPECT_DECIMAL("1.46", a);

    a -= (-1);
    EXPECT_DECIMAL("2.46", a);

    Decimal b{"1.01"};
    a -= b;
    EXPECT_DECIMAL("1.45", a);
    EXPECT_DECIMAL("1.01", b);

    b = Decimal{"-1.23"};
    a -= b;
    EXPECT_DECIMAL("2.68", a);
    EXPECT_DECIMAL("-1.23", b);

    b = Decimal{"18446744073709551615.56"};
    a -= b;
    EXPECT_DECIMAL("-18446744073709551612.88", a);
    EXPECT_DECIMAL("18446744073709551615.56", b);

    EXPECT_DECIMAL("-36893488147419103228.44", a - b);
    EXPECT_DECIMAL("-18446744073709551613.88", a - 1);
    EXPECT_DECIMAL("-18446744073709551611.88", a - (-1));
    EXPECT_DECIMAL("18446744073709551613.88", 1 - a);
    EXPECT_DECIMAL("18446744073709551611.88", -1 - a);
  }
}

TEST(Decimal_test, multiplication_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(a *= b, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(b *= a, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(a * b, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(b * a, std::logic_error, k_fractional_digits_error);
  }

  {
    Decimal a{"2"};
    a *= a;
    EXPECT_DECIMAL("4", a);

    a *= 1;
    EXPECT_DECIMAL("4", a);

    a *= (-1);
    EXPECT_DECIMAL("-4", a);

    Decimal b{"1"};
    a *= b;
    EXPECT_DECIMAL("-4", a);
    EXPECT_DECIMAL("1", b);

    b = Decimal{"-1"};
    a *= b;
    EXPECT_DECIMAL("4", a);
    EXPECT_DECIMAL("-1", b);

    b = Decimal{"18446744073709551615"};
    a *= b;
    EXPECT_DECIMAL("73786976294838206460", a);
    EXPECT_DECIMAL("18446744073709551615", b);

    EXPECT_DECIMAL("1361129467683753853705924477137396432900", a * b);
    EXPECT_DECIMAL("73786976294838206460", a * 1);
    EXPECT_DECIMAL("-73786976294838206460", a * (-1));
    EXPECT_DECIMAL("73786976294838206460", 1 * a);
    EXPECT_DECIMAL("-73786976294838206460", -1 * a);
  }

  {
    Decimal a{"1.23"};
    a *= a;
    EXPECT_DECIMAL("1.51", a);

    a *= 1;
    EXPECT_DECIMAL("1.51", a);

    a *= (-1);
    EXPECT_DECIMAL("-1.51", a);

    Decimal b{"1.01"};
    a *= b;
    EXPECT_DECIMAL("-1.52", a);
    EXPECT_DECIMAL("1.01", b);

    b = Decimal{"-1.23"};
    a *= b;
    EXPECT_DECIMAL("1.86", a);
    EXPECT_DECIMAL("-1.23", b);

    b = Decimal{"18446744073709551615.56"};
    a *= b;
    EXPECT_DECIMAL("34310943977099766004.94", a);
    EXPECT_DECIMAL("18446744073709551615.56", b);

    EXPECT_DECIMAL("632925202472945542011653624332723143940.86", a * b);
    EXPECT_DECIMAL("34310943977099766004.94", a * 1);
    EXPECT_DECIMAL("-34310943977099766004.94", a * (-1));
    EXPECT_DECIMAL("34310943977099766004.94", 1 * a);
    EXPECT_DECIMAL("-34310943977099766004.94", -1 * a);
  }
}

TEST(Decimal_test, division_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(a /= b, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(b /= a, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(a / b, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(b / a, std::logic_error, k_fractional_digits_error);
  }

  {
    Decimal a{"2"};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif  // __clang__
    a /= a;
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
    EXPECT_DECIMAL("1", a);

    a /= 1;
    EXPECT_DECIMAL("1", a);

    a /= (-1);
    EXPECT_DECIMAL("-1", a);

    Decimal b{"0"};
    EXPECT_THROW_MSG_CONTAINS(a /= b, std::runtime_error, "div by zero");

    b = Decimal{"1"};
    a /= b;
    EXPECT_DECIMAL("-1", a);
    EXPECT_DECIMAL("1", b);

    b = Decimal{"-1"};
    a /= b;
    EXPECT_DECIMAL("1", a);
    EXPECT_DECIMAL("-1", b);

    a = Decimal{"-680564733841876926852962238568698216450"};
    b = Decimal{"18446744073709551615"};
    a /= b;
    EXPECT_DECIMAL("-36893488147419103230", a);
    EXPECT_DECIMAL("18446744073709551615", b);

    EXPECT_DECIMAL("-2", a / b);
    EXPECT_DECIMAL("-36893488147419103230", a / 1);
    EXPECT_DECIMAL("36893488147419103230", a / (-1));
    EXPECT_DECIMAL("0", 1 / a);
    EXPECT_DECIMAL("0", -1 / a);
  }

  {
    Decimal a{"1.23"};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif  // __clang__
    a /= a;
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
    EXPECT_DECIMAL("1.00", a);

    a /= 1;
    EXPECT_DECIMAL("1.00", a);

    a /= (-1);
    EXPECT_DECIMAL("-1.00", a);

    Decimal b{"0.00"};
    EXPECT_THROW_MSG_CONTAINS(a /= b, std::runtime_error, "div by zero");

    b = Decimal{"1.01"};
    a /= b;
    EXPECT_DECIMAL("-0.99", a);
    EXPECT_DECIMAL("1.01", b);

    b = Decimal{"-1.23"};
    a /= b;
    EXPECT_DECIMAL("0.80", a);
    EXPECT_DECIMAL("-1.23", b);

    a = Decimal{"-680564733841876926852962238568698216450.98"};
    b = Decimal{"18446744073709551615.56"};
    a /= b;
    EXPECT_DECIMAL("-36893488147419103228.88", a);
    EXPECT_DECIMAL("18446744073709551615.56", b);

    EXPECT_DECIMAL("-1.99", a / b);
    EXPECT_DECIMAL("-36893488147419103228.88", a / 1);
    EXPECT_DECIMAL("36893488147419103228.88", a / (-1));
    EXPECT_DECIMAL("0.00", 1 / a);
    EXPECT_DECIMAL("0.00", -1 / a);
  }
}

TEST(Decimal_test, compare) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(a.compare(b), std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(b.compare(a), std::logic_error, k_fractional_digits_error);
  }

  {
    Decimal a{"0"};

    EXPECT_LT(0, a.compare(Decimal{"-1"}));
    EXPECT_EQ(0, a.compare(Decimal{"0"}));
    EXPECT_GT(0, a.compare(Decimal{"1"}));

    EXPECT_LT(0, a.compare(-1));
    EXPECT_EQ(0, a.compare(0));
    EXPECT_GT(0, a.compare(1));
  }

  {
    Decimal a{"0.00"};

    EXPECT_LT(0, a.compare(Decimal{"-0.01"}));
    EXPECT_EQ(0, a.compare(Decimal{"0.00"}));
    EXPECT_GT(0, a.compare(Decimal{"0.01"}));
  }
}

TEST(Decimal_test, equality_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(if (a == b){}, std::logic_error,
                     k_fractional_digits_error);
    EXPECT_THROW_MSG(if (b == a){}, std::logic_error,
                     k_fractional_digits_error);
  }

  {
    Decimal a{"1234567890"};
    EXPECT_TRUE(a == a);
    EXPECT_TRUE(a == UINT64_C(1234567890));
    EXPECT_TRUE(UINT64_C(1234567890) == a);
    EXPECT_FALSE(a == 1);
    EXPECT_FALSE(1 == a);

    Decimal b{"1234567890"};
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);

    b = Decimal{"1"};
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
  }

  {
    Decimal a{"1234567890.01"};
    EXPECT_TRUE(a == a);
    EXPECT_FALSE(a == UINT64_C(1234567890));
    EXPECT_FALSE(UINT64_C(1234567890) == a);
    EXPECT_FALSE(a == 1);
    EXPECT_FALSE(1 == a);

    Decimal b{"1234567890.01"};
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);

    b = Decimal{"1.01"};
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
  }
}

TEST(Decimal_test, inequality_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(if (a != b){}, std::logic_error,
                     k_fractional_digits_error);
    EXPECT_THROW_MSG(if (b != a){}, std::logic_error,
                     k_fractional_digits_error);
  }

  {
    Decimal a{"1234567890"};
    EXPECT_FALSE(a != a);
    EXPECT_FALSE(a != UINT64_C(1234567890));
    EXPECT_FALSE(UINT64_C(1234567890) != a);
    EXPECT_TRUE(a != 1);
    EXPECT_TRUE(1 != a);

    Decimal b{"1234567890"};
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(b != a);

    b = Decimal{"1"};
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
  }

  {
    Decimal a{"1234567890.01"};
    EXPECT_FALSE(a != a);
    EXPECT_TRUE(a != UINT64_C(1234567890));
    EXPECT_TRUE(UINT64_C(1234567890) != a);
    EXPECT_TRUE(a != 1);
    EXPECT_TRUE(1 != a);

    Decimal b{"1234567890.01"};
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(b != a);

    b = Decimal{"1.01"};
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
  }
}

TEST(Decimal_test, less_than_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(if (a < b){}, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(if (b < a){}, std::logic_error, k_fractional_digits_error);
  }

  {
    Decimal a{"1234567890"};
    EXPECT_FALSE(a < a);
    EXPECT_FALSE(a < UINT64_C(1234567890));
    EXPECT_FALSE(UINT64_C(1234567890) < a);
    EXPECT_FALSE(a < 1);
    EXPECT_TRUE(1 < a);

    Decimal b{"1234567890"};
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(b < a);

    b = Decimal{"1"};
    EXPECT_FALSE(a < b);
    EXPECT_TRUE(b < a);
  }

  {
    Decimal a{"1234567890.01"};
    EXPECT_FALSE(a < a);
    EXPECT_FALSE(a < UINT64_C(1234567890));
    EXPECT_TRUE(UINT64_C(1234567890) < a);
    EXPECT_FALSE(a < 1);
    EXPECT_TRUE(1 < a);

    Decimal b{"1234567890.01"};
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(b < a);

    b = Decimal{"1.01"};
    EXPECT_FALSE(a < b);
    EXPECT_TRUE(b < a);
  }
}

TEST(Decimal_test, greater_than_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(if (a > b){}, std::logic_error, k_fractional_digits_error);
    EXPECT_THROW_MSG(if (b > a){}, std::logic_error, k_fractional_digits_error);
  }

  {
    Decimal a{"1234567890"};
    EXPECT_FALSE(a > a);
    EXPECT_FALSE(a > UINT64_C(1234567890));
    EXPECT_FALSE(UINT64_C(1234567890) > a);
    EXPECT_TRUE(a > 1);
    EXPECT_FALSE(1 > a);

    Decimal b{"1234567890"};
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(b > a);

    b = Decimal{"1"};
    EXPECT_TRUE(a > b);
    EXPECT_FALSE(b > a);
  }

  {
    Decimal a{"1234567890.01"};
    EXPECT_FALSE(a > a);
    EXPECT_TRUE(a > UINT64_C(1234567890));
    EXPECT_FALSE(UINT64_C(1234567890) > a);
    EXPECT_TRUE(a > 1);
    EXPECT_FALSE(1 > a);

    Decimal b{"1234567890.01"};
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(b > a);

    b = Decimal{"1.01"};
    EXPECT_TRUE(a > b);
    EXPECT_FALSE(b > a);
  }
}

TEST(Decimal_test, less_than_or_equal_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(if (a <= b){}, std::logic_error,
                     k_fractional_digits_error);
    EXPECT_THROW_MSG(if (b <= a){}, std::logic_error,
                     k_fractional_digits_error);
  }

  {
    Decimal a{"1234567890"};
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a <= UINT64_C(1234567890));
    EXPECT_TRUE(UINT64_C(1234567890) <= a);
    EXPECT_FALSE(a <= 1);
    EXPECT_TRUE(1 <= a);

    Decimal b{"1234567890"};
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b <= a);

    b = Decimal{"1"};
    EXPECT_FALSE(a <= b);
    EXPECT_TRUE(b <= a);
  }

  {
    Decimal a{"1234567890.01"};
    EXPECT_TRUE(a <= a);
    EXPECT_FALSE(a <= UINT64_C(1234567890));
    EXPECT_TRUE(UINT64_C(1234567890) <= a);
    EXPECT_FALSE(a <= 1);
    EXPECT_TRUE(1 <= a);

    Decimal b{"1234567890.01"};
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b <= a);

    b = Decimal{"1.01"};
    EXPECT_FALSE(a <= b);
    EXPECT_TRUE(b <= a);
  }
}

TEST(Decimal_test, greater_than_or_equal_operators) {
  {
    Decimal a{"1"};
    Decimal b{"1.1"};
    EXPECT_THROW_MSG(if (a >= b){}, std::logic_error,
                     k_fractional_digits_error);
    EXPECT_THROW_MSG(if (b >= a){}, std::logic_error,
                     k_fractional_digits_error);
  }

  {
    Decimal a{"1234567890"};
    EXPECT_TRUE(a >= a);
    EXPECT_TRUE(a >= UINT64_C(1234567890));
    EXPECT_TRUE(UINT64_C(1234567890) >= a);
    EXPECT_TRUE(a >= 1);
    EXPECT_FALSE(1 >= a);

    Decimal b{"1234567890"};
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(b >= a);

    b = Decimal{"1"};
    EXPECT_TRUE(a >= b);
    EXPECT_FALSE(b >= a);
  }

  {
    Decimal a{"1234567890.01"};
    EXPECT_TRUE(a >= a);
    EXPECT_TRUE(a >= UINT64_C(1234567890));
    EXPECT_FALSE(UINT64_C(1234567890) >= a);
    EXPECT_TRUE(a >= 1);
    EXPECT_FALSE(1 >= a);

    Decimal b{"1234567890.01"};
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(b >= a);

    b = Decimal{"1.01"};
    EXPECT_TRUE(a >= b);
    EXPECT_FALSE(b >= a);
  }
}

}  // namespace dump
}  // namespace mysqlsh
