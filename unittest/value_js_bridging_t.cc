/* Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.

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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "gtest_clean.h"
#include "modules/mod_sys.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "scripting/common.h"
#include "scripting/jscript_context.h"
#include "scripting/lang_base.h"
#include "scripting/object_registry.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "test_utils.h"
#include "utils/utils_string.h"

using namespace std::placeholders;
using namespace shcore;

extern void JScript_context_init();

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

class Maparray : public shcore::Cpp_object_bridge {
 public:
  std::vector<shcore::Value> values;
  std::map<std::string, int> keys;

  Maparray() {}

  virtual bool is_indexed() const { return true; }
  virtual std::string class_name() const { return "MapArray"; }

  virtual std::string &append_descr(std::string &s_out, int UNUSED(indent) = -1,
                                    int UNUSED(quote_strings) = 0) const {
    s_out.append(str_format("<MapArray>"));
    return s_out;
  }

  virtual std::string &append_repr(std::string &s_out) const {
    return append_descr(s_out);
  }

  //! Returns the list of members that this object has
  virtual std::vector<std::string> get_members() const {
    std::vector<std::string> l = shcore::Cpp_object_bridge::get_members();
    for (std::map<std::string, int>::const_iterator i = keys.begin();
         i != keys.end(); ++i)
      l.push_back(i->first);
    return l;
  }

  //! Implements equality operator
  virtual bool operator==(const Object_bridge &UNUSED(other)) const {
    return false;
  }

  //! Returns the value of a member
  virtual shcore::Value get_member(const std::string &prop) const {
    if (prop == "length")
      return shcore::Value((int)values.size());
    else {
      std::map<std::string, int>::const_iterator it;
      if ((it = keys.find(prop)) != keys.end()) return values[it->second];
    }

    return shcore::Cpp_object_bridge::get_member(prop);
  }

  virtual shcore::Value get_member(size_t index) const {
    if (index > values.size() - 1) return shcore::Value();
    return values[index];
  }

  void add_item(const std::string &key, shcore::Value value) {
    values.push_back(value);
    keys[key] = values.size() - 1;
  }
};

namespace shcore {
namespace tests {
class Environment {
 public:
  Environment()
      : m_options{std::make_shared<mysqlsh::Shell_options>(0, nullptr)},
        m_console{
            std::make_shared<mysqlsh::Shell_console>(&output_handler.deleg)} {
    JScript_context_init();

    js = new JScript_context(&reg);

    js->set_global(
        "sys", shcore::Value::wrap<mysqlsh::Sys>(new mysqlsh::Sys(nullptr)));
  }

  ~Environment() { delete js; }

  Shell_test_output_handler output_handler;
  Object_registry reg;
  JScript_context *js;

 private:
  mysqlsh::Scoped_shell_options m_options;
  mysqlsh::Scoped_console m_console;
};

class JavaScript : public ::testing::Test {
 protected:
  Environment env;
};

TEST_F(JavaScript, basic) {
  ASSERT_TRUE(env.js);

  shcore::Value result = env.js->execute("'hello world'");
  ASSERT_EQ(result.repr(), "\"hello world\"");

  result = env.js->execute("1+1");
  ASSERT_EQ(result.as_int(), 2);
}

TEST_F(JavaScript, globals) {
  ASSERT_TRUE(env.js);

  env.js->set_global("testglobal", Value(12345));
  ASSERT_EQ(env.js->get_global("testglobal").repr(), "12345");
}

TEST_F(JavaScript, simple_to_js_and_back) {
  v8::Isolate::Scope isolate_scope(env.js->isolate());
  v8::HandleScope handle_scope(env.js->isolate());
  v8::TryCatch try_catch;
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(env.js->isolate(), env.js->context()));

  {
    shcore::Value v(Value::True());
    ASSERT_EQ(
        env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
        v);
  }
  {
    shcore::Value v(Value::False());
    ASSERT_EQ(
        env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
        v);
  }
  {
    shcore::Value v(Value(1234));
    ASSERT_EQ(
        env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
        v);
  }
  {
    shcore::Value v(Value(1234));
    ASSERT_EQ(
        env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v))
            .repr(),
        "1234");
  }
  {
    shcore::Value v(Value("hello"));
    ASSERT_EQ(
        env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
        v);
  }
  {
    shcore::Value v(Value(123.45));
    ASSERT_EQ(
        env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
        v);
  }
  {
    shcore::Value v(Value::Null());
    ASSERT_EQ(
        env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
        v);
  }

  {
    shcore::Value v1(Value(123));
    shcore::Value v2(Value(1234));
    ASSERT_NE(
        env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v1)),
        v2);
  }
  {
    shcore::Value v1(Value(123));
    shcore::Value v2(Value("123"));
    ASSERT_NE(
        env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v1)),
        v2);
  }
}

TEST_F(JavaScript, array_to_js) {
  v8::Isolate::Scope isolate_scope(env.js->isolate());
  v8::HandleScope handle_scope(env.js->isolate());
  v8::TryCatch try_catch;
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(env.js->isolate(), env.js->context()));

  std::shared_ptr<Value::Array_type> arr2(new Value::Array_type);
  arr2->push_back(Value(444));

  std::shared_ptr<Value::Array_type> arr(new Value::Array_type);
  arr->push_back(Value(123));
  arr->push_back(Value("text"));
  arr->push_back(Value(arr2));

  shcore::Value v(arr);

  // this will also test conversion of a wrapped array
  ASSERT_EQ(
      env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v))
          .repr(),
      "[123, \"text\", [444]]");

  ASSERT_EQ(
      env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)), v);

  // addressing a wrapped shcore::Value array from JS
  env.js->set_global("arr", v);
  ASSERT_EQ(env.js->execute("arr[0]").repr(), Value(123).repr());

  ASSERT_EQ(env.js->execute("arr.length").repr(), Value(3).repr());

  // tests enumeration
  env.js->execute("type(arr[0])");

  env.js->execute("for (i in arr) { g = i; }");
  // enumrated array keys become strings, this is normal
  ASSERT_EQ(Value("2").repr(), env.js->get_global("g").repr());

  // this forces conversion of a native JS array into a Value
  shcore::Value result = env.js->execute("[1,2,3]");
  ASSERT_EQ(result.repr(), "[1, 2, 3]");
}

TEST_F(JavaScript, map_to_js) {
  v8::Isolate::Scope isolate_scope(env.js->isolate());
  v8::HandleScope handle_scope(env.js->isolate());
  v8::TryCatch try_catch;
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(env.js->isolate(), env.js->context()));

  std::shared_ptr<Value::Map_type> map2(new Value::Map_type);
  (*map2)["submap"] = Value(444);

  std::shared_ptr<Value::Map_type> map(new Value::Map_type);
  (*map)["k1"] = Value(123);
  (*map)["k2"] = Value("text");
  (*map)["k3"] = Value(map2);

  shcore::Value v(map);

  // this will also test conversion of a wrapped array
  ASSERT_EQ(
      env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v))
          .repr(),
      "{\"k1\": 123, \"k2\": \"text\", \"k3\": {\"submap\": 444}}");

  ASSERT_EQ(
      env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)), v);

  env.js->set_global("mapval", v);

  // test enumerator
  ASSERT_EQ("[\"k1\", \"k2\", \"k3\"]",
            env.js->execute("Object.keys(mapval)").descr(false));

  // test setter
  env.js->execute("mapval[\"k4\"] = 'test'");
  ASSERT_EQ(static_cast<int>((*map).size()), 4);
  ASSERT_EQ((*map)["k4"].descr(false), Value("test").descr(false));

  // this forces conversion of a native JS map into a Value
  shcore::Value result = env.js->execute("a={\"submap\": 444}");
  ASSERT_EQ(result, Value(map2));
}

TEST_F(JavaScript, object_to_js) {
  v8::Isolate::Scope isolate_scope(env.js->isolate());
  v8::HandleScope handle_scope(env.js->isolate());
  v8::TryCatch try_catch;
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(env.js->isolate(), env.js->context()));

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
            env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(
                Value(std::static_pointer_cast<Object_bridge>(obj)))));

  // expose the object to JS
  env.js->set_global("test_obj",
                     Value(std::static_pointer_cast<Object_bridge>(obj)));
  ASSERT_EQ(env.js->execute("type(test_obj)").descr(false), "m.Test");

  // test getting member from obj
  ASSERT_EQ(env.js->execute("test_obj.constant").descr(false), "BLA");

  // test setting member of obj
  env.js->execute("test_obj.value=42");

  ASSERT_EQ(obj->_value, 42);
}

TEST_F(JavaScript, maparray_to_js) {
  v8::Isolate::Scope isolate_scope(env.js->isolate());
  v8::HandleScope handle_scope(env.js->isolate());
  v8::TryCatch try_catch;
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(env.js->isolate(), env.js->context()));

  std::shared_ptr<Maparray> obj = std::shared_ptr<Maparray>(new Maparray());

  obj->add_item("one", Value(42));
  obj->add_item("two", Value("hello"));
  obj->add_item("three", Value::Null());

  // expose the object to JS
  env.js->set_global("test_ma",
                     Value(std::static_pointer_cast<Object_bridge>(obj)));
  ASSERT_EQ(env.js->execute("type(test_ma)").descr(false), "m.MapArray");

  ASSERT_EQ(env.js->execute("test_ma.length").descr(false), "3");
  ASSERT_EQ(env.js->execute("test_ma[0]").descr(false), "42");
  ASSERT_EQ(env.js->execute("test_ma[1]").descr(false), "hello");
  ASSERT_EQ(env.js->execute("test_ma[2]").descr(false), "null");

  ASSERT_EQ(env.js->execute("test_ma['one']").descr(false), "42");
  ASSERT_EQ(env.js->execute("test_ma['two']").descr(false), "hello");
  ASSERT_EQ(env.js->execute("test_ma['three']").descr(false), "null");
}

shcore::Value do_test(const Argument_list &args) {
  args.ensure_count(1, "do_test");
  return Value(str_upper(args.string_at(0)));
}

TEST_F(JavaScript, function_to_js) {
  v8::Isolate::Scope isolate_scope(env.js->isolate());
  v8::HandleScope handle_scope(env.js->isolate());
  v8::TryCatch try_catch;
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(env.js->isolate(), env.js->context()));
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

  ASSERT_EQ(env.js->execute("type(test_func)").descr(false), "m.Function");

  ASSERT_EQ(env.js->execute("test_func('hello')").descr(false), "HELLO");

  ASSERT_THROW(env.js->execute("test_func(123)"), shcore::Exception);
}

TEST_F(JavaScript, builtin_functions) {
  v8::Isolate::Scope isolate_scope(env.js->isolate());
  v8::HandleScope handle_scope(env.js->isolate());
  v8::TryCatch try_catch;
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(env.js->isolate(), env.js->context()));

  ASSERT_EQ(env.js->execute("repr(unrepr(repr(\"hello world\")))").descr(false),
            "\"hello world\"");
}

TEST_F(JavaScript, js_date_object) {
  v8::Isolate::Scope isolate_scope(env.js->isolate());
  v8::HandleScope handle_scope(env.js->isolate());
  v8::TryCatch try_catch;
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(env.js->isolate(), env.js->context()));
  shcore::Value object = env.js->execute("new Date(2014,0,1)");

  ASSERT_EQ(Object, object.type);
  ASSERT_TRUE(object.as_object()->class_name() == "Date");
  ASSERT_EQ("\"2014-01-01 00:00:00\"", object.repr());
}
}  // namespace tests
}  // namespace shcore
