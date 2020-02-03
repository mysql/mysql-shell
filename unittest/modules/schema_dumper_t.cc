/* Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include <stdio.h>

#include "modules/util/dump/schema_dumper.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/storage/backend/file.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/test_utils.h"

extern "C" const char *g_test_home;

namespace mysqlsh {

class Schema_dumper_test : public Shell_core_test_wrapper {
 public:
  static void TearDownTestCase() {
    if (initialized) {
      auto session = connect_session();
      session->execute(std::string("drop database ") + db_name);
    }
  }

  Schema_dumper_test()
      : file_path(
            shcore::path::join_path(shcore::path::tmpdir(), "dumpt.sql")) {}

 protected:
  virtual void SetUp() {
    Shell_core_test_wrapper::SetUp();
    session = connect_session();
    if (!initialized) {
      testutil->call_mysqlsh_c(
          {_uri, "--force", "-f",
           shcore::path::join_path(g_test_home, "data", "sql",
                                   "mysqldump_t.sql")});
      testutil->call_mysqlsh_c(
          {_uri, "--force", "-f",
           shcore::path::join_path(g_test_home, "data", "sql",
                                   "crazy_names.sql")});

      const auto out_path = shcore::path::join_path(shcore::path::tmpdir(),
                                                    "mysqlaas_compat.sql");
      run_directory_tests =
          _target_server_version < mysqlshdk::utils::Version(8, 0, 0) &&
          shcore::get_os_type() != shcore::OperatingSystem::WINDOWS;
      if (run_directory_tests)
        testutil->preprocess_file(
            shcore::path::join_path(g_test_home, "data", "sql",
                                    "mysqlaas_compat57.sql"),
            {"TMPDIR=" + shcore::path::tmpdir()}, out_path);
      else
        shcore::copy_file(shcore::path::join_path(g_test_home, "data", "sql",
                                                  "mysqlaas_compat8.sql"),
                          out_path);
      testutil->call_mysqlsh_c({_uri, "--force", "-f", out_path});
      puts(output_handler.std_out.c_str());
    }
    initialized = true;
    file.reset(new mysqlshdk::storage::backend::File(file_path));
    file->open(mysqlshdk::storage::Mode::WRITE);
    assert(file->is_open());
  }

  virtual void TearDown() {
    session->close();
    file->close();
    Shell_core_test_wrapper::TearDown();
  }

  static std::shared_ptr<mysqlshdk::db::ISession> connect_session() {
    auto session = mysqlshdk::db::mysql::Session::create();
    session->connect(shcore::get_connection_options(_mysql_uri));
    return session;
  }

  void expect_output_eq(const char *res) {
    file->flush();
    file->close();
    EXPECT_EQ(res, testutil->cat_file(file_path));
  }

  void expect_output_contains(std::vector<const char *> items) {
    file->flush();
    file->close();
    auto out = testutil->cat_file(file_path);
    for (const auto i : items)
      if (std::string::npos == out.find(i)) {
        EXPECT_EQ(out, i);
      }
  }

  std::shared_ptr<mysqlshdk::db::ISession> session;
  std::string file_path;
  std::unique_ptr<IFile> file;
  bool run_directory_tests = false;
  static bool initialized;
  static constexpr auto db_name = "mysqldump_test_db";
  static constexpr auto compat_db_name = "mysqlaas_compat";
  static constexpr auto crazy_names_db = "crazy_names_db";
};

bool Schema_dumper_test::initialized = false;

TEST_F(Schema_dumper_test, dump_table) {
  Schema_dumper sd(session);
  EXPECT_NO_THROW(sd.dump_table_ddl(file.get(), db_name, "at1"));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;

  const char *res = R"(
--
-- Table structure for table `at1`
--

DROP TABLE IF EXISTS at1;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE IF NOT EXISTS `at1` (
  `t1_name` varchar(255) DEFAULT NULL,
  `t1_id` int unsigned NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`t1_id`),
  KEY `t1_name` (`t1_name`)
) ENGINE=InnoDB AUTO_INCREMENT=1003 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
)";

  expect_output_eq(res);
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_table_with_trigger) {
  Schema_dumper sd(session);
  sd.opt_drop_trigger = true;
  EXPECT_NO_THROW(sd.dump_table_ddl(file.get(), db_name, "t1"));
  if (sd.count_triggers_for_table(db_name, "t1"))
    EXPECT_NO_THROW(sd.dump_triggers_for_table_ddl(file.get(), db_name, "t1"));
  EXPECT_NO_THROW(sd.dump_table_ddl(file.get(), db_name, "t2"));
  if (sd.count_triggers_for_table(db_name, "t2"))
    EXPECT_NO_THROW(sd.dump_triggers_for_table_ddl(file.get(), db_name, "t2"));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;

  const char *res = R"(
--
-- Table structure for table `t1`
--

DROP TABLE IF EXISTS t1;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE IF NOT EXISTS `t1` (
  `a` int DEFAULT NULL,
  `b` bigint DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping triggers for table 'mysqldump_test_db'.'t1'
--

-- begin trigger mysqldump_test_db.trg1
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
/*!50032 DROP TRIGGER IF EXISTS trg1 */;
DELIMITER ;;
/*!50003 CREATE DEFINER=`root`@`localhost` TRIGGER `trg1` BEFORE INSERT ON `t1` FOR EACH ROW begin
  if new.a > 10 then
    set new.a := 10;
    set new.a := 11;
  end if;
end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end trigger mysqldump_test_db.trg1

-- begin trigger mysqldump_test_db.trg2
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
/*!50032 DROP TRIGGER IF EXISTS trg2 */;
DELIMITER ;;
/*!50003 CREATE DEFINER=`root`@`localhost` TRIGGER `trg2` BEFORE UPDATE ON `t1` FOR EACH ROW begin
  if old.a % 2 = 0 then set new.b := 12; end if;
end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end trigger mysqldump_test_db.trg2

-- begin trigger mysqldump_test_db.trg3
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,TRADITIONAL,NO_ENGINE_SUBSTITUTION' */ ;
/*!50032 DROP TRIGGER IF EXISTS trg3 */;
DELIMITER ;;
/*!50003 CREATE DEFINER=`root`@`localhost` TRIGGER `trg3` AFTER UPDATE ON `t1` FOR EACH ROW begin
  if new.a = -1 then
    set @fired:= "Yes";
  end if;
end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end trigger mysqldump_test_db.trg3


--
-- Table structure for table `t2`
--

DROP TABLE IF EXISTS t2;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE IF NOT EXISTS `t2` (
  `a` int DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping triggers for table 'mysqldump_test_db'.'t2'
--

-- begin trigger mysqldump_test_db.trg4
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,TRADITIONAL,NO_ENGINE_SUBSTITUTION' */ ;
/*!50032 DROP TRIGGER IF EXISTS trg4 */;
DELIMITER ;;
/*!50003 CREATE DEFINER=`root`@`localhost` TRIGGER `trg4` BEFORE INSERT ON `t2` FOR EACH ROW begin
  if new.a > 10 then
    set @fired:= "No";
  end if;
end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end trigger mysqldump_test_db.trg4

)";

  expect_output_eq(res);
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_schema) {
  Schema_dumper sd(session);
  sd.opt_mysqlaas = true;
  EXPECT_NO_THROW(sd.write_header(file.get(), db_name));
  EXPECT_EQ(1, sd.dump_schema_ddl(file.get(), db_name).size());
  EXPECT_NO_THROW(sd.write_footer(file.get()));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;

  const char *res =
      R"(/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Current Database: mysqldump_test_db
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `mysqldump_test_db` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 -- DEFAULT ENCRYPTION='N'
 */;

