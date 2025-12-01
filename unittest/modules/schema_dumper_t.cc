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
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// needs to be included first for FRIEND_TEST
#include "unittest/gprod_clean.h"

#include "modules/util/dump/compatibility_issue.h"
#include "modules/util/dump/instance_cache.h"
#include "modules/util/dump/schema_dumper.h"

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/storage/backend/file.h"
#include "mysqlshdk/libs/storage/backend/memory_file.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_mysql_session.h"

extern "C" const char *g_test_home;

namespace mysqlsh {
namespace dump {

namespace {

#include "unittest/modules/mapped_collations.inc"

std::string quote(const std::string &db, const std::string &object) {
  return shcore::quote_identifier(db) + "." + shcore::quote_identifier(object);
}

void EXPECT_ISSUES(std::unordered_set<std::string> &&expected,
                   const std::vector<Compatibility_issue> &actual,
                   bool report_unexpected = true) {
  for (const auto &issue : actual) {
    if (const auto it = expected.find(issue.description);
        it != expected.end()) {
      expected.erase(it);
    } else if (report_unexpected) {
      EXPECT_TRUE(false) << "Unexpected issue: " << issue.description;
    }
  }

  for (const auto &issue : expected) {
    EXPECT_TRUE(false) << "Missing issue: " << issue;
  }
}

}  // namespace

using ::testing::AnyOf;
using ::testing::HasSubstr;
using ::testing::Not;

using mysqlshdk::db::Filtering_options;
using User_filters = Filtering_options::User_filters;

std::ostream &operator<<(std::ostream &os, const Compatibility_issue &issue) {
  return os << "c=" << to_string(issue.check)
            << ", s=" << to_string(issue.status)
            << ", t=" << to_string(issue.object_type)
            << ", n=" << issue.object_name << ", d=" << issue.description
            << ", co="
            << shcore::str_join(issue.compatibility_options.values(), ",",
                                [](const auto o) { return to_string(o); });
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
          {_mysql_uri, "--force", "-f",
           shcore::path::join_path(g_test_home, "data", "sql",
                                   "mysqldump_t.sql")});
      testutil->call_mysqlsh_c(
          {_mysql_uri, "--force", "-f",
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
      testutil->call_mysqlsh_c({_mysql_uri, "--force", "-f", out_path});
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

  Schema_dumper schema_dumper() {
    Filtering_options filters;
    filters.users().exclude(std::array{
        "mysql.infoschema",
        "mysql.session",
        "mysql.sys",
        "root",
    });

    m_cache = Instance_cache_builder(session, filters)
                  .metadata({})
                  .events()
                  .libraries()
                  .routines()
                  .triggers()
                  .users()
                  .build();

    return Schema_dumper{session, m_cache};
  }

  std::shared_ptr<mysqlshdk::db::ISession> session;
  Instance_cache m_cache;
  std::string file_path;
  std::unique_ptr<Schema_dumper::IFile> file;
  static bool run_directory_tests;
  static bool initialized;
  static constexpr auto db_name = "mysqldump_test_db";
  static constexpr auto compat_db_name = "mysqlaas_compat";
  static constexpr auto crazy_names_db = "crazy_names_db";
};

bool Schema_dumper_test::run_directory_tests = false;
bool Schema_dumper_test::initialized = false;

TEST_F(Schema_dumper_test, dump_table) {
  auto sd = schema_dumper();
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
  auto sd = schema_dumper();
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
  auto sd = schema_dumper();
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
  auto sd = schema_dumper();
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
  auto sd = schema_dumper();
  EXPECT_NO_THROW(sd.dump_events_ddl(file.get(), db_name));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;

  expect_output_contains({
      R"(
--
-- Dumping events for database 'mysqldump_test_db'
--
/*!50106 SET @save_time_zone= @@TIME_ZONE */ ;
)",
      R"(
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
DELIMITER ;
-- end event `mysqldump_test_db`.`ee1`
)",
      R"(
-- begin event `mysqldump_test_db`.`ee2`
/*!50106 DROP EVENT IF EXISTS `ee2` */;
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
DELIMITER ;
-- end event `mysqldump_test_db`.`ee2`
)",
      R"(
/*!50106 SET TIME_ZONE= @save_time_zone */ ;
)",
  });
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_routines) {
  auto sd = schema_dumper();
  EXPECT_NO_THROW(sd.dump_routines_ddl(file.get(), db_name));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 20)) return;

  expect_output_contains({
      R"(
--
-- Dumping routines for database 'mysqldump_test_db'
--
)",
      R"(
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
)",
      R"(
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
)",
      R"(
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
)",
      R"(
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
)",
      R"(
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
)",
  });
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_libraries) {
  auto sd = schema_dumper();
  EXPECT_NO_THROW(sd.dump_libraries_ddl(file.get(), db_name));
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();

  if (!compatibility::supports_library_ddl(_target_server_version)) {
    return;
  }

  expect_output_contains({
      R"(
--
-- Dumping libraries for database 'mysqldump_test_db'
--
)",
      R"(
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
)",
      R"(
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
)",
  });
  wipe_all();
}

