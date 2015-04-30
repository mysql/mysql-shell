
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

    //-------------------------- Connection Tests --------------------------
    TEST(Mysql_connection_test, connect_errors)
    {
      Argument_list args;
      const char *uri = getenv("MYSQL_URI");
      const char *pwd = getenv("MYSQL_PWD");
      std::string mysql_uri;

      if (uri)
      {
        mysql_uri.assign(uri);
        mysql_uri = "mysql://" + mysql_uri;
      }
      else
        mysql_uri = "mysql://root@localhost";

      mysh::Mysql_connection *conn;
      std::string temp_uri;

      // Error parsing URI
      temp_uri = mysql_uri + ":fake_port"; // Fake non numeric port will made URI invalid
      EXPECT_THROW(conn = new mysh::Mysql_connection(temp_uri, NULL), shcore::Exception);

      // Connection Error (using invalid port)
      temp_uri = mysql_uri + ":4563"; // Fake invalid port will made connection fail
      EXPECT_THROW(conn = new mysh::Mysql_connection(temp_uri, NULL), shcore::Exception);

      // Connection Error (using invalid password)
      EXPECT_THROW(conn = new mysh::Mysql_connection(mysql_uri, "fake_pwd"), shcore::Exception);
    }

    TEST(Mysql_connection_test, connect_success)
    {
      Argument_list args;
      const char *uri = getenv("MYSQL_URI");
      const char *pwd = getenv("MYSQL_PWD");
      const char *port = getenv("MYSQL_PORT");
      std::string x_uri;

      if (uri)
      {
        x_uri.assign(uri);
        x_uri = "mysql://" + x_uri;
      }
      else
        x_uri = "mysql://root@localhost";

      if (port)
      {
        x_uri.append(":");
        x_uri.append(port);
      }

      env.db.reset(new mysh::Mysql_connection(x_uri, pwd ? pwd : NULL));
    }

    TEST(Mysql_connection_test, sql_no_results_drop_unexisting_schema)
    {
      Value result;
      boost::shared_ptr<Object_bridge> result_ptr;
      Value data;

      // Cleanup to ensure next tests succeed
      result = env.db->sql("drop schema if exists shell_tests", Value());

      // Database does not exist to a warning will be generated
      result = env.db->sql("drop schema if exists shell_tests", Value());
      result_ptr = result.as_object<Object_bridge>();
      data = result_ptr->get_member("warning_count");
      EXPECT_EQ(1, data.as_int());

      data = result_ptr->get_member("affected_rows");
      EXPECT_EQ(0, data.as_int());
    }

    TEST(Mysql_connection_test, sql_no_results_drop_existing_schema)
    {
      Value result;
      boost::shared_ptr<Object_bridge> result_ptr;
      Value data;

      // Now creates the database
      result = env.db->sql("create schema shell_tests", Value());
      result_ptr = result.as_object<Object_bridge>();
      data = result_ptr->get_member("warning_count");
      EXPECT_EQ(0, data.as_int());

      data = result_ptr->get_member("affected_rows");
      EXPECT_EQ(1, data.as_int());

      // Database does not exist to a warning will be generated
      result = env.db->sql("drop schema if exists shell_tests", Value());
      result_ptr = result.as_object<Object_bridge>();
      data = result_ptr->get_member("warning_count");
      EXPECT_EQ(0, data.as_int());

      data = result_ptr->get_member("affected_rows");
      EXPECT_EQ(0, data.as_int());
    }

    TEST(Mysql_connection_test, sql_invalid_query)
    {
      // Creates the test database
      EXPECT_THROW(env.db->sql("select * from hopefully.unexisting", Value()), shcore::Exception);

      // Creates the test database
      EXPECT_THROW(env.db->sql_one("select * from hopefully.unexisting"), shcore::Exception);
    }


    TEST(Mysql_connection_test, sql_single_result)
    {
      Value result;
      boost::shared_ptr<Object_bridge> result_ptr;
      Value data;

      // Creates the test database
      result = env.db->sql("show databases", Value());
      result_ptr = result.as_object<Object_bridge>();
      mysh::Mysql_resultset *real_result = dynamic_cast<mysh::Mysql_resultset *>(result_ptr.get());

      // The unique result should be returned and ready to be read
      if (!real_result)
        FAIL();

      // There should NOT be a second result
      EXPECT_FALSE(env.db->next_result(real_result));
    }

    TEST(Mysql_connection_test, sql_multiple_results)
    {
      Value result;
      boost::shared_ptr<Object_bridge> result_ptr;
      Value data;

      std::string sp =
        "create procedure `shell_tests`.`sp`()\n"
        "begin\n"
        "  select 1 as 'whatever';\n"
        "  show databases;\n"
        "end\n";

      result = env.db->sql("create schema shell_tests", Value());
      result_ptr = result.as_object<Object_bridge>();
      data = result_ptr->get_member("affected_rows");
      EXPECT_EQ(1, data.as_int());

      result = env.db->sql(sp, Value());
      result_ptr = result.as_object<Object_bridge>();
      data = result_ptr->get_member("affected_rows");
      EXPECT_EQ(0, data.as_int());

      result = env.db->sql("call shell_tests.sp()", Value());
      result_ptr = result.as_object<Object_bridge>();
      mysh::Mysql_resultset *real_result = dynamic_cast<mysh::Mysql_resultset *>(result_ptr.get());

      // First result returned by the sp
      if (!real_result)
        FAIL();
      
      // Second result returned by the sp
      EXPECT_TRUE(env.db->next_result(real_result));

      // The result of processing the actual sp
      EXPECT_TRUE(env.db->next_result(real_result));

      // There should NOT be any additional result
      EXPECT_FALSE(env.db->next_result(real_result));

      // We drop the test schema
      result = env.db->sql("drop schema shell_tests", Value());
      data = result_ptr->get_member("affected_rows");
      EXPECT_EQ(0, data.as_int());
    }


    TEST(Mysql_resultset_test, sql_one)
    {
      Value result = env.db->sql_one("select 1 as a");
      EXPECT_EQ("{\"a\": 1}", result.descr());
    }

    //-------------------------- Resultset Tests --------------------------

    TEST(Mysql_resultset_test, sql_metadata_content)
    {
      boost::shared_ptr<Object_bridge> result_ptr;
      mysh::Mysql_resultset *real_result;
      Value data;

      Value result = env.db->sql("show databases", shcore::Value());
      result_ptr = result.as_object<Object_bridge>();
      real_result = dynamic_cast<mysh::Mysql_resultset *>(result_ptr.get());

      // This query only has 1 column
      EXPECT_EQ(1, real_result->fetch_metadata());

      data = real_result->call("fetch_metadata", shcore::Argument_list());
      boost::shared_ptr<shcore::Value::Array_type> array = data.as_array();
      EXPECT_EQ(1, array->size());
      boost::shared_ptr<shcore::Value::Map_type> map = (*array)[0].as_map();

      // Validates the content of a field metadata
      EXPECT_EQ(11, map->size());
      EXPECT_EQ(1, map->count("catalog"));
      EXPECT_EQ(1, map->count("db"));
      EXPECT_EQ(1, map->count("table"));
      EXPECT_EQ(1, map->count("org_table"));
      EXPECT_EQ(1, map->count("name"));
      EXPECT_EQ(1, map->count("org_name"));
      EXPECT_EQ(1, map->count("charset"));
      EXPECT_EQ(1, map->count("length"));
      EXPECT_EQ(1, map->count("type"));
      EXPECT_EQ(1, map->count("flags"));
      EXPECT_EQ(1, map->count("decimal"));
    }

    TEST(Mysql_resultset_test, sql_fetch_table_metadata)
    {
      boost::shared_ptr<Object_bridge> result_ptr;
      mysh::Mysql_resultset *real_result;
      Value data;

      std::string create_table =
        "CREATE TABLE `shell_tests`.`alpha` ("
        "`idalpha` int(11) NOT NULL,"
        "`alphacol` varchar(45) DEFAULT NULL,"
        "PRIMARY KEY(`idalpha`)"
        ") ENGINE = InnoDB DEFAULT CHARSET = utf8";

      Value result = env.db->sql("create schema shell_tests", shcore::Value());
      result_ptr = result.as_object<Object_bridge>();

      EXPECT_EQ(1, result_ptr->get_member("affected_rows").as_int());

      result = env.db->sql(create_table, shcore::Value());
      result_ptr = result.as_object<Object_bridge>();

      EXPECT_EQ(0, result_ptr->get_member("affected_rows").as_int());

      result = env.db->sql("select * from shell_tests.alpha", shcore::Value());
      result_ptr = result.as_object<Object_bridge>();
      data = result_ptr->call("fetch_metadata", shcore::Argument_list());

      boost::shared_ptr<shcore::Value::Array_type> array = data.as_array();
      EXPECT_EQ(2, array->size());
      boost::shared_ptr<shcore::Value::Map_type> map = (*array)[0].as_map();

      EXPECT_EQ("def", (*map)["catalog"].as_string());
      EXPECT_EQ("shell_tests", (*map)["db"].as_string());
      EXPECT_EQ("alpha", (*map)["table"].as_string());
      EXPECT_EQ("alpha", (*map)["org_table"].as_string());
      EXPECT_EQ("idalpha", (*map)["name"].as_string());
      EXPECT_EQ("idalpha", (*map)["org_name"].as_string());
      EXPECT_EQ(11, (*map)["length"].as_int());

      // TODO: At the moment these values support is not well defined
      //EXPECT_EQ("", (*map)["charset"]);
      //EXPECT_EQ("", (*map)["type"]);
      //EXPECT_EQ("", (*map)["flags"]);
      //EXPECT_EQ("", (*map)["decimal"]);

      map = (*array)[1].as_map();
      EXPECT_EQ("def", (*map)["catalog"].as_string());
      EXPECT_EQ("shell_tests", (*map)["db"].as_string());
      EXPECT_EQ("alpha", (*map)["table"].as_string());
      EXPECT_EQ("alpha", (*map)["org_table"].as_string());
      EXPECT_EQ("alphacol", (*map)["name"].as_string());
      EXPECT_EQ("alphacol", (*map)["org_name"].as_string());
      EXPECT_EQ(45, (*map)["length"].as_int());
    }

    TEST(Mysql_resultset_test, sql_fetch_one)
    {
      boost::shared_ptr<Object_bridge> result_ptr;
      mysh::Mysql_resultset *real_result;
      Value data;

      Value result = env.db->sql("INSERT INTO `shell_tests`.`alpha` VALUES(1, 'first'), (2, 'second'), (3, 'third')", shcore::Value());
      result_ptr = result.as_object<Object_bridge>();

      EXPECT_EQ(3, result_ptr->get_member("affected_rows").as_int());

      result = env.db->sql("select * from shell_tests.alpha", shcore::Value());
      result_ptr = result.as_object<Object_bridge>();

      // Fetches the first record with no arguments
      // A document is expected
      shcore::Argument_list args;
      data = result_ptr->call("fetch_one", args);
      EXPECT_EQ("{\"alphacol\": \"first\", \"idalpha\": 1}", data.descr());
      EXPECT_EQ(1, result_ptr->get_member("fetched_row_count").as_int());

      // Fetches the second record with the RAW argument set to false
      // Same format as default is expected
      args.push_back(shcore::Value(false));
      data = result_ptr->call("fetch_one", args);
      EXPECT_EQ("{\"alphacol\": \"second\", \"idalpha\": 2}", data.descr());
      EXPECT_EQ(2, result_ptr->get_member("fetched_row_count").as_int());

      // Fetches the third record as RAW
      // No document is expected, value array instead
      args.clear();
      args.push_back(shcore::Value(true));
      data = result_ptr->call("fetch_one", args);
      EXPECT_EQ("[3,\"third\"]", data.descr());
      EXPECT_EQ(3, result_ptr->get_member("fetched_row_count").as_int());

      // Next attempt to fetch anything should return null
      data = result_ptr->call("fetch_one", args);
      EXPECT_EQ("null", data.descr());

      EXPECT_EQ(3, result_ptr->get_member("fetched_row_count").as_int());
    }

    TEST(Mysql_resultset_test, sql_fetch_all)
    {
      boost::shared_ptr<Object_bridge> result_ptr;
      mysh::Mysql_resultset *real_result;
      Value data;

      Value result = env.db->sql("select * from shell_tests.alpha", shcore::Value());
      result_ptr = result.as_object<Object_bridge>();

      // Fetches all the records with no arguments
      // A list of documents is expected
      shcore::Argument_list args;
      data = result_ptr->call("fetch_all", args);
      std::string expected =
        "["
        "{\"alphacol\": \"first\", \"idalpha\": 1},"
        "{\"alphacol\": \"second\", \"idalpha\": 2},"
        "{\"alphacol\": \"third\", \"idalpha\": 3}"
        "]";

      EXPECT_EQ(expected, data.descr());
      EXPECT_EQ(3, result_ptr->get_member("fetched_row_count").as_int());


      result = env.db->sql("select * from shell_tests.alpha", shcore::Value());
      result_ptr = result.as_object<Object_bridge>();

      // Fetches the records with the RAW argument set to false
      // Same format as default is expected
      args.push_back(shcore::Value(false));
      data = result_ptr->call("fetch_all", args);
      EXPECT_EQ(expected, data.descr());
      EXPECT_EQ(3, result_ptr->get_member("fetched_row_count").as_int());

      result = env.db->sql("select * from shell_tests.alpha", shcore::Value());
      result_ptr = result.as_object<Object_bridge>();

      // Fetches the records with the RAW argument set to true
      // A list of lists is expected
      args.clear();
      args.push_back(shcore::Value(true));
      data = result_ptr->call("fetch_all", args);

      expected =
        "["
        "[1,\"first\"],"
        "[2,\"second\"],"
        "[3,\"third\"]"
        "]";
      EXPECT_EQ(expected, data.descr());
      EXPECT_EQ(3, result_ptr->get_member("fetched_row_count").as_int());
    }

    TEST(Mysql_resultset_test, sql_single_result)
    {
      Value result;
      boost::shared_ptr<Object_bridge> result_ptr;
      Value data;

      // Creates the test database
      result = env.db->sql("show databases", Value());
      result_ptr = result.as_object<Object_bridge>();
      mysh::Mysql_resultset *real_result = dynamic_cast<mysh::Mysql_resultset *>(result_ptr.get());

      // The unique result should be returned and ready to be read
      if (!real_result)
        FAIL();

      // There should NOT be a second result
      EXPECT_FALSE(result_ptr->call("next_result", shcore::Argument_list()).as_bool());
    }

    TEST(Mysql_resultset_test, sql_multiple_results)
    {
      Value result;
      boost::shared_ptr<Object_bridge> result_ptr;
      Value data;

      std::string sp =
        "create procedure `shell_tests`.`sp`()\n"
        "begin\n"
        "  select 1 as 'whatever';\n"
        "  show databases;\n"
        "end\n";

      result = env.db->sql(sp, Value());
      result_ptr = result.as_object<Object_bridge>();
      data = result_ptr->get_member("affected_rows");
      EXPECT_EQ(0, data.as_int());

      result = env.db->sql("call shell_tests.sp()", Value());
      result_ptr = result.as_object<Object_bridge>();
      mysh::Mysql_resultset *real_result = dynamic_cast<mysh::Mysql_resultset *>(result_ptr.get());

      // First result returned by the sp
      if (!real_result)
        FAIL();

      // Second result returned by the sp
      EXPECT_TRUE(result_ptr->call("next_result", shcore::Argument_list()).as_bool());

      // The result of processing the actual sp
      EXPECT_TRUE(result_ptr->call("next_result", shcore::Argument_list()).as_bool());

      // There should NOT be any additional result
      EXPECT_FALSE(result_ptr->call("next_result", shcore::Argument_list()).as_bool());

      // We drop the test schema
      result = env.db->sql("drop schema shell_tests", Value());
      data = result_ptr->get_member("affected_rows");
      EXPECT_EQ(0, data.as_int());
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
