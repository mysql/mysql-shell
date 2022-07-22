/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

/*
 * python_contex.h includes Python.h so it needs to be the first include to
 * avoid error: "_POSIX_C_SOURCE" redefined
 */
#include "scripting/python_context.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "gtest_clean.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "scripting/common.h"
#include "scripting/lang_base.h"
#include "scripting/obj_date.h"
#include "scripting/object_registry.h"
#include "scripting/python_utils.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

#include "scripting/python_array_wrapper.h"
#include "test_utils.h"
#include "utils/utils_string.h"

using namespace shcore;

/*extern void Python_context_init();
extern void Python_context_finish();*/

class Test_object : public shcore::Cpp_object_bridge {
 public:
  int _value;

  Test_object(int v) : _value(v) {}

  virtual std::string class_name() const { return "Test"; }

  virtual std::string &append_descr(std::string &s_out, int UNUSED(indent) = -1,
                                    int UNUSED(quote_strings) = 0) const {
    s_out.append(str_format("<Test:%d>", _value));
    return s_out;
  }

  virtual std::string &append_repr(std::string &s_out) const {
    return append_descr(s_out);
  }

  //! Returns the list of members that this object has
  virtual std::vector<std::string> get_members() const {
    std::vector<std::string> l = shcore::Cpp_object_bridge::get_members();
    return l;
  }

  //! Implements equality operator
  virtual bool operator==(const Object_bridge &other) const {
    if (class_name() == other.class_name())
      return _value == ((Test_object *)&other)->_value;
    return false;
  }

  //! Returns the value of a member
  virtual shcore::Value get_member(const std::string &prop) const {
    if (prop == "value")
      return shcore::Value(_value);
    else if (prop == "constant")
      return shcore::Value("BLA");
    return shcore::Cpp_object_bridge::get_member(prop);
  }

  //! Sets the value of a member
  virtual void set_member(const std::string &prop, shcore::Value value) {
    if (prop == "value")
      _value = value.as_int();
    else
      shcore::Cpp_object_bridge::set_member(prop, value);
  }

  //! Calls the named method with the given args
  virtual shcore::Value call(const std::string &name,
                             const shcore::Argument_list &args) {
    return Cpp_object_bridge::call(name, args);
  }
};

namespace shcore {
namespace tests {
class Python : public ::testing::Test {
 public:
  Python()
      : m_options{std::make_shared<mysqlsh::Shell_options>(0, nullptr)},
        m_console{
            std::make_shared<mysqlsh::Shell_console>(&output_handler.deleg)} {
    py = new Python_context(false);
  }

  static void print(void *, const char *text) { std::cout << text << "\n"; }

  ~Python() { delete py; }

  Shell_test_output_handler output_handler;
  Python_context *py;

