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
#include "modules/interactive_object_wrapper.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/admin_api_test.h"
#include "unittest/test_utils/mocks/modules/adminapi/mock_dba.h"

using mysqlshdk::utils::nullable;
namespace testing {

class Interactive_global_dba : public tests::Admin_api_test {
 protected:
  virtual void SetUp() {
    Admin_api_test::SetUp();

    // Resets the shell in non interactive mode
    _options->session_type = mysqlsh::SessionType::Classic;
    _options->uri = "mysql://user:@localhost:" + _mysql_sandbox_port1;
    _options->wizards = true;
    _output_tokens["sb_port_1"] = _mysql_sandbox_port1;
    reset_shell();
    _mock_started = false;
  }

  virtual void TearDown() {
    if (_mock_started)
      stop_server_mock(_mysql_sandbox_nport1);

    Admin_api_test::TearDown();
  }

  // Initializes the server mock, establishes the connection
  // and retrieves the global Dba object
  void init_mocks(bool start_mock_server) {
    if (start_mock_server) {
      start_server_mock(_mysql_sandbox_nport1, _queries);
      _mock_started = true;
    }

    _dba.initialize(_interactive_shell->shell_context().get(), false);

    auto global_dba = _interactive_shell->shell_context()->get_global("dba");

    auto interactive_dba =
        std::dynamic_pointer_cast<shcore::Interactive_object_wrapper>(
            global_dba.as_object());

    std::shared_ptr<Mock_dba> shared_dba(&_dba, SharedDoNotDelete());

    interactive_dba->set_target(
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(shared_dba));
  }