USE mysqldump_test_db;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;)";
  expect_output_contains({res});
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_view) {
  Schema_dumper sd(session);
  EXPECT_NO_THROW(sd.dump_temporary_view_ddl(file.get(), db_name, "v1"));
  EXPECT_NO_THROW(sd.dump_temporary_view_ddl(file.get(), db_name, "v2"));
  EXPECT_NO_THROW(sd.dump_temporary_view_ddl(file.get(), db_name, "v3"));
  EXPECT_NO_THROW(sd.dump_view_ddl(file.get(), db_name, "v1"));
  EXPECT_NO_THROW(sd.dump_view_ddl(file.get(), db_name, "v2"));
  EXPECT_NO_THROW(sd.dump_view_ddl(file.get(), db_name, "v3"));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;
  const char *res = R"(
--
-- Temporary view structure for view `v1`
--

DROP TABLE IF EXISTS v1;
/*!50001 DROP VIEW IF EXISTS v1*/;
SET @saved_cs_client     = @@character_set_client;
/*!50503 SET character_set_client = utf8mb4 */;
/*!50001 CREATE VIEW `v1` AS SELECT 
 1 AS a,
 1 AS b,
 1 AS c */;
SET character_set_client = @saved_cs_client;

--
-- Temporary view structure for view `v2`
--

DROP TABLE IF EXISTS v2;
/*!50001 DROP VIEW IF EXISTS v2*/;
SET @saved_cs_client     = @@character_set_client;
/*!50503 SET character_set_client = utf8mb4 */;
/*!50001 CREATE VIEW `v2` AS SELECT 
 1 AS a */;
SET character_set_client = @saved_cs_client;

--
-- Temporary view structure for view `v3`
--

DROP TABLE IF EXISTS v3;
/*!50001 DROP VIEW IF EXISTS v3*/;
SET @saved_cs_client     = @@character_set_client;
/*!50503 SET character_set_client = utf8mb4 */;
/*!50001 CREATE VIEW `v3` AS SELECT 
 1 AS a,
 1 AS b,
 1 AS c */;
SET character_set_client = @saved_cs_client;

--
-- Final view structure for view `v1`
--

/*!50001 DROP VIEW IF EXISTS v1*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = utf8mb4 */;
/*!50001 SET character_set_results     = utf8mb4 */;
/*!50001 SET collation_connection      = utf8mb4_0900_ai_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `v1` AS select `v3`.`a` AS `a`,`v3`.`b` AS `b`,`v3`.`c` AS `c` from `v3` where (`v3`.`b` in (1,2,3,4,5,6,7)) */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;

--
-- Final view structure for view `v2`
--

/*!50001 DROP VIEW IF EXISTS v2*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = utf8mb4 */;
/*!50001 SET character_set_results     = utf8mb4 */;
/*!50001 SET collation_connection      = utf8mb4_0900_ai_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `v2` AS select `tv2`.`a` AS `a` from `tv2` where (`tv2`.`a` like 'a%') WITH CASCADED CHECK OPTION */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;

--
-- Final view structure for view `v3`
--

/*!50001 DROP VIEW IF EXISTS v3*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = utf8mb4 */;
/*!50001 SET character_set_results     = utf8mb4 */;
/*!50001 SET collation_connection      = utf8mb4_0900_ai_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `v3` AS select `tv1`.`a` AS `a`,`tv1`.`b` AS `b`,`tv1`.`c` AS `c` from `tv1` */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;
)";
  expect_output_eq(res);
}