TEST_F(Schema_dumper_test, dump_tablespaces) {
  auto sd = schema_dumper();
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

  auto sd = schema_dumper();
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

  auto sd = schema_dumper();
  sd.opt_mysqlaas = true;
  sd.opt_strip_restricted_grants = true;

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

  auto sd = schema_dumper();
  sd.opt_mysqlaas = true;
  sd.opt_strip_restricted_grants = true;

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
  session->execute(std::string("use ") + compat_db_name);
  const auto mstg =
      create_table_in_mysql_schema_for_grant("testusr6@localhost");

  auto sd = schema_dumper();
  sd.opt_mysqlaas = true;

  const auto EXPECT_TABLE = [&](const std::string &table,
                                std::vector<std::string> msg) {
    SCOPED_TRACE(table);
    ASSERT_TRUE(file->is_open());
    std::vector<Compatibility_issue> iss;
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
      {"Table `mysqlaas_compat`.`blackhole_tbl1` uses unsupported storage "
       "engine BLACKHOLE",
       "Table `mysqlaas_compat`.`blackhole_tbl1` does not have a Primary Key, "
       "which is required for High Availability in MySQL HeatWave Service"});

  EXPECT_TABLE("myisam_tbl2",
               {"Table `mysqlaas_compat`.`myisam_tbl2` uses unsupported "
                "storage engine MyISAM"});

  // TABLESPACE
  EXPECT_TABLE("ts1_tbl1", {"Table `mysqlaas_compat`.`ts1_tbl1` uses "
                            "unsupported tablespace option"});

  {
    SCOPED_TRACE("DEFINER - triggers");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss = sd.dump_triggers_for_table_ddl(
                        file.get(), compat_db_name, "myisam_tbl1"));

    EXPECT_ISSUES(
        {
            "Trigger `mysqlaas_compat`.`myisam_tbl1`.`ins_sum` - definition "
            "uses DEFINER clause set to user `root`@`localhost` which can only "
            "be executed by this user or a user with SET_ANY_DEFINER, "
            "SET_USER_ID or SUPER privileges",
        },
        iss);
  }

  {
    SCOPED_TRACE("DEFINER - events");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss = sd.dump_events_ddl(file.get(), compat_db_name));

    EXPECT_ISSUES(
        {
            "Event `mysqlaas_compat`.`event2` - definition uses DEFINER clause "
            "set to user `root`@`localhost` which can only be executed by this "
            "user or a user with SET_ANY_DEFINER, SET_USER_ID or SUPER "
            "privileges",
        },
        iss);
  }

  const auto user = "`" + _user + "`@`" + _host + "`";

  {
    SCOPED_TRACE("DEFINER - routines");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss = sd.dump_routines_ddl(file.get(), compat_db_name));

    EXPECT_ISSUES(
        {
            "Function `mysqlaas_compat`.`func1` - definition uses DEFINER "
            "clause set to user " +
                user +
                " which can only be executed by this user or a user with "
                "SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
            "Function `mysqlaas_compat`.`func1` - definition does not use SQL "
            "SECURITY INVOKER characteristic, which is mandatory when the "
            "DEFINER clause is omitted or removed",
            "Function `mysqlaas_compat`.`func2` - definition uses DEFINER "
            "clause set to user " +
                user +
                " which can only be executed by this user or a user with "
                "SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
            "Procedure `mysqlaas_compat`.`labeled` - definition uses DEFINER "
            "clause set to user " +
                user +
                " which can only be executed by this user or a user with "
                "SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
            "Procedure `mysqlaas_compat`.`labeled` - definition does not use "
            "SQL SECURITY INVOKER characteristic, which is mandatory when the "
            "DEFINER clause is omitted or removed",
            "Procedure `mysqlaas_compat`.`proc1` - definition uses DEFINER "
            "clause set to user " +
                user +
                " which can only be executed by this user or a user with "
                "SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
            "Procedure `mysqlaas_compat`.`proc1` - definition does not use SQL "
            "SECURITY INVOKER characteristic, which is mandatory when the "
            "DEFINER clause is omitted or removed",
            "Procedure `mysqlaas_compat`.`proc2` - definition uses DEFINER "
            "clause set to user " +
                user +
                " which can only be executed by this user or a user with "
                "SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
        },
        iss);
  }

  {
    SCOPED_TRACE("DEFINER - view1");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss =
                        sd.dump_view_ddl(file.get(), compat_db_name, "view1"));

    EXPECT_ISSUES(
        {
            "View `mysqlaas_compat`.`view1` - definition uses DEFINER clause "
            "set to user " +
                user +
                " which can only be executed by this user or a user with "
                "SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
            "View `mysqlaas_compat`.`view1` - definition does not use SQL "
            "SECURITY INVOKER characteristic, which is mandatory when the "
            "DEFINER clause is omitted or removed",
        },
        iss);
  }

  {
    SCOPED_TRACE("DEFINER - view3");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss =
                        sd.dump_view_ddl(file.get(), compat_db_name, "view3"));

    EXPECT_ISSUES(
        {
            "View `mysqlaas_compat`.`view3` - definition uses DEFINER clause "
            "set to user " +
                user +
                " which can only be executed by this user or a user with "
                "SET_ANY_DEFINER, SET_USER_ID or SUPER privileges",
        },
        iss);
  }

  {
    SCOPED_TRACE("users");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss = sd.dump_grants(file.get()));

    EXPECT_ISSUES(
        run_directory_tests
            ? std::unordered_set<std::string>{
              "User 'testusr1'@'localhost' is granted restricted privileges: FILE, RELOAD, SUPER",
              "User 'testusr2'@'localhost' is granted restricted privilege: SUPER",
              "User 'testusr3'@'localhost' is granted restricted privileges: FILE, RELOAD",
              "User 'testusr4'@'localhost' is granted restricted privilege: SUPER",
              "User 'testusr5'@'localhost' is granted restricted privilege: FILE",
              "User 'testusr6'@'localhost' is granted restricted privilege: FILE",
              "User 'testusr6'@'localhost' has explicit grants on mysql schema object: `mysql`.`abradabra`",
              "User 'testusr6'@'localhost' has a wildcard grant statement at the database level (GRANT SELECT, INSERT, UPDATE, DELETE ON `mysqlaas_compat`.* TO 'testusr6'@'localhost')",
            }
            : std::unordered_set<std::string>{
              "User 'testusr1'@'localhost' is granted restricted privileges: BINLOG_ADMIN, FILE, RELOAD",
              "User 'testusr3'@'localhost' is granted restricted privileges: BINLOG_ADMIN, FILE, RELOAD",
              "User 'testusr5'@'localhost' is granted restricted privilege: FILE",
              "User 'testusr6'@'localhost' is granted restricted privilege: FILE",
              "User 'testusr6'@'localhost' has explicit grants on mysql schema object: `mysql`.`abradabra`",
              "User 'testusr6'@'localhost' has a wildcard grant statement at the database level (GRANT SELECT, INSERT, UPDATE, DELETE ON `mysqlaas_compat`.* TO `testusr6`@`localhost`)",
            }, iss, false); // don't report unexpected issues here, there could be some other users
  }
}

