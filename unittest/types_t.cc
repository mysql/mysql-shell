/* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.

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

#include "gtest_clean.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

using namespace ::testing;

namespace shcore {
namespace tests {
TEST(ValueTests, SimpleInt) {
  shcore::Value v(20);
  std::string mydescr = v.descr(true);
  std::string myrepr = v.repr();
  std::string myjson = v.json();

  EXPECT_EQ(shcore::Integer, v.type);
  EXPECT_STREQ("20", mydescr.c_str());
  EXPECT_STREQ("20", myrepr.c_str());
  EXPECT_STREQ("20", myjson.c_str());

  shcore::Value v2 = Value::parse(myrepr);
  mydescr = v2.descr(true);
  myrepr = v2.repr();
  myjson = v2.json();
  EXPECT_EQ(shcore::Integer, v2.type);
  EXPECT_STREQ("20", mydescr.c_str());
  EXPECT_STREQ("20", myrepr.c_str());
  EXPECT_STREQ("20", myjson.c_str());
}

TEST(ValueTests, SimpleBool) {
  shcore::Value v;

  v = shcore::Value(true);
  EXPECT_EQ(shcore::Bool, v.type);
  EXPECT_STREQ("true", v.descr().c_str());
  EXPECT_STREQ("true", v.repr().c_str());
  EXPECT_STREQ("true", v.json().c_str());

  v = shcore::Value(false);
  EXPECT_EQ(shcore::Bool, v.type);
  EXPECT_STREQ("false", v.descr().c_str());
  EXPECT_STREQ("false", v.repr().c_str());
  EXPECT_STREQ("false", v.json().c_str());

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

TEST(ValueTests, SimpleDouble) {
  EXPECT_EQ(Value(1.1234).as_double(), 1.1234);
}

TEST(ValueTests, SimpleString) {
  const std::string input("Hello world");
  shcore::Value v(input);
  std::string mydescr = v.descr(true);
  std::string myrepr = v.repr();
  std::string myjson = v.json();

  EXPECT_EQ(shcore::String, v.type);
  EXPECT_STREQ("Hello world", mydescr.c_str());
  EXPECT_STREQ("\"Hello world\"", myrepr.c_str());
  EXPECT_STREQ("\"Hello world\"", myjson.c_str());

  shcore::Value v2("Hello / world");
  mydescr = v2.descr(true);
  myrepr = v2.repr();
  myjson = v2.json();

  EXPECT_EQ(shcore::String, v.type);
  EXPECT_STREQ("Hello / world", mydescr.c_str());
  EXPECT_STREQ("\"Hello / world\"", myrepr.c_str());
  EXPECT_STREQ("\"Hello / world\"", myjson.c_str());
}

TEST(ValueTests, ArrayCompare) {
  Value arr1(Value::new_array());
  Value arr2(Value::new_array());

  arr1.as_array()->push_back(Value(12345));
  arr2.as_array()->push_back(Value(12345));

  EXPECT_TRUE(arr1 == arr2);
}

static Value do_test(const Argument_list &args) {
  args.ensure_count(1, 2, "do_test");

  int code = args.int_at(0);
  switch (code) {
    case 0:
      // test string_at
      return Value(args.string_at(1));
  }
  return Value::Null();
}

TEST(Functions, function_wrappers) {
  std::shared_ptr<Function_base> f(Cpp_function::create("test", do_test,
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

TEST(Parsing, Integer) {
  const std::string data = "1984";
  shcore::Value v = shcore::Value::parse(data);
  std::string mydescr = v.descr(true);
  std::string myrepr = v.repr();

  EXPECT_EQ(shcore::Integer, v.type);
  EXPECT_STREQ("1984", mydescr.c_str());
  EXPECT_STREQ("1984", myrepr.c_str());
}

TEST(Parsing, Float) {
  const std::string data = "3.1";
  shcore::Value v = shcore::Value::parse(data);
  std::string mydescr = v.descr(true);
  std::string myrepr = v.repr();

  EXPECT_EQ(shcore::Float, v.type);
  double d = std::stod(myrepr);
  EXPECT_EQ(3.1, d);
}

TEST(Parsing, Bool) {
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

  const std::string data3 = "FALSE";
  const std::string data4 = "TRUE";

  v = shcore::Value::parse(data3);
  mydescr = v.descr(true);
  myrepr = v.repr();

  EXPECT_EQ(shcore::Bool, v.type);
  EXPECT_STREQ("false", mydescr.c_str());
  EXPECT_STREQ("false", myrepr.c_str());

  v = shcore::Value::parse(data4);
  mydescr = v.descr(true);
  myrepr = v.repr();

  EXPECT_EQ(shcore::Bool, v.type);
  EXPECT_STREQ("true", mydescr.c_str());
  EXPECT_STREQ("true", myrepr.c_str());
}

TEST(Parsing, Null) {
  const std::string data = "undefined";
  shcore::Value v = shcore::Value::parse(data);
  std::string mydescr = v.descr(true);
  std::string myrepr = v.repr();

  EXPECT_EQ(shcore::Undefined, v.type);
  EXPECT_STREQ("undefined", mydescr.c_str());
  EXPECT_STREQ("undefined", myrepr.c_str());
}

TEST(Parsing, String) {
  const std::string data = "\"Hello World\"";
  shcore::Value v = shcore::Value::parse(data);
  std::string mydescr = v.descr(true);
  std::string myrepr = v.repr();

  EXPECT_EQ(shcore::String, v.type);
  EXPECT_STREQ("Hello World", mydescr.c_str());
  EXPECT_STREQ("\"Hello World\"", myrepr.c_str());
}

TEST(Parsing, StringSingleQuoted) {
  const std::string data = "\'Hello World\'";
  shcore::Value v = shcore::Value::parse(data);
  std::string mydescr = v.descr(true);
  std::string myrepr = v.repr();

  EXPECT_EQ(shcore::String, v.type);
  EXPECT_STREQ("Hello World", mydescr.c_str());
  EXPECT_STREQ("\"Hello World\"", myrepr.c_str());
}

TEST(Parsing, Map) {
  const std::string data = "{\"null\" : null, \"false\" : false, \"true\" : true, \"string\" : \"string value\", \"integer\":560, \"nested\": {\"inner\": \"value\"}}";
  shcore::Value v = shcore::Value::parse(data);

  EXPECT_EQ(shcore::Map, v.type);

  Value::Map_type_ref map = v.as_map();

  EXPECT_TRUE(map->has_key("null"));
  EXPECT_EQ(shcore::Null, (*map)["null"].type);
  EXPECT_EQ(NULL, (*map)["null"]);

  EXPECT_TRUE(map->has_key("false"));
  EXPECT_EQ(shcore::Bool, (*map)["false"].type);
  EXPECT_FALSE((*map)["false"].as_bool());

  EXPECT_TRUE(map->has_key("true"));
  EXPECT_EQ(shcore::Bool, (*map)["true"].type);
  EXPECT_TRUE((*map)["true"].as_bool());

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

  shcore::Value v2 = shcore::Value::parse("{}");
  EXPECT_EQ(shcore::Map, v.type);
  Value::Map_type_ref map2 = v2.as_map();
  EXPECT_EQ(map2->size(), 0);
}

TEST(Parsing, Array) {
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
  EXPECT_TRUE((*array)[6].as_bool());

  EXPECT_EQ(shcore::Bool, (*array)[7].type);
  EXPECT_FALSE((*array)[7].as_bool());

  EXPECT_EQ(shcore::Null, (*array)[8].type);
  EXPECT_EQ(NULL, (*array)[8]);

  EXPECT_EQ(shcore::Undefined, (*array)[9].type);
  EXPECT_EQ(NULL, (*array)[9]);

  shcore::Value v2 = shcore::Value::parse("[]");
  EXPECT_EQ(shcore::Array, v2.type);
  Value::Array_type_ref array2 = v2.as_array();
  EXPECT_EQ(array2->size(), 0);
}

TEST(Argument_map, all) {
  {
    Argument_map args;

    args["int"] = Value(-1234);
    args["bool"] = Value(true);
    args["uint"] = Value(4321);
    args["str"] = Value("string");
    args["flt"] = Value(1.234);
    args["vec"] = Value::new_array();
    args["map"] = Value::new_map();

    ASSERT_NO_THROW(
      args.ensure_keys({"int", "bool", "uint", "str", "flt", "vec"}, {"map"}, "test1"));

    ASSERT_THROW(
      args.ensure_keys({"int", "bool", "uint", "str", "flt"}, {"map"}, "test2"),
      Exception);

    ASSERT_THROW(
      args.ensure_keys({"int", "bool", "uint", "str", "flt", "bla"}, {"map"}, "test3"),
      Exception);

    ASSERT_NO_THROW(
      args.ensure_keys({"int", "bool", "uint", "str", "flt", "vec"}, {"map", "bla"}, "test2"));

    EXPECT_EQ(args.bool_at("bool"), true);
    EXPECT_EQ(args.bool_at("int"), true);
    EXPECT_EQ(args.bool_at("uint"), true);
    EXPECT_THROW(args.bool_at("str"), Exception);
    EXPECT_EQ(args.bool_at("flt"), true);
    EXPECT_THROW(args.bool_at("vec"), Exception);
    EXPECT_THROW(args.bool_at("map"), Exception);

    EXPECT_EQ(args.int_at("bool"), 1);
    EXPECT_EQ(args.int_at("int"), -1234);
    EXPECT_EQ(args.int_at("uint"), 4321);
    EXPECT_THROW(args.int_at("str"), Exception);
    EXPECT_EQ(args.int_at("flt"), 1);
    EXPECT_THROW(args.int_at("vec"), Exception);
    EXPECT_THROW(args.int_at("map"), Exception);

    EXPECT_EQ(args.uint_at("bool"), 1);
    EXPECT_THROW(args.uint_at("int"), Exception);
    EXPECT_EQ(args.uint_at("uint"), 4321);
    EXPECT_THROW(args.uint_at("str"), Exception);
    EXPECT_EQ(args.uint_at("flt"), 1);
    EXPECT_THROW(args.uint_at("vec"), Exception);
    EXPECT_THROW(args.uint_at("map"), Exception);

    EXPECT_EQ(args.double_at("bool"), 1.0);
    EXPECT_EQ(args.double_at("int"), -1234.0);
    EXPECT_EQ(args.double_at("uint"), 4321.0);
    EXPECT_THROW(args.double_at("str"), Exception);
    EXPECT_EQ(args.double_at("flt"), 1.234);
    EXPECT_THROW(args.double_at("vec"), Exception);
    EXPECT_THROW(args.double_at("map"), Exception);

    EXPECT_THROW(args.string_at("bool"), Exception);
    EXPECT_THROW(args.string_at("int"), Exception);
    EXPECT_THROW(args.string_at("uint"), Exception);
    EXPECT_EQ(args.string_at("str"), "string");
    EXPECT_THROW(args.string_at("flt"), Exception);
    EXPECT_THROW(args.uint_at("vec"), Exception);
    EXPECT_THROW(args.uint_at("map"), Exception);
  }
  
  // option alias, success alias 1
  {
    shcore::Value::Map_type_ref options(new shcore::Value::Map_type());
    (*options)["user"] = shcore::Value("sample");
    (*options)["password"] = shcore::Value("sample");
    
    Argument_map args(*options);
    ASSERT_NO_THROW(
      args.ensure_keys({"password|dbPassword"}, {"user"}, "test1"));
  }
  
  // option alias, success alias 2
  {
    shcore::Value::Map_type_ref options(new shcore::Value::Map_type());
    (*options)["user"] = shcore::Value("sample");
    (*options)["dbPassword"] = shcore::Value("sample");
    
    Argument_map args(*options);
    ASSERT_NO_THROW(
      args.ensure_keys({"password|dbPassword"}, {"user"}, "test1"));
  }
  
  // option alias, failure two aliases
  {
    shcore::Value::Map_type_ref options(new shcore::Value::Map_type());
    (*options)["user"] = shcore::Value("sample");
    (*options)["password"] = shcore::Value("sample");
    (*options)["dbPassword"] = shcore::Value("sample");
    
    Argument_map args(*options);
    
    ASSERT_THROW(
      args.ensure_keys({"password|dbPassword"}, {"user"}, "multiple aliases"),
                 Exception);
  }
  
  // option alias, failure no aliases
  {
    shcore::Value::Map_type_ref options(new shcore::Value::Map_type());
    (*options)["user"] = shcore::Value("sample");
    
    Argument_map args(*options);
    
    ASSERT_THROW(
      args.ensure_keys({"password|dbPassword"}, {"user"}, "no aliases"),
                 Exception);
  }
}

}
}
