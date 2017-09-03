/* Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.

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

#include "test_utils.h"

namespace shcore {
namespace shell_core_tests {
class Interactive_global_schema_py_test : public Shell_core_test_wrapper {
public:
  virtual void set_options() {
    _options->interactive = true;
    _options->wizards = true;
    _options->initial_mode = IShell_core::Mode::Python;
  };
};

TEST_F(Interactive_global_schema_py_test, undefined_db_with_no_session) {
  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
  output_handler.wipe_all();

  _interactive_shell->process_line("db.getName()");
  MY_EXPECT_STDERR_CONTAINS("The db variable is not set, establish a session first.");
  output_handler.wipe_all();

  _interactive_shell->process_line("db.name");
  MY_EXPECT_STDERR_CONTAINS("The db variable is not set, establish a session first.");
  output_handler.wipe_all();
}

TEST_F(Interactive_global_schema_py_test, defined_db_usage) {
  _interactive_shell->process_line("\\connect " + _uri + "/mysql");
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
  output_handler.wipe_all();

  _interactive_shell->process_line("db.get_name()");
  MY_EXPECT_STDOUT_CONTAINS("mysql");
  output_handler.wipe_all();

  _interactive_shell->process_line("db.name");
  MY_EXPECT_STDOUT_CONTAINS("mysql");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_global_schema_py_test, resolve_property_to_empty_schema) {
  _interactive_shell->process_line("\\connect " + _uri);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
  output_handler.wipe_all();

  output_handler.prompts.push_back("y");  // The db variable is not set, do you want to set the active schema?
  output_handler.prompts.push_back("");   // Please specify the schema
  _interactive_shell->process_line("print('Schema: ' + db.name)");

  MY_EXPECT_STDERR_CONTAINS("Invalid schema specified");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_global_schema_py_test, resolve_property_to_unexisting_schema) {
  _interactive_shell->process_line("\\connect " + _uri);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
  output_handler.wipe_all();

  output_handler.prompts.push_back("y");          // The db variable is not set, do you want to set the active schema?
  output_handler.prompts.push_back("unexisting"); // Please specify the schema
  _interactive_shell->process_line("print('Schema: ' + db.name)");

  MY_EXPECT_STDERR_CONTAINS("Unknown database 'unexisting'");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_global_schema_py_test, resolve_property_to_valid_schema) {
  _interactive_shell->process_line("\\connect " + _uri);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
  output_handler.wipe_all();

  // TEST: Attempt to configure an unexisting schema
  output_handler.prompts.push_back("y");     // The db variable is not set, do you want to set the active schema?
  output_handler.prompts.push_back("mysql"); // Please specify the schema
  _interactive_shell->process_line("print('Schema: ' + db.name)");

  MY_EXPECT_STDOUT_CONTAINS("Schema: mysql");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_global_schema_py_test, resolve_method_to_empty_schema) {
  _interactive_shell->process_line("\\connect " + _uri);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
  output_handler.wipe_all();

  output_handler.prompts.push_back("y");  // The db variable is not set, do you want to set the active schema?
  output_handler.prompts.push_back("");   // Please specify the schema
  _interactive_shell->process_line("print('Schema: ' + db.get_name())");

  MY_EXPECT_STDERR_CONTAINS("Invalid schema specified");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_global_schema_py_test, resolve_method_to_unexisting_schema) {
  _interactive_shell->process_line("\\connect " + _uri);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
  output_handler.wipe_all();

  output_handler.prompts.push_back("y");          // The db variable is not set, do you want to set the active schema?
  output_handler.prompts.push_back("unexisting"); // Please specify the schema
  _interactive_shell->process_line("print('Schema: ' + db.get_name())");

  MY_EXPECT_STDERR_CONTAINS("Unknown database 'unexisting'");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_global_schema_py_test, resolve_method_to_valid_schema) {
  _interactive_shell->process_line("\\connect " + _uri);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
  output_handler.wipe_all();

  // TEST: Attempt to configure an unexisting schema
  output_handler.prompts.push_back("y");     // The db variable is not set, do you want to set the active schema?
  output_handler.prompts.push_back("mysql"); // Please specify the schema
  _interactive_shell->process_line("print('Schema: ' + db.get_name())");

  MY_EXPECT_STDOUT_CONTAINS("Schema: mysql");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}
}
}