TEST_F(Schema_dumper_test, compat_ddl) {
  session->execute(std::string("use ") + compat_db_name);
  const auto mstg =
      create_table_in_mysql_schema_for_grant("testusr6@localhost");

  auto sd = schema_dumper();
  sd.opt_mysqlaas = true;
  sd.opt_force_innodb = true;
  sd.opt_strip_restricted_grants = true;
  sd.opt_strip_tablespaces = true;
  sd.opt_strip_definer = true;

  const auto EXPECT_TABLE = [&](const std::string &table,
                                std::vector<std::string> msg,
                                const std::string &ddl = "") {
    SCOPED_TRACE(table);
    file.reset(new mysqlshdk::storage::backend::File(file_path));
    file->open(mysqlshdk::storage::Mode::WRITE);
    ASSERT_TRUE(file->is_open());
    std::vector<Compatibility_issue> iss;
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

  {
    SCOPED_TRACE("DEFINER - triggers");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss = sd.dump_triggers_for_table_ddl(
                        file.get(), compat_db_name, "myisam_tbl1"));

    EXPECT_ISSUES(
        {
            "Trigger `mysqlaas_compat`.`myisam_tbl1`.`ins_sum` had definer "
            "clause removed",
        },
        iss);

    EXPECT_DUMP_CONTAINS({
        "/*!50003 CREATE TRIGGER ins_sum BEFORE INSERT ON myisam_tbl1",
    });
  }

  {
    SCOPED_TRACE("DEFINER - events");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss = sd.dump_events_ddl(file.get(), compat_db_name));

    EXPECT_ISSUES(
        {
            "Event `mysqlaas_compat`.`event2` had definer clause removed",
        },
        iss);

    EXPECT_DUMP_CONTAINS({
        "/*!50106 CREATE EVENT IF NOT EXISTS `event2`",
    });
  }

  {
    SCOPED_TRACE("DEFINER - routines");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss = sd.dump_routines_ddl(file.get(), compat_db_name));

    EXPECT_ISSUES(
        {
            "Function `mysqlaas_compat`.`func1` had definer clause removed",
            "Function `mysqlaas_compat`.`func1` had SQL SECURITY "
            "characteristic set to INVOKER",
            "Function `mysqlaas_compat`.`func2` had definer clause removed",
            "Procedure `mysqlaas_compat`.`labeled` had definer clause removed",
            "Procedure `mysqlaas_compat`.`labeled` had SQL SECURITY "
            "characteristic set to INVOKER",
            "Procedure `mysqlaas_compat`.`proc1` had definer clause removed",
            "Procedure `mysqlaas_compat`.`proc1` had SQL SECURITY "
            "characteristic set to INVOKER",
            "Procedure `mysqlaas_compat`.`proc2` had definer clause removed",
        },
        iss);

    EXPECT_DUMP_CONTAINS({
        R"(
CREATE FUNCTION `func1`() RETURNS int
    NO SQL
SQL SECURITY INVOKER
RETURN 0 ;;
)",
        R"(
CREATE FUNCTION `func2`() RETURNS int
    NO SQL
    SQL SECURITY INVOKER
RETURN 0 ;;
)",
        R"(
