/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <gtest/gtest.h>
#include <tuple>
#include "modules/adminapi/common/sql.h"
#include "modules/mod_mysql_session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/test_utils/admin_api_test.h"

namespace tests {
class Dba_sql_test : public Admin_api_test {
 public:
  virtual void SetUp() {
    Admin_api_test::SetUp();
    reset_replayable_shell(
        ::testing::UnitTest::GetInstance()->current_test_info()->name());
  }

  static void SetUpTestCase() {
    SetUpSampleCluster("Dba_sql_test/SetUpTestCase");
  }

  static void TearDownTestCase() {
    TearDownSampleCluster("Dba_sql_test/TearDownTestCase");
  }
};

// Test get_peer_seeds()
TEST_F(Dba_sql_test, get_peer_seeds_md_in_synch) {
  // Test with metadata GR addresses not in gr_group_seed variable:
  // group_replication_group_seeds
  // -----------------------------
  // hostname:port1, hostname:port2

  // JSON_UNQUOTE(addresses->'$.grLocal')
  // ------------------------------------
  // hostname:port2
  auto session = create_session(_mysql_sandbox_ports[0]);
  try {
    std::vector<std::string> seeds = mysqlsh::dba::get_peer_seeds(
        session, hostname() + ":" + std::to_string(_mysql_sandbox_ports[0]));
    std::vector<std::string> result = {
        hostname() + ":" + std::to_string(_mysql_sandbox_ports[1]) + "1"};
    EXPECT_EQ(result, seeds);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure getting peer seeds.");
    ADD_FAILURE();
  }
  session->close();
}

TEST_F(Dba_sql_test, get_peer_seeds_only_in_metadata) {
  // Test with metadata GR addresses in gr_group_seed variable:
  // group_replication_group_seeds
  // -----------------------------
  // hostname:port1,hostname:port2,localhost:port3

  // JSON_UNQUOTE(addresses->'$.grLocal')
  // ------------------------------------
  // hostname:port2
  // hostname:port3
  auto session = create_session(_mysql_sandbox_ports[0]);

  // Insert a fake record for the third instance on the metadata
  std::string query =
      "insert into mysql_innodb_cluster_metadata.instances "
      "values (0, 1, " +
      std::to_string(_replicaset->get_id()) + ", '" + uuid_3 +
      "', 'localhost:<port>', "
      "'HA', NULL, '{\"mysqlX\": \"localhost:<port>0\", "
      "\"grLocal\": \"localhost:<port>1\", "
      "\"mysqlClassic\": \"localhost:<port>\"}', "
      "NULL, NULL, NULL)";

  query = shcore::str_replace(query, "<port>",
                              std::to_string(_mysql_sandbox_ports[2]));

  session->query(query);

  try {
    std::vector<std::string> seeds = mysqlsh::dba::get_peer_seeds(
        session, hostname() + ":" + std::to_string(_mysql_sandbox_ports[0]));
    std::vector<std::string> result = {
        hostname() + ":" + std::to_string(_mysql_sandbox_ports[1]) + "1",
        "localhost:" + std::to_string(_mysql_sandbox_ports[2]) + "1"};
    EXPECT_EQ(result, seeds);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure getting peer seeds.");
    ADD_FAILURE();
  }

  session->query(
      "delete from mysql_innodb_cluster_metadata.instances "
      " where mysql_server_uuid = '" +
      uuid_3 + "'");
  session->close();
}

TEST_F(Dba_sql_test, get_peer_seeds_not_in_metadata) {
  SKIP_TEST(
      "Failing Test: get_seeds only gets info from metadata, "
      "group_replication_group_seeds is always empty.");

  // NOTE(rennox) I suspect this test was wrong since ever since text indicates
  // the tests would be done with metadata addresses empty, and even so it was
  // passing some data. Letting comments for further reference.

  // Test with metadata GR addresses and gr_group_seed variable is "" (empty):
  // group_replication_group_seeds
  // -----------------------------
  // "" (empty)

  // JSON_UNQUOTE(addresses->'$.grLocal')
  // ------------------------------------
  // localhost:13301
  // localhost:13302
  /*queries.clear();
  gr_group_seed = "";
  metadata_values = {{"localhost:13301"}, {"localhost:13302"}};
  add_get_peer_seeds_queries(&queries, metadata_values, gr_group_seed,
                             "localhost:3300");*/
  auto session = create_session(_mysql_sandbox_ports[0]);
  session->query(
      "delete from mysql_innodb_cluster_metadata.instances "
      " where mysql_server_uuid = '" +
      uuid_2 + "'");

  try {
    std::vector<std::string> seeds = mysqlsh::dba::get_peer_seeds(
        session, "localhost:" + std::to_string(_mysql_sandbox_ports[0]));
    std::vector<std::string> result = {
        "localhost:" + std::to_string(_mysql_sandbox_ports[1]) + "1"};
    EXPECT_EQ(result, seeds);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure getting peer seeds.");
    ADD_FAILURE();
  }
  session->close();

  // NOTE: No test added to validate the returned seeds when no rows are
  // returned from the metadata query (only one instance in the cluster)
  // because returning results with no rows (empty) is currently NOT
  // supported by server mock code.
  // TODO: Add a test later (when supported) to validate this situation,
  //       using the following data:
  // group_replication_group_seeds
  // -----------------------------
  // localhost:13300,localhost:13301,localhost:13302

  // JSON_UNQUOTE(addresses->'$.grLocal')
  // ------------------------------------
  // (empty)
}

}  // namespace tests