 private:
  mysqlsh::Scoped_shell_options m_options;
  mysqlsh::Scoped_console m_console;
};

TEST_F(Python, simple_to_py_and_back) {
  ASSERT_TRUE(py);
  WillEnterPython lock;

  {
    Value v(Value::True());
    ASSERT_EQ(py->convert(py->convert(v).get()), v);
  }
  {
    Value v(Value::False());
    ASSERT_EQ(py->convert(py->convert(v).get()), v);
  }
  {
    Value v(Value(1234));
    ASSERT_EQ(py->convert(py->convert(v).get()), v);
  }
  {
    Value v(Value(1234));
    ASSERT_EQ(py->convert(py->convert(v).get()).repr(), "1234");
  }
  {
    Value v(Value("hello"));
    ASSERT_EQ(py->convert(py->convert(v).get()), v);
  }
  {
    Value v(Value(123.45));
    ASSERT_EQ(py->convert(py->convert(v).get()), v);
  }
  {
    Value v(Value::Null());
    ASSERT_EQ(py->convert(py->convert(v).get()), v);
  }

  {
    Value v1(Value(123));
    Value v2(Value(1234));
    ASSERT_NE(py->convert(py->convert(v1).get()), v2);
  }
  {
    Value v1(Value(123));
    Value v2(Value("123"));
    ASSERT_NE(py->convert(py->convert(v1).get()), v2);
  }
}

TEST_F(Python, basic) {
  ASSERT_TRUE(py);

  WillEnterPython lock;
  Input_state cont = Input_state::Ok;

  Value result = py->execute_interactive("'hello world'", cont);
  ASSERT_EQ(result.repr(), "\"hello world\"");

  result = py->execute_interactive("1+1", cont);
  ASSERT_EQ(result.as_int(), 2);

  result = py->execute("");
  ASSERT_EQ(result, Value::Null());

  result = py->execute("1+1+");
  ASSERT_EQ(result, Value());

  // Test lower bigint, handled as Integer
  py->execute_interactive("a = -9223372036854775808", cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.repr(), "-9223372036854775808");
  ASSERT_EQ(result.type, shcore::Value_type::Integer);

  // Test bigger bigint, handled as Integer
  py->execute_interactive("a = 9223372036854775807", cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.repr(), "9223372036854775807");
  ASSERT_EQ(result.type, shcore::Value_type::Integer);

  // Test bigger unsiged bigint, handled as UInteger
  py->execute_interactive("a = 18446744073709551615", cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.repr(), "18446744073709551615");
  ASSERT_EQ(result.type, shcore::Value_type::UInteger);

  // Test a bigger number than bigger unsiged bigint
  // (bigger than the unsigned long long range),
  // handled as String representation
  py->execute_interactive("a = 18446744073709551615999", cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.repr(), "\"18446744073709551615999\"");
  ASSERT_EQ(result.type, shcore::Value_type::String);

  py->execute_interactive("a = -1", cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.value.i, -1);
  ASSERT_EQ(result.type, shcore::Value_type::Integer);

  py->execute_interactive("a = +1", cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.value.i, 1);
  ASSERT_EQ(result.type, shcore::Value_type::Integer);

  py->execute_interactive("a = -10", cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.value.i, -10);
  ASSERT_EQ(result.type, shcore::Value_type::Integer);

  py->execute_interactive("a = +10", cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.value.i, 10);
  ASSERT_EQ(result.type, shcore::Value_type::Integer);

  py->execute_interactive("a = 0", cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.value.i, 0);
  ASSERT_EQ(result.type, shcore::Value_type::Integer);

  std::stringstream s;
  s << "a = " << std::numeric_limits<uint64_t>::max();
  py->execute_interactive(s.str(), cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.value.ui, std::numeric_limits<uint64_t>::max());
  ASSERT_EQ(result.type, shcore::Value_type::UInteger);

  s.str("");
  s << "a = " << std::numeric_limits<int64_t>::min();
  py->execute_interactive(s.str(), cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.value.i, std::numeric_limits<int64_t>::min());
  ASSERT_EQ(result.type, shcore::Value_type::Integer);

  s.str("");
  s << "a = " << std::numeric_limits<int64_t>::max();
  py->execute_interactive(s.str(), cont);
  result = py->execute_interactive("a", cont);
  ASSERT_EQ(result.value.ui, std::numeric_limits<int64_t>::max());
  ASSERT_EQ(result.type, shcore::Value_type::Integer);
}

TEST_F(Python, globals) {
  ASSERT_TRUE(py);

  WillEnterPython lock;
  py->set_global("testglobal", Value(12345));
  ASSERT_EQ(py->get_global("testglobal").repr(), "12345");
}

TEST_F(Python, array_to_py) {
  std::error_code error;
  std::shared_ptr<Value::Array_type> arr2(new Value::Array_type);
  arr2->push_back(Value(444));

  std::shared_ptr<Value::Array_type> arr(new Value::Array_type);
  arr->push_back(Value(123));
  arr->push_back(Value("text"));
  arr->push_back(Value(arr2));

  Value v(arr);
  Input_state cont = Input_state::Ok;
  WillEnterPython lock;
  // this will also test conversion of a wrapped array
  ASSERT_EQ(py->convert(py->convert(v).get()).repr(), "[123, \"text\", [444]]");

  ASSERT_EQ(py->convert(py->convert(v).get()), v);

  py->set_global("arr", v);
  ASSERT_EQ(py->get_global("arr").repr(), "[123, \"text\", [444]]");

  ASSERT_EQ(py->execute_interactive("arr[0]", cont).repr(), Value(123).repr());

  // TODO:    py->execute("foo = shell.List()", error);
  // TODO:    py->execute("shell.List.insert(0, \"1\")", error);

  // this forces conversion of a native Python list into a Value
  ASSERT_EQ(py->execute_interactive("[1,2,3]", cont).repr(), "[1, 2, 3]");
}

TEST_F(Python, map_to_py) {
  std::error_code error;
  std::shared_ptr<Value::Map_type> map2(new Value::Map_type);
  (*map2)["submap"] = Value(444);

  std::shared_ptr<Value::Map_type> map(new Value::Map_type);
  (*map)["k1"] = Value(123);
  (*map)["k2"] = Value("text");
  (*map)["k3"] = Value(map2);

  Value v(map);

  WillEnterPython lock;

  // this will also test conversion of a wrapped array
  ASSERT_EQ(py->convert(py->convert(v).get()).repr(),
            "{\"k1\": 123, \"k2\": \"text\", \"k3\": {\"submap\": 444}}");

  ASSERT_EQ(py->convert(py->convert(v).get()), v);

  py->set_global("mapval", v);
  ASSERT_EQ(py->get_global("mapval").repr(),
            "{\"k1\": 123, \"k2\": \"text\", \"k3\": {\"submap\": 444}}");

  Input_state cont = Input_state::Ok;

  // test enumerator
  ASSERT_EQ("[\"k1\", \"k2\", \"k3\"]",
            py->execute_interactive("mapval.keys()", cont).descr(false));

  // test setter
  py->execute_interactive("mapval[\"k4\"] = 'test'", cont);
  ASSERT_EQ((*map).size(), 4);
  ASSERT_EQ((*map)["k4"].descr(false), Value("test").descr(false));

  // this forces conversion of a native JS map into a Value
  shcore::Value result = py->execute_interactive("{\"submap\": 444}", cont);
  ASSERT_EQ(result, Value(map2));

  // BUG29599261 - test if maps are iterable - 'in' keyword
  EXPECT_EQ(false, py->execute_interactive("'k0' in mapval", cont).as_bool());
  EXPECT_EQ(true, py->execute_interactive("'k1' in mapval", cont).as_bool());

  // BUG29599261 - iterate over map
  static constexpr const auto expected = R"*(k1 -> 123
k2 -> text
k3 -> {"submap": 444}
k4 -> test)*";
  output_handler.wipe_all();
  EXPECT_EQ(Value_type::Null,
            py->execute_interactive("for i in mapval:\n"
                                    "  print('%s -> %s' % (i, mapval[i]))",
                                    cont)
                .type);
  MY_EXPECT_STDOUT_CONTAINS(expected);

