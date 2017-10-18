/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef UNITTEST_TEST_UTILS_ADMIN_API_TEST_H_
#define UNITTEST_TEST_UTILS_ADMIN_API_TEST_H_

#include <memory>
#include <string>
#include <vector>
#include "modules/adminapi/mod_dba_common.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "unittest/test_utils.h"
#include "mysqlshdk/libs/db/mysql/session.h"

using mysqlshdk::utils::nullable;

namespace tests {
class Admin_api_test : public Shell_core_test_wrapper {
 public:
  virtual void SetUp();

  void add_instance_type_queries(std::vector<testing::Fake_result_data> *data,
                                 mysqlsh::dba::GRInstanceType type);
  void add_get_server_variable_query(
      std::vector<testing::Fake_result_data> *data, const std::string &variable,
      mysqlshdk::db::Type type, const std::string &value);
  void add_set_global_variable_query(
      std::vector<testing::Fake_result_data> *data, const std::string &variable,
      const std::string &value);
  void add_show_databases_query(std::vector<testing::Fake_result_data> *data,
                                const std::string &variable,
                                const std::string &value);
  void add_replication_filters_query(
      std::vector<testing::Fake_result_data> *data,
      const std::string &binlog_do_db, const std::string &binlog_ignore_db);
  void add_ps_gr_group_members_query(
      std::vector<testing::Fake_result_data> *data,
      const std::vector<std::vector<std::string>> &values);
  void add_ps_gr_group_members_full_query(
      std::vector<testing::Fake_result_data> *data,
      const std::string &member_id,
      const std::vector<std::vector<std::string>> &values);
  void add_md_group_members_query(
      std::vector<testing::Fake_result_data> *data,
      const std::vector<std::vector<std::string>> &values);
  void add_md_group_members_full_query(
      std::vector<testing::Fake_result_data> *data,
      const std::string &mysql_server_uuid,
      const std::vector<std::vector<std::string>> &values);
  void add_gr_primary_member_query(std::vector<testing::Fake_result_data> *data,
                                   const std::string &primary_uuid);
  void add_member_state_query(std::vector<testing::Fake_result_data> *data,
                              const std::string &address,
                              const std::string &mysql_server_uuid,
                              const std::string &instance_name,
                              const std::string &member_state);
  void add_md_group_name_query(std::vector<testing::Fake_result_data> *data,
                               const std::string &value);
  void add_get_cluster_matching_query(
      std::vector<testing::Fake_result_data> *data,
      const std::string &cluster_name);
  void add_get_replicaset_query(std::vector<testing::Fake_result_data> *data,
                                const std::string &replicaset_name);
  void add_is_instance_on_rs_query(std::vector<testing::Fake_result_data> *data,
                                   const std::string &replicast_id,
                                   const std::string &instance_address);

  void add_precondition_queries(std::vector<testing::Fake_result_data> *data,
                                mysqlsh::dba::GRInstanceType instance_type,
                                nullable<std::string> primary_uuid,
                                nullable<std::string> instance_uuid = {},
                                nullable<std::string> instance_state = {},
                                nullable<int> instance_count = {},
                                nullable<int> reachable_instances = {});

  void add_super_read_only_queries(
      std::vector<testing::Fake_result_data> *data, bool super_read_only,
      bool query_open_sessions,
      std::vector<std::vector<std::string>> open_sessions);

  void add_is_gtid_subset_query(std::vector<testing::Fake_result_data> *data,
                                const std::string &start,
                                const std::string &end, bool success);

void add_validate_cluster_admin_user_privileges_queries(
                                std::vector<testing::Fake_result_data> *data,
                                const std::string& user,
                                const std::string& host,
                                const std::string& non_grantable = "",
                                const std::string& missing = "");
 public:
  std::shared_ptr<mysqlshdk::db::ISession> get_classic_session() {
    return _interactive_shell->shell_context()->get_dev_session()->get_core_session();
  }

  std::shared_ptr<mysqlshdk::db::ISession> create_local_session(
      int port) {

    mysqlshdk::db::Connection_options session_args;
    session_args.set_scheme("mysql");
    session_args.set_host("localhost");
    session_args.set_port(port);
    session_args.set_user("user");
    session_args.set_password("");

    auto session = mysqlshdk::db::mysql::Session::create();
    session->connect(session_args);
    return session;
  }
};
}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_ADMIN_API_TEST_H_
