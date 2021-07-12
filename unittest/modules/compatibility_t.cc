/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include "modules/util/dump/compatibility.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/test_combo_utilities.h"

namespace mysqlsh {
namespace compatibility {

class Compatibility_test : public tests::Shell_base_test {
 protected:
  const std::vector<std::string> multiline = {
      R"(CREATE TABLE t(i int)
ENCRYPTION = 'N' 
DATA DIRECTORY = '\tmp' INDEX DIRECTORY = '\tmp',
ENGINE = MyISAM)",
      R"(CREATE TABLE t (i int PRIMARY KEY) ENGINE = MyISAM
  DATA DIRECTORY = '/tmp'
  ENCRYPTION = 'y'
  PARTITION BY LIST (i) (
    PARTITION p0 VALUES IN (0) ENGINE = MyISAM,
    PARTITION p1 VALUES IN (1)
    DATA DIRECTORY = '/tmp',
    ENGINE = BLACKHOLE
  ))",
      R"(CREATE TABLE `tmq` (
  `i` int(11) DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1 DATA DIRECTORY='/tmp/' INDEX DIRECTORY='/tmp/')",
      R"(CREATE TABLE `tmq` (
  `i` int(11) DEFAULT NULL
) /*!50100 TABLESPACE `t s 1` */ ENGINE=InnoDB DEFAULT CHARSET=latin1 ENCRYPTION='N')",
      R"(CREATE TABLE `tmq` (
  `i` int(11) NOT NULL,
  PRIMARY KEY (`i`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1
/*!50100 PARTITION BY LIST (i)
(PARTITION p0 VALUES IN (0) ENGINE = MyISAM,
 PARTITION p1 VALUES IN (1) DATA DIRECTORY = '/tmp' ENGINE = MyISAM) */)",
      R"(CREATE TABLE `tmq` (
  `i` int NOT NULL,
  PRIMARY KEY (`i`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci
/*!50100 PARTITION BY LIST (`i`)
(PARTITION p0 VALUES IN (0) DATA DIRECTORY = '/tmp/' ENGINE = InnoDB,
 PARTITION p1 VALUES IN (1) DATA DIRECTORY = '/tmp/' ENGINE = InnoDB) */)",
      R"(CREATE TABLE `tmq` (
  `i` int(11) NOT NULL,
  PRIMARY KEY (`i`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1
/*!50100 PARTITION BY LIST (i)
(PARTITION p0 VALUES IN (0) TABLESPACE = `t s 1` ENGINE = MyISAM,
 PARTITION p1 VALUES IN (1) TABLESPACE = `t s 1` ENGINE = MyISAM) */)",
      "CREATE DATABASE `dcs` /*!40100 DEFAULT CHARACTER SET latin1 COLLATE "
      "latin1_danish_ci */ /*!80016 DEFAULT ENCRYPTION='N' */"};

  const std::vector<std::string> rogue = {
      R"(CREATE TABLE `rogue` (
  `data` int DEFAULT NULL,
  `index` int DEFAULT NULL,
  `directory` int DEFAULT NULL,
  `encryption` int DEFAULT NULL,
  `engine` int DEFAULT NULL,
  `tablespace` int DEFAULT NULL,
  `collate` int DEFAULT NULL,
  `charset` int DEFAULT NULL,
  `character` int DEFAULT NULL,
  `definer` int DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)",
      " create table rogue (data int, `index` int, directory int, encryption "
      "int, engine int, tablespace int, `collate` int, charset int, "
      "`character` int, definer int);"};
};

TEST_F(Compatibility_test, check_privileges) {
  std::string rewritten;

  EXPECT_TRUE(
      check_privileges(
          "GRANT `app_delete`@`%`,`app_read`@`%`,`app_write`@`%`,`combz`@`%` "
          "TO `combined`@`%`",
          &rewritten)
          .empty());

  EXPECT_EQ(1,
            check_privileges(
                "GRANT SUPER, LOCK TABLES ON *.* TO 'superfirst'@'localhost';",
                &rewritten)
                .size());
  EXPECT_EQ("GRANT LOCK TABLES ON *.* TO 'superfirst'@'localhost';", rewritten);

  EXPECT_EQ(
      1, check_privileges(
             "GRANT INSERT,super, UPDATE ON *.* TO 'superafter'@'localhost';",
             &rewritten)
             .size());
  EXPECT_EQ("GRANT INSERT, UPDATE ON *.* TO 'superafter'@'localhost';",
            rewritten);

  EXPECT_EQ(1,
            check_privileges("GRANT  SUPER ON *.* TO 'superonly'@'localhost';",
                             &rewritten)
                .size());
  EXPECT_TRUE(rewritten.empty());

  EXPECT_EQ(4, check_privileges("GRANT SUPER,FILE, reload, BINLOG_ADMIN ON *.* "
                                "TO 'empty'@'localhost';",
                                &rewritten)
                   .size());
  EXPECT_TRUE(rewritten.empty());

  EXPECT_EQ(4, check_privileges("GRANT INSERT,SUPER,FILE,LOCK TABLES , reload, "
                                "BINLOG_ADMIN, SELECT ON *.* "
                                "TO 'empty'@'localhost';",
                                &rewritten)
                   .size());
  EXPECT_EQ("GRANT INSERT,LOCK TABLES , SELECT ON *.* TO 'empty'@'localhost';",
            rewritten);

  EXPECT_EQ(4, check_privileges("GRANT BINLOG_ADMIN, INSERT    , SUPER "
                                ",`app_delete`@`%`, LOCK TABLES , reload, "
                                "UPDATE,FILE, SELECT ON *.* "
                                "TO 'empty'@'localhost';",
                                &rewritten)
                   .size());
  EXPECT_EQ(
      "GRANT INSERT     ,`app_delete`@`%`, LOCK TABLES , UPDATE, SELECT ON *.* "
      "TO 'empty'@'localhost';",
      rewritten);

  EXPECT_EQ(0,
            check_privileges(
                "REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE "
                "TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, "
                "ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `root`@`%`")
                .size());

  EXPECT_EQ(3,
            check_privileges(
                "REVOKE SUPER, DROP, REFERENCES, INDEX, FILE, CREATE "
                "TEMPORARY TABLES, LOCK TABLES, BINLOG_ADMIN, CREATE ROUTINE, "
                "ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `root`@`%`",
                &rewritten)
                .size());

  EXPECT_EQ(
      "REVOKE DROP, REFERENCES, INDEX, CREATE TEMPORARY TABLES, LOCK TABLES, "
      "CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM "
      "`root`@`%`",
      rewritten);
}

TEST_F(Compatibility_test, data_index_dir_option) {
  std::string rewritten;

  EXPECT_FALSE(check_create_table_for_data_index_dir_option(
      "CREATE TABLE t(i int) ENGINE=MyIsam", &rewritten));

  EXPECT_TRUE(check_create_table_for_data_index_dir_option(
      "CREATE TABLE t(i int) DATA DIRECTORY = 'c:/temporary directory'",
      &rewritten));
  EXPECT_EQ(
      "CREATE TABLE t(i int) /* DATA DIRECTORY = 'c:/temporary directory'*/ ",
      rewritten);

  EXPECT_TRUE(check_create_table_for_data_index_dir_option(
      "CREATE TABLE t(i int) index DIRECTORY = '\\tmp\\one\\t w o\\3'",
      &rewritten));
  EXPECT_EQ(
      "CREATE TABLE t(i int) /* index DIRECTORY = '\\tmp\\one\\t w o\\3'*/ ",
      rewritten);

  EXPECT_TRUE(
      check_create_table_for_data_index_dir_option(multiline[0], &rewritten));
  EXPECT_EQ(
      "CREATE TABLE t(i int)\n"
      "ENCRYPTION = 'N' \n"
      "/* DATA DIRECTORY = '\\tmp' INDEX DIRECTORY = '\\tmp',*/ \n"
      "ENGINE = MyISAM",
      rewritten);

  EXPECT_TRUE(
      check_create_table_for_data_index_dir_option(multiline[1], &rewritten));
  EXPECT_EQ(
      "CREATE TABLE t (i int PRIMARY KEY) ENGINE = MyISAM\n"
      "  /* DATA DIRECTORY = '/tmp'*/ \n"
      "  ENCRYPTION = 'y'\n"
      "  PARTITION BY LIST (i) (\n"
      "    PARTITION p0 VALUES IN (0) ENGINE = MyISAM,\n"
      "    PARTITION p1 VALUES IN (1)\n"
      "    /* DATA DIRECTORY = '/tmp',*/ \n"
      "    ENGINE = BLACKHOLE\n"
      "  )",
      rewritten);

  EXPECT_TRUE(
      check_create_table_for_data_index_dir_option(multiline[4], &rewritten));
  EXPECT_EQ(
      "CREATE TABLE `tmq` (\n"
      "  `i` int(11) NOT NULL,\n"
      "  PRIMARY KEY (`i`)\n"
      ") ENGINE=MyISAM DEFAULT CHARSET=latin1\n"
      "/*!50100 PARTITION BY LIST (i)\n"
      "(PARTITION p0 VALUES IN (0) ENGINE = MyISAM,\n"
      " PARTITION p1 VALUES IN (1) -- DATA DIRECTORY = '/tmp'\n"
      " ENGINE = MyISAM) */",
      rewritten);

  EXPECT_TRUE(
      check_create_table_for_data_index_dir_option(multiline[5], &rewritten));
  EXPECT_EQ(R"(CREATE TABLE `tmq` (
  `i` int NOT NULL,
  PRIMARY KEY (`i`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci
/*!50100 PARTITION BY LIST (`i`)
(PARTITION p0 VALUES IN (0) -- DATA DIRECTORY = '/tmp/'
 ENGINE = InnoDB,
 PARTITION p1 VALUES IN (1) -- DATA DIRECTORY = '/tmp/'
 ENGINE = InnoDB) */)",
            rewritten);
}

TEST_F(Compatibility_test, encryption_option) {
  std::string rewritten;
  auto ct = "CREATE TABLE t(i int) ENCRYPTION 'N'";
  EXPECT_TRUE(check_create_table_for_encryption_option(ct, &rewritten));
  EXPECT_EQ("CREATE TABLE t(i int) /* ENCRYPTION 'N'*/ ", rewritten);

  EXPECT_TRUE(
      check_create_table_for_encryption_option(multiline[0], &rewritten));
  EXPECT_EQ(
      "CREATE TABLE t(i int)\n"
      "/* ENCRYPTION = 'N'*/  \n"
      "DATA DIRECTORY = '\\tmp' INDEX DIRECTORY = '\\tmp',\n"
      "ENGINE = MyISAM",
      rewritten);

  EXPECT_TRUE(
      check_create_table_for_encryption_option(multiline[1], &rewritten));
  EXPECT_EQ(
      "CREATE TABLE t (i int PRIMARY KEY) ENGINE = MyISAM\n"
      "  DATA DIRECTORY = '/tmp'\n"
      "  /* ENCRYPTION = 'y'*/ \n"
      "  PARTITION BY LIST (i) (\n"
      "    PARTITION p0 VALUES IN (0) ENGINE = MyISAM,\n"
      "    PARTITION p1 VALUES IN (1)\n"
      "    DATA DIRECTORY = '/tmp',\n"
      "    ENGINE = BLACKHOLE\n"
      "  )",
      rewritten);

  EXPECT_TRUE(compatibility::check_create_table_for_encryption_option(
      multiline[7], &rewritten));
  EXPECT_EQ(
      "CREATE DATABASE `dcs` /*!40100 DEFAULT CHARACTER SET latin1 COLLATE "
      "latin1_danish_ci */ /*!80016 -- DEFAULT ENCRYPTION='N'\n"
      " */",
      rewritten);
}