  // Using a strict mock will:
  // - Properly validate expected calls to mock functions
  // - Error out on unexpected calls to mock functions
  StrictMock<Mock_dba> _dba;
  std::vector<testing::Fake_result_data> _queries;
  bool _mock_started;
};

class Interactive_dba_create_cluster : public Interactive_global_dba {};

TEST_F(Interactive_dba_create_cluster, read_only_no_prompts) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is OFF, no active sessions
  add_super_read_only_queries(&_queries, false, false, {});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());

  // It must call the Non Interactive checkInstanceConfiguration, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_check_instance_configuration(_options->uri, "success");

  // It must call createCluster with no additional options since the interactive
  // layer did not require to resolve anything
  _dba.expect_create_cluster("dev", NO_MULTI_MASTER, NO_FORCE, NO_ADOPT_FROM_GR,
                             NO_CLEAN_READ_ONLY, true);

  _interactive_shell->process_line("var c = dba.createCluster('dev');");

  MY_EXPECT_MULTILINE_OUTPUT(
      "create_cluster_no_prompts",
      multiline({"A new InnoDB cluster will be created on instance "
                 "'user@localhost:<<<sb_port_1>>>'.",
                 "",
                 "Creating InnoDB cluster 'dev' on "
                 "'user@localhost:<<<sb_port_1>>>'...",
                 "Adding Seed Instance...", "",
                 "Cluster successfully created. Use Cluster.addInstance() to "
                 "add MySQL instances.",
                 "At least 3 instances are needed for the cluster to be able "
                 "to withstand up to",
                 "one server failure."}),
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_create_cluster, read_only_no_flag_prompt_yes) {
  // Running on a standalone instance
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // It must call the Non Interactive checkInstanceConfiguration, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_check_instance_configuration(_options->uri, "success");

  // It must call createCluster with the resolved flag for clean read only set
  // to true
  _dba.expect_create_cluster("dev", NO_MULTI_MASTER, NO_FORCE, NO_ADOPT_FROM_GR,
                             CLEAN_READ_ONLY_TRUE, true);

  // The answer to the prompt about continuing cleaning up the read only
  output_handler.prompts.push_back("y");

  _interactive_shell->process_line("var c = dba.createCluster('dev');");

  MY_EXPECT_MULTILINE_OUTPUT(
      "create_cluster_ro_server_no_clean_ro_flag_prompt_yes",
      multiline(
          {"A new InnoDB cluster will be created on instance "
           "'user@localhost:<<<sb_port_1>>>'.",
           "",
           "The MySQL instance at 'localhost:<<<sb_port_1>>>' currently has "
           "the super_read_only ",
           "system variable set to protect it from inadvertent updates from "
           "applications.",
           "You must first unset it to be able to perform any changes to this "
           "instance.",
           "For more information see: "
           "https://dev.mysql.com/doc/refman/en/"
           "server-system-variables.html#sysvar_super_read_only.",
           "", "Note: there are open sessions to 'localhost:<<<sb_port_1>>>'.",
           "You may want to kill these sessions to prevent them from "
           "performing unexpected updates:",
           "", "1 open session(s) of 'root@localhost'.", "",
           "Do you want to disable super_read_only and continue? [y|N]:"}),
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_create_cluster, read_only_no_flag_prompt_no) {
  // Running on a standalone instance
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // It must call the Non Interactive checkInstanceConfiguration, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_check_instance_configuration(_options->uri, "success");

  // The answer to the prompt about continuing cleaning up the read only
  output_handler.prompts.push_back("n");

  // Since no expectation is set for create_cluster, a call to it would raise
  // an exception
  _interactive_shell->process_line("var c = dba.createCluster('dev');");

  MY_EXPECT_MULTILINE_OUTPUT(
      "create_cluster_ro_server_no_clean_ro_flag_prompt_no",
      multiline({"A new InnoDB cluster will be created on instance "
                 "'user@localhost:<<<sb_port_1>>>'.",
                 "",
                 "The MySQL instance at 'localhost:<<<sb_port_1>>>' currently "
                 "has the super_read_only ",
                 "system variable set to protect it from inadvertent updates "
                 "from applications.",
                 "You must first unset it to be able to perform any changes to "
                 "this instance.",
                 "For more information see: "
                 "https://dev.mysql.com/doc/refman/en/"
                 "server-system-variables.html#sysvar_super_read_only.",
                 "",
                 "Do you want to disable super_read_only and continue? [y|N]:",
                 "Cancelled"}),
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_create_cluster, read_only_invalid_flag_value) {
  // Running on a standalone instance
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // Since no expectation is set for create_cluster, a call to it would raise
  // an exception
  _interactive_shell->process_line(
      "var c = dba.createCluster('dev', {clearReadOnly: 'NotABool'});");

  MY_EXPECT_OUTPUT_CONTAINS(
      "Dba.createCluster: Argument 'clearReadOnly' is expected to be a bool",
      output_handler.std_err);

  output_handler.wipe_all();

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_create_cluster, read_only_flag_true) {
  // Running on a standalone instance
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // It must call the Non Interactive checkInstanceConfiguration, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_check_instance_configuration(_options->uri, "success");

  // It must call createCluster with the resolved flag for clean read only set
  // to true
  _dba.expect_create_cluster("dev", NO_MULTI_MASTER, NO_FORCE, NO_ADOPT_FROM_GR,
                             CLEAN_READ_ONLY_TRUE, true);

  _interactive_shell->process_line(
      "var c = dba.createCluster('dev', {clearReadOnly: true});");

  MY_EXPECT_MULTILINE_OUTPUT(
      "create_cluster_ro_server_clean_ro_flag_true",
      multiline({"A new InnoDB cluster will be created on instance "
                 "'user@localhost:<<<sb_port_1>>>'.",
                 "",
                 "Creating InnoDB cluster 'dev' on "
                 "'user@localhost:<<<sb_port_1>>>'...",
                 "Adding Seed Instance...", "",
                 "Cluster successfully created. Use Cluster.addInstance() to "
                 "add MySQL instances.",
                 "At least 3 instances are needed for the cluster to be able "
                 "to withstand up to",
                 "one server failure."}),
      output_handler.std_out);

  MY_EXPECT_OUTPUT_NOT_CONTAINS(
      "The MySQL instance at 'localhost:<<<sb_port_1>>>' currently has the "
      "super_read_only",
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_create_cluster, read_only_flag_false) {
  // Running on a standalone instance
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // It must call the Non Interactive checkInstanceConfiguration, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_check_instance_configuration(_options->uri, "success");

  // It must call createCluster with the resolved flag for clean read only set
  // to true
  _dba.expect_create_cluster("dev", NO_MULTI_MASTER, NO_FORCE, NO_ADOPT_FROM_GR,
                             CLEAN_READ_ONLY_FALSE, false);

  _interactive_shell->process_line(
      "var c = dba.createCluster('dev', {clearReadOnly: false});");

  MY_EXPECT_MULTILINE_OUTPUT("create_cluster_ro_server_clean_ro_flag_false",
                             multiline({
                                 "A new InnoDB cluster will be created on "
                                 "instance 'user@localhost:<<<sb_port_1>>>'.",
                                 "",
                                 "Creating InnoDB cluster 'dev' on "
                                 "'user@localhost:<<<sb_port_1>>>'...",
                             }),
                             output_handler.std_out);

  MY_EXPECT_OUTPUT_NOT_CONTAINS("Adding Seed Instance...",
                                output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

//---- Configure Local Instance Tests

class Interactive_dba_configure_local_instance : public Interactive_global_dba {
};

TEST_F(Interactive_dba_configure_local_instance, read_only_no_prompts) {
  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, false, false, {});

  init_mocks(true);

  // It must call the Non Interactive configureLocalInstance, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_configure_local_instance(
      _options->uri, std::string("/path/to/file.cnf"),
      std::string("'root'@'%'"), std::string("pwd"), NO_CLEAN_READ_ONLY, true);

  _interactive_shell->process_line(
      "dba.configureLocalInstance('" + _options->uri +
      "', {mycnfPath:'/path/to/file.cnf', clusterAdmin:'root', "
      "clusterAdminPassword:'pwd'});");

  MY_EXPECT_MULTILINE_OUTPUT(
      "configure_local_instance_no_prompts",
      multiline({"Validating instance...", "",
                 "The instance 'localhost:<<<sb_port_1>>>' is valid for "
                 "Cluster usage",
                 "You can now use it in an InnoDB Cluster."}),
      output_handler.std_out);
}

TEST_F(Interactive_dba_configure_local_instance, read_only_no_flag_prompt_yes) {
  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  // It must call the Non Interactive configureLocalInstance, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_configure_local_instance(
      _options->uri, std::string("/path/to/file.cnf"),
      std::string("'root'@'%'"), std::string("pwd"), CLEAN_READ_ONLY_TRUE,
      true);

  // The answer to the prompt about continuing cleaning up the read only
  output_handler.prompts.push_back("y");

  _interactive_shell->process_line(
      "dba.configureLocalInstance('" + _options->uri +
      "', {mycnfPath:'/path/to/file.cnf', clusterAdmin:'root', "
      "clusterAdminPassword:'pwd'});");

  _interactive_shell->process_line(
      "var c = dba.configureLocalInstance('root@localhost" +
      _mysql_sandbox_port1 + "');");

  MY_EXPECT_MULTILINE_OUTPUT(
      "configure_local_instance_ro_server_no_clean_ro_flag_prompt_yes",
      multiline(
          {"The MySQL instance at 'localhost:<<<sb_port_1>>>' currently has "
           "the super_read_only ",
           "system variable set to protect it from inadvertent updates from "
           "applications.",
           "You must first unset it to be able to perform any changes to this "
           "instance.",
           "For more information see: "
           "https://dev.mysql.com/doc/refman/en/"
           "server-system-variables.html#sysvar_super_read_only.",
           "", "Note: there are open sessions to 'localhost:<<<sb_port_1>>>'.",
           "You may want to kill these sessions to prevent them from "
           "performing unexpected updates:",
           "", "1 open session(s) of 'root@localhost'.", "",
           "Do you want to disable super_read_only and continue? [y|N]:",
           "Validating instance...", "",
           "The instance 'localhost:<<<sb_port_1>>>' is valid for Cluster "
           "usage",
           "You can now use it in an InnoDB Cluster."}),
      output_handler.std_out);
}

TEST_F(Interactive_dba_configure_local_instance, read_only_no_flag_prompt_no) {
  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {});

  init_mocks(true);

  // The answer to the prompt about continuing cleaning up the read only
  output_handler.prompts.push_back("n");

  _interactive_shell->process_line(
      "dba.configureLocalInstance('" + _options->uri +
      "', {mycnfPath:'/path/to/file.cnf', clusterAdmin:'root', "
      "clusterAdminPassword:'pwd'});");

  MY_EXPECT_MULTILINE_OUTPUT(
      "configure_local_instance_ro_server_no_clean_ro_flag_prompt_no",
      multiline({"The MySQL instance at 'localhost:<<<sb_port_1>>>' currently "
                 "has the super_read_only ",
                 "system variable set to protect it from inadvertent updates "
                 "from applications.",
                 "You must first unset it to be able to perform any changes to "
                 "this instance.",
                 "For more information see: "
                 "https://dev.mysql.com/doc/refman/en/"
                 "server-system-variables.html#sysvar_super_read_only.",
                 "",
                 "Do you want to disable super_read_only and continue? [y|N]:",
                 "Cancelled"}),
      output_handler.std_out);
}

TEST_F(Interactive_dba_configure_local_instance, read_only_invalid_flag_value) {
  // Since no expectation is set for create_cluster, a call to it would raise
  // an exception
  _interactive_shell->process_line(
      "dba.configureLocalInstance('" + _options->uri +
      "', {mycnfPath:'/path/to/file.cnf', clusterAdmin:'root', "
      "clusterAdminPassword:'pwd', clearReadOnly: 'NotABool'});");

  MY_EXPECT_OUTPUT_CONTAINS(
      "Dba.configureLocalInstance: Argument 'clearReadOnly' is expected to be "
      "a bool",
      output_handler.std_err);

  output_handler.wipe_all();
}

TEST_F(Interactive_dba_configure_local_instance, read_only_flag_true) {
  init_mocks(false);

  // It must call the Non Interactive configureLocalInstance, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_configure_local_instance(
      _options->uri, std::string("/path/to/file.cnf"),
      std::string("'root'@'%'"), std::string("pwd"), CLEAN_READ_ONLY_TRUE,
      true);

  _interactive_shell->process_line(
      "dba.configureLocalInstance('" + _options->uri +
      "', {mycnfPath:'/path/to/file.cnf', clusterAdmin:'root', "
      "clusterAdminPassword:'pwd', clearReadOnly: true});");

  MY_EXPECT_MULTILINE_OUTPUT(
      "configure_local_instance_ro_server_clean_ro_flag_true",
      multiline({"Validating instance...", "",
                 "The instance 'localhost:<<<sb_port_1>>>' is valid for "
                 "Cluster usage",
                 "You can now use it in an InnoDB Cluster."}),
      output_handler.std_out);

  MY_EXPECT_OUTPUT_NOT_CONTAINS(
      "The MySQL instance at 'localhost:<<<sb_port_1>>>' currently has the "
      "super_read_only",
      output_handler.std_out);
}

