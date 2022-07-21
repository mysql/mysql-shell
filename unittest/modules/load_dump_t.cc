/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#include "modules/util/dump/compatibility.h"
#include "modules/util/dump/dump_utils.h"
#include "modules/util/dump/schema_dumper.h"
#include "modules/util/load/dump_loader.h"
#include "modules/util/load/dump_reader.h"
#include "modules/util/load/load_dump_options.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/storage/backend/memory_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"

#include "unittest/gtest_clean.h"
#include "unittest/modules/dummy_dumpdir.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_mysql_session.h"

extern "C" const char *g_test_home;

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Exactly;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrEq;

using mysqlshdk::utils::Version;

namespace mysqlsh {

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

TEST(Load_dump, add_execute_conditionally) {
  const auto call = [](const Dump_loader::Sql_transform &tx, const char *s) {
    std::string tmp;
    EXPECT_TRUE(tx(s, strlen(s), &tmp));
    return tmp;
  };

  {
    Dump_loader::Sql_transform tx;
    tx.add_execute_conditionally(
        [](const std::string &, const std::string &) { return true; });

    EXPECT_EQ("CREATE EVENT foo", call(tx, "CREATE EVENT foo"));
  }

  {
    Dump_loader::Sql_transform tx;
    tx.add_execute_conditionally(
        [](const std::string &, const std::string &) { return false; });

    EXPECT_EQ("", call(tx, "CREATE EVENT foo"));
  }

  const auto EXPECT_TYPE_NAME =
      [](const std::string &stmt, const std::string &type,
         const std::string &name, const bool called = true) {
        SCOPED_TRACE("statement: " + stmt + ", type: " + type + ", name " +
                     name + ", called: " + (called ? "true" : "false"));

        bool was_called = false;
        Dump_loader::Sql_transform tx;

        tx.add_execute_conditionally(
            [&was_called, &type, &name](const std::string &t,
                                        const std::string &n) {
              EXPECT_EQ(type, t);
              EXPECT_EQ(name, n);
              was_called = true;
              return true;
            });

        std::string tmp;
        EXPECT_TRUE(tx(stmt.c_str(), stmt.length(), &tmp));

        EXPECT_EQ(was_called, called);
      };

  EXPECT_TYPE_NAME("CREATE EVENT foo", "EVENT", "foo");
  EXPECT_TYPE_NAME("/*!50106 DROP EVENT foo */", "EVENT", "foo");
  EXPECT_TYPE_NAME("/*!50106 DROP EVENT IF EXISTS foo */", "EVENT", "foo");
  EXPECT_TYPE_NAME("/*!50106 CREATE EVENT `foo`", "EVENT", "foo");
  EXPECT_TYPE_NAME("/*!50106 CREATE DEFINER=root EVENT `foo`", "EVENT", "foo");
  EXPECT_TYPE_NAME("/*!50106 CREATE DEFINER=root EVENT IF NOT EXISTS `foo`",
                   "EVENT", "foo");
  EXPECT_TYPE_NAME("/*!50106 CREATE DEFINER=`root` EVENT `foo`", "EVENT",
                   "foo");
  EXPECT_TYPE_NAME("/*!50106 CREATE DEFINER=`root`@`localhost` EVENT `foo`",
                   "EVENT", "foo");
  EXPECT_TYPE_NAME(
      "/*!50106 CREATE DEFINER=`root`@`localhost` EVENT IF NOT EXISTS `foo`",
      "EVENT", "foo");
  EXPECT_TYPE_NAME(
      "/*!50106 CREATE DEFINER=`root`@`localhost` EVENT IF NOT EXISTS "
      "bar.`foo`",
      "EVENT", "foo");

  EXPECT_TYPE_NAME("DROP FUNCTION foo", "FUNCTION", "foo");
  EXPECT_TYPE_NAME("/*!50003 DROP FUNCTION IF EXISTS foo */", "FUNCTION",
                   "foo");
  EXPECT_TYPE_NAME(
      "/*!50003 CREATE DEFINER=`root`@`localhost` FUNCTION `foo`(p INT) "
      "RETURNS int */",
      "FUNCTION", "foo");

  EXPECT_TYPE_NAME("/*!50003 DROP PROCEDURE IF EXISTS foo */", "PROCEDURE",
                   "foo");
  EXPECT_TYPE_NAME("CREATE DEFINER=`root`@`localhost` PROCEDURE `bar`.`foo`()",
                   "PROCEDURE", "foo");

  EXPECT_TYPE_NAME("/*!50032 DROP TRIGGER IF EXISTS foo */;", "TRIGGER", "foo");
  EXPECT_TYPE_NAME(
      "/*!50003 CREATE DEFINER=`root`@`localhost` TRIGGER `foo` BEFORE INSERT "
      "ON `bar` FOR EACH ROW BEGIN END */",
      "TRIGGER", "foo");

  EXPECT_TYPE_NAME("/*!50001 CREATE VIEW `foo` AS */", "", "", false);
  EXPECT_TYPE_NAME("/*!50001 CREATE SQL SECURITY DEFINER VIEW `foo` AS */", "",
                   "", false);
  EXPECT_TYPE_NAME(
      "/*!50001 CREATE DEFINER=`root`@`%` SQL SECURITY DEFINER VIEW `foo` AS "
      "*/",
      "", "", false);
  EXPECT_TYPE_NAME(
      "/*!50001 CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`%` SQL SECURITY "
      "DEFINER VIEW `foo` AS */",
      "", "", false);
}

static std::string table_name_for_chunk_file(const std::string &f) {
  return shcore::str_rstrip(f.substr(0, f.rfind('@')), "@");
}

shcore::Interpreter_print_handler g_silencer(
    nullptr, [](void *, const char *) { return true; },
    [](void *, const char *) { return true; },
    [](void *, const char *) { return true; });

namespace {
#include "unittest/data/load/test_dump1.h"

class Schedule_checker {
 public:
  Schedule_checker(const mysqlshdk::storage::IDirectory::File_info *file_list,
                   size_t file_list_size, size_t num_threads)
      : m_num_threads(num_threads) {
    for (size_t i = 0; i < file_list_size; i++) {
      if (shcore::str_endswith(file_list[i].name(), ".zst")) {
        m_chunks_available.insert(file_list[i].name());
      }
      m_file_sizes[file_list[i].name()] = file_list[i].size();
    }
    tables_by_size();
  }

