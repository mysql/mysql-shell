
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
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"


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
      const std::string data = "false";
      const std::string data2 = "true";
      shcore::Value v = shcore::Value::parse(data);
      std::string mydescr = v.descr(true);
      std::string myrepr = v.repr();

      EXPECT_EQ(shcore::Integer, v.type);
      EXPECT_STREQ("0", mydescr.c_str());
      EXPECT_STREQ("0", myrepr.c_str());

      v = shcore::Value::parse(data2);
      mydescr = v.descr(true);
      myrepr = v.repr();

      EXPECT_EQ(shcore::Integer, v.type);
      EXPECT_STREQ("1", mydescr.c_str());
      EXPECT_STREQ("1", myrepr.c_str());

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
