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
}}
