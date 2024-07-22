/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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

#include "mysqlshdk/libs/parser/mysql_parser_utils.h"

namespace mysqlshdk {
namespace parser {

using Version = mysqlshdk::utils::Version;

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
  EXPECT_NO_THROW(
      check_sql_syntax("show master status", Parser_config{Version(8, 3, 0)}));
  EXPECT_THROW(
      check_sql_syntax("show master status", Parser_config{Version(8, 4, 0)}),
      mysqlshdk::parser::Sql_syntax_error);

  EXPECT_NO_THROW(check_sql_syntax("change master to master_auto_position=1",
                                   Parser_config{Version(8, 3, 0)}));
  EXPECT_THROW(check_sql_syntax("change master to master_auto_position=1",
                                Parser_config{Version(8, 4, 0)}),
               mysqlshdk::parser::Sql_syntax_error);
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

TEST(MysqlParserUtils, extract_table_references) {
  const auto EXPECT = [](const std::string &stmt,
                         const std::vector<Table_reference> &expected) {
    SCOPED_TRACE(stmt);

    std::vector<Table_reference> actual;
    EXPECT_NO_THROW(actual = extract_table_references(
                        stmt, mysqlshdk::utils::k_shell_version));
    EXPECT_EQ(expected, actual);
  };

  EXPECT("SELECT * FROM DUAL", {});

  EXPECT("SELECT * FROM t", {{"", "t"}});
  EXPECT("SELECT * FROM s.t", {{"s", "t"}});

  EXPECT("SELECT * FROM `t`", {{"", "t"}});
  EXPECT("SELECT * FROM `s`.t", {{"s", "t"}});
  EXPECT("SELECT * FROM s.`t`", {{"s", "t"}});
  EXPECT("SELECT * FROM `s`.`t`", {{"s", "t"}});

  EXPECT("SELECT * FROM s.t WHERE 1=0", {{"s", "t"}});
  EXPECT("SELECT * FROM s.t, t2", {{"s", "t"}, {"", "t2"}});
  EXPECT("SELECT * FROM s.t, s2.t2 WHERE 1=0", {{"s", "t"}, {"s2", "t2"}});

  EXPECT("SELECT * FROM t PARTITION (p1, p2) AS a FORCE KEY (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t PARTITION (p) FORCE KEY FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE KEY FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE KEY FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t FORCE INDEX (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE INDEX FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE INDEX FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t FORCE INDEX FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t IGNORE KEY (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE KEY FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE KEY FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE KEY FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t IGNORE INDEX (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE INDEX FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE INDEX FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t IGNORE INDEX FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t USE KEY (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE KEY FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE KEY FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE KEY FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t USE INDEX (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE INDEX FOR JOIN (i)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE INDEX FOR ORDER BY (i1, i2)", {{"", "t"}});
  EXPECT("SELECT * FROM t USE INDEX FOR GROUP BY (PRIMARY)", {{"", "t"}});

  EXPECT("SELECT * FROM t USE INDEX ()", {{"", "t"}});

  EXPECT("SELECT * FROM t USE KEY (i1) USE KEY (i2)", {{"", "t"}});

  EXPECT("SELECT * FROM (t)", {{"", "t"}});
  EXPECT("SELECT * FROM ((t))", {{"", "t"}});
  EXPECT("SELECT * FROM (t USE KEY (i))", {{"", "t"}});

  EXPECT("SELECT * FROM (SELECT * FROM t) AS s", {{"", "t"}});
  EXPECT("SELECT * FROM LATERAL (SELECT * FROM t) AS s (c1, c2)", {{"", "t"}});

  EXPECT("SELECT * FROM (t1 JOIN t2)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM ((t1 JOIN t2))", {{"", "t1"}, {"", "t2"}});

  EXPECT("SELECT * FROM (t1, t2)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM ((t1), (t2))", {{"", "t1"}, {"", "t2"}});

  EXPECT(
      "SELECT * FROM JSON_TABLE('[{\"c1\":null}]','$[*]' COLUMNS(c1 INT PATH "
      "'$.c1' ERROR ON ERROR)) AS jt, t",
      {{"", "t"}});

  EXPECT("SELECT * FROM t1 JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 INNER JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 CROSS JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 STRAIGHT_JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 ON c1=c2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 USING (c)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 USING (c1, c2)", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 JOIN t2 JOIN t3",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});
  EXPECT("SELECT * FROM t1 JOIN t2 JOIN { OJ t3 }",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});

  EXPECT("SELECT * FROM t1 LEFT JOIN t2 ON c1=c2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 LEFT OUTER JOIN t2 USING (c)",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 RIGHT JOIN t2 USING (c1, c2)",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 RIGHT OUTER JOIN t2 ON c1=c2",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 LEFT JOIN t2 USING (c) JOIN t3",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});
  EXPECT("SELECT * FROM t1 LEFT JOIN t2 USING (c1, c2) JOIN { OJ t3 }",
         {{"", "t1"}, {"", "t2"}, {"", "t3"}});

  EXPECT("SELECT * FROM t1 NATURAL JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL INNER JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL RIGHT JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL RIGHT OUTER JOIN t2",
         {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL LEFT JOIN t2", {{"", "t1"}, {"", "t2"}});
  EXPECT("SELECT * FROM t1 NATURAL LEFT OUTER JOIN t2",
         {{"", "t1"}, {"", "t2"}});

  EXPECT("SELECT * FROM { OJ t }", {{"", "t"}});
  EXPECT("SELECT * FROM { OJ t1 JOIN t2 }", {{"", "t1"}, {"", "t2"}});

  EXPECT("TABLE t", {{"", "t"}});
  EXPECT("TABLE t ORDER BY c LIMIT 1 OFFSET 2", {{"", "t"}});

  EXPECT("WITH cte AS (SELECT c FROM t) SELECT c FROM cte", {{"", "t"}});
  EXPECT(
      "WITH cte (c1, c2) AS (SELECT 1, 2 UNION ALL SELECT 3, 4) SELECT c1, c2 "
      "FROM cte;",
      {});
}

}  // namespace parser
}  // namespace mysqlshdk
