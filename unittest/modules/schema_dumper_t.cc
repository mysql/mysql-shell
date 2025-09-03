/* Copyright (c) 2020, 2025, Oracle and/or its affiliates.
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

#include <stdio.h>

#include <algorithm>
#include <array>
#include <set>

// needs to be included first for FRIEND_TEST
#include "unittest/gprod_clean.h"

#include "modules/util/dump/schema_dumper.h"

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/storage/backend/file.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

extern "C" const char *g_test_home;

namespace mysqlsh {
namespace dump {

using ::testing::AnyOf;
using ::testing::HasSubstr;
using ::testing::Not;

using mysqlshdk::db::Filtering_options;
using User_filters = Filtering_options::User_filters;

std::string to_string(Schema_dumper::Issue::Status s) {
  using Status = Schema_dumper::Issue::Status;

  switch (s) {
    case Status::FIXED_BY_CREATE_INVISIBLE_PKS:
      return "FIXED_BY_CREATE_INVISIBLE_PKS";

    case Status::FIXED_BY_IGNORE_MISSING_PKS:
      return "FIXED_BY_IGNORE_MISSING_PKS";

    case Status::FIXED:
      return "FIXED";

    case Status::NOTE:
      return "NOTE";

    case Status::WARNING_DEPRECATED_DEFINERS:
      return "WARNING_DEPRECATED_STRIP_DEFINERS";

    case Status::WARNING_ESCAPED_WILDCARDS:
      return "WARNING_ESCAPED_WILDCARDS";

    case Status::WARNING_INVALID_VIEW_REFERENCE:
      return "WARNING_INVALID_VIEW_REFERENCE";

    case Status::WARNING:
      return "WARNING";

    case Status::FIX_MANUALLY:
      return "FIX_MANUALLY";

    case Status::FIX_WILDCARD_GRANTS:
      return "FIX_WILDCARD_GRANTS";

    case Status::FIX_INVALID_VIEW_REFERENCE:
      return "FIX_INVALID_VIEW_REFERENCE";

    case Status::USE_CREATE_OR_IGNORE_PKS:
      return "USE_CREATE_OR_IGNORE_PKS";

    case Status::USE_FORCE_INNODB:
      return "USE_FORCE_INNODB";

    case Status::USE_STRIP_DEFINERS:
      return "USE_STRIP_DEFINERS";

    case Status::USE_STRIP_RESTRICTED_GRANTS:
      return "USE_STRIP_RESTRICTED_GRANTS";

    case Status::USE_STRIP_TABLESPACES:
      return "USE_STRIP_TABLESPACES";

    case Status::USE_LOCK_OR_SKIP_INVALID_ACCOUNTS:
      return "USE_LOCK_OR_SKIP_INVALID_ACCOUNTS";

    case Status::USE_STRIP_INVALID_GRANTS:
      return "USE_STRIP_INVALID_GRANTS";
  }

  throw std::logic_error("Should not happen");
}

std::ostream &operator<<(std::ostream &os, const Schema_dumper::Issue &issue) {
  return os << issue.description << " - " << to_string(issue.status);
}

class Schema_dumper_test : public Shell_core_test_wrapper {
 public:
  static void TearDownTestCase() {
    if (initialized) {
      auto session = connect_session();
      session->execute(std::string("DROP SCHEMA IF EXISTS ") + db_name);
      session->execute(std::string("DROP SCHEMA IF EXISTS ") + compat_db_name);
      session->execute(std::string("DROP SCHEMA IF EXISTS ") + crazy_names_db);
      session->execute("DROP USER IF EXISTS testusr1@localhost;");
      session->execute("DROP USER IF EXISTS testusr2@localhost;");
      session->execute("DROP USER IF EXISTS testusr3@localhost;");
      session->execute("DROP USER IF EXISTS testusr4@localhost;");
      session->execute("DROP USER IF EXISTS testusr5@localhost;");
      session->execute("DROP USER IF EXISTS testusr6@localhost;");
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
          shcore::get_os_type() != shcore::OperatingSystem::WINDOWS_OS;
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
    session->connect(mysqlshdk::db::Connection_options(_mysql_uri));
    return session;
  }

  void expect_output_eq(const char *res) {
    file->flush();
    file->close();
    EXPECT_EQ(res, testutil->cat_file(file_path));
  }

  void expect_output_contains(std::vector<const char *> items,
                              std::string *output = nullptr) {
    file->flush();
    file->close();
    auto out = testutil->cat_file(file_path);
    if (output != nullptr) *output = out;
    for (const auto i : items) {
      EXPECT_THAT(out, HasSubstr(i));
    }
  }

  auto create_table_in_mysql_schema_for_grant(const std::string &user) {
    session->execute("create table IF NOT EXISTS mysql.abradabra (i integer);");
    session->execute("grant select on mysql.abradabra to " + user);
    const auto deleter = [s = session](mysqlshdk::db::ISession *) {
      s->execute("drop table mysql.abradabra;");
    };
    return std::unique_ptr<mysqlshdk::db::ISession, decltype(deleter)>(
        session.get(), std::move(deleter));
  }

  std::shared_ptr<mysqlshdk::db::ISession> session;
  std::string file_path;
  std::unique_ptr<IFile> file;
  static bool run_directory_tests;
  static bool initialized;
  static constexpr auto db_name = "mysqldump_test_db";
  static constexpr auto compat_db_name = "mysqlaas_compat";
  static constexpr auto crazy_names_db = "crazy_names_db";
};

bool Schema_dumper_test::run_directory_tests = false;
bool Schema_dumper_test::initialized = false;

TEST_F(Schema_dumper_test, dump_table) {
  Schema_dumper sd(session);
  sd.opt_drop_table = true;
  EXPECT_NO_THROW(sd.dump_table_ddl(file.get(), db_name, "at1"));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;

  const char *res = R"(
--
-- Table structure for table `at1`
--

DROP TABLE IF EXISTS `at1`;
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
  sd.opt_drop_table = true;
  sd.opt_drop_trigger = true;
  EXPECT_NO_THROW(sd.dump_table_ddl(file.get(), db_name, "t1"));
  if (sd.count_triggers_for_table(db_name, "t1")) {
    EXPECT_NO_THROW(sd.dump_triggers_for_table_ddl(file.get(), db_name, "t1"));
  }
  EXPECT_NO_THROW(sd.dump_table_ddl(file.get(), db_name, "t2"));
  if (sd.count_triggers_for_table(db_name, "t2")) {
    EXPECT_NO_THROW(sd.dump_triggers_for_table_ddl(file.get(), db_name, "t2"));
  }
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;

  const char *res = R"(
--
-- Table structure for table `t1`
--

DROP TABLE IF EXISTS `t1`;
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

-- begin trigger `mysqldump_test_db`.`trg1`
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
/*!50032 DROP TRIGGER IF EXISTS `trg1` */;
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
-- end trigger `mysqldump_test_db`.`trg1`

-- begin trigger `mysqldump_test_db`.`trg2`
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
/*!50032 DROP TRIGGER IF EXISTS `trg2` */;
DELIMITER ;;
/*!50003 CREATE DEFINER=`root`@`localhost` TRIGGER `trg2` BEFORE UPDATE ON `t1` FOR EACH ROW begin
  if old.a % 2 = 0 then set new.b := 12; end if;
end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
-- end trigger `mysqldump_test_db`.`trg2`

-- begin trigger `mysqldump_test_db`.`trg3`
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,TRADITIONAL,NO_ENGINE_SUBSTITUTION' */ ;
/*!50032 DROP TRIGGER IF EXISTS `trg3` */;
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
-- end trigger `mysqldump_test_db`.`trg3`