  void on_file_load(const std::string &filename) {
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      m_tables_being_loaded.push_back(filename);

      validate_scheduling(m_tables_being_loaded, m_num_threads,
                          tables_by_size());
    }

    shcore::sleep_ms(m_file_sizes[filename] / 4000000);

    {
      std::lock_guard<std::mutex> lock(m_mutex);

      m_tables_being_loaded.remove(filename);
      m_chunks_available.erase(filename);
    }
  }

 private:
  void validate_scheduling(const std::list<std::string> &files,
                           size_t num_threads,
                           const std::vector<std::string> &bigger_tables) {
    std::set<std::string> unique_tables_available;
    std::set<std::string> unique_tables_loading;

    for (const auto &f : m_chunks_available) {
      unique_tables_available.insert(table_name_for_chunk_file(f));
    }

    for (const auto &f : files) {
      unique_tables_loading.insert(table_name_for_chunk_file(f));
    }

    // this validates that we give preference to loading different tables
    // at the same time

    // Only check distribution if all threads are busy, since there will be
    // a delay from chunks finishing loading to new chunks being scheduled
    if (files.size() >= num_threads) {
      EXPECT_GE(unique_tables_loading.size(),
                std::min(unique_tables_available.size(), files.size()))
          << "\nunique_available=" << unique_tables_available.size()
          << "\tunique_loading=" << unique_tables_loading.size()
          << "\ttotal loading=" << files.size() << "\n"
          << "LOADING:\n"
          << " - " + shcore::str_join(files, "\n - ") << "\n"
          << "UNIQUE LOADING:\n"
          << " - " << shcore::str_join(unique_tables_loading, "\n - ") << "\n"
          << "TABLES AVAILABLE:\n"
          << " - "
          << shcore::str_join(bigger_tables, "\n - ",
                              [&](const std::string &f) {
                                return shcore::str_ljust(f, 26) + " " +
                                       std::to_string(total_table_size(f));
                              })
          << "\n";

      // this validates that bigger tables are loaded first
      int count = unique_tables_loading.size();

      for (auto it = bigger_tables.begin();
           it != bigger_tables.end() && count > 0; ++it, --count) {
        EXPECT_TRUE(unique_tables_loading.count(*it) > 0 ||
                    unique_tables_available.count(*it) == 0)
            << "BAD TABLE ORDER\n"
            << "LOADING:\n"
            << " - " << shcore::str_join(unique_tables_loading, "\n - ") << "\n"
            << "AVAILABLE:\n"
            << " - " << shcore::str_join(unique_tables_available, "\n - ")
            << "\n"
            << "TOP TABLES:\n"
            << " - "
            << shcore::str_join(bigger_tables, "\n - ",
                                [&](const std::string &f) {
                                  return shcore::str_ljust(f, 26) + " " +
                                         std::to_string(total_table_size(f));
                                })
            << "\n";
      }
    }
  }

