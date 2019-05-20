/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlsh/mysql_shell.h"
#include <mysqld_error.h>
#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "modules/devapi/mod_mysqlx.h"
#include "modules/devapi/mod_mysqlx_resultset.h"  // temporary
#include "modules/devapi/mod_mysqlx_schema.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql.h"
#include "modules/mod_mysql_resultset.h"  // temporary
#include "modules/mod_mysql_session.h"
#include "modules/mod_shell.h"
#include "modules/mod_utils.h"
#include "modules/util/mod_util.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/db/utils_error.h"
#include "mysqlshdk/libs/innodbcluster/cluster.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/shellcore/credential_manager.h"
#include "scripting/shexcept.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/shell_resultset_dumper.h"

#include "commands/command_help.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "src/interactive/interactive_dba_cluster.h"
#include "src/interactive/interactive_global_dba.h"
#include "src/interactive/interactive_global_shell.h"
#include "src/mysqlsh/commands/command_show.h"
#include "src/mysqlsh/commands/command_watch.h"
#include "utils/debug.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

DEBUG_OBJ_ENABLE(Mysql_shell);

REGISTER_HELP(HELP_AVAILABLE_TOPICS_TITLE, "The available topics include:");

REGISTER_HELP(HELP_AVAILABLE_TOPICS_ALL, "@li The available shell commands.");
REGISTER_HELP(HELP_AVAILABLE_TOPICS_ALL1,
              "@li Any word that is part of an SQL statement.");
REGISTER_HELP(HELP_AVAILABLE_TOPICS_ALL2,
              "@li <b>Command Line</b> - invoking built-in shell "
              "functions without entering interactive mode.");

REGISTER_HELP(HELP_AVAILABLE_TOPICS_SCRIPTING,
              "${HELP_AVAILABLE_TOPICS_TITLE}");
REGISTER_HELP(HELP_AVAILABLE_TOPICS_SCRIPTING1,
              "@li The <b>dba</b> global object and the classes available at "
              "the AdminAPI.");
REGISTER_HELP(
    HELP_AVAILABLE_TOPICS_SCRIPTING2,
    "@li The <b>mysqlx</b> module and the classes available at the X DevAPI.");
REGISTER_HELP(HELP_AVAILABLE_TOPICS_SCRIPTING3,
              "@li The <b>mysql</b> module and the global objects and classes "
              "available at the ShellAPI.");
REGISTER_HELP(
    HELP_AVAILABLE_TOPICS_SCRIPTING4,
    "@li The functions and properties of the classes exposed by the APIs.");
REGISTER_HELP(HELP_AVAILABLE_TOPICS_SCRIPTING5, "${HELP_AVAILABLE_TOPICS_ALL}");

REGISTER_HELP(HELP_AVAILABLE_TOPICS_SQL, "${HELP_AVAILABLE_TOPICS_TITLE}");
REGISTER_HELP(HELP_AVAILABLE_TOPICS_SQL1, "${HELP_AVAILABLE_TOPICS_ALL}");

REGISTER_HELP(HELP_PATTERN,
              "The pattern is a filter to identify topics for which help is "
              "required, it can use the following wildcards:");
REGISTER_HELP(HELP_PATTERN1, "@li <b>?</b> matches any single charecter.");
REGISTER_HELP(HELP_PATTERN2, "@li <b>*</b> matches any character sequence.");

REGISTER_HELP(CONTENTS_DETAIL,
              "The Shell Help is organized in categories and topics. To get "
              "help for a specific category or topic use: <b>\\?</b> "
              "<pattern>");
REGISTER_HELP(
    CONTENTS_DETAIL1,
    "The <pattern> argument should be the name of a category or a topic.");
REGISTER_HELP(CONTENTS_DETAIL2, "${HELP_PATTERN}");

REGISTER_HELP(GLOBALS_DESC,
              "The following modules and objects are ready for use when the "
              "shell starts:");
REGISTER_HELP(GLOBALS_CLOSING_DESC,
              "For additional information on these global objects use: "
              "<object>.<b>help()</b>");
REGISTER_HELP(GLOBALS_EXAMPLE_SCRIPTING, "<b>\\?</b> AdminAPI");
REGISTER_HELP(GLOBALS_EXAMPLE_SCRIPTING_DESC,
              "Displays information about the AdminAPI.");
REGISTER_HELP(GLOBALS_EXAMPLE_SCRIPTING1, "<b>\\?</b> \\connect");
REGISTER_HELP(GLOBALS_EXAMPLE_SCRIPTING1_DESC,
              "Displays usage details for the <b>\\connect</b> command.");
REGISTER_HELP(GLOBALS_EXAMPLE_SCRIPTING2,
              "<b>\\?</b> <<<checkInstanceConfiguration>>>");
REGISTER_HELP(GLOBALS_EXAMPLE_SCRIPTING2_DESC,
              "Displays usage details for the "
              "dba.<b><<<checkInstanceConfiguration>>></b> function.");
REGISTER_HELP(GLOBALS_EXAMPLE_SCRIPTING3, "<b>\\?</b> sql syntax");
REGISTER_HELP(GLOBALS_EXAMPLE_SCRIPTING3_DESC,
              "Displays the main SQL help categories.");

REGISTER_HELP(GLOBALS_EXAMPLE_SQL, "<b>\\?</b> sql syntax");
REGISTER_HELP(GLOBALS_EXAMPLE_SQL_DESC,
              "Displays the main SQL help categories.");
REGISTER_HELP(GLOBALS_EXAMPLE_SQL1, "<b>\\?</b> select");
REGISTER_HELP(GLOBALS_EXAMPLE_SQL1_DESC,
              "Displays information about the <b>SELECT</b> SQL statement.");

// The main category may have many topics created everywhere which are for
// reference from different parts, we don't want those displayed.
// i.e. For more information use: \? connection
// Connection is a topic hung on the root node
REGISTER_HELP(CONTENTS_CHILDS_DESC, "IGNORE");
REGISTER_HELP(CONTENTS_CATEGORIES_DESC,
              "The following are the main help categories:");
REGISTER_HELP(CONTENTS_CATEGORIES_CLOSING_DESC, "${HELP_AVAILABLE_TOPICS}");
REGISTER_HELP(CONTENTS_CLOSING,
              "Use <b>\\? \\help</b> for additional details.");

REGISTER_HELP(SQL_CONTENTS_BRIEF,
              "Entry point to retrieve syntax help on SQL statements.");

namespace mysqlsh {

namespace {
void print_diag(const std::string &s) { current_console()->print_diag(s); }

void print_warning(const std::string &s) {
  current_console()->print_warning(s);
}

void println(const std::string &s) { current_console()->println(s); }
}  // namespace

class Shell_command_provider : public shcore::completer::Provider {
 public:
  explicit Shell_command_provider(Mysql_shell *shell) : shell_(shell) {}

  shcore::completer::Completion_list complete(const std::string &text,
                                              size_t *compl_offset) {
    shcore::completer::Completion_list options;
    size_t cmdend;
    if (text[0] == '\\' && (cmdend = text.find(' ')) != std::string::npos) {
      // check if we're completing params for a \command

      // keep it simple for now just and handle \command completions here...
      options =
          complete_command(text.substr(0, cmdend), text.substr(*compl_offset));
    } else if ((*compl_offset > 0 && *compl_offset < text.length() &&
                text[*compl_offset - 1] == '\\') ||
               (*compl_offset == 0 && text.length() > 1 && text[0] == '\\')) {
      // handle escape commands
      if (*compl_offset > 0) {
        // extend the completed string beyond the default, which will break
        // on the \\ .. this requires that this provider be the 1st in the list
        *compl_offset -= 1;
      }
      auto names = shell_->shell_context()
                       ->command_handler()
                       ->get_command_names_matching(text.substr(*compl_offset));
      std::copy(names.begin(), names.end(), std::back_inserter(options));
    } else if (text == "\\") {
      auto names = shell_->shell_context()
                       ->command_handler()
                       ->get_command_names_matching("");
      if (*compl_offset > 0) {
        // extend the completed string beyond the default, which will break
        // on the \\ .. this requires that this provider be the 1st in the list
        *compl_offset -= 1;
      }
      std::copy(names.begin(), names.end(), std::back_inserter(options));
    }
    return options;
  }

 private:
  Mysql_shell *shell_;