TEST_F(Compatibility_test, engine_change) {
  std::string rewritten;

  EXPECT_EQ("MyISAM",
            check_create_table_for_engine_option(multiline[0], &rewritten));
  EXPECT_EQ(R"(CREATE TABLE t(i int)
ENCRYPTION = 'N' 
DATA DIRECTORY = '\tmp' INDEX DIRECTORY = '\tmp',
ENGINE = InnoDB)",
            rewritten);

  EXPECT_EQ("BLACKHOLE",
            check_create_table_for_engine_option(multiline[1], &rewritten));
  EXPECT_EQ(R"(CREATE TABLE t (i int PRIMARY KEY) ENGINE = InnoDB
  DATA DIRECTORY = '/tmp'
  ENCRYPTION = 'y'
  PARTITION BY LIST (i) (
    PARTITION p0 VALUES IN (0) ENGINE = InnoDB,
    PARTITION p1 VALUES IN (1)
    DATA DIRECTORY = '/tmp',
    ENGINE = InnoDB
  ))",
            rewritten);
}

TEST_F(Compatibility_test, tablespace_removal) {
  std::string rewritten;
  EXPECT_TRUE(check_create_table_for_tablespace_option(
      "CREATE TABLE t1 (c1 INT, c2 INT) TABLESPACE ts_1 ENGINE NDB;",
      &rewritten));

  EXPECT_TRUE(check_create_table_for_tablespace_option(
      "CREATE TABLE t1 (c1 INT, c2 INT) TABLESPACE `ts 1`;", &rewritten));
  EXPECT_EQ("CREATE TABLE t1 (c1 INT, c2 INT) ;", rewritten);

  EXPECT_TRUE(check_create_table_for_tablespace_option(
      "CREATE TABLE t1 (c1 INT) TABLESPACE=`ts_1` ENGINE NDB", &rewritten));
  EXPECT_EQ("CREATE TABLE t1 (c1 INT)  ENGINE NDB", rewritten);

  EXPECT_TRUE(check_create_table_for_tablespace_option(
      "CREATE TABLE t1 (c1 INT) ENGINE NDB, TABLESPACE= ts_1", &rewritten));
  EXPECT_EQ("CREATE TABLE t1 (c1 INT) ENGINE NDB", rewritten);

  EXPECT_TRUE(check_create_table_for_tablespace_option(
      "CREATE TABLE t1 (c1 INT) TABLESPACE =ts_1", &rewritten));
  EXPECT_EQ("CREATE TABLE t1 (c1 INT) ", rewritten);

  EXPECT_TRUE(check_create_table_for_tablespace_option(
      "CREATE TABLE t(i int) ENCRYPTION = 'N' DATA DIRECTORY = '\tmp', "
      "/*!50100 TABLESPACE `t s 1` */ ENGINE = InnoDB;",
      &rewritten));
  EXPECT_EQ(
      "CREATE TABLE t(i int) ENCRYPTION = 'N' DATA DIRECTORY = '\tmp' ENGINE = "
      "InnoDB;",
      rewritten);

  EXPECT_TRUE(
      check_create_table_for_tablespace_option(multiline[6], &rewritten));
  EXPECT_EQ(R"(CREATE TABLE `tmq` (
  `i` int(11) NOT NULL,
  PRIMARY KEY (`i`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1
/*!50100 PARTITION BY LIST (i)
(PARTITION p0 VALUES IN (0)  ENGINE = MyISAM,
 PARTITION p1 VALUES IN (1)  ENGINE = MyISAM) */)",
            rewritten);
}

TEST_F(Compatibility_test, check_create_table_for_fixed_row_format) {
  EXPECT_FALSE(
      check_create_table_for_fixed_row_format("CREATE TABLE t (a int);"));

  const auto validate = [](const std::string &statement, bool changed,
                           const std::string &result = {}) {
    SCOPED_TRACE(statement);

    auto s = statement;

    EXPECT_EQ(changed, check_create_table_for_fixed_row_format(s, &s));

    if (changed) {
      EXPECT_EQ(result, s);
    } else {
      EXPECT_EQ(statement, s);
    }
  };

  validate("CREATE TABLE t (a int);", false);

  validate("CREATE TABLE t (a int) COMMENT = 'tmp', ROW_FORMAT DYNAMIC", false);
  validate("CREATE TABLE t (a int) COMMENT = 'tmp' ROW_FORMAT DYNAMIC", false);
  validate("CREATE TABLE t (a int) ROW_FORMAT = DYNAMIC", false);
  validate("CREATE TABLE t (a int) ROW_FORMAT=DYNAMIC,COMMENT='tmp'", false);

  validate("CREATE TABLE t (a int) COMMENT = 'tmp', ROW_FORMAT FIXED", true,
           "CREATE TABLE t (a int) COMMENT = 'tmp'");
  validate("CREATE TABLE t (a int) COMMENT = 'tmp' ROW_FORMAT FIXED", true,
           "CREATE TABLE t (a int) COMMENT = 'tmp'");
  validate("CREATE TABLE t (a int) ROW_FORMAT = FIXED", true,
           "CREATE TABLE t (a int)");
  validate("CREATE TABLE t (a int) ROW_FORMAT=FIXED,COMMENT='tmp'", true,
           "CREATE TABLE t (a int) COMMENT='tmp'");
  validate(
      "CREATE TABLE t (a int) COMPRESSION=NONE ROW_FORMAT=FIXED,COMMENT='tmp'",
      true, "CREATE TABLE t (a int) COMPRESSION=NONE COMMENT='tmp'");
  validate(
      "CREATE TABLE t (a int) COMPRESSION=NONE,ROW_FORMAT=FIXED,COMMENT='tmp'",
      true, "CREATE TABLE t (a int) COMPRESSION=NONE,COMMENT='tmp'");
}

TEST_F(Compatibility_test, charset_option) {
  std::string rewritten;
  EXPECT_EQ(2, compatibility::check_statement_for_charset_option(
                   R"(CREATE TABLE `dsc` (
  `v` varchar(20) CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL,
  `v1` varchar(10) COLLATE latin1_danish_ci DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_danish_ci)",
                   &rewritten)
                   .size());

  EXPECT_EQ(R"(CREATE TABLE `dsc` (
  `v` varchar(20)   DEFAULT NULL,
  `v1` varchar(10)  DEFAULT NULL
) ENGINE=InnoDB  )",
            rewritten);

  EXPECT_EQ(1, compatibility::check_statement_for_charset_option(multiline[7],
                                                                 &rewritten)
                   .size());
  EXPECT_EQ("CREATE DATABASE `dcs`  /*!80016 DEFAULT ENCRYPTION='N' */",
            rewritten);
}

TEST_F(Compatibility_test, definer_option) {
  std::string rewritten;
  EXPECT_EQ(
      "`root`@`localhost`",
      compatibility::check_statement_for_definer_clause(
          R"(CREATE DEFINER=`root`@`localhost` PROCEDURE `bug9056_proc2`(OUT a INT)
BEGIN
  select sum(id) from tr1 into a;
END ;;)",
          &rewritten));
  EXPECT_EQ(R"(CREATE PROCEDURE `bug9056_proc2`(OUT a INT)
BEGIN
  select sum(id) from tr1 into a;
END ;;)",
            rewritten);

  EXPECT_EQ(
      "`root`@`localhost`",
      compatibility::check_statement_for_definer_clause(
          R"(CREATE DEFINER = `root`@`localhost` FUNCTION `bug9056_func2`(f1 char binary) RETURNS char(1) CHARSET utf8mb4
begin
  set f1= concat( 'hello', f1 );
  return f1;
end ;;)",
          &rewritten));
  EXPECT_EQ(
      R"(CREATE FUNCTION `bug9056_func2`(f1 char binary) RETURNS char(1) CHARSET utf8mb4
begin
  set f1= concat( 'hello', f1 );
  return f1;
end ;;)",
      rewritten);

  EXPECT_EQ("`root`@`localhost`",
            compatibility::check_statement_for_definer_clause(
                "/*!50106 CREATE DEFINER=`root` @ `localhost` EVENT IF NOT "
                "EXISTS `ee1` "
                "ON "
                "SCHEDULE AT '2035-12-31 20:01:23' ON COMPLETION NOT PRESERVE "
                "ENABLE DO "
                "set @a=5 */ ;;",
                &rewritten));
  EXPECT_EQ(
      "/*!50106 CREATE EVENT IF NOT EXISTS `ee1` ON "
      "SCHEDULE AT '2035-12-31 "
      "20:01:23' ON COMPLETION NOT PRESERVE ENABLE DO set @a=5 */ ;;",
      rewritten);

  EXPECT_EQ(
      "`root`@`localhost`",
      compatibility::check_statement_for_definer_clause(
          "/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL "
          "SECURITY DEFINER VIEW `v3` AS select `tv1`.`a` AS `a`,`tv1`.`b` AS "
          "`b`,`tv1`.`c` AS `c` from `tv1` */;",
          &rewritten));
  EXPECT_EQ(
      "/*!50001 CREATE ALGORITHM=UNDEFINED SQL SECURITY "
      "DEFINER VIEW `v3` AS "
      "select `tv1`.`a` AS `a`,`tv1`.`b` AS `b`,`tv1`.`c` AS `c` from `tv1` "
      "*/;",
      rewritten);

  EXPECT_EQ(
      "`root`@`localhost`",
      compatibility::check_statement_for_definer_clause(
          R"(/*!50003 CREATE DEFINER=`root`@`localhost` TRIGGER `trg4` BEFORE INSERT ON `t2` FOR EACH ROW begin
  if new.a > 10 then
    set @fired:= "No";
  end if;
end */;;)",
          &rewritten));
  EXPECT_EQ(
      R"(/*!50003 CREATE TRIGGER `trg4` BEFORE INSERT ON `t2` FOR EACH ROW begin
  if new.a > 10 then
    set @fired:= "No";
  end if;
end */;;)",
      rewritten);
}

TEST_F(Compatibility_test, sql_security_clause) {
  std::string rewritten;

  EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
      R"(CREATE DEFINER = `root`@`localhost` FUNCTION `bug9056_func2`(f1 char binary) RETURNS char(1) SQL SECURITY DEFINER CHARSET utf8mb4
begin
  set f1= concat( 'hello', f1 );
  return f1;
end ;;)",
      &rewritten));
  EXPECT_EQ(
      R"(CREATE DEFINER = `root`@`localhost` FUNCTION `bug9056_func2`(f1 char binary) RETURNS char(1) SQL SECURITY INVOKER CHARSET utf8mb4
begin
  set f1= concat( 'hello', f1 );
  return f1;
end ;;)",
      rewritten);

  EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
      R"(CREATE procedure test.for_loop_example()
wholeblock:BEGIN
  DECLARE x INT;
  DECLARE str VARCHAR(255);
  SET x = -5;
  SET str = '';

  loop_label: LOOP
    IF x > 0 THEN
      LEAVE loop_label;
    END IF;
    SET str = CONCAT(str,x,',');
    SET x = x + 1;
    ITERATE loop_label;
  END LOOP;

  SELECT str;

END//)",
      &rewritten));
  EXPECT_EQ(R"(CREATE procedure test.for_loop_example()
SQL SECURITY INVOKER
wholeblock:BEGIN
  DECLARE x INT;
  DECLARE str VARCHAR(255);
  SET x = -5;
  SET str = '';

  loop_label: LOOP
    IF x > 0 THEN
      LEAVE loop_label;
    END IF;
    SET str = CONCAT(str,x,',');
    SET x = x + 1;
    ITERATE loop_label;
  END LOOP;

  SELECT str;

END//)",
            rewritten);

  EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
      "/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL "
      "SECURITY DEFINER VIEW `v3` AS select `tv1`.`a` AS `a`,`tv1`.`b` AS "
      "`b`,`tv1`.`c` AS `c` from `tv1` */;",
      &rewritten));
  EXPECT_EQ(
      "/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL "
      "SECURITY INVOKER VIEW `v3` AS select `tv1`.`a` AS `a`,`tv1`.`b` AS "
      "`b`,`tv1`.`c` AS `c` from `tv1` */;",
      rewritten);

  EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
      R"(CREATE DEFINER=`root`@`localhost` FUNCTION `hello`(s CHAR(20)) RETURNS char(50) CHARSET utf8mb4
    DETERMINISTIC
