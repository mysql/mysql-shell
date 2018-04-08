/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "mysqlshdk/libs/mysql/utils.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace mysqlshdk {
namespace mysql {

class Mysql_utils : public tests::Shell_test_env {};

#define COMPARE_ACCOUNTS(user1, host1, user2, host2)                          \
  EXPECT_EQ(                                                                  \
      1,                                                                      \
      session                                                                 \
          ->queryf(                                                           \
              "select "                                                       \
              "coalesce((select group_concat(concat(table_catalog, "          \
              "privilege_type, "                                              \
              "is_grantable)) from information_schema.user_privileges where " \
              "grantee = ? order by privilege_type), '')"                     \
              " = coalesce((select group_concat(concat(table_catalog, "       \
              "privilege_type, "                                              \
              "is_grantable)) from information_schema.user_privileges where " \
              "grantee = ? order by privilege_type), '')",                    \
              "'" user1 "'@'" host1 "'", "'" user2 "'@'" host2 "'")           \
          ->fetch_one()                                                       \
          ->get_int(0));                                                      \
  EXPECT_EQ(                                                                  \
      1,                                                                      \
      session                                                                 \
          ->queryf("select "                                                  \
                   "coalesce((select group_concat(concat(table_catalog, "     \
                   "table_schema, "                                           \
                   "privilege_type, is_grantable)) from "                     \
                   "information_schema.schema_privileges where grantee = ? "  \
                   "order by privilege_type), '') "                           \
                   " = coalesce((select group_concat(concat(table_catalog, "  \
                   "table_schema, privilege_type, is_grantable)) from "       \
                   "information_schema.schema_privileges where grantee = ? "  \
                   "order by privilege_type), '')",                           \
                   "'" user1 "'@'" host1 "'", "'" user2 "'@'" host2 "'")      \
          ->fetch_one()                                                       \
          ->get_int(0));                                                      \
  EXPECT_EQ(                                                                  \
      1,                                                                      \
      session                                                                 \
          ->queryf("select "                                                  \
                   "coalesce((select group_concat(concat(table_catalog, "     \
                   "table_schema, table_name, "                               \
                   "privilege_type, is_grantable)) from "                     \
                   "information_schema.table_privileges "                     \
                   "where grantee = ? order by privilege_type), '') "         \
                   " = coalesce((select group_concat(concat(table_catalog, "  \
                   "table_schema, table_name, "                               \
                   "privilege_type, is_grantable)) from "                     \
                   "information_schema.table_privileges "                     \
                   "where grantee = ? order by privilege_type), '')",         \
                   "'" user1 "'@'" host1 "'", "'" user2 "'@'" host2 "'")      \
          ->fetch_one()                                                       \
          ->get_int(0));                                                      \
  EXPECT_EQ(                                                                  \
      1,                                                                      \
      session                                                                 \
          ->queryf(                                                           \
              "select "                                                       \
              "coalesce((select group_concat(concat(table_catalog, "          \
              "table_schema, "                                                \
              "table_name, column_name, privilege_type, is_grantable)) from " \
              "information_schema.column_privileges where grantee = ? order " \
              "by privilege_type), '') "                                      \
              " = coalesce((select group_concat(concat(table_catalog, "       \
              "table_schema, "                                                \
              "table_name, "                                                  \
              "column_name, privilege_type, is_grantable)) from "             \
              "information_schema.column_privileges where grantee = ? order " \
              "by privilege_type), '')",                                      \
              "'" user1 "'@'" host1 "'", "'" user2 "'@'" host2 "'")           \
          ->fetch_one()                                                       \
          ->get_int(0));

TEST_F(Mysql_utils, clone_user) {
  auto session = create_mysql_session();

  session->execute("drop user if exists rootey@localhost");
  session->execute("drop user if exists testusr@localhost");

  session->execute("create user testusr@localhost");
  session->execute(
      "grant shutdown on *.* to testusr@localhost with grant option");
  session->execute(
      "grant select on performance_schema.* to testusr@localhost with "
      "grant "
      "option");
  session->execute("grant select,insert on mysql.user to testusr@localhost");
  session->execute(
      "grant select (user), update (user) on mysql.db to "
      "testusr@localhost");

  // clone the current user (presumably root@localhost)
  clone_user(session, "root", "localhost", "rootey", "localhost", "foo");
  // compare grants through the I_S table
  COMPARE_ACCOUNTS("root", "localhost", "rootey", "localhost");
  session->execute("drop user rootey@localhost");

  clone_user(session, "testusr", "localhost", "rootey", "localhost", "foo");
  COMPARE_ACCOUNTS("testusr", "localhost", "rootey", "localhost");

  // drop the account
  session->execute("drop user testusr@localhost");
  session->execute("drop user rootey@localhost");
  session->close();
}

namespace detail {
std::string replace_grantee(const std::string &grant,
                            const std::string &replacement);
}

TEST_F(Mysql_utils, clone_user_replace_grantee) {
  EXPECT_EQ("GRANT PROXY ON ''@'' TO 'foo'@'bar' WITH GRANT OPTION",
            detail::replace_grantee(
                "GRANT PROXY ON ''@'' TO 'root'@'localhost' WITH GRANT OPTION",
                "'foo'@'bar'"));

  EXPECT_EQ("GRANT PROXY ON ``@`` TO 'foo'@'bar' WITH GRANT OPTION",
            detail::replace_grantee(
                "GRANT PROXY ON ``@`` TO `root`@`localhost` WITH GRANT OPTION",
                "'foo'@'bar'"));

  EXPECT_EQ(
      "GRANT ALL PRIVILEGES ON *.* TO 'foo'@'bar' WITH GRANT OPTION",
      detail::replace_grantee(
          "GRANT ALL PRIVILEGES ON *.* TO 'root'@'localhost' WITH GRANT OPTION",
          "'foo'@'bar'"));

  EXPECT_EQ(
      "GRANT ALL PRIVILEGES ON *.* TO 'foo'@'bar' WITH GRANT OPTION",
      detail::replace_grantee(
          "GRANT ALL PRIVILEGES ON *.* TO `root`@`localhost` WITH GRANT OPTION",
          "'foo'@'bar'"));

  EXPECT_EQ(
      "GRANT ALL PRIVILEGES ON ` TO 'bla'@'bla' `.* TO 'foo'@'bar'",
      detail::replace_grantee(
          "GRANT ALL PRIVILEGES ON ` TO 'bla'@'bla' `.* TO 'root'@'localhost'",
          "'foo'@'bar'"));
}

}  // namespace mysql
}  // namespace mysqlshdk