CREATE PROCEDURE labeled()
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

END ;;
)",
        R"(
CREATE PROCEDURE `proc1`()
    NO SQL
SQL SECURITY INVOKER
BEGIN
END ;;
)",
        R"(
CREATE PROCEDURE `proc2`()
    NO SQL
    SQL SECURITY INVOKER
BEGIN
END ;;
)",
    });
  }

  {
    SCOPED_TRACE("DEFINER - view1");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss =
                        sd.dump_view_ddl(file.get(), compat_db_name, "view1"));

    EXPECT_ISSUES(
        {
            "View `mysqlaas_compat`.`view1` had definer clause removed",
            "View `mysqlaas_compat`.`view1` had SQL SECURITY characteristic "
            "set to INVOKER",
        },
        iss);

    EXPECT_DUMP_CONTAINS({
        "/*!50001 CREATE ALGORITHM=UNDEFINED SQL SECURITY INVOKER VIEW view1 "
        "AS select 1 AS 1 */;",
    });
  }

  {
    SCOPED_TRACE("DEFINER - view3");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss =
                        sd.dump_view_ddl(file.get(), compat_db_name, "view3"));

    EXPECT_ISSUES(
        {
            "View `mysqlaas_compat`.`view3` had definer clause removed",
        },
        iss);

    EXPECT_DUMP_CONTAINS({
        "/*!50001 CREATE ALGORITHM=UNDEFINED SQL SECURITY INVOKER VIEW view3 "
        "AS select 1 AS 1 */",
    });
  }

  {
    SCOPED_TRACE("users");

    std::vector<Compatibility_issue> iss;
    EXPECT_NO_THROW(iss = sd.dump_grants(file.get()));

    EXPECT_ISSUES(
        {
            "User 'testusr6'@'localhost' had explicit grants on mysql schema "
            "object `mysql`.`abradabra` removed",
        },
        iss, false);
  }
}

