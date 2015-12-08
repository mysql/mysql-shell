/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <stdarg.h>

#include "gtest/gtest.h"
#include "src/shell_cmdline_options.h"

namespace shcore
{
#define IS_CONNECTION_DATA true
#define IS_NULLABLE true
#define AS__STRING(x) boost::lexical_cast<std::string>(x)
  class Shell_cmdline_options_t : public ::testing::Test
  {
  public:
    Shell_cmdline_options_t()
    {
    }

    std::string get_string(Shell_command_line_options*options, const std::string &option)
    {
      if (option == "app")
        return options->app;
      else if (option == "host")
        return options->host;
      else if (option == "user")
        return options->user;
      else if (option == "password")
        return options->password ? std::string(options->password) : "";
      else if (option == "port")
        return AS__STRING(options->port);
      else if (option == "schema")
        return options->schema;
      else if (option == "sock")
        return options->sock;
      else if (option == "ssl-ca")
        return options->ssl_ca;
      else if (option == "ssl-cert")
        return options->ssl_cert;
      else if (option == "ssl-key")
        return options->ssl_key;
      else if (option == "ssl")
        return AS__STRING(options->ssl);
      else if (option == "uri")
        return options->uri;
      else if (option == "output_format")
        return options->output_format;
      else if (option == "session_type")
        return AS__STRING(options->session_type);
      else if (option == "force")
        return AS__STRING(options->force);
      else if (option == "interactive")
        return AS__STRING(options->interactive);
      else if (option == "full_interactive")
        return AS__STRING(options->full_interactive);
      else if (option == "passwords_from_stdin")
        return AS__STRING(options->passwords_from_stdin);
      else if (option == "prompt_password")
        return AS__STRING(options->prompt_password);
      else if (option == "recreate_database")
        return AS__STRING(options->recreate_database);
      else if (option == "trace_protocol")
        return AS__STRING(options->trace_protocol);
      else if (option == "log_level")
        return AS__STRING(options->log_level);
      else if (option == "initial-mode")
        return AS__STRING(options->initial_mode);
      else if (option == "session-type")
        return AS__STRING(options->session_type);

      return "";
    }

    void test_option_equal_value(const std::string &option, const std::string & value, bool connection_data, const std::string& target_option = "", const std::string& target_value = "")
    {
      // Redirect cout.
      std::streambuf* backup = std::cout.rdbuf();
      std::ostringstream cerr;
      std::cerr.rdbuf(cerr.rdbuf());

      // Tests --option=value
      std::string arg;
      arg.append("--").append(option).append("=").append(value);
      SCOPED_TRACE("TESTING: " + arg);
      char *argv[] = { const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), NULL };
      Shell_command_line_options options(2, argv);
      EXPECT_EQ(0, options.exit_code);
      EXPECT_EQ(connection_data, options.has_connection_data());

      std::string tgt_val = target_value.empty() ? value : target_value;
      std::string tgt_option = target_option.empty() ? option : target_option;
      EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

      EXPECT_STREQ("", cerr.str().c_str());

      // Restore old cout.
      std::cerr.rdbuf(backup);
    }

    void test_option_space_value(const std::string &option, const std::string & value, bool connection_data, const std::string &target_option = "", const std::string& target_value = "")
    {
      // Redirect cout.
      std::streambuf* backup = std::cout.rdbuf();
      std::ostringstream cerr;
      std::cerr.rdbuf(cerr.rdbuf());

      // Tests --option [value]
      std::string arg;
      arg.append("--").append(option);
      SCOPED_TRACE("TESTING: " + arg + " " + value);
      char *argv[] = { const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), const_cast<char *>(value.c_str()), NULL };
      Shell_command_line_options options(3, argv);
      EXPECT_EQ(0, options.exit_code);
      EXPECT_EQ(connection_data, options.has_connection_data());

      std::string tgt_val = target_value.empty() ? value : target_value;
      std::string tgt_option = target_option.empty() ? option : target_option;
      EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

      EXPECT_STREQ("", cerr.str().c_str());