RETURN CONCAT('Hello, ',s,'!'))",
      &rewritten));

  EXPECT_EQ(
      R"(CREATE DEFINER=`root`@`localhost` FUNCTION `hello`(s CHAR(20)) RETURNS char(50) CHARSET utf8mb4
    DETERMINISTIC
SQL SECURITY INVOKER
RETURN CONCAT('Hello, ',s,'!'))",
      rewritten);

  EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
      R"(CREATE DEFINER=`root`@`localhost` PROCEDURE `p1`()
BEGIN   UPDATE t1 SET counter = counter + 1;
END)",
      &rewritten));
  EXPECT_EQ(R"(CREATE DEFINER=`root`@`localhost` PROCEDURE `p1`()
SQL SECURITY INVOKER
BEGIN   UPDATE t1 SET counter = counter + 1;
END)",
            rewritten);

  EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
      R"(CREATE FUNCTION `func1`() RETURNS int(11)
    NO SQL
RETURN 0)",
      &rewritten));
  EXPECT_EQ(R"(CREATE FUNCTION `func1`() RETURNS int(11)
    NO SQL
SQL SECURITY INVOKER
RETURN 0)",
            rewritten);

  EXPECT_FALSE(compatibility::check_statement_for_sqlsecurity_clause(
      R"(CREATE FUNCTION `func2`() RETURNS int(11)
    NO SQL
    SQL SECURITY INVOKER
RETURN 0)",
      &rewritten));

  EXPECT_FALSE(compatibility::check_statement_for_sqlsecurity_clause(
      "/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL "
      "SECURITY INVOKER VIEW `v3` AS select `tv1`.`a` AS `a`,`tv1`.`b` AS "
      "`b`,`tv1`.`c` AS `c` from `tv1` */;",
      &rewritten));

  EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
      R"(create view tmp as select count(*) from s.t)", &rewritten));
  EXPECT_EQ(R"(create SQL SECURITY INVOKER
view tmp as select count(*) from s.t)",
            rewritten);

  EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
      R"(create function tmp() returns integer deterministic
return 1)",
      &rewritten));
  EXPECT_EQ(R"(create function tmp() returns integer deterministic
SQL SECURITY INVOKER
return 1)",
            rewritten);

  const std::vector<std::string> statements = {
      "IF @v > 0 THEN SELECT 1; ELSE SELECT 2; END IF",
      "case @v when 1 then select 1; else select 2; end case",
      "label1: BEGIN END",
      "label1:BEGIN END",
      "BEGIN END",
      "LOOP select 1; end loop",
      "while @v > 1 do select 1; end while",
      "repeat select 1; until @v > 1 end repeat",
      "alter table s.t",
      "analyze table s.t",
      "BINLOG 'str'",
      "call tmp",
      "change replication source to source_PORT = 1234",
      "check table s.t",
      "checksum table s.t",
      "clone LOCAL DATA DIRECTORY 'clone_dir'",
      "commit",
      "create table s.t1 (a int)",
      "deallocate prepare stmt",
      "delete from s.t",
      "desc s.t",
      "describe s.t",
      "explain s.t",
      "do 1",
      "drop table s.t",
      "execute stmt",
      "flush tables",
      "GET DIAGNOSTICS @v = ROW_COUNT",
      "start group_replication",
      "stop group_replication",
      "grant select on *.* to sample_user",
      "handler s.t OPEN",
      "IMPORT TABLE FROM 'sdi_file'",
      "Insert into s.t values (1)",
      "INSTALL PLUGIN name SONAME 'so'",
      "kill query 1",
      "lock instance for backup",
      "optimize table s.t",
      "CACHE INDEX i IN hot_cache",
      "load INDEX INto cache s.t",
      "PREPARE stmt FROM 'select 1'",
      "purge binary logs to 'log'",
      "release savepoint x",
      "rename table s.t to s.t1",
      "repair table s.t",
      "replace s.t VALUES (1)",
      "reset replica",
      "resignal",
      "restart",
      "revoke select on *.* from sample_user",
      "rollback",
      "savepoint x",
      "select 1",
      "(select 1)",
      "values row(1)",
      "(values row(1))",
      "table s.t",
      "(table s.t)",
      "set password to random",
      "show tables",
      "shutdown",
      "SIGNAL SQLSTATE '01000'",
      "truncate s.t",
      "UNINSTALL PLUGIN plugin_name",
      "UNlock instance",
      "update s.t set a = 1",
      "WITH cte1 AS (SELECT 1) update s.t set a = 1",
      "xa start 'xid'",
  };
  const std::string create_procedure = "create procedure ";
  const std::string routine_params = "() ";
  const std::string create_procedure_tmp =
      create_procedure + "tmp" + routine_params;
  const std::string sql_security_invoker = "SQL SECURITY INVOKER";
  const std::string sql_security_invoker_n = sql_security_invoker + "\n";

  for (const auto &stmt : statements) {
    SCOPED_TRACE(stmt);

    // create procedure statement with the given body
    EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
        create_procedure_tmp + stmt, &rewritten));
    EXPECT_EQ(create_procedure_tmp + sql_security_invoker_n + stmt, rewritten);

    const auto keyword = shcore::str_split(stmt, " ")[0];

    if ('(' != keyword[0]) {
      // create procedure statement with the given body, procedure name is the
      // first word from a statement (in most cases this is not a valid SQL)
      EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
          create_procedure + keyword + routine_params + stmt, &rewritten));
      EXPECT_EQ(create_procedure + keyword + routine_params +
                    sql_security_invoker_n + stmt,
                rewritten);

      const auto quoted_keyword = shcore::quote_identifier(keyword);

      // create procedure statement with the given body, procedure name is the
      // first word from a statement, quoted
      EXPECT_TRUE(compatibility::check_statement_for_sqlsecurity_clause(
          create_procedure + quoted_keyword + routine_params + stmt,
          &rewritten));
      EXPECT_EQ(create_procedure + quoted_keyword + routine_params +
                    sql_security_invoker_n + stmt,
                rewritten);
    }
  }

  const std::vector<std::string> types = {
      // int_type [ '(' NUM ')' ] [ SIGNED ] [ UNSIGNED ] [ ZEROFILL ]
      "INT4 (2) SIGNED",
      "MIDDLEINT UNSIGNED",
      // { REAL | DOUBLE | FLOAT8 } [ '(' NUM ',' NUM ')' ] [ SIGNED ]
      //                                                    [ UNSIGNED ]
      //                                                    [ ZEROFILL ]
      "REAL(4, 3) ZEROFILL",
      "DOUBLE SIGNED UNSIGNED",
      // { DOUBLE | FLOAT8 } PRECISION [ '(' NUM ',' NUM ')' ] [ SIGNED ]
      //                                                       [ UNSIGNED ]
      //                                                       [ ZEROFILL ]
      "FLOAT8 PRECISION (2,1) SIGNED ZEROFILL",
      "DOUBLE PRECISION UNSIGNED ZEROFILL",
      // numeric_type [ '(' NUM ',' NUM ')' ]  [ SIGNED ] [ UNSIGNED ]
      //                                       [ ZEROFILL ]
      "FLOAT4 (33, 4) UNSIGNED SIGNED",
      "DEC ZEROFILL SIGNED",
      // numeric_type '(' NUM ')'  [ SIGNED ] [ UNSIGNED ] [ ZEROFILL ]
      "FIXED(2) ZEROFILL UNSIGNED",
      // BIT
      "BIT",
      // BIT '(' NUM ')'
      "BIT(7)",
      // BOOL
      "BOOL",
      // BOOLEAN
      "BOOLEAN",
      // { CHAR | CHARACTER } '(' NUM ')' opt_charset_with_opt_binary
      "CHAR(8)",
      "CHARACTER (1) ASCII",
      // { CHAR | CHARACTER } opt_charset_with_opt_binary
      "CHAR BINARY ASCII",
      "CHARACTER ASCII BINARY",
      // NCHAR '(' NUM ')' [ BINARY ]
      "NCHAR(4)",
      "NCHAR(10) BINARY",
      // NATIONAL { CHAR | CHARACTER } '(' NUM ')' [ BINARY ]
      "NATIONAL CHAR (2)",
      "NATIONAL CHARACTER (2) BINARY",
      // NCHAR [ BINARY ]
      "NCHAR",
      "NCHAR BINARY",
      // NATIONAL { CHAR | CHARACTER } [ BINARY ]
      "NATIONAL CHAR BINARY",
      "NATIONAL CHARACTER",
      // BINARY '(' NUM ')'
      "BINARY(11)",
      // BINARY
      "BINARY",
      // { CHAR | CHARACTER } VARYING '(' NUM ')' opt_charset_with_opt_binary
      "CHAR VARYING (9) UNICODE",
      "CHARACTER VARYING (33) UNICODE BINARY",
      // { VARCHAR | VARCHARACTER } '(' NUM ')' opt_charset_with_opt_binary
      "VARCHAR (5) BINARY UNICODE",
      "VARCHARACTER (44) BYTE",
      // NATIONAL { VARCHAR | VARCHARACTER } '(' NUM ')' [ BINARY ]
      "NATIONAL VARCHAR(13)",
      "NATIONAL VARCHARACTER( 8 ) BINARY",
      // NVARCHAR '(' NUM ')' [ BINARY ]
      "NVARCHAR (6)",
      "NVARCHAR (7) BINARY",
      // NCHAR { VARCHAR | VARCHARACTER } '(' NUM ')' [ BINARY ]
      "NCHAR VARCHAR (4) BINARY",
      "NCHAR VARCHARACTER (32)",
      // NATIONAL { CHAR | CHARACTER } VARYING '(' NUM ')' [ BINARY ]
      "NATIONAL CHAR VARYING(2)",
      "NATIONAL CHARACTER VARYING(02) BINARY",
      // NCHAR VARYING '(' NUM ')' [ BINARY ]
      "NCHAR VARYING (77)",
      "NCHAR VARYING (77)BINARY",
      // VARBINARY '(' NUM ')'
      "VARBINARY(1)",
      // { SQL_TSI_YEAR | YEAR } [ '(' NUM ')' ]  [ SIGNED ] [ UNSIGNED ]
      //                                          [ ZEROFILL ]
      "SQL_TSI_YEAR(4) ZEROFILL UNSIGNED SIGNED",
      "YEAR SIGNED UNSIGNED SIGNED UNSIGNED",
      // DATE
      "DATE",
      // TIME [ '(' NUM ')' ]
      "TIME",
      "TIME (5)",
      // TIMESTAMP [ '(' NUM ')' ]
      "TIMESTAMP",
      "TIMESTAMP(6)",
      // DATETIME [ '(' NUM ')' ]
      "DATETIME",
      "DATETIME (2)",
      // TINYBLOB
      "TINYBLOB",
      // BLOB [ '(' NUM ')' ]
      "BLOB",
      "BLOB (55)",
      // spatial_type: GEOMETRY | GEOMCOLLECTION | GEOMETRYCOLLECTION | POINT |
      //               MULTIPOINT | LINESTRING | MULTILINESTRING | POLYGON |
      //               MULTIPOLYGON
      "GEOMETRY",
      // MEDIUMBLOB
      "MEDIUMBLOB",
      // LONGBLOB
      "LONGBLOB",
      // LONG VARBINARY
      "LONG VARBINARY",
      // LONG { CHAR | CHARACTER } VARYING opt_charset_with_opt_binary
      "LONG CHAR VARYING CHAR SET utf8mb4",
      "LONG CHARACTER VARYING CHARACTER SET latin1 BINARY",
      // LONG { VARCHAR | VARCHARACTER } opt_charset_with_opt_binary
      "LONG VARCHAR CHARSET BINARY",
      "LONG VARCHARACTER CHARSET BINARY BINARY",
      // TINYTEXT opt_charset_with_opt_binary
      "TINYTEXT BINARY",
      // TEXT [ '(' NUM ')' ] opt_charset_with_opt_binary
      "TEXT (8) BINARY CHAR SET ascii",
      "TEXT BINARY CHARACTER SET BINARY",
      // MEDIUMTEXT opt_charset_with_opt_binary
      "MEDIUMTEXT BINARY CHARSET greek",
      // LONGTEXT opt_charset_with_opt_binary
      "LONGTEXT CHARSET hebrew COLLATE hebrew_general_ci",
      // ENUM '(' string_list ')' opt_charset_with_opt_binary
      "ENUM ('a') CHARSET binary COLLATE BINARY",
      // SET '(' string_list ')' opt_charset_with_opt_binary
      "SET ('a','b' ,'c', 'd') CHARSET utf8mb4 COLLATE utf8mb4_esperanto_ci",
      // LONG opt_charset_with_opt_binary
      "LONG CHAR SET ascii",
      "LONG CHARACTER SET ascii COLLATE ascii_bin",
      "LONG",
      // SERIAL
      "SERIAL",
      // JSON
      "JSON",
  };

  const std::vector<std::string> characteristics = {
      "",
      "COMMENT 'string'",
      "LANGUAGE SQL",
      "NOT DETERMINISTIC",
      "CONTAINS SQL",
      "NO SQL",
      "READS SQL DATA",
      "MODIFIES SQL DATA",
      "SQL SECURITY DEFINER",
      sql_security_invoker.c_str(),
      "LANGUAGE SQL CONTAINS SQL MODIFIES SQL DATA",
  };

  const std::string create_function = "create function ";
  const std::string create_function_tmp =
      create_function + "tmp" + routine_params + "returns ";

  for (const auto &stmt : statements) {
    for (const auto &type : types) {
      for (const auto &chistics : characteristics) {
        SCOPED_TRACE(type + " - " + stmt + " - " + chistics);

        // functions cannot use (SELECT 1) type statements, as it is not allowed
        // to return a result set from a function
        if ('(' != stmt[0]) {
          const auto result = chistics != sql_security_invoker;

          // create function statement with the given body and return type, most
          // of these combinations are not valid SQL statements
          EXPECT_EQ(
              result,
              compatibility::check_statement_for_sqlsecurity_clause(
                  create_function_tmp + type + " " + chistics + " " + stmt,
                  &rewritten));

          if (result) {
            EXPECT_EQ(
                create_function_tmp + type + " " +
                    (shcore::str_beginswith(chistics.c_str(), "SQL SECURITY")
                         ? sql_security_invoker + " "
                         : chistics + " " + sql_security_invoker_n) +
                    stmt,
                rewritten);
          }
        }
      }
    }
  }
}