TEST_F(Schema_dumper_test, dump_and_load) {
  static constexpr auto k_target_schema = "verification";
  using mysqlshdk::storage::fprintf;

  for (const char *db : {compat_db_name, db_name, crazy_names_db}) {
    if (!file->is_open()) file->open(mysqlshdk::storage::Mode::WRITE);

    fprintf(file.get(), "DROP SCHEMA IF EXISTS `%s`;\n", k_target_schema);
    fprintf(file.get(), "CREATE SCHEMA `%s`;\n", k_target_schema);
    fprintf(file.get(), "USE `%s`;\n", k_target_schema);

    auto sd = schema_dumper();
    sd.opt_drop_trigger = true;
    sd.opt_mysqlaas = true;
    sd.opt_force_innodb = true;
    sd.opt_strip_restricted_grants = true;
    sd.opt_strip_tablespaces = true;
    sd.opt_strip_definer = true;

    EXPECT_NO_THROW(sd.write_header(file.get()));
    EXPECT_NO_THROW(sd.write_comment(file.get(), db));
    EXPECT_NO_THROW(sd.dump_schema_ddl(file.get(), db));

    session->executef("USE !", db);

    std::vector<std::string> tables;

    if (const auto res = session->query("show tables")) {
      while (const auto row = res->fetch_one()) {
        tables.emplace_back(row->get_string(0));
      }
    }

    std::vector<std::string> views;

    if (const auto res =
            session->queryf("select TABLE_NAME FROM information_schema.views "
                            "where TABLE_SCHEMA = ?",
                            db)) {
      while (const auto row = res->fetch_one()) {
        views.emplace_back(row->get_string(0));
      }
    }

    for (const auto &table : tables) {
      SCOPED_TRACE(std::string{"`"} + db + "`.`" + table + "`");

      EXPECT_NO_THROW(sd.dump_table_ddl(file.get(), db, table));

      if (views.end() != std::find(views.begin(), views.end(), table)) {
        continue;
      }

      bool has_triggers = false;
      EXPECT_NO_THROW(has_triggers = sd.count_triggers_for_table(db, table));

      if (has_triggers) {
        EXPECT_NO_THROW(sd.dump_triggers_for_table_ddl(file.get(), db, table));
      }
    }

    EXPECT_NO_THROW(sd.dump_events_ddl(file.get(), db));
    EXPECT_NO_THROW(sd.dump_routines_ddl(file.get(), db));

    for (const auto &view : views) {
      SCOPED_TRACE(std::string{"`"} + db + "`.`" + view + "`");
      EXPECT_NO_THROW(sd.dump_view_ddl(file.get(), db, view));
    }

    EXPECT_NO_THROW(sd.write_footer(file.get()));
    EXPECT_TRUE(output_handler.std_err.empty());

    file->flush();
    file->close();
    session->close();

    wipe_all();
    testutil->call_mysqlsh_c({_mysql_uri, "--sql", "-f", file_path});
    EXPECT_TRUE(output_handler.std_err.empty());

    session = connect_session();
    session->executef("USE !", k_target_schema);

    if (const auto res = session->query("show tables")) {
      while (const auto row = res->fetch_one()) {
        std::erase(tables, row->get_string(0));
      }
    }

    EXPECT_TRUE(tables.empty());
  }

  session->executef("DROP SCHEMA IF EXISTS !", k_target_schema);
}

TEST_F(Schema_dumper_test, check_object_for_definer) {
  // BUG#33087120
  auto sd = schema_dumper();

  sd.opt_strip_definer = true;

  std::vector<std::pair<std::string, Compatibility_issue::Object_type>>
      statements = {
          {"CREATE ${definer} PROCEDURE ${schema}.${object}() ${security} "
           "BEGIN END",
           Compatibility_issue::Object_type::PROCEDURE},
          {"CREATE ${definer} FUNCTION ${schema}.${object}() RETURNS integer "
           "DETERMINISTIC ${security} RETURN 1",
           Compatibility_issue::Object_type::FUNCTION},
          {"CREATE ${definer} ${security} VIEW ${schema}.${object}() AS SELECT "
           "COUNT(*) FROM s.t",
           Compatibility_issue::Object_type::VIEW},
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
        SCOPED_TRACE("statement: " + stmt.first + ", definer: " + definer +
                     ", security: " + security);

        auto ddl = shcore::str_subvars(
            stmt.first,
            [&](std::string_view var) -> std::string {
              if ("schema" == var) return schema;
              if ("object" == var) return object;
              if ("definer" == var) return definer;
              if ("security" == var) return security;

              throw std::logic_error("Unknown variable: " + std::string{var});
            },
            "${", "}");
        std::vector<Compatibility_issue> issues;

        sd.check_object_for_definer(stmt.second, quote(schema, object), &ddl,
                                    &issues);

        EXPECT_THAT(ddl, HasSubstr(sql_security_invoker));
        EXPECT_THAT(ddl, Not(HasSubstr("DEFINER")));
      }
    }
  }
}

