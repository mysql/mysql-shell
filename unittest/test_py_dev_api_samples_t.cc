/* Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.

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

#include "shell_script_tester.h"
#include "utils/utils_general.h"

namespace shcore {
class Shell_py_dev_api_sample_tester : public Shell_py_script_tester {
 protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    Shell_py_script_tester::SetUp();

    std::string user, host, password;
    auto connection_options = shcore::get_connection_options(_uri);

    if (connection_options.has_user()) user = connection_options.get_user();

    if (connection_options.has_host()) host = connection_options.get_host();

    if (connection_options.has_password())
      password = connection_options.get_password();

    if (_port.empty()) _port = "33060";

    std::string code = "__uripwd = '" + user + ":" + password + "@" + host +
                       ":" + _port + "';";
    exec_and_out_equals(code);

    set_config_folder("py_dev_api_examples");
    set_setup_script("setup.py");

    _extension = "py";
    _new_format = true;

    // Some tests will define global vars, which will affect other tests because
    // of the global Python interpreter context. So we clean them up.
    // snapshot globals
    execute("__global_names = set(globals().keys())");
  }

  void TearDown() {
    // cleanup globals / restore snapshot
    execute(
        "for k in set(globals().keys()) - __global_names:\n"
        "    del globals()[k]\n");

    Shell_py_script_tester::TearDown();
  }

  virtual void pre_process_line(const std::string &path, std::string &line) {
    // Unit tests work using default ports, if that is not the case
    // We need to update them before being executed
    if (!_port.empty() && _port != "33060") {
      // Some examples contain a hardcoded port
      // we need to change that port by the right one
      size_t pos = line.find("33060");
      if (pos != std::string::npos) {
        std::string new_line = line.substr(0, pos);
        new_line += _port;
        new_line += line.substr(pos + 5);
        line = new_line;
      }

      // Other examples use an URI assumin the defaut port
      // If that's not the case, we need to add it to the URI
      pos = line.find("'mike:paSSw0rd@localhost'");
      if (pos != std::string::npos) {
        std::string new_line = line.substr(0, pos);
        new_line += "'mike:paSSw0rd@localhost:";
        new_line += _port;
        new_line += "');";
        line = new_line;
      }
    }
  }
};

//==================>>> building_expressions
TEST_F(Shell_py_dev_api_sample_tester, Expression_Strings) {
  validate_interactive("building_expressions/Expression_Strings");
}

//==================>>> concepts
TEST_F(Shell_py_dev_api_sample_tester, Connecting_to_a_Single_MySQL_Server) {
  validate_interactive("concepts/Connecting_to_a_Single_MySQL_Server");
}

TEST_F(Shell_py_dev_api_sample_tester, Connecting_to_a_Single_MySQL_Server_1) {
  output_handler.prompts.push_back({"*", "mike"});
  output_handler.passwords.push_back({"*", "paSSw0rd"});

  validate_interactive("concepts/Connecting_to_a_Single_MySQL_Server_1");
}

TEST_F(Shell_py_dev_api_sample_tester, Database_Connection_Example) {
  validate_interactive("concepts/Database_Connection_Example");
}

TEST_F(Shell_py_dev_api_sample_tester, Dynamic_SQL) {
  validate_interactive("concepts/Dynamic_SQL");
}

TEST_F(Shell_py_dev_api_sample_tester, Setting_the_Current_Schema) {
  validate_interactive("concepts/Setting_the_Current_Schema");
}

TEST_F(Shell_py_dev_api_sample_tester, Using_SQL_with_Session) {
  validate_interactive("concepts/Using_SQL_with_Session");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_a_Session_Object) {
  validate_interactive("concepts/Working_with_a_Session_Object");
}

//==================>>> crud_operations
TEST_F(Shell_py_dev_api_sample_tester, Method_Chaining) {
  validate_interactive("crud_operations/Method_Chaining");
}

TEST_F(Shell_py_dev_api_sample_tester, Parameter_Binding) {
  validate_interactive("crud_operations/Parameter_Binding");
}

TEST_F(Shell_py_dev_api_sample_tester, Parameter_Binding_1) {
  validate_interactive("crud_operations/Parameter_Binding_1");
}

TEST_F(Shell_py_dev_api_sample_tester, Preparing_CRUD_Statements) {
  validate_interactive("crud_operations/Preparing_CRUD_Statements");
}

//==================>>> results
TEST_F(Shell_py_dev_api_sample_tester, Fetching_All_Data_Items_at_Once) {
  validate_interactive("results/Fetching_All_Data_Items_at_Once");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_Data_Sets) {
  validate_interactive("results/Working_with_Data_Sets");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_Data_Sets_1) {
  validate_interactive("results/Working_with_Data_Sets_1");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_Results) {
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 5))
    SKIP_TEST("DevAPI samples require the lastest version of the DevAPI");
  validate_interactive("results/Working_with_Results");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_Results_1) {
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 5))
    SKIP_TEST("DevAPI samples require the lastest version of the DevAPI");
  validate_interactive("results/Working_with_Results_1");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_SQL_Result_Sets) {
  validate_interactive("results/Working_with_SQL_Result_Sets");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_SQL_Result_Sets_1) {
  validate_interactive("results/Working_with_SQL_Result_Sets_1");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_SQL_Result_Sets_2) {
  validate_interactive("results/Working_with_SQL_Result_Sets_2");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_SQL_Result_Sets_3) {
  validate_interactive("results/Working_with_SQL_Result_Sets_3");
}

//==================>>> statement_execution
// This specific example does not work for python 2.6 so we disable it for it
// (OEL6)
#if PY_MAJOR_VERSION > 2 || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION > 6)

TEST_F(Shell_py_dev_api_sample_tester, Error_Handling) {
  validate_interactive("statement_execution/Error_Handling");
}
#endif

TEST_F(Shell_py_dev_api_sample_tester, Processing_Warnings) {
  validate_interactive("statement_execution/Processing_Warnings");
}

TEST_F(Shell_py_dev_api_sample_tester, Processing_Warnings_1) {
  validate_interactive("statement_execution/Processing_Warnings_1");
}

TEST_F(Shell_py_dev_api_sample_tester, Transaction_Handling) {
  validate_interactive("statement_execution/Transaction_Handling");
}

//==================>>> working_with_collections
TEST_F(Shell_py_dev_api_sample_tester, Basic_CRUD_Operations_on_Collections) {
  validate_interactive(
      "working_with_collections/Basic_CRUD_Operations_on_Collections");
}

TEST_F(Shell_py_dev_api_sample_tester, Collection_add) {
  validate_interactive("working_with_collections/Collection_add");
}

TEST_F(Shell_py_dev_api_sample_tester, Collection_find) {
  validate_interactive("working_with_collections/Collection_find");
}

TEST_F(Shell_py_dev_api_sample_tester, Creating_a_Collection) {
  validate_interactive("working_with_collections/Creating_a_Collection");
}

TEST_F(Shell_py_dev_api_sample_tester, Document_Identity) {
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 5))
    SKIP_TEST("DevAPI samples require the lastest version of the DevAPI");
  validate_interactive("working_with_collections/Document_Identity");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_Existing_Collections) {
  validate_interactive(
      "working_with_collections/Working_with_Existing_Collections");
}

//==================>>> working_with_documents
TEST_F(Shell_py_dev_api_sample_tester, Document_Object___Class_Diagram) {
  validate_interactive(
      "working_with_documents/Document_Object___Class_Diagram");
}

//==================>>> working_with_relational_tables
TEST_F(Shell_py_dev_api_sample_tester, Table_insert) {
  validate_interactive("working_with_relational_tables/Table_insert");
}

TEST_F(Shell_py_dev_api_sample_tester, Working_with_Relational_Tables) {
  validate_interactive(
      "working_with_relational_tables/Working_with_Relational_Tables");
}

//==================>>> working_with_tables_documents
TEST_F(Shell_py_dev_api_sample_tester, Collections_as_Relational_Tables) {
  validate_interactive(
      "working_with_tables_documents/Collections_as_Relational_Tables");
}
}  // namespace shcore