TEST_F(Compatibility_test, create_table_combo) {
  const auto EXPECT_MLE = [&](size_t i, const char *res) {
    std::string rewritten;
    check_create_table_for_data_index_dir_option(multiline[i], &rewritten);
    check_create_table_for_encryption_option(rewritten, &rewritten);
    check_create_table_for_engine_option(rewritten, &rewritten);
    check_create_table_for_tablespace_option(rewritten, &rewritten);
    check_statement_for_charset_option(rewritten, &rewritten);
    if (res)
      EXPECT_EQ(res, rewritten);
    else
      puts(rewritten.c_str());
  };

  const auto EXPECT_UNCHANGED = [&](size_t i) {
    std::string rewritten;
    check_create_table_for_data_index_dir_option(rogue[i], &rewritten);
    check_create_table_for_encryption_option(rewritten, &rewritten);
    check_create_table_for_engine_option(rewritten, &rewritten);
    check_create_table_for_tablespace_option(rewritten, &rewritten);
    check_statement_for_charset_option(rewritten, &rewritten);
    EXPECT_EQ(rogue[i], rewritten);
  };

  EXPECT_MLE(0,
             "CREATE TABLE t(i int)\n"
             "/* ENCRYPTION = 'N'*/  \n"
             "/* DATA DIRECTORY = '\\tmp' INDEX DIRECTORY = '\\tmp',*/ \n"
             "ENGINE = InnoDB");

  EXPECT_MLE(1,
             "CREATE TABLE t (i int PRIMARY KEY) ENGINE = InnoDB\n"
             "  /* DATA DIRECTORY = '/tmp'*/ \n"
             "  /* ENCRYPTION = 'y'*/ \n"
             "  PARTITION BY LIST (i) (\n"
             "    PARTITION p0 VALUES IN (0) ENGINE = InnoDB,\n"
             "    PARTITION p1 VALUES IN (1)\n"
             "    /* DATA DIRECTORY = '/tmp',*/ \n"
             "    ENGINE = InnoDB"
             "\n  )");

  EXPECT_MLE(
      2,
      "CREATE TABLE `tmq` (\n"
      "  `i` int(11) DEFAULT NULL\n"
      ") ENGINE=InnoDB  /* DATA DIRECTORY='/tmp/' INDEX DIRECTORY='/tmp/'*/ ");

  EXPECT_MLE(3,
             "CREATE TABLE `tmq` (\n"
             "  `i` int(11) DEFAULT NULL\n"
             ")  ENGINE=InnoDB  /* ENCRYPTION='N'*/ ");

  EXPECT_MLE(4,
             "CREATE TABLE `tmq` (\n"
             "  `i` int(11) NOT NULL,\n"
             "  PRIMARY KEY (`i`)\n"
             ") ENGINE=InnoDB \n"
             "/*!50100 PARTITION BY LIST (i)\n"
             "(PARTITION p0 VALUES IN (0) ENGINE = InnoDB,\n"
             " PARTITION p1 VALUES IN (1) -- DATA DIRECTORY = '/tmp'\n"
             " ENGINE = InnoDB) */");

  EXPECT_MLE(
      5,
      "CREATE TABLE `tmq` (\n"
      "  `i` int NOT NULL,\n"
      "  PRIMARY KEY (`i`)\n"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci\n"
      "/*!50100 PARTITION BY LIST (`i`)\n"
      "(PARTITION p0 VALUES IN (0) -- DATA DIRECTORY = '/tmp/'\n"
      " ENGINE = InnoDB,\n"
      " PARTITION p1 VALUES IN (1) -- DATA DIRECTORY = '/tmp/'\n"
      " ENGINE = InnoDB) */");

  EXPECT_MLE(6,
             "CREATE TABLE `tmq` (\n"
             "  `i` int(11) NOT NULL,\n"
             "  PRIMARY KEY (`i`)\n"
             ") ENGINE=InnoDB \n"
             "/*!50100 PARTITION BY LIST (i)\n"
             "(PARTITION p0 VALUES IN (0)  ENGINE = InnoDB,\n"
             " PARTITION p1 VALUES IN (1)  ENGINE = InnoDB) */");

  std::string rewritten;
  compatibility::check_statement_for_charset_option(multiline[7], &rewritten);
  compatibility::check_create_table_for_encryption_option(rewritten,
                                                          &rewritten);
  EXPECT_EQ(
      "CREATE DATABASE `dcs`  /*!80016 -- DEFAULT ENCRYPTION='N'\n"
      " */",
      rewritten);

  EXPECT_UNCHANGED(0);
  EXPECT_UNCHANGED(1);
}

TEST_F(Compatibility_test, indexes) {
  std::string rewritten;
  std::vector<std::string> idxs;

  EXPECT_NO_THROW(idxs = compatibility::check_create_table_for_indexes(
                      R"(CREATE TABLE `author` (
  `author` varchar(200) DEFAULT NULL,
  `age` int DEFAULT NULL,
  FULLTEXT KEY `author` (`author`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)",
                      false, &rewritten));
  ASSERT_EQ(1, idxs.size());
  EXPECT_EQ("FULLTEXT KEY `author` (`author`)", idxs[0]);
  EXPECT_EQ(rewritten, R"(CREATE TABLE `author` (
  `author` varchar(200) DEFAULT NULL,
  `age` int DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)");

  EXPECT_NO_THROW(idxs = compatibility::check_create_table_for_indexes(
                      R"(CREATE TABLE `opening_lines` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `opening_line` text,
  `author` varchar(200) DEFAULT NULL,
  `title` varchar(200) DEFAULT NULL,
  PRIMARY KEY (`id`),
  FULLTEXT INDEX `idx` (`opening_line`),
  FULLTEXT key `idx2` (`author`,`title`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)",
                      false, &rewritten, true));

  ASSERT_EQ(2, idxs.size());
  EXPECT_EQ(
      "ALTER TABLE `opening_lines` ADD FULLTEXT INDEX `idx` (`opening_line`);",
      idxs[0]);
  EXPECT_EQ(
      "ALTER TABLE `opening_lines` ADD FULLTEXT key `idx2` "
      "(`author`,`title`);",
      idxs[1]);
  EXPECT_EQ(rewritten, R"(CREATE TABLE `opening_lines` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `opening_line` text,
  `author` varchar(200) DEFAULT NULL,
  `title` varchar(200) DEFAULT NULL,
  PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)");

  EXPECT_NO_THROW(idxs = compatibility::check_create_table_for_indexes(
                      R"(CREATE TABLE `opening_lines` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `opening_line` text,
  `author` varchar(200) DEFAULT NULL,
  `title` varchar(200) DEFAULT NULL,
  PRIMARY KEY (`id`),
  FULLTEXT INDEX `idx` (`opening_line`),
  FULLTEXT key `idx2` (`author`,`title`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)",
                      true, &rewritten, true));

  ASSERT_EQ(2, idxs.size());
  EXPECT_EQ(
      "ALTER TABLE `opening_lines` ADD FULLTEXT INDEX `idx` (`opening_line`);",
      idxs[0]);
  EXPECT_EQ(
      "ALTER TABLE `opening_lines` ADD FULLTEXT key `idx2` "
      "(`author`,`title`);",
      idxs[1]);
  EXPECT_EQ(rewritten, R"(CREATE TABLE `opening_lines` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `opening_line` text,
  `author` varchar(200) DEFAULT NULL,
  `title` varchar(200) DEFAULT NULL,
  PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)");

  EXPECT_NO_THROW(idxs = compatibility::check_create_table_for_indexes(
                      R"(CREATE TABLE IF NOT EXISTS `films` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `opening_line` text,
  `author` varchar(200) DEFAULT NULL,
  `title` varchar(200) DEFAULT NULL,
  `oli` int unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `oli` (`oli`),
  FULLTEXT KEY (`opening_line`),
  CONSTRAINT `films_ibfk_1` FOREIGN KEY (`oli`) REFERENCES `opening_lines` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)",
                      false, &rewritten, true));
  ASSERT_EQ(3, idxs.size());
  EXPECT_EQ("ALTER TABLE `films` ADD KEY `oli` (`oli`);", idxs[0]);
  EXPECT_EQ("ALTER TABLE `films` ADD FULLTEXT KEY (`opening_line`);", idxs[1]);
  EXPECT_EQ(
      "ALTER TABLE `films` ADD CONSTRAINT `films_ibfk_1` FOREIGN KEY (`oli`) "
      "REFERENCES `opening_lines` (`id`);",
      idxs[2]);
  EXPECT_EQ(rewritten, R"(CREATE TABLE IF NOT EXISTS `films` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `opening_line` text,
  `author` varchar(200) DEFAULT NULL,
  `title` varchar(200) DEFAULT NULL,
  `oli` int unsigned NOT NULL,
  PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)");

  EXPECT_NO_THROW(idxs = compatibility::check_create_table_for_indexes(
                      R"(CREATE TABLE IF NOT EXISTS `films` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `opening_line` text,
  `author` varchar(200) DEFAULT NULL,
  `title` varchar(200) DEFAULT NULL,
  `oli` int unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `oli` (`oli`),
  FULLTEXT KEY (`opening_line`),
  CONSTRAINT `films_ibfk_1` FOREIGN KEY (`oli`) REFERENCES `opening_lines` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)",
                      true, &rewritten, true));
  ASSERT_EQ(1, idxs.size());
  EXPECT_EQ("ALTER TABLE `films` ADD FULLTEXT KEY (`opening_line`);", idxs[0]);
  EXPECT_EQ(rewritten, R"(CREATE TABLE IF NOT EXISTS `films` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `opening_line` text,
  `author` varchar(200) DEFAULT NULL,
  `title` varchar(200) DEFAULT NULL,
  `oli` int unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `oli` (`oli`),
  CONSTRAINT `films_ibfk_1` FOREIGN KEY (`oli`) REFERENCES `opening_lines` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)");

  EXPECT_NO_THROW(idxs = compatibility::check_create_table_for_indexes(
                      R"(CREATE TABLE part_tbl2 (
  pk INT PRIMARY KEY,
  xx INT GENERATED ALWAYS AS (pk+2)
) PARTITION BY HASH(pk) (
    PARTITION p1
    DATA DIRECTORY = '${TMPDIR}/test datadir',
    PARTITION p2
    DATA DIRECTORY = '${TMPDIR}/test datadir2'
  );)",
                      false));
  EXPECT_TRUE(idxs.empty());

  // auto_increment key should not be extracted
  EXPECT_NO_THROW(idxs = compatibility::check_create_table_for_indexes(
                      R"(CREATE TABLE `aik` (
  `id` int NOT NULL,
  `uai` int NOT NULL AUTO_INCREMENT,
  `data` text,
  PRIMARY KEY (`id`),
  KEY `uai` (`uai`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci)",
                      false, &rewritten, true));
  EXPECT_TRUE(idxs.empty());

  EXPECT_NO_THROW(idxs = compatibility::check_create_table_for_indexes(
                      R"(CREATE TABLE IF NOT EXISTS `page_restrictions` (
  `pr_page` int NOT NULL DEFAULT '0',
  `pr_type` varbinary(255) NOT NULL DEFAULT '',
  `pr_level` varbinary(255) NOT NULL DEFAULT '',
  `pr_cascade` tinyint NOT NULL DEFAULT '0',
  `pr_user` int unsigned DEFAULT NULL,
  `pr_expiry` varbinary(14) DEFAULT NULL,
  `pr_id` int unsigned NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`pr_page`,`pr_type`),
  UNIQUE KEY `pr_id` (`pr_id`),
  KEY `pr_page` (`pr_page`),
  KEY `pr_typelevel` (`pr_type`,`pr_level`),
  KEY `pr_level` (`pr_level`),
  KEY `pr_cascade` (`pr_cascade`)
) ENGINE=InnoDB AUTO_INCREMENT=854046 DEFAULT CHARSET=binary;)",
                      false, &rewritten, true));
  ASSERT_EQ(4, idxs.size());
  EXPECT_EQ("ALTER TABLE `page_restrictions` ADD KEY `pr_page` (`pr_page`);",
            idxs[0]);
  EXPECT_EQ(
      "ALTER TABLE `page_restrictions` ADD KEY `pr_typelevel` "
      "(`pr_type`,`pr_level`);",
      idxs[1]);
  EXPECT_EQ("ALTER TABLE `page_restrictions` ADD KEY `pr_level` (`pr_level`);",
            idxs[2]);
  EXPECT_EQ(
      "ALTER TABLE `page_restrictions` ADD KEY `pr_cascade` (`pr_cascade`);",
      idxs[3]);
}