  size_t total_table_size(const std::string &table) {
    size_t total = 0;
    for (const auto &f : m_chunks_available) {
      if (shcore::str_endswith(f, ".zst")) {
        if (table == table_name_for_chunk_file(f)) total += m_file_sizes[f];
      }
    }
    return total;
  }

  std::vector<std::string> tables_by_size() {
    std::vector<std::string> bigger_tables;

    std::map<std::string, size_t> table_sizes;

    for (const auto &f : m_chunks_available) {
      if (shcore::str_endswith(f, ".zst")) {
        auto table = table_name_for_chunk_file(f);
        table_sizes[table] += m_file_sizes[f];
      }
    }
    for (const auto &i : table_sizes) {
      bigger_tables.push_back(i.first);
    }
    std::sort(bigger_tables.begin(), bigger_tables.end(),
              [&table_sizes](const std::string &t1, const std::string &t2) {
                return table_sizes[t1] > table_sizes[t2];
              });

    return bigger_tables;
  }

  size_t m_num_threads;
  std::mutex m_mutex;
  std::list<std::string> m_bigger_tables;
  std::list<std::string> m_tables_being_loaded;
  std::set<std::string> m_chunks_available;
  std::map<std::string, size_t> m_file_sizes;
};

}  // namespace

class Load_dump_mocked : public Shell_core_test_wrapper {
 public:
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();

    // silence the console
    mysqlsh::current_console()->add_print_handler(&g_silencer);

    m_session_count = 0;

