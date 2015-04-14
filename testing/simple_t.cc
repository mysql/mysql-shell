
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

      EXPECT_EQ( shcore::Integer, v.type );
      EXPECT_STREQ( "20", mydescr.c_str() );
      EXPECT_STREQ( "20", myrepr.c_str() );

      shcore::Value v2 = Value::parse(myrepr);
      mydescr = v2.descr(true);
      myrepr = v2.repr();
      EXPECT_EQ( shcore::Integer, v2.type );
      EXPECT_STREQ( "20", mydescr.c_str() );
      EXPECT_STREQ( "20", myrepr.c_str() );
    }

    TEST(ValueTests, SimpleBool)
    {
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

    TEST( ValueTests, SimpleString)
    {
      const std::string input( "Hello world" );
      shcore::Value v( input );
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ( shcore::String, v.type );
      EXPECT_STREQ( "Hello world", mydescr.c_str() );
      EXPECT_STREQ( "\"Hello world\"", myrepr.c_str() );
    }

    TEST( ValueTests, ArrayCompare)
    {
      Value arr1(Value::new_array());
      Value arr2(Value::new_array());

      arr1.as_array()->push_back(Value(12345));
      arr2.as_array()->push_back(Value(12345));

      EXPECT_TRUE(arr1==arr2);
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


    TEST( Parsing, Integer )
    {
      const std::string data = "1984";
      shcore::Value v = shcore::Value::parse(data);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ( shcore::Integer, v.type);
      EXPECT_STREQ( "1984", mydescr.c_str() );
      EXPECT_STREQ( "1984", myrepr.c_str() );
    }

    TEST( Parsing, Float )
    {
      const std::string data = "3.1";
      shcore::Value v = shcore::Value::parse(data);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ( shcore::Float, v.type);
      double d = boost::lexical_cast<double>(myrepr);
      EXPECT_EQ( 3.1, d );
    }

    TEST(Parsing, Bool)
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
      shcore::Value v = shcore::Value::parse( data );
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::Undefined, v.type);
      EXPECT_STREQ("Undefined", mydescr.c_str());
      EXPECT_STREQ("Undefined", myrepr.c_str());

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


    // TODO: Parsing Map, Array
  }
}