TEST_F(Compatibility_test, indexes_recreation) {
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(shcore::get_connection_options(_mysql_uri));
  session->execute("drop database if exists index_recreation_test");
  session->execute("create database index_recreation_test");
  session->execute("use index_recreation_test");

  const auto check_recreation_one = [&](const std::string &table_name,
                                        const char *ddl, int index_count,
                                        bool fulltext_only) {
    SCOPED_TRACE(table_name);

    ASSERT_NO_THROW(session->execute(ddl));
    auto create_table = session->query("show create table " + table_name)
                            ->fetch_one()
                            ->get_string(1);
    std::string rewritten;
    std::vector<std::string> idxs;
    EXPECT_NO_THROW(idxs = compatibility::check_create_table_for_indexes(
                        create_table, fulltext_only, &rewritten, true));
    EXPECT_EQ(index_count, idxs.size());
    if (idxs.size() == 0) return;
    ASSERT_NO_THROW(session->execute("drop table " + table_name));
    ASSERT_NO_THROW(session->execute(rewritten));
    for (const auto &q : idxs) {
      ASSERT_NO_THROW(session->execute(q));
    }
    EXPECT_EQ(create_table, session->query("show create table " + table_name)
                                ->fetch_one()
                                ->get_string(1));
  };

  const auto check_recreation = [&](const std::string &table_name,
                                    const char *ddl, int index_count,
                                    int ft_index_count) {
    check_recreation_one(table_name, ddl, index_count, false);
    ASSERT_NO_THROW(session->execute("drop table " + table_name));
    check_recreation_one(table_name, ddl, ft_index_count, true);
    ASSERT_NO_THROW(session->execute("drop table " + table_name));
  };

  check_recreation("t1", "create table t1 (i int primary key, j int unique)", 1,
                   0);
  check_recreation(
      "t2",
      "create table t2 (i int, j int, t text, fulltext (t), primary key (i,j))",
      1, 1);

  check_recreation("t3", R"(CREATE TABLE t3 (
    id INT NOT NULL,
    `SPATIAL` TEXT,
    `FULLTEXT` LONGTEXT,
    FULLTEXT `UNIQUE` (`SPATIAL`),
    FULLTEXT `SPATIAL` (`SPATIAL`, `FULLTEXT`),
    PRIMARY KEY (id)
) ENGINE=INNODB;)",
                   2, 2);

  check_recreation("t4", R"(CREATE TABLE t4 (
    id INT NOT NULL,
    t1 TEXT,
    `fulltext` VARCHAR(32),
    FULLTEXT KEY `fulltext` (t1),
    UNIQUE (`fulltext`),
    PRIMARY KEY (id)
) ENGINE=INNODB;)",
                   2, 1);

  check_recreation_one("parent", R"(CREATE TABLE parent (
    id INT NOT NULL,
    PRIMARY KEY (id)
) ENGINE=INNODB;)",
                       0, false);

  check_recreation_one("child", R"(CREATE TABLE child (
    id INT,
    category INT NOT NULL,
    parent_id INT CHECK (parent_id <> 666),
    first_words TEXT,
    PRIMARY KEY(category, id),
    INDEX par_ind (parent_id),
    FULLTEXT(first_words),
    FOREIGN KEY (parent_id)
        REFERENCES parent(id)
        ON DELETE CASCADE
) ENGINE=INNODB;)",
                       3, false);

  check_recreation_one("parentf", R"(CREATE TABLE parentf (
    id INT NOT NULL,
    PRIMARY KEY (id)
) ENGINE=INNODB;)",
                       0, true);

  check_recreation_one("childf", R"(CREATE TABLE childf (
    id INT,
    category INT NOT NULL,
    parent_id INT CHECK (parent_id <> 666),
    first_words TEXT,
    PRIMARY KEY(category, id),
    INDEX par_ind (parent_id),
    FULLTEXT(first_words),
    FOREIGN KEY (parent_id)
        REFERENCES parentf(id)
        ON DELETE CASCADE
) ENGINE=INNODB;)",
                       1, true);

  check_recreation("pair", R"(CREATE TABLE pair (
    no INT NOT NULL AUTO_INCREMENT,
    product_category INT NOT NULL,
    product_id INT NOT NULL,
    customer_id INT NOT NULL UNIQUE,
    customer_hash INT NOT NULL,

    CHECK (customer_id >= customer_hash),
    PRIMARY KEY(no),
    INDEX (product_category, product_id),
    INDEX (customer_id),

    FOREIGN KEY (product_category, product_id)
      REFERENCES child(category, id)
      ON UPDATE CASCADE ON DELETE RESTRICT,

    FOREIGN KEY (customer_id)
      REFERENCES parent(id)
)   ENGINE=INNODB;)",
                   5, 0);

  check_recreation("jemp", R"(CREATE TABLE jemp (
    c JSON,
    g INT GENERATED ALWAYS AS (c->"$.id"),
    geo GEOMETRY NOT NULL,
    SPATIAL INDEX(geo),
    INDEX i (g)
    );)",
                   2, 0);

  check_recreation("part_tbl2", R"(CREATE TABLE part_tbl2 (
  pk INT PRIMARY KEY,
  idx INT,
  xx INT GENERATED ALWAYS AS (pk+2),
  INDEX (idx)
) PARTITION BY HASH(pk) (
    PARTITION p1,
    PARTITION p2    
  );)",
                   1, 0);

  session->execute("drop database index_recreation_test");
}

std::vector<std::string> tokenize(const std::string &s) {
  std::vector<std::string> tokens;
  mysqlshdk::utils::SQL_iterator it(s, 0, false);
  while (it.valid()) {
    tokens.push_back(it.next_token());
  }
  return tokens;
}

TEST_F(Compatibility_test, filter_revoke_parsing) {
  // this just checks parsing of revoke stmts

  tests::String_combinator combinator(
      "REVOKE {grants} ON {type} {level} FROM {user}");

  std::vector<std::string> grant_test_cases = {
      "SELECT", "SELECT, INSERT, DELETE", "CREATE TEMPORARY TABLES, EVENT",
      "SELECT (col1, col2) ,INSERT(col)"};

  combinator.add("grants", grant_test_cases);
  combinator.add("type", {"", "TABLE", "FUNCTION", "PROCEDURE"});
  combinator.add("level", {"*", "*.*", "bla.*", "`FROM`.`TO`", "bla.` `"});
  combinator.add("user", {"root@localhost", "root@'%'", "`FROM`@localhost",
                          "'FROM'@localhost", "\"FROM\"@localhost"});

  std::string stmt = combinator.get();
  while (!stmt.empty()) {
    SCOPED_TRACE(stmt);

    std::string expected_grants = combinator.get("grants");
    expected_grants = shcore::str_replace(expected_grants, " ", "");

    auto filter = [&](bool is_revoke, const std::string &priv_type,
                      const std::string &column_list,
                      const std::string &object_type,
                      const std::string &priv_level) {
      EXPECT_TRUE(is_revoke);

      // strip privs we've seen from the expected list
      std::string r = shcore::str_replace(
          expected_grants,
          shcore::str_replace(priv_type, " ", "") + column_list, "");
      EXPECT_NE(r, expected_grants) << priv_type << "/" << column_list;
      expected_grants = r;

      EXPECT_EQ(combinator.get("type"), object_type);
      EXPECT_EQ(combinator.get("level"), priv_level);

      return true;
    };

    EXPECT_EQ(tokenize(stmt), tokenize(filter_grant_or_revoke(stmt, filter)));

    // this should be empty now
    expected_grants = shcore::str_replace(expected_grants, ",", "");
    EXPECT_EQ("", expected_grants);

    stmt = combinator.get();
  }

  EXPECT_EQ(
      "REVOKE role@'%' FROM 'admin'@'%';",
      filter_grant_or_revoke(
          "REVOKE role@'%' FROM 'admin'@'%';",
          [&](bool is_revoke, const std::string &priv_type,
              const std::string &column_list, const std::string &object_type,
              const std::string &priv_level) {
            EXPECT_TRUE(is_revoke);
            EXPECT_EQ(priv_type, "role@'%'");
            EXPECT_EQ(column_list, "");
            EXPECT_EQ(object_type, "");
            EXPECT_EQ(priv_level, "");
            return true;
          }));

  EXPECT_EQ(
      "REVOKE PROXY ON ''@'' FROM 'admin'@'%';",
      filter_grant_or_revoke(
          "REVOKE PROXY ON ''@'' FROM 'admin'@'%';",
          [&](bool is_revoke, const std::string &priv_type,
              const std::string &column_list, const std::string &object_type,
              const std::string &priv_level) {
            EXPECT_TRUE(is_revoke);
            EXPECT_EQ(priv_type, "PROXY");
            EXPECT_EQ(column_list, "");
            EXPECT_EQ(object_type, "");
            EXPECT_EQ(priv_level, "");
            return true;
          }));
}