TEST_F(Interactive_dba_configure_local_instance, read_only_flag_false) {
  init_mocks(false);

  // It must call the Non Interactive configureLocalInstance, the non
  // interactive API will fail because the instance is read only
  _dba.expect_configure_local_instance(
      _options->uri, std::string("/path/to/file.cnf"),
      std::string("'root'@'%'"), std::string("pwd"), CLEAN_READ_ONLY_FALSE,
      false);

  _interactive_shell->process_line(
      "dba.configureLocalInstance('" + _options->uri +
      "', {mycnfPath:'/path/to/file.cnf', clusterAdmin:'root', "
      "clusterAdminPassword:'pwd', clearReadOnly: false});");

  MY_EXPECT_OUTPUT_CONTAINS("Validating instance...", output_handler.std_out);

  MY_EXPECT_OUTPUT_NOT_CONTAINS(
      "The instance 'localhost:<<<sb_port_1>>>' is valid for Cluster usage",
      output_handler.std_out);
}

//---- Drop Metadata Schema Tests

class Interactive_dba_drop_metadata_schema : public Interactive_global_dba {};

TEST_F(Interactive_dba_drop_metadata_schema, read_only_no_prompts) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);

  // super_read_only is OFF, no active sessions
  add_super_read_only_queries(&_queries, false, false, {});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // It must call the Non Interactive dropMetadataSchema, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_drop_metadata_schema(FORCE_TRUE, NO_CLEAN_READ_ONLY, true);

  _interactive_shell->process_line(
      "var c = dba.dropMetadataSchema({force: true});");

  MY_EXPECT_OUTPUT_CONTAINS("Metadata Schema successfully removed.",
                            output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_drop_metadata_schema, read_only_no_flag_prompt_yes) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // It must call the Non Interactive checkInstanceConfiguration, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_drop_metadata_schema(FORCE_TRUE, CLEAN_READ_ONLY_TRUE, true);

  // The answer to the prompt about continuing cleaning up the read only
  output_handler.prompts.push_back("y");

  _interactive_shell->process_line(
      "var c = dba.dropMetadataSchema({force: true});");

  MY_EXPECT_MULTILINE_OUTPUT(
      "create_cluster_ro_server_no_clean_ro_flag_prompt_yes",
      multiline(
          {"The MySQL instance at 'localhost:<<<sb_port_1>>>' currently has "
           "the super_read_only ",
           "system variable set to protect it from inadvertent updates from "
           "applications.",
           "You must first unset it to be able to perform any changes to this "
           "instance.",
           "For more information see: "
           "https://dev.mysql.com/doc/refman/en/"
           "server-system-variables.html#sysvar_super_read_only.",
           "", "Note: there are open sessions to 'localhost:<<<sb_port_1>>>'.",
           "You may want to kill these sessions to prevent them from "
           "performing unexpected updates:",
           "", "1 open session(s) of 'root@localhost'.", "",
           "Do you want to disable super_read_only and continue? [y|N]:"}),
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_drop_metadata_schema, read_only_no_flag_prompt_no) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // The answer to the prompt about continuing cleaning up the read only
  output_handler.prompts.push_back("n");

  // Since no expectation is set for create_cluster, a call to it would raise
  // an exception
  _interactive_shell->process_line(
      "var c = dba.dropMetadataSchema({force: true});");

  MY_EXPECT_MULTILINE_OUTPUT(
      "create_cluster_ro_server_no_clean_ro_flag_prompt_no",
      multiline({"The MySQL instance at 'localhost:<<<sb_port_1>>>' currently "
                 "has the super_read_only ",
                 "system variable set to protect it from inadvertent updates "
                 "from applications.",
                 "You must first unset it to be able to perform any changes to "
                 "this instance.",
                 "For more information see: "
                 "https://dev.mysql.com/doc/refman/en/"
                 "server-system-variables.html#sysvar_super_read_only.",
                 "",
                 "Do you want to disable super_read_only and continue? [y|N]:",
                 "Cancelled"}),
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_drop_metadata_schema, read_only_invalid_flag_value) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // Since no expectation is set for create_cluster, a call to it would raise
  // an exception
  _interactive_shell->process_line(
      "var c = dba.dropMetadataSchema({force: true, clearReadOnly: "
      "'NotABool'});");

  MY_EXPECT_OUTPUT_CONTAINS(
      "Dba.dropMetadataSchema: Argument 'clearReadOnly' is expected to be a "
      "bool",
      output_handler.std_err);

  output_handler.wipe_all();

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_drop_metadata_schema, read_only_flag_true) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // It must call the Non Interactive checkInstanceConfiguration, we choose to
  // Return success since the path we are covering is abouy super_read_only
  _dba.expect_drop_metadata_schema(FORCE_TRUE, CLEAN_READ_ONLY_TRUE, true);

  _interactive_shell->process_line(
      "var c = dba.dropMetadataSchema({force: true, clearReadOnly: true});");

  MY_EXPECT_OUTPUT_CONTAINS("Metadata Schema successfully removed.",
                            output_handler.std_out);

  MY_EXPECT_OUTPUT_NOT_CONTAINS(
      "The MySQL instance at 'localhost:<<<sb_port_1>>>' currently has the "
      "super_read_only",
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_drop_metadata_schema, read_only_flag_false) {
  // Running on a standalone instance
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // It must call the Non Interactive checkInstanceConfiguration,
  // Non interactive API will fail since the instance is read only
  _dba.expect_drop_metadata_schema(FORCE_TRUE, CLEAN_READ_ONLY_FALSE, false);

  _interactive_shell->process_line(
      "var c = dba.dropMetadataSchema({force: true, clearReadOnly: false});");

  MY_EXPECT_OUTPUT_NOT_CONTAINS("Metadata Schema successfully removed.",
                                output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

//---- Drop Metadata Schema Tests

class Interactive_dba_reboot_cluster : public Interactive_global_dba {};

TEST_F(Interactive_dba_reboot_cluster, read_only_no_prompts) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is OFF, no active sessions
  add_super_read_only_queries(&_queries, false, false, {});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  EXPECT_CALL(_dba, validate_instances_status_reboot_cluster(_));
  std::vector<std::pair<std::string, std::string>> status;
  EXPECT_CALL(_dba, get_replicaset_instances_status(_, _))
      .WillOnce(Return(status));

  // It must call the Non Interactive rebootClusterFromCompleteOutage, we choose
  // to Return success since the path we are covering is abouy super_read_only
  _dba.expect_reboot_cluster(std::string("dev"), NO_REJOIN_LIST, NO_REMOVE_LIST,
                             NO_CLEAN_READ_ONLY, true);

  _interactive_shell->process_line(
      "var c = dba.rebootClusterFromCompleteOutage('dev');");

  MY_EXPECT_MULTILINE_OUTPUT(
      "Interactive_dba_reboot_cluster, read_only_no_prompts",
      multiline({"Reconfiguring the cluster 'dev' from complete outage...", "",
                 "", "The cluster was successfully rebooted."}),
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_reboot_cluster, read_only_no_flag_prompt_yes) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  EXPECT_CALL(_dba, validate_instances_status_reboot_cluster(_));
  std::vector<std::pair<std::string, std::string>> status;
  EXPECT_CALL(_dba, get_replicaset_instances_status(_, _))
      .WillOnce(Return(status));

  // It must call the Non Interactive rebootClusterFromCompleteOutage, we choose
  // to Return success since the path we are covering is abouy super_read_only
  _dba.expect_reboot_cluster(std::string("dev"), NO_REJOIN_LIST, NO_REMOVE_LIST,
                             CLEAN_READ_ONLY_TRUE, true);

  // The answer to the prompt about continuing cleaning up the read only
  output_handler.prompts.push_back("y");

  _interactive_shell->process_line(
      "var c = dba.rebootClusterFromCompleteOutage('dev');");

  MY_EXPECT_MULTILINE_OUTPUT(
      "create_cluster_ro_server_no_clean_ro_flag_prompt_yes",
      multiline(
          {"Reconfiguring the cluster 'dev' from complete outage...", "",
           "The MySQL instance at 'localhost:<<<sb_port_1>>>' currently has "
           "the super_read_only ",
           "system variable set to protect it from inadvertent updates from "
           "applications.",
           "You must first unset it to be able to perform any changes to this "
           "instance.",
           "For more information see: "
           "https://dev.mysql.com/doc/refman/en/"
           "server-system-variables.html#sysvar_super_read_only.",
           "", "Note: there are open sessions to 'localhost:<<<sb_port_1>>>'.",
           "You may want to kill these sessions to prevent them from "
           "performing unexpected updates:",
           "", "1 open session(s) of 'root@localhost'.", "",
           "Do you want to disable super_read_only and continue? [y|N]:", "",
           "The cluster was successfully rebooted."}),
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_reboot_cluster, read_only_no_flag_prompt_no) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  EXPECT_CALL(_dba, validate_instances_status_reboot_cluster(_));
  std::vector<std::pair<std::string, std::string>> status;
  EXPECT_CALL(_dba, get_replicaset_instances_status(_, _))
      .WillOnce(Return(status));

  // The answer to the prompt about continuing cleaning up the read only
  output_handler.prompts.push_back("n");

  // Since no expectation is set for create_cluster, a call to it would raise
  // an exception
  _interactive_shell->process_line(
      "var c = dba.rebootClusterFromCompleteOutage('dev');");

  MY_EXPECT_MULTILINE_OUTPUT(
      "create_cluster_ro_server_no_clean_ro_flag_prompt_no",
      multiline({"Reconfiguring the cluster 'dev' from complete outage...", "",
                 "The MySQL instance at 'localhost:<<<sb_port_1>>>' currently "
                 "has the super_read_only ",
                 "system variable set to protect it from inadvertent updates "
                 "from applications.",
                 "You must first unset it to be able to perform any changes to "
                 "this instance.",
                 "For more information see: "
                 "https://dev.mysql.com/doc/refman/en/"
                 "server-system-variables.html#sysvar_super_read_only.",
                 "",
                 "Do you want to disable super_read_only and continue? [y|N]:",
                 "Cancelled"}),
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_reboot_cluster, read_only_invalid_flag_value) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  // Since no expectation is set for create_cluster, a call to it would raise
  // an exception
  _interactive_shell->process_line(
      "var c = dba.rebootClusterFromCompleteOutage('dev', {clearReadOnly: "
      "'NotABool'});");

  MY_EXPECT_OUTPUT_CONTAINS(
      "Dba.rebootClusterFromCompleteOutage: Argument 'clearReadOnly' is "
      "expected to be a bool",
      output_handler.std_err);

  output_handler.wipe_all();

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_reboot_cluster, read_only_flag_true) {
  // Sets the required statements for the session
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  EXPECT_CALL(_dba, validate_instances_status_reboot_cluster(_));
  std::vector<std::pair<std::string, std::string>> status;
  EXPECT_CALL(_dba, get_replicaset_instances_status(_, _))
      .WillOnce(Return(status));

  // It must call the Non Interactive rebootClusterFromCompleteOutage, we choose
  // to Return success since the path we are covering is abouy super_read_only
  _dba.expect_reboot_cluster(std::string("dev"), NO_REJOIN_LIST, NO_REMOVE_LIST,
                             CLEAN_READ_ONLY_TRUE, true);

  _interactive_shell->process_line(
      "var c = dba.rebootClusterFromCompleteOutage('dev', {clearReadOnly: "
      "true});");

  MY_EXPECT_MULTILINE_OUTPUT(
      "create_cluster_ro_server_no_clean_ro_flag_prompt_no",
      multiline({"Reconfiguring the cluster 'dev' from complete outage...", "",
                 "", "The cluster was successfully rebooted."}),
      output_handler.std_out);

  MY_EXPECT_OUTPUT_NOT_CONTAINS(
      "The MySQL instance at 'localhost:<<<sb_port_1>>>' currently has the "
      "super_read_only",
      output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Interactive_dba_reboot_cluster, read_only_flag_false) {
  // Running on a standalone instance
  add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});

  // super_read_only is ON, no active sessions
  add_super_read_only_queries(&_queries, true, true, {{"root@localhost", "1"}});

  init_mocks(true);

  _interactive_shell->connect(_options->connection_options());


  EXPECT_CALL(_dba, validate_instances_status_reboot_cluster(_));
  std::vector<std::pair<std::string, std::string>> status;
  EXPECT_CALL(_dba, get_replicaset_instances_status(_, _))
      .WillOnce(Return(status));

  // It must call the Non Interactive rebootClusterFromCompleteOutage, non
  // interactive API will fail because the instance is read only
  _dba.expect_reboot_cluster(std::string("dev"), NO_REJOIN_LIST, NO_REMOVE_LIST,
                             CLEAN_READ_ONLY_FALSE, false);

  _interactive_shell->process_line(
      "var c = dba.rebootClusterFromCompleteOutage('dev', {clearReadOnly: "
      "false});");

  MY_EXPECT_OUTPUT_CONTAINS(
      "Reconfiguring the cluster 'dev' from complete outage...",
      output_handler.std_out);

  MY_EXPECT_OUTPUT_NOT_CONTAINS("The cluster was successfully rebooted.",
                                output_handler.std_out);

  _interactive_shell->shell_context()->get_dev_session()->close();
}
}  // namespace testing
