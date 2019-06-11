/* Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms, as
   designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.
   This program is distributed in the hope that it will be useful,  but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "unittest/test_utils/admin_api_test.h"
#include <fstream>
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_json.h"
#include "utils/utils_string.h"

namespace tests {

// uint64_t Dba_replicaset_test::replicaset_id = 0;
std::shared_ptr<mysqlsh::dba::ReplicaSet> Admin_api_test::_replicaset = {};
std::string Admin_api_test::uuid_1 = "";
std::string Admin_api_test::uuid_2 = "";
std::string Admin_api_test::uuid_3 = "";
std::string Admin_api_test::group_name = "";

void Admin_api_test::SetUp() { Shell_core_test_wrapper::SetUp(); }

void Admin_api_test::SetUpSampleCluster(const char *context) {
  Shell_test_wrapper shell_env;
  shell_env.reset_replayable_shell(context);

  // Set report_host with the hostname for the deployed sandboxes.
  shcore::Dictionary_t sandbox_opts = shcore::make_dict();
  (*sandbox_opts)["report_host"] = shcore::Value(shell_env.hostname());

  shell_env.utils()->deploy_sandbox(shell_env.sb_port1(), "root", sandbox_opts);
  shell_env.utils()->deploy_sandbox(shell_env.sb_port2(), "root", sandbox_opts);
  shell_env.utils()->deploy_sandbox(shell_env.sb_port3(), "root", sandbox_opts);

  shell_env.execute(
      "shell.connect('root:root@localhost:" + shell_env.sb_str_port1() + "')");

  auto dba = shell_env.get_global<mysqlsh::dba::Dba>("dba");

  shcore::Argument_list args;
  shcore::Dictionary_t options = shcore::make_dict();
  options->emplace("memberSslMode", "REQUIRED");
  options->emplace("gtidSetIsComplete", true);
  auto cluster =
      dba->create_cluster("sample", options).as_object<mysqlsh::dba::Cluster>();

  cluster->add_instance("root:root@localhost:" + shell_env.sb_str_port2(), {});
  _replicaset = cluster->impl()->get_default_replicaset();

  shell_env.execute("session.close()");

  {
    auto session = create_session(shell_env.sb_port1());
    Instance instance(session);
    uuid_1 = *instance.get_sysvar_string("server_uuid", Var_qualifier::GLOBAL);
    group_name = *instance.get_sysvar_string("group_replication_group_name",
                                             Var_qualifier::GLOBAL);
    session->close();
  }

  {
    auto session = create_session(shell_env.sb_port2());
    Instance instance(session);

    uuid_2 = *instance.get_sysvar_string("server_uuid", Var_qualifier::GLOBAL);
    session->close();
  }

  {
    auto session = create_session(shell_env.sb_port3());
    Instance instance(session);
    uuid_3 = *instance.get_sysvar_string("server_uuid", Var_qualifier::GLOBAL);
    session->close();
  }
  shell_env.teardown_replayable_shell();
}

void Admin_api_test::TearDownSampleCluster(const char *context) {
  Shell_test_wrapper shell_env;
  shell_env.reset_replayable_shell(context);

  shell_env.utils()->destroy_sandbox(shell_env.sb_port1());
  shell_env.utils()->destroy_sandbox(shell_env.sb_port2());
  shell_env.utils()->destroy_sandbox(shell_env.sb_port3());
  shell_env.teardown_replayable_shell();
}

}  // namespace tests