TEST_F(Compatibility_test, filter_grant_parsing) {
  tests::String_combinator combinator(
      "GRANT {grants} ON {type} {level} TO {user}{extras}");

  std::vector<std::string> grant_test_cases = {
      "SELECT", "SELECT, INSERT, DELETE", "CREATE TEMPORARY TABLES, EVENT",
      "SELECT (col1, col2) ,INSERT(col)"};

  combinator.add("grants", grant_test_cases);
  combinator.add("type", {"", "TABLE", "FUNCTION", "PROCEDURE"});
  combinator.add("level",
                 {"*", "*.*", "bla.*", "a.b", "`FROM`.`TO`", "bla.` `"});
  combinator.add("user",
                 {"root@localhost", "root@'%'", "`FROM`@localhost",
                  "'FROM'@localhost", "\"FROM\"@localhost", "`aaa`@`bbbb`"});
  combinator.add("extras", {" WITH GRANT OPTION", ""});

  std::string stmt = combinator.get();
  while (!stmt.empty()) {
    SCOPED_TRACE(stmt);

    std::string expected_grants = combinator.get("grants");
    expected_grants = shcore::str_replace(expected_grants, " ", "");

    auto filter = [&](bool is_revoke, const std::string &priv_type,
                      const std::string &column_list,
                      const std::string &object_type,
                      const std::string &priv_level) {
      EXPECT_FALSE(is_revoke);

      // strip privs we've seen from the expected list
      std::string r = shcore::str_replace(
          expected_grants,
          shcore::str_replace(priv_type, " ", "") + column_list, "");
      EXPECT_NE(r, expected_grants) << priv_type << "/" << column_list << "/";
      expected_grants = r;

      EXPECT_EQ(combinator.get("type"), object_type);
      EXPECT_EQ(combinator.get("level"), priv_level);

      return true;
    };

    EXPECT_EQ(tokenize(stmt), tokenize(filter_grant_or_revoke(stmt, filter)));

    // this should be empty now
    expected_grants = shcore::str_replace(expected_grants, ",", "");
    EXPECT_EQ("", expected_grants);

    stmt = combinator.get();
  }

  EXPECT_EQ(
      "GRANT PROXY ON ''@'' TO 'admin'@'%' WITH GRANT OPTION;",
      filter_grant_or_revoke(
          "GRANT PROXY ON ''@'' TO 'admin'@'%' WITH GRANT OPTION;",
          [&](bool is_revoke, const std::string &priv_type,
              const std::string &column_list, const std::string &object_type,
              const std::string &priv_level) {
            EXPECT_FALSE(is_revoke);
            EXPECT_EQ(priv_type, "PROXY");
            EXPECT_EQ(column_list, "");
            EXPECT_EQ(object_type, "");
            EXPECT_EQ(priv_level, "");
            return true;
          }));

  EXPECT_EQ(
      "GRANT `administrator`@`%` TO `admin`@`%` WITH ADMIN OPTION;",
      filter_grant_or_revoke(
          "GRANT `administrator`@`%` TO `admin`@`%` WITH ADMIN OPTION;",
          [&](bool is_revoke, const std::string &priv_type,
              const std::string &column_list, const std::string &object_type,
              const std::string &priv_level) {
            EXPECT_FALSE(is_revoke);
            EXPECT_EQ(priv_type, "`administrator`@`%`");
            EXPECT_EQ(column_list, "");
            EXPECT_EQ(object_type, "");
            EXPECT_EQ(priv_level, "");
            return true;
          }));

  EXPECT_EQ(
      "GRANT administrator TO `admin`@`%` WITH ADMIN OPTION;",
      filter_grant_or_revoke(
          "GRANT administrator TO `admin`@`%` WITH ADMIN OPTION;",
          [&](bool is_revoke, const std::string &priv_type,
              const std::string &column_list, const std::string &object_type,
              const std::string &priv_level) {
            EXPECT_FALSE(is_revoke);
            EXPECT_EQ(priv_type, "administrator");
            EXPECT_EQ(column_list, "");
            EXPECT_EQ(object_type, "");
            EXPECT_EQ(priv_level, "");
            return true;
          }));
}

TEST_F(Compatibility_test, filter_revoke) {
  auto update_filter = [](bool is_revoke, const std::string &priv_type,
                          const std::string &, const std::string &,
                          const std::string &priv_level) {
    EXPECT_TRUE(is_revoke);

    if (priv_level == "mysql.*") {
      if (priv_type == "insert" || priv_type == "delete" ||
          priv_type == "update")
        return false;
    }
    return true;
  };

  EXPECT_EQ("REVOKE select on mysql.* from root@localhost",
            filter_grant_or_revoke("REVOKE select, insert, update, delete on "
                                   "mysql.* from root@localhost",
                                   update_filter));

  EXPECT_EQ(
      "REVOKE select, insert, update, delete on mysqlx.* from root@localhost",
      filter_grant_or_revoke("REVOKE select, insert, update, delete on "
                             "mysqlx.* from root@localhost",
                             update_filter));

  EXPECT_EQ("revoke select(col1) on mysql.* from root@localhost",
            filter_grant_or_revoke(
                "revoke select (col1), insert (col2), update, delete on "
                "mysql.* from root@localhost",
                update_filter));

  EXPECT_EQ("", filter_grant_or_revoke("revoke insert, update, delete on "
                                       "mysql.* from root@localhost",
                                       update_filter));
}

TEST_F(Compatibility_test, filter_grant) {
  auto update_filter = [](bool is_revoke, const std::string &priv_type,
                          const std::string &, const std::string &,
                          const std::string &priv_level) {
    EXPECT_FALSE(is_revoke);

    if (priv_level == "mysql.*") {
      if (priv_type == "insert" || priv_type == "delete" ||
          priv_type == "update")
        return false;
    }
    return true;
  };

  EXPECT_EQ("GRANT select on mysql.* to root@localhost",
            filter_grant_or_revoke("GRANT select, insert, update, delete on "
                                   "mysql.* to root@localhost",
                                   update_filter));

  EXPECT_EQ(
      "GRANT select, insert, update, delete on mysqlx.* to root@localhost",
      filter_grant_or_revoke("GRANT select, insert, update, delete on "
                             "mysqlx.* to root@localhost",
                             update_filter));

  EXPECT_EQ("GRANT select(col1) on mysql.* to root@localhost",
            filter_grant_or_revoke(
                "GRANT select (col1), insert (col2), update, delete on "
                "mysql.* to root@localhost",
                update_filter));

  EXPECT_EQ("", filter_grant_or_revoke("grant insert, update, delete on "
                                       "mysql.* to root@localhost",
                                       update_filter));
}

TEST_F(Compatibility_test, filter_grant_malformed) {
  auto filter = [](bool, const std::string &, const std::string &,
                   const std::string &, const std::string &) { return true; };

  EXPECT_THROW(filter_grant_or_revoke("grant", filter), std::runtime_error);

  EXPECT_THROW(filter_grant_or_revoke("grant on", filter), std::runtime_error);

  EXPECT_THROW(filter_grant_or_revoke("grant on table", filter),
               std::runtime_error);

  EXPECT_THROW(filter_grant_or_revoke("revoke *.*", filter),
               std::runtime_error);

  EXPECT_THROW(filter_grant_or_revoke("revoke from", filter),
               std::runtime_error);

  EXPECT_THROW(filter_grant_or_revoke("revoke from (", filter),
               std::runtime_error);

  EXPECT_THROW(filter_grant_or_revoke("revoke x (", filter),
               std::runtime_error);

  EXPECT_THROW(filter_grant_or_revoke("revoke on all on bla", filter),
               std::runtime_error);
}

TEST_F(Compatibility_test, check_create_user_for_authentication_plugin) {
  const std::string exception =
      "This check can be only performed on CREATE USER statements";

  // unsupported statements
  EXPECT_THROW_LIKE(check_create_user_for_authentication_plugin(""),
                    std::invalid_argument, exception);
  EXPECT_THROW_LIKE(check_create_user_for_authentication_plugin("CREATE"),
                    std::invalid_argument, exception);
  EXPECT_THROW_LIKE(check_create_user_for_authentication_plugin("create Table"),
                    std::invalid_argument, exception);

  // statements without authentication plugin
  EXPECT_EQ("",
            check_create_user_for_authentication_plugin("CREATE USER r@l;"));
  EXPECT_EQ(
      "", check_create_user_for_authentication_plugin("CREATE USER 'r'@'l';"));
  EXPECT_EQ("",
            check_create_user_for_authentication_plugin("CREATE USER 'r'@l;"));
  EXPECT_EQ("",
            check_create_user_for_authentication_plugin("CREATE USER r@'l';"));
  EXPECT_EQ("", check_create_user_for_authentication_plugin(
                    "CREATE USER if not exists 'r'@'l';"));
  EXPECT_EQ("", check_create_user_for_authentication_plugin(
                    "CREATE USER r@l IDENTIFIED BY 'pass';"));

  // unknown plugin not in the default list
  EXPECT_EQ("plugin", check_create_user_for_authentication_plugin(
                          "CREATE USER r@l IDENTIFIED with 'plugin';"));
  EXPECT_EQ("plugin", check_create_user_for_authentication_plugin(
                          "CREATE USER 'r'@'l' identified WITH 'plugin';"));
  EXPECT_EQ("plugin", check_create_user_for_authentication_plugin(
                          "CREATE USER r@'l' identified WITH 'plugin';"));
  EXPECT_EQ("plugin", check_create_user_for_authentication_plugin(
                          "CREATE USER 'r'@l identified WITH 'plugin';"));
  EXPECT_EQ("plugin",
            check_create_user_for_authentication_plugin(
                "CREATE USER if NOT Exists 'r'@'l' IDENTIFIED WITH 'plugin';"));
  EXPECT_EQ("plugin", check_create_user_for_authentication_plugin(
                          "CREATE USER r@l IDENTIFIED WITH \"plugin\""));
  EXPECT_EQ("plugin", check_create_user_for_authentication_plugin(
                          "CREATE USER r@l IDENTIFIED WITH `plugin`"));
  EXPECT_EQ("plugin", check_create_user_for_authentication_plugin(
                          "CREATE USER r@l IDENTIFIED WITH plugin"));

  // plugins on the default list
  EXPECT_EQ("", check_create_user_for_authentication_plugin(
                    "CREATE USER r@l IDENTIFIED WITH 'caching_sha2_password'"));
  EXPECT_EQ("", check_create_user_for_authentication_plugin(
                    "CREATE USER r@l IDENTIFIED WITH 'mysql_native_password'"));
  EXPECT_EQ("", check_create_user_for_authentication_plugin(
                    "CREATE USER r@l IDENTIFIED WITH 'sha256_password'"));

  // disallowed plugins
  EXPECT_EQ("authentication_ldap_sasl",
            check_create_user_for_authentication_plugin(
                "CREATE USER r@l IDENTIFIED WITH 'authentication_ldap_sasl'"));
  EXPECT_EQ(
      "authentication_ldap_simple",
      check_create_user_for_authentication_plugin(
          "CREATE USER r@l IDENTIFIED WITH 'authentication_ldap_simple'"));
  EXPECT_EQ("authentication_pam",
            check_create_user_for_authentication_plugin(
                "CREATE USER r@l IDENTIFIED WITH 'authentication_pam'"));
  EXPECT_EQ("authentication_windows",
            check_create_user_for_authentication_plugin(
                "CREATE USER r@l IDENTIFIED WITH 'authentication_windows'"));

  // plugins which are not enabled
  EXPECT_EQ("auth_socket",
            check_create_user_for_authentication_plugin(
                "CREATE USER r@l IDENTIFIED WITH 'auth_socket'"));
  EXPECT_EQ("mysql_no_login",
            check_create_user_for_authentication_plugin(
                "CREATE USER r@l IDENTIFIED WITH 'mysql_no_login'"));

  // non-default list
  EXPECT_EQ("",
            check_create_user_for_authentication_plugin(
                "CREATE USER r@l IDENTIFIED WITH 'plugin_a'", {"plugin_a"}));
  EXPECT_EQ("PLUGIN_A",
            check_create_user_for_authentication_plugin(
                "CREATE USER r@l IDENTIFIED WITH 'PLUGIN_A'", {"plugin_a"}));
  EXPECT_EQ("plugin_b",
            check_create_user_for_authentication_plugin(
                "CREATE USER r@l IDENTIFIED WITH 'plugin_b'", {"plugin_a"}));
}

