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

#include "modules/util/dump/dump_utils.h"
#include "unittest/gtest_clean.h"

namespace mysqlsh {
namespace dump {

TEST(Dump_utils, encode_schema_basename) {
  EXPECT_EQ(".sql", get_schema_filename(encode_schema_basename("")));
  EXPECT_EQ("sakila.sql",
            get_schema_filename(encode_schema_basename("sakila")));
  EXPECT_EQ("sak%20ila.sql",
            get_schema_filename(encode_schema_basename("sak ila")));
  EXPECT_EQ("sakila.table.sql",
            get_schema_filename(encode_schema_basename("sakila.table")));
  EXPECT_EQ(
      "sakila.table%40real_table.sql",
      get_schema_filename(encode_schema_basename("sakila.table@real_table")));
  EXPECT_EQ("sákila.sql",
            get_schema_filename(encode_schema_basename("sákila")));
}

TEST(Dump_utils, encode_table_basename) {
  EXPECT_EQ("@.sql", get_table_filename(encode_table_basename("", "")));
  EXPECT_EQ("sakila@actor.sql",
            get_table_filename(encode_table_basename("sakila", "actor")));
  EXPECT_EQ("sak%20ila@acto%20.sql",
            get_table_filename(encode_table_basename("sak ila", "acto ")));
  EXPECT_EQ("sakila.table@bla.sql",
            get_table_filename(encode_table_basename("sakila.table", "bla")));
  EXPECT_EQ("sakila.table%40real_table@actual_real_table.sql",
            get_table_filename(encode_table_basename("sakila.table@real_table",
                                                     "actual_real_table")));
  EXPECT_EQ("sákila@tablê%40.sql",
            get_table_filename(encode_table_basename("sákila", "tablê@")));
}

TEST(Dump_utils, encode_table_data_filename) {
  EXPECT_EQ("@.csv",
            get_table_data_filename(encode_table_basename("", ""), "csv"));
  EXPECT_EQ(
      "sakila@actor.csv",
      get_table_data_filename(encode_table_basename("sakila", "actor"), "csv"));
  EXPECT_EQ("sak%20ila@acto%20.csv",
            get_table_data_filename(encode_table_basename("sak ila", "acto "),
                                    "csv"));
  EXPECT_EQ("sakila.table@bla.csv",
            get_table_data_filename(
                encode_table_basename("sakila.table", "bla"), "csv"));
  EXPECT_EQ(
      "sakila.table%40real_table@actual_real_table.csv",
      get_table_data_filename(
          encode_table_basename("sakila.table@real_table", "actual_real_table"),
          "csv"));
  EXPECT_EQ("sákila@tablê%40.csv",
            get_table_data_filename(encode_table_basename("sákila", "tablê@"),
                                    "csv"));
}

TEST(Dump_utils, encode_table_data_filename_chunks) {
  EXPECT_EQ("@@1.csv", get_table_data_filename(encode_table_basename("", ""),
                                               "csv", 1, false));
  EXPECT_EQ("sakila@actor@2.csv",
            get_table_data_filename(encode_table_basename("sakila", "actor"),
                                    "csv", 2, false));
  EXPECT_EQ("sakila@actor@@3.csv",
            get_table_data_filename(encode_table_basename("sakila", "actor"),
                                    "csv", 3, true));

  EXPECT_EQ("sak%20ila@acto%20@123.csv",
            get_table_data_filename(encode_table_basename("sak ila", "acto "),
                                    "csv", 123, false));
  EXPECT_EQ("sakila.table@bla@0.csv",
            get_table_data_filename(
                encode_table_basename("sakila.table", "bla"), "csv", 0, false));
  EXPECT_EQ(
      "sakila.table%40real_table@actual_real_table@42.csv",
      get_table_data_filename(
          encode_table_basename("sakila.table@real_table", "actual_real_table"),
          "csv", 42, false));
  EXPECT_EQ("sákila@tablê%40@4.csv",
            get_table_data_filename(encode_table_basename("sákila", "tablê@"),
                                    "csv", 4, false));

  EXPECT_EQ("sákila@tablê%40@@4.csv",
            get_table_data_filename(encode_table_basename("sákila", "tablê@"),
                                    "csv", 4, true));
}

}  // namespace dump
}  // namespace mysqlsh