--
-- Table structure for table `t2`
--

DROP TABLE IF EXISTS `t2`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE IF NOT EXISTS `t2` (
  `a` int DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping triggers for table 'mysqldump_test_db'.'t2'
--

-- begin trigger `mysqldump_test_db`.`trg4`
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,TRADITIONAL,NO_ENGINE_SUBSTITUTION' */ ;
/*!50032 DROP TRIGGER IF EXISTS `trg4` */;
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
-- end trigger `mysqldump_test_db`.`trg4`

)";

  expect_output_eq(res);
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_schema) {
  Schema_dumper sd(session);
  sd.opt_mysqlaas = true;
  EXPECT_NO_THROW(sd.write_header(file.get()));
  EXPECT_GE(1, sd.dump_schema_ddl(file.get(), db_name).size());
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
-- Dumping database 'mysqldump_test_db'
--

-- begin database `mysqldump_test_db`
CREATE DATABASE /*!32312 IF NOT EXISTS*/ `mysqldump_test_db` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;
-- end database `mysqldump_test_db`

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
  sd.opt_drop_view = true;
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

DROP TABLE IF EXISTS `v1`;
/*!50001 DROP VIEW IF EXISTS `v1`*/;
SET @saved_cs_client     = @@character_set_client;
/*!50503 SET character_set_client = utf8mb4 */;
/*!50001 CREATE VIEW `v1` AS SELECT
 1 AS `a`,
 1 AS `b`,
 1 AS `c` */;
SET character_set_client = @saved_cs_client;

--
-- Temporary view structure for view `v2`
--

DROP TABLE IF EXISTS `v2`;
/*!50001 DROP VIEW IF EXISTS `v2`*/;
SET @saved_cs_client     = @@character_set_client;
/*!50503 SET character_set_client = utf8mb4 */;
/*!50001 CREATE VIEW `v2` AS SELECT
 1 AS `a` */;
SET character_set_client = @saved_cs_client;

--
-- Temporary view structure for view `v3`
--

DROP TABLE IF EXISTS `v3`;
/*!50001 DROP VIEW IF EXISTS `v3`*/;
SET @saved_cs_client     = @@character_set_client;
/*!50503 SET character_set_client = utf8mb4 */;
/*!50001 CREATE VIEW `v3` AS SELECT
 1 AS `a`,
 1 AS `b`,
 1 AS `c` */;
SET character_set_client = @saved_cs_client;

--
-- Final view structure for view `v1`
--

/*!50001 DROP VIEW IF EXISTS `v1`*/;
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

/*!50001 DROP VIEW IF EXISTS `v2`*/;
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

/*!50001 DROP VIEW IF EXISTS `v3`*/;
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

-- begin event `mysqldump_test_db`.`ee1`
/*!50106 DROP EVENT IF EXISTS `ee1` */;
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
-- end event `mysqldump_test_db`.`ee1`

-- begin event `mysqldump_test_db`.`ee2`
/*!50106 DROP EVENT IF EXISTS `ee2` */;;
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
-- end event `mysqldump_test_db`.`ee2`

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

-- begin function `mysqldump_test_db`.`bug9056_func1`
/*!50003 DROP FUNCTION IF EXISTS `bug9056_func1` */;
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
-- end function `mysqldump_test_db`.`bug9056_func1`

-- begin function `mysqldump_test_db`.`bug9056_func2`
/*!50003 DROP FUNCTION IF EXISTS `bug9056_func2` */;
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
-- end function `mysqldump_test_db`.`bug9056_func2`

-- begin procedure `mysqldump_test_db`.`a'b`
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
-- end procedure `mysqldump_test_db`.`a'b`

-- begin procedure `mysqldump_test_db`.`bug9056_proc1`
/*!50003 DROP PROCEDURE IF EXISTS `bug9056_proc1` */;
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
-- end procedure `mysqldump_test_db`.`bug9056_proc1`

-- begin procedure `mysqldump_test_db`.`bug9056_proc2`
/*!50003 DROP PROCEDURE IF EXISTS `bug9056_proc2` */;
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
-- end procedure `mysqldump_test_db`.`bug9056_proc2`

)";
  expect_output_eq(res);
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_libraries) {
  Schema_dumper sd(session);
  EXPECT_NO_THROW(sd.dump_libraries_ddl(file.get(), db_name));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();

  if (!compatibility::supports_library_ddl(_target_server_version)) {
    return;
  }

  expect_output_eq(R"(
--
-- Dumping libraries for database 'mysqldump_test_db'
--

-- begin library `mysqldump_test_db`.`a'b`
DROP LIBRARY IF EXISTS `a'b`;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'REAL_AS_FLOAT,PIPES_AS_CONCAT,ANSI_QUOTES,IGNORE_SPACE,ONLY_FULL_GROUP_BY,ANSI' */ ;
DELIMITER ;;
CREATE LIBRARY "a'b"
    LANGUAGE JAVASCRIPT
AS $$
      export function f(n) {
        return n;
      }
    $$ ;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
-- end library `mysqldump_test_db`.`a'b`

-- begin library `mysqldump_test_db`.`lib1`
DROP LIBRARY IF EXISTS `lib1`;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
CREATE LIBRARY `lib1`
    LANGUAGE JAVASCRIPT
AS $$ $$ ;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
-- end library `mysqldump_test_db`.`lib1`

)");
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
  // setup
  session->execute(
      "CREATE USER IF NOT EXISTS 'first'@'localhost' IDENTIFIED BY 'pwd';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'first'@'10.11.12.13' IDENTIFIED BY 'pwd';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'firstfirst'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'second'@'localhost' IDENTIFIED BY 'pwd';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'second'@'10.11.12.14' IDENTIFIED BY 'pwd';");

  Schema_dumper sd(session);
  EXPECT_NO_THROW(sd.dump_grants(file.get()));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  std::string out;
  expect_output_contains({"CREATE USER IF NOT EXISTS", "GRANT"}, &out);

  EXPECT_THAT(out, HasSubstr("'second'"));

  const auto filtered =
      Schema_dumper::preprocess_users_script(out, [](const std::string &user) {
        return !shcore::str_beginswith(user, "'second'");
      });

  for (const auto &f : filtered) {
    EXPECT_THAT(f.account, Not(HasSubstr("'second'")));
  }

  // cleanup
  session->execute("DROP USER 'first'@'localhost';");
  session->execute("DROP USER 'first'@'10.11.12.13';");
  session->execute("DROP USER 'firstfirst'@'localhost';");
  session->execute("DROP USER 'second'@'localhost';");
  session->execute("DROP USER 'second'@'10.11.12.14';");
}

