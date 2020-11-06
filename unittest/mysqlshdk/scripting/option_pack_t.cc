/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include <gtest_clean.h>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "unittest/mysqlshdk/scripting/test_option_packs.h"
#include "unittest/test_utils/shell_test_env.h"

namespace tests {

TEST(Option_pack, test_unpacking) {
  auto dict = shcore::make_dict();
  dict->set("myString", shcore::Value("Some Value"));
  dict->set("myPString", shcore::Value("Parent Value"));
  dict->set("myAString", shcore::Value("Some Aggregated Value"));
  dict->set("myIndirect", shcore::Value("Indirect Value"));
  dict->set("myPIndirect", shcore::Value("Indirect Value"));
  Sample_options my_options;
  Sample_options::options().unpack(dict, &my_options);

  EXPECT_STREQ("Some Value", my_options.str.c_str());
  EXPECT_STREQ("Some Aggregated Value", my_options.other_opts.str.c_str());
  EXPECT_STREQ("Modified Indirect Value", my_options.indirect_str.c_str());
  EXPECT_STREQ("Parent Value", my_options.parent_str.c_str());
  EXPECT_STREQ("Modified Parent Indirect Value",
               my_options.parent_indirect_str.c_str());
  EXPECT_TRUE(my_options.done_called);
}

TEST(Option_pack, test_unpacking_templated_options) {
  auto dict = shcore::make_dict();
  dict->set("myString", shcore::Value("Some Value"));
  dict->set("myInt", shcore::Value(1));
  dict->set("myIndirect", shcore::Value("Indirect Value"));
  dict->set("myBool", shcore::Value::True());
  auto array = shcore::make_array();
  array->emplace_back("first element");
  dict->set("myList", shcore::Value(array));

  Templated_options_pack<Templated_options_type::One> my_one_options;
  EXPECT_THROW_LIKE(my_one_options.options().unpack(dict, &my_one_options),
                    shcore::Exception, "Invalid options: myBool, myList");

  Templated_options_pack<Templated_options_type::Two> my_two_options;
  EXPECT_THROW_LIKE(my_two_options.options().unpack(dict, &my_two_options),
                    shcore::Exception,
                    "Invalid options: myIndirect, myInt, myString");

  dict->erase("myBool");
  dict->erase("myList");
  my_one_options.options().unpack(dict, &my_one_options);

  Templated_options &data = my_one_options;
  EXPECT_STREQ("Some Value", data.str.c_str());
  EXPECT_EQ(1, data.integer);
  EXPECT_FALSE(data.boolean);
  EXPECT_STREQ("Modified Indirect Value", data.indirect_str.c_str());
  EXPECT_TRUE(data.list.empty());
  EXPECT_FALSE(data.done_called);

  dict->clear();
  dict->set("myBool", shcore::Value::True());
  dict->set("myList", shcore::Value(array));
  my_two_options.options().unpack(dict, &my_two_options);

  data = my_two_options;
  EXPECT_TRUE(data.str.empty());
  EXPECT_EQ(0, data.integer);
  EXPECT_TRUE(data.boolean);
  EXPECT_TRUE(data.indirect_str.empty());
  EXPECT_FALSE(data.list.empty());
  EXPECT_STREQ("first element", data.list.at(0).c_str());
  EXPECT_TRUE(data.done_called);
}

}  // namespace tests