TEST_F(Schema_dumper_test, dump_events) {
  Schema_dumper sd(session);
  EXPECT_NO_THROW(sd.dump_events_ddl(file.get(), db_name));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;

  const char *res = R"(
--
-- Dumping events for database 'mysqldump_test_db'
--
/*!50106 SET @save_time_zone= @@TIME_ZONE */ ;

-- begin event mysqldump_test_db.ee1
/*!50106 DROP EVENT IF EXISTS ee1 */;
DELIMITER ;;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;;
/*!50003 SET character_set_client  = utf8mb4 */ ;;
/*!50003 SET character_set_results = utf8mb4 */ ;;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;;
/*!50003 SET @saved_time_zone      = @@time_zone */ ;;
/*!50003 SET time_zone             = 'SYSTEM' */ ;;
/*!50106 CREATE DEFINER=`root`@`localhost` EVENT IF NOT EXISTS `ee1` ON SCHEDULE AT '2035-12-31 20:01:23' ON COMPLETION NOT PRESERVE ENABLE DO set @a=5 */ ;;
/*!50003 SET time_zone             = @saved_time_zone */ ;;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;;
/*!50003 SET character_set_client  = @saved_cs_client */ ;;
/*!50003 SET character_set_results = @saved_cs_results */ ;;
/*!50003 SET collation_connection  = @saved_col_connection */ ;;
-- end event mysqldump_test_db.ee1

-- begin event mysqldump_test_db.ee2
/*!50106 DROP EVENT IF EXISTS ee2 */;;
DELIMITER ;;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;;
/*!50003 SET character_set_client  = utf8mb4 */ ;;
/*!50003 SET character_set_results = utf8mb4 */ ;;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;;
/*!50003 SET @saved_time_zone      = @@time_zone */ ;;
/*!50003 SET time_zone             = 'SYSTEM' */ ;;
/*!50106 CREATE DEFINER=`root`@`localhost` EVENT IF NOT EXISTS `ee2` ON SCHEDULE AT '2029-12-31 21:01:23' ON COMPLETION NOT PRESERVE ENABLE DO set @a=5 */ ;;
/*!50003 SET time_zone             = @saved_time_zone */ ;;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;;
/*!50003 SET character_set_client  = @saved_cs_client */ ;;
/*!50003 SET character_set_results = @saved_cs_results */ ;;
/*!50003 SET collation_connection  = @saved_col_connection */ ;;
-- end event mysqldump_test_db.ee2

DELIMITER ;
/*!50106 SET TIME_ZONE= @save_time_zone */ ;
)";
  expect_output_eq(res);
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_routines) {
  Schema_dumper sd(session);
  EXPECT_NO_THROW(sd.dump_routines_ddl(file.get(), db_name));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;

  const char *res = R"(
--
-- Dumping routines for database 'mysqldump_test_db'
--

-- begin function mysqldump_test_db.bug9056_func1
/*!50003 DROP FUNCTION IF EXISTS bug9056_func1 */;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
CREATE DEFINER=`root`@`localhost` FUNCTION `bug9056_func1`(a INT, b INT) RETURNS int
RETURN a+b ;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end function mysqldump_test_db.bug9056_func1

-- begin function mysqldump_test_db.bug9056_func2
/*!50003 DROP FUNCTION IF EXISTS bug9056_func2 */;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
CREATE DEFINER=`root`@`localhost` FUNCTION `bug9056_func2`(f1 char binary) RETURNS char(1) CHARSET utf8mb4
begin
  set f1= concat( 'hello', f1 );
  return f1;
end ;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end function mysqldump_test_db.bug9056_func2

-- begin procedure mysqldump_test_db.`a'b`
/*!50003 DROP PROCEDURE IF EXISTS `a'b` */;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,IGNORE_SPACE,ONLY_FULL_GROUP_BY,ANSI' */ ;
DELIMITER ;;
CREATE DEFINER="root"@"localhost" PROCEDURE "a'b"()
select 1 ;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end procedure mysqldump_test_db.`a'b`

-- begin procedure mysqldump_test_db.bug9056_proc1
/*!50003 DROP PROCEDURE IF EXISTS bug9056_proc1 */;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
CREATE DEFINER=`root`@`localhost` PROCEDURE `bug9056_proc1`(IN a INT, IN b INT, OUT c INT)
BEGIN SELECT a+b INTO c; end ;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end procedure mysqldump_test_db.bug9056_proc1

-- begin procedure mysqldump_test_db.bug9056_proc2
/*!50003 DROP PROCEDURE IF EXISTS bug9056_proc2 */;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
CREATE DEFINER=`root`@`localhost` PROCEDURE `bug9056_proc2`(OUT a INT)
BEGIN
  select sum(id) from tr1 into a;
END ;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end procedure mysqldump_test_db.bug9056_proc2

)";
  expect_output_eq(res);
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_tablespaces) {
  Schema_dumper sd(session);
  EXPECT_NO_THROW(sd.dump_tablespaces_ddl_for_dbs(file.get(), {db_name}));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();

  EXPECT_NO_THROW(
      sd.dump_tablespaces_ddl_for_tables(file.get(), db_name, {"at1"}));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  file->flush();
  file->close();
  EXPECT_TRUE(shcore::str_strip(testutil->cat_file(file_path)).empty());
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_grants) {
  Schema_dumper sd(session);
  EXPECT_NO_THROW(sd.dump_grants(file.get(), {}));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  expect_output_contains({"CREATE USER IF NOT EXISTS", "ALTER USER", "GRANT",
                          "FLUSH PRIVILEGES;"});
}