TEST_F(Compatibility_test, check_create_user_for_empty_password) {
  const std::string exception =
      "This check can be only performed on CREATE USER statements";

  // unsupported statements
  EXPECT_THROW_LIKE(check_create_user_for_empty_password(""),
                    std::invalid_argument, exception);
  EXPECT_THROW_LIKE(check_create_user_for_empty_password("CREATE"),
                    std::invalid_argument, exception);
  EXPECT_THROW_LIKE(check_create_user_for_empty_password("create Table"),
                    std::invalid_argument, exception);

  // statements without authentication plugin
  EXPECT_EQ(true, check_create_user_for_empty_password("CREATE USER r@l"));
  EXPECT_EQ(true, check_create_user_for_empty_password("CREATE USER r@l;"));
  EXPECT_EQ(true, check_create_user_for_empty_password("CREATE USER 'r'@'l';"));
  EXPECT_EQ(true, check_create_user_for_empty_password("CREATE USER 'r'@l;"));
  EXPECT_EQ(true, check_create_user_for_empty_password("CREATE USER r@'l';"));

  EXPECT_EQ(true, check_create_user_for_empty_password(
                      "CREATE USER r@l REQUIRE NONE"));

  EXPECT_EQ(true, check_create_user_for_empty_password(
                      "CREATE USER if not exists 'r'@'l';"));
  EXPECT_EQ(false, check_create_user_for_empty_password(
                       "CREATE USER if not exists r@l IDENTIFIED BY 'pass';"));

  EXPECT_EQ(true, check_create_user_for_empty_password(
                      "CREATE USER r@l IDENTIFIED BY \"\";"));
  EXPECT_EQ(false, check_create_user_for_empty_password(
                       "CREATE USER r@l IDENTIFIED BY \"pass\";"));

  EXPECT_EQ(true, check_create_user_for_empty_password(
                      "CREATE USER r@l IDENTIFIED BY PASSWORD '';"));
  EXPECT_EQ(false, check_create_user_for_empty_password(
                       "CREATE USER r@l IDENTIFIED BY PASSWORD 'pass';"));

  EXPECT_EQ(false, check_create_user_for_empty_password(
                       "CREATE USER r@l IDENTIFIED BY RANDOM PASSWORD;"));

  EXPECT_EQ(true, check_create_user_for_empty_password(
                      "CREATE USER r@l IDENTIFIED WITH 'sha256_password'"));

  EXPECT_EQ(true, check_create_user_for_empty_password(
                      "CREATE USER r@l IDENTIFIED WITH sha256_password BY ''"));
  EXPECT_EQ(false,
            check_create_user_for_empty_password(
                "CREATE USER r@l IDENTIFIED WITH 'sha256_password' BY 'pass'"));

  EXPECT_EQ(false, check_create_user_for_empty_password(
                       "CREATE USER r@l IDENTIFIED WITH 'sha256_password' BY "
                       "RANDOM PASSWORD;"));

  EXPECT_EQ(false,
            check_create_user_for_empty_password(
                "CREATE USER r@l IDENTIFIED WITH 'sha256_password' AS '';"));

  EXPECT_EQ(
      true,
      check_create_user_for_empty_password(
          "CREATE USER r@l IDENTIFIED WITH 'sha256_password' REQUIRE NONE"));
}

TEST_F(Compatibility_test, convert_create_user_to_create_role) {
  EXPECT_EQ("CREATE ROLE IF NOT EXISTS r@l",
            convert_create_user_to_create_role("CREATE USER r@l"));
  EXPECT_EQ("CREATE ROLE IF NOT EXISTS 'r'@'l'",
            convert_create_user_to_create_role("CREATE USER 'r'@'l'"));
  EXPECT_EQ("CREATE ROLE IF NOT EXISTS 'r'@l",
            convert_create_user_to_create_role("CREATE USER 'r'@l"));
  EXPECT_EQ("CREATE ROLE IF NOT EXISTS r@'l'",
            convert_create_user_to_create_role("CREATE USER r@'l'"));

  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l REQUIRE NONE",
      convert_create_user_to_create_role("CREATE USER r@l REQUIRE NONE"));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l IDENTIFIED BY \"\"",
      convert_create_user_to_create_role("CREATE USER r@l IDENTIFIED BY \"\""));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l IDENTIFIED BY 'pass'",
      convert_create_user_to_create_role(
          "CREATE USER r@l IDENTIFIED BY 'pass'"));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l PASSWORD EXPIRE DEFAULT",
      convert_create_user_to_create_role(
          "CREATE USER r@l PASSWORD EXPIRE DEFAULT"));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l PASSWORD EXPIRE DEFAULT PASSWORD HISTORY DEFAULT",
      convert_create_user_to_create_role(
          "CREATE USER r@l PASSWORD EXPIRE DEFAULT PASSWORD HISTORY DEFAULT"));

  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l DEFAULT ROLE role",
      convert_create_user_to_create_role("CREATE USER r@l DEFAULT ROLE role"));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l DEFAULT ROLE role;\n"
      "ALTER USER r@l IDENTIFIED WITH 'sha256_password'",
      convert_create_user_to_create_role(
          "CREATE USER r@l IDENTIFIED WITH 'sha256_password' DEFAULT ROLE "
          "role"));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l DEFAULT ROLE role;\n"
      "ALTER USER r@l EXPIRE DEFAULT",
      convert_create_user_to_create_role(
          "CREATE USER r@l DEFAULT ROLE role EXPIRE DEFAULT"));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l DEFAULT ROLE role;\n"
      "ALTER USER r@l IDENTIFIED WITH 'sha256_password' EXPIRE DEFAULT",
      convert_create_user_to_create_role(
          "CREATE USER r@l IDENTIFIED WITH 'sha256_password' DEFAULT ROLE role "
          "EXPIRE DEFAULT "));

  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l DEFAULT ROLE r@l, 'r'@'l'",
      convert_create_user_to_create_role(
          "CREATE USER r@l DEFAULT ROLE r@l, 'r'@'l'"));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l DEFAULT ROLE 'r'@'l' ,'r'@l",
      convert_create_user_to_create_role(
          "CREATE USER r@l DEFAULT ROLE 'r'@'l' ,'r'@l "));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l DEFAULT ROLE 'r'@l , r@'l';\n"
      "ALTER USER r@l EXPIRE DEFAULT",
      convert_create_user_to_create_role(
          "CREATE USER r@l DEFAULT ROLE 'r'@l , r@'l' EXPIRE DEFAULT"));
  EXPECT_EQ(
      "CREATE ROLE IF NOT EXISTS r@l;\n"
      "ALTER USER r@l DEFAULT ROLE r@'l',r@l;\n"
      "ALTER USER r@l EXPIRE DEFAULT",
      convert_create_user_to_create_role(
          "CREATE USER r@l DEFAULT ROLE r@'l',r@l EXPIRE DEFAULT"));
}

TEST_F(Compatibility_test, add_invisible_pk) {
  const auto EXPECT_PK = [](const std::string &statement) {
    SCOPED_TRACE(statement);
    std::string result;

    EXPECT_NO_THROW(add_pk_to_create_table(statement, &result));

    const std::string pk =
        ",`my_row_id` BIGINT UNSIGNED AUTO_INCREMENT INVISIBLE PRIMARY KEY";
    const auto pos = statement.find_last_of(')');
    const auto expected = statement.substr(0, pos) + pk + statement.substr(pos);
    EXPECT_EQ(expected, result) << "PK should be added";
  };

  {
    const std::string exception =
        "Invisible primary key can be only added to CREATE TABLE statements";
    std::string result;

    // unsupported statements
    EXPECT_THROW_LIKE(add_pk_to_create_table("", &result), std::runtime_error,
                      exception);
    EXPECT_THROW_LIKE(add_pk_to_create_table("CREATE", &result),
                      std::runtime_error, exception);
    EXPECT_THROW_LIKE(add_pk_to_create_table("create User", &result),
                      std::runtime_error, exception);
  }

  {
    const std::string exception = "Unsupported CREATE TABLE statement";
    std::string result;

    EXPECT_THROW_LIKE(
        add_pk_to_create_table("create table s.t2 like s.t1", &result),
        std::runtime_error, exception);
    EXPECT_THROW_LIKE(
        add_pk_to_create_table(
            "CREATE TABLE new_tbl AS SELECT * FROM orig_tbl;", &result),
        std::runtime_error, exception);
    EXPECT_THROW_LIKE(
        add_pk_to_create_table("CREATE TABLE new_tbl SELECT * FROM orig_tbl;",
                               &result),
        std::runtime_error, exception);
    EXPECT_THROW_LIKE(
        add_pk_to_create_table("CREATE TABLE bar (m INT) AS SELECT n FROM foo",
                               &result),
        std::runtime_error, exception);
    EXPECT_THROW_LIKE(
        add_pk_to_create_table("CREATE TABLE bar (m INT) SELECT n FROM foo",
                               &result),
        std::runtime_error, exception);
  }

  EXPECT_PK("create table s.t (c int)");
  EXPECT_PK("CREATE TABLE IF NOT EXISTS s.t (c int) Engine = InnoDB;");

  EXPECT_PK("create table s.t (c int, Index i (c))");
  EXPECT_PK(
      "create table s.t (key USING BTREE (d, c) COMMENT 'string', c int, d "
      "int)");
  EXPECT_PK(
      "create table t (FULLTEXT x (c), c int, spatial (d, (expr)), d int)");

  EXPECT_PK("create table t (CONSTRAINT FOREIGN KEY (x.t), c int)");
  EXPECT_PK("create table t (c int, CONSTRAINT symbol FOREIGN KEY (x.t))");
  EXPECT_PK("create table t (FOREIGN KEY (x.t), c int)");

  EXPECT_PK("create table t (c int, CONSTRAINT CHECK (expr (expr)))");
  EXPECT_PK(
      "create table t (CONSTRAINT symbol CHECK (expr (expr)) ENFORCED,c int)");
  EXPECT_PK("create table t (c int, CHECK ((expr) expr))");

  EXPECT_PK("create table t (c int, CONSTRAINT PRIMARY KEY USING BTREE (c))");
  EXPECT_PK(
      "create table t (CONSTRAINT symbol PRIMARY KEY (c) USING BTREE, c int)");
  EXPECT_PK("create table t (c int, PRIMARY KEY (c, d), d int)");

  EXPECT_PK("create table t (CONSTRAINT UNIQUE KEY (c), `c` int NOT NULL)");
  EXPECT_PK(
      "create table t (c int NULL, CONSTRAINT symbol UNIQUE INDEX (`c`) "
      "KEY_BLOCK_SIZE = 7)");
  EXPECT_PK("create table t (UNIQUE (`c`), c int DEFAULT 1 NOT NULL)");

  EXPECT_PK(
      "create table t (d varchar NOT NULL, UNIQUE u (`c`(7), d DESC), c int "
      "DEFAULT 1 NOT NULL)");

  EXPECT_PK(
      "create table t (d varchar NOT NULL, UNIQUE u ((expr), d, (expr) ASC, c "
      "(1)), c int DEFAULT 1 NOT NULL)");
}

