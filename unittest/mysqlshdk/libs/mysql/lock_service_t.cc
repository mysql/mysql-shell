/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

//#include <gtest/gtest.h>
#include <memory>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/lock_service.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_base_test.h"

namespace testing {

class Lock_service_test : public tests::Shell_base_test {
 protected:
  void SetUp() {
    tests::Shell_base_test::SetUp();

    // Create instance and Open the session for the tests.
    _connection_options = shcore::get_connection_options(_mysql_uri);
    _session->connect(_connection_options);
    instance = new mysqlshdk::mysql::Instance(_session);

    // Create a second instance (session).
    _session2->connect(_connection_options);
    instance2 = new mysqlshdk::mysql::Instance(_session2);
  }

  void TearDown() {
    tests::Shell_base_test::TearDown();

    // Close the session.
    _session->close();
    delete instance;
    if (_session2->is_open()) {
      _session2->close();
      delete instance2;
    }
  }

  std::shared_ptr<mysqlshdk::db::ISession> _session =
      mysqlshdk::db::mysql::Session::create();
  mysqlshdk::mysql::Instance *instance;
  mysqlshdk::db::Connection_options _connection_options;

  std::shared_ptr<mysqlshdk::db::ISession> _session2 =
      mysqlshdk::db::mysql::Session::create();
  mysqlshdk::mysql::Instance *instance2;
};

TEST_F(Lock_service_test, install_udfs) {
  if (mysqlshdk::mysql::has_lock_service_udfs(*instance)) {
    // NOTE: Error issued when trying to install UDFs that already exist.
    SKIP_TEST("Lock service UDFs already installed on test server.");
  }

  // TEST: No lock service UDFs installed, installation should succeed.
  EXPECT_NO_THROW(mysqlshdk::mysql::install_lock_service_udfs(instance));
  EXPECT_TRUE(mysqlshdk::mysql::has_lock_service_udfs(*instance));

  // TEST: Installation succeed even if some UDFs are already installed.
  // BUG#30589080: shell fails using lock service although it could activate it.
  instance->execute("DROP FUNCTION IF EXISTS service_get_read_locks");
  EXPECT_FALSE(mysqlshdk::mysql::has_lock_service_udfs(*instance));
  EXPECT_NO_THROW(mysqlshdk::mysql::install_lock_service_udfs(instance));
  EXPECT_TRUE(mysqlshdk::mysql::has_lock_service_udfs(*instance));

  instance->execute("DROP FUNCTION IF EXISTS service_get_write_locks");
  EXPECT_FALSE(mysqlshdk::mysql::has_lock_service_udfs(*instance));
  EXPECT_NO_THROW(mysqlshdk::mysql::install_lock_service_udfs(instance));
  EXPECT_TRUE(mysqlshdk::mysql::has_lock_service_udfs(*instance));

  instance->execute("DROP FUNCTION IF EXISTS service_release_locks");
  EXPECT_FALSE(mysqlshdk::mysql::has_lock_service_udfs(*instance));
  EXPECT_NO_THROW(mysqlshdk::mysql::install_lock_service_udfs(instance));
  EXPECT_TRUE(mysqlshdk::mysql::has_lock_service_udfs(*instance));
}

TEST_F(Lock_service_test, lock_erros) {
  using mysqlshdk::mysql::Lock_mode;

  // Name space and lock name cannot be empty.
  EXPECT_THROW_LIKE(mysqlshdk::mysql::get_lock(*instance, "", "lock1",
                                               Lock_mode::EXCLUSIVE, 0),
                    mysqlshdk::db::Error,
                    "Incorrect locking service lock name ''.");
  EXPECT_THROW_LIKE(mysqlshdk::mysql::get_lock(*instance, "test_locks", "",
                                               Lock_mode::SHARED, 0),
                    mysqlshdk::db::Error,
                    "Incorrect locking service lock name ''.");
  EXPECT_THROW_LIKE(mysqlshdk::mysql::release_lock(*instance, ""),
                    mysqlshdk::db::Error,
                    "Incorrect locking service lock name ''.");

  // Name space and lock name have a maximum length of 64 characters.
  EXPECT_THROW_LIKE(
      mysqlshdk::mysql::get_lock(
          *instance,
          "very_very_long_name_space_with_more_than_64_characters_are_invalid",
          "lock1", Lock_mode::SHARED, 0),
      mysqlshdk::db::Error,
      "Incorrect locking service lock name "
      "'very_very_long_name_space_with_more_than_64_characters_are_invalid'.");
  EXPECT_THROW_LIKE(
      mysqlshdk::mysql::get_lock(
          *instance, "test_locks",
          "very_very_long_lock_name_with_more_than_64_characters_are_invalid",
          Lock_mode::EXCLUSIVE, 0),
      mysqlshdk::db::Error,
      "Incorrect locking service lock name "
      "'very_very_long_lock_name_with_more_than_64_characters_are_invalid'.");
  EXPECT_THROW_LIKE(
      mysqlshdk::mysql::release_lock(
          *instance,
          "very_very_long_name_space_with_more_than_64_characters_are_invalid"),
      mysqlshdk::db::Error,
      "Incorrect locking service lock name "
      "'very_very_long_name_space_with_more_than_64_characters_are_invalid'.");
}

TEST_F(Lock_service_test, locks_usage) {
  using mysqlshdk::mysql::Lock_mode;

  // Enable metadata lock instruments to allow lock monitoring.
  try {
    instance->execute(
        "UPDATE performance_schema.setup_instruments SET ENABLED = 'YES' "
        "WHERE NAME = 'wait/lock/metadata/sql/mdl'");
  } catch (...) {
    SKIP_TEST("Failed to enable metadata lock instruments.");
  }

  // Ok to acquire multiple shared locks with the same name.
  mysqlshdk::mysql::get_lock(*instance, "test_locks", "lock1",
                             Lock_mode::SHARED, 0);
  EXPECT_NO_THROW(mysqlshdk::mysql::get_lock(*instance, "test_locks", "lock1",
                                             Lock_mode::SHARED, 0));
  EXPECT_NO_THROW(mysqlshdk::mysql::get_lock(*instance2, "test_locks", "lock1",
                                             Lock_mode::SHARED, 0));

  // Fail to acquire an exclusive lock if a shared lock is already hold by
  // another session.
  EXPECT_THROW_LIKE(
      mysqlshdk::mysql::get_lock(*instance2, "test_locks", "lock1",
                                 Lock_mode::EXCLUSIVE, 0),
      mysqlshdk::db::Error, "Service lock wait timeout exceeded.");

  // Ok to acquire an exclusive lock on a different name.
  EXPECT_NO_THROW(mysqlshdk::mysql::get_lock(*instance2, "test_locks", "lock2",
                                             Lock_mode::EXCLUSIVE, 0));

  // Release previous shared locks.
  mysqlshdk::mysql::release_lock(*instance, "test_locks");

  // Ok to acquire an exclusive lock on the same name now.
  EXPECT_NO_THROW(mysqlshdk::mysql::get_lock(*instance2, "test_locks", "lock1",
                                             Lock_mode::EXCLUSIVE, 0));

  // Fail to acquire a shared lock if an exclusive lock is already hold by
  // another session.
  EXPECT_THROW_LIKE(mysqlshdk::mysql::get_lock(*instance, "test_locks", "lock1",
                                               Lock_mode::SHARED, 0),
                    mysqlshdk::db::Error,
                    "Service lock wait timeout exceeded.");

  // Fail to acquire an exclusive lock if one is already hold by another
  // session.
  EXPECT_THROW_LIKE(mysqlshdk::mysql::get_lock(*instance, "test_locks", "lock1",
                                               Lock_mode::EXCLUSIVE, 0),
                    mysqlshdk::db::Error,
                    "Service lock wait timeout exceeded.");
  EXPECT_THROW_LIKE(mysqlshdk::mysql::get_lock(*instance, "test_locks", "lock2",
                                               Lock_mode::EXCLUSIVE, 0),
                    mysqlshdk::db::Error,
                    "Service lock wait timeout exceeded.");

  // Closing a session, implicitly releases locks.
  instance2->close_session();
  delete instance2;

  // Make sure the lock was released to avoid non-deterministic issue due to
  // locks still being hold a small amount of time after closing the session.

  // Get the number of locks still hold for 'test_locks'.
  auto num_of_locks = [this]() {
    auto res = instance->query(
        "SELECT COUNT(*) FROM performance_schema.metadata_locks "
        "WHERE object_type = 'LOCKING SERVICE' AND object_schema = "
        "'test_locks'");
    return res->fetch_one()->get_int(0);
  };

  // Wait until no lock are hold for 'test_locks'. Check every 0.5 sec at most
  // 20 times (max 10 sec).
  for (int i = 0; i < 20; ++i) {
    int n = num_of_locks();
    if (n == 0) {
      break;
    }
    shcore::sleep_ms(500);
  }

  // Ok to acquire any locks again on the same name.
  EXPECT_NO_THROW(mysqlshdk::mysql::get_lock(*instance, "test_locks", "lock1",
                                             Lock_mode::EXCLUSIVE, 0));

  // Release locks at the end.
  mysqlshdk::mysql::release_lock(*instance, "test_locks");
}

TEST_F(Lock_service_test, uninstall_udfs) {
  EXPECT_NO_THROW(mysqlshdk::mysql::uninstall_lock_service_udfs(instance));
  EXPECT_FALSE(mysqlshdk::mysql::has_lock_service_udfs(*instance));
}

TEST_F(Lock_service_test, has_lock_service_udfs) {
  using mysqlshdk::db::Type;

  // Test to ensure exact plugin name is used in query to check UDFs existence.
  // BUG#30590441 : locking service availability check should use exact name
  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();

  // Lambda function to simplify testing for different scenarios.
  auto test = [&mock_session](int existing_udfs, std::string os) -> bool {
    SCOPED_TRACE("Test has_lock_service_udfs() with " +
                 std::to_string(existing_udfs) +
                 " existing locking UDFs for '" + os + "' platform.");
    mysqlshdk::mysql::Instance mock_instance{mock_session};

    // Query for the used OS.
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in "
            "('version_compile_os')")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String, Type::String},
                       {{"version_compile_os", os}}}});

    // Set the lib extension according to the OS.
    std::string instance_os = shcore::str_upper(os);
    std::string ext =
        (instance_os.find("WIN") != std::string::npos) ? ".dll" : ".so";

    // Query to verify existence of lock service UDFs.
    std::string str_query =
        "SELECT COUNT(*) FROM mysql.func "
        "WHERE dl = /*(*/ 'locking_service" +
        ext +
        "' /*(*/ "
        "AND name IN ('service_get_read_locks', 'service_get_write_locks', "
        "'service_release_locks')";
    mock_session->expect_query(str_query).then_return(
        {{"",
          {"COUNT(*)"},
          {Type::Integer},
          {{std::to_string(existing_udfs)}}}});

    // Check if lock service UDFs is installed (3 UDFs expected).
    return mysqlshdk::mysql::has_lock_service_udfs(mock_instance);
  };

  // Test similuating we are on Windows (3 UDFs expected when installed).
  EXPECT_FALSE(test(0, "windows"));
  EXPECT_FALSE(test(2, "windows"));
  EXPECT_TRUE(test(3, "windows"));
  EXPECT_FALSE(test(4, "windows"));

  // Test similuating we are on non-windows (3 UDFs expected when installed).
  EXPECT_FALSE(test(0, "macOS"));
  EXPECT_FALSE(test(2, "macOS"));
  EXPECT_TRUE(test(3, "macOS"));
  EXPECT_FALSE(test(4, "macOS"));
}

}  // namespace testing