TEST_F(Schema_dumper_test, dump_filtered_grants) {
  session->execute(
      "CREATE USER IF NOT EXISTS 'superfirst'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute(
      "GRANT SUPER, LOCK TABLES ON *.* TO 'superfirst'@'localhost';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'superafter'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute(
      "GRANT INSERT,super, UPDATE ON *.* TO 'superafter'@'localhost';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'superonly'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute("GRANT SUPER, RELOAD ON *.* TO 'superonly'@'localhost';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'dumptestuser'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute("GRANT SELECT ON * . * TO 'dumptestuser'@'localhost';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'abr@dab'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute(
      "GRANT INSERT,SUPER,FILE,LOCK TABLES , reload, "
      "SELECT ON * . * TO 'abr@dab'@'localhost';");
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 0)) {
    session->execute("CREATE ROLE IF NOT EXISTS da_dumper");
    session->execute(
        "GRANT INSERT, BINLOG_ADMIN, UPDATE, DELETE ON * . * TO 'da_dumper';");
    session->execute("GRANT 'da_dumper' TO 'dumptestuser'@'localhost';");
  }

  Schema_dumper sd(session);
  sd.opt_mysqlaas = true;
  sd.opt_strip_restricted_grants = true;
  EXPECT_GE(sd.dump_grants(file.get(), {"mysql%", "root"}).size(), 3);
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  file->flush();
  file->close();
  auto out = testutil->cat_file(file_path);

  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 0)) {
    EXPECT_NE(
        std::string::npos,
        out.find(
            "CREATE USER IF NOT EXISTS 'da_dumper'@'%' IDENTIFIED WITH "
            "'caching_sha2_password' REQUIRE NONE PASSWORD EXPIRE ACCOUNT "
            "LOCK PASSWORD HISTORY DEFAULT PASSWORD REUSE INTERVAL DEFAULT "
            "PASSWORD REQUIRE CURRENT DEFAULT;"));
    EXPECT_NE(std::string::npos,
              out.find("ALTER USER 'da_dumper'@'%' IDENTIFIED WITH "
                       "'caching_sha2_password' "
                       "REQUIRE NONE PASSWORD EXPIRE ACCOUNT LOCK PASSWORD "
                       "HISTORY DEFAULT "
                       "PASSWORD REUSE INTERVAL DEFAULT PASSWORD REQUIRE "
                       "CURRENT DEFAULT;"));
    EXPECT_NE(
        std::string::npos,
        out.find("GRANT INSERT, UPDATE, DELETE ON *.* TO `da_dumper`@`%`;"));
    EXPECT_NE(std::string::npos,
              out.find("GRANT `da_dumper`@`%` TO `dumptestuser`@`localhost`;"));
  }

  EXPECT_NE(std::string::npos,
            out.find("CREATE USER IF NOT EXISTS 'dumptestuser'@'localhost'"));
  EXPECT_NE(std::string::npos, out.find("-- begin user abr@dab@localhost"));
  EXPECT_NE(std::string::npos,
            out.find("ALTER USER 'dumptestuser'@'localhost'"));
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 0)) {
    EXPECT_NE(std::string::npos,
              out.find("GRANT SELECT ON *.* TO `dumptestuser`@`localhost`"));
    EXPECT_NE(
        std::string::npos,
        out.find("GRANT LOCK TABLES ON *.* TO `superfirst`@`localhost`;"));
    EXPECT_NE(
        std::string::npos,
        out.find("GRANT INSERT, UPDATE ON *.* TO `superafter`@`localhost`;"));
    EXPECT_EQ(std::string::npos, out.find("TO `superonly`@`localhost`;"));
    EXPECT_NE(std::string::npos, out.find(R"(-- begin grants abr@dab@localhost
GRANT SELECT, INSERT, LOCK TABLES ON *.* TO `abr@dab`@`localhost`;
-- end grants abr@dab@localhost)"));
  } else {
    EXPECT_NE(std::string::npos,
              out.find("GRANT SELECT ON *.* TO 'dumptestuser'@'localhost';"));
    EXPECT_NE(
        std::string::npos,
        out.find("GRANT LOCK TABLES ON *.* TO 'superfirst'@'localhost';"));
    EXPECT_NE(
        std::string::npos,
        out.find("GRANT INSERT, UPDATE ON *.* TO 'superafter'@'localhost';"));
    EXPECT_EQ(std::string::npos, out.find("TO 'superonly'@'localhost';"));
    EXPECT_NE(std::string::npos, out.find(R"(-- begin grants abr@dab@localhost
GRANT SELECT, INSERT, LOCK TABLES ON *.* TO 'abr@dab'@'localhost';
-- end grants abr@dab@localhost)"));
  }
  EXPECT_EQ(std::string::npos, out.find("SUPER"));

  EXPECT_NE(std::string::npos, out.find("FLUSH PRIVILEGES;"));
  EXPECT_EQ(std::string::npos, out.find("'root'"));
  EXPECT_EQ(std::string::npos, out.find("EXISTS 'mysql"));

  wipe_all();
  testutil->call_mysqlsh_c({_mysql_uri, "--sql", "-f", file_path});
  EXPECT_TRUE(output_handler.std_err.empty());

  session->execute("drop user 'superfirst'@'localhost';");
  session->execute("drop user 'superafter'@'localhost';");
  session->execute("drop user 'superonly'@'localhost';");
  session->execute("drop user 'dumptestuser'@'localhost';");
  session->execute("drop user 'abr@dab'@'localhost';");
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 0))
    session->execute("DROP ROLE da_dumper");
}

