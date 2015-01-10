
/* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.

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
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"
#include "shellcore/object_registry.h"
#include "shellcore/jscript_context.h"

extern void JScript_context_init();


class Test_object : public shcore::Cpp_object_bridge
{
public:
  int _value;

  Test_object(int v) : _value(v) {}

  virtual std::string class_name() const { return "Test"; }

  virtual std::string &append_descr(std::string &s_out, int indent=-1, bool quote_strings=false) const
  {
    s_out.append((boost::format("<Test:%1%>") % _value).str());
    return s_out;
  }

  virtual std::string &append_repr(std::string &s_out) const
  {
    return append_descr(s_out);
  }

  //! Returns the list of members that this object has
  virtual std::vector<std::string> get_members() const
  {
    std::vector<std::string> l = shcore::Cpp_object_bridge::get_members();
    return l;
  }

  //! Implements equality operator
  virtual bool operator == (const Object_bridge &other) const
  {
    if (class_name() == other.class_name())
      return _value == ((Test_object*)&other)->_value;
    return false;
  }

  //! Returns the value of a member
  virtual shcore::Value get_member(const std::string &prop) const
  {
    if (prop == "value")
      return shcore::Value(_value);
    else if (prop == "constant")
      return shcore::Value("BLA");
    return shcore::Cpp_object_bridge::get_member(prop);
  }

  //! Sets the value of a member
  virtual void set_member(const std::string &prop, shcore::Value value)
  {
    if (prop == "value")
      _value = value.as_int();
    else
      shcore::Cpp_object_bridge::set_member(prop, value);
  }

  //! Calls the named method with the given args
  virtual shcore::Value call(const std::string &name, const shcore::Argument_list &args)
  {
    return Cpp_object_bridge::call(name, args);
  }
};

namespace shcore {

namespace tests {
  class Environment
  {
  public:
    Environment()
    {
      JScript_context_init();

      static Interpreter_delegate deleg;

      deleg.print = print;

      js = new JScript_context(&reg, &deleg);
    }

    static void print(void *, const char *text)
    {
      std::cout << text << "\n";
    }

    ~Environment()
    {
      delete js;
    }

    Object_registry reg;
    JScript_context *js;
  };
  Environment env;

  TEST(JavaScript, basic)
  {
    ASSERT_TRUE(env.js);

    boost::system::error_code error;
    Value result = env.js->execute("'hello world'", error);
    ASSERT_EQ(result.repr(), "\"hello world\"");

    result = env.js->execute("1+1", error);
    ASSERT_EQ(result.as_int(), 2);
  }

  TEST(JavaScript, globals)
  {
    ASSERT_TRUE(env.js);

    boost::system::error_code error;
    env.js->set_global("testglobal", Value(12345));
    ASSERT_EQ(env.js->get_global("testglobal").repr(), "12345");
  }


  TEST(JavaScript, simple_to_js_and_back)
  {
    v8::Isolate::Scope isolate_scope(env.js->isolate());
    v8::HandleScope handle_scope(env.js->isolate());
    v8::TryCatch try_catch;
    v8::Context::Scope context_scope(v8::Local<v8::Context>::New(env.js->isolate(),
                                                                 env.js->context()));

    {
      Value v(Value::True());
      ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
                v);
    }
    {
      Value v(Value::False());
      ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
                v);
    }
    {
      Value v(Value(1234));
      ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
                v);
    }
    {
      Value v(Value(1234));
      ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)).repr(),
                "1234");
    }
    {
      Value v(Value("hello"));
      ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
                v);
    }
    {
      Value v(Value(123.45));
      ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
                v);
    }
    {
      Value v(Value::Null());
      ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)),
                v);
    }

    {
      Value v1(Value(123));
      Value v2(Value(1234));
      ASSERT_NE(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v1)),
                v2);
    }
    {
      Value v1(Value(123));
      Value v2(Value("123"));
      ASSERT_NE(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v1)),
                v2);
    }
  }

  TEST(JavaScript, array_to_js)
  {
    v8::Isolate::Scope isolate_scope(env.js->isolate());
    v8::HandleScope handle_scope(env.js->isolate());
    v8::TryCatch try_catch;
    v8::Context::Scope context_scope(v8::Local<v8::Context>::New(env.js->isolate(),
                                                                 env.js->context()));

    boost::shared_ptr<Value::Array_type> arr2(new Value::Array_type);
    arr2->push_back(Value(444));

    boost::shared_ptr<Value::Array_type> arr(new Value::Array_type);
    arr->push_back(Value(123));
    arr->push_back(Value("text"));
    arr->push_back(Value(arr2));

    Value v(arr);

    // this will also test conversion of a wrapped array
    ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)).repr(),
              "[123, \"text\", [444]]");

    ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)), v);

    boost::system::error_code error;

    // addressing a wrapped Value array from JS
    env.js->set_global("arr", v);
    ASSERT_EQ(env.js->execute("arr[0]", error).repr(), Value(123).repr());

    ASSERT_EQ(env.js->execute("arr.length", error).repr(), Value(3).repr());

    // tests enumeration
    env.js->execute("type(arr[0])", error);

    env.js->execute("for (i in arr) { g = i; }", error);
    // enumrated array keys become strings, this is normal
    ASSERT_EQ(Value("2").repr(), env.js->get_global("g").repr());

    // this forces conversion of a native JS array into a Value
    Value result = env.js->execute("[1,2,3]", error);
    ASSERT_EQ(result.repr(), "[1, 2, 3]");
  }

  TEST(JavaScript, map_to_js)
  {
    v8::Isolate::Scope isolate_scope(env.js->isolate());
    v8::HandleScope handle_scope(env.js->isolate());
    v8::TryCatch try_catch;
    v8::Context::Scope context_scope(v8::Local<v8::Context>::New(env.js->isolate(),
                                                                 env.js->context()));

    boost::shared_ptr<Value::Map_type> map2(new Value::Map_type);
    (*map2)["submap"] = Value(444);

    boost::shared_ptr<Value::Map_type> map(new Value::Map_type);
    (*map)["k1"] = Value(123);
    (*map)["k2"] = Value("text");
    (*map)["k3"] = Value(map2);

    Value v(map);

    // this will also test conversion of a wrapped array
    ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)).repr(),
              "{\"k1\": 123, \"k2\": \"text\", \"k3\": {\"submap\": 444}}");

    ASSERT_EQ(env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(v)), v);

    boost::system::error_code error;

    env.js->set_global("mapval", v);

    // test enumerator
    ASSERT_EQ("[\"k1\",\"k2\",\"k3\"]",
              env.js->execute("Object.keys(mapval)", error).descr(false));

    // test setter
    env.js->execute("mapval[\"k4\"] = 'test'", error);
    ASSERT_EQ((*map).size(), 4);
    ASSERT_EQ((*map)["k4"].descr(false), Value("test").descr(false));

    // this forces conversion of a native JS map into a Value
    Value result = env.js->execute("a={\"submap\": 444}", error);
    ASSERT_EQ(result, Value(map2));
  }

  TEST(JavaScript, object_to_js)
  {
    v8::Isolate::Scope isolate_scope(env.js->isolate());
    v8::HandleScope handle_scope(env.js->isolate());
    v8::TryCatch try_catch;
    v8::Context::Scope context_scope(v8::Local<v8::Context>::New(env.js->isolate(),
                                                                 env.js->context()));

    boost::shared_ptr<Test_object> obj = boost::shared_ptr<Test_object>(new Test_object(1234));
    boost::shared_ptr<Test_object> obj2 = boost::shared_ptr<Test_object>(new Test_object(1234));
    boost::shared_ptr<Test_object> obj3 = boost::shared_ptr<Test_object>(new Test_object(123));

    ASSERT_EQ(*obj, *obj2);
    ASSERT_EQ(Value(obj), Value(obj2));
    ASSERT_NE(*obj, *obj3);

    ASSERT_EQ(Value(obj2), env.js->v8_value_to_shcore_value(env.js->shcore_value_to_v8_value(Value(obj))));

    boost::system::error_code error;

    // expose the object to JS
    env.js->set_global("test_obj", Value(obj));
    ASSERT_EQ(env.js->execute("type(test_obj)", error).descr(false), "m.Test");

    // test getting member from obj
    ASSERT_EQ(env.js->execute("test_obj.constant", error).descr(false), "BLA");

    // test setting member of obj
    env.js->execute("test_obj.value=42", error);

    ASSERT_EQ(obj->_value, 42);
  }


  Value do_test(const Argument_list &args)
  {
    args.ensure_count(1, "do_test");
    return Value(boost::to_upper_copy(args.string_at(0)));
  }


  TEST(JavaScript, function_to_js)
  {
    v8::Isolate::Scope isolate_scope(env.js->isolate());
    v8::HandleScope handle_scope(env.js->isolate());
    v8::TryCatch try_catch;
    v8::Context::Scope context_scope(v8::Local<v8::Context>::New(env.js->isolate(),
                                                                 env.js->context()));

    boost::shared_ptr<Function_base> func(Cpp_function::create("do_test",
                                                       boost::bind(do_test, _1), "bla", String, NULL));

    Value v(func);
    Value v2(func);

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
    ASSERT_THROW((v == v2), Exception);
#pragma clang diagnostic pop
#else
    ASSERT_THROW((v == v2), Exception);
#endif

    env.js->set_global("test_func", v);

    boost::system::error_code error;

    ASSERT_EQ(env.js->execute("type(test_func)", error).descr(false), "m.Function");

    ASSERT_EQ(env.js->execute("test_func('hello')", error).descr(false), "HELLO");

    ASSERT_THROW(env.js->execute("test_func(123)", error), shcore::Exception);
  }



  TEST(JavaScript, builtin_functions)
  {
    v8::Isolate::Scope isolate_scope(env.js->isolate());
    v8::HandleScope handle_scope(env.js->isolate());
    v8::TryCatch try_catch;
    v8::Context::Scope context_scope(v8::Local<v8::Context>::New(env.js->isolate(),
                                                                 env.js->context()));
    boost::system::error_code error;

    ASSERT_EQ(env.js->execute("repr(unrepr(repr(\"hello world\")))", error).descr(false), "\"hello world\"");
  }


  TEST(JavaScript, js_date_object)
  {
    v8::Isolate::Scope isolate_scope(env.js->isolate());
    v8::HandleScope handle_scope(env.js->isolate());
    v8::TryCatch try_catch;
    v8::Context::Scope context_scope(v8::Local<v8::Context>::New(env.js->isolate(),
                                                                 env.js->context()));
    boost::system::error_code error;

    env.js->execute("dt = new Date(2014,1,1)", error);

    Value v(env.js->get_global("dt"));

//    ASSERT_EQ(type_name(v.type), "Object");
  }

}}
