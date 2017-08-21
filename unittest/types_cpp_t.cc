/* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.

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
#include <iostream>
#include <limits>
#include <string>
#include "unittest/gtest_clean.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "unittest/test_utils.h"

namespace shcore {

class Test_object : public Cpp_object_bridge {
 public:
  Test_object() {}

  virtual std::string class_name() const { return "Test_object"; }

  void do_expose() {
    expose("f_i_v", &Test_object::f_i_v);
    expose("f_ui_v", &Test_object::f_ui_v);
    expose("f_i64_v", &Test_object::f_i64_v);
    expose("f_u64_v", &Test_object::f_u64_v);
    expose("f_b_v", &Test_object::f_b_v);
    expose("f_s_v", &Test_object::f_s_v);
    expose("f_o_v", &Test_object::f_o_v);
    expose("f_d_v", &Test_object::f_d_v);
    expose("f_f_v", &Test_object::f_f_v);
    expose("f_m_v", &Test_object::f_m_v);
    expose("f_a_v", &Test_object::f_a_v);
    expose("f_s_i", &Test_object::f_s_i, "iarg");
    expose("f_s_u", &Test_object::f_s_u, "uarg");
    expose("f_s_i64", &Test_object::f_s_i64, "iarg");
    expose("f_s_u64", &Test_object::f_s_u64, "uarg");
    expose("f_s_b", &Test_object::f_s_b, "barg");
    expose("f_s_a", &Test_object::f_s_a, "iarg");
    expose("f_s_s", &Test_object::f_s_s, "sarg");
    expose("f_s_d", &Test_object::f_s_d, "darg");
    expose("f_s_o", &Test_object::f_s_o, "oarg");
    expose("f_o_o", &Test_object::f_o_o, "oarg");
    expose("f_s_D", &Test_object::f_s_D, "darg");
    expose("f_s_is", &Test_object::f_s_is, "iarg", "sarg");
    expose("f_s_isd", &Test_object::f_s_isd, "iarg", "sarg", "darg");
    expose("f_s_ii_op", &Test_object::f_s_ii_op, "iarg1", "?iarg2", 111);
  }

  void do_expose_optionals() {
    expose("test1", &Test_object::f_s_isd, "i", "s", "?d",
           shcore::Dictionary_t(), "bla", -1);
    expose("test2", &Test_object::f_s_isd, "i", "?s", "?d",
           shcore::Dictionary_t(), "bla", -1);
    expose("test3", &Test_object::f_s_isd, "?i", "?s", "?d",
           shcore::Dictionary_t(), "bla", -1);
  }

  void do_expose_optional_bad1() {
    expose("test1", &Test_object::f_s_isd, "?i", "s", "?d",
           shcore::Dictionary_t(), "bla", -1);
  }

  void do_expose_optional_bad2() {
    expose("test2", &Test_object::f_s_isd, "?i", "?s", "d",
           shcore::Dictionary_t(), "bla", -1);
  }

  void do_expose_thrower() {
    expose("throw_runtime", &Test_object::f_throw_runtime);
    expose("throw_argument", &Test_object::f_throw_argument);
  }

  void do_expose_overloaded() {
    expose("overload", &Test_object::f_overload);
    expose<int, int>("overload", &Test_object::f_overload, "i");
    expose<int, int, const std::string &>("overload", &Test_object::f_overload,
                                          "i", "?s");
    expose<int, int, int>("overload", &Test_object::f_overload, "i", "ii");
    expose<int, const std::string &>("overload", &Test_object::f_overload, "a");
    expose<int, shcore::Dictionary_t>("overload", &Test_object::f_overload,
                                      "a");
  }

  int f_i_v() { return std::numeric_limits<int>::min(); }

  unsigned int f_ui_v() { return std::numeric_limits<unsigned int>::max(); }

  int64_t f_i64_v() { return std::numeric_limits<int64_t>::min(); }

  uint64_t f_u64_v() { return std::numeric_limits<uint64_t>::max(); }

  bool f_b_v() { return true; }

  std::string f_s_v() { return "foo"; }

  std::shared_ptr<Test_object> f_o_v() {
    return std::shared_ptr<Test_object>(new Test_object());
  }

  double f_d_v() { return 1.234567890123456; }

  float f_f_v() { return 1.234567f; }

  shcore::Dictionary_t f_m_v() {
    shcore::Dictionary_t dict = shcore::make_dict();
    (*dict)["bla"] = shcore::Value(1234);
    return dict;
  }

  shcore::Array_t f_a_v() {
    shcore::Array_t array = shcore::make_array();
    array->push_back(shcore::Value("1234"));
    return array;
  }

  std::string f_s_i(int i) { return std::to_string(i); }

  std::string f_s_u(unsigned int i) { return std::to_string(i); }

  std::string f_s_d(double i) { return std::to_string(i); }

  std::string f_s_i64(int64_t i) { return std::to_string(i); }

  std::string f_s_u64(uint64_t i) { return std::to_string(i); }

  std::string f_s_b(bool b) { return b ? "T" : "F"; }

  std::string f_s_s(const std::string &s) { return s; }

  std::string f_s_D(shcore::Dictionary_t d) { return (*d)["bla"].as_string(); }

  std::string f_s_a(shcore::Array_t a) {
    if (a)
      return (*a)[0].repr();
    return "null";
  }

  std::string f_s_o(std::shared_ptr<Test_object> obj) { return "ble"; }

  std::shared_ptr<Test_object> f_o_o(std::shared_ptr<Test_object> obj) {
    return obj;
  }

  std::string f_s_is(int i, const std::string &s) {
    return s + std::to_string(i);
  }

  std::string f_s_isd(int i, const std::string &s, shcore::Dictionary_t dict) {
    return s + std::to_string(i) +
           (dict.get() ? std::to_string(dict->size()) : "NULL");
  }

  std::string f_s_ii_op(int i, int j) { return std::to_string(i + j); }

  int f_throw_argument() { throw std::invalid_argument("argument"); }

  int f_throw_runtime() { throw std::runtime_error("runtime"); }

  int f_overload() { return 10; }

  int f_overload(int a) { return 11; }

  int f_overload_dup(int a) { return 12; }

  int f_overload(int a, int b) { return 13; }

  int f_overload(const std::string &a) { return 14; }

  int f_overload(shcore::Dictionary_t dict) { return 15; }

  int f_overload(int a, const std::string &b) { return 121; }

  float f_overload_bad(int a) { return 16; }

  std::string f_overload_bad2(int a, int b, int c) { return "17"; }

  int f_overload2(int a) { return 18; }

  FRIEND_TEST(Types_cpp, arg_check_overload);
  FRIEND_TEST(Types_cpp, arg_check_overload_ambiguous);
};

class Types_cpp : public ::testing::Test {
  virtual void SetUp() {}

  virtual void TearDown() { Cpp_object_bridge::clear_metadata(); }

 public:
  Test_object obj;
};

shcore::Argument_list make_args() {
  return shcore::Argument_list();
}

template <typename A1>
shcore::Argument_list make_args(A1 a) {
  shcore::Argument_list args;
  args.push_back(shcore::Value(a));
  return args;
}

template <typename A1, typename A2>
shcore::Argument_list make_args(A1 a1, A2 a2) {
  shcore::Argument_list args;
  args.push_back(shcore::Value(a1));
  args.push_back(shcore::Value(a2));
  return args;
}

template <typename A1, typename A2, typename A3>
shcore::Argument_list make_args(A1 a1, A2 a2, A3 a3) {
  shcore::Argument_list args;
  args.push_back(shcore::Value(a1));
  args.push_back(shcore::Value(a2));
  args.push_back(shcore::Value(a3));
  return args;
}

TEST_F(Types_cpp, expose_float) {
  EXPECT_EQ(1, obj.get_members().size());
  obj.do_expose();
  EXPECT_EQ(26, obj.get_members().size());
  EXPECT_DOUBLE_EQ(Value(obj.f_f_v()).as_double(),
                   obj.call("f_f_v", make_args()).as_double());
  //EXPECT_EQ(obj.f_f_v(), obj.call("f_f_v", make_args()).as_double());
}

TEST_F(Types_cpp, expose) {
  EXPECT_EQ(1, obj.get_members().size());
  obj.do_expose();
  EXPECT_EQ(26, obj.get_members().size());

  EXPECT_EQ(obj.f_i_v(), obj.call("f_i_v", make_args()).as_int());
  EXPECT_EQ(obj.f_ui_v(), obj.call("f_ui_v", make_args()).as_uint());
  EXPECT_EQ(obj.f_i64_v(), obj.call("f_i64_v", make_args()).as_int());
  EXPECT_EQ(obj.f_u64_v(), obj.call("f_u64_v", make_args()).as_uint());
  EXPECT_EQ(obj.f_b_v(), obj.call("f_b_v", make_args()).as_bool());
  EXPECT_EQ(obj.f_s_v(), obj.call("f_s_v", make_args()).as_string());
  EXPECT_TRUE(obj.call("f_o_v", make_args()).as_object<Test_object>().get() !=
              nullptr);
  EXPECT_EQ(obj.f_d_v(), obj.call("f_d_v", make_args()).as_double());
  EXPECT_EQ(*obj.f_m_v(), *obj.call("f_m_v", make_args()).as_map());
  EXPECT_EQ(*obj.f_a_v(), *obj.call("f_a_v", make_args()).as_array());

  EXPECT_EQ(obj.f_s_i(42), obj.call("f_s_i", make_args(42)).as_string());
  EXPECT_EQ(obj.f_s_u(42U), obj.call("f_s_u", make_args(42U)).as_string());
  EXPECT_EQ(obj.f_s_i64(-42), obj.call("f_s_i64", make_args(-42)).as_string());
  EXPECT_EQ(obj.f_s_u64(42U), obj.call("f_s_u64", make_args(42U)).as_string());
  EXPECT_EQ(obj.f_s_d(0.1234543),
            obj.call("f_s_d", make_args(0.1234543)).as_string());
  EXPECT_EQ(obj.f_s_b(true), obj.call("f_s_b", make_args(true)).as_string());
  EXPECT_EQ(obj.f_s_s("bla"), obj.call("f_s_s", make_args("bla")).as_string());
  shcore::Array_t array = shcore::make_array();
  array->push_back(shcore::Value(765));
  EXPECT_EQ(obj.f_s_a(array), obj.call("f_s_a", make_args(array)).as_string());
  EXPECT_EQ(obj.f_s_a(shcore::Array_t()),
            obj.call("f_s_a", make_args(shcore::Value::Null())).as_string());
  shcore::Dictionary_t dict = shcore::make_dict();
  (*dict)["bla"] = shcore::Value("hello");
  EXPECT_EQ(obj.f_s_D(dict), obj.call("f_s_D", make_args(dict)).as_string());
  std::shared_ptr<Test_object> o(new Test_object());
  EXPECT_EQ(obj.f_s_o(o), obj.call("f_s_o", make_args(o)).as_string());
  EXPECT_EQ(o.get(),
            obj.call("f_o_o", make_args(o)).as_object<Test_object>().get());
  EXPECT_EQ(obj.f_s_is(50, "bla"),
            obj.call("f_s_is", make_args(50, "bla")).as_string());
  EXPECT_EQ(obj.f_s_isd(22, "xx", dict),
            obj.call("f_s_isd", make_args(22, "xx", dict)).as_string());
  EXPECT_EQ(obj.f_s_ii_op(32, 111),
            obj.call("f_s_ii_op", make_args(32)).as_string());
  EXPECT_NE(obj.f_s_ii_op(32, 112),
            obj.call("f_s_ii_op", make_args(32)).as_string());
  EXPECT_EQ(obj.f_s_ii_op(32, 20),
            obj.call("f_s_ii_op", make_args(32, 20)).as_string());
}

TEST_F(Types_cpp, arg_check_optional) {
  // Check a1, ?a2, a3 and other combinations

  obj.do_expose_optionals();

  EXPECT_EQ(obj.f_s_isd(42, "foo", shcore::Dictionary_t()),
            obj.call("test1", make_args(42, "foo")).as_string());

  EXPECT_EQ(obj.f_s_isd(42, "foo", shcore::Dictionary_t()),
            obj.call("test1", make_args(42.0, "foo")).as_string());
  EXPECT_EQ(obj.f_s_isd(42, "foo", shcore::Dictionary_t()),
            obj.call("test1", make_args(42U, "foo")).as_string());
  EXPECT_EQ(obj.f_s_isd(1, "foo", shcore::Dictionary_t()),
            obj.call("test1", make_args(true, "foo")).as_string());
  EXPECT_EQ(obj.f_s_isd(0, "foo", shcore::Dictionary_t()),
            obj.call("test1", make_args(false, "foo")).as_string());

  EXPECT_EQ(obj.f_s_isd(42, "bla", shcore::Dictionary_t()),
            obj.call("test2", make_args(42)).as_string());
  EXPECT_EQ(obj.f_s_isd(42, "foo", shcore::Dictionary_t()),
            obj.call("test2", make_args(42, "foo")).as_string());

  EXPECT_EQ(obj.f_s_isd(-1, "bla", shcore::Dictionary_t()),
            obj.call("test3", make_args()).as_string());
  EXPECT_EQ(obj.f_s_isd(42, "bla", shcore::Dictionary_t()),
            obj.call("test3", make_args(42)).as_string());
  EXPECT_EQ(obj.f_s_isd(42, "foo", shcore::Dictionary_t()),
            obj.call("test3", make_args(42, "foo")).as_string());
}

TEST_F(Types_cpp, arg_check_type_fail) {
  obj.do_expose();
  obj.do_expose_optionals();
  EXPECT_THROW(obj.call("f_s_i", make_args("42")), shcore::Exception);

  EXPECT_THROW(obj.call("test1", make_args(42, 42)), shcore::Exception);
  EXPECT_THROW(obj.call("test1", make_args("foo", 42)), shcore::Exception);
  EXPECT_THROW(obj.call("test1", make_args("foo")), shcore::Exception);

  EXPECT_THROW(obj.call("f_s_i", make_args(shcore::make_array())),
               shcore::Exception);
  EXPECT_THROW(obj.call("f_s_i", make_args(shcore::make_dict())),
               shcore::Exception);
  shcore::Argument_list args;
  args.push_back(shcore::Value::Null());
  EXPECT_THROW(obj.call("f_s_i", args), shcore::Exception);
}

TEST_F(Types_cpp, argc_check_fail) {
  obj.do_expose();
  obj.do_expose_optionals();

  EXPECT_THROW(obj.call("f_s_i", make_args()), shcore::Exception);
  EXPECT_THROW(obj.call("f_s_i", make_args(1, 2)), shcore::Exception);
  EXPECT_THROW(obj.call("test1", make_args(42, "foo", 32)), shcore::Exception);
  EXPECT_THROW(obj.call("test1", make_args(42)), shcore::Exception);
}

TEST_F(Types_cpp, arg_optional_bad) {
  EXPECT_THROW(obj.do_expose_optional_bad1(), std::logic_error);
  EXPECT_THROW(obj.do_expose_optional_bad2(), std::logic_error);
}

TEST_F(Types_cpp, throwing_functions) {
  obj.do_expose_thrower();

  EXPECT_THROW_LIKE(obj.call("throw_argument", make_args()), shcore::Exception,
                    "argument");
  EXPECT_THROW_LIKE(obj.call("throw_runtime", make_args()), shcore::Exception,
                    "runtime");
}

TEST_F(Types_cpp, expose_overload) {
  obj.do_expose_overloaded();

  EXPECT_THROW_LIKE(obj.call("overload", make_args(42)).as_int(),
                    shcore::Exception, "ambiguous");
  EXPECT_EQ(obj.f_overload(), obj.call("overload", make_args()).as_int());
  EXPECT_EQ(obj.f_overload(42, 34),
            obj.call("overload", make_args(42, 34)).as_int());
  EXPECT_EQ(obj.f_overload(42, "bla"),
            obj.call("overload", make_args(42, "bla")).as_int());
}

// check expose() of multiple functions with same name, but diff signatures
TEST_F(Types_cpp, arg_check_overload) {
  obj.do_expose_overloaded();

  // test call of the correct overload by argument types
  EXPECT_EQ(obj.f_overload(), obj.call("overload", make_args()).as_int());
  EXPECT_EQ(obj.f_overload(1, 2),
            obj.call("overload", make_args(1, 2)).as_int());
  EXPECT_EQ(obj.f_overload("a"), obj.call("overload", make_args("a")).as_int());
  EXPECT_EQ(obj.f_overload(shcore::Dictionary_t()),
            obj.call("overload", make_args(shcore::Dictionary_t())).as_int());

  // test call of the correct overload by compatible types
  EXPECT_EQ(obj.f_overload(), obj.call("overload", make_args()).as_int());
  EXPECT_EQ(obj.f_overload(1, 2),
            obj.call("overload", make_args(1.0, 2.0)).as_int());
  shcore::Argument_list args;
  args.push_back(shcore::Value::Null());
  EXPECT_EQ(obj.f_overload(shcore::Dictionary_t()),
            obj.call("overload", args).as_int());

  // test call with bad argument type
  EXPECT_THROW(obj.call("overload", make_args("foo", "bar")), std::exception);

  // invalid overload
  // same function sig twice
  EXPECT_THROW(obj.expose("overload", &Test_object::f_overload_dup, "a"),
               std::exception);

  // ambiguous optional
  try {
    obj.expose<int, int>("overload", &Test_object::f_overload, "?i");
    FAIL() << "Expected exception but didn't get one";
  } catch (std::logic_error) {
  }
}

TEST_F(Types_cpp, arg_check_overload_ambiguous) {
  obj.expose<int, const std::string &>("overload", &Test_object::f_overload,
                                       "?s");
  obj.expose<int, int>("overload", &Test_object::f_overload, "?i");

  EXPECT_EQ(obj.f_overload(""), obj.call("overload", make_args("")).as_int());
  EXPECT_EQ(obj.f_overload(11), obj.call("overload", make_args(11)).as_int());
  EXPECT_THROW_LIKE(obj.call("overload", make_args()), shcore::Exception,
                    "ambiguous");
}
}