TEST_F(Schema_dumper_test, check_object_for_definer_set_any_definer) {
  // WL#15887 - test if SET_ANY_DEFINER handling works with all combinations

  Instance_cache cache;
  cache.users = {{"root", "host"}};

  auto sd = Schema_dumper{session, cache};
  sd.opt_mysqlaas = true;
  sd.opt_target_version = mysqlshdk::utils::Version(8, 2, 0);

  std::vector<std::pair<std::string, Compatibility_issue::Object_type>>
      statements = {
          {"CREATE ${definer} PROCEDURE ${schema}.${object}() ${security} "
           "BEGIN END",
           Compatibility_issue::Object_type::PROCEDURE},
          {"CREATE ${definer} FUNCTION ${schema}.${object}() RETURNS integer "
           "DETERMINISTIC ${security} RETURN 1",
           Compatibility_issue::Object_type::FUNCTION},
          {"CREATE ${definer} ${security} VIEW ${schema}.${object}() AS SELECT "
           "COUNT(*) FROM s.t",
           Compatibility_issue::Object_type::VIEW},
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
        SCOPED_TRACE("statement: " + stmt.first + ", definer: " + definer +
                     ", security: " + security);

        auto ddl = shcore::str_subvars(
            stmt.first,
            [&](std::string_view var) -> std::string {
              if ("schema" == var) return schema;
              if ("object" == var) return object;
              if ("definer" == var) return definer;
              if ("security" == var) return security;

              throw std::logic_error("Unknown variable: " + std::string{var});
            },
            "${", "}");
        std::vector<Compatibility_issue> issues;

        EXPECT_NO_THROW(sd.check_object_for_definer(
            stmt.second, quote(schema, object), &ddl, &issues));
      }
    }
  }
}

