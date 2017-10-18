/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
*/

#include <tuple>
#include <gtest/gtest.h>
#include "unittest/test_utils/admin_api_test.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "modules/mod_mysql_session.h"

namespace tests {
class Dba_sql_test: public Admin_api_test {
 protected:
  std::shared_ptr<mysqlsh::mysql::ClassicSession> create_session(int port) {
    auto session = std::shared_ptr<mysqlsh::mysql::ClassicSession>
        (new mysqlsh::mysql::ClassicSession());

    shcore::Argument_list args;
    args.push_back(shcore::Value("user:@localhost:" + std::to_string(port)));
    session->connect(args);

    return session;
  }
};

// Test get_instance_state()
TEST_F(Dba_sql_test, get_instance_state) {
  using namespace mysqlsh::dba::ManagedInstance;
  std::vector<std::string> member_states = {
      "ONLINE", "RECOVERING", "OFFLINE", "UNREACHABLE", "ERROR", "NULL"};
  std::vector<std::tuple<State, State, State>> expected_results = {
      std::make_tuple(State::OnlineRW, State::OnlineRO, State::OnlineRW),
      std::make_tuple(State::Recovering, State::Recovering, State::Recovering),
      std::make_tuple(State::Offline, State::Offline, State::Offline),
      std::make_tuple(State::Unreachable, State::Unreachable, State::Unreachable),
      std::make_tuple(State::Error, State::Error, State::Error),
      std::make_tuple(State::Missing, State::Missing, State::Missing),
  };

  for (int i=0; i < member_states.size(); i++) {
    std::string member_state = member_states[i];
    std::tuple<State, State, State> expected_res = expected_results[i];

    // Test single-primary scenario (gr_primary_member == mysql_server_uuid):
    // variable_value (group_replication_primary_member)
    // --------------
    // 8b53fb76-5c0e-11e7-b067-5ce0c50b9d66

    // mysql_server_uuid | instance_name | member_state
    // ----------------- + ------------- + ------------
    // 8b53fb76-5c0e-11e7-b067-5ce0c50b9d66 | localhost:3300 | <member_state>
    std::vector< tests::Fake_result_data > queries;
    add_gr_primary_member_query(&queries,
                                "8b53fb76-5c0e-11e7-b067-5ce0c50b9d66");
    add_member_state_query(&queries, "localhost:3300",
                           "8b53fb76-5c0e-11e7-b067-5ce0c50b9d66",
                           "localhost:3300", member_state);
    START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
    auto session = create_session(_mysql_sandbox_nport1);
    try {
      mysqlsh::dba::ManagedInstance::State state =
          mysqlsh::dba::get_instance_state(session->connection(),
                                           "localhost:3300");
      EXPECT_EQ(state, std::get<0>(expected_res));
    } catch (const shcore::Exception &e) {
      SCOPED_TRACE(e.what());
      SCOPED_TRACE("Unexpected failure getting instance state for "
                   "single-primary scenario (gr_primary_member = "
                   "mysql_server_uuid) with member_state: " +
                   member_state + ".");
      ADD_FAILURE();
    }
    session->close(shcore::Argument_list());
    stop_server_mock(_mysql_sandbox_nport1);

    // Test single-primary scenario (gr_primary_member != mysql_server_uuid):
    // variable_value (group_replication_primary_member)
    // --------------
    // 8b53fb76-5c0e-11e7-b067-5ce0c50b9d66

    // mysql_server_uuid | instance_name | member_state
    // ----------------- + ------------- + ------------
    // 1406a49b-5c24-11e7-b979-5ce0c50b9d66 | localhost:3300 | <member_state>
    queries.clear();
    add_gr_primary_member_query(&queries,
                                "8b53fb76-5c0e-11e7-b067-5ce0c50b9d66");
    add_member_state_query(&queries, "localhost:3300",
                           "1406a49b-5c24-11e7-b979-5ce0c50b9d66",
                           "localhost:3300", member_state);
    START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
    session = create_session(_mysql_sandbox_nport1);
    try {
      mysqlsh::dba::ManagedInstance::State state =
          mysqlsh::dba::get_instance_state(session->connection(),
                                           "localhost:3300");
      EXPECT_EQ(state, std::get<1>(expected_res));
    } catch (const shcore::Exception &e) {
      SCOPED_TRACE(e.what());
      SCOPED_TRACE("Unexpected failure getting instance state for "
                   "single-primary scenario (gr_primary_member != "
                   "mysql_server_uuid) with member_state: " +
                   member_state + ".");
      ADD_FAILURE();
    }
    session->close(shcore::Argument_list());
    stop_server_mock(_mysql_sandbox_nport1);

    // Test multi-primary scenario (gr_primary_member = ""):
    // variable_value (group_replication_primary_member)
    // --------------
    // ""

    // mysql_server_uuid | instance_name | member_state
    // ----------------- + ------------- + ------------
    // 1406a49b-5c24-11e7-b979-5ce0c50b9d66 | localhost:3300 | <member_state>
    queries.clear();
    add_gr_primary_member_query(&queries, "");
    add_member_state_query(&queries, "localhost:3300",
                           "1406a49b-5c24-11e7-b979-5ce0c50b9d66",
                           "localhost:3300", member_state);
    START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
    session = create_session(_mysql_sandbox_nport1);
    try {
      mysqlsh::dba::ManagedInstance::State state =
          mysqlsh::dba::get_instance_state(session->connection(),
                                           "localhost:3300");
      EXPECT_EQ(state, std::get<2>(expected_res));
    } catch (const shcore::Exception &e) {
      SCOPED_TRACE(e.what());
      SCOPED_TRACE("Unexpected failure getting instance state for "
                   "multi-primary scenario (gr_primary_member = '') with "
                   "member_state: " + member_state + ".");
      ADD_FAILURE();
    }
    session->close(shcore::Argument_list());
    stop_server_mock(_mysql_sandbox_nport1);
  }
}

// Test get_instance_state() errors
TEST_F(Dba_sql_test, get_instance_state_errors) {
  // Test invalid member state (unexpected state from server side).
  // variable_value (group_replication_primary_member)
  // --------------
  // 8b53fb76-5c0e-11e7-b067-5ce0c50b9d66

  // mysql_server_uuid | instance_name | member_state
  // ----------------- + ------------- + ------------
  // 1406a49b-5c24-11e7-b979-5ce0c50b9d66 | localhost:3300 | INVALID
  std::vector< tests::Fake_result_data > queries;
  add_gr_primary_member_query(&queries, "8b53fb76-5c0e-11e7-b067-5ce0c50b9d66");
  add_member_state_query(&queries, "localhost:3300",
                         "1406a49b-5c24-11e7-b979-5ce0c50b9d66",
                         "localhost:3300", "INVALID");
  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);
  try {
    mysqlsh::dba::ManagedInstance::State state =
        mysqlsh::dba::get_instance_state(session->connection(),
                                         "localhost:3300");
    SCOPED_TRACE("Exception expected to be thrown for 'INVALID' state.");
    ADD_FAILURE();
  } catch (shcore::Exception &err) {
    EXPECT_STREQ("RuntimeError", err.type());
    EXPECT_STREQ(
        "The instance 'localhost:3300' has an unexpected status: 'INVALID'.",
        err.what());
  } catch (...) {
    SCOPED_TRACE("Excpected runtime_error for 'INVALID' state.");
    ADD_FAILURE();
  }
  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);

  // NOTE: No test added to validate the error thrown when no state
  // information is available for the instance (instance no longer part of the
  // cluster) because returning results with no rows (empty) is currently
  // supported by server mock code.
  // TODO: Add a test later (when supported) to validate this error,
  //       using the following data:
  // variable_value (group_replication_primary_member)
  // --------------
  // 8b53fb76-5c0e-11e7-b067-5ce0c50b9d66

  // mysql_server_uuid | instance_name | member_state
  // ----------------- + ------------- + ------------
  // (empty)
}