TEST_F(Schema_dumper_test, dump_filtered_grants) {
  session->execute(
      "CREATE USER IF NOT EXISTS 'admin'@'localhost' IDENTIFIED BY 'pwd';");
  session->execute("GRANT ALL ON *.* TO 'admin'@'localhost';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'admin2'@'localhost' IDENTIFIED BY 'pwd';");
  session->execute(
      "GRANT ALL ON *.* TO 'admin2'@'localhost' WITH GRANT OPTION;");
  session->execute(
      "CREATE USER IF NOT EXISTS 'dumptestuser'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute("GRANT SELECT ON * . * TO 'dumptestuser'@'localhost';");

  std::string partial_revoke = "ON";
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 20)) {
    partial_revoke = session->query("show variables like 'partial_revokes';")
                         ->fetch_one()
                         ->get_string(1);
    if (partial_revoke != "ON")
      session->execute("set global partial_revokes = 'ON';");
    session->execute("CREATE USER IF NOT EXISTS `dave`@`%` IDENTIFIED BY 'p'");
    session->execute(
        "GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, "
        "REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, "
        "LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE "
        "VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, "
        "TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `dave`@`%`");
    session->execute(
        "REVOKE SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, "
        "REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, "
        "LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE "
        "VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, "
        "TRIGGER, CREATE ROLE, DROP ROLE ON `mysql`.* FROM `dave`@`%`");

    session->execute("CREATE ROLE IF NOT EXISTS da_dumper");
    session->execute(
        "GRANT INSERT, BINLOG_ADMIN, UPDATE, ROLE_ADMIN, DELETE ON * . * TO "
        "'da_dumper';");
    session->execute("GRANT 'da_dumper' TO 'dumptestuser'@'localhost';");
  }

  Schema_dumper sd(session);
  sd.opt_mysqlaas = true;
  sd.opt_strip_restricted_grants = true;
  Filtering_options filters;
  filters.users().exclude(
      std::array{"mysql.infoschema", "mysql.session", "mysql.sys", "root"});
  sd.use_filters(&filters);
  EXPECT_GE(sd.dump_grants(file.get()).size(), 3);
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  file->flush();
  file->close();
  auto out = testutil->cat_file(file_path);

  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 0)) {
    EXPECT_THAT(
        out,
        HasSubstr("GRANT "
                  "SELECT,INSERT,UPDATE,DELETE,CREATE,DROP,PROCESS,REFERENCES,"
                  "INDEX,ALTER,SHOW DATABASES,CREATE TEMPORARY TABLES,LOCK "
                  "TABLES,EXECUTE,REPLICATION SLAVE,REPLICATION CLIENT,CREATE "
                  "VIEW,SHOW VIEW,CREATE ROUTINE,ALTER ROUTINE,CREATE "
                  "USER,EVENT,TRIGGER ON *.* TO 'admin'@'localhost';"));
    EXPECT_THAT(
        out,
        HasSubstr(
            "GRANT SELECT,INSERT,UPDATE,DELETE,CREATE,DROP,PROCESS,REFERENCES,"
            "INDEX,ALTER,SHOW DATABASES,CREATE TEMPORARY TABLES,LOCK "
            "TABLES,EXECUTE,REPLICATION SLAVE,REPLICATION CLIENT,CREATE "
            "VIEW,SHOW VIEW,CREATE ROUTINE,ALTER ROUTINE,CREATE "
            "USER,EVENT,TRIGGER ON *.* TO 'admin2'@'localhost' WITH GRANT "
            "OPTION;"));
    EXPECT_THAT(out, Not(HasSubstr("ALL PRIVILEGES")));
  }

  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 20)) {
    const std::string da_dumper =
        "CREATE ROLE IF NOT EXISTS 'da_dumper'@'%';\n"
        "ALTER USER 'da_dumper'@'%' IDENTIFIED WITH "
        "'caching_sha2_password' REQUIRE NONE PASSWORD EXPIRE ACCOUNT "
        "LOCK PASSWORD HISTORY DEFAULT PASSWORD REUSE INTERVAL DEFAULT "
        "PASSWORD REQUIRE CURRENT DEFAULT;";
    EXPECT_THAT(
        out, AnyOf(HasSubstr(da_dumper),
                   HasSubstr(shcore::str_replace(da_dumper, "'da_dumper'@'%'",
                                                 "`da_dumper`@`%`"))));

    EXPECT_THAT(
        out,
        HasSubstr("GRANT INSERT, UPDATE, DELETE ON *.* TO `da_dumper`@`%`;"));
    EXPECT_THAT(
        out, HasSubstr("GRANT `da_dumper`@`%` TO `dumptestuser`@`localhost`;"));
    EXPECT_THAT(
        out,
        HasSubstr(
            "REVOKE SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, "
            "REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY "
            "TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION "
            "CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, "
            "CREATE USER, EVENT, TRIGGER ON `mysql`.* FROM `dave`@`%`;"));
  }

  const std::string dumptestuser =
      "CREATE USER IF NOT EXISTS 'dumptestuser'@'localhost'";
  EXPECT_THAT(out,
              AnyOf(HasSubstr(dumptestuser),
                    HasSubstr(shcore::str_replace(dumptestuser, "'", "`"))));

  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 20)) {
    EXPECT_THAT(out,
                HasSubstr("GRANT SELECT ON *.* TO `dumptestuser`@`localhost`"));
  } else {
    EXPECT_THAT(
        out, HasSubstr("GRANT SELECT ON *.* TO 'dumptestuser'@'localhost';"));
  }
  EXPECT_THAT(out, Not(HasSubstr("SUPER")));

  EXPECT_THAT(out, Not(HasSubstr("'root'")));
  EXPECT_THAT(out, Not(HasSubstr("EXISTS 'mysql")));

  wipe_all();
  testutil->call_mysqlsh_c({_mysql_uri, "--sql", "-f", file_path});
  EXPECT_TRUE(output_handler.std_err.empty());

  session->execute("drop user 'admin'@'localhost';");
  session->execute("drop user 'admin2'@'localhost';");
  session->execute("drop user 'dumptestuser'@'localhost';");

  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 20)) {
    session->execute("DROP ROLE da_dumper");
    session->execute("DROP USER `dave`@`%`");
    if (partial_revoke != "ON")
      session->execute("set global partial_revokes = 'OFF';");
  }
}