  shcore::completer::Completion_list complete_command(const std::string &cmd,
                                                      const std::string &arg) {
    if (cmd == "\\u" || cmd == "\\use") {
      // complete schema names
      auto provider = shell_->provider_sql();
      assert(provider);
      return provider->complete_schema(arg);
    }

    return {};
  }
};
REGISTER_HELP_TOPIC(ShellAPI, CATEGORY, shellapi, Contents, SCRIPTING);
REGISTER_HELP(SHELLAPI_BRIEF,
              "Contains information about the <b>shell</b> and <b>util</b> "
              "global objects as well as the <b>mysql</b> module that enables "
              "executing SQL on MySQL Servers.");
REGISTER_HELP(CMD_CONNECT_BRIEF,
              "Connects the shell to a MySQL server and "
              "assigns the global session.");
REGISTER_HELP(CMD_CONNECT_SYNTAX, "<b>\\connect</b> [<TYPE>] <URI>");
REGISTER_HELP(CMD_CONNECT_SYNTAX1, "<b>\\c</b> [<TYPE>] <URI>");
REGISTER_HELP(CMD_CONNECT_DETAIL,
              "TYPE is an optional parameter to specify the session type. "
              "Accepts the following values:");
REGISTER_HELP(
    CMD_CONNECT_DETAIL1,
    "@li <b>--mc</b>, <b>--mysql</b>: create a classic MySQL protocol session "
    "(default port 3306)");
REGISTER_HELP(CMD_CONNECT_DETAIL2,
              "@li <b>--mx</b>, <b>--mysqlx</b>: create an X protocol session "
              "(default port 33060)");
REGISTER_HELP(
    CMD_CONNECT_DETAIL3,
    "If TYPE is omitted, automatic protocol detection is done, unless the "
    "protocol is given in the URI.");
REGISTER_HELP(CMD_CONNECT_DETAIL4,
              "URI format is: [user[:password]@]hostname[:port]");
REGISTER_HELP(CMD_CONNECT_EXAMPLE, "<b>\\connect --mx</b> root@localhost");
REGISTER_HELP(
    CMD_CONNECT_EXAMPLE_DESC,
    "Creates a global session using the X protocol to the indicated URI.");

REGISTER_HELP(CMD_OPTION_BRIEF,
              "Allows working with the available shell options.");
REGISTER_HELP(CMD_OPTION_SYNTAX, "<b>\\option</b> [args]");
REGISTER_HELP(CMD_OPTION_DETAIL,
              "The given [args] define the operation to be done by this "
              "command, the following values are accepted");
REGISTER_HELP(
    CMD_OPTION_DETAIL1,
    "@li <b>-h</b>, <b>--help</b> [<filter>]: print help for the shell options "
    "matching filter.");
REGISTER_HELP(CMD_OPTION_DETAIL2,
              "@li <b>-l</b>, <b>--list</b> [<b>--show-origin</b>]: list all "
              "the shell options.");
REGISTER_HELP(CMD_OPTION_DETAIL3,
              "@li <shell_option>: print value of the shell option.");
REGISTER_HELP(
    CMD_OPTION_DETAIL4,
    "@li <shell_option> [=] <value> sets the value for the shell option.");
REGISTER_HELP(CMD_OPTION_DETAIL5,
              "@li <b>--persist</b> causes an option to be stored on the "
              "configuration file.");
REGISTER_HELP(
    CMD_OPTION_DETAIL6,
    "@li <b>--unset</b> resets an option value to the default value, "
    "removes the option from configuration file when used together with "
    "<b>--persist</b> option.");
REGISTER_HELP(CMD_OPTION_EXAMPLE, "\\option --persist defaultMode sql");
REGISTER_HELP(CMD_OPTION_EXAMPLE1, "\\option --unset --persist defaultMode");

REGISTER_HELP(CMD_SOURCE_BRIEF, "Loads and executes a script from a file.");
REGISTER_HELP(CMD_SOURCE_SYNTAX, "<b>\\source</b> <path>");
REGISTER_HELP(CMD_SOURCE_SYNTAX1, "<b>\\.</b> <path>");
REGISTER_HELP(
    CMD_SOURCE_DETAIL,
    "Executes a script from a file, the following languages are supported:");
REGISTER_HELP(CMD_SOURCE_DETAIL1, "@li JavaScript");
REGISTER_HELP(CMD_SOURCE_DETAIL2, "@li Python");
REGISTER_HELP(CMD_SOURCE_DETAIL3, "@li SQL");
REGISTER_HELP(
    CMD_SOURCE_DETAIL4,
    "The file will be loaded and executed using the active language.");
#ifdef _WIN32
REGISTER_HELP(CMD_SOURCE_EXAMPLE,
              "<b>\\source</b> C:\\Users\\MySQL\\sakila.sql");
REGISTER_HELP(CMD_SOURCE_EXAMPLE1, "<b>\\.</b> C:\\Users\\MySQL\\sakila.sql");
#else
REGISTER_HELP(CMD_SOURCE_EXAMPLE, "<b>\\source</b> /home/me/sakila.sql");
REGISTER_HELP(CMD_SOURCE_EXAMPLE1, "<b>\\.</b> /home/me/sakila.sql");
#endif

REGISTER_HELP(CMD_USE_BRIEF, "Sets the active schema.");
REGISTER_HELP(CMD_USE_SYNTAX, "<b>\\use</b> <schema>");
REGISTER_HELP(CMD_USE_SYNTAX1, "<b>\\u</b> <schema>");
REGISTER_HELP(CMD_USE_DETAIL,
              "Uses the global session to set the active schema.");
REGISTER_HELP(CMD_USE_DETAIL1, "When this command is used:");
REGISTER_HELP(CMD_USE_DETAIL2,
              "@li The active schema in SQL mode will be updated.");
REGISTER_HELP(
    CMD_USE_DETAIL3,
    "@li The <b>db</b> global variable available on the scripting modes "
    "will be updated.");
REGISTER_HELP(CMD_USE_EXAMPLE, "<b>\\use</b> mysql");
REGISTER_HELP(CMD_USE_EXAMPLE1, "<b>\\u</b> mysql");

REGISTER_HELP(CMD_REHASH_BRIEF, "Refresh the autocompletion cache.");
REGISTER_HELP(CMD_REHASH_SYNTAX, "<b>\\rehash</b>");
REGISTER_HELP(CMD_REHASH_DETAIL,
              "Populate or refresh the schema object name cache used for SQL "
              "auto-completion and the DevAPI schema object.");
REGISTER_HELP(CMD_REHASH_DETAIL1,
              "A rehash is automatically done whenever the 'use' command is "
              "executed, unless the shell is started with --no-name-cache.");
REGISTER_HELP(CMD_REHASH_DETAIL2,
              "This may take a long time if you have many schemas or many "
              "objects in the default schema.");

REGISTER_HELP(CMD_HELP_BRIEF,
              "Prints help information about a specific topic.");
REGISTER_HELP(CMD_HELP_SYNTAX, "<b>\\help</b> [pattern]");
REGISTER_HELP(CMD_HELP_SYNTAX1, "<b>\\h</b> [pattern]");
REGISTER_HELP(CMD_HELP_SYNTAX2, "<b>\\?</b> [pattern]");
REGISTER_HELP(CMD_HELP_DETAIL, "%{contents:detail,categories}");

REGISTER_HELP(
    CMD_HELP_DETAIL10,
    "If no pattern is specified a generic help is going to be displayed.");
REGISTER_HELP(
    CMD_HELP_DETAIL11,
    "To display the available help categories use <b>\\help<b> contents");
REGISTER_HELP(CMD_HELP_EXAMPLE, "<b>\\help</b>");
REGISTER_HELP(CMD_HELP_EXAMPLE_DESC,
              "With no parameters this command prints the general shell help.");
REGISTER_HELP(CMD_HELP_EXAMPLE1, "<b>\\?</b> contents");
REGISTER_HELP(CMD_HELP_EXAMPLE1_DESC,
              "Describes information about the help organization.");
REGISTER_HELP(CMD_HELP_EXAMPLE2, "<b>\\h</b> cluster");
REGISTER_HELP(CMD_HELP_EXAMPLE2_DESC,
              "Prints the information available for the <b>Cluster</b> class.");
REGISTER_HELP(CMD_HELP_EXAMPLE3, "<b>\\?</b> *sandbox*");
REGISTER_HELP(CMD_HELP_EXAMPLE3_DESC,
              "List the available functions for sandbox operations.");

REGISTER_HELP(CMD_SQL_BRIEF,
              "Executes SQL statement or switches to SQL processing mode when "
              "no statement is given.");
REGISTER_HELP(CMD_SQL_SYNTAX, "<b>\\sql [statement]</b>");

REGISTER_HELP(CMD_JS_BRIEF, "Switches to JavaScript processing mode.");
REGISTER_HELP(CMD_JS_SYNTAX, "<b>\\js</b>");

REGISTER_HELP(CMD_PY_BRIEF, "Switches to Python processing mode.");
REGISTER_HELP(CMD_PY_SYNTAX, "<b>\\py</b>");

REGISTER_HELP(CMD_ML_BRIEF, "Start multi-line input when in SQL mode.");
REGISTER_HELP(CMD_ML_SYNTAX, "<b>\\</b>");

REGISTER_HELP(CMD_EXIT_BRIEF, "Exits the MySQL Shell, same as \\quit.");
REGISTER_HELP(CMD_EXIT_SYNTAX, "<b>\\exit</b>");

REGISTER_HELP(CMD_QUIT_BRIEF, "Exits the MySQL Shell.");
REGISTER_HELP(CMD_QUIT_SYNTAX, "<b>\\quit</b>");

REGISTER_HELP(CMD_RECONNECT_BRIEF, "Reconnects the global session.");
REGISTER_HELP(CMD_RECONNECT_SYNTAX, "<b>\\reconnect</b>");

REGISTER_HELP(CMD_STATUS_BRIEF,
              "Print information about the current global session.");
REGISTER_HELP(CMD_STATUS_SYNTAX, "<b>\\status</b>");
REGISTER_HELP(CMD_STATUS_SYNTAX1, "<b>\\s</b>");

REGISTER_HELP(CMD_WARNINGS_BRIEF, "Show warnings after every statement.");
REGISTER_HELP(CMD_WARNINGS_SYNTAX, "<b>\\warnings</b>");
REGISTER_HELP(CMD_WARNINGS_SYNTAX1, "<b>\\W</b>");

REGISTER_HELP(CMD_NOWARNINGS_BRIEF,
              "Don't show warnings after every statement.");
REGISTER_HELP(CMD_NOWARNINGS_SYNTAX, "<b>\\nowarnings</b>");
REGISTER_HELP(CMD_NOWARNINGS_SYNTAX1, "<b>\\w</b>");

REGISTER_HELP(CMD_SHOW_BRIEF,
              "Executes the given report with provided options and arguments.");
REGISTER_HELP(CMD_SHOW_SYNTAX,
              "<b>\\show</b> <report_name> [options] [arguments]");
REGISTER_HELP(CMD_SHOW_DETAIL,
              "The report name accepted by the \\show command is "
              "case-insensitive, '-' and '_' characters can be used "
              "interchangeably.");
REGISTER_HELP(CMD_SHOW_DETAIL1, "Common options:");
REGISTER_HELP(CMD_SHOW_DETAIL2,
              "@li --help - Display help of the given report.");
REGISTER_HELP(CMD_SHOW_DETAIL3,
              "@li --vertical, -E - For 'list' type reports, display records "
              "vertically.");
REGISTER_HELP(
    CMD_SHOW_DETAIL4,
    "The output format of \\show command depends on the type of report:");
REGISTER_HELP(CMD_SHOW_DETAIL5,
              "@li 'list' - displays records in tabular form (or vertically, "
              "if --vertical is used),");
REGISTER_HELP(CMD_SHOW_DETAIL6, "@li 'report' - displays a YAML text report,");
REGISTER_HELP(CMD_SHOW_DETAIL7,
              "@li 'print' - does not display anything, report is responsible "
              "for text output.");
REGISTER_HELP(CMD_SHOW_DETAIL8,
              "If executed without the report name, lists available reports.");
REGISTER_HELP(CMD_SHOW_DETAIL9,
              "Note: user-defined reports can be registered with "
              "shell.<<<registerReport>>>() method.");
REGISTER_HELP(CMD_SHOW_EXAMPLE, "<b>\\show</b>");
REGISTER_HELP(CMD_SHOW_EXAMPLE_DESC,
              "Lists available reports, both built-in and user-defined.");
REGISTER_HELP(CMD_SHOW_EXAMPLE1,
              "<b>\\show</b> query show session status like 'Uptime%'");
REGISTER_HELP(CMD_SHOW_EXAMPLE1_DESC,
              "Executes 'query' report with the provided SQL statement.");
REGISTER_HELP(
    CMD_SHOW_EXAMPLE2,
    "<b>\\show</b> query --vertical show session status like 'Uptime%'");
REGISTER_HELP(CMD_SHOW_EXAMPLE2_DESC,
              "As above, but results are displayed in vertical form.");
REGISTER_HELP(CMD_SHOW_EXAMPLE3, "<b>\\show</b> query --help");
REGISTER_HELP(CMD_SHOW_EXAMPLE3_DESC, "Displays help for the 'query' report.");

REGISTER_HELP(
    CMD_WATCH_BRIEF,
    "Executes the given report with provided options and arguments in a loop.");
REGISTER_HELP(CMD_WATCH_SYNTAX,
              "<b>\\watch</b> <report_name> [options] [arguments]");
REGISTER_HELP(CMD_WATCH_DETAIL,
              "This command behaves like \\show command, but the given report "
              "is executed repeatedly, refreshing the screen every 2 seconds "
              "until CTRL-C is pressed.");
REGISTER_HELP(
    CMD_WATCH_DETAIL1,
    "In addition to \\show command options, following are also supported:");
REGISTER_HELP(CMD_WATCH_DETAIL2,
              "@li --interval=float, -i float - Number of seconds to wait "
              "between refreshes. Default 2. Allowed values are in range [0.1, "
              "86400].");
REGISTER_HELP(CMD_WATCH_DETAIL3,
              "@li --nocls - Don't clear the screen between refreshes.");
REGISTER_HELP(CMD_WATCH_DETAIL4,
              "If executed without the report name, lists available reports.");
REGISTER_HELP(CMD_WATCH_DETAIL5, "For more information see \\show command.");
REGISTER_HELP(CMD_WATCH_EXAMPLE, "<b>\\watch</b>");
REGISTER_HELP(CMD_WATCH_EXAMPLE_DESC,
              "Lists available reports, both built-in and user-defined.");
REGISTER_HELP(
    CMD_WATCH_EXAMPLE1,
    "<b>\\watch</b> query --interval=1 show session status like 'Uptime%'");
REGISTER_HELP(
    CMD_WATCH_EXAMPLE1_DESC,
    "Executes the 'query' report refreshing the screen every second.");
REGISTER_HELP(
    CMD_WATCH_EXAMPLE2,
    "<b>\\watch</b> query --nocls show session status like 'Uptime%'");
REGISTER_HELP(CMD_WATCH_EXAMPLE2_DESC,
              "As above, but screen is not cleared, results are displayed one "
              "after another.");

Mysql_shell::Mysql_shell(std::shared_ptr<Shell_options> cmdline_options,
                         shcore::Interpreter_delegate *custom_delegate)
    : mysqlsh::Base_shell(cmdline_options, custom_delegate) {
  DEBUG_OBJ_ALLOC(Mysql_shell);

  // Registers the interactive objects if required
  _global_shell = std::shared_ptr<mysqlsh::Shell>(new mysqlsh::Shell(this));
  _global_js_sys =
      std::shared_ptr<mysqlsh::Sys>(new mysqlsh::Sys(_shell.get()));
  _global_dba =
      std::shared_ptr<mysqlsh::dba::Dba>(new mysqlsh::dba::Dba(_shell.get()));
  _global_util =
      std::shared_ptr<mysqlsh::Util>(new mysqlsh::Util(_shell.get()));

  if (options().wizards) {
    auto interactive_shell = std::shared_ptr<shcore::Global_shell>(
        new shcore::Global_shell(*_shell.get()));
    auto interactive_dba = std::shared_ptr<shcore::Global_dba>(
        new shcore::Global_dba(*_shell.get()));

    interactive_shell->set_target(_global_shell);
    interactive_dba->set_target(_global_dba);

    set_global_object(
        "shell",
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(interactive_shell),
        shcore::IShell_core::all_scripting_modes());
    set_global_object(
        "dba",
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(interactive_dba),
        shcore::IShell_core::all_scripting_modes());
  } else {
    set_global_object(
        "shell",
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(_global_shell),
        shcore::IShell_core::all_scripting_modes());
    set_global_object(
        "dba",
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(_global_dba),
        shcore::IShell_core::all_scripting_modes());
  }

  set_global_object(
      "sys",
      std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(_global_js_sys),
      shcore::IShell_core::Mode_mask(shcore::IShell_core::Mode::JavaScript));
  set_global_object(
      "util",
      std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(_global_util),
      shcore::IShell_core::all_scripting_modes());

  auto shell_cli_operation = cmdline_options->get_shell_cli_operation();
  if (shell_cli_operation) {
    shell_cli_operation->register_provider("dba", [this]() {
      return std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(
          _shell->get_global("dba").as_object());
    });
    shell_cli_operation->register_provider("shell", [this]() {
      return std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(
          _shell->get_global("shell").as_object());
    });
    shell_cli_operation->register_provider(
        "cluster", [this]() { return this->set_default_cluster(""); });
    shell_cli_operation->register_provider("util",
                                           [this]() { return _global_util; });
    shell_cli_operation->register_provider("shell.options", [this]() {
      return _global_shell->get_shell_options();
    });
  }

  // dummy initialization
  _global_shell->set_session_global({});

  INIT_MODULE(mysqlsh::mysql::Mysql);
  INIT_MODULE(mysqlsh::mysqlx::Mysqlx);

  set_sql_safe_for_logging(get_options()->get(SHCORE_HISTIGNORE).descr());
  // completion provider for shell \commands (must be the 1st)
  completer()->add_provider(shcore::IShell_core::Mode_mask::any(),
                            std::unique_ptr<shcore::completer::Provider>(
                                new Shell_command_provider(this)),
                            true);

  // Register custom auto-completion rules
  add_devapi_completions();

  auto global_command = shcore::IShell_core::Mode_mask::all();
  SET_SHELL_COMMAND("\\help|\\?|\\h", "CMD_HELP",
                    Mysql_shell::cmd_print_shell_help);
  SET_CUSTOM_SHELL_COMMAND(
      "\\sql", "CMD_SQL",
      [this](const std::vector<std::string> &args) -> bool {
        std::string command = shcore::str_strip(args[0]);
        const auto command_pos =
            command.empty() ? std::string::npos : command.find(' ');
        command = command_pos != std::string::npos &&
                          command_pos + 1 < command.length()
                      ? command.substr(command_pos + 1)
                      : std::string();

        if (!command.empty() && !_shell->get_dev_session()) {
          print_diag("ERROR: Not connected.");
          return true;
        }

        shcore::Shell_core::Mode old_mode = _shell->interactive_mode();
        if (command.empty()) current_console()->disable_global_pager();

        if (switch_shell_mode(shcore::Shell_core::Mode::SQL, {},
                              !command.empty(), command.empty()))
          refresh_completion();

        if (!command.empty()) {
          try {
            auto sql = dynamic_cast<shcore::Shell_sql *>(
                _shell->language_object(shcore::Shell_core::Mode::SQL));
            assert(sql != nullptr);
            sql->execute(command);
          } catch (...) {
          }
          switch_shell_mode(old_mode, {}, true, false);
        }

        return true;
      },
      false, global_command, false);
#ifdef HAVE_V8
  SET_CUSTOM_SHELL_COMMAND(
      "\\js", "CMD_JS",
      [this](const std::vector<std::string> &args) -> bool {
        current_console()->disable_global_pager();
        switch_shell_mode(shcore::Shell_core::Mode::JavaScript, args);
        return true;
      },
      false, global_command);
#endif
#ifdef HAVE_PYTHON
  SET_CUSTOM_SHELL_COMMAND(
      "\\py", "CMD_PY",
      [this](const std::vector<std::string> &args) -> bool {
        current_console()->disable_global_pager();
        switch_shell_mode(shcore::Shell_core::Mode::Python, args);
        return true;
      },
      false, global_command);
#endif
  SET_SHELL_COMMAND("\\source|\\.", "CMD_SOURCE",
                    Mysql_shell::cmd_process_file);
  SET_SHELL_COMMAND("\\", "CMD_ML", Mysql_shell::cmd_start_multiline);
  SET_SHELL_COMMAND("\\quit|\\q", "CMD_QUIT", Mysql_shell::cmd_quit);
  SET_SHELL_COMMAND("\\exit", "CMD_EXIT", Mysql_shell::cmd_quit);
  SET_SHELL_COMMAND("\\connect|\\c", "CMD_CONNECT", Mysql_shell::cmd_connect);
  SET_SHELL_COMMAND("\\reconnect", "CMD_RECONNECT", Mysql_shell::cmd_reconnect);
  SET_SHELL_COMMAND("\\option", "CMD_OPTION", Mysql_shell::cmd_option);
  SET_CUSTOM_SHELL_COMMAND("\\warnings|\\W", "CMD_WARNINGS",
                           std::bind(&Mysql_shell::cmd_warnings, this, _1),
                           true, global_command);
  SET_CUSTOM_SHELL_COMMAND("\\nowarnings|\\w", "CMD_NOWARNINGS",
                           std::bind(&Mysql_shell::cmd_nowarnings, this, _1),
                           true, global_command);
  SET_SHELL_COMMAND("\\status|\\s", "CMD_STATUS", Mysql_shell::cmd_status);
  SET_SHELL_COMMAND("\\use|\\u", "CMD_USE", Mysql_shell::cmd_use);
  SET_SHELL_COMMAND("\\rehash", "CMD_REHASH", Mysql_shell::cmd_rehash);
  SET_SHELL_COMMAND("\\show", "CMD_SHOW", Mysql_shell::cmd_show);
  SET_SHELL_COMMAND("\\watch", "CMD_WATCH", Mysql_shell::cmd_watch);

  shcore::Credential_manager::get().initialize();
}

Mysql_shell::~Mysql_shell() { DEBUG_OBJ_DEALLOC(Mysql_shell); }

void Mysql_shell::finish_init() {
  Base_shell::finish_init();

  File_list startup_files;
  get_startup_scripts(&startup_files);
  load_files(startup_files, "startup files");

  File_list plugins;
  get_plugins(&plugins);
  load_files(plugins, "plugins");
}

void Mysql_shell::load_files(const File_list &file_list,
                             const std::string &context) {
  // if plugins are found, switch to the appropriate mode and load all files
  bool load_failed = false;
  log_info("Loading %s...", context.c_str());
  for (const auto &files : file_list) {
    const auto &mode = files.first;
    const auto &files_to_load = files.second;

    if (!files_to_load.empty()) {
      for (const auto &file : files_to_load) {
        log_info("- %s", file.c_str());
        if (!_shell->load_plugin(mode, file)) {
          load_failed = true;
        }
      }
    }
  }

  if (load_failed) {
    auto msg = shcore::str_format(
        "Found errors loading %s, for more details look at the log at: %s",
        context.c_str(), shcore::Logger::singleton()->logfile_name().c_str());
    current_console()->print_warning(msg);
  }
}

void Mysql_shell::get_startup_scripts(File_list *file_list) {
  std::string dir =
      shcore::path::join_path(shcore::get_user_config_path(), "init.d");

  // iterate over directories, find files to load
  if (shcore::is_folder(dir)) {
    shcore::iterdir(dir, [&dir, file_list](const std::string &name) {
      const auto full_path = shcore::path::join_path(dir, name);

      // make sure it's a file, not a directory
      if (shcore::is_file(full_path)) {
        if (shcore::str_iendswith(name, ".js")) {
#ifdef HAVE_V8
          (*file_list)[shcore::IShell_core::Mode::JavaScript].emplace_back(
              full_path);
#else
        log_warning("Ignoring startup script at '%s', JavaScript is not "
                    "available.", full_path.c_str());
#endif  // HAVE_V8
        } else if (shcore::str_iendswith(name, ".py")) {
#ifdef HAVE_PYTHON
          (*file_list)[shcore::IShell_core::Mode::Python].emplace_back(
              full_path);
#else
          log_warning("Ignoring startup script at '%s', Python is not"
                      " available.", full_path.c_str());
#endif
        }
      }

      return true;
    });
  }
}

void Mysql_shell::get_plugins(File_list *file_list) {
  const auto initial_mode = _shell->interactive_mode();
  const std::string plugin_directories[] = {
      shcore::path::join_path(shcore::get_share_folder(), "plugins"),
      shcore::path::join_path(shcore::get_user_config_path(), "plugins")};

  // A plugin is contained in a folder inside of the pre-defined "plugin"
  // directories, and it contains strictly one of the initialization files.
  // This loop identifies the valid plugins to be loaded.
  for (const auto &dir : plugin_directories) {
    if (shcore::is_folder(dir)) {
      shcore::iterdir(dir, [&dir, file_list](const std::string &name) {
        const auto plugin_dir = shcore::path::join_path(dir, name);

        if (shcore::is_folder(plugin_dir)) {
          auto init_js = shcore::path::join_path(plugin_dir, "init.js");
          auto init_py = shcore::path::join_path(plugin_dir, "init.py");

          bool is_js = shcore::is_file(init_js);
          bool is_py = shcore::is_file(init_py);

          std::string msg;
          if (is_js && is_py) {
            auto msg = shcore::str_format(
                "Found multiple plugin initialization files for plugin "
                "'%s' at %s, ignoring plugin.",
                name.c_str(), plugin_dir.c_str());
            current_console()->print_warning(msg);
          } else if (is_js) {
#ifdef HAVE_V8
            (*file_list)[shcore::IShell_core::Mode::JavaScript].emplace_back(
                init_js);
#else
            log_warning("Ignoring plugin at '%s', JavaScript is not available.",
                        plugin_dir.c_str());
#endif
          } else if (is_py) {
#ifdef HAVE_PYTHON
            (*file_list)[shcore::IShell_core::Mode::Python].emplace_back(
                init_py);
#else
            log_warning("Ignoring plugin at '%s', Python is not available.",
                        plugin_dir.c_str());
#endif
          } else {
            auto msg = shcore::str_format(
                "Missing initialization file for plugin "
                "'%s' at %s, ignoring plugin.",
                name.c_str(), plugin_dir.c_str());
            current_console()->print_warning(msg);
          }
        }

        return true;
      });
    }
  }

  // switch back to the initial mode
  switch_shell_mode(initial_mode, {}, true);
}

void Mysql_shell::print_connection_message(mysqlsh::SessionType type,
                                           const std::string &uri,
                                           const std::string &sessionid) {
  std::string stype;

  switch (type) {
    case mysqlsh::SessionType::X:
      stype = "an X protocol";
      break;
    case mysqlsh::SessionType::Classic:
      stype = "a Classic";
      break;
    case mysqlsh::SessionType::Auto:
      stype = "a";
      break;
  }

  std::string message;
  message += "Creating " + stype + " session to '" + uri + "'";

  println(message);
}

void Mysql_shell::connect(
    const mysqlshdk::db::Connection_options &connection_options_,
    bool recreate_schema) {
  mysqlshdk::db::Connection_options connection_options(connection_options_);
  std::string pass;
  std::string schema_name;
  bool interactive =
      options().interactive && !get_options()->get_shell_cli_operation();

  connection_options.set_default_connection_data();
  if (!connection_options.has_compression() &&
      get_options()->get().default_compress)
    connection_options.set_compression(true);

  if (interactive)
    print_connection_message(
        connection_options.get_session_type(),
        connection_options.as_uri(
            mysqlshdk::db::uri::formats::no_scheme_no_password()),
        /*_options.app*/ "");

  // Retrieves the schema on which the session will work on
  if (connection_options.has_schema()) {
    schema_name = connection_options.get_schema();
  }

  if (recreate_schema && schema_name.empty())
    throw shcore::Exception::runtime_error(
        "Recreate schema requested, but no schema specified");

  auto old_session(_shell->get_dev_session());
  auto new_session = set_active_session(
      establish_session(connection_options, options().wizards));

  if (old_session && old_session->is_open()) {
    if (interactive) println("Closing old connection...");

    old_session->close();
  }

  if (recreate_schema) {
    println("Recreating schema " + schema_name + "...");
    try {
      new_session->drop_schema(schema_name);
    } catch (const shcore::Exception &e) {
      if (e.is_mysql() && e.code() == 1008) {
        // ignore DB doesn't exist error
      } else {
        throw;
      }
    }
    new_session->create_schema(schema_name);

    new_session->set_current_schema(schema_name);
  }
  if (interactive) {
    std::string session_type = new_session->class_name();
    std::string message;

    message = "Your MySQL connection id is " +
              std::to_string(new_session->get_connection_id());
    if (session_type == "Session") message += " (X protocol)";
    try {
      message += "\nServer version: " +
                 new_session->get_core_session()
                     ->query("select concat(@@version, ' ', @@version_comment)")
                     ->fetch_one()
                     ->get_string(0);
    } catch (const mysqlshdk::db::Error &e) {
      // ignore password expired errors
      if (e.code() == ER_MUST_CHANGE_PASSWORD) {
      } else {
        throw;
      }
    } catch (const shcore::Exception &e) {
      // ignore password expired errors
      if (e.is_mysql() && e.code() == ER_MUST_CHANGE_PASSWORD) {
      } else {
        throw;
      }
    }
    message += "\n";
    // Any session could have a default schema after connection is done
    std::string default_schema_name = new_session->get_default_schema();

    // This will cause two things to happen
    // 1) Validation that the default schema is real
    // 2) Triggers the auto loading of the schema cache
    shcore::Object_bridge_ref default_schema;
    auto x_session =
        std::dynamic_pointer_cast<mysqlsh::mysqlx::Session>(new_session);

    if (!default_schema_name.empty()) {
      if (x_session && interactive_mode() != shcore::IShell_core::Mode::SQL) {
        default_schema = x_session->get_schema(default_schema_name);
        message += "Default schema `" + default_schema_name +
                   "` accessible through db.";
      } else {
        message += "Default schema set to `" + default_schema_name + "`.";
      }
    } else {
      message += "No default schema selected; type \\use <schema> to set one.";
    }
    println(message);
  }
}

std::shared_ptr<mysqlsh::ShellBaseSession> Mysql_shell::set_active_session(
    std::shared_ptr<mysqlshdk::db::ISession> session) {
  std::shared_ptr<mysqlsh::ShellBaseSession> new_session;

  if (auto classic =
          std::dynamic_pointer_cast<mysqlshdk::db::mysql::Session>(session)) {
    new_session = std::make_shared<mysql::ClassicSession>(classic);
  } else if (auto x = std::dynamic_pointer_cast<mysqlshdk::db::mysqlx::Session>(
                 session)) {
    new_session = std::make_shared<mysqlsh::mysqlx::Session>(x);
  } else {
    throw shcore::Exception::argument_error(
        "Invalid session type given for shell connection.");
  }
  if (session->get_connection_options().has_schema() &&
      options().devapi_schema_object_handles &&
      new_session->update_schema_cache)
    new_session->update_schema_cache(
        session->get_connection_options().get_schema(), true);

  _shell->set_dev_session(new_session);
  _global_shell->set_session_global(new_session);

  request_prompt_variables_update(true);

  if (options().interactive && !get_options()->get_shell_cli_operation()) {
    // Always refresh schema name completion cache because it can be used in
    // \use in any mode
    refresh_schema_completion();
    if (_shell->interactive_mode() == shcore::Shell_core::Mode::SQL) {
      refresh_completion();
    }
  }

  return new_session;
}

bool Mysql_shell::redirect_session_if_needed(bool secondary) {
  // Check that the connection is to a primary of a InnoDB cluster
  std::shared_ptr<mysqlshdk::db::ISession> session(
      shell_context()->get_dev_session()->get_core_session());

  std::string uri = shell_context()->get_dev_session()->uri();

  log_info("Redirecting session from '%s' to a %s of its InnoDB cluster...",
           uri.c_str(), secondary ? "SECONDARY" : "PRIMARY");

  std::shared_ptr<mysqlshdk::innodbcluster::Metadata_mysql> meta(
      mysqlshdk::innodbcluster::Metadata_mysql::create(session));

  mysqlshdk::mysql::Instance instance(session);
  mysqlshdk::innodbcluster::Cluster_group_client cluster(meta, session);

  std::vector<mysqlshdk::innodbcluster::Instance_info> candidates;
  std::string redirect_uri;

  mysqlshdk::db::Connection_options connection =
      session->get_connection_options();
  mysqlshdk::innodbcluster::Protocol_type proto =
      mysqlshdk::innodbcluster::Protocol_type::X;
  switch (connection.get_session_type()) {
    case mysqlsh::SessionType::X:
      proto = mysqlshdk::innodbcluster::Protocol_type::X;
      break;
    case mysqlsh::SessionType::Auto:
      assert(0);  // not supposed to happen, but let it fall through in release
    case mysqlsh::SessionType::Classic:
      proto = mysqlshdk::innodbcluster::Protocol_type::Classic;
      break;
  }

  if (secondary) {
    if (!cluster.single_primary()) {
      log_error(
          "Redirection to secondary but cluster for %s is not single_primary "
          "mode",
          uri.c_str());
      throw std::runtime_error(
          "Secondary member requested, but cluster is multi-primary");
    }

    // check if this session goes to a GR secondary
    if (!mysqlshdk::gr::is_primary(instance)) {
      log_info("%s is already a secondary", uri.c_str());
      return false;
    }
    log_info("Connected host %s is not secondary, trying to find one...",
             uri.c_str());
    redirect_uri = cluster.find_uri_to_any_secondary(proto);
  } else {
    // check if this session goes to a GR primary
    if (mysqlshdk::gr::is_primary(instance)) {
      log_info("%s is already a primary", uri.c_str());
      return false;
    }
    log_info("Connected host is not primary, trying to find one...");
    redirect_uri = cluster.find_uri_to_any_primary(proto);
  }

  log_info("Reconnecting to %s instance of the InnoDB cluster (%s)...",
           redirect_uri.c_str(), secondary ? "SECONDARY" : "PRIMARY");
  println("Reconnecting to " +
          std::string(secondary ? "SECONDARY" : "PRIMARY") +
          " instance of the InnoDB cluster (" + redirect_uri + ")...");

  mysqlshdk::db::Connection_options redirected_connection(redirect_uri);

  connection.clear_host();
  connection.clear_port();
  connection.clear_socket();
  connection.set_host(redirected_connection.get_host());
  if (redirected_connection.has_port())
    connection.set_port(redirected_connection.get_port());
  if (redirected_connection.has_socket())
    connection.set_socket(redirected_connection.get_socket());

  connect(connection);
  return true;
}

std::shared_ptr<mysqlsh::dba::Cluster> Mysql_shell::set_default_cluster(
    const std::string &name) {
  std::shared_ptr<shcore::Cpp_object_bridge> dba(
      _shell->get_global("dba").as_object<shcore::Cpp_object_bridge>());

  shcore::Argument_list args;
  if (!name.empty()) args.push_back(shcore::Value(name));

  shcore::Value vcluster(dba->call("getCluster", args));
  _shell->set_global("cluster", vcluster,
                     shcore::IShell_core::all_scripting_modes());

  auto cluster = vcluster.as_object<mysqlsh::dba::Cluster>();
  if (!cluster) {
    auto icluster = vcluster.as_object<shcore::Interactive_dba_cluster>();
    assert(icluster);
    if (icluster) {
      cluster = std::dynamic_pointer_cast<mysqlsh::dba::Cluster>(
          icluster->get_target());
      assert(cluster);
    }
  }
  return cluster;
}

bool Mysql_shell::cmd_print_shell_help(const std::vector<std::string> &args) {
  const auto pager = current_console()->enable_pager();
  return Command_help(_shell)(args);
}

bool Mysql_shell::cmd_start_multiline(const std::vector<std::string> &args) {
  // This command is only available for SQL Mode
  if (args.size() == 1 &&
      _shell->interactive_mode() == shcore::Shell_core::Mode::SQL) {
    _input_mode = shcore::Input_state::ContinuedBlock;

    return true;
  }

  return false;
}

bool Mysql_shell::cmd_connect(const std::vector<std::string> &args) {
  bool error = false;
  Shell_options::Storage options;

  // Holds the argument index for the target to which the session will be
  // established
  size_t target_index = 1;

  if (args.size() > 1 && args.size() < 4) {
    if (args.size() == 3) {
      std::string arg_2 = args[2];
      arg_2 = shcore::str_strip(arg_2);

      if (!arg_2.empty()) {
        target_index++;
        options.uri = args[target_index];
      }
    }

    std::string arg = args[1];
    arg = shcore::str_strip(arg);

    if (arg.empty()) {
      error = true;
    } else if (shcore::str_caseeq("-n", arg) ||
               shcore::str_caseeq("-mx", arg)) {
      options.session_type = mysqlsh::SessionType::X;
      print_warning(
          "The " + arg +
          " option is deprecated, please use --mysqlx or --mx instead.\n");
    } else if (shcore::str_caseeq("-c", arg) ||
               shcore::str_caseeq("-mc", arg)) {
      options.session_type = mysqlsh::SessionType::Classic;
      print_warning(
          "The " + arg +
          " option is deprecated, please use --mysql or --mc instead.\n");
    } else if (shcore::str_caseeq("-ma", arg)) {
      print_warning("The " + arg + " option is deprecated.\n");
    } else if (!arg.compare("--mysqlx") || !arg.compare("--mx")) {
      options.session_type = mysqlsh::SessionType::X;
    } else if (!arg.compare("--mysql") || !arg.compare("--mc")) {
      options.session_type = mysqlsh::SessionType::Classic;
    } else {
      if (args.size() == 3) {
        error = true;
      } else {
        options.uri = args[target_index];
      }
    }

    if (!error && !options.uri.empty()) {
      try {
        connect(options.connection_options());
      } catch (const shcore::Exception &e) {
        print_diag(std::string(e.format()) + "\n");
      } catch (const mysqlshdk::db::Error &e) {
        std::string msg;
        if (e.sqlstate() && *e.sqlstate())
          msg = shcore::str_format("MySQL Error %i (%s): %s", e.code(),
                                   e.sqlstate(), e.what());
        else
          msg = shcore::str_format("MySQL Error %i: %s", e.code(), e.what());
        print_diag(msg + "\n");
      } catch (const std::exception &e) {
        print_diag(std::string(e.what()) + "\n");
      }
    }
  } else {
    error = true;
  }
  if (error) print_diag("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");

  return true;
}

bool Mysql_shell::cmd_reconnect(const std::vector<std::string> &args) {
  if (args.size() > 1)
    print_diag("\\reconnect command does not accept any arguments.");
  else if (!_shell->get_dev_session())
    print_diag("There had to be a connection first to enable reconnection.");
  else
    reconnect_if_needed(true);

  return true;
}

bool Mysql_shell::cmd_quit(const std::vector<std::string> &UNUSED(args)) {
  get_options()->set_interactive(false);

  return true;
}

bool Mysql_shell::cmd_warnings(const std::vector<std::string> &UNUSED(args)) {
  get_options()->set(SHCORE_SHOW_WARNINGS, shcore::Value::True());

  println("Show warnings enabled.");

  return true;
}

bool Mysql_shell::cmd_nowarnings(const std::vector<std::string> &UNUSED(args)) {
  get_options()->set(SHCORE_SHOW_WARNINGS, shcore::Value::False());

  println("Show warnings disabled.");

  return true;
}

bool Mysql_shell::cmd_status(const std::vector<std::string> &UNUSED(args)) {
  std::string version_msg("MySQL Shell version ");
  version_msg += MYSH_FULL_VERSION;
  version_msg += "\n";
  println(version_msg);
  auto session = _shell->get_dev_session();

  if (session && session->is_open()) {
    auto status = session->get_status();
    (*status)["DELIMITER"] = shcore::Value(_shell->get_main_delimiter());
    std::string format = options().wrap_json;

    if (format.find("json") == 0) {
      println(shcore::Value(status).json(format == "json"));
    } else {
      const std::string format = "%-30s%s";

      if (status->has_key("STATUS_ERROR")) {
        println(
            shcore::str_format(format.c_str(), "Error Retrieving Status: ",
                               (*status)["STATUS_ERROR"].descr(true).c_str()));
      } else {
        if (status->has_key("SESSION_TYPE"))
          println(shcore::str_format(
              format.c_str(),
              "Session type: ", (*status)["SESSION_TYPE"].descr(true).c_str()));

        if (status->has_key("CONNECTION_ID"))
          println(shcore::str_format(
              format.c_str(), "Connection Id: ",
              (*status)["CONNECTION_ID"].descr(true).c_str()));

        if (status->has_key("DEFAULT_SCHEMA"))
          println(shcore::str_format(
              format.c_str(), "Default schema: ",
              (*status)["DEFAULT_SCHEMA"].descr(true).c_str()));

        if (status->has_key("CURRENT_SCHEMA"))
          println(shcore::str_format(
              format.c_str(), "Current schema: ",
              (*status)["CURRENT_SCHEMA"].descr(true).c_str()));

        if (status->has_key("CURRENT_USER"))
          println(shcore::str_format(
              format.c_str(),
              "Current user: ", (*status)["CURRENT_USER"].descr(true).c_str()));

        if (status->has_key("SSL_CIPHER") &&
            status->get_type("SSL_CIPHER") == shcore::String &&
            !status->get_string("SSL_CIPHER").empty())
          println(shcore::str_format(
              format.c_str(), "SSL: ",
              ("Cipher in use: " + (*status)["SSL_CIPHER"].descr(true))
                  .c_str()));
        else
          println(shcore::str_format(format.c_str(), "SSL: ", "Not in use."));

        if (status->has_key("DELIMITER"))
          println(shcore::str_format(
              format.c_str(),
              "Using delimiter: ", (*status)["DELIMITER"].descr(true).c_str()));
        if (status->has_key("SERVER_VERSION"))
          println(shcore::str_format(
              format.c_str(), "Server version: ",
              (*status)["SERVER_VERSION"].descr(true).c_str()));

        if (status->has_key("PROTOCOL_VERSION"))
          println(shcore::str_format(
              format.c_str(), "Protocol version: ",
              (*status)["PROTOCOL_VERSION"].descr(true).c_str()));

        if (status->has_key("CLIENT_LIBRARY"))
          println(shcore::str_format(
              format.c_str(), "Client library: ",
              (*status)["CLIENT_LIBRARY"].descr(true).c_str()));

        if (status->has_key("CONNECTION"))
          println(shcore::str_format(
              format.c_str(),
              "Connection: ", (*status)["CONNECTION"].descr(true).c_str()));

        if (status->has_key("TCP_PORT"))
          println(shcore::str_format(
              format.c_str(),
              "TCP port: ", (*status)["TCP_PORT"].descr(true).c_str()));

        if (status->has_key("UNIX_SOCKET"))
          println(shcore::str_format(
              format.c_str(),
              "Unix socket: ", (*status)["UNIX_SOCKET"].descr(true).c_str()));

        if (status->has_key("SERVER_CHARSET"))
          println(shcore::str_format(
              format.c_str(), "Server characterset: ",
              (*status)["SERVER_CHARSET"].descr(true).c_str()));

        if (status->has_key("SCHEMA_CHARSET"))
          println(shcore::str_format(
              format.c_str(), "Schema characterset: ",
              (*status)["SCHEMA_CHARSET"].descr(true).c_str()));

        if (status->has_key("CLIENT_CHARSET"))
          println(shcore::str_format(
              format.c_str(), "Client characterset: ",
              (*status)["CLIENT_CHARSET"].descr(true).c_str()));

        if (status->has_key("CONNECTION_CHARSET"))
          println(shcore::str_format(
              format.c_str(), "Conn. characterset: ",
              (*status)["CONNECTION_CHARSET"].descr(true).c_str()));

        if (status->has_key("COMPRESSION"))
          println(shcore::str_format(
              format.c_str(),
              "Compression: ", (*status)["COMPRESSION"].descr(true).c_str()));

        if (status->has_key("UPTIME"))
          println(shcore::str_format(format.c_str(), "Uptime: ",
                                     (*status)["UPTIME"].descr(true).c_str()));

        if (status->has_key("SERVER_STATS")) {
          std::string stats = (*status)["SERVER_STATS"].descr(true);
          size_t start = stats.find(" ");
          start++;
          size_t end = stats.find(" ", start);

          std::string time = stats.substr(start, end - start);
          std::string str_time =
              mysqlshdk::utils::format_seconds(std::stod(time));

          println(
              shcore::str_format(format.c_str(), "Uptime: ", str_time.c_str()));
          println("");
          println(stats.substr(end + 2));
        }
      }
    }
  } else {
    print_diag("Not Connected.\n");
  }
  return true;
}

bool Mysql_shell::cmd_use(const std::vector<std::string> &args) {
  std::string error;
  if (_shell->get_dev_session() && _shell->get_dev_session()->is_open()) {
    std::string real_param;

    // If quoted, takes as param what's inside of the quotes
    auto start = args[0].find_first_of("\"'`");
    if (start != std::string::npos) {
      std::string quote = args[0].substr(start, 1);

      if (args[0].size() >= start) {
        auto end = args[0].find(quote, start + 1);

        if (end != std::string::npos)
          real_param = args[0].substr(start + 1, end - start - 1);
        else
          error = "Mistmatched quote on command parameter: " +
                  args[0].substr(start) + "\n";
      }
    } else if (args.size() == 2) {
      real_param = args[1];
    } else {
      error = "\\use <schema_name>\n";
    }
    if (error.empty()) {
      try {
        _global_shell->set_current_schema(real_param);
        _last_active_schema = real_param;

        auto session = _shell->get_dev_session();

        if (session) {
          if (session->class_name() == "ClassicSession" ||
              interactive_mode() == shcore::IShell_core::Mode::SQL)
            println("Default schema set to `" + real_param + "`.");
          else
            println("Default schema `" + real_param +
                    "` accessible through db.");

          request_prompt_variables_update();
          refresh_completion();
        }
      } catch (const shcore::Exception &e) {
        error = e.format();
      } catch (const mysqlshdk::db::Error &e) {
        error = shcore::Exception::mysql_error_with_code(e.what(), e.code())
                    .format();
      }
    }
  } else {
    error = "Not connected.\n";
  }

  if (!error.empty()) print_diag(error);

  return true;
}

bool Mysql_shell::cmd_rehash(const std::vector<std::string> &args) {
  if (_shell->get_dev_session()) {
    refresh_schema_completion(true);
    refresh_completion(true);

    shcore::Value vdb(_shell->get_global("db"));
    if (vdb) {
      auto db(vdb.as_object<mysqlsh::mysqlx::Schema>());
      if (db) {
        db->update_cache();
      }
    }
  } else {
    println("Not connected.\n");
  }
  return true;
}

void Mysql_shell::refresh_schema_completion(bool force) {
  if (options().db_name_cache || force) {
    std::shared_ptr<mysqlsh::ShellBaseSession> session(
        _shell->get_dev_session());
    if (session && _provider_sql) {
      println(
          "Fetching schema names for autocompletion... "
          "Press ^C to stop.");
      try {
        _provider_sql->refresh_schema_cache(session);
      } catch (const std::exception &e) {
        print_diag(
            shcore::str_format(
                "Error during auto-completion cache update: %s\n", e.what())
                .c_str());
      }
    }
  }
}

void Mysql_shell::refresh_completion(bool force) {
  if (options().db_name_cache || force) {
    std::shared_ptr<mysqlsh::ShellBaseSession> session(
        _shell->get_dev_session());
    std::string current_schema;
    auto handle_error = [](const std::exception &e) {
      print_diag(shcore::str_format(
          "Error during auto-completion cache update: %s\n", e.what()));
    };

    if (session) {
      try {
        current_schema = session->get_current_schema();
      } catch (const std::exception &e) {
        handle_error(e);
        return;
      }
    }

    if (session && _provider_sql && !current_schema.empty()) {
      // Only refresh the full DB name cache if we're in SQL mode
      if (_shell->interactive_mode() == shcore::IShell_core::Mode::SQL) {
        println("Fetching table and column names from `" + current_schema +
                "` for auto-completion... Press ^C to stop.");
        try {
          _provider_sql->refresh_name_cache(session, current_schema,
                                            nullptr,  // &table_names,
                                            true);
        } catch (const std::exception &e) {
          handle_error(e);
        }
      }
    }
  }
}

bool Mysql_shell::cmd_option(const std::vector<std::string> &args) {
  const auto last_option = [&args](std::size_t offset) {
    std::string res(args[offset++]);
    for (; offset < args.size(); offset++) res += " " + args[offset];
    return res;
  };

  try {
    if (args.size() < 2 || args.size() > 5) {
      print_diag(_shell->get_helper()->get_help("Shell Commands \\option"));
    } else if (args[1] == "-h" || args[1] == "--help") {
      std::string filter = args.size() > 2 ? last_option(2) : "";
      auto help = get_options()->get_named_help(filter, 80, 1);
      if (help.empty())
        print_diag("No help found for filter: " + filter);
      else
        println(help.substr(0, help.size() - 1));
    } else if (args[1] == "-l" || args[1] == "--list") {
      bool show_origin =
          args.size() > 2 && args[2] == "--show-origin" ? true : false;
      for (const auto &line :
           get_options()->get_options_description(show_origin))
        println(" " + line);
    } else if (args.size() == 2 && args[1].find('=') == std::string::npos) {
      println(get_options()->get_value_as_string(args[1]));
    } else if (args[1] == "--unset") {
      if (args[2] == "--persist") {
        if (args.size() > 3)
          get_options()->unset(last_option(3), true);
        else
          print_diag("Unset requires option to be specified");
      } else {
        get_options()->unset(last_option(2), false);
      }
    } else {
      std::size_t args_start = 1;
      bool persist = false;
      if (args[args_start] == "--persist") {
        args_start++;
        persist = true;
      }

      if (args.size() - args_start > 1) {
        std::string optname = args[args_start];
        std::string value =
            args[args_start + 1] == "=" && args.size() >= args_start + 3
                ? last_option(args_start + 2)
                : last_option(args_start + 1);
        const auto eq = optname.find('=');
        if (eq != std::string::npos) {
          if (eq + 1 < optname.size())
            value = optname.substr(eq + 1) + ' ' + value;
          optname = optname.substr(0, eq);
        }
        if (value[0] == '=') value = value.substr(1);
        get_options()->set_and_notify(optname, value, persist);
      } else {
        std::size_t offset = args[args_start].find('=');
        if (offset == std::string::npos) {
          print_diag("Setting an option requires value to be specified");
        } else {
          std::string opt = args[args_start].substr(0, offset);
          std::string val = args[args_start].substr(offset + 1);
          get_options()->set_and_notify(opt, val, persist);
        }
      }
    }
  } catch (const std::exception &e) {
    print_diag(e.what());
  }

  return true;
}

bool Mysql_shell::cmd_process_file(const std::vector<std::string> &params) {
  std::string file;

  if (params.size() < 2)
    throw shcore::Exception::runtime_error("Filename not specified");

  // The parameter 0 contains the somplete command as submitted by the user
  // File name would be on parameter 1
  file = shcore::str_strip(params[1]);

  // Adds support for quoted files in case there are spaces in the path
  if ((file[0] == '\'' && file[file.size() - 1] == '\'') ||
      (file[0] == '"' && file[file.size() - 1] == '"'))
    file = file.substr(1, file.size() - 2);

  if (file.empty()) {
    print_diag("Usage: \\. <filename> | \\source <filename>\n");
    return true;
  }

  std::vector<std::string> args(params);

  // Deletes the original command
  args.erase(args.begin());
  args[0] = file;

  Base_shell::process_file(file, args);

  return true;
}

bool Mysql_shell::cmd_show(const std::vector<std::string> &args) {
  return Command_show(_shell, _global_shell->get_shell_reports())(args);
}

bool Mysql_shell::cmd_watch(const std::vector<std::string> &args) {
  return Command_watch(_shell, _global_shell->get_shell_reports())(args);
}

bool Mysql_shell::do_shell_command(const std::string &line) {
  // Special handling for use <db>, which in the classic client was overridden
  // as a built-in command and thus didn't need ; at the end
  if (options().interactive &&
      _shell->interactive_mode() == shcore::IShell_core::Mode::SQL) {
    std::string tmp = shcore::str_rstrip(shcore::str_strip(line), ";");
    if (shcore::str_ibeginswith(tmp, "use ")) {
      return _shell->handle_shell_command("\\" + tmp);
    }
  }

  bool handled = _shell->handle_shell_command(line);
  if (line.length() > 1 && line[0] == '\\' && !handled) {
    print_diag("Unknown command: '" + line + "'");
    handled = true;
  }

  return handled;
}

void Mysql_shell::process_line(const std::string &line) {
  bool handled_as_command = false;
  std::string to_history;

  // check if the line is an escape/shell command
  if (_input_buffer.empty() && !line.empty() &&
      _input_mode == shcore::Input_state::Ok) {
    try {
      handled_as_command = do_shell_command(line);
    } catch (const std::exception &exc) {
      std::string error(exc.what());
      error += "\n";
      print_diag(error);
      handled_as_command = true;
    }
  }

  if (handled_as_command)
    notify_executed_statement(line);
  else
    Base_shell::process_line(line);
}

void Mysql_shell::process_sql_result(
    std::shared_ptr<mysqlshdk::db::IResult> result,
    const shcore::Sql_result_info &info) {
  Base_shell::process_sql_result(result, info);

  if (result) {
    std::shared_ptr<IPager> pager;

    if (result->has_resultset()) {
      pager = current_console()->enable_pager();
    }

    if (options().show_column_type_info) {
      auto cols = result->get_metadata();
      auto console = mysqlsh::current_console();
      if (options().wrap_json != "off") {
        shcore::JSON_dumper dumper(options().wrap_json == "json");
        dumper.start_object();
        for (std::size_t i = 0; i < cols.size(); i++) {
          dumper.append_string(shcore::str_format("Field %zu", i + 1));
          dumper.start_object();
          std::stringstream ss(to_string(cols[i]));
          std::string line;
          while (getline(ss, line)) {
            auto sep = line.find(":");
            assert(sep + 1 < line.length());
            dumper.append_string(line.substr(0, sep));
            dumper.append_string(shcore::str_strip(line.substr(sep + 1)));
          }
          dumper.end_object();
        }
        dumper.end_object();
        console->raw_print(dumper.str(), mysqlsh::Output_stream::STDOUT, false);
        console->raw_print("\n", mysqlsh::Output_stream::STDOUT, false);
      } else {
        for (std::size_t i = 0; i < cols.size(); i++) {
          console->println(shcore::str_format("Field %zu", i + 1));
          console->println(to_string(cols[i]));
        }
      }
    }

    auto old_format = options().result_format;
    if (info.show_vertical)
      mysqlsh::current_shell_options()->set_result_format("vertical");
    shcore::Scoped_callback clean([old_format]() {
      mysqlsh::current_shell_options()->set_result_format(old_format);
    });

    Resultset_dumper dumper(result.get(), false);
    dumper.dump("row", false, false);

    auto cresult = dynamic_cast<mysqlshdk::db::mysql::Result *>(result.get());
    if (cresult && options().interactive) {
      const std::vector<std::string> &gtids(result->get_gtids());
      if (!gtids.empty()) {
        println("GTIDs: " + shcore::str_join(gtids, ", "));
      }
    }
  }
}

bool Mysql_shell::reconnect_if_needed(bool force) {
  bool ret_val = false;
  bool reconnect_session = false;
  auto session = _shell->get_dev_session();

  if (session) {
    const auto core_session = session->get_core_session();
    if (core_session) {
      auto error = core_session->get_last_error();
      reconnect_session =
          (error != nullptr) &&
          mysqlshdk::db::is_server_connection_error(error->code());
    }
  }

  if (reconnect_session || force) {
    Connection_options co = session->get_connection_options();
    if (!_last_active_schema.empty() &&
        (!co.has_schema() || co.get_schema() != _last_active_schema))
      co.set_schema(_last_active_schema);
    if (!force)
      m_console_handler.get()->print("The global session got disconnected..\n");
    m_console_handler.get()->print("Attempting to reconnect to '" +
                                   co.as_uri() + "'..");

    shcore::sleep_ms(500);
    int attempts = 6;
    while (!ret_val && attempts > 0) {
      try {
        session->connect(co);
        ret_val = true;
      } catch (const shcore::Exception &e) {
        ret_val = false;
      }
      if (!ret_val) {
        m_console_handler.get()->print("..");
        attempts--;
        if (attempts > 0) {
          // Try again
          shcore::sleep_ms(1000);
        }
      }
    }

    if (ret_val)
      m_console_handler.get()->print(
          "\nThe global session was successfully reconnected.\n");
    else
      m_console_handler.get()->print(
          "\nThe global session could not be reconnected automatically.\n"
          "Please use '\\reconnect' instead to manually reconnect.\n");
  }

  return ret_val;
}

static std::vector<std::string> g_patterns;

bool Mysql_shell::sql_safe_for_logging(const std::string &sql) {
  for (auto &pat : g_patterns) {
    if (shcore::match_glob(pat, sql)) {
      return false;
    }
  }
  return true;
}

void Mysql_shell::set_sql_safe_for_logging(const std::string &patterns) {
  g_patterns = shcore::split_string(patterns, ":");
}

void Mysql_shell::add_devapi_completions() {
  std::shared_ptr<shcore::completer::Object_registry> registry(
      completer_object_registry());

  // These rules allow performing context-aware auto-completion
  // They don't necessarily match 1:1 with real objects, they just
  // represent possible productions in the chaining DevAPI syntax

  // TODO(alfredo) add a meta-class system so that these can be determined
  // dynamically/automatically

  registry->add_completable_type("mysqlx", {{"getSession", "Session", true}});
  registry->add_completable_type("mysql",
                                 {{"getClassicSession", "ClassicSession", true},
                                  {"getSession", "ClassicSession", true}});

  registry->add_completable_type("Operation*", {{"execute", "Result", true}});
  registry->add_completable_type("Bind*", {{"bind", "Bind*", true},
                                           {"execute", "Result", true},
                                           {"help", "", true}});
  registry->add_completable_type("SqlBind*", {{"bind", "SqlBind*", true},
                                              {"execute", "SqlResult", true},
                                              {"help", "", true}});
  registry->add_completable_type("SqlOperation*",
                                 {{"bind", "SqlBind*", true},
                                  {"execute", "SqlResult", true},
                                  {"help", "", true}});

  registry->add_completable_type("Session",
                                 {{"uri", "", false},
                                  {"defaultSchema", "Schema", true},
                                  {"currentSchema", "Schema", true},
                                  {"close", "", true},
                                  {"commit", "SqlResult", true},
                                  {"createSchema", "Schema", true},
                                  {"dropSchema", "", true},
                                  {"getCurrentSchema", "Schema", true},
                                  {"getDefaultSchema", "Schema", true},
                                  {"getSchema", "Schema", true},
                                  {"getSchemas", "", true},
                                  {"getUri", "", true},
                                  {"help", "", true},
                                  {"isOpen", "", true},
                                  {"quoteName", "", true},
                                  {"rollback", "SqlResult", true},
                                  {"runSql", "SqlResult", true},
                                  {"setCurrentSchema", "Schema", true},
                                  {"setFetchWarnings", "Result", true},
                                  {"sql", "SqlOperation*", true},
                                  {"startTransaction", "SqlResult", true},
                                  {"setSavepoint", "", true},
                                  {"releaseSavepoint", "", true},
                                  {"rollbackTo", "", true}});

  registry->add_completable_type("Schema",
                                 {{"name", "name", false},
                                  {"session", "session", false},
                                  {"schema", "schema", false},
                                  {"createCollection", "Collection", true},
                                  {"dropCollection", "", true},
                                  {"existsInDatabase", "", true},
                                  {"getCollection", "Collection", true},
                                  {"getCollectionAsTable", "Table", true},
                                  {"getCollections", "list", true},
                                  {"getName", "string", true},
                                  {"getSchema", "Schema", true},
                                  {"getSession", "Session", true},
                                  {"getTable", "Table", true},
                                  {"getTables", "list", true},
                                  {"help", "string", true}});

  registry->add_completable_type("Collection",
                                 {{"add", "CollectionAdd", true},
                                  {"addOrReplaceOne", "Result", true},
                                  {"modify", "CollectionModify", true},
                                  {"find", "CollectionFind", true},
                                  {"remove", "CollectionRemove", true},
                                  {"removeOne", "Result", true},
                                  {"replaceOne", "Result", true},
                                  {"createIndex", "SqlResult", true},
                                  {"dropIndex", "", true},
                                  {"existsInDatabase", "", true},
                                  {"session", "Session", false},
                                  {"schema", "Schema", false},
                                  {"getSchema", "Schema", true},
                                  {"getSession", "Session", true},
                                  {"getName", "", true},
                                  {"getOne", "", true},
                                  {"name", "", false},
                                  {"help", "", true},
                                  {"count", "", false}});

  registry->add_completable_type("CollectionFind",
                                 {{"fields", "CollectionFind*fields", true},
                                  {"groupBy", "CollectionFind*groupBy", true},
                                  {"sort", "CollectionFind*sort", true},
                                  {"limit", "CollectionFind*limit", true},
                                  {"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "DocResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("CollectionFind*fields",
                                 {{"groupBy", "CollectionFind*groupBy", true},
                                  {"sort", "CollectionFind*sort", true},
                                  {"limit", "CollectionFind*limit", true},
                                  {"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "DocResult", true},
                                  {"help", "", true}});
  registry->add_completable_type(
      "CollectionFind*groupBy",
      {{"having", "CollectionFind*having", true},
       {"sort", "CollectionFind*sort", true},
       {"limit", "CollectionFind*limit", true},
       {"lockShared", "CollectionFind*lockShared", true},
       {"lockExclusive", "CollectionFind*lockShared", true},
       {"bind", "Bind*", true},
       {"execute", "DocResult", true},
       {"help", "", true}});
  registry->add_completable_type("CollectionFind*having",
                                 {{"sort", "CollectionFind*sort", true},
                                  {"limit", "CollectionFind*limit", true},
                                  {"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "DocResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("CollectionFind*sort",
                                 {{"limit", "CollectionFind*limit", true},
                                  {"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "DocResult", true},
                                  {"help", "", true}});
  registry->add_completable_type(
      "CollectionFind*limit", {{"skip", "CollectionFind*skip_offset", true},
                               {"offset", "CollectionFind*skip_offset", true},
                               {"lockShared", "Bind*", true},
                               {"lockExclusive", "Bind*", true},
                               {"bind", "Bind*", true},
                               {"execute", "DocResult", true},
                               {"help", "", true}});
  registry->add_completable_type("CollectionFind*skip_offset",
                                 {{"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "DocResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("CollectionFind*lockShared",
                                 {{"bind", "Bind*", true},
                                  {"execute", "DocResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("CollectionFind*lockExclusive",
                                 {{"bind", "Bind*", true},
                                  {"execute", "DocResult", true},
                                  {"help", "", true}});

  registry->add_completable_type("CollectionAdd",
                                 {{"add", "CollectionAdd", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("CollectionRemove",
                                 {{"sort", "CollectionRemove*sort", true},
                                  {"limit", "CollectionRemove*limit", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("CollectionRemove*sort",
                                 {{"limit", "CollectionRemove*limit", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("CollectionRemove*limit",
                                 {{"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("CollectionModify",
                                 {{"set", "CollectionModify*", true},
                                  {"unset", "CollectionModify*", true},
                                  {"merge", "CollectionModify*", true},
                                  {"patch", "CollectionModify*", true},
                                  {"arrayInsert", "CollectionModify*", true},
                                  {"arrayAppend", "CollectionModify*", true},
                                  {"arrayDelete", "CollectionModify*", true},
                                  {"help", "", true}});

  registry->add_completable_type("CollectionModify*",
                                 {{"set", "CollectionModify*", true},
                                  {"unset", "CollectionModify*", true},
                                  {"merge", "CollectionModify*", true},
                                  {"patch", "CollectionModify*", true},
                                  {"arrayInsert", "CollectionModify*", true},
                                  {"arrayAppend", "CollectionModify*", true},
                                  {"arrayDelete", "CollectionModify*", true},
                                  {"sort", "CollectionModify*sort", true},
                                  {"limit", "CollectionModify*limit", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("CollectionModify*sort",
                                 {{"limit", "CollectionModify*limit", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("CollectionModify*limit",
                                 {{"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  // Table

  registry->add_completable_type("Table", {{"select", "TableSelect", true},
                                           {"update", "TableUpdate", true},
                                           {"delete", "TableDelete", true},
                                           {"insert", "TableInsert", true},
                                           {"existsInDatabase", "", true},
                                           {"session", "Session", false},
                                           {"schema", "Schema", false},
                                           {"getSchema", "Schema", true},
                                           {"getSession", "Session", true},
                                           {"getName", "", true},
                                           {"isView", "", true},
                                           {"name", "", false},
                                           {"help", "", true},
                                           {"count", "", false}});

  registry->add_completable_type(
      "TableSelect", {{"where", "TableSelect*where", true},
                      {"groupBy", "TableSelect*groupBy", true},
                      {"orderBy", "TableSelect*sort", true},
                      {"limit", "TableSelect*limit", true},
                      {"lockShared", "TableSelect*lockShared", true},
                      {"lockExclusive", "TableSelect*lockExclusive", true},
                      {"bind", "Bind*", true},
                      {"execute", "RowResult", true},
                      {"help", "", true}});
  registry->add_completable_type("TableSelect*where",
                                 {{"groupBy", "TableSelect*groupBy", true},
                                  {"orderBy", "TableSelect*sort", true},
                                  {"limit", "TableSelect*limit", true},
                                  {"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "RowResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("TableSelect*groupBy",
                                 {{"having", "TableSelect*having", true},
                                  {"orderBy", "TableSelect*sort", true},
                                  {"limit", "TableSelect*limit", true},
                                  {"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "RowResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("TableSelect*having",
                                 {{"orderBy", "TableSelect*sort", true},
                                  {"limit", "TableSelect*limit", true},
                                  {"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "RowResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("TableSelect*sort",
                                 {{"limit", "TableSelect*limit", true},
                                  {"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "RowResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("TableSelect*limit",
                                 {{"offset", "TableSelect*skip", true},
                                  {"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "RowResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("TableSelect*skip",
                                 {{"lockShared", "Bind*", true},
                                  {"lockExclusive", "Bind*", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "RowResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("TableSelect*lockShared",
                                 {{"bind", "Bind*", true},
                                  {"execute", "RowResult", true},
                                  {"help", "", true}});
  registry->add_completable_type("TableSelect*lockExclusive",
                                 {{"bind", "Bind*", true},
                                  {"execute", "RowResult", true},
                                  {"help", "", true}});

  registry->add_completable_type(
      "TableInsert",
      {{"values", "TableInsert*values", true}, {"help", "", true}});

  registry->add_completable_type("TableInsert*values",
                                 {{"values", "TableInsert", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("TableDelete",
                                 {{"where", "TableDelete*where", true},
                                  {"orderBy", "TableDelete*sort", true},
                                  {"limit", "TableDelete*limit", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("TableDelete*where",
                                 {{"orderBy", "TableDelete*sort", true},
                                  {"limit", "TableDelete*limit", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("TableDelete*sort",
                                 {{"limit", "TableDelete*limit", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("TableDelete*limit",
                                 {{"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type(
      "TableUpdate", {{"set", "TableUpdate*set", true}, {"help", "", true}});

  registry->add_completable_type("TableUpdate*set",
                                 {{"set", "TableUpdate*", true},
                                  {"where", "TableUpdate*where", true},
                                  {"orderBy", "TableUpdate*sort", true},
                                  {"limit", "TableUpdate*limit", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("TableUpdate*where",
                                 {{"orderBy", "TableUpdate*sort", true},
                                  {"limit", "TableUpdate*limit", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("TableUpdate*sort",
                                 {{"limit", "TableUpdate*limit", true},
                                  {"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  registry->add_completable_type("TableUpdate*limit",
                                 {{"bind", "Bind*", true},
                                  {"execute", "Result", true},
                                  {"help", "", true}});

  // Results

  registry->add_completable_type("DocResult",
                                 {{"affectedItemsCount", "", false},
                                  {"fetchOne", "", true},
                                  {"fetchAll", "", true},
                                  {"help", "", true},
                                  {"executionTime", "", false},
                                  {"warningCount", "", false},
                                  {"warnings", "", false},
                                  {"warningsCount", "", false},
                                  {"getAffectedItemsCount", "", false},
                                  {"getExecutionTime", "", true},
                                  {"getWarningCount", "", true},
                                  {"getWarnings", "", true},
                                  {"getWarningsCount", "", true}});

  registry->add_completable_type("RowResult",
                                 {{"affectedItemsCount", "", false},
                                  {"fetchOne", "Row", true},
                                  {"fetchOneObject", "", true},
                                  {"fetchAll", "Row", true},
                                  {"help", "", true},
                                  {"columns", "", false},
                                  {"columnCount", "", false},
                                  {"columnNames", "", false},
                                  {"getAffectedItemsCount", "", false},
                                  {"getColumns", "", true},
                                  {"getColumnCount", "", true},
                                  {"getColumnNames", "", true},
                                  {"executionTime", "", false},
                                  {"warningCount", "", false},
                                  {"warnings", "", false},
                                  {"warningsCount", "", false},
                                  {"getExecutionTime", "", true},
                                  {"getWarningCount", "", true},
                                  {"getWarnings", "", true},
                                  {"getWarningsCount", "", true}});

  registry->add_completable_type("Result",
                                 {{"affectedItemsCount", "", false},
                                  {"executionTime", "", false},
                                  {"warningCount", "", false},
                                  {"warnings", "", false},
                                  {"warningsCount", "", false},
                                  {"affectedItemCount", "", false},
                                  {"autoIncrementValue", "", false},
                                  {"getAffectedItemsCount", "", false},
                                  {"generatedIds", "", false},
                                  {"getAffectedItemCount", "", true},
                                  {"getAutoIncrementValue", "", true},
                                  {"getExecutionTime", "", true},
                                  {"getGeneratedIds", "", true},
                                  {"getWarningCount", "", true},
                                  {"getWarnings", "", true},
                                  {"getWarningsCount", "", true},
                                  {"help", "", true}});

  registry->add_completable_type("SqlResult",
                                 {{"affectedItemsCount", "", false},
                                  {"executionTime", "", true},
                                  {"warningCount", "", true},
                                  {"warnings", "", true},
                                  {"warningsCount", "", true},
                                  {"columnCount", "", true},
                                  {"columns", "", true},
                                  {"columnNames", "", true},
                                  {"autoIncrementValue", "", false},
                                  {"affectedRowCount", "", false},
                                  {"fetchAll", "", true},
                                  {"fetchOne", "", true},
                                  {"fetchOneObject", "", true},
                                  {"getAffectedItemsCount", "", false},
                                  {"getAffectedRowCount", "", true},
                                  {"getAutoIncrementValue", "", true},
                                  {"getColumnCount", "", true},
                                  {"getColumnNames", "", true},
                                  {"getColumns", "", true},
                                  {"getExecutionTime", "", true},
                                  {"getWarningCount", "", true},
                                  {"getWarnings", "", true},
                                  {"getWarningsCount", "", true},
                                  {"hasData", "", true},
                                  {"help", "", true},
                                  {"nextDataSet", "", true},
                                  {"nextResult", "", true}});

  // ===
  registry->add_completable_type("ClassicSession",
                                 {{"close", "", true},
                                  {"commit", "ClassicResult", true},
                                  {"getUri", "", true},
                                  {"help", "", true},
                                  {"isOpen", "", true},
                                  {"rollback", "ClassicResult", true},
                                  {"runSql", "ClassicResult", true},
                                  {"query", "ClassicResult", true},
                                  {"startTransaction", "ClassicResult", true},
                                  {"uri", "", false}});

  registry->add_completable_type("ClassicResult",
                                 {{"executionTime", "", true},
                                  {"warningCount", "", true},
                                  {"warningsCount", "", true},
                                  {"warnings", "", true},
                                  {"columnCount", "", true},
                                  {"columns", "", true},
                                  {"columnNames", "", true},
                                  {"autoIncrementValue", "", false},
                                  {"affectedItemsCount", "", true},
                                  {"affectedRowCount", "", true},
                                  {"fetchAll", "", true},
                                  {"fetchOne", "", true},
                                  {"fetchOneObject", "", true},
                                  {"getAffectedItemsCount", "", true},
                                  {"getAffectedRowCount", "", true},
                                  {"getAutoIncrementValue", "", true},
                                  {"getColumnCount", "", true},
                                  {"getColumnNames", "", true},
                                  {"getColumns", "", true},
                                  {"getExecutionTime", "", true},
                                  {"getWarningCount", "", true},
                                  {"getWarningsCount", "", true},
                                  {"getWarnings", "", true},
                                  {"getInfo", "", true},
                                  {"info", "", false},
                                  {"hasData", "", true},
                                  {"help", "", true},
                                  {"nextDataSet", "", true},
                                  {"nextResult", "", true}});
}
}  // namespace mysqlsh