TEST_F(Schema_dumper_test, opt_mysqlaas) {
  Schema_dumper sd(session);
  sd.opt_mysqlaas = true;
  sd.opt_pk_mandatory_check = true;

  session->execute(std::string("use ") + compat_db_name);

  const auto EXPECT_TABLE = [&](const std::string &table,
                                std::vector<std::string> msg) {
    ASSERT_TRUE(file->is_open());
    std::vector<Schema_dumper::Issue> iss;
    try {
      iss = sd.dump_table_ddl(file.get(), compat_db_name, table);
    } catch (const std::exception &e) {
      puts(("Exception for table " + table + e.what()).c_str());
      ASSERT_TRUE(false);
    }
    if (msg.size() != iss.size()) {
      puts(("issues mismatch for table: " + table).c_str());
      ASSERT_EQ(msg.size(), iss.size());
    }
    for (std::size_t i = 0; i < iss.size(); ++i) {
      EXPECT_EQ(msg[i], iss[i].description);
    }
  };

  // Engines
  EXPECT_TABLE("myisam_tbl1",
               {"Table 'mysqlaas_compat'.'myisam_tbl1' uses unsupported "
                "charset: latin1",
                "Table 'mysqlaas_compat'.'myisam_tbl1' uses unsupported "
                "storage engine MyISAM"});

  EXPECT_TABLE(
      "blackhole_tbl1",
      {"Table 'mysqlaas_compat'.'blackhole_tbl1' does not have primary or "
       "unique non null key defined",
       "Table 'mysqlaas_compat'.'blackhole_tbl1' uses unsupported charset: "
       "latin1",
       "Table 'mysqlaas_compat'.'blackhole_tbl1' uses unsupported storage "
       "engine BLACKHOLE"});

  EXPECT_TABLE("myisam_tbl2",
               {"Table 'mysqlaas_compat'.'myisam_tbl2' uses unsupported "
                "charset: utf8",
                "Table 'mysqlaas_compat'.'myisam_tbl2' uses unsupported "
                "storage engine MyISAM"});

  // TABLESPACE
  EXPECT_TABLE("ts1_tbl1", {"Table 'mysqlaas_compat'.'ts1_tbl1' uses "
                            "unsupported tablespace option"});

  // DEFINERS
  std::vector<Schema_dumper::Issue> iss;
  EXPECT_NO_THROW(iss = sd.dump_triggers_for_table_ddl(
                      file.get(), compat_db_name, "myisam_tbl1"));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ(
      "Trigger mysqlaas_compat.ins_sum - definition uses DEFINER clause set "
      "to user `root`@`localhost` which can only be executed by this user or a "
      "user with SET_USER_ID or SUPER privileges",
      iss.front().description);

  EXPECT_NO_THROW(iss = sd.dump_events_ddl(file.get(), compat_db_name));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ(
      "Event mysqlaas_compat.event2 - definition uses DEFINER clause set to "
      "user `root`@`localhost` which can only be executed by this user or a "
      "user with SET_USER_ID or SUPER privileges",
      iss.front().description);

  const auto user = "`" + _user + "`@`" + _host + "`";
  EXPECT_NO_THROW(iss = sd.dump_routines_ddl(file.get(), compat_db_name));
  ASSERT_EQ(5, iss.size());
  EXPECT_EQ(
      "Function mysqlaas_compat.func1 - definition uses DEFINER clause set to "
      "user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_USER_ID or SUPER privileges",
      iss[0].description);
  EXPECT_EQ(
      "Function mysqlaas_compat.func2 - definition uses DEFINER clause set to "
      "user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_USER_ID or SUPER privileges",
      iss[1].description);
  EXPECT_EQ(
      "Procedure mysqlaas_compat.labeled - definition uses DEFINER clause set "
      "to user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_USER_ID or SUPER privileges",
      iss[2].description);
  EXPECT_EQ(
      "Procedure mysqlaas_compat.proc1 - definition uses DEFINER clause set to "
      "user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_USER_ID or SUPER privileges",
      iss[3].description);
  EXPECT_EQ(
      "Procedure mysqlaas_compat.proc2 - definition uses DEFINER clause set to "
      "user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_USER_ID or SUPER privileges",
      iss[4].description);

  EXPECT_NO_THROW(iss = sd.dump_view_ddl(file.get(), compat_db_name, "view1"));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ(
      "View mysqlaas_compat.view1 - definition uses DEFINER clause set to "
      "user " +
          user +
          " which can only be executed by this user or a user "
          "with SET_USER_ID or SUPER privileges",
      iss.front().description);

  EXPECT_NO_THROW(iss = sd.dump_view_ddl(file.get(), compat_db_name, "view3"));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ(
      "View mysqlaas_compat.view3 - definition uses DEFINER clause set to "
      "user " +
          user +
          " which can only be executed by this user or a user "
          "with SET_USER_ID or SUPER privileges",
      iss.front().description);

  // Users
  EXPECT_NO_THROW(iss = sd.dump_grants(file.get(), {"mysql%", "root"}));

  auto expected_issues =
      _target_server_version < mysqlshdk::utils::Version(8, 0, 0) ?
      std::set<std::string>{
      "User testusr1@localhost is granted restricted privileges: RELOAD, FILE, "
      "SUPER",
      "User testusr2@localhost is granted restricted privilege: SUPER",
      "User testusr3@localhost is granted restricted privileges: RELOAD, FILE",
      "User testusr4@localhost is granted restricted privilege: SUPER",
      "User testusr5@localhost is granted restricted privilege: FILE",
      "User testusr6@localhost is granted restricted privilege: FILE"} :
      std::set<std::string>{
        "User testusr1@localhost is granted restricted privileges: RELOAD, "
        "FILE, SUPER, BINLOG_ADMIN",
        "User testusr2@localhost is granted restricted privilege: SUPER",
        "User testusr3@localhost is granted restricted privileges: RELOAD, "
        "FILE, BINLOG_ADMIN",
        "User testusr4@localhost is granted restricted privilege: SUPER",
        "User testusr5@localhost is granted restricted privilege: FILE",
        "User testusr6@localhost is granted restricted privilege: FILE"};
  for (const auto &i : iss) {
    const auto it = expected_issues.find(i.description);
    if (it != expected_issues.end()) expected_issues.erase(it);
  }
  if (!expected_issues.empty()) {
    puts("Missing issues: ");
    for (const auto &i : expected_issues) puts(i.c_str());
    EXPECT_TRUE(false);
  }
}