TEST_F(Schema_dumper_test, dump_filtered_grants_super_priv) {
  // Skip if version >= 8.0. Super is deprecated in 8.0.
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0)) {
    SKIP_TEST("SUPER has been deprecated in 8.0.");
  };

  session->execute(
      "CREATE USER IF NOT EXISTS 'superfirst'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute("GRANT SUPER ON *.* TO 'superfirst'@'localhost';");
  session->execute("GRANT LOCK TABLES ON *.* TO 'superfirst'@'localhost';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'superafter'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute("GRANT INSERT, UPDATE ON *.* TO 'superafter'@'localhost';");
  session->execute("GRANT SUPER ON *.* TO 'superafter'@'localhost';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'superonly'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute("GRANT RELOAD ON *.* TO 'superonly'@'localhost';");
  session->execute("GRANT SUPER ON *.* TO 'superonly'@'localhost';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'abr@dab'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute(
      "GRANT INSERT,SUPER,FILE,LOCK TABLES , reload, "
      "SELECT ON * . * TO 'abr@dab'@'localhost';");

  Schema_dumper sd(session);
  sd.opt_mysqlaas = true;
  sd.opt_strip_restricted_grants = true;
  Filtering_options filters;
  filters.users().exclude(
      std::array{"mysql.infoschema", "mysql.session", "mysql.sys", "root"});
  sd.use_filters(&filters);
  EXPECT_GE(sd.dump_grants(file.get()).size(), 3);
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  file->flush();
  file->close();
  auto out = testutil->cat_file(file_path);

  EXPECT_THAT(
      out, HasSubstr("GRANT LOCK TABLES ON *.* TO 'superfirst'@'localhost';"));
  EXPECT_THAT(
      out,
      HasSubstr("GRANT INSERT, UPDATE ON *.* TO 'superafter'@'localhost';"));
  EXPECT_THAT(out, Not(HasSubstr("TO 'superonly'@'localhost';")));

  EXPECT_THAT(out, HasSubstr("-- begin user 'abr@dab'@'localhost'"));
  EXPECT_THAT(out, HasSubstr(R"(-- begin grants 'abr@dab'@'localhost'
GRANT SELECT, INSERT, LOCK TABLES ON *.* TO 'abr@dab'@'localhost';
-- end grants 'abr@dab'@'localhost')"));

  EXPECT_THAT(out, Not(HasSubstr("SUPER")));

  EXPECT_THAT(out, Not(HasSubstr("'root'")));
  EXPECT_THAT(out, Not(HasSubstr("EXISTS 'mysql")));

  wipe_all();
  testutil->call_mysqlsh_c({_mysql_uri, "--sql", "-f", file_path});
  EXPECT_TRUE(output_handler.std_err.empty());

  session->execute("drop user 'superfirst'@'localhost';");
  session->execute("drop user 'superafter'@'localhost';");
  session->execute("drop user 'superonly'@'localhost';");
  session->execute("drop user 'abr@dab'@'localhost';");
}

TEST_F(Schema_dumper_test, opt_mysqlaas) {
  Schema_dumper sd(session);
  sd.opt_mysqlaas = true;
  sd.opt_pk_mandatory_check = true;

  session->execute(std::string("use ") + compat_db_name);
  auto mstg = create_table_in_mysql_schema_for_grant("testusr6@localhost");

  const auto EXPECT_TABLE = [&](const std::string &table,
                                std::vector<std::string> msg) {
    SCOPED_TRACE(table);
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
               {"Table `mysqlaas_compat`.`myisam_tbl1` uses unsupported "
                "storage engine MyISAM"});

  EXPECT_TABLE(
      "blackhole_tbl1",
      {"Table `mysqlaas_compat`.`blackhole_tbl1` does not have primary or "
       "unique non null key defined",
       "Table `mysqlaas_compat`.`blackhole_tbl1` uses unsupported storage "
       "engine BLACKHOLE",
       "Table `mysqlaas_compat`.`blackhole_tbl1` does not have a Primary Key, "
       "which is required for High Availability in MySQL HeatWave Service"});

  EXPECT_TABLE("myisam_tbl2",
               {"Table `mysqlaas_compat`.`myisam_tbl2` uses unsupported "
                "storage engine MyISAM"});

  // TABLESPACE
  EXPECT_TABLE("ts1_tbl1", {"Table `mysqlaas_compat`.`ts1_tbl1` uses "
                            "unsupported tablespace option"});

  // DEFINERS
  std::vector<Schema_dumper::Issue> iss;
  EXPECT_NO_THROW(iss = sd.dump_triggers_for_table_ddl(
                      file.get(), compat_db_name, "myisam_tbl1"));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ(
      "Trigger `mysqlaas_compat`.`ins_sum` - definition uses DEFINER clause "
      "set to user `root`@`localhost` which can only be executed by this user "
      "or a user with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
      iss.front().description);

  EXPECT_NO_THROW(iss = sd.dump_events_ddl(file.get(), compat_db_name));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ(
      "Event `mysqlaas_compat`.`event2` - definition uses DEFINER clause set "
      "to user `root`@`localhost` which can only be executed by this user or a "
      "user with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
      iss.front().description);

  const auto user = "`" + _user + "`@`" + _host + "`";
  EXPECT_NO_THROW(iss = sd.dump_routines_ddl(file.get(), compat_db_name));
  ASSERT_EQ(8, iss.size());
  EXPECT_EQ(
      "Function `mysqlaas_compat`.`func1` - definition uses DEFINER clause set "
      "to user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
      iss[0].description);
  EXPECT_EQ(
      "Function `mysqlaas_compat`.`func1` - definition does not use SQL "
      "SECURITY INVOKER characteristic, which is mandatory when the DEFINER "
      "clause is omitted or removed",
      iss[1].description);
  EXPECT_EQ(
      "Function `mysqlaas_compat`.`func2` - definition uses DEFINER clause set "
      "to user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
      iss[2].description);
  EXPECT_EQ(
      "Procedure `mysqlaas_compat`.`labeled` - definition uses DEFINER clause "
      "set to user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
      iss[3].description);
  EXPECT_EQ(
      "Procedure `mysqlaas_compat`.`labeled` - definition does not use SQL "
      "SECURITY INVOKER characteristic, which is mandatory when the DEFINER "
      "clause is omitted or removed",
      iss[4].description);
  EXPECT_EQ(
      "Procedure `mysqlaas_compat`.`proc1` - definition uses DEFINER clause "
      "set to user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
      iss[5].description);
  EXPECT_EQ(
      "Procedure `mysqlaas_compat`.`proc1` - definition does not use SQL "
      "SECURITY INVOKER characteristic, which is mandatory when the DEFINER "
      "clause is omitted or removed",
      iss[6].description);
  EXPECT_EQ(
      "Procedure `mysqlaas_compat`.`proc2` - definition uses DEFINER clause "
      "set to user " +
          user +
          " which can only be executed by this user or a "
          "user with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
      iss[7].description);

  EXPECT_NO_THROW(iss = sd.dump_view_ddl(file.get(), compat_db_name, "view1"));
  ASSERT_EQ(2, iss.size());
  EXPECT_EQ(
      "View `mysqlaas_compat`.`view1` - definition uses DEFINER clause set to "
      "user " +
          user +
          " which can only be executed by this user or a user "
          "with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
      iss[0].description);
  EXPECT_EQ(
      "View `mysqlaas_compat`.`view1` - definition does not use SQL SECURITY "
      "INVOKER characteristic, which is mandatory when the DEFINER clause is "
      "omitted or removed",
      iss[1].description);

  EXPECT_NO_THROW(iss = sd.dump_view_ddl(file.get(), compat_db_name, "view3"));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ(
      "View `mysqlaas_compat`.`view3` - definition uses DEFINER clause set to "
      "user " +
          user +
          " which can only be executed by this user or a user "
          "with SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
      iss[0].description);

  // Users
  Filtering_options filters;
  filters.users().exclude(std::array{"mysql", "root"});
  sd.use_filters(&filters);
  EXPECT_NO_THROW(iss = sd.dump_grants(file.get()));

  auto expected_issues = run_directory_tests ?
      std::set<std::string>{
      "User 'testusr1'@'localhost' is granted restricted privileges: FILE, "
      "RELOAD, SUPER",
      "User 'testusr2'@'localhost' is granted restricted privilege: SUPER",
      "User 'testusr3'@'localhost' is granted restricted privileges: FILE, "
      "RELOAD",
      "User 'testusr4'@'localhost' is granted restricted privilege: SUPER",
      "User 'testusr5'@'localhost' is granted restricted privilege: FILE",
      "User 'testusr6'@'localhost' is granted restricted privilege: FILE",
      "User 'testusr6'@'localhost' has explicit grants on mysql schema object:"
      " `mysql`.`abradabra`"} :
      std::set<std::string>{
        "User 'testusr1'@'localhost' is granted restricted privileges: "
        "BINLOG_ADMIN, FILE, RELOAD",
        "User 'testusr3'@'localhost' is granted restricted privileges: "
        "BINLOG_ADMIN, FILE, RELOAD",
        "User 'testusr5'@'localhost' is granted restricted privilege: FILE",
        "User 'testusr6'@'localhost' is granted restricted privilege: FILE",
        "User 'testusr6'@'localhost' has explicit grants on mysql "
        "schema object: `mysql`.`abradabra`"};
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
  auto mstg = create_table_in_mysql_schema_for_grant("testusr6@localhost");

  const auto EXPECT_TABLE = [&](const std::string &table,
                                std::vector<std::string> msg,
                                const std::string &ddl = "") {
    SCOPED_TRACE(table);
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
    EXPECT_THAT(out, HasSubstr(_target_server_version >=
                                       mysqlshdk::utils::Version(8, 0, 0)
                                   ? shcore::str_replace(ddl, "(11)", "")
                                   : ddl));
  };

  // Engines
  EXPECT_TABLE(
      "myisam_tbl1",
      {"Table `mysqlaas_compat`.`myisam_tbl1` had unsupported engine MyISAM "
       "changed to InnoDB"},
      "CREATE TABLE IF NOT EXISTS `myisam_tbl1` (\n"
      "  `id` int(11) NOT NULL AUTO_INCREMENT,\n"
      "  PRIMARY KEY (`id`)\n"
      ") ENGINE=InnoDB DEFAULT CHARSET=latin1");

  sd.opt_create_invisible_pks = true;
  sd.opt_ignore_missing_pks = false;

  EXPECT_TABLE(
      "blackhole_tbl1",
      {"Table `mysqlaas_compat`.`blackhole_tbl1` had unsupported engine "
       "BLACKHOLE changed to InnoDB",
       "Table `mysqlaas_compat`.`blackhole_tbl1` does not have a Primary Key, "
       "this will be fixed when the dump is loaded"},
      "CREATE TABLE IF NOT EXISTS `blackhole_tbl1` (\n"
      "  `id` int(11) DEFAULT NULL,\n"
      "  UNIQUE KEY `id` (`id`)\n"
      ") ENGINE=InnoDB DEFAULT CHARSET=latin1;");

  sd.opt_create_invisible_pks = false;
  sd.opt_ignore_missing_pks = true;

  EXPECT_TABLE(
      "blackhole_tbl1",
      {"Table `mysqlaas_compat`.`blackhole_tbl1` had unsupported engine "
       "BLACKHOLE changed to InnoDB",
       "Table `mysqlaas_compat`.`blackhole_tbl1` does not have a Primary Key, "
       "this is ignored"},
      "CREATE TABLE IF NOT EXISTS `blackhole_tbl1` (\n"
      "  `id` int(11) DEFAULT NULL,\n"
      "  UNIQUE KEY `id` (`id`)\n"
      ") ENGINE=InnoDB DEFAULT CHARSET=latin1;");

  sd.opt_create_invisible_pks = true;
  sd.opt_ignore_missing_pks = true;

  EXPECT_TABLE(
      "blackhole_tbl1",
      {"Table `mysqlaas_compat`.`blackhole_tbl1` had unsupported engine "
       "BLACKHOLE changed to InnoDB",
       "Table `mysqlaas_compat`.`blackhole_tbl1` does not have a Primary Key, "
       "this will be fixed when the dump is loaded"},
      "CREATE TABLE IF NOT EXISTS `blackhole_tbl1` (\n"
      "  `id` int(11) DEFAULT NULL,\n"
      "  UNIQUE KEY `id` (`id`)\n"
      ") ENGINE=InnoDB DEFAULT CHARSET=latin1;");

  sd.opt_create_invisible_pks = false;
  sd.opt_ignore_missing_pks = false;

  EXPECT_TABLE(
      "myisam_tbl2",
      {"Table `mysqlaas_compat`.`myisam_tbl2` had unsupported engine MyISAM "
       "changed to InnoDB"},
      "CREATE TABLE IF NOT EXISTS `myisam_tbl2` (\n"
      "  `id` int(11) NOT NULL AUTO_INCREMENT,\n"
      "  `text` text,\n"
      "  PRIMARY KEY (`id`)\n"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8");

  // TABLESPACE
  EXPECT_TABLE("ts1_tbl1",
               {"Table `mysqlaas_compat`.`ts1_tbl1` had unsupported tablespace "
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
                 {"Table `mysqlaas_compat`.`path_tbl1` had {DATA|INDEX} "
                  "DIRECTORY table option commented out"},
                 "CREATE TABLE IF NOT EXISTS `path_tbl1` (\n"
                 "  `pk` int(11) NOT NULL,\n"
                 "  PRIMARY KEY (`pk`)\n"
                 ") ENGINE=InnoDB DEFAULT CHARSET=latin1 /* DATA "
                 "DIRECTORY='" +
                     shcore::path::tmpdir() + "/'*/;");

    EXPECT_TABLE("path_tbl2",
                 {"Table `mysqlaas_compat`.`path_tbl2` had {DATA|INDEX} "
                  "DIRECTORY table option commented out",
                  "Table `mysqlaas_compat`.`path_tbl2` had unsupported engine "
                  "MyISAM changed to InnoDB"},
                 "CREATE TABLE IF NOT EXISTS `path_tbl2` (\n"
                 "  `pk` int(11) NOT NULL,\n"
                 "  PRIMARY KEY (`pk`)\n"
                 ") ENGINE=InnoDB DEFAULT CHARSET=latin1 /* DATA "
                 "DIRECTORY='" +
                     shcore::path::tmpdir() + "/' INDEX DIRECTORY='" +
                     shcore::path::tmpdir() + "/'*/;");

    EXPECT_TABLE("path_tbl3",
                 {"Table `mysqlaas_compat`.`path_tbl3` had {DATA|INDEX} "
                  "DIRECTORY table option commented out",
                  "Table `mysqlaas_compat`.`path_tbl3` had unsupported engine "
                  "MyISAM changed to InnoDB"},
                 "CREATE TABLE IF NOT EXISTS `path_tbl3` (\n"
                 "  `pk` int(11) NOT NULL,\n  PRIMARY KEY (`pk`)\n"
                 ") ENGINE=InnoDB DEFAULT CHARSET=latin1 /* INDEX "
                 "DIRECTORY='" +
                     shcore::path::tmpdir() + "/'*/;");

    EXPECT_TABLE("part_tbl2",
                 {"Table `mysqlaas_compat`.`part_tbl2` had {DATA|INDEX} "
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
                 {"Table `mysqlaas_compat`.`part_tbl3` had {DATA|INDEX} "
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
    for (const auto i : items) {
      EXPECT_THAT(out, HasSubstr(shcore::str_replace(i, "`", "")));
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
  EXPECT_EQ("Trigger `mysqlaas_compat`.`ins_sum` had definer clause removed",
            iss.front().description);
  EXPECT_DUMP_CONTAINS(
      {"/*!50003 CREATE TRIGGER ins_sum BEFORE INSERT ON myisam_tbl1"});

  EXPECT_NO_THROW(iss = sd.dump_events_ddl(file.get(), compat_db_name));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ("Event `mysqlaas_compat`.`event2` had definer clause removed",
            iss.front().description);
  EXPECT_DUMP_CONTAINS({"/*!50106 CREATE EVENT IF NOT EXISTS `event2`"});

  EXPECT_NO_THROW(iss = sd.dump_routines_ddl(file.get(), compat_db_name));
  ASSERT_EQ(8, iss.size());
  EXPECT_EQ("Function `mysqlaas_compat`.`func1` had definer clause removed",
            iss[0].description);
  EXPECT_EQ(
      "Function `mysqlaas_compat`.`func1` had SQL SECURITY characteristic set "
      "to INVOKER",
      iss[1].description);
  EXPECT_EQ("Function `mysqlaas_compat`.`func2` had definer clause removed",
            iss[2].description);
  EXPECT_EQ("Procedure `mysqlaas_compat`.`labeled` had definer clause removed",
            iss[3].description);
  EXPECT_EQ(
      "Procedure `mysqlaas_compat`.`labeled` had SQL SECURITY characteristic "
      "set to INVOKER",
      iss[4].description);
  EXPECT_EQ("Procedure `mysqlaas_compat`.`proc1` had definer clause removed",
            iss[5].description);
  EXPECT_EQ(
      "Procedure `mysqlaas_compat`.`proc1` had SQL SECURITY characteristic set "
      "to INVOKER",
      iss[6].description);
  EXPECT_EQ("Procedure `mysqlaas_compat`.`proc2` had definer clause removed",
            iss[7].description);

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
  ASSERT_EQ(2, iss.size());
  EXPECT_EQ("View `mysqlaas_compat`.`view1` had definer clause removed",
            iss[0].description);
  EXPECT_EQ(
      "View `mysqlaas_compat`.`view1` had SQL SECURITY characteristic set to "
      "INVOKER",
      iss[1].description);
  EXPECT_DUMP_CONTAINS(
      {"/*!50001 CREATE ALGORITHM=UNDEFINED SQL SECURITY INVOKER VIEW view1 AS "
       "select 1 AS 1 */;"});

  EXPECT_NO_THROW(iss = sd.dump_view_ddl(file.get(), compat_db_name, "view3"));
  ASSERT_EQ(1, iss.size());
  EXPECT_EQ("View `mysqlaas_compat`.`view3` had definer clause removed",
            iss[0].description);
  EXPECT_DUMP_CONTAINS(
      {"/*!50001 CREATE ALGORITHM=UNDEFINED SQL SECURITY INVOKER VIEW view3 AS "
       "select 1 AS 1 */"});

  // Users
  Filtering_options filters;
  filters.users().exclude(std::array{"mysql", "root"});
  sd.use_filters(&filters);
  EXPECT_NO_THROW(iss = sd.dump_grants(file.get()));

  EXPECT_NE(iss.end(),
            std::find_if(iss.begin(), iss.end(), [](const auto &issue) {
              return issue.description ==
                     "User 'testusr6'@'localhost' had explicit grants on mysql "
                     "schema object `mysql`.`abradabra` removed";
            }));
}

TEST_F(Schema_dumper_test, dump_and_load) {
  for (const char *db : {compat_db_name, db_name, crazy_names_db}) {
    if (!file->is_open()) file->open(mysqlshdk::storage::Mode::WRITE);
    Schema_dumper sd(session);
    sd.opt_drop_trigger = true;
    sd.opt_mysqlaas = true;
    sd.opt_force_innodb = true;
    sd.opt_strip_restricted_grants = true;
    sd.opt_strip_tablespaces = true;
    sd.opt_strip_definer = true;
    EXPECT_NO_THROW(sd.write_header(file.get()));
    EXPECT_NO_THROW(sd.write_comment(file.get(), db));
    EXPECT_NO_THROW(sd.dump_schema_ddl(file.get(), db));
    session->execute(std::string("use ") + db);
    auto res = session->query("show tables", true);
    std::vector<std::string> tables;
    while (auto row = res->fetch_one()) tables.emplace_back(row->get_string(0));
    for (const auto &table : tables) {
      EXPECT_NO_THROW(sd.dump_table_ddl(file.get(), db, table));
      if (sd.count_triggers_for_table(db, table)) {
        EXPECT_NO_THROW(sd.dump_triggers_for_table_ddl(file.get(), db, table));
      }
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
    session = connect_session();
    session->execute(std::string("use ") + db);
    res = session->query("show tables", true);
    while (auto row = res->fetch_one())
      tables.erase(
          std::remove(tables.begin(), tables.end(), row->get_string(0)),
          tables.end());
    EXPECT_TRUE(tables.empty());
  }
}

TEST_F(Schema_dumper_test, get_users) {
  // setup
  session->execute(
      "CREATE USER IF NOT EXISTS 'first'@'localhost' IDENTIFIED BY 'pwd';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'first'@'10.11.12.13' IDENTIFIED BY 'pwd';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'firstfirst'@'localhost' IDENTIFIED BY "
      "'pwd';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'second'@'localhost' IDENTIFIED BY 'pwd';");
  session->execute(
      "CREATE USER IF NOT EXISTS 'second'@'10.11.12.14' IDENTIFIED BY 'pwd';");

  const auto contains = [](const std::vector<shcore::Account> &result,
                           const std::string &account) {
    const auto it =
        std::find_if(result.begin(), result.end(), [&account](const auto &e) {
          return shcore::make_account(e) == account;
        });

    std::set<std::string> accounts;

    for (const auto &e : result) {
      accounts.emplace(shcore::make_account(e));
    }

    if (result.end() != it) {
      return ::testing::AssertionSuccess()
             << "account found: " << account
             << ", contents: " << shcore::str_join(accounts, ", ");
    } else {
      return ::testing::AssertionFailure()
             << "account not found: " << account
             << ", contents: " << shcore::str_join(accounts, ", ");
    }
  };

  Schema_dumper sd(session);

  {
    // no filtering
    const auto result = sd.get_users();
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
    EXPECT_TRUE(contains(result, "'firstfirst'@'localhost'"));
    EXPECT_TRUE(contains(result, "'second'@'localhost'"));
    EXPECT_TRUE(contains(result, "'second'@'10.11.12.14'"));
  }

  {
    // include non-existent user
    Filtering_options filters;
    filters.users().include(std::array{"third"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(0, result.size());
  }

  {
    // exclude non-existent user
    Filtering_options filters;
    filters.users().exclude(std::array{"third"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
    EXPECT_TRUE(contains(result, "'firstfirst'@'localhost'"));
    EXPECT_TRUE(contains(result, "'second'@'localhost'"));
    EXPECT_TRUE(contains(result, "'second'@'10.11.12.14'"));
  }

  {
    // include all accounts for the user first
    Filtering_options filters;
    filters.users().include(std::array{"first"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(2, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
  }

  {
    // include only 'first'@'localhost'
    Filtering_options filters;
    filters.users().include(std::array{"'first'@'localhost'"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(1, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
  }

  {
    // include all accounts for the user first, exclude second
    Filtering_options filters;
    filters.users().include(std::array{"first"});
    filters.users().exclude(std::array{"second"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(2, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
  }

  {
    // include all accounts for the user first, exclude 'first'@'10.11.12.13'
    Filtering_options filters;
    filters.users().include(std::array{"first"});
    filters.users().exclude(std::array{"'first'@'10.11.12.13'"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(1, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
  }

  {
    // include all accounts for the user first, exclude first -> cancels out
    Filtering_options filters;
    filters.users().include(std::array{"first"});
    filters.users().exclude(std::array{"first"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(0, result.size());
  }

  {
    // include all accounts for the user first and second
    Filtering_options filters;
    filters.users().include(std::array{"first", "second"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(4, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
    EXPECT_TRUE(contains(result, "'second'@'localhost'"));
    EXPECT_TRUE(contains(result, "'second'@'10.11.12.14'"));
  }

  {
    // include all accounts for the user first and second, exclude second
    Filtering_options filters;
    filters.users().include(std::array{"first", "second"});
    filters.users().exclude(std::array{"second"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(2, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
  }

  {
    // include all accounts for the user first and second, exclude
    // 'second'@'10.11.12.14'
    Filtering_options filters;
    filters.users().include(std::array{"first", "second"});
    filters.users().exclude(std::array{"'second'@'10.11.12.14'"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(3, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
    EXPECT_TRUE(contains(result, "'second'@'localhost'"));
  }

  {
    // include all accounts for the user first, include and exclude
    // 'second'@'10.11.12.14'
    Filtering_options filters;
    filters.users().include(std::array{"first", "'second'@'10.11.12.14'"});
    filters.users().exclude(std::array{"'second'@'10.11.12.14'"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(2, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
  }

  {
    // include all accounts for the user first and 'second'@'10.11.12.14',
    // exclude all accounts for second
    Filtering_options filters;
    filters.users().include(std::array{"first", "'second'@'10.11.12.14'"});
    filters.users().exclude(std::array{"second"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();
    EXPECT_EQ(2, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
  }

  {
    // include all accounts for the user first and non-existent third, exclude
    // non-existent fourth
    Filtering_options filters;
    filters.users().include(std::array{"first", "third"});
    filters.users().exclude(std::array{"fourth"});
    sd.use_filters(&filters);
    const auto result = sd.get_users();

    EXPECT_EQ(2, result.size());
    EXPECT_TRUE(contains(result, "'first'@'localhost'"));
    EXPECT_TRUE(contains(result, "'first'@'10.11.12.13'"));
  }

  // cleanup
  session->execute("DROP USER 'first'@'localhost';");
  session->execute("DROP USER 'first'@'10.11.12.13';");
  session->execute("DROP USER 'firstfirst'@'localhost';");
  session->execute("DROP USER 'second'@'localhost';");
  session->execute("DROP USER 'second'@'10.11.12.14';");
}

TEST_F(Schema_dumper_test, check_object_for_definer) {
  // BUG#33087120
  Schema_dumper sd(session);

  sd.opt_strip_definer = true;

  std::vector<std::string> statements = {
      "CREATE ${definer} PROCEDURE ${schema}.${object}() ${security} BEGIN END",
      "CREATE ${definer} FUNCTION ${schema}.${object}() RETURNS integer "
      "DETERMINISTIC ${security} RETURN 1",
      "CREATE ${definer} ${security} VIEW ${schema}.${object}() AS SELECT "
      "COUNT(*) FROM s.t",
  };

  const std::string schema = "test_schema";
  const std::string object = "test_object";
  const std::string sql_security_invoker = "SQL SECURITY INVOKER";

  for (const auto &stmt : statements) {
    for (const auto &definer :
         {"DEFINER=root", "DEFINER = root@`host`", "DEFINER = 'root'@host",
          "DEFINER = \"root\"@\"host\"", ""}) {
      for (const auto &security :
           {sql_security_invoker.c_str(), "SQL SECURITY DEFINER", ""}) {
        SCOPED_TRACE("statement: " + stmt + ", definer: " + definer +
                     ", security: " + security);

        auto ddl = shcore::str_subvars(
            stmt,
            [&](std::string_view var) -> std::string {
              if ("schema" == var) return schema;
              if ("object" == var) return object;
              if ("definer" == var) return definer;
              if ("security" == var) return security;

              throw std::logic_error("Unknown variable: " + std::string{var});
            },
            "${", "}");
        std::vector<Schema_dumper::Issue> issues;

        sd.check_object_for_definer(schema, "OBJECT", object, &ddl, &issues);

        EXPECT_THAT(ddl, HasSubstr(sql_security_invoker));
        EXPECT_THAT(ddl, Not(HasSubstr("DEFINER")));
      }
    }
  }
}

TEST_F(Schema_dumper_test, check_object_for_definer_set_any_definer) {
  // WL#15887 - test if SET_ANY_DEFINER handling works with all combinations
  Schema_dumper sd(session);

  sd.opt_mysqlaas = true;
  sd.opt_target_version = mysqlshdk::utils::Version(8, 2, 0);

  Instance_cache cache;
  cache.users = {{"root", "host"}};

  sd.use_cache(&cache);

  std::vector<std::string> statements = {
      "CREATE ${definer} PROCEDURE ${schema}.${object}() ${security} BEGIN END",
      "CREATE ${definer} FUNCTION ${schema}.${object}() RETURNS integer "
      "DETERMINISTIC ${security} RETURN 1",
      "CREATE ${definer} ${security} VIEW ${schema}.${object}() AS SELECT "
      "COUNT(*) FROM s.t",
  };

  const std::string schema = "test_schema";
  const std::string object = "test_object";
  const std::string sql_security_invoker = "SQL SECURITY INVOKER";

  for (const auto &stmt : statements) {
    for (const auto &definer :
         {"DEFINER=root", "DEFINER = root@`host`", "DEFINER = 'root'@host",
          "DEFINER = \"root\"@\"host\"", ""}) {
      for (const auto &security :
           {sql_security_invoker.c_str(), "SQL SECURITY DEFINER", ""}) {
        SCOPED_TRACE("statement: " + stmt + ", definer: " + definer +
                     ", security: " + security);

        auto ddl = shcore::str_subvars(
            stmt,
            [&](std::string_view var) -> std::string {
              if ("schema" == var) return schema;
              if ("object" == var) return object;
              if ("definer" == var) return definer;
              if ("security" == var) return security;

              throw std::logic_error("Unknown variable: " + std::string{var});
            },
            "${", "}");
        std::vector<Schema_dumper::Issue> issues;

        EXPECT_NO_THROW(sd.check_object_for_definer(schema, "OBJECT", object,
                                                    &ddl, &issues));
      }
    }
  }
}

TEST_F(Schema_dumper_test, check_object_for_definer_set_any_definer_issues) {
  // WL#15887 - test SET_ANY_DEFINER issues
  using Status = Schema_dumper::Issue::Status;
  using Version = mysqlshdk::utils::Version;

  Schema_dumper sd(session);

  sd.opt_mysqlaas = true;
  sd.opt_target_version = Version(8, 2, 0);

  const std::string stmt =
      "CREATE ${definer} PROCEDURE ${schema}.${object}() ${security} BEGIN END";
  const std::string schema = "test_schema";
  const std::string object = "test_object";
  const std::string sql_security_invoker = "SQL SECURITY INVOKER";
  const std::string sql_security_definer = "SQL SECURITY DEFINER";
  const auto make_ddl = [&](const std::string &definer,
                            const std::string &security = {}) {
    return shcore::str_subvars(
        stmt,
        [&](std::string_view var) -> std::string {
          if ("schema" == var) return schema;
          if ("object" == var) return object;
          if ("definer" == var) return definer;
          if ("security" == var) return security;

          throw std::logic_error("Unknown variable: " + std::string{var});
        },
        "${", "}");
  };
  const auto issue = [&](const std::string &text) {
    return "Object `" + schema + "`.`" + object + "` - " + text;
  };
  const auto unknown_user = [&](const std::string &user) {
    return issue("definition uses DEFINER clause set to user " + user +
                 " which does not exist or is not included");
  };
  const auto restricted_user = [&](const std::string &user) {
    return issue("definition uses DEFINER clause set to user " + user +
                 " whose user name is restricted in MySQL HeatWave Service");
  };
  const auto definer_disallowed = [&](const std::string &user) {
    return issue("definition uses DEFINER clause set to user " + user +
                 " which can only be executed by this user or a user with "
                 "SET_ANY_DEFINER, SET_USER_ID or SUPER privileges");
  };
  const auto definer_or_invoker_required = [&]() {
    return issue(
        "definition does not have a DEFINER clause and does not use SQL "
        "SECURITY INVOKER characteristic, either one of these is required");
  };
  const auto security_definer_disallowed = [&]() {
    return issue(
        "definition does not use SQL SECURITY INVOKER characteristic, which is "
        "mandatory when the DEFINER clause is omitted or removed");
  };
  const auto EXPECT =
      [&](const std::string &definer,
          const std::vector<Schema_dumper::Issue> &expected_issues = {},
          const std::string &security = {}) {
        auto ddl = make_ddl(definer, security);

        SCOPED_TRACE("statement: " + ddl);

        std::vector<Schema_dumper::Issue> issues;

        sd.check_object_for_definer(schema, "OBJECT", object, &ddl, &issues);
        EXPECT_EQ(expected_issues, issues);
      };

  // missing cache
  EXPECT_THROW(EXPECT("DEFINER=root@host"), std::logic_error);

  // set cache
  Instance_cache cache;
  sd.use_cache(&cache);

  // no users in cache, warning that it's not possible to verify if user exits,
  // but only once
  EXPECT("DEFINER=root@host",
         {{"One or more DDL statements contain DEFINER clause but user "
           "information is not included in the dump. Loading will fail if "
           "accounts set as definers do not already exist in the target DB "
           "System instance.",
           Status::WARNING}});
  EXPECT("DEFINER=root@host");

  // add a user to cache
  cache.users = {{"user", "host"}};

  // warnings about unknown user, reported for each user
  EXPECT("DEFINER=root@host", {{unknown_user("root@host"), Status::WARNING}});
  EXPECT("DEFINER=user@localhost",
         {{unknown_user("user@localhost"), Status::WARNING}});

  // restricted user names
  for (const std::string user : {
           "mysql.infoschema",
           "mysql.session",
           "mysql.sys",
           "ociadmin",
           "ocidbm",
           "ocirpl",
       }) {
    const auto account = user + "@localhost";
    EXPECT("DEFINER=" + account,
           {{restricted_user(account), Status::FIX_MANUALLY},
            {unknown_user(account), Status::WARNING}});
  }

  // user known, no security clause - OK
  EXPECT("DEFINER=user@host");

  // user known, definer security - OK
  EXPECT("DEFINER=user@host", {}, sql_security_definer);

  // user known, invoker security - OK
  EXPECT("DEFINER=user@host", {}, sql_security_invoker);

  // display deprecated errors
  sd.opt_report_deprecated_errors_as_warnings = true;
  //  - user known, no security clause
  EXPECT(
      "DEFINER=user@host",
      {{definer_disallowed("user@host"), Status::WARNING_DEPRECATED_DEFINERS},
       {security_definer_disallowed(), Status::WARNING_DEPRECATED_DEFINERS}});
  //  - user known, definer security
  EXPECT(
      "DEFINER=user@host",
      {{definer_disallowed("user@host"), Status::WARNING_DEPRECATED_DEFINERS},
       {security_definer_disallowed(), Status::WARNING_DEPRECATED_DEFINERS}},
      sql_security_definer);
  //  - user known, invoker security
  EXPECT(
      "DEFINER=user@host",
      {{definer_disallowed("user@host"), Status::WARNING_DEPRECATED_DEFINERS}},
      sql_security_invoker);
  sd.opt_report_deprecated_errors_as_warnings = false;

  // version which does not support SET_ANY_DEFINER - regular error
  sd.opt_target_version = Version(8, 1, 0);
  //  - user known, no security clause
  EXPECT("DEFINER=user@host",
         {{definer_disallowed("user@host"), Status::USE_STRIP_DEFINERS},
          {security_definer_disallowed(), Status::USE_STRIP_DEFINERS}});
  //  - user known, definer security
  EXPECT("DEFINER=user@host",
         {{definer_disallowed("user@host"), Status::USE_STRIP_DEFINERS},
          {security_definer_disallowed(), Status::USE_STRIP_DEFINERS}},
         sql_security_definer);
  //  - user known, invoker security
  EXPECT("DEFINER=user@host",
         {{definer_disallowed("user@host"), Status::USE_STRIP_DEFINERS}},
         sql_security_invoker);
  sd.opt_target_version = Version(8, 2, 0);

  // version which does not support SET_ANY_DEFINER, deprecated errors are
  // enabled - still regular error
  sd.opt_target_version = Version(8, 1, 0);
  sd.opt_report_deprecated_errors_as_warnings = true;
  //  - user known, no security clause
  EXPECT("DEFINER=user@host",
         {{definer_disallowed("user@host"), Status::USE_STRIP_DEFINERS},
          {security_definer_disallowed(), Status::USE_STRIP_DEFINERS}});
  //  - user known, definer security
  EXPECT("DEFINER=user@host",
         {{definer_disallowed("user@host"), Status::USE_STRIP_DEFINERS},
          {security_definer_disallowed(), Status::USE_STRIP_DEFINERS}},
         sql_security_definer);
  //  - user known, invoker security
  EXPECT("DEFINER=user@host",
         {{definer_disallowed("user@host"), Status::USE_STRIP_DEFINERS}},
         sql_security_invoker);
  sd.opt_target_version = Version(8, 2, 0);
  sd.opt_report_deprecated_errors_as_warnings = false;

  // no definer & no security - error
  // WL15887-TSFR_3_4_1
  EXPECT("", {{definer_or_invoker_required(), Status::FIX_MANUALLY}});

  // no definer clause & definer security - error
  // WL15887-TSFR_3_4_1
  EXPECT("", {{definer_or_invoker_required(), Status::FIX_MANUALLY}},
         sql_security_definer);
  // no definer clause & invoker security - OK
  // WL15887-TSFR_3_4_1
  EXPECT("", {}, sql_security_invoker);
}

TEST_F(Schema_dumper_test, strip_restricted_grants_set_any_definer) {
  // WL#15887 - test SET_ANY_DEFINER grant
  using Status = Schema_dumper::Issue::Status;
  using Version = mysqlshdk::utils::Version;

  Schema_dumper sd(session);

  sd.opt_mysqlaas = true;

  const std::string user = "user@host";
  const std::string stmt = "GRANT ${privileges} ON *.* TO " + user;
  const std::string set_user_id = "SET_USER_ID";
  const std::string set_any_definer = "SET_ANY_DEFINER";
  const auto make_ddl = [&](const std::vector<std::string> &privileges) {
    return shcore::str_subvars(
        stmt,
        [&](std::string_view var) -> std::string {
          if ("privileges" == var) return shcore::str_join(privileges, ", ");
          throw std::logic_error("Unknown variable: " + std::string{var});
        },
        "${", "}");
  };
  const auto privilege_replaced = [&]() {
    return Schema_dumper::Issue{"User " + user + " had restricted privilege " +
                                    std::string(set_user_id) +
                                    " replaced with " +
                                    std::string(set_any_definer),
                                Status::FIXED};
  };
  const auto EXPECT =
      [&](const std::vector<std::string> &privileges,
          const std::vector<std::string> &expected_restricted,
          const std::vector<Schema_dumper::Issue> &expected_issues = {}) {
        auto ddl = make_ddl(privileges);

        SCOPED_TRACE("statement: " + ddl);

        std::vector<Schema_dumper::Issue> issues;

        const auto restricted = sd.strip_restricted_grants(user, &ddl, &issues);

        EXPECT_EQ(expected_restricted, restricted);
        EXPECT_EQ(expected_issues, issues);
      };

  // SET_ANY_DEFINER supported, privilege is not restricted
  sd.opt_target_version = Version(8, 2, 0);
  EXPECT({set_any_definer}, {});

  // SET_ANY_DEFINER not supported, privilege is restricted
  sd.opt_target_version = Version(8, 1, 0);
  EXPECT({set_any_definer}, {set_any_definer});

  // SET_ANY_DEFINER supported, SET_USER_ID is restricted
  sd.opt_target_version = Version(8, 2, 0);
  EXPECT({set_user_id}, {set_user_id});

  // SET_ANY_DEFINER not supported, SET_USER_ID is restricted
  sd.opt_target_version = Version(8, 1, 0);
  EXPECT({set_user_id}, {set_user_id});

  // SET_ANY_DEFINER supported, strip_restricted_grants enabled, SET_USER_ID is
  // replaced
  sd.opt_target_version = Version(8, 2, 0);
  sd.opt_strip_restricted_grants = true;
  EXPECT({set_user_id}, {}, {privilege_replaced()});
  sd.opt_strip_restricted_grants = false;

  // SET_ANY_DEFINER not supported, strip_restricted_grants enabled, SET_USER_ID
  // is removed
  sd.opt_target_version = Version(8, 1, 0);
  sd.opt_strip_restricted_grants = true;
  EXPECT({set_user_id}, {set_user_id});
  sd.opt_strip_restricted_grants = false;
}

}  // namespace dump
}  // namespace mysqlsh