      // Restore old cout.
      std::cerr.rdbuf(backup);
    }

    void test_short_option_value(const std::string &option, const std::string &soption, const std::string &value, bool connection_data, const std::string& target_option = "", const std::string& target_value = "")
    {
      // Redirect cout.
      std::streambuf* backup = std::cout.rdbuf();
      std::ostringstream cerr;
      std::cerr.rdbuf(cerr.rdbuf());

      // Tests --ovalue
      std::string arg;
      arg.append("-").append(soption).append(value);
      SCOPED_TRACE("TESTING: " + arg);
      char *argv[] = { const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), NULL };
      Shell_command_line_options options(2, argv);
      EXPECT_EQ(0, options.exit_code);
      EXPECT_EQ(connection_data, options.has_connection_data());

      std::string tgt_val = target_value.empty() ? value : target_value;
      std::string tgt_option = target_option.empty() ? option : target_option;
      EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

      EXPECT_STREQ("", cerr.str().c_str());

      // Restore old cout.
      std::cerr.rdbuf(backup);
    }

    void test_short_option_space_value(const std::string &option, const std::string& soption, const std::string &value, bool connection_data, const std::string& target_option = "", const std::string& target_value = "")
    {
      // Redirect cout.
      std::streambuf* backup = std::cout.rdbuf();
      std::ostringstream cerr;
      std::cerr.rdbuf(cerr.rdbuf());

      // Tests --option [value]
      std::string arg;
      arg.append("-").append(soption);
      SCOPED_TRACE("TESTING: " + arg + " " + value);
      char *argv[] = { const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), const_cast<char *>(value.c_str()), NULL };
      Shell_command_line_options options(3, argv);
      EXPECT_EQ(0, options.exit_code);
      EXPECT_EQ(connection_data, options.has_connection_data());

      std::string tgt_val = target_value.empty() ? value : target_value;
      std::string tgt_option = target_option.empty() ? option : target_option;
      EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

      EXPECT_STREQ("", cerr.str().c_str());

      // Restore old cout.
      std::cerr.rdbuf(backup);
    }

    void test_option_space_no_value(const std::string &option, bool valid, const std::string& defval, const std::string target_option = "", const std::string& target_value = "")
    {
      // Redirect cout.
      std::streambuf* backup = std::cout.rdbuf();
      std::ostringstream cerr;
      std::cerr.rdbuf(cerr.rdbuf());

      // Tests --option [value]
      std::string arg;
      arg.append("--").append(option);
      SCOPED_TRACE("TESTING: " + arg);
      char *argv[] = { const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), NULL };
      Shell_command_line_options options(2, argv);

      if (valid)
      {
        EXPECT_EQ(0, options.exit_code);

        std::string tgt_val = target_value.empty() ? defval : target_value;
        std::string tgt_option = target_option.empty() ? option : target_option;
        EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

        EXPECT_STREQ("", cerr.str().c_str());
      }
      else
      {
        EXPECT_EQ(1, options.exit_code);
        std::string message = "ut: option ";
        message.append("--").append(option).append(" requires an argument\n");
        EXPECT_STREQ(message.c_str(), cerr.str().c_str());
      }

      // Restore old cout.
      std::cerr.rdbuf(backup);
    }

    void test_option_equal_no_value(const std::string &option, bool valid)
    {
      // Redirect cout.
      std::streambuf* backup = std::cout.rdbuf();
      std::ostringstream cerr;
      std::cerr.rdbuf(cerr.rdbuf());

      // Tests --option=
      std::string arg;
      arg.append("--").append(option).append("=");
      SCOPED_TRACE("TESTING: " + arg);
      char *argv[] = { const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), NULL };
      Shell_command_line_options options(2, argv);

      if (valid)
      {
        EXPECT_EQ(0, options.exit_code);
        EXPECT_STREQ("", cerr.str().c_str());
      }
      else
      {
        EXPECT_EQ(1, options.exit_code);
        std::string message = "ut: option ";
        message.append("--").append(option).append("= requires an argument\n");
        EXPECT_STREQ(message.c_str(), cerr.str().c_str());
      }

      // Restore old cout.
      std::cerr.rdbuf(backup);
    }

    void test_option_with_value(const std::string &option, const std::string &soption, const std::string &value, const std::string &defval, bool is_connection_data, bool nullable, const std::string& target_option = "", const std::string& target_value = "")
    {
      // --option=<value>
      test_option_equal_value(option, value, is_connection_data, target_option, target_value);

      // --option value
      test_option_space_value(option, value, is_connection_data, target_option, target_value);

      // --option
      test_option_space_no_value(option, !defval.empty() || nullable, defval, target_option);

      if (!soption.empty())
      {
        // -o<value>
        test_short_option_value(option, soption, value, is_connection_data, target_option, target_value);

        // -o <value>
        test_short_option_space_value(option, soption, value, is_connection_data, target_option, target_value);
      }

      // --option=
      test_option_equal_no_value(option, !defval.empty() || nullable);
    }

    void test_option_with_no_value(const std::string &option, const std::string &target_option, const std::string &target_value)
    {
      // Redirect cout.
      std::streambuf* backup = std::cout.rdbuf();
      std::ostringstream cerr;
      std::cerr.rdbuf(cerr.rdbuf());

      // Tests --option or -o

      SCOPED_TRACE("TESTING: " + option);
      char *argv[] = { const_cast<char *>("ut"), const_cast<char *>(option.c_str()), NULL };
      Shell_command_line_options options(2, argv);

      EXPECT_EQ(0, options.exit_code);
      EXPECT_STREQ(target_value.c_str(), get_string(&options, target_option).c_str());
      EXPECT_STREQ("", cerr.str().c_str());

      // Restore old cout.
      std::cerr.rdbuf(backup);
    }
  };

  TEST(Shell_cmdline_options, default_values)
  {
    int argc = 0;
    char **argv = NULL;

    Shell_command_line_options options(argc, argv);

    EXPECT_TRUE(options.app.empty());
    EXPECT_TRUE(options.exit_code == 0);
    EXPECT_FALSE(options.force);
    EXPECT_FALSE(options.full_interactive);
    EXPECT_FALSE(options.has_connection_data());
    EXPECT_TRUE(options.host.empty());

#ifdef HAVE_V8
    EXPECT_EQ(options.initial_mode, IShell_core::Mode_JScript);
#else
#ifdef HAVE_PYTHON
    EXPECT_EQ(options.initial_mode, IShell_core::Mode_Python);
#else
    EXPECT_EQ(options.initial_mode, IShell_core::Mode_SQL);
#endif
#endif

    EXPECT_FALSE(options.interactive);
    EXPECT_EQ(options.log_level, ngcommon::Logger::LOG_ERROR);
    EXPECT_TRUE(options.output_format.empty());
    EXPECT_EQ(NULL, options.password);
    EXPECT_FALSE(options.passwords_from_stdin);
    EXPECT_EQ(options.port, 0);
    EXPECT_FALSE(options.prompt_password);
    EXPECT_TRUE(options.protocol.empty());
    EXPECT_FALSE(options.recreate_database);
    EXPECT_TRUE(options.run_file.empty());
    EXPECT_TRUE(options.schema.empty());
    EXPECT_EQ(options.session_type, mysh::Application);
    EXPECT_TRUE(options.sock.empty());
    EXPECT_EQ(options.ssl, 0);
    EXPECT_TRUE(options.ssl_ca.empty());
    EXPECT_TRUE(options.ssl_cert.empty());
    EXPECT_TRUE(options.ssl_key.empty());
    EXPECT_FALSE(options.trace_protocol);
    EXPECT_TRUE(options.uri.empty());
    EXPECT_TRUE(options.user.empty());
  }

  TEST_F(Shell_cmdline_options_t, app)
  {
    test_option_with_value("app", "", "sample", "", IS_CONNECTION_DATA, !IS_NULLABLE);
    test_option_with_value("host", "h", "localhost", "", IS_CONNECTION_DATA, !IS_NULLABLE);
    test_option_with_value("port", "P", "3306", "", IS_CONNECTION_DATA, !IS_NULLABLE);
    test_option_with_value("schema", "D", "sakila", "", IS_CONNECTION_DATA, !IS_NULLABLE);
    test_option_with_value("database", "", "sakila", "", IS_CONNECTION_DATA, !IS_NULLABLE, "schema");
    test_option_with_value("user", "u", "root", "", IS_CONNECTION_DATA, !IS_NULLABLE);
    test_option_with_value("dbuser", "u", "root", "", IS_CONNECTION_DATA, !IS_NULLABLE, "user");
    test_option_with_value("password", "p", "mypwd", "", IS_CONNECTION_DATA, IS_NULLABLE);
    test_option_with_value("dbpassword", "p", "mypwd", "", IS_CONNECTION_DATA, IS_NULLABLE, "password");

    test_option_with_no_value("-p", "prompt_password", "1");

    test_option_equal_value("dbpassword", "mypwd", IS_CONNECTION_DATA, "password");
    test_option_space_value("dbpassword", "mypwd", IS_CONNECTION_DATA, "password");
    test_short_option_value("dbpassword", "p", "mypwd", IS_CONNECTION_DATA, "password");
    test_short_option_space_value("dbpassword", "p", "mypwd", IS_CONNECTION_DATA, "password");
    test_option_equal_no_value("dbpassword", true);
    test_option_with_no_value("--dbpassword", "prompt_password", "1");

    test_option_with_value("ssl-ca", "", "some_value", "", IS_CONNECTION_DATA, false);
    test_option_with_value("ssl-cert", "", "some_value", "", IS_CONNECTION_DATA, !IS_NULLABLE);
    test_option_with_value("ssl-key", "", "some_value", "", IS_CONNECTION_DATA, !IS_NULLABLE);
    test_option_with_value("ssl", "", "1", "1", IS_CONNECTION_DATA, !IS_NULLABLE);
    test_option_with_value("ssl", "", "0", "1", !IS_CONNECTION_DATA, !IS_NULLABLE);
    test_option_with_value("ssl", "", "yes", "1", IS_CONNECTION_DATA, !IS_NULLABLE, "", "1");
    //test_option_with_value("ssl", "", "no", "1", !IS_CONNECTION_DATA, !IS_NULLABLE, "", "0");

    test_option_with_value("session-type", "", "classic", "", !IS_CONNECTION_DATA, !IS_NULLABLE, "", AS__STRING(mysh::Classic));
    test_option_with_value("session-type", "", "node", "", !IS_CONNECTION_DATA, !IS_NULLABLE, "", AS__STRING(mysh::Node));
    test_option_with_value("session-type", "", "app", "", !IS_CONNECTION_DATA, !IS_NULLABLE, "", AS__STRING(mysh::Application));

    test_option_with_no_value("--x", "session-type", AS__STRING(mysh::Application));
    test_option_with_no_value("--classic", "session-type", AS__STRING(mysh::Classic));
    test_option_with_no_value("--node", "session-type", AS__STRING(mysh::Node));

    test_option_with_no_value("--sql", "session-type", AS__STRING(mysh::Node));
    test_option_with_no_value("--sql", "initial-mode", AS__STRING(IShell_core::Mode_SQL));

    test_option_with_no_value("--sqlc", "session-type", AS__STRING(mysh::Classic));
    test_option_with_no_value("--sqlc", "initial-mode", AS__STRING(IShell_core::Mode_SQL));

    test_option_with_no_value("--javascript", "initial-mode", AS__STRING(IShell_core::Mode_JScript));
    test_option_with_no_value("--js", "initial-mode", AS__STRING(IShell_core::Mode_JScript));

    test_option_with_no_value("--python", "initial-mode", AS__STRING(IShell_core::Mode_Python));
    test_option_with_no_value("--py", "initial-mode", AS__STRING(IShell_core::Mode_Python));

    test_option_with_no_value("--recreate-schema", "recreate_database", "1");

    test_option_with_value("json", "", "pretty", "json", !IS_CONNECTION_DATA, IS_NULLABLE, "output_format", "json");
    test_option_with_value("json", "", "raw", "json", !IS_CONNECTION_DATA, IS_NULLABLE, "output_format", "json/raw");
    test_option_with_no_value("--json", "output_format", "json");
    test_option_with_no_value("--table", "output_format", "table");
    test_option_with_no_value("--trace-proto", "trace_protocol", "1");
    test_option_with_no_value("--force", "force", "1");
    test_option_with_no_value("--interactive", "interactive", "1");
    test_option_with_no_value("-i", "interactive", "1");

    test_option_with_value("interactive", "", "full", "1", !IS_CONNECTION_DATA, IS_NULLABLE, "interactive", "1");
    //test_option_with_value("interactive", "", "full", "1", !IS_CONNECTION_DATA, IS_NULLABLE, "full_interactive", "1");

    test_option_with_no_value("--passwords-from-stdin", "passwords_from_stdin", "1");
  }
}