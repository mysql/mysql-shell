/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "unittest/gprod_clean.h"

#include "modules/util/dump/dump_utils.h"
#include "modules/util/load/dump_loader.h"
#include "modules/util/load/dump_reader.h"
#include "modules/util/load/load_dump_options.h"
#include "mysqlshdk/libs/storage/backend/memory_file.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace mysqlsh {

TEST(Load_dump_options, include_schema) {}

TEST(Load_dump_options, exclude_schema) {}

TEST(Load_dump_options, include_exclude_schema) {}

TEST(Load_dump_options, include_table) {}

TEST(Load_dump_options, exclude_table) {}

TEST(Load_dump_options, include_exclude_table) {}

TEST(Load_dump, sql_transforms_strip_sql_mode) {
  Dump_loader::Sql_transform tx;

  auto call = [&tx](const char *s, std::string *out) {
    return tx(s, strlen(s), out);
  };

  auto callr = [&tx](const char *s) {
    std::string tmp;
    EXPECT_TRUE(tx(s, strlen(s), &tmp));
    return tmp;
  };

  std::string out;
  EXPECT_FALSE(call("SET sql_mode = ''", &out));
  EXPECT_EQ("", out);

  tx.add_strip_removed_sql_modes();

  // no changes
  EXPECT_EQ("SET sql_mode='ansi_quotes'", callr("SET sql_mode='ansi_quotes'"));
  EXPECT_EQ("SET sql_mode=''", callr("SET sql_mode=''"));
  EXPECT_EQ("/*!50003 SET sql_mode    =    '' */;",
            callr("/*!50003 SET sql_mode    =    '' */;"));
  EXPECT_EQ("SELECT 'SET sql_mode='NO_AUTO_CREATE_USER''",
            callr("SELECT 'SET sql_mode='NO_AUTO_CREATE_USER''"));
  EXPECT_EQ("SET sql_mode ='ansi_quotes'",
            callr("SET sql_mode ='ansi_quotes'"));
  EXPECT_EQ("SET sql_mode= ''", callr("SET sql_mode= ''"));
  EXPECT_EQ("/*!50003 SET sql_mode    =    '' */;",
            callr("/*!50003 SET sql_mode    =    '' */;"));
  EXPECT_EQ("SELECT 'SET sql_mode='NO_AUTO_CREATE_USER''",
            callr("SELECT 'SET sql_mode='NO_AUTO_CREATE_USER''"));

  // changes
  EXPECT_EQ("SET sql_mode=''", callr("SET sql_mode='NO_AUTO_CREATE_USER'"));
  EXPECT_EQ("SET sql_mode='ANSI_QUOTES'",
            callr("SET sql_mode='ANSI_QUOTES,NO_AUTO_CREATE_USER'"));
  EXPECT_EQ("SET sql_mode='ANSI_QUOTES'",
            callr("SET sql_mode='NO_AUTO_CREATE_USER,ANSI_QUOTES'"));
  EXPECT_EQ(
      "set sql_mode='ANSI_QUOTES,NO_ZERO_DATE'",
      callr("set sql_mode='ANSI_QUOTES,NO_AUTO_CREATE_USER,NO_ZERO_DATE'"));

  EXPECT_EQ("SET sql_mode    =''",
            callr("SET sql_mode    ='NO_AUTO_CREATE_USER'"));
  EXPECT_EQ("SET sql_mode=    'ANSI_QUOTES'",
            callr("SET sql_mode=    'ANSI_QUOTES,NO_AUTO_CREATE_USER'"));
  EXPECT_EQ("SET sql_mode = 'ANSI_QUOTES'",
            callr("SET sql_mode = 'NO_AUTO_CREATE_USER,ANSI_QUOTES'"));
  EXPECT_EQ("set sql_mode       =       'ANSI_QUOTES,NO_ZERO_DATE'",
            callr("set sql_mode       =       "
                  "'ANSI_QUOTES,NO_AUTO_CREATE_USER,NO_ZERO_DATE'"));

  EXPECT_EQ("/*!50003 SET sql_mode='' */",
            callr("/*!50003 SET sql_mode='NO_AUTO_CREATE_USER' */"));
  EXPECT_EQ(
      "/*!50003 SET sql_mode='ANSI_QUOTES' */",
      callr("/*!50003 SET sql_mode='ANSI_QUOTES,NO_AUTO_CREATE_USER' */"));
  EXPECT_EQ(
      "/*!50003 SET sql_mode='ANSI_QUOTES' */",
      callr("/*!50003 SET sql_mode='NO_AUTO_CREATE_USER,ANSI_QUOTES' */"));
  EXPECT_EQ(
      "/*!50003 SET sql_mode='ANSI_QUOTES,NO_ZERO_DATE' */",
      callr("/*!50003 SET "
            "sql_mode='ANSI_QUOTES,NO_AUTO_CREATE_USER,NO_ZERO_DATE' */"));
}

}  // namespace mysqlsh
