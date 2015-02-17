
/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

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

#include <gtest/gtest.h>
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include "../modules/mod_mysql.h"

namespace shcore
{
  namespace mod_mysql_tests
  {
    struct Environment
    {
      boost::shared_ptr<mysh::Mysql_connection> db;
    } env;


    TEST(MySQL, connect)
    {
      Argument_list args;
      const char *uri = getenv("MYSQL_URI");
      const char *pwd = getenv("MYSQL_PWD");
      env.db.reset(new mysh::Mysql_connection(uri && *uri ? uri : "root@localhost", pwd ? pwd : ""));
    }

    TEST(MySQL, query)
    {
      Value result = env.db->sql("select 1 as a", Value());
      Value row = result.as_object<Object_bridge>()->call("next", Argument_list());
      EXPECT_EQ("{\"a\": 1}", row.descr());
      row = result.as_object<Object_bridge>()->call("next", Argument_list());
      EXPECT_TRUE(result.as_object<Object_bridge>()->call("next", Argument_list()) == Value::Null());
    }

    /* not implemented yet
    TEST(MySQL, query_with_binds_int)
    {
      {
        Argument_list args;
        args.push_back(Value("select ? as a, ? as b"));
        Value params(Value::new_array());
        params.as_array()->push_back(Value(42));
        params.as_array()->push_back(Value(43));
        args.push_back(params);

        Value result = env.db->sql(args);
        Value row = result.as_object<Object_bridge>()->call("next", Argument_list());
        EXPECT_EQ("{\"a\": 42, \"b\": 43}", row.descr());
      }

      {
        Argument_list args;
        args.push_back(Value("select :first as a, :second as b"));
        Value params(Value::new_map());
        (*params.as_map())["first"] = Value(42);
        (*params.as_map())["second"] = Value(43);
        args.push_back(params);

        Value result = env.db->sql(args);
        Value row = result.as_object<Object_bridge>()->call("next", Argument_list());
        EXPECT_EQ("{\"a\": 42, \"b\": 43}", row.descr());
      }
    }

    TEST(MySQL, query_with_binds_date)
    {
      {
        Argument_list args;
        args.push_back(Value("select ? as a, ? as b"));
        Value params(Value::new_array());
        params.as_array()->push_back(Value(42));
        params.as_array()->push_back(Value(43));
        args.push_back(params);

        Value result = env.db->sql(args);
        Value row = result.as_object<Object_bridge>()->call("next", Argument_list());
        EXPECT_EQ("{\"a\": 42, \"b\": 43}", row.descr());
      }

      {
        Argument_list args;
        args.push_back(Value("select :first as 1, :second as b"));
        Value params(Value::new_map());
        (*params.as_map())["first"] = Value(42);
        (*params.as_map())["second"] = Value(43);
        args.push_back(params);

        Value result = env.db->sql(args);
        Value row = result.as_object<Object_bridge>()->call("next", Argument_list());
        EXPECT_EQ("{\"a\": 42, \"b\": 43}", row.descr());
      }
    }
     */
  }
}
