/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/shell_script_tester.h"

using Version = mysqlshdk::utils::Version;

namespace shcore {
class Shell_py_mysqlx_tests : public Shell_py_script_tester {
 protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    Shell_py_script_tester::SetUp();

    // All of the test cases share the same config folder
    // and setup script
    set_config_folder("py_devapi");
    set_setup_script("setup.py");
  }

  virtual void set_defaults() {
    Shell_py_script_tester::set_defaults();

    std::string user, host, password;
    auto connection_options = shcore::get_connection_options(_uri);

    if (connection_options.has_user()) user = connection_options.get_user();

    if (connection_options.has_host()) host = connection_options.get_host();

    if (connection_options.has_password())
      password = connection_options.get_password();

    if (_port.empty()) _port = "33060";

    std::string code = "__user = '" + user + "';";
    exec_and_out_equals(code);
    code = "__pwd = '" + password + "';";
    exec_and_out_equals(code);
    code = "__host = '" + host + "';";
    exec_and_out_equals(code);
    code = "__port = " + _port + ";";
    exec_and_out_equals(code);
    code = "__schema = 'mysql';";
    exec_and_out_equals(code);
    code = "__uri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__uripwd = '" + user + ":" + password + "@" + host + ":" + _port +
           "';";
    exec_and_out_equals(code);
    code = "__displayuri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__displayuridb = '" + user + "@" + host + ":" + _port + "/mysql';";
    exec_and_out_equals(code);
  }
};

TEST_F(Shell_py_mysqlx_tests, mysqlx_module) {
  validate_interactive("mysqlx_module.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_session) {
  validate_interactive("mysqlx_session.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_session_sql) {
  validate_interactive("mysqlx_session_sql.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_schema) {
  validate_interactive("mysqlx_schema.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_table) {
  validate_interactive("mysqlx_table.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_table_insert) {
  validate_interactive("mysqlx_table_insert.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_table_select) {
  validate_interactive("mysqlx_table_select.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_table_update) {
  validate_interactive("mysqlx_table_update.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_table_delete) {
  validate_interactive("mysqlx_table_delete.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_collection) {
  validate_interactive("mysqlx_collection.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_collection_find) {
  validate_interactive("mysqlx_collection_find.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_collection_modify) {
  validate_interactive("mysqlx_collection_modify.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_collection_remove) {
  validate_interactive("mysqlx_collection_remove.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_view) {
  validate_interactive("mysqlx_view.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_resultset) {
  validate_interactive("mysqlx_resultset.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_constants) {
  validate_interactive("mysqlx_constants.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_column_metadata) {
  validate_interactive("mysqlx_column_metadata.py");
}

TEST_F(Shell_py_mysqlx_tests, mysqlx_bool_expression) {
  if (_target_server_version >= Version("8.0")) {
    validate_interactive("mysqlx_bool_expression.py");
  }
}

/**
 * Prepared statement tests are verified using protocol tracing
 */
class Shell_py_mysqlx_prepared_tests : public Shell_py_mysqlx_tests {
 protected:
  void set_options() override {
    _options->interactive = true;
    _options->wizards = true;
    _options->trace_protocol = true;
  }

  bool supports_prepared_statements() {
    bool ret_val = true;

    execute("shell.connect('" + _uri + "')");
    execute("schema = session.get_schema('mysql')");
    execute("table = schema.get_table('user')");
    execute("crud = table.select('user').limit(1)");
    execute("crud.execute()");
    _cout.str("");
    _cout.clear();
    execute("crud.execute()");

    auto out = _cout.str();
    if (out.find("Unexpected message received") != std::string::npos) {
      ret_val = false;
    }
    _cout.str("");
    _cout.clear();
    output_handler.wipe_all();
    execute("session.close()");

    return ret_val;
  }
};

TEST_F(Shell_py_mysqlx_prepared_tests, collection_find) {
  if (!supports_prepared_statements())
    SKIP_TEST("Prepared statements are not supported.");

  validate_interactive("mysqlx_collection_find_prepared.py");
}

TEST_F(Shell_py_mysqlx_prepared_tests, collection_modify) {
  if (!supports_prepared_statements())
    SKIP_TEST("Prepared statements are not supported.");

  validate_interactive("mysqlx_collection_modify_prepared.py");
}

TEST_F(Shell_py_mysqlx_prepared_tests, collection_remove) {
  if (!supports_prepared_statements())
    SKIP_TEST("Prepared statements are not supported.");

  validate_interactive("mysqlx_collection_remove_prepared.py");
}

TEST_F(Shell_py_mysqlx_prepared_tests, table_select) {
  if (!supports_prepared_statements())
    SKIP_TEST("Prepared statements are not supported.");

  validate_interactive("mysqlx_table_select_prepared.py");
}

TEST_F(Shell_py_mysqlx_prepared_tests, table_update) {
  if (!supports_prepared_statements())
    SKIP_TEST("Prepared statements are not supported.");

  validate_interactive("mysqlx_table_update_prepared.py");
}

TEST_F(Shell_py_mysqlx_prepared_tests, table_delete) {
  if (!supports_prepared_statements())
    SKIP_TEST("Prepared statements are not supported.");

  validate_interactive("mysqlx_table_delete_prepared.py");
}

TEST_F(Shell_py_mysqlx_prepared_tests, session_sql) {
  if (!supports_prepared_statements())
    SKIP_TEST("Prepared statements are not supported.");

  validate_interactive("mysqlx_session_sql_prepared.py");
}

TEST_F(Shell_py_mysqlx_prepared_tests, unsupported_from_crud) {
  if (supports_prepared_statements())
    SKIP_TEST("Prepared statements are supported.");

  validate_interactive("mysqlx_unsupported_prepared_crud.py");
}

TEST_F(Shell_py_mysqlx_prepared_tests, unsupported_from_sql) {
  if (supports_prepared_statements())
    SKIP_TEST("Prepared statements are supported.");

  validate_interactive("mysqlx_unsupported_prepared_sql.py");
}

}  // namespace shcore
