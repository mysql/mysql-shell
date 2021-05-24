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

#include "mysqlshdk/libs/utils/masked_value.h"

#include "unittest/gtest_clean.h"

namespace mysqlshdk {
namespace utils {

TEST(Masked_value_test, constructors) {
  const std::string expected_real_value = "real value";
  const std::string expected_masked_value = "masked value";

  // one-parameter constructors

  // reference is copied

  {
    std::string real_value = expected_real_value;

    Masked_string ms{real_value};

    EXPECT_TRUE(ms.m_real.empty());
    EXPECT_EQ(expected_real_value, ms.real());
    EXPECT_EQ(expected_real_value, ms.masked());
  }

  {
    const std::string real_value = expected_real_value;

    Masked_string ms{real_value};

    EXPECT_TRUE(ms.m_real.empty());
    EXPECT_EQ(expected_real_value, ms.real());
    EXPECT_EQ(expected_real_value, ms.masked());
  }

  {
    // string is implicitly converted to Masked_string
    [&expected_real_value](const Masked_string &ms) {
      EXPECT_TRUE(ms.m_real.empty());
      EXPECT_EQ(expected_real_value, ms.real());
      EXPECT_EQ(expected_real_value, ms.masked());
    }(expected_real_value);
  }

  // value is copied

  {
    const char *real_value = expected_real_value.c_str();

    Masked_string ms{real_value};

    EXPECT_FALSE(ms.m_real.empty());
    EXPECT_EQ(expected_real_value, ms.real());
    EXPECT_EQ(expected_real_value, ms.masked());
  }

  {
    std::string real_value = expected_real_value;

    Masked_string ms{std::move(real_value)};

    EXPECT_FALSE(ms.m_real.empty());
    EXPECT_EQ(expected_real_value, ms.real());
    EXPECT_EQ(expected_real_value, ms.masked());
  }

  {
    std::string real_value = expected_real_value;

    // string is implicitly converted to Masked_string
    [&expected_real_value](const Masked_string &ms) {
      EXPECT_FALSE(ms.m_real.empty());
      EXPECT_EQ(expected_real_value, ms.real());
      EXPECT_EQ(expected_real_value, ms.masked());
    }(std::move(real_value));
  }

  // two-parameter constructors

  // reference is copied

  {
    std::string real_value = expected_real_value;

    Masked_string ms{real_value, expected_masked_value};

    EXPECT_TRUE(ms.m_real.empty());
    EXPECT_EQ(expected_real_value, ms.real());
    EXPECT_EQ(expected_masked_value, ms.masked());
  }

  {
    const std::string real_value = expected_real_value;

    Masked_string ms{real_value, expected_masked_value};

    EXPECT_TRUE(ms.m_real.empty());
    EXPECT_EQ(expected_real_value, ms.real());
    EXPECT_EQ(expected_masked_value, ms.masked());
  }

  {
    [&expected_real_value, &expected_masked_value](const Masked_string &ms) {
      EXPECT_TRUE(ms.m_real.empty());
      EXPECT_EQ(expected_real_value, ms.real());
      EXPECT_EQ(expected_masked_value, ms.masked());
    }({expected_real_value, expected_masked_value});
  }

  // value is copied

  {
    const char *real_value = expected_real_value.c_str();

    Masked_string ms{real_value, expected_masked_value};

    EXPECT_FALSE(ms.m_real.empty());
    EXPECT_EQ(expected_real_value, ms.real());
    EXPECT_EQ(expected_masked_value, ms.masked());
  }

  {
    std::string real_value = expected_real_value;

    Masked_string ms{std::move(real_value), expected_masked_value};

    EXPECT_FALSE(ms.m_real.empty());
    EXPECT_EQ(expected_real_value, ms.real());
    EXPECT_EQ(expected_masked_value, ms.masked());
  }

  {
    std::string real_value = expected_real_value;

    [&expected_real_value, &expected_masked_value](const Masked_string &ms) {
      EXPECT_FALSE(ms.m_real.empty());
      EXPECT_EQ(expected_real_value, ms.real());
      EXPECT_EQ(expected_masked_value, ms.masked());
    }({std::move(real_value), expected_masked_value});
  }
}

TEST(Masked_value_test, implicit_conversion) {
  const std::string expected_real_value = "real value";
  const std::string expected_masked_value = "masked value";

  {
    Masked_string ms{expected_real_value};

    [&expected_real_value](const std::string &s) {
      EXPECT_EQ(expected_real_value, s);
    }(ms);
  }

  {
    Masked_string ms{expected_real_value, expected_masked_value};

    [&expected_masked_value](const std::string &s) {
      EXPECT_EQ(expected_masked_value, s);
    }(ms);
  }
}

}  // namespace utils
}  // namespace mysqlshdk
