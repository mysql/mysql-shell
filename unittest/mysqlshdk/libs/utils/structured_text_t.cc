/* Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/utils/structured_text.h"

namespace shcore {

TEST(UtilsStructuredText, make_kvp) {
  EXPECT_EQ("foo=bar", make_kvp("foo", "bar"));
  EXPECT_EQ("'foo bar'=bar", make_kvp("foo bar", "bar"));
  EXPECT_EQ("'foo bar'='bar-baz'", make_kvp("foo bar", "bar-baz"));
  EXPECT_EQ("bla123='o\\'neil'", make_kvp("bla123", "o'neil"));
  EXPECT_EQ("bla123=\"o'neil\"", make_kvp("bla123", "o'neil", '"'));
  EXPECT_EQ("bla123=\"1234567890@#$%^&*()\"",
            make_kvp("bla123", "1234567890@#$%^&*()", '"'));

  EXPECT_EQ("foo=1234", make_kvp("foo", 1234));
  EXPECT_EQ("foo=1", make_kvp("foo", true));
  EXPECT_EQ("'foo bar'=1234.500000", make_kvp("foo bar", 1234.5f));
  EXPECT_EQ("\"foo bar\"=-42", make_kvp("foo bar", -42, '"'));
}

}  // namespace shcore
