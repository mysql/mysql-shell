/*
 * Copyright (c) 2014, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "gtest_clean.h"
#include "modules/mod_sys.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/scripting/polyglot/polyglot_context.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "scripting/lang_base.h"
#include "scripting/object_registry.h"
#include "scripting/types.h"
#include "scripting/types/cpp.h"
#include "test_utils.h"
#include "utils/utils_string.h"

using namespace std::placeholders;
using namespace shcore;

class Test_object : public shcore::Cpp_object_bridge {
 public:
  int _value;

  Test_object(int v) : _value(v) {
    add_property("value");
    add_property("constant");
  }

  virtual std::string class_name() const { return "Test"; }

  virtual std::string &append_descr(
      std::string &s_out, [[maybe_unused]] int indent = -1,
      [[maybe_unused]] int quote_strings = 0) const {
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

class Maparray : public shcore::Cpp_object_bridge {
 public:
  std::vector<shcore::Value> values;
  std::map<std::string, int> keys;

  Maparray() {}

  bool is_indexed() const override { return true; }
  std::string class_name() const override { return "MapArray"; }

  std::string &append_descr(
      std::string &s_out, [[maybe_unused]] int indent = -1,
      [[maybe_unused]] int quote_strings = 0) const override {
    s_out.append(str_format("<MapArray>"));
    return s_out;
  }

  std::string &append_repr(std::string &s_out) const override {
    return append_descr(s_out);
  }

  //! Returns the list of members that this object has
  std::vector<std::string> get_members() const override {
    std::vector<std::string> l = shcore::Cpp_object_bridge::get_members();
    for (std::map<std::string, int>::const_iterator i = keys.begin();
         i != keys.end(); ++i)
      l.push_back(i->first);
    return l;
  }

  //! Implements equality operator
  bool operator==([[maybe_unused]] const Object_bridge &other) const override {
    return false;
  }

  //! Returns the value of a member
  shcore::Value get_member(const std::string &prop) const override {
    if (prop == "length")
      return shcore::Value((int)values.size());
    else {
      std::map<std::string, int>::const_iterator it;
      if ((it = keys.find(prop)) != keys.end()) return values[it->second];
    }

    return shcore::Cpp_object_bridge::get_member(prop);
  }

  shcore::Value get_member(size_t index) const override {
    if (index > values.size() - 1) return shcore::Value();
    return values[index];
  }

  size_t length() const override { return values.size(); }

  bool has_member(const std::string &prop) const override {
    return (keys.find(prop) != keys.end()) ||
           shcore::Cpp_object_bridge::has_member(prop) || prop == "length";
  }

  void add_item(const std::string &key, shcore::Value value) {
    values.push_back(value);
    keys[key] = values.size() - 1;
  }
};

namespace shcore {
class Environment {
 public:
  Environment()
      : m_options{std::make_shared<mysqlsh::Shell_options>(0, nullptr)},
        m_console{
            std::make_shared<mysqlsh::Shell_console>(&output_handler.deleg)} {
    js = std::make_shared<polyglot::Polyglot_context>(
        &reg, polyglot::Language::JAVASCRIPT);

    js->set_global("sys", shcore::Value::wrap<mysqlsh::Sys>(
                              std::make_shared<mysqlsh::Sys>(nullptr)));
  }

  ~Environment() {}

  Shell_test_output_handler output_handler;
  Object_registry reg;
  std::shared_ptr<polyglot::Polyglot_context> js;

 private:
  mysqlsh::Scoped_shell_options m_options;
  mysqlsh::Scoped_console m_console;
};

namespace polyglot {
class JavaScript : public ::testing::Test {
 protected:
  Environment env;
};

TEST_F(JavaScript, basic) {
  ASSERT_TRUE(env.js);

  shcore::Value result = env.js->execute("'hello world'").first;
  ASSERT_EQ(result.repr(), "\"hello world\"");

  result = env.js->execute("1+1").first;
  ASSERT_EQ(result.as_int(), 2);
}

TEST_F(JavaScript, globals) {
  ASSERT_TRUE(env.js);

  env.js->set_global("testglobal", Value(12345));
  ASSERT_EQ(env.js->get_global("testglobal").repr(), "12345");
}

TEST_F(JavaScript, simple_to_js_and_back) {
  {
    shcore::Value v(Value::True());
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value::False());
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value(1234));
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value(INT64_C(-9007199254740991)));  // -(2 ^ 53 - 1)
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value(INT64_C(-2147483649)));  // -(2 ^ 31) - 1
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value(INT64_C(-2147483648)));  // -(2 ^ 31)
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value(INT64_C(2147483647)));  // 2 ^ 31 - 1
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value(INT64_C(2147483648)));  // 2 ^ 31
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value(INT64_C(9007199254740991)));  // 2 ^ 53 - 1
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value(1234));
    ASSERT_EQ(env.js->convert(env.js->convert(v)).repr(), "1234");
  }
  {
    shcore::Value v(Value("hello"));
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value(123.45));
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    shcore::Value v(Value::Null());
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }
  {
    // undefined
    shcore::Value v;
    ASSERT_EQ(env.js->convert(env.js->convert(v)), v);
  }

  {
    shcore::Value v1(Value(123));
    shcore::Value v2(Value(1234));
    ASSERT_NE(env.js->convert(env.js->convert(v1)), v2);
  }
  {
    shcore::Value v1(Value(123));
    shcore::Value v2(Value("123"));
    ASSERT_NE(env.js->convert(env.js->convert(v1)), v2);
  }
}

TEST_F(JavaScript, array_to_js) {
  std::shared_ptr<Value::Array_type> arr2(new Value::Array_type);
  arr2->push_back(Value(444));

  std::shared_ptr<Value::Array_type> arr(new Value::Array_type);
  arr->push_back(Value(123));
  arr->push_back(Value("text"));
  arr->push_back(Value(arr2));

  shcore::Value v(arr);

  // this will also test conversion of a wrapped array
  ASSERT_EQ(env.js->convert(env.js->convert(v)).repr(),
            "[123, \"text\", [444]]");

  ASSERT_EQ(env.js->convert(env.js->convert(v)), v);

  // addressing a wrapped shcore::Value array from JS
  env.js->set_global("arr", v);
  ASSERT_EQ(env.js->execute("arr[0]").first.repr(), Value(123).repr());

  ASSERT_EQ(env.js->execute("arr.length").first.repr(), Value(3).repr());

  // tests enumeration
  env.js->execute("type(arr[0])");

  env.js->execute("for (i in arr) { g = i; }");
  // enumerated array keys become strings, this is normal

  // NOTE: In graal, the array proxy does not have an enumerator
  // interface that allows us to return a list of indexes as strings as in V8
  // For example:
  //    for (i in arr) { g = i; print(i); print(type(i)) }
  // Would output:
  //    0Integer1Integer2Integer
  // While in V8 it would output
  //    0String1String2String
  // However, if arr is defined using pure JavaScript (is not a bridged array)
  // it would behave the same as in V8
  ASSERT_EQ(Value(2).repr(), env.js->get_global("g").repr());

  // this forces conversion of a native JS array into a Value
  shcore::Value result = env.js->execute("[1,2,3]").first;
  ASSERT_EQ(result.repr(), "[1, 2, 3]");

  {
    static constexpr const auto expected = R"*(123
text
[
    444
])*";
    Input_state cont = Input_state::Ok;
    env.output_handler.wipe_all();

    EXPECT_EQ(
        Value_type::Undefined,
        env.js->execute_interactive("for (var v of arr) println(v)", &cont)
            .first.get_type());
    env.output_handler.validate_stdout_content(expected, true);
    EXPECT_TRUE(env.output_handler.std_err.empty());
  }
}

TEST_F(JavaScript, map_to_js) {
  std::shared_ptr<Value::Map_type> map2(new Value::Map_type);
  (*map2)["submap"] = Value(444);

  std::shared_ptr<Value::Map_type> map(new Value::Map_type);
  (*map)["k1"] = Value(123);
  (*map)["k2"] = Value("text");
  (*map)["k3"] = Value(map2);

  shcore::Value v(map);

  // this will also test conversion of a wrapped array
  ASSERT_EQ(env.js->convert(env.js->convert(v)).repr(),
            "{\"k1\": 123, \"k2\": \"text\", \"k3\": {\"submap\": 444}}");

  ASSERT_EQ(env.js->convert(env.js->convert(v)), v);

  env.js->set_global("mapval", v);

  // test enumerator
  ASSERT_EQ("[\"k1\", \"k2\", \"k3\"]",
            env.js->execute("Object.keys(mapval)").first.descr(false));

  // test setter
  env.js->execute("mapval[\"k4\"] = 'test'");
  ASSERT_EQ(static_cast<int>((*map).size()), 4);
  ASSERT_EQ((*map)["k4"].descr(false), Value("test").descr(false));

  // this forces conversion of a native JS map into a Value
  shcore::Value result = env.js->execute("a={\"submap\": 444}").first;
  ASSERT_EQ(result, Value(map2));

  result = env.js->execute("'k1' in mapval").first;
  EXPECT_EQ("true", result.descr());

  result = env.js->execute("'bad' in mapval").first;
  EXPECT_EQ("false", result.descr());

  result = env.js->execute("mapval['invalid']").first;
  EXPECT_EQ(shcore::Undefined, result.get_type());

  {
    static constexpr const auto expected = R"*([
    "k1", 
    123
]
[
    "k2", 
    "text"
]
[
    "k3", 
    {
        "submap": 444
    }
]
[
    "k4", 
    "test"
])*";
    Input_state cont = Input_state::Ok;
    env.output_handler.wipe_all();

    EXPECT_EQ(
        Value_type::Undefined,
        env.js->execute_interactive("for (var m of mapval) println(m)", &cont)
            .first.get_type());
    env.output_handler.validate_stdout_content(expected, true);
    EXPECT_TRUE(env.output_handler.std_err.empty());
  }
}

TEST_F(JavaScript, object_to_js) {
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

  ASSERT_EQ(Value(std::static_pointer_cast<Object_bridge>(obj2)),
            env.js->convert(env.js->convert(
                Value(std::static_pointer_cast<Object_bridge>(obj)))));

  // expose the object to JS
  env.js->set_global("test_obj",
                     Value(std::static_pointer_cast<Object_bridge>(obj)));
  ASSERT_EQ(env.js->execute("type(test_obj)").first.descr(false), "m.Test");

  // test getting member from obj
  ASSERT_EQ(env.js->execute("test_obj.constant").first.descr(false), "BLA");

  // test setting member of obj
  env.js->execute("test_obj.value=42");

  ASSERT_EQ(obj->_value, 42);
}

TEST_F(JavaScript, maparray_to_js) {
  std::shared_ptr<Maparray> obj = std::shared_ptr<Maparray>(new Maparray());

  obj->add_item("one", Value(42));
  obj->add_item("two", Value("hello"));
  obj->add_item("three", Value::Null());

  // expose the object to JS
  env.js->set_global("test_ma",
                     Value(std::static_pointer_cast<Object_bridge>(obj)));
  ASSERT_EQ(env.js->execute("type(test_ma)").first.descr(false), "m.MapArray");

  ASSERT_EQ(env.js->execute("test_ma.length").first.descr(false), "3");
  ASSERT_EQ(env.js->execute("test_ma[0]").first.descr(false), "42");
  ASSERT_EQ(env.js->execute("test_ma[1]").first.descr(false), "hello");
  ASSERT_EQ(env.js->execute("test_ma[2]").first.descr(false), "null");

  ASSERT_EQ(env.js->execute("test_ma['one']").first.descr(false), "42");
  ASSERT_EQ(env.js->execute("test_ma['two']").first.descr(false), "hello");
  ASSERT_EQ(env.js->execute("test_ma['three']").first.descr(false), "null");
}

shcore::Value do_test(const Argument_list &args) {
  args.ensure_count(1, "do_test");
  return Value(str_upper(args.string_at(0)));
}

TEST_F(JavaScript, function_to_js) {
  std::shared_ptr<Function_base> func(Cpp_function::create(
      "do_test", std::bind(do_test, _1), {{"bla", String}}));

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

  env.js->set_global("test_func", v);

  std::string expected_type = "m.Function";

  ASSERT_EQ(env.js->execute("type(test_func)").first.descr(false),
            expected_type.c_str());

  ASSERT_EQ(env.js->execute("test_func('hello')").first.descr(false), "HELLO");

  ASSERT_THROW(env.js->execute("test_func(123)"), shcore::Exception);

  env.js->execute("function js_function() { return 'it_works';}");

  ASSERT_EQ(env.js->execute("type(js_function)").first.descr(false),
            "Function");

  ASSERT_EQ(env.js->execute("js_function()").first.descr(false), "it_works");
}

TEST_F(JavaScript, builtin_functions) {
  ASSERT_EQ(
      env.js->execute("repr(unrepr(repr(\"hello world\")))").first.descr(false),
      "\"hello world\"");
}

TEST_F(JavaScript, js_date_object) {
  shcore::Value object = env.js->execute("new Date(2014,0,1)").first;

  ASSERT_EQ(Object, object.get_type());
  ASSERT_TRUE(object.as_object()->class_name() == "Date");
  ASSERT_EQ("\"2014-01-01 00:00:00\"", object.repr());
}
}  // namespace polyglot
}  // namespace shcore