    // override the session factory
    m_old_session_factory = mysqlshdk::db::mysql::Session::set_factory_function(
        [this]() -> std::shared_ptr<mysqlshdk::db::mysql::Session> {
          std::lock_guard<std::mutex> lock(m_sessions_mutex);

          ++m_session_count;

          auto mock = std::make_shared<testing::Mock_mysql_session>();
          EXPECT_CALL(*mock, do_connect(_)).Times(1);
          EXPECT_CALL(*mock, executes(_, _)).Times(AtLeast(0));
          EXPECT_CALL(*mock, execute(_)).Times(AtLeast(0));
          EXPECT_CALL(*mock, get_connection_options())
              .WillRepeatedly(ReturnRef(m_coptions));
          EXPECT_CALL(*mock, get_connection_id())
              .WillRepeatedly(Return(m_session_count));
          EXPECT_CALL(*mock, is_open()).WillRepeatedly(Return(false));

          if (m_auto_generate_pk_value &&
              *m_auto_generate_pk_value != m_create_invisible_pks) {
            // this query is executed only if the requested value is different
            // from the current value
            const auto query = shcore::sqlformat(
                "SET @@SESSION.sql_generate_invisible_primary_key=?",
                m_create_invisible_pks);
            EXPECT_CALL(*mock, executes(StrEq(query.c_str()), _))
                .Times(Exactly(1));
          }

          {
            // this is the main connection used by the loader

            mock->expect_query(
                    "SELECT CONCAT(@@version, ' ', @@version_comment)")
                .then({"a"})
                .add_row({m_version + " mocked"});

            mock->expect_query(
                    "SELECT schema_name FROM information_schema.schemata WHERE "
                    "schema_name in ('stackoverflow')")
                .then({"a"});

            if (Version(m_version) >= Version(8, 0, 13) &&
                !m_create_invisible_pks) {
              mock->expect_query(
                      "show variables like 'sql_require_primary_key';")
                  .then({"a", "b"})
                  .add_row({"sql_require_primary_key", "1"});
            }

            mock->expect_query(
                    "SELECT VARIABLE_VALUE = 'OFF' FROM "
                    "performance_schema.global_status WHERE variable_name = "
                    "'Innodb_redo_log_enabled'")
                .then({"a"});

            mock->expect_query("SHOW GLOBAL VARIABLES LIKE 'local_infile'")
                .then({"a", "b"})
                .add_row({"local_infile", "1"});
          }

          mock->set_query_handler(
              [this](const std::string &sql)
                  -> std::shared_ptr<mysqlshdk::db::IResult> {
                if (sql.find("LOAD DATA LOCAL INFILE ") != std::string::npos) {
                  size_t start = sql.find('\'') + 1;
                  size_t end = sql.find('\'', start);

                  std::string filename = sql.substr(start, end - start);

                  if (m_on_load_data) {
                    m_on_load_data(filename);
                  }

                  auto r = std::make_shared<testing::Mock_result>();
                  EXPECT_CALL(*r, get_warning_count())
                      .WillRepeatedly(Return(0));
                  return r;
                }
                return {};
              });

          m_sessions.push_back(mock);

          return mock;
        });
  }

  std::shared_ptr<mysqlshdk::db::ISession> make_mock_main_session() {
    auto mock_main_session = std::make_shared<testing::Mock_mysql_session>();
    EXPECT_CALL(*mock_main_session, get_connection_options())
        .WillRepeatedly(ReturnRef(m_coptions));
    mock_main_session->expect_query("SELECT @@version")
        .then({"version"})
        .add_row({m_version});

    {
      auto &r = mock_main_session
                    ->expect_query(
                        "show GLOBAL variables where `variable_name` in "
                        "('sql_generate_invisible_primary_key')")
                    .then({"Variable_name", "Value"});

      if (m_auto_generate_pk_value) {
        r.add_row({"sql_generate_invisible_primary_key",
                   *m_auto_generate_pk_value ? "ON" : "OFF"});

        const auto query = shcore::sqlformat(
            "SET @@SESSION.sql_generate_invisible_primary_key=?",
            *m_auto_generate_pk_value);
        EXPECT_CALL(*mock_main_session, executes(StrEq(query.c_str()), _))
            .Times(Exactly(1));
      }
    }

    mock_main_session->expect_query("SELECT @@server_uuid")
        .then({"@@server_uuid"})
        .add_row({"UUID"});

    mock_main_session
        ->expect_query(
            "SELECT VARIABLE_VALUE = 'OFF' FROM "
            "performance_schema.global_status WHERE variable_name = "
            "'Innodb_redo_log_enabled'")
        .then({""});

    mock_main_session->expect_query("SHOW GLOBAL VARIABLES LIKE 'local_infile'")
        .then({"a", "b"})
        .add_row({"local_infile", "1"});
    return std::static_pointer_cast<mysqlshdk::db::ISession>(mock_main_session);
  }

  void TearDown() override {
    m_sessions.clear();

    mysqlsh::current_console()->remove_print_handler(&g_silencer);

    mysqlshdk::db::mysql::Session::set_factory_function(m_old_session_factory);

    Shell_core_test_wrapper::TearDown();
  }

  template <typename... Options>
  void load_dump(int num_threads, bool check_scheduling, Options &&... opts) {
    auto dir = std::make_unique<tests::Dummy_dump_directory>(
        shcore::path::join_path(g_test_home, "data/load/test_dump1"),
        test_dump1_files, test_dump1_files_size);

    Load_dump_options options;
    options.set_session(make_mock_main_session(), "");

    Load_dump_options::options().unpack(
        shcore::make_dict("threads", num_threads, "showProgress", false,
                          "progressFile", "", std::forward<Options>(opts)...),
        &options);

    Dump_loader loader(options);

    ASSERT_EQ(options.threads_count(), num_threads);

    Schedule_checker checker(test_dump1_files, test_dump1_files_size,
                             num_threads);

    if (check_scheduling) {
      m_on_load_data = [&](const std::string &filename) {
        checker.on_file_load(filename);
      };
    }

    // open the mock dump
    loader.open_dump(std::move(dir));
    loader.m_dump->rescan();

    // run the load
    loader.spawn_workers();
    {
      shcore::on_leave_scope cleanup([&loader]() {
        loader.join_workers();

        loader.m_progress_thread.finish();
      });

      loader.execute_tasks();
    }
  }

  mysqlshdk::db::Connection_options m_coptions =
      mysqlshdk::db::Connection_options("mysql://root@localhost:3306");
  int m_session_count;
  std::function<std::shared_ptr<mysqlshdk::db::mysql::Session>()>
      m_old_session_factory;

  std::mutex m_sessions_mutex;
  std::vector<std::shared_ptr<testing::Mock_mysql_session>> m_sessions;

  std::function<void(const std::string &)> m_on_load_data;

  std::string m_version = "8.0.20";
  mysqlshdk::null_bool m_auto_generate_pk_value;
  bool m_create_invisible_pks = false;
};