TEST_F(Compatibility_test, add_invisible_pk_if_missing) {
  const auto EXPECT_PK = [](const std::string &statement, bool pk_added,
                            bool ignore_pke = true) {
    SCOPED_TRACE(statement);
    std::string result;

    EXPECT_EQ(pk_added, add_pk_to_create_table_if_missing(statement, &result,
                                                          ignore_pke));

    if (pk_added) {
      const std::string pk =
          ",`my_row_id` BIGINT UNSIGNED AUTO_INCREMENT INVISIBLE PRIMARY KEY";
      const auto pos = statement.find_last_of(')');
      const auto expected =
          statement.substr(0, pos) + pk + statement.substr(pos);
      EXPECT_EQ(expected, result) << "PK should be added";
    } else {
      EXPECT_EQ("", result) << "Result should be empty";
    }
  };

  {
    const std::string exception =
        "Invisible primary key can be only added to CREATE TABLE statements";
    std::string result;

    // unsupported statements
    EXPECT_THROW_LIKE(add_pk_to_create_table_if_missing("", &result),
                      std::runtime_error, exception);
    EXPECT_THROW_LIKE(add_pk_to_create_table_if_missing("CREATE", &result),
                      std::runtime_error, exception);
    EXPECT_THROW_LIKE(add_pk_to_create_table_if_missing("create User", &result),
                      std::runtime_error, exception);
  }

  {
    const std::string exception = "Unsupported CREATE TABLE statement";
    std::string result;

    EXPECT_THROW_LIKE(add_pk_to_create_table_if_missing(
                          "create table s.t2 like s.t1", &result),
                      std::runtime_error, exception);
    EXPECT_THROW_LIKE(add_pk_to_create_table_if_missing(
                          "create table s.t2 (like s.t1)", &result),
                      std::runtime_error, exception);
    EXPECT_THROW_LIKE(
        add_pk_to_create_table_if_missing(
            "CREATE TABLE new_tbl AS SELECT * FROM orig_tbl;", &result),
        std::runtime_error, exception);
    EXPECT_THROW_LIKE(
        add_pk_to_create_table_if_missing(
            "CREATE TABLE new_tbl SELECT * FROM orig_tbl;", &result),
        std::runtime_error, exception);
    EXPECT_THROW_LIKE(
        add_pk_to_create_table_if_missing(
            "CREATE TABLE bar (m INT) AS SELECT n FROM foo", &result),
        std::runtime_error, exception);
    EXPECT_THROW_LIKE(
        add_pk_to_create_table_if_missing(
            "CREATE TABLE bar (m INT) SELECT n FROM foo", &result),
        std::runtime_error, exception);
    EXPECT_THROW_LIKE(
        add_pk_to_create_table_if_missing(
            "CREATE TABLE bar (m INT NOT NULL, Unique (m)) SELECT n FROM foo",
            &result),
        std::runtime_error, exception);
  }

  EXPECT_PK("create table s.t (c int)", true);
  EXPECT_PK("CREATE TABLE IF NOT EXISTS s.t (c int) Engine = InnoDB;", true);

  EXPECT_PK("create table s.t (c int, Index i (c))", true);
  EXPECT_PK(
      "create table s.t (key USING BTREE (d, c) COMMENT 'string', c int, d "
      "int)",
      true);
  EXPECT_PK(
      "create table t (FULLTEXT x (c), c int, spatial (d, (expr)), d int)",
      true);

  EXPECT_PK("create table t (CONSTRAINT FOREIGN KEY (x.t), c int)", true);
  EXPECT_PK("create table t (c int, CONSTRAINT symbol FOREIGN KEY (x.t))",
            true);
  EXPECT_PK("create table t (FOREIGN KEY (x.t), c int)", true);

  EXPECT_PK("create table t (c int, CONSTRAINT CHECK (expr (expr)))", true);
  EXPECT_PK(
      "create table t (CONSTRAINT symbol CHECK (expr (expr)) ENFORCED,c int)",
      true);
  EXPECT_PK("create table t (c int, CHECK ((expr) expr))", true);

  EXPECT_PK("create table t (c int, CONSTRAINT PRIMARY KEY USING BTREE (c))",
            false);
  EXPECT_PK(
      "create table t (CONSTRAINT symbol PRIMARY KEY (c) USING BTREE, c int)",
      false);
  EXPECT_PK("create table t (c int, PRIMARY KEY (c, d), d int)", false);

  EXPECT_PK("create table t (CONSTRAINT UNIQUE KEY (c), `c` int NOT NULL)",
            true);
  EXPECT_PK(
      "create table t (c int NULL, CONSTRAINT symbol UNIQUE INDEX (`c`) "
      "KEY_BLOCK_SIZE = 7)",
      true);
  EXPECT_PK("create table t (UNIQUE (`c`), c int DEFAULT 1 NOT NULL)", true);

  EXPECT_PK("create table t (CONSTRAINT UNIQUE KEY (c), `c` int NOT NULL)",
            false, false);
  EXPECT_PK(
      "create table t (c int NULL, CONSTRAINT symbol UNIQUE INDEX (`c`) "
      "KEY_BLOCK_SIZE = 7)",
      true, false);
  EXPECT_PK("create table t (UNIQUE (`c`), c int DEFAULT 1 NOT NULL)", false,
            false);

  EXPECT_PK(
      "create table t (d varchar NOT NULL, UNIQUE u (`c`(7), d DESC), c int "
      "DEFAULT 1 NOT NULL)",
      true);
  EXPECT_PK(
      "create table t (d varchar NOT NULL, UNIQUE u (`c`(7), d DESC), c int "
      "DEFAULT 1 NOT NULL)",
      false, false);

  EXPECT_PK(
      "create table t (d varchar NOT NULL, UNIQUE u ((expr), d, (expr) ASC, c "
      "(1)), c int DEFAULT 1 NOT NULL)",
      true);
  EXPECT_PK(
      "create table t (d varchar NOT NULL, UNIQUE u ((expr (expr)), d, ((expr) "
      "expr (expr)) ASC), c int DEFAULT 1 NOT NULL)",
      true, false);

  EXPECT_PK("create table t (c int, d bigint NULL, e varchar UNIQUE)", true);
  EXPECT_PK("create table t (c int, d bigint NULL, e varchar UNIQUE)", true,
            false);

  EXPECT_PK(R"*(create table t (
    c int REFERENCES x (a, b) ON DELETE CASCADE,
    d bigint NULL CHECK (expr),
    e varchar UNIQUE CONSTRAINT symbol CHECK (expr)
  ))*",
            true);
  EXPECT_PK(
      "create table t (c int REFERENCES x (a, b) ON DELETE CASCADE, d bigint "
      "NULL CHECK (expr), e varchar UNIQUE CONSTRAINT symbol CHECK (expr))",
      true, false);

  EXPECT_PK("create table t (c int NOT NULL PRIMARY)", false);

  EXPECT_PK("create table t (c int PRIMARY NULL)", false);

  EXPECT_PK("create table t (c int PRIMARY NULL, d varchar)", false);

  EXPECT_PK("create table t (c int PRIMARY NULL, PRIMARY KEY c)", false);

  EXPECT_PK("create table t (PRIMARY KEY c, c int PRIMARY NULL)", false);

  EXPECT_PK("create table t (d varchar UNIQUE NOT NULL, c int PRIMARY KEY)",
            false);
  EXPECT_PK("create table t (d varchar UNIQUE NOT NULL, c int PRIMARY KEY)",
            false, false);

  EXPECT_PK("create table t (d varchar UNIQUE NOT NULL, c int)", true);
  EXPECT_PK("create table t (d varchar UNIQUE NOT NULL, c int)", false, false);

  EXPECT_PK("create table t (d varchar UNIQUE NOT NULL, c int, UNIQUE (c))",
            true);
  EXPECT_PK("create table t (d varchar UNIQUE NOT NULL, c int, UNIQUE (c))",
            false, false);

  EXPECT_PK(
      "create table t (d varchar UNIQUE NULL, c int NOT NULL, UNIQUE (`c`))",
      true);
  EXPECT_PK(
      "create table t (d varchar UNIQUE NULL, c int NOT NULL, UNIQUE (`c`))",
      false, false);

  EXPECT_PK(
      "create table t (d varchar UNIQUE NOT NULL, c int NOT NULL, UNIQUE (c))",
      true);
  EXPECT_PK(
      "create table t (d varchar UNIQUE NOT NULL, c int NOT NULL, UNIQUE (c))",
      false, false);

  EXPECT_PK(
      "create table t (`d` varchar NOT NULL, c int NOT NULL, UNIQUE (c, d))",
      true);
  EXPECT_PK(
      "create table t (`d` varchar NOT NULL, c int NOT NULL, UNIQUE (c, d))",
      false, false);

  EXPECT_PK("create table t (`d` varchar NULL, UNIQUE (c, d), c int NOT NULL)",
            true);
  EXPECT_PK("create table t (`d` varchar NULL, UNIQUE (d, c), c int NOT NULL)",
            true, false);

  EXPECT_PK("create table t (UNIQUE (c, d),`d` varchar NOT NULL, c int)", true);
  EXPECT_PK("create table t (UNIQUE (c, d),`d` varchar NOT NULL, c int)", true,
            false);

  EXPECT_PK("CREATE TABLE bar (m INT PRIMARY) SELECT n FROM foo", false);
  EXPECT_PK("CREATE TABLE bar (m INT, primary KEY m) SELECT n FROM foo", false);

  EXPECT_PK("CREATE TABLE bar (m INT NOT NULL, Unique (m)) SELECT n FROM foo",
            false, false);
}

TEST_F(Compatibility_test, convert_grant_to_create_user) {
  {
    const std::string exception =
        "Only GRANT statement can be converted to CREATE USER statement";
    std::string result;

    // unsupported statements
    EXPECT_THROW_LIKE(convert_grant_to_create_user("", "", &result),
                      std::logic_error, exception);
    EXPECT_THROW_LIKE(convert_grant_to_create_user("CREATE", "", &result),
                      std::logic_error, exception);
  }

  const auto EXPECT = [](const std::string &statement,
                         const std::string &create_user,
                         const std::string &grant) {
    SCOPED_TRACE(statement);

    std::string g = statement;
    const auto cr = convert_grant_to_create_user(g, "plugin", &g);

    EXPECT_EQ(create_user, cr);
    EXPECT_EQ(grant, g);
  };

  EXPECT("GRANT SELECT ON *.* TO \"u\"@`h`", "CREATE USER \"u\"@`h`",
         "GRANT SELECT ON *.* TO \"u\"@`h`");

  EXPECT("GRANT SELECT (`to`) ON `to`.`to` TO root@localhost",
         "CREATE USER root@localhost",
         "GRANT SELECT (`to`) ON `to`.`to` TO root@localhost");

  EXPECT("GRANT SELECT ON *.* TO u@h WITH GRANT OPTION", "CREATE USER u@h",
         "GRANT SELECT ON *.* TO u@h WITH GRANT OPTION");

  EXPECT(
      "GRANT SELECT ON *.* TO u@h IDENTIFIED WITH auth_plugin WITH GRANT "
      "OPTION MAX_QUERIES_PER_HOUR 10",
      "CREATE USER u@h IDENTIFIED WITH auth_plugin WITH MAX_QUERIES_PER_HOUR "
      "10",
      "GRANT SELECT ON *.* TO u@h WITH GRANT OPTION");

  EXPECT(
      "GRANT SELECT ON *.* TO u@h IDENTIFIED WITH 'auth_plugin' AS 'hash' WITH "
      "MAX_QUERIES_PER_HOUR 10 GRANT OPTION",
      "CREATE USER u@h IDENTIFIED WITH 'auth_plugin' AS 'hash' WITH "
      "MAX_QUERIES_PER_HOUR 10",
      "GRANT SELECT ON *.* TO u@h WITH GRANT OPTION");

  EXPECT_THROW_LIKE(
      EXPECT("GRANT SELECT ON *.* TO u@h IDENTIFIED BY 'pwd' WITH "
             "MAX_QUERIES_PER_HOUR 10",
             "", ""),
      std::logic_error,
      "The GRANT statement contains clear-text password which is not "
      "supported");

  EXPECT("GRANT SELECT ON *.* TO u@h IDENTIFIED BY PASSWORD 'hash'",
         "CREATE USER u@h IDENTIFIED WITH 'plugin' AS 'hash'",
         "GRANT SELECT ON *.* TO u@h");

  EXPECT(
      "GRANT ALL PRIVILEGES ON *.* TO u@h IDENTIFIED BY PASSWORD 'hash' "
      "REQUIRE SSL AND X509 AND CIPHER 'cipher' AND ISSUER 'issuer' AND "
      "SUBJECT 'subject' WITH MAX_QUERIES_PER_HOUR 7 MAX_UPDATES_PER_HOUR 6 "
      "GRANT OPTION MAX_CONNECTIONS_PER_HOUR 5 MAX_USER_CONNECTIONS 4",
      "CREATE USER u@h IDENTIFIED WITH 'plugin' AS 'hash' REQUIRE SSL AND X509 "
      "AND CIPHER 'cipher' AND ISSUER 'issuer' AND SUBJECT 'subject' WITH "
      "MAX_QUERIES_PER_HOUR 7 MAX_UPDATES_PER_HOUR 6 MAX_CONNECTIONS_PER_HOUR "
      "5 MAX_USER_CONNECTIONS 4",
      "GRANT ALL PRIVILEGES ON *.* TO u@h WITH GRANT OPTION");
}

}  // namespace compatibility
}  // namespace mysqlsh