TEST_F(Schema_dumper_test, compat_ddl) {
  Schema_dumper sd(session);
  sd.opt_mysqlaas = true;
  sd.opt_force_innodb = true;
  sd.opt_strip_restricted_grants = true;
  sd.opt_strip_tablespaces = true;
  sd.opt_strip_definer = true;

  session->execute(std::string("use ") + compat_db_name);

  const auto EXPECT_TABLE = [&](const std::string &table,
                                std::vector<std::string> msg,
                                const std::string &ddl = "") {
    file.reset(new mysqlshdk::storage::backend::File(file_path));
    file->open(mysqlshdk::storage::Mode::WRITE);
    ASSERT_TRUE(file->is_open());
    std::vector<Schema_dumper::Issue> iss;
    try {
      iss = sd.dump_table_ddl(file.get(), compat_db_name, table);
    } catch (const std::exception &e) {
      puts(("Exception for table " + table + e.what()).c_str());
      ASSERT_TRUE(false);
    }
    if (msg.size() != iss.size()) {
      puts(("issues mismatch for table: " + table).c_str());
      ASSERT_EQ(msg.size(), iss.size());
    }
    for (std::size_t i = 0; i < iss.size(); ++i) {
      if (iss[i].description != msg[i]) puts(iss[i].description.c_str());
      EXPECT_EQ(msg[i], iss[i].description);
    }
    file->flush();
    file->close();
    if (ddl.empty()) return;
    const auto out = testutil->cat_file(file_path);
    if (out.find(_target_server_version >= mysqlshdk::utils::Version(8, 0, 0)
                     ? shcore::str_replace(ddl, "(11)", "")
                     : ddl) == std::string::npos) {
      EXPECT_EQ(out, ddl);
    }
  };

  // Engines
  EXPECT_TABLE(
      "myisam_tbl1",
      {"Table 'mysqlaas_compat'.'myisam_tbl1' uses unsupported "
       "charset: latin1",
       "Table 'mysqlaas_compat'.'myisam_tbl1' had unsupported engine MyISAM "
       "changed to InnoDB"},
      "CREATE TABLE IF NOT EXISTS `myisam_tbl1` (\n"
      "  `id` int(11) NOT NULL AUTO_INCREMENT,\n"
      "  PRIMARY KEY (`id`)\n"
      ") ENGINE=InnoDB DEFAULT CHARSET=latin1");

  EXPECT_TABLE(
      "blackhole_tbl1",
      {"Table 'mysqlaas_compat'.'blackhole_tbl1' uses unsupported charset: "
       "latin1",
       "Table 'mysqlaas_compat'.'blackhole_tbl1' had unsupported engine "
       "BLACKHOLE changed to InnoDB"},
      "CREATE TABLE IF NOT EXISTS `blackhole_tbl1` (\n"
      "  `id` int(11) DEFAULT NULL,\n"
      "  UNIQUE KEY `id` (`id`)\n"
      ") ENGINE=InnoDB DEFAULT CHARSET=latin1;");

  EXPECT_TABLE(
      "myisam_tbl2",
      {"Table 'mysqlaas_compat'.'myisam_tbl2' uses unsupported "
       "charset: utf8",
       "Table 'mysqlaas_compat'.'myisam_tbl2' had unsupported engine MyISAM "
       "changed to InnoDB"},
      "CREATE TABLE IF NOT EXISTS `myisam_tbl2` (\n"
      "  `id` int(11) NOT NULL AUTO_INCREMENT,\n"
      "  `text` text,\n"
      "  PRIMARY KEY (`id`)\n"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8");

  // TABLESPACE
  EXPECT_TABLE("ts1_tbl1",
               {"Table 'mysqlaas_compat'.'ts1_tbl1' had unsupported tablespace "
                "option removed"},
               "CREATE TABLE IF NOT EXISTS `ts1_tbl1` (\n"
               "  `pk` int(11) NOT NULL,\n"
               "  `val` varchar(40) DEFAULT NULL,\n"
               "  PRIMARY KEY (`pk`)\n"
               ")  ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

  EXPECT_TABLE("ts1_tbl2", {},
               "CREATE TABLE IF NOT EXISTS `ts1_tbl2` (\n"
               "  `pk` int(11) NOT NULL,\n"
               "  `val` varchar(40) DEFAULT NULL,\n"
               "  PRIMARY KEY (`pk`)\n"
               ") /*!50100 TABLESPACE `innodb_system` */ ENGINE=InnoDB DEFAULT "
               "CHARSET=utf8mb4");

  // DATA/INDEX DIRECTORY
  if (run_directory_tests) {
    EXPECT_TABLE("path_tbl1",
                 {"Table 'mysqlaas_compat'.'path_tbl1' uses unsupported "
                  "charset: latin1",
                  "Table 'mysqlaas_compat'.'path_tbl1' had {DATA|INDEX} "
                  "DIRECTORY table option commented out"},
                 "CREATE TABLE IF NOT EXISTS `path_tbl1` (\n"
                 "  `pk` int(11) NOT NULL,\n"
                 "  PRIMARY KEY (`pk`)\n"
                 ") ENGINE=InnoDB DEFAULT CHARSET=latin1 /* DATA "
                 "DIRECTORY='" +
                     shcore::path::tmpdir() + "/'*/ ;");

    EXPECT_TABLE("path_tbl2",
                 {"Table 'mysqlaas_compat'.'path_tbl2' uses unsupported "
                  "charset: latin1",
                  "Table 'mysqlaas_compat'.'path_tbl2' had {DATA|INDEX} "
                  "DIRECTORY table option commented out",
                  "Table 'mysqlaas_compat'.'path_tbl2' had unsupported engine "
                  "MyISAM changed to InnoDB"},
                 "CREATE TABLE IF NOT EXISTS `path_tbl2` (\n"
                 "  `pk` int(11) NOT NULL,\n"
                 "  PRIMARY KEY (`pk`)\n"
                 ") ENGINE=InnoDB DEFAULT CHARSET=latin1 /* DATA "
                 "DIRECTORY='" +
                     shcore::path::tmpdir() + "/' INDEX DIRECTORY='" +
                     shcore::path::tmpdir() + "/'*/ ;");

    EXPECT_TABLE("path_tbl3",
                 {"Table 'mysqlaas_compat'.'path_tbl3' uses unsupported "
                  "charset: latin1",
                  "Table 'mysqlaas_compat'.'path_tbl3' had {DATA|INDEX} "
                  "DIRECTORY table option commented out",
                  "Table 'mysqlaas_compat'.'path_tbl3' had unsupported engine "
                  "MyISAM changed to InnoDB"},
                 "CREATE TABLE IF NOT EXISTS `path_tbl3` (\n"
                 "  `pk` int(11) NOT NULL,\n  PRIMARY KEY (`pk`)\n"
                 ") ENGINE=InnoDB DEFAULT CHARSET=latin1 /* INDEX "
                 "DIRECTORY='" +
                     shcore::path::tmpdir() + "/'*/ ;");

    EXPECT_TABLE("part_tbl2",
                 {"Table 'mysqlaas_compat'.'part_tbl2' uses unsupported "
                  "charset: latin1",
                  "Table 'mysqlaas_compat'.'part_tbl2' had {DATA|INDEX} "
                  "DIRECTORY table option commented out"},
                 "CREATE TABLE IF NOT EXISTS `part_tbl2` (\n"
                 "  `pk` int(11) NOT NULL,\n"
                 "  PRIMARY KEY (`pk`)\n"
                 ") ENGINE=InnoDB DEFAULT CHARSET=latin1\n"
                 "/*!50100 PARTITION BY HASH (pk)\n"
                 "(PARTITION p1 -- DATA DIRECTORY = '" +
                     shcore::path::tmpdir() +
                     "/'\n"
                     " ENGINE = InnoDB,\n"
                     " PARTITION p2 -- DATA DIRECTORY = '" +
                     shcore::path::tmpdir() +
                     "/'\n"
                     " ENGINE = InnoDB) */;");

    EXPECT_TABLE("part_tbl3",
                 {"Table 'mysqlaas_compat'.'part_tbl3' uses unsupported "
                  "charset: latin1",
                  "Table 'mysqlaas_compat'.'part_tbl3' had {DATA|INDEX} "
                  "DIRECTORY table option commented out"},
                 "CREATE TABLE IF NOT EXISTS `part_tbl3` (\n"
                 "  `pk` int(11) NOT NULL,\n"
                 "  PRIMARY KEY (`pk`)\n"
                 ") ENGINE=InnoDB DEFAULT CHARSET=latin1\n"
                 "/*!50100 PARTITION BY RANGE (pk)\n"
                 "SUBPARTITION BY HASH (pk)\n"
                 "(PARTITION p1 VALUES LESS THAN (100)\n"
                 " (SUBPARTITION sp1 -- DATA DIRECTORY = '" +
                     shcore::path::tmpdir() +
                     "/'\n"
                     " ENGINE = InnoDB)) */;");
  }

  // definer clauses
  const auto EXPECT_DUMP_CONTAINS = [&](std::vector<const char *> items) {
    file->flush();
    file->close();
    auto out = testutil->cat_file(file_path);
    out = shcore::str_replace(out, "`", "");
    out = shcore::str_replace(out, "(11)", "");
    for (const auto i : items)
      if (std::string::npos == out.find(shcore::str_replace(i, "`", ""))) {
        EXPECT_EQ(out, i);
      }
    file.reset(new mysqlshdk::storage::backend::File(file_path));
    file->open(mysqlshdk::storage::Mode::WRITE);
  };

  file.reset(new mysqlshdk::storage::backend::File(file_path));
  file->open(mysqlshdk::storage::Mode::WRITE);
  std::vector<Schema_dumper::Issue> iss;
  EXPECT_NO_THROW(iss = sd.dump_triggers_for_table_ddl(
                      file.get(), compat_db_name, "myisam_tbl1"));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ("Trigger mysqlaas_compat.ins_sum had definer clause removed",
            iss.front().description);
  EXPECT_DUMP_CONTAINS(
      {"/*!50003 CREATE TRIGGER ins_sum BEFORE INSERT ON myisam_tbl1"});

  EXPECT_NO_THROW(iss = sd.dump_events_ddl(file.get(), compat_db_name));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ("Event mysqlaas_compat.event2 had definer clause removed",
            iss.front().description);
  EXPECT_DUMP_CONTAINS({"/*!50106 CREATE EVENT IF NOT EXISTS `event2`"});

  EXPECT_NO_THROW(iss = sd.dump_routines_ddl(file.get(), compat_db_name));
  ASSERT_EQ(5, iss.size());
  EXPECT_EQ(
      "Function mysqlaas_compat.func1 had definer clause removed and SQL "
      "SECURITY characteristic set to INVOKER",
      iss[0].description);
  EXPECT_EQ("Function mysqlaas_compat.func2 had definer clause removed",
            iss[1].description);
  EXPECT_EQ(
      "Procedure mysqlaas_compat.labeled had definer clause removed and SQL "
      "SECURITY characteristic set to INVOKER",
      iss[2].description);
  EXPECT_EQ(
      "Procedure mysqlaas_compat.proc1 had definer clause removed and SQL "
      "SECURITY characteristic set to INVOKER",
      iss[3].description);
  EXPECT_EQ("Procedure mysqlaas_compat.proc2 had definer clause removed",
            iss[4].description);
  EXPECT_DUMP_CONTAINS(
      {"CREATE FUNCTION `func1`() RETURNS int\n"
       "    NO SQL\n"
       "SQL SECURITY INVOKER\n"
       "RETURN 0",
       "CREATE FUNCTION `func2`() RETURNS int\n"
       "    NO SQL\n"
       "    SQL SECURITY INVOKER\n"
       "RETURN 0 ;",
       "CREATE PROCEDURE labeled()\n"
       "SQL SECURITY INVOKER\n"
       "wholeblock:BEGIN\n"
       "  DECLARE x INT;\n"
       "  DECLARE str VARCHAR(255);\n"
       "  SET x = -5;\n"
       "  SET str = '';\n"
       "\n"
       "  loop_label: LOOP\n"
       "    IF x > 0 THEN\n"
       "      LEAVE loop_label;\n"
       "    END IF;\n"
       "    SET str = CONCAT(str,x,',');\n"
       "    SET x = x + 1;\n"
       "    ITERATE loop_label;\n"
       "  END LOOP;\n"
       "\n"
       "  SELECT str;\n"
       "\n"
       "END ;;",
       "CREATE PROCEDURE `proc1`()\n    NO SQL\nSQL SECURITY "
       "INVOKER\nBEGIN\nEND",
       "CREATE PROCEDURE `proc2`()\n    NO SQL\n    SQL SECURITY "
       "INVOKER\nBEGIN\nEND"});

  EXPECT_NO_THROW(iss = sd.dump_view_ddl(file.get(), compat_db_name, "view1"));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ(
      "View mysqlaas_compat.view1 had definer clause removed and SQL SECURITY "
      "characteristic set to INVOKER",
      iss.front().description);
  EXPECT_DUMP_CONTAINS(
      {"/*!50001 CREATE ALGORITHM=UNDEFINED SQL SECURITY INVOKER VIEW view1 AS "
       "select 1 AS 1 */;"});

  EXPECT_NO_THROW(iss = sd.dump_view_ddl(file.get(), compat_db_name, "view3"));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ("View mysqlaas_compat.view3 had definer clause removed",
            iss.front().description);
  EXPECT_DUMP_CONTAINS(
      {"/*!50001 CREATE ALGORITHM=UNDEFINED SQL SECURITY INVOKER VIEW view3 AS "
       "select 1 AS 1 */"});
}

TEST_F(Schema_dumper_test, dump_and_load) {
  for (const char *db : {compat_db_name, db_name, crazy_names_db}) {
    if (!file->is_open()) file->open(mysqlshdk::storage::Mode::WRITE);
    if (!session->is_open()) session = connect_session();
    Schema_dumper sd(session);
    sd.opt_drop_trigger = true;
    sd.opt_mysqlaas = true;
    sd.opt_force_innodb = true;
    sd.opt_strip_restricted_grants = true;
    sd.opt_strip_tablespaces = true;
    sd.opt_strip_definer = true;
    EXPECT_NO_THROW(sd.write_header(file.get(), db));
    EXPECT_NO_THROW(sd.dump_schema_ddl(file.get(), db));
    session->execute(std::string("use ") + db);
    auto res = session->query("show tables", true);
    std::vector<std::string> tables;
    while (auto row = res->fetch_one()) tables.emplace_back(row->get_string(0));
    for (const auto &table : tables) {
      EXPECT_NO_THROW(sd.dump_table_ddl(file.get(), db, table));
      if (sd.count_triggers_for_table(db, table))
        EXPECT_NO_THROW(sd.dump_triggers_for_table_ddl(file.get(), db, table));
    }
    EXPECT_NO_THROW(sd.dump_events_ddl(file.get(), db));
    EXPECT_NO_THROW(sd.dump_routines_ddl(file.get(), db));
    res = session->query(
        std::string("select TABLE_NAME FROM information_schema.views where "
                    "TABLE_SCHEMA = '") +
            db + "'",
        true);
    std::vector<std::string> views;
    while (auto row = res->fetch_one()) views.emplace_back(row->get_string(0));
    for (const auto &view : views) {
      EXPECT_NO_THROW(sd.dump_view_ddl(file.get(), db, view));
    }

    EXPECT_NO_THROW(sd.write_footer(file.get()));
    EXPECT_TRUE(output_handler.std_err.empty());
    wipe_all();
    file->flush();
    file->close();
    session->close();
    testutil->call_mysqlsh_c({_mysql_uri, "--sql", "-f", file_path});
    EXPECT_TRUE(output_handler.std_err.empty());
  }
}

}  // namespace mysqlsh
