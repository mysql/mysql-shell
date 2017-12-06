/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "unittest/test_utils/admin_api_test.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "modules/adminapi/mod_dba_replicaset.h"
#include "modules/mod_shell.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"

namespace tests {

class Dba_replicaset_test: public Admin_api_test {
 protected:
  std::shared_ptr<mysqlshdk::db::ISession> create_base_session(
        int port) {
    mysqlshdk::db::Connection_options connection_options;

    connection_options.set_host("localhost");
    connection_options.set_port(port);
    connection_options.set_user("user");
    connection_options.set_password("");

    auto session = mysqlshdk::db::mysql::Session::create();

    session->connect(connection_options);

    return session;
  }

  // Creates a replicaset instance mock
  void init_test() {
    _base_session = create_local_session(_mysql_sandbox_nport1);
    std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
    metadata.reset(new mysqlsh::dba::MetadataStorage(_base_session));

    std::string topology_type =
        mysqlsh::dba::ReplicaSet::kTopologyPrimaryMaster;
    _replicaset.reset(new mysqlsh::dba::ReplicaSet("default",
                            topology_type, metadata));
    _replicaset->set_id(1);
  }

  std::shared_ptr<mysqlshdk::db::ISession> _base_session;
  std::shared_ptr<mysqlsh::dba::ReplicaSet> _replicaset;
  std::vector<testing::Fake_result_data> _queries;
};

// If the information on the Metadata and the GR group
// P_S info matches, the rescan result should be empty
TEST_F(Dba_replicaset_test, rescan_cluster_001) {
// get_instances_gr():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

// get_instances_md():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9

  std::vector<testing::Fake_result_data> queries;

  std::vector<std::vector<std::string>> values;
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  // statements required for get_newly_discovered_instances()
  add_ps_gr_group_members_query(&queries, values);
  add_md_group_members_query(&queries, values);

  // statements required for get_unavailable_instances()
  add_ps_gr_group_members_query(&queries, values);
  add_md_group_members_query(&queries, values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  init_test();

  shcore::Argument_list args;

  try {
    shcore::Value ret_val = _replicaset->rescan(args);
    auto result = ret_val.as_map();

    EXPECT_TRUE(result->has_key("name"));
    EXPECT_STREQ("default", result->get_string("name").c_str());

    EXPECT_TRUE(result->has_key("newlyDiscoveredInstances"));
    auto newly_discovered_instances =
        result->get_array("newlyDiscoveredInstances");
    EXPECT_TRUE(newly_discovered_instances->empty());

    EXPECT_TRUE(result->has_key("unavailableInstances"));
    auto unavailable_instances =
        result->get_array("unavailableInstances");
    EXPECT_TRUE(unavailable_instances->empty());
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
  }

  _base_session->close();
  stop_server_mock(_mysql_sandbox_nport1);
}

// If the information on the Metadata and the GR group
// P_S info is the same but in different order,
// the rescan result should be empty as well
//
// Regression test for BUG #25534693
TEST_F(Dba_replicaset_test, rescan_cluster_002) {
// get_instances_gr():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

// get_instances_md():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9

  std::vector<testing::Fake_result_data> queries;

  // statements required for get_newly_discovered_instances()
  std::vector<std::vector<std::string>> values;
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  // statements required for get_unavailable_instances()
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  init_test();

  shcore::Argument_list args;

  try {
    shcore::Value ret_val = _replicaset->rescan(args);
    auto result = ret_val.as_map();

    EXPECT_TRUE(result->has_key("name"));
    EXPECT_STREQ("default", result->get_string("name").c_str());

    EXPECT_TRUE(result->has_key("newlyDiscoveredInstances"));
    auto newly_discovered_instances =
        result->get_array("newlyDiscoveredInstances");
    EXPECT_TRUE(newly_discovered_instances->empty());

    EXPECT_TRUE(result->has_key("unavailableInstances"));
    auto unavailable_instances =
        result->get_array("unavailableInstances");
    EXPECT_TRUE(unavailable_instances->empty());
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
  }

  _base_session->close();
  stop_server_mock(_mysql_sandbox_nport1);
}

// If the GR group P_S info contains more instances than the ones
// as seen in Metadata, the rescan
// result should include those on the newlyDiscoveredInstances list
TEST_F(Dba_replicaset_test, rescan_cluster_003) {
// get_instances_gr():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

// get_instances_md():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

  std::vector<testing::Fake_result_data> queries;

  std::vector<std::vector<std::string>> values;
  // statements required for get_newly_discovered_instances()
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  values = {{"8a8ae9ce-5730-11e7-a437-b86b230042b9", "localhost", "3320"}};

  add_ps_gr_group_members_full_query(&queries,
      "8a8ae9ce-5730-11e7-a437-b86b230042b9", values);

  // statements required for get_unavailable_instances()
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  init_test();

  shcore::Argument_list args;

  try {
    shcore::Value ret_val = _replicaset->rescan(args);
    auto result = ret_val.as_map();

    EXPECT_TRUE(result->has_key("name"));
    EXPECT_STREQ("default", result->get_string("name").c_str());

    EXPECT_TRUE(result->has_key("newlyDiscoveredInstances"));
    auto newly_discovered_instances =
        result->get_array("newlyDiscoveredInstances");

    auto unknown_instances = result->get_array("newlyDiscoveredInstances");
    auto instance = unknown_instances.get()->at(0);
    auto instance_map = instance.as_map();

    EXPECT_STREQ("8a8ae9ce-5730-11e7-a437-b86b230042b9",
        instance_map->get_string("member_id").c_str());
    EXPECT_STREQ("localhost:3320", instance_map->get_string("name").c_str());
    EXPECT_STREQ("localhost:3320", instance_map->get_string("host").c_str());

    EXPECT_TRUE(result->has_key("unavailableInstances"));
    auto unavailable_instances =
        result->get_array("unavailableInstances");
    EXPECT_TRUE(unavailable_instances->empty());
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
  }

  _base_session->close();
  stop_server_mock(_mysql_sandbox_nport1);
}

// If the Metadata contains more instances than the ones
// as seen in GR group P_S info, the rescan
// result should include those on the unavailableInstances list
TEST_F(Dba_replicaset_test, rescan_cluster_004) {
// get_instances_gr():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

// get_instances_md():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9

  std::vector<testing::Fake_result_data> queries;
  std::vector<std::vector<std::string>> values;

  // statements required for get_newly_discovered_instances()
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  // statements required for get_unavailable_instances()
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  values = {{"8a8ae9ce-5730-11e7-a437-b86b230042b9", "localhost:3320",
              "localhost:3320"}};

  add_md_group_members_full_query(&queries,
      "8a8ae9ce-5730-11e7-a437-b86b230042b9", values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  init_test();

  shcore::Argument_list args;

  try {
    shcore::Value ret_val = _replicaset->rescan(args);
    auto result = ret_val.as_map();

    EXPECT_TRUE(result->has_key("name"));
    EXPECT_STREQ("default", result->get_string("name").c_str());

    EXPECT_TRUE(result->has_key("newlyDiscoveredInstances"));
    auto newly_discovered_instances =
        result->get_array("newlyDiscoveredInstances");
    EXPECT_TRUE(newly_discovered_instances->empty());

    auto unavailable_instances = result->get_array("unavailableInstances");
    auto instance = unavailable_instances.get()->at(0);
    auto instance_map = instance.as_map();

    EXPECT_STREQ("8a8ae9ce-5730-11e7-a437-b86b230042b9",
        instance_map->get_string("member_id").c_str());
    EXPECT_STREQ("localhost:3320", instance_map->get_string("label").c_str());
    EXPECT_STREQ("localhost:3320", instance_map->get_string("host").c_str());
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
  }

  _base_session->close();
  stop_server_mock(_mysql_sandbox_nport1);
}

}  // namespace tests