// Test get_peer_seeds()
TEST_F(Dba_sql_test, get_peer_seeds) {
  std::vector< tests::Fake_result_data > queries;
  std::vector<std::vector<std::string>> metadata_values;
  std::string gr_group_seed;

  // Test with metadata GR addresses not in gr_group_seed variable:
  // group_replication_group_seeds
  // -----------------------------
  // localhost:13300

  // JSON_UNQUOTE(addresses->'$.grLocal')
  // ------------------------------------
  // localhost:13301
  // localhost:13302
  gr_group_seed = "localhost:13300";
  metadata_values = {{"localhost:13301"}, {"localhost:13302"}};
  add_get_peer_seeds_queries(&queries, metadata_values, gr_group_seed,
                             "localhost:3300");
  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);
  try {
    std::vector<std::string> seeds =
        mysqlsh::dba::get_peer_seeds(session->connection(), "localhost:3300");
    std::vector<std::string> result = {"localhost:13300", "localhost:13301",
                                       "localhost:13302"};
    EXPECT_EQ(result, seeds);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure getting peer seeds.");
    ADD_FAILURE();
  }
  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);

  // Test with metadata GR addresses in gr_group_seed variable:
  // group_replication_group_seeds
  // -----------------------------
  // localhost:13300,localhost:13301,localhost:13302

  // JSON_UNQUOTE(addresses->'$.grLocal')
  // ------------------------------------
  // localhost:13301
  // localhost:13302
  queries.clear();
  gr_group_seed = "localhost:13300,localhost:13301,localhost:13302";
  metadata_values = {{"localhost:13301"}, {"localhost:13302"}};
  add_get_peer_seeds_queries(&queries, metadata_values, gr_group_seed,
                             "localhost:3300");
  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  session = create_session(_mysql_sandbox_nport1);
  try {
    std::vector<std::string> seeds =
        mysqlsh::dba::get_peer_seeds(session->connection(), "localhost:3300");
    std::vector<std::string> result = {"localhost:13300", "localhost:13301",
                                       "localhost:13302"};
    EXPECT_EQ(result, seeds);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure getting peer seeds.");
    ADD_FAILURE();
  }
  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);

  // Test with metadata GR addresses and gr_group_seed variable is "" (empty):
  // group_replication_group_seeds
  // -----------------------------
  // "" (empty)

  // JSON_UNQUOTE(addresses->'$.grLocal')
  // ------------------------------------
  // localhost:13301
  // localhost:13302
  queries.clear();
  gr_group_seed = "";
  metadata_values = {{"localhost:13301"}, {"localhost:13302"}};
  add_get_peer_seeds_queries(&queries, metadata_values, gr_group_seed,
                             "localhost:3300");
  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  session = create_session(_mysql_sandbox_nport1);
  try {
    std::vector<std::string> seeds =
        mysqlsh::dba::get_peer_seeds(session->connection(), "localhost:3300");
    std::vector<std::string> result = {"localhost:13301", "localhost:13302"};
    EXPECT_EQ(result, seeds);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure getting peer seeds.");
    ADD_FAILURE();
  }
  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);

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
