/* Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

namespace shcore
{
  namespace tests {
    TEST(ValueTests, SimpleInt)
    {
      shcore::Value v(20);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::Integer, v.type);
      EXPECT_STREQ("20", mydescr.c_str());
      EXPECT_STREQ("20", myrepr.c_str());

      shcore::Value v2 = Value::parse(myrepr);
      mydescr = v2.descr(true);
      myrepr = v2.repr();
      EXPECT_EQ(shcore::Integer, v2.type);
      EXPECT_STREQ("20", mydescr.c_str());
      EXPECT_STREQ("20", myrepr.c_str());
    }

    TEST(ValueTests, SimpleBool)
    {
      shcore::Value v;

      v = shcore::Value(true);
      EXPECT_EQ(shcore::Bool, v.type);
      EXPECT_STREQ("true", v.descr().c_str());
      EXPECT_STREQ("true", v.repr().c_str());

      v = shcore::Value(false);
      EXPECT_EQ(shcore::Bool, v.type);
      EXPECT_STREQ("false", v.descr().c_str());
      EXPECT_STREQ("false", v.repr().c_str());

      EXPECT_EQ(Value::True().as_bool(), true);

      EXPECT_TRUE(Value::True() == Value::True());
      EXPECT_TRUE(Value::True() == Value(1));
      EXPECT_FALSE(Value::True() == Value(1.1));
      EXPECT_FALSE(Value::True() == Value("1"));
      EXPECT_FALSE(Value::True() == Value::Null());

      EXPECT_TRUE(Value::False() == Value(0));
      EXPECT_FALSE(Value::False() == Value(0.1));
      EXPECT_FALSE(Value::False() == Value("0"));
      EXPECT_FALSE(Value::False() == Value::Null());
    }

    TEST(ValueTests, SimpleDouble)
    {
      EXPECT_EQ(Value(1.1234).as_double(), 1.1234);
    }

    TEST(ValueTests, SimpleString)
    {
      const std::string input("Hello world");
      shcore::Value v(input);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::String, v.type);
      EXPECT_STREQ("Hello world", mydescr.c_str());
      EXPECT_STREQ("\"Hello world\"", myrepr.c_str());
    }

    TEST(ValueTests, ArrayCompare)
    {
      Value arr1(Value::new_array());
      Value arr2(Value::new_array());

      arr1.as_array()->push_back(Value(12345));
      arr2.as_array()->push_back(Value(12345));

      EXPECT_TRUE(arr1 == arr2);
    }

    static Value do_test(const Argument_list &args)
    {
      args.ensure_count(1, 2, "do_test");

      int code = args.int_at(0);
      switch (code)
      {
        case 0:
          // test string_at
          return Value(args.string_at(1));
      }
      return Value::Null();
    }

    TEST(Functions, function_wrappers)
    {
      boost::shared_ptr<Function_base> f(Cpp_function::create("test", do_test,
                                                              "test_index", Integer,
                                                              "test_arg", String,
                                                              NULL));

      {
        Argument_list args;

        args.push_back(Value(1234));
        args.push_back(Value("hello"));
        args.push_back(Value(333));

        EXPECT_THROW(f->invoke(args), Exception);
      }
      {
        Argument_list args;

        args.push_back(Value(0));
        args.push_back(Value("hello"));

        EXPECT_EQ(f->invoke(args), Value("hello"));
      }
    }

    TEST(Parsing, Integer)
    {
      const std::string data = "1984";
      shcore::Value v = shcore::Value::parse(data);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::Integer, v.type);
      EXPECT_STREQ("1984", mydescr.c_str());
      EXPECT_STREQ("1984", myrepr.c_str());
    }

    TEST(Parsing, Float)
    {
      const std::string data = "3.1";
      shcore::Value v = shcore::Value::parse(data);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::Float, v.type);
      double d = boost::lexical_cast<double>(myrepr);
      EXPECT_EQ(3.1, d);
    }

    TEST(Parsing, Bool)
    {
      shcore::Value v;

      const std::string data = "false";
      const std::string data2 = "true";

      v = shcore::Value::parse(data);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::Bool, v.type);
      EXPECT_STREQ("false", mydescr.c_str());
      EXPECT_STREQ("false", myrepr.c_str());

      v = shcore::Value::parse(data2);
      mydescr = v.descr(true);
      myrepr = v.repr();

      EXPECT_EQ(shcore::Bool, v.type);
      EXPECT_STREQ("true", mydescr.c_str());
      EXPECT_STREQ("true", myrepr.c_str());
    }

    TEST(Parsing, Null)
    {
      const std::string data = "undefined";
      shcore::Value v = shcore::Value::parse(data);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::Undefined, v.type);
      EXPECT_STREQ("undefined", mydescr.c_str());
      EXPECT_STREQ("undefined", myrepr.c_str());
    }

    TEST(Parsing, String)
    {
      const std::string data = "\"Hello World\"";
      shcore::Value v = shcore::Value::parse(data);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::String, v.type);
      EXPECT_STREQ("Hello World", mydescr.c_str());
      EXPECT_STREQ("\"Hello World\"", myrepr.c_str());
    }

    TEST(Parsing, StringSingleQuoted)
    {
      const std::string data = "\'Hello World\'";
      shcore::Value v = shcore::Value::parse(data);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::String, v.type);
      EXPECT_STREQ("Hello World", mydescr.c_str());
      EXPECT_STREQ("\"Hello World\"", myrepr.c_str());
    }

    TEST(Parsing, Map)
    {
      const std::string data = "{\"null\" : null, \"false\" : false, \"true\" : true, \"string\" : \"string value\", \"integer\":560, \"nested\": {\"inner\": \"value\"}}";
      shcore::Value v = shcore::Value::parse(data);

      EXPECT_EQ(shcore::Map, v.type);

      Value::Map_type_ref map = v.as_map();

      EXPECT_TRUE(map->has_key("null"));
      EXPECT_EQ(shcore::Null, (*map)["null"].type);
      EXPECT_EQ(NULL, (*map)["null"]);

      EXPECT_TRUE(map->has_key("false"));
      EXPECT_EQ(shcore::Bool, (*map)["false"].type);
      EXPECT_EQ(false, (*map)["false"].as_bool());

      EXPECT_TRUE(map->has_key("true"));
      EXPECT_EQ(shcore::Bool, (*map)["true"].type);
      EXPECT_EQ(true, (*map)["true"].as_bool());

      EXPECT_TRUE(map->has_key("string"));
      EXPECT_EQ(shcore::String, (*map)["string"].type);
      EXPECT_EQ("string value", (*map)["string"].as_string());

      EXPECT_TRUE(map->has_key("integer"));
      EXPECT_EQ(shcore::Integer, (*map)["integer"].type);
      EXPECT_EQ(560, (*map)["integer"].as_int());

      EXPECT_TRUE(map->has_key("nested"));
      EXPECT_EQ(shcore::Map, (*map)["nested"].type);

      Value::Map_type_ref nested = (*map)["nested"].as_map();

      EXPECT_TRUE(nested->has_key("inner"));
      EXPECT_EQ(shcore::String, (*nested)["inner"].type);
      EXPECT_EQ("value", (*nested)["inner"].as_string());
    }

    TEST(Parsing, Array)
    {
      const std::string data = "[450, 450.3, +3.5e-10, \"a string\", [1,2,3], {\"nested\":\"document\"}, true, false, null, undefined]";
      shcore::Value v = shcore::Value::parse(data);

      EXPECT_EQ(shcore::Array, v.type);

      Value::Array_type_ref array = v.as_array();

      EXPECT_EQ(shcore::Integer, (*array)[0].type);
      EXPECT_EQ(450, (*array)[0].as_int());

      EXPECT_EQ(shcore::Float, (*array)[1].type);
      EXPECT_EQ(450.3, (*array)[1].as_double());

      EXPECT_EQ(shcore::Float, (*array)[2].type);
      EXPECT_EQ(3.5e-10, (*array)[2].as_double());

      EXPECT_EQ(shcore::String, (*array)[3].type);
      EXPECT_EQ("a string", (*array)[3].as_string());

      EXPECT_EQ(shcore::Array, (*array)[4].type);
      Value::Array_type_ref inner = (*array)[4].as_array();

      EXPECT_EQ(shcore::Integer, (*inner)[0].type);
      EXPECT_EQ(1, (*inner)[0].as_int());

      EXPECT_EQ(shcore::Integer, (*inner)[1].type);
      EXPECT_EQ(2, (*inner)[1].as_int());

      EXPECT_EQ(shcore::Integer, (*inner)[2].type);
      EXPECT_EQ(3, (*inner)[2].as_int());

      EXPECT_EQ(shcore::Map, (*array)[5].type);
      Value::Map_type_ref nested = (*array)[5].as_map();

      EXPECT_TRUE(nested->has_key("nested"));
      EXPECT_EQ(shcore::String, (*nested)["nested"].type);
      EXPECT_EQ("document", (*nested)["nested"].as_string());

      EXPECT_EQ(shcore::Bool, (*array)[6].type);
      EXPECT_EQ(true, (*array)[6].as_bool());

      EXPECT_EQ(shcore::Bool, (*array)[7].type);
      EXPECT_EQ(false, (*array)[7].as_bool());

      EXPECT_EQ(shcore::Null, (*array)[8].type);
      EXPECT_EQ(NULL, (*array)[8]);

      EXPECT_EQ(shcore::Undefined, (*array)[9].type);
      EXPECT_EQ(NULL, (*array)[9]);
    }
  }
}