  // BUG29599261, BUG29702627 - accessing non-existing key should result in
  // KeyError exception
  output_handler.wipe_all();
  EXPECT_EQ(Value_type::Undefined,
            py->execute_interactive("mapval['k0']", cont).type);
  MY_EXPECT_STDERR_CONTAINS("KeyError: 'k0'");

  // BUG#34403275 - accessing non-existing key with get() should return None
  output_handler.wipe_all();
  EXPECT_EQ(Value_type::Null,
            py->execute_interactive("mapval.get('k0')", cont).type);
  MY_EXPECT_STDERR_NOT_CONTAINS("Error");
}

TEST_F(Python, object_to_py) {
  std::error_code error;
  std::shared_ptr<Test_object> obj =
      std::shared_ptr<Test_object>(new Test_object(1234));
  std::shared_ptr<Test_object> obj2 =
      std::shared_ptr<Test_object>(new Test_object(1234));
  std::shared_ptr<Test_object> obj3 =
      std::shared_ptr<Test_object>(new Test_object(123));

  ASSERT_EQ(*obj, *obj2);
  ASSERT_EQ(Value(std::static_pointer_cast<Object_bridge>(obj)),
            Value(std::static_pointer_cast<Object_bridge>(obj2)));
  ASSERT_NE(*obj, *obj3);

  WillEnterPython lock;
  {
    auto tmp = py->convert(Value(std::static_pointer_cast<Object_bridge>(obj)));
    ASSERT_EQ(Value(std::static_pointer_cast<Object_bridge>(obj2)),
              py->convert(tmp.get()));
  }

  // expose the object to JS
  // py->set_global("test_obj",
  // Value(std::static_pointer_cast<Object_bridge>(obj)));

  // ASSERT_EQ(py->execute_interactive("type(test_obj)").descr(false),
  // "m.Test");

  // test getting member from obj
  // ASSERT_EQ(py->execute_interactive("test_obj.constant").descr(false),
  // "BLA");

  // test setting member of obj
  // py->execute_interactive("test_obj.value=42");

  // ASSERT_EQ(obj->_value, 42);
}

shcore::Value do_tests(const Argument_list &args) {
  args.ensure_count(1, "do_tests");
  return Value(str_upper(args.string_at(0).c_str()));
}

TEST_F(Python, function_to_py) {
  std::error_code error;
  std::shared_ptr<Function_base> func(Cpp_function::create(
      "do_tests", std::bind(do_tests, _1), {{"bla", String}}));

  shcore::Value v(func);
  shcore::Value v2(func);

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
  ASSERT_THROW((v == v2), Exception);
#pragma clang diagnostic pop
#else
  ASSERT_THROW((v == v2), Exception);
#endif

  WillEnterPython lock;

  py->set_global("test_func", v);

  /*  TODO: disabled due to PyRun_String issue
  ASSERT_EQ(py->execute("type(test_func)").descr(false), "m.Function");

  ASSERT_EQ(py->execute("test_func('hello')").descr(false), "HELLO");

  ASSERT_THROW(py->execute("test_func(123)"), shcore::Exception);
  */
}

TEST_F(Python, datetime_to_py) {
  std::shared_ptr<shcore::Date> date(
      new shcore::Date(2000, 1, 1, 12, 23, 35, 526));

  Value v(date);
  Input_state cont = Input_state::Ok;
  WillEnterPython lock;
  // this will also test conversion of a wrapped array
  ASSERT_EQ(py->convert(py->convert(v).get()).repr(), v.repr());

  ASSERT_EQ(py->convert(py->convert(v).get()), v);

  py->set_global("gdate", v);
  ASSERT_EQ(py->get_global("gdate").repr(), v.repr());

  ASSERT_EQ(py->execute_interactive("gdate", cont).repr(), v.repr());

  ASSERT_EQ(py->execute_interactive("str(type(gdate))", cont).repr(),
            "\"<class \\'datetime.datetime\\'>\"");

  py->execute_interactive("import datetime", cont);

  auto value = py->execute_interactive(
      "datetime.datetime(2000, 1, 1, 12, 23, 35, 526)", cont);

  ASSERT_EQ(value.as_object()->class_name(), "Date");
  ASSERT_EQ(v, value);

  Value zero(std::make_shared<shcore::Date>(0, 0, 0, 0, 0, 0, 0));
  ASSERT_EQ(py->convert(py->convert(zero).get()), shcore::Value::Null());
}

TEST_F(Python, date_to_py) {
  std::shared_ptr<shcore::Date> date(new shcore::Date(2000, 1, 1));

  Value v(date);
  Input_state cont = Input_state::Ok;
  WillEnterPython lock;
  // this will also test conversion of a wrapped array
  ASSERT_EQ(py->convert(py->convert(v).get()).repr(), v.repr());

  ASSERT_EQ(py->convert(py->convert(v).get()), v);

  py->set_global("gdate", v);
  ASSERT_EQ(py->get_global("gdate").repr(), v.repr());

  ASSERT_EQ(py->execute_interactive("gdate", cont).repr(), v.repr());

  ASSERT_EQ(py->execute_interactive("str(type(gdate))", cont).repr(),
            "\"<class \\'datetime.date\\'>\"");

  py->execute_interactive("import datetime", cont);

  auto value = py->execute_interactive("datetime.date(2000, 1, 1)", cont);

  ASSERT_EQ(value.as_object()->class_name(), "Date");
  ASSERT_EQ(v, value);

  Value zero(std::make_shared<shcore::Date>(0, 0, 0));
  ASSERT_EQ(py->convert(py->convert(zero).get()), shcore::Value::Null());
}

TEST_F(Python, time_to_py) {
  std::shared_ptr<shcore::Date> time1(new shcore::Date(0, 0, 0, 0));
  std::shared_ptr<shcore::Date> time2(new shcore::Date(10, 20, 30, 0));

  Value v1(time1);
  Value v2(time2);
  Input_state cont = Input_state::Ok;
  WillEnterPython lock;
  // this will also test conversion of a wrapped array
  ASSERT_EQ(py->convert(py->convert(v1).get()).repr(), v1.repr());
  ASSERT_EQ(py->convert(py->convert(v2).get()).repr(), v2.repr());

  ASSERT_EQ(py->convert(py->convert(v1).get()), v1);
  ASSERT_EQ(py->convert(py->convert(v2).get()), v2);

  py->set_global("gtime1", v1);
  ASSERT_EQ(py->get_global("gtime1").repr(), v1.repr());

  py->set_global("gtime2", v2);
  ASSERT_EQ(py->get_global("gtime2").repr(), v2.repr());

  ASSERT_EQ(py->execute_interactive("gtime1", cont).repr(), v1.repr());

  ASSERT_EQ(py->execute_interactive("str(type(gtime1))", cont).repr(),
            "\"<class \\'datetime.time\\'>\"");

  py->execute_interactive("import datetime", cont);

  auto value = py->execute_interactive("datetime.time(0, 0, 0)", cont);

  ASSERT_EQ(value.as_object()->class_name(), "Date");
  ASSERT_EQ(v1, value);

  ASSERT_EQ(py->execute_interactive("gtime2", cont).repr(), v2.repr());

  ASSERT_EQ(py->execute_interactive("str(type(gtime2))", cont).repr(),
            "\"<class \\'datetime.time\\'>\"");

  value = py->execute_interactive("datetime.time(10, 20, 30)", cont);

  ASSERT_EQ(value.as_object()->class_name(), "Date");
  ASSERT_EQ(v2, value);
}

}  // namespace tests
}  // namespace shcore
