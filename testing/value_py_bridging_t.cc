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

      py = new Python_context();
    }

    static void print(void *, const char *text)
    {
      std::cout << text << "\n";
    }

    ~Python()
    {
      delete py;
    }

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

    WillEnterPython lock;
    Value result = py->execute("'hello world'");
    ASSERT_EQ(result.repr(), "\"hello world\"");

    result = py->execute("1+1");
    ASSERT_EQ(result.as_int(), 2);

    result = py->execute("");
    ASSERT_EQ(result, Value());

    result = py->execute("1+1+");
    ASSERT_EQ(result, Value());
  }

  TEST_F(Python, globals)
  {
    ASSERT_TRUE(py);

    WillEnterPython lock;
    py->set_global("testglobal", Value(12345));
    ASSERT_EQ((py->pyobj_to_shcore_value(py->get_global("testglobal"))).repr(), "12345");
  }
}}