TEST_F(Schema_dumper_test, check_object_for_definer_set_any_definer_issues) {
  // WL#15887 - test SET_ANY_DEFINER issues
  using Status = Compatibility_issue::Status;
  using Object_type = Compatibility_issue::Object_type;
  using Version = mysqlshdk::utils::Version;

  Instance_cache cache;
  auto sd = Schema_dumper{session, cache};

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
  const auto description = [&](const std::string &text) {
    return "Procedure `" + schema + "`.`" + object + "` - " + text;
  };
  const auto unknown_user = [&](const std::string &user) {
    return description("definition uses DEFINER clause set to user " + user +
                       " which does not exist or is not included");
  };
  const auto restricted_user = [&](const std::string &user) {
    return description(
        "definition uses DEFINER clause set to user " + user +
        " whose user name is restricted in MySQL HeatWave Service");
  };
  const auto definer_disallowed = [&](const std::string &user) {
    return description(
        "definition uses DEFINER clause set to user " + user +
        " which can only be executed by this user or a user with "
        "SET_ANY_DEFINER, SET_USER_ID or SUPER privileges");
  };
  const auto definer_or_invoker_required = [&]() {
    return description(
        "definition does not have a DEFINER clause and does not use SQL "
        "SECURITY INVOKER characteristic, either one of these is required");
  };
  const auto security_definer_disallowed = [&]() {
    return description(
        "definition does not use SQL SECURITY INVOKER characteristic, which is "
        "mandatory when the DEFINER clause is omitted or removed");
  };
  const auto no_users = []() {
    return "One or more DDL statements contain DEFINER clause but user "
           "information is not included in the dump. Loading will fail if "
           "accounts set as definers do not already exist in the target DB "
           "System instance.";
  };
  const auto issue = [](Compatibility_check c, Status s, const std::string &d,
                        auto... options) {
    Compatibility_issue i{c, s, Object_type::PROCEDURE,
                          "`test_schema`.`test_object`"};
    i.description = d;
    (i.compatibility_options.set(options), ...);
    return i;
  };
  const auto issue_no_users = [&]() {
    auto i = issue(Compatibility_check::OBJECT_INVALID_DEFINER_USERS_NOT_DUMPED,
                   Status::WARNING, no_users());
    i.object_type = Object_type::UNSPECIFIED;
    i.object_name.clear();
    return i;
  };
  const auto EXPECT =
      [&](const std::string &definer,
          const std::vector<Compatibility_issue> &expected_issues = {},
          const std::string &security = {}) {
        auto ddl = make_ddl(definer, security);

        SCOPED_TRACE("statement: " + ddl);

        std::vector<Compatibility_issue> issues;

        sd.check_object_for_definer(Object_type::PROCEDURE,
                                    quote(schema, object), &ddl, &issues);
        EXPECT_EQ(expected_issues, issues);
      };

  // no users in cache, warning that it's not possible to verify if user exits,
  // but only once
  EXPECT("DEFINER=root@host", {issue_no_users()});
  EXPECT("DEFINER=root@host");

  // add a user to cache
  cache.users = {{"user", "host"}};

  // warnings about unknown user, reported for each user
  EXPECT("DEFINER=root@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER_MISSING_USER,
                Status::WARNING, unknown_user("root@host"))});
  EXPECT("DEFINER=user@localhost",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER_MISSING_USER,
                Status::WARNING, unknown_user("user@localhost"))});

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
           {issue(Compatibility_check::OBJECT_RESTRICTED_DEFINER, Status::ERROR,
                  restricted_user(account)),
            issue(Compatibility_check::OBJECT_INVALID_DEFINER_MISSING_USER,
                  Status::WARNING, unknown_user(account))});
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
  EXPECT("DEFINER=user@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER, Status::WARNING,
                definer_disallowed("user@host"),
                Compatibility_option::STRIP_DEFINERS),
          issue(Compatibility_check::OBJECT_MISSING_SQL_SECURITY,
                Status::WARNING, security_definer_disallowed(),
                Compatibility_option::STRIP_DEFINERS)});

  //  - user known, definer security
  EXPECT("DEFINER=user@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER, Status::WARNING,
                definer_disallowed("user@host"),
                Compatibility_option::STRIP_DEFINERS),
          issue(Compatibility_check::OBJECT_MISSING_SQL_SECURITY,
                Status::WARNING, security_definer_disallowed(),
                Compatibility_option::STRIP_DEFINERS)},
         sql_security_definer);

  //  - user known, invoker security
  EXPECT("DEFINER=user@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER, Status::WARNING,
                definer_disallowed("user@host"),
                Compatibility_option::STRIP_DEFINERS)},
         sql_security_invoker);
  sd.opt_report_deprecated_errors_as_warnings = false;

  // version which does not support SET_ANY_DEFINER - regular error
  sd.opt_target_version = Version(8, 1, 0);
  //  - user known, no security clause
  EXPECT("DEFINER=user@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER, Status::ERROR,
                definer_disallowed("user@host"),
                Compatibility_option::STRIP_DEFINERS),
          issue(Compatibility_check::OBJECT_MISSING_SQL_SECURITY, Status::ERROR,
                security_definer_disallowed(),
                Compatibility_option::STRIP_DEFINERS)});
  //  - user known, definer security
  EXPECT("DEFINER=user@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER, Status::ERROR,
                definer_disallowed("user@host"),
                Compatibility_option::STRIP_DEFINERS),
          issue(Compatibility_check::OBJECT_MISSING_SQL_SECURITY, Status::ERROR,
                security_definer_disallowed(),
                Compatibility_option::STRIP_DEFINERS)},
         sql_security_definer);
  //  - user known, invoker security
  EXPECT("DEFINER=user@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER, Status::ERROR,
                definer_disallowed("user@host"),
                Compatibility_option::STRIP_DEFINERS)},
         sql_security_invoker);
  sd.opt_target_version = Version(8, 2, 0);

  // version which does not support SET_ANY_DEFINER, deprecated errors are
  // enabled - still regular error
  sd.opt_target_version = Version(8, 1, 0);
  sd.opt_report_deprecated_errors_as_warnings = true;
  //  - user known, no security clause
  EXPECT("DEFINER=user@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER, Status::ERROR,
                definer_disallowed("user@host"),
                Compatibility_option::STRIP_DEFINERS),
          issue(Compatibility_check::OBJECT_MISSING_SQL_SECURITY, Status::ERROR,
                security_definer_disallowed(),
                Compatibility_option::STRIP_DEFINERS)});
  //  - user known, definer security
  EXPECT("DEFINER=user@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER, Status::ERROR,
                definer_disallowed("user@host"),
                Compatibility_option::STRIP_DEFINERS),
          issue(Compatibility_check::OBJECT_MISSING_SQL_SECURITY, Status::ERROR,
                security_definer_disallowed(),
                Compatibility_option::STRIP_DEFINERS)},
         sql_security_definer);
  //  - user known, invoker security
  EXPECT("DEFINER=user@host",
         {issue(Compatibility_check::OBJECT_INVALID_DEFINER, Status::ERROR,
                definer_disallowed("user@host"),
                Compatibility_option::STRIP_DEFINERS)},
         sql_security_invoker);
  sd.opt_target_version = Version(8, 2, 0);
  sd.opt_report_deprecated_errors_as_warnings = false;

  // no definer & no security - error
  // WL15887-TSFR_3_4_1
  EXPECT("",
         {issue(Compatibility_check::OBJECT_MISSING_SQL_SECURITY_AND_DEFINER,
                Status::ERROR, definer_or_invoker_required())});

  // no definer clause & definer security - error
  // WL15887-TSFR_3_4_1
  EXPECT("",
         {issue(Compatibility_check::OBJECT_MISSING_SQL_SECURITY_AND_DEFINER,
                Status::ERROR, definer_or_invoker_required())},
         sql_security_definer);
  // no definer clause & invoker security - OK
  // WL15887-TSFR_3_4_1
  EXPECT("", {}, sql_security_invoker);
}

