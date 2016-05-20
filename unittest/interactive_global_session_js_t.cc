/* Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.

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

namespace shcore
{
  namespace shell_core_tests
  {
    class Interactive_global_session_js_test : public Shell_core_test_wrapper
    {
    public:
      virtual void set_options()
      {
        _options->interactive = true;
        _options->wizards = true;
        _options->initial_mode = IShell_core::Mode_JScript;
      };
    };

    TEST_F(Interactive_global_session_js_test, undefined_session_usage)
    {
      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
      output_handler.wipe_all();

      // Answers no to the question if session should be established
      output_handler.prompts.push_back("n");
      _interactive_shell->process_line("session.uri");
      MY_EXPECT_STDOUT_CONTAINS("The global session is not set, do you want to establish a session?");
      MY_EXPECT_STDERR_CONTAINS("Invalid object member uri");
      output_handler.wipe_all();

      // Answers no to the question if session should be established
      output_handler.prompts.push_back("n");
      _interactive_shell->process_line("session.getUri()");
      MY_EXPECT_STDOUT_CONTAINS("The global session is not set, do you want to establish a session?");
      MY_EXPECT_STDERR_CONTAINS("Invalid object member getUri");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_global_session_js_test, defined_session_usage)
    {
      _interactive_shell->process_line("\\connect " + _uri);
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<XSession:" + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session.uri");
      MY_EXPECT_STDOUT_CONTAINS(_uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session.getUri()");
      MY_EXPECT_STDOUT_CONTAINS(_uri_nopasswd);
      output_handler.wipe_all();
    }

    TEST_F(Interactive_global_session_js_test, resolve_property_access_to_x)
    {
      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
      output_handler.wipe_all();

      // Answers no to the question if session should be established
      output_handler.prompts.push_back("y");  // Would you like to establish a session
      output_handler.prompts.push_back("1");  // Session type 1) X
      output_handler.prompts.push_back(_uri_nopasswd); // Connection data
      output_handler.ret_pwd = "";
      _interactive_shell->process_line("println('Resolved: ' + session.uri);");
      MY_EXPECT_STDOUT_CONTAINS("The global session is not set, do you want to establish a session?");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the session type:");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the MySQL server URI (or $slias):");
      MY_EXPECT_STDOUT_CONTAINS("Enter password:");
      MY_EXPECT_STDOUT_CONTAINS("Resolved: " + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<XSession:" + _uri_nopasswd);
      output_handler.wipe_all();
    }

    TEST_F(Interactive_global_session_js_test, resolve_method_call_to_x)
    {
      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
      output_handler.wipe_all();

      // Answers no to the question if session should be established
      output_handler.prompts.push_back("y");  // Would you like to establish a session
      output_handler.prompts.push_back("1");  // Session type 1) X
      output_handler.prompts.push_back(_uri_nopasswd); // Connection data
      output_handler.ret_pwd = "";
      _interactive_shell->process_line("println('Resolved: ' + session.getUri());");
      MY_EXPECT_STDOUT_CONTAINS("The global session is not set, do you want to establish a session?");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the session type:");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the MySQL server URI (or $slias):");
      MY_EXPECT_STDOUT_CONTAINS("Enter password:");
      MY_EXPECT_STDOUT_CONTAINS("Resolved: " + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<XSession:" + _uri_nopasswd);
      output_handler.wipe_all();
    }

    TEST_F(Interactive_global_session_js_test, resolve_property_access_to_node)
    {
      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
      output_handler.wipe_all();

      // Answers no to the question if session should be established
      output_handler.prompts.push_back("y");  // Would you like to establish a session
      output_handler.prompts.push_back("2");  // Session type 1) Node
      output_handler.prompts.push_back(_uri_nopasswd); // Connection data
      output_handler.ret_pwd = "";
      _interactive_shell->process_line("println('Resolved: ' + session.uri);");
      MY_EXPECT_STDOUT_CONTAINS("The global session is not set, do you want to establish a session?");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the session type:");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the MySQL server URI (or $slias):");
      MY_EXPECT_STDOUT_CONTAINS("Enter password:");
      MY_EXPECT_STDOUT_CONTAINS("Resolved: " + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
      output_handler.wipe_all();
    }

    TEST_F(Interactive_global_session_js_test, resolve_method_call_to_node)
    {
      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
      output_handler.wipe_all();

      // Answers no to the question if session should be established
      output_handler.prompts.push_back("y");  // Would you like to establish a session
      output_handler.prompts.push_back("2");  // Session type 1) Node
      output_handler.prompts.push_back(_uri_nopasswd); // Connection data
      output_handler.ret_pwd = "";
      _interactive_shell->process_line("println('Resolved: ' + session.getUri());");
      MY_EXPECT_STDOUT_CONTAINS("The global session is not set, do you want to establish a session?");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the session type:");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the MySQL server URI (or $slias):");
      MY_EXPECT_STDOUT_CONTAINS("Enter password:");
      MY_EXPECT_STDOUT_CONTAINS("Resolved: " + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
      output_handler.wipe_all();
    }

    TEST_F(Interactive_global_session_js_test, resolve_property_access_to_classic)
    {
      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
      output_handler.wipe_all();

      // Answers no to the question if session should be established
      output_handler.prompts.push_back("y");  // Would you like to establish a session
      output_handler.prompts.push_back("3");  // Session type 1) Classic
      output_handler.prompts.push_back(_mysql_uri_nopasswd); // Connection data
      output_handler.ret_pwd = "";
      _interactive_shell->process_line("println('Resolved: ' + session.uri);");
      MY_EXPECT_STDOUT_CONTAINS("The global session is not set, do you want to establish a session?");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the session type:");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the MySQL server URI (or $slias):");
      MY_EXPECT_STDOUT_CONTAINS("Enter password:");
      MY_EXPECT_STDOUT_CONTAINS("Resolved: " + _mysql_uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
      output_handler.wipe_all();
    }

    TEST_F(Interactive_global_session_js_test, resolve_method_call_to_classic)
    {
      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
      output_handler.wipe_all();

      // Answers no to the question if session should be established
      output_handler.prompts.push_back("y");  // Would you like to establish a session
      output_handler.prompts.push_back("3");  // Session type 1) Classic
      output_handler.prompts.push_back(_mysql_uri_nopasswd); // Connection data
      output_handler.ret_pwd = "";
      _interactive_shell->process_line("println('Resolved: ' + session.getUri());");
      MY_EXPECT_STDOUT_CONTAINS("The global session is not set, do you want to establish a session?");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the session type:");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the MySQL server URI (or $slias):");
      MY_EXPECT_STDOUT_CONTAINS("Enter password:");
      MY_EXPECT_STDOUT_CONTAINS("Resolved: " + _mysql_uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
      output_handler.wipe_all();
    }

    TEST_F(Interactive_global_session_js_test, get_unexisting_schema)
    {
      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<Undefined>");
      output_handler.wipe_all();

      // Answers no to the question if session should be established
      output_handler.prompts.push_back("y");  // Would you like to establish a session
      output_handler.prompts.push_back("2");  // Session type 1) Node
      output_handler.prompts.push_back(_uri_nopasswd); // Connection data
      output_handler.ret_pwd = "";
      output_handler.prompts.push_back("y"); // Would you like to create the schema
      _interactive_shell->process_line("var myschema = session.getSchema('mysample')");
      MY_EXPECT_STDOUT_CONTAINS("The global session is not set, do you want to establish a session?");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the session type:");
      MY_EXPECT_STDOUT_CONTAINS("Please specify the MySQL server URI (or $slias):");
      MY_EXPECT_STDOUT_CONTAINS("Enter password:");
      output_handler.wipe_all();

      _interactive_shell->process_line("myschema");
      MY_EXPECT_STDOUT_CONTAINS("<Schema:mysample>");
      output_handler.wipe_all();
    }
  }
}