TEST_F(Load_dump_mocked, chunk_scheduling_more_threads) {
  // Simulate a dump load by using a semi-mock of a dump (all metadata/ddl is
  // real, but no actual data) and DB sessions are mocked
  // The goal is to test that the chunk scheduling is working as expected
  load_dump(36, true);
}

TEST_F(Load_dump_mocked, chunk_scheduling_more_tables) {
  // Simulate a dump load by using a semi-mock of a dump (all metadata/ddl is
  // real, but no actual data) and DB sessions are mocked
  // The goal is to test that the chunk scheduling is working as expected
  load_dump(4, true);
}

static constexpr const char *k_mds_administrator_restrictions =
    R"*([{"Database": "sys", "Privileges": ["CREATE", "DROP", "REFERENCES", "INDEX", "ALTER", "CREATE TEMPORARY TABLES", "LOCK TABLES", "CREATE VIEW", "CREATE ROUTINE", "ALTER ROUTINE", "EVENT", "TRIGGER"]}, {"Database": "mysql", "Privileges": ["INSERT", "UPDATE", "DELETE", "CREATE", "DROP", "REFERENCES", "INDEX", "ALTER", "CREATE TEMPORARY TABLES", "LOCK TABLES", "EXECUTE", "CREATE VIEW", "CREATE ROUTINE", "ALTER ROUTINE", "EVENT", "TRIGGER"]}])*";

TEST_F(Load_dump_mocked, filter_user_script_for_mds) {
  Load_dump_options options;

  auto mock_main_session = std::make_shared<testing::Mock_mysql_session>();

  EXPECT_CALL(*mock_main_session, get_connection_options())
      .WillRepeatedly(ReturnRef(m_coptions));

  mock_main_session->expect_query("SELECT @@version")
      .then({"version"})
      .add_row({"8.0.21-u1-cloud"});

  mock_main_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('sql_generate_invisible_primary_key')")
      .then({"Variable_name", "Value"});

  mock_main_session->expect_query("SELECT @@server_uuid")
      .then({"@@server_uuid"})
      .add_row({"UUID"});

  options.set_session(mock_main_session, "");

  Dump_loader loader(options);

  loader.m_session = options.base_session();

  shcore::iterdir(
      shcore::path::join_path(g_test_home,
                              "data/load/filter_user_script_for_mds"),
      [&](const std::string &f) -> bool {
        if (!shcore::str_endswith(f, ".sql")) return true;
        auto path = shcore::path::join_path(
            g_test_home, "data/load/filter_user_script_for_mds", f);

        std::string script;
        std::string expected_out;

        EXPECT_TRUE(shcore::load_text_file(path, script));
        EXPECT_TRUE(shcore::load_text_file(path + ".out", expected_out));

        auto &r = mock_main_session
                      ->expect_query(
                          "SELECT privilege_type FROM "
                          "information_schema.user_privileges "
                          "WHERE grantee=concat(quote('administrator'), '@', "
                          "quote('%'))")
                      .then({"privilege_type"});
        for (const auto &p :
             mysqlsh::compatibility::k_mysqlaas_allowed_privileges)
          r.add_row({p});

        mock_main_session
            ->expect_query(
                "SELECT User_attributes->>'$.Restrictions' FROM mysql.user "
                "WHERE user='administrator' AND host='%'")
            .then({"res"})
            .add_row({k_mds_administrator_restrictions});

        auto filter = loader.filter_user_script_for_mds();
        auto out = dump::Schema_dumper::preprocess_users_script(
            script, [](const std::string &) { return true; }, {}, filter);

        EXPECT_EQ(out, expected_out) << path;

        return true;
      });
}

TEST_F(Load_dump_mocked, sql_generate_invisible_primary_key) {
  // WL14506-TSFR_4.6_1
  m_version = "8.0.24";
  // sql_generate_invisible_primary_key is supported, global value is false
  m_auto_generate_pk_value = false;
  // we want to create PKs, and expect that query which sets
  // sql_generate_invisible_primary_key to ON is executed
  m_create_invisible_pks = true;

  load_dump(4, false, "createInvisiblePKs", m_create_invisible_pks);
}

TEST_F(Load_dump_mocked, bug_34110375) {
  m_version = "8.0.30";
  // sql_generate_invisible_primary_key is supported, global value is true
  m_auto_generate_pk_value = true;
  // we want to create PKs, but the global value is already set to true, query
  // should NOT be executed
  m_create_invisible_pks = true;

  load_dump(4, false, "createInvisiblePKs", m_create_invisible_pks);
}

}  // namespace mysqlsh