TEST_F(Schema_dumper_test, strip_restricted_grants_set_any_definer) {
  // WL#15887 - test SET_ANY_DEFINER grant
  using Status = Compatibility_issue::Status;
  using Object_type = Compatibility_issue::Object_type;
  using Version = mysqlshdk::utils::Version;

  auto sd = schema_dumper();

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
    Compatibility_issue i{Compatibility_check::USER_RESTRICTED_GRANTS,
                          Status::FIXED, Object_type::USER, "user@host"};
    i.description = "User " + user + " had restricted privilege " +
                    std::string(set_user_id) + " replaced with " +
                    std::string(set_any_definer);
    i.compatibility_options |= Compatibility_option::STRIP_RESTRICTED_GRANTS;
    return i;
  };
  const auto EXPECT =
      [&](const std::vector<std::string> &privileges,
          const std::vector<std::string> &expected_restricted,
          const std::vector<Compatibility_issue> &expected_issues = {}) {
        auto ddl = make_ddl(privileges);

        SCOPED_TRACE("statement: " + ddl);

        std::vector<Compatibility_issue> issues;

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

TEST_F(Schema_dumper_test, unknown_collations) {
  const auto dump_schema = [](std::string_view collation) {
    const auto s = std::make_shared<testing::Mock_mysql_session>();

    s->expect_query("SHOW CREATE DATABASE IF NOT EXISTS `s`")
        .then({"Database", "Create Database"})
        .add_row(
            {"s", shcore::str_format("CREATE DATABASE `s` /*!40100 DEFAULT "
                                     "CHARACTER SET utf8mb4 COLLATE %.*s */",
                                     static_cast<int>(collation.size()),
                                     collation.data())});

    const auto f =
        std::make_unique<mysqlshdk::storage::backend::Memory_file>("f");

    f->open(mysqlshdk::storage::Mode::WRITE);
    Instance_cache cache;
    cache.schemas.emplace("s", Instance_cache::Schema{})
        .first->second.default_collation = collation;
    auto issues = Schema_dumper{s, cache}.dump_schema_ddl(f.get(), "s");
    f->close();

    return std::make_pair(f->content(), std::move(issues));
  };

  for (const auto &map : k_collation_map) {
    SCOPED_TRACE(map.first);

    const auto result = dump_schema(map.first);

    EXPECT_EQ(shcore::str_format(R"(
--
-- Dumping database 's'
--

-- begin database `s`
CREATE DATABASE `s` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE %.*s */;
-- end database `s`

)",
                                 static_cast<int>(map.second.size()),
                                 map.second.data()),
              result.first);
    ASSERT_EQ(1, result.second.size());
    EXPECT_EQ(shcore::str_format(
                  "Schema `s` had default collation set to '%.*s', it has been "
                  "replaced with '%.*s'",
                  static_cast<int>(map.first.size()), map.first.data(),
                  static_cast<int>(map.second.size()), map.second.data()),
              result.second[0].description);
    EXPECT_EQ(Compatibility_issue::Status::WARNING, result.second[0].status);
    EXPECT_EQ(Compatibility_check::OBJECT_COLLATION_REPLACED,
              result.second[0].check);
  }

  for (const auto &unsupported : k_unsupported_collations) {
    SCOPED_TRACE(unsupported);

    const auto result = dump_schema(unsupported);

    EXPECT_EQ(shcore::str_format(R"(
--
-- Dumping database 's'
--

-- begin database `s`
CREATE DATABASE `s` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE %.*s */;
-- end database `s`

)",
                                 static_cast<int>(unsupported.size()),
                                 unsupported.data()),
              result.first);
    ASSERT_EQ(1, result.second.size());
    EXPECT_EQ(shcore::str_format("Schema `s` has default collation set to "
                                 "unsupported collation '%.*s'",
                                 static_cast<int>(unsupported.size()),
                                 unsupported.data()),
              result.second[0].description);
    EXPECT_EQ(Compatibility_issue::Status::ERROR, result.second[0].status);
    EXPECT_EQ(Compatibility_check::OBJECT_COLLATION_UNSUPPORTED,
              result.second[0].check);
  }
}

}  // namespace dump
}  // namespace mysqlsh
