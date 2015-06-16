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
#include <boost/pointer_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"
#include "shellcore/object_registry.h"
#include "shellcore/python_context.h"
#include "shellcore/python_utils.h"

#include "shellcore/python_array_wrapper.h"
#include "test_utils.h"

extern void Python_context_init();

class Test_object : public shcore::Cpp_object_bridge
{
public:
  int _value;

  Test_object(int v) : _value(v) {}

  virtual std::string class_name() const { return "Test"; }

  virtual std::string &append_descr(std::string &s_out, int indent=-1, int quote_strings=0) const
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

  class Python : public ::testing::Test
  {
  public:
    Python()
    {
      Python_context_init();

      py = new Python_context(&output_handler.deleg);
    }

    static void print(void *, const char *text)
    {
      std::cout << text << "\n";
    }

    ~Python()
    {
      delete py;
    }

    Shell_test_output_handler output_handler;
    Python_context *py;
  };

  TEST_F(Python, simple_to_py_and_back)
  {
    ASSERT_TRUE(py);

    {
      Value v(Value::True());
      ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)),
                v);
    }
    {
      Value v(Value::False());
      ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)),
                v);
    }
    {
      Value v(Value(1234));
      ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)),
                v);
    }
    {
      Value v(Value(1234));
      ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)).repr(),
                "1234");
    }
    {
      Value v(Value("hello"));
      ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)),
                v);
    }
    {
      Value v(Value(123.45));
      ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)),
                v);
    }
    {
      Value v(Value::Null());
      ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)),
                v);
    }

    {
      Value v1(Value(123));
      Value v2(Value(1234));
      ASSERT_NE(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v1)), v2);
    }
    {
      Value v1(Value(123));
      Value v2(Value("123"));
      ASSERT_NE(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v1)), v2);
    }
  }

  TEST_F(Python, basic)
  {
    ASSERT_TRUE(py);

    boost::system::error_code error;
    WillEnterPython lock;
// TODO: disabled due to PyRun_String issue
    //Value result = py->execute("'hello world'", error);
    //ASSERT_EQ(result.repr(), "\"hello world\"");

    //result = py->execute("1+1", error);
    //ASSERT_EQ(result.as_int(), 2);

    Value result = py->execute("", error);
    ASSERT_EQ(result, Value());

    result = py->execute("1+1+", error);
    ASSERT_EQ(result, Value());
  }

  TEST_F(Python, globals)
  {
    ASSERT_TRUE(py);

    WillEnterPython lock;
    py->set_global("testglobal", Value(12345));
    ASSERT_EQ(py->get_global("testglobal").repr(), "12345");
  }

  TEST_F(Python, array_to_py)
  {
    boost::system::error_code error;
    boost::shared_ptr<Value::Array_type> arr2(new Value::Array_type);
    arr2->push_back(Value(444));

    boost::shared_ptr<Value::Array_type> arr(new Value::Array_type);
    arr->push_back(Value(123));
    arr->push_back(Value("text"));
    arr->push_back(Value(arr2));

    Value v(arr);

    WillEnterPython lock;
    // this will also test conversion of a wrapped array
    ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)).repr(), "[123, \"text\", [444]]");

    ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)), v);

    py->set_global("arr", v);
    ASSERT_EQ(py->get_global("arr").repr(), "[123, \"text\", [444]]");

// TODO: disabled due to PyRun_String issue
    //ASSERT_EQ(py->execute("arr[0]", error).repr(), Value(123).repr());

// TODO:    py->execute("foo = shell.List()", error);
// TODO:    py->execute("shell.List.insert(0, \"1\")", error);


    // this forces conversion of a native Python list into a Value
// TODO: disabled due to PyRun_String issue
    //ASSERT_EQ(py->execute("[1,2,3]", error).repr(), "[1, 2, 3]");
  }

  TEST_F(Python, map_to_py)
  {
    boost::system::error_code error;
    boost::shared_ptr<Value::Map_type> map2(new Value::Map_type);
    (*map2)["submap"] = Value(444);

    boost::shared_ptr<Value::Map_type> map(new Value::Map_type);
    (*map)["k1"] = Value(123);
    (*map)["k2"] = Value("text");
    (*map)["k3"] = Value(map2);

    Value v(map);

    WillEnterPython lock;

    // this will also test conversion of a wrapped array
    ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)).repr(),
              "{\"k1\": 123, \"k2\": \"text\", \"k3\": {\"submap\": 444}}");

    ASSERT_EQ(py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(v)), v);


    py->set_global("mapval", v);
    ASSERT_EQ(py->get_global("mapval").repr(), "{\"k1\": 123, \"k2\": \"text\", \"k3\": {\"submap\": 444}}");

/*  TODO: disabled due to PyRun_String issue
    // test enumerator
    ASSERT_EQ("[\"k1\",\"k2\",\"k3\"]",
              py->execute("Object.keys(mapval)").descr(false));

    // test setter
    py->execute("mapval[\"k4\"] = 'test'", error);
    ASSERT_EQ((*map).size(), 4);
    ASSERT_EQ((*map)["k4"].descr(false), Value("test").descr(false));

    // this forces conversion of a native JS map into a Value
    shcore::Value result = py->execute("a={\"submap\": 444}");
    ASSERT_EQ(result, Value(map2));
*/
  }

  TEST_F(Python, object_to_py)
  {
    boost::system::error_code error;
    boost::shared_ptr<Test_object> obj = boost::shared_ptr<Test_object>(new Test_object(1234));
    boost::shared_ptr<Test_object> obj2 = boost::shared_ptr<Test_object>(new Test_object(1234));
    boost::shared_ptr<Test_object> obj3 = boost::shared_ptr<Test_object>(new Test_object(123));

    ASSERT_EQ(*obj, *obj2);
    ASSERT_EQ(Value(boost::static_pointer_cast<Object_bridge>(obj)), Value(boost::static_pointer_cast<Object_bridge>(obj2)));
    ASSERT_NE(*obj, *obj3);

    WillEnterPython lock;

    ASSERT_EQ(Value(boost::static_pointer_cast<Object_bridge>(obj2)), py->pyobj_to_shcore_value(py->shcore_value_to_pyobj(Value(boost::static_pointer_cast<Object_bridge>(obj)))));

    // expose the object to JS
    py->set_global("test_obj", Value(boost::static_pointer_cast<Object_bridge>(obj)));

/*  TODO: disabled due to PyRun_String issue
    ASSERT_EQ(py->execute("type(test_obj)").descr(false), "m.Test");

    // test getting member from obj
    ASSERT_EQ(py->execute("test_obj.constant").descr(false), "BLA");

    // test setting member of obj
    py->execute("test_obj.value=42");

    ASSERT_EQ(obj->_value, 42);
*/
  }


  shcore::Value do_tests(const Argument_list &args)
  {
    args.ensure_count(1, "do_tests");
    return Value(boost::to_upper_copy(args.string_at(0)));
  }

  TEST_F(Python, function_to_py)
  {
    boost::system::error_code error;
    boost::shared_ptr<Function_base> func(Cpp_function::create("do_tests",
                                                       boost::bind(do_tests, _1), "bla", String, NULL));

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
}}
