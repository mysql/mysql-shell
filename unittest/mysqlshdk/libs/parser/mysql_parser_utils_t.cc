/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "unittest/gtest_clean.h"

// hack to workaround antlr4 trying to include Token.h but getting token.h from
// Python in macos
#define Py_LIMITED_API
#include "mysqlshdk/libs/parser/mysql_parser_utils.h"

namespace mysqlshdk {
namespace parser {

TEST(MysqlParserUtils, check_syntax_ok) {
  EXPECT_NO_THROW(check_sql_syntax(R"*(select 1, '1', "1")*", {}));

  EXPECT_NO_THROW(check_sql_syntax(R"*(
CREATE TABLE reservedTestTable (
      `ROWS` binary(16) NOT NULL,
      `ROW` binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (`ROWS`,`C`),
      KEY `BD_idx` (`ROW`,`D`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
)*",
                                   {}));

  EXPECT_NO_THROW(check_sql_syntax(R"*(
DELIMITER $$
select 1$$
select 2$$
create procedure a() begin
select 1;
select 2;
end$$
select 3$$
    )*",
                                   {}));

  EXPECT_NO_THROW(check_sql_syntax(R"*(
DELIMITER $$
CREATE PROCEDURE GetTotalOrder()
BEGIN
  DECLARE `rows` INT DEFAULT 0;
  DECLARE `ROW`  INT DEFAULT 4;

  CREATE TABLE reservedTestTable (
      `ROWS` binary(16) NOT NULL,
      `ROW` binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (`ROWS`,`C`),
      KEY `BD_idx` (`ROW`,`D`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

  select now();
END$$

DELIMITER ;
)*",
                                   {}));
}

TEST(MysqlParserUtils, syntax_deprecated_but_not_removed) {
  EXPECT_NO_THROW(check_sql_syntax("show master status"));
  EXPECT_NO_THROW(check_sql_syntax("change master to master_auto_position=1"));
}

TEST(MysqlParserUtils, check_syntax_bad) {
  // ROWS is reserved
  EXPECT_THROW(check_sql_syntax(R"*(
CREATE TABLE reservedTestTable (
      ROWS binary(16) NOT NULL,
      ROW binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (ROWS,`C`),
      KEY `BD_idx` (ROW,`D`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
)*",
                                {}),
               Sql_syntax_error);

  // ROWS is reserved
  EXPECT_THROW(check_sql_syntax(R"*(
DELIMITER $$
CREATE PROCEDURE GetTotalOrder()
BEGIN
  DECLARE rows INT DEFAULT 0;
  DECLARE ROW  INT DEFAULT 4;

  CREATE TABLE reservedTestTable (
      ROWS binary(16) NOT NULL,
      ROW binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (ROWS,`C`),
      KEY `BD_idx` (ROW,`D`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

  select now();
END$$

DELIMITER ;
)*",
                                {}),
               Sql_syntax_error);
}

}  // namespace parser
}  // namespace mysqlshdk