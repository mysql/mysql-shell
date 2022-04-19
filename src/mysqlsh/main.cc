/*
 * Copyright (c) 2014, 2022, Oracle and/or its affiliates.
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

#include "modules/mod_utils.h"
#include "modules/util/json_importer.h"
#include "mysqlsh/cmdline_shell.h"
#include "mysqlsh/json_shell.h"
#include "mysqlshdk/include/shellcore/interrupt_helper.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/document_parser.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_cli_mapper.h"
#ifdef HAVE_PYTHON
#include "mysqlshdk/include/scripting/python_context.h"
#endif

#include <sys/stat.h>
#include <clocale>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#define tzset _tzset
#else  // !_WIN32
#include <signal.h>
#endif  // !_WIN32

#ifdef ENABLE_SESSION_RECORDING
void handle_debug_options(int *argc, char ***argv);
void init_debug_shell(std::shared_ptr<mysqlsh::Command_line_shell> shell);
void finalize_debug_shell(mysqlsh::Command_line_shell *shell);
#endif

const char *g_mysqlsh_path;

static int enable_x_protocol(
    std::shared_ptr<mysqlsh::Command_line_shell> shell) {
  auto shell_session = shell->shell_context()->get_dev_session();

  mysqlshdk::db::Connection_options connection_options =
      shell_session->get_connection_options();

  connection_options.clear_scheme();
  connection_options.set_scheme("mysqlx");
  // Temporary port to be replaced by the X port
  connection_options.clear_port();
  connection_options.set_port(0);

  std::string temp_uri =
      connection_options.as_uri(mysqlshdk::db::uri::formats::full());

  shell->process_line("var uri_template = '" + temp_uri + "'");

  // clang-format off
  static const char *script =
      R"*(
// Returns object with code and message about X Protocol state
function checkXProtocol() {
  result = {}
  result.code = 0;
  result.message = '';

  // Return codes:
  // - 0: X Protocol is active for TCP connections
  // - 1: X Protocol is active but is not listening for TCP connections
  // - 2: X Protocol is inactive
  // - 3: Error verifying xprotocol state

  try {
    if (session.uri.indexOf("mysqlx://") == 0) {
      let x_port = session.runSql('select @@mysqlx_port as xport').fetchOneObject().xport;
      result.code = 0;
      result.message = `X Protocol plugin is already enabled and listening for connections on port ${x_port}`;
    } else {
      let active = session.runSql(`SELECT COUNT(*) AS active
                                    FROM information_schema.plugins WHERE plugin_name='mysqlx'
                                    AND plugin_status='ACTIVE'`).fetchOneObject().active;

      if (active) {
        let x_port = session.runSql("SELECT @@mysqlx_port AS xport").fetchOneObject().xport;
        let server_uuid = session.runSql("SELECT @@server_uuid AS uuid").fetchOneObject().uuid;

        // Attempts a TCP connection to the X protocol
        let x_uri = uri_template.replace(":0", ":" + x_port.toString());
        let x_session;
        try {
          x_session = shell.openSession(x_uri);
          let x_server_uuid = x_session.runSql("SELECT @@server_uuid AS uuid").fetchOneObject().uuid;
          x_session.close();
          if (server_uuid != x_server_uuid) {
            result.code = 1;
            result.message = 'The X Protocol plugin is enabled, however a different ' +
                            `server is listening for TCP connections at port ${x_port}` +
                            ', check the MySQL Server log for more details';
          } else {
            result.message = `The X Protocol plugin is already enabled and listening for connections on port ${x_port}`;
          }
        } catch (error) {
          result.code = 1;
          result.message = 'The X plugin is enabled, however failed to create a session using ' +
                    `port ${x_port}, check the MySQL Server log for more details`;
        }
      } else {
        result.code = 2;
        result.message = 'The X plugin is not enabled';
      }
    }
  } catch (error) {
    result.code = 3;
    result.message = `Error checking for X Protocol plugin: ${error["message"]}`;
  }

  return result;
}

function enableXProtocol() {
  println('enableXProtocol: Installing plugin mysqlx...');
  var row = session.runSql("select @@version_compile_os, substr(@@version, 1, instr(@@version, '-')-1)").fetchOne();
  var version = row[1].split(".");
  var vernum = parseInt(version[0]) * 10000 + parseInt(version[1]) * 100 + parseInt(version[2]);
  let error_str = '';

  if (row[0] == "Win32" || row[0] == "Win64") {
    var r = session.runSql("install plugin mysqlx soname 'mysqlx.dll';");
    if (vernum >= 80004)
      var r = session.runSql("install plugin mysqlx_cache_cleaner soname 'mysqlx.dll';");
  } else {
    var r = session.runSql("install plugin mysqlx soname 'mysqlx.so';")
    if (vernum >= 80004)
      var r = session.runSql("install plugin mysqlx_cache_cleaner soname 'mysqlx.so';");
  }
  println('enableXProtocol: Verifying plugin is active...')
  let active = session.runSql(`SELECT COUNT(*) AS active
                                FROM information_schema.plugins WHERE plugin_name='mysqlx'
                                AND plugin_status='ACTIVE'`).fetchOneObject().active;

  return error_str;
}

// Initial verification for X protocol state
let state = checkXProtocol(session);

// If it is OK or there was an error verifying there's nothing else to do.
if (state.code == 0 || state.code == 3) {
  println(`enableXProtocol: ${state.message}`);
} else {
  try {
    if (state.code == 2) {
      error = enableXProtocol();

      if (error == '') {
        state = checkXProtocol();
      }
    }

    switch (state.code) {
      case 0:
        println('enableXProtocol: successfully installed the X protocol plugin!');
        break;
      case 1:
        println(`enableXProtocol: WARNING: ${state.message}`);
        break;
      case 2:
        throw state.message;
        break;
      case 3:
        println(`enableXProtocol: Error verifying X protocol state: ${state.message}`);
        break;
    }
  } catch (error) {
    println('Error installing the X Plugin: '+ error['message']);
  }
}
println();
)*";
  // clang-format on
  std::stringstream stream(script);
  return shell->process_stream(stream, "(command line)", {});
}

// Execute a Administrative DB command passed from the command line via the
// --dba option Currently, only the enableXProtocol command is supported.
int execute_dba_command(std::shared_ptr<mysqlsh::Command_line_shell> shell,
                        const std::string &command) {
  if (command != "enableXProtocol") {
    mysqlsh::current_console()->raw_print(
        "Unsupported dba command " + command + "\n",
        mysqlsh::Output_stream::STDERR);
    return 1;
  }

  // this is a temporary solution, ideally there will be a dba object/module
  // that implements all commands are the requested command will be invoked as
  // dba.command() with param handling and others, from both interactive shell
  // and cmdline

  return enable_x_protocol(shell);
}

int execute_import_command(mysqlsh::Command_line_shell *shell,
                           const std::vector<std::string> &import_args,
                           const std::vector<std::string> &import_opts) {
  auto shell_session = shell->shell_context()->get_dev_session();

  if (!shell_session) {
    throw shcore::Exception::runtime_error(
        "Please connect the shell to the MySQL server.");
  }

  auto node_type = shell_session->get_node_type();
  if (node_type.compare("X") != 0) {
    throw shcore::Exception::runtime_error(
        "An X Protocol session is required for JSON import.");
  }

  mysqlshdk::db::Connection_options connection_options =
      shell_session->get_connection_options();

  std::shared_ptr<mysqlshdk::db::mysqlx::Session> xsession =
      mysqlshdk::db::mysqlx::Session::create();

  if (mysqlsh::current_shell_options()->get().trace_protocol) {
    xsession->enable_protocol_trace(true);
  }

  xsession->connect(connection_options);

  mysqlsh::Prepare_json_import prepare{xsession};
  const std::string &schema = shell_session->get_current_schema();
  prepare.schema(schema);
  prepare.use_stdin();

  switch (import_args.size()) {
    case 2:
      mysqlsh::current_console()->raw_print(
          "Target collection or table must be set if filename is a STDIN\n"
          "Usage: --import filename [collection] | [table [column]]\n",
          mysqlsh::Output_stream::STDERR);
      return 1;
    case 3: {
      const std::string &target_table = import_args.at(2);
      std::string type;
      try {
        shell_session->db_object_exists(type, target_table, schema);
      } catch (const mysqlshdk::db::Error &e) {
        // ignore errors generated by this call
      }
      if (type == "VIEW") {
        throw std::runtime_error(
            "'" + schema + "'.'" + target_table +
            "' is a view. Target must be table or collection.");
      } else if (type == "TABLE") {
        prepare.table(target_table);
      } else {
        prepare.collection(target_table);
      }
      break;
    }
    case 4: {
      prepare.table(import_args.at(2));
      prepare.column(import_args.at(3));
      break;
    }
    default:
      mysqlsh::current_console()->raw_print(
          "Usage: --import filename [collection] | [table [column]]\n",
          mysqlsh::Output_stream::STDERR);
      return 1;
  }

  auto importer = prepare.build();
  mysqlsh::current_console()->print_info(
      prepare.to_string() + " in MySQL Server at " +
      connection_options.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      "\n");
  importer.set_print_callback([](const std::string &msg) -> void {
    mysqlsh::current_console()->print(msg);
  });

  try {
    shcore::cli::Shell_cli_mapper mapper;
    for (auto &option : import_opts) {
      mapper.add_cmdline_argument(option);
    }

    shcore::Dictionary_t options = shcore::make_dict();
    for (const auto &arg : mapper.get_cmdline_args()) {
      options->set(arg.option, arg.value);
    }

    shcore::Document_reader_options roptions;
    shcore::Document_reader_options::options().unpack(options, &roptions);

    importer.load_from(roptions);
  } catch (...) {
    importer.print_stats();
    throw;
  }

  importer.print_stats();
  return 0;
}

// Detects whether the shell will be running in interactive mode or not
// Non interactive mode is used when:
// - A file is processed using the --file option
// - A file is processed through the OS redirection mechanism
// - It is an --import operation
// - It is a API CLI operation
// - stdin is not a tty
//
// Interactive mode is used when:
// - stdin is a tty
// - --interactive option is passed
//
// An error occurs when both --file and STDIN redirection are used
void detect_interactive(mysqlsh::Shell_options *options, bool *stdin_is_tty,
                        bool *stdout_is_tty) {
  assert(options);

  bool is_interactive = true;

  *stdin_is_tty = false;
  *stdout_is_tty = false;

  *stdin_is_tty = isatty(STDIN_FILENO) != 0;
  *stdout_is_tty = isatty(STDOUT_FILENO) != 0;

  if (!*stdin_is_tty || !*stdout_is_tty)
    is_interactive = false;
  else
    is_interactive = options->get().run_file.empty() &&
                     options->get().run_module.empty() &&
                     options->get().execute_statement.empty() &&
                     options->get().execute_dba_statement.empty() &&
                     options->get().import_args.empty() &&
                     !options->get_shell_cli_operation();

  // The --interactive option forces the shell to work emulating the
  // interactive mode no matter if:
  // - Input is being redirected from file
  // - Input is being redirected from STDIN
  // - It is not running on a terminal
  if (options->get().interactive) is_interactive = true;

  options->set_interactive(is_interactive);
}

static bool detect_color_capability() {
  mysqlshdk::textui::Color_capability color_mode = mysqlshdk::textui::Color_256;

  if (const char *force_mode = getenv("MYSQLSH_TERM_COLOR_MODE")) {
    if (strcmp(force_mode, "rgb") == 0) {
      color_mode = mysqlshdk::textui::Color_rgb;
    } else if (strcmp(force_mode, "256") == 0) {
      color_mode = mysqlshdk::textui::Color_256;
    } else if (strcmp(force_mode, "16") == 0) {
      color_mode = mysqlshdk::textui::Color_16;
    } else if (strcmp(force_mode, "nocolor") == 0) {
      color_mode = mysqlshdk::textui::No_color;
    } else if (strcmp(force_mode, "") != 0) {
      std::cout << "NOTE: MYSQLSH_TERM_COLOR_MODE environment variable set to "
                   "invalid value. Must be one of rgb, 256, 16, nocolor\n";
      return false;
    }
  } else {
#ifdef _WIN32
    bool vterm_supported = false;
#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    {
      DWORD mode = 0;
      GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode);

      if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) {
        vterm_supported = true;
      }
    }
#endif  // ENABLE_VIRTUAL_TERMINAL_PROCESSING

    if (!vterm_supported) {
      if (getenv("ANSICON")) {
        // ConEmu
        color_mode = mysqlshdk::textui::Color_rgb;
      } else {
        color_mode = mysqlshdk::textui::No_color;
      }
    } else {
      color_mode = mysqlshdk::textui::Color_rgb;
    }
#else   // !_WIN32
    const char *term = getenv("TERM");
    if (term) {
      if (shcore::str_endswith(term, "-256color") == 0) {
        color_mode = mysqlshdk::textui::Color_256;
      }
    } else {
      color_mode = mysqlshdk::textui::Color_16;
    }
#endif  // !_WIN32
  }

  mysqlshdk::textui::set_color_capability(color_mode);

  return true;
}

std::string pick_prompt_theme() {
  // check environment variable to override prompt theme
  if (char *theme = getenv("MYSQLSH_PROMPT_THEME")) {
    if (*theme) {
      if (!shcore::is_file(theme)) {
        const std::string prompt_theme_msg =
            "NOTE: MYSQLSH_PROMPT_THEME prompt theme file '" +
            std::string{theme} + "' does not exist.\n";
        mysqlsh::current_console()->print(prompt_theme_msg.c_str());
        return "";
      }
      log_debug("Using prompt theme file %s", theme);
    }
    return theme;
  }

  // check user overriden prompt theme
  std::string path =
      shcore::path::join_path(shcore::get_user_config_path(), "prompt.json");

  if (shcore::is_file(path)) {
    log_debug("Using prompt theme file %s", path.c_str());
    return path;
  }
  mysqlshdk::textui::Color_capability mode;
  mode = mysqlshdk::textui::get_color_capability();
  switch (mode) {
    case mysqlshdk::textui::Color_rgb:
    case mysqlshdk::textui::Color_256:
      path = "prompt_256.json";
      break;
    case mysqlshdk::textui::Color_16:
      path = "prompt_16.json";
      break;
    case mysqlshdk::textui::No_color:
      path = "prompt_nocolor.json";
      break;
    default:
      path = "prompt_classic.json";
      break;
  }

  path = shcore::path::join_path(shcore::get_share_folder(), "prompt", path);

  log_debug("Using prompt theme file %s", path.c_str());
  return path;
}

static std::string option_for(
    mysqlsh::Shell_options::Storage::Redirect_to target) {
  using Redirect_to = mysqlsh::Shell_options::Storage::Redirect_to;

  switch (target) {
    case Redirect_to::Primary:
      return "--redirect-primary";

    case Redirect_to::Secondary:
      return "--redirect-secondary";

    default:
      break;
  }

  throw std::logic_error("No option associated");
}

static void handle_redirect(
    const std::shared_ptr<mysqlsh::Command_line_shell> &shell,
    mysqlsh::Shell_options::Storage::Redirect_to target) {
  using Redirect_to = mysqlsh::Shell_options::Storage::Redirect_to;

  if (Redirect_to::None != target) {
    try {
      const auto secondary = Redirect_to::Secondary == target;

      if (!shell->redirect_session_if_needed(secondary)) {
        std::cerr << "NOTE: " << option_for(target)
                  << " ignored because target is already a "
                  << (secondary ? "SECONDARY" : "PRIMARY") << "\n";
      }
    } catch (...) {
      std::cerr << "While handling " << option_for(target) << ":\n";
      throw;
    }
  }
}

static std::string version_string(const char *argv0, bool extra) {
  std::string version_msg;
  version_msg.resize(1024);

  if (*MYSH_BUILD_ID && extra) {
    snprintf(&version_msg[0], version_msg.size(), "%s   %s - build %s", argv0,
             shcore::get_long_version(), MYSH_BUILD_ID);
    version_msg.resize(strlen(&version_msg[0]));
    if (*MYSH_COMMIT_ID) {
      version_msg.append(" - commit_id ");
      version_msg.append(MYSH_COMMIT_ID);
    }
  } else {
    snprintf(&version_msg[0], version_msg.size(), "%s   %s", argv0,
             shcore::get_long_version());
    version_msg.resize(strlen(&version_msg[0]));
  }
  return version_msg;
}

bool stdin_is_tty = false;
bool stdout_is_tty = false;

static std::shared_ptr<mysqlsh::Shell_options> process_args(int *argc,
                                                            char ***argv) {
#ifdef ENABLE_SESSION_RECORDING
  handle_debug_options(argc, argv);
#endif
  const auto user_config_path = []() -> std::string {
    try {
      return shcore::get_user_config_path();
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
      exit(1);
    }
  }();

  auto shell_options = std::make_shared<mysqlsh::Shell_options>(
      *argc, *argv, shcore::path::join_path(user_config_path, "options.json"));
  const mysqlsh::Shell_options::Storage &options = shell_options->get();

  detect_interactive(shell_options.get(), &stdin_is_tty, &stdout_is_tty);

  // If not a tty, then autocompletion can't be used, so we disable
  // name cache for autocompletion... but keep it for db object from DevAPI
  if (!options.db_name_cache_set &&
      (!options.interactive || !stdin_is_tty || !stdout_is_tty))
    shell_options->set_db_name_cache(false);

  // Switch default output format to tab separated instead of table
  if (!options.interactive &&
      shell_options->get_option_source(SHCORE_RESULT_FORMAT) ==
          shcore::opts::Source::Compiled_default)
    shell_options->set_result_format("tabbed");

  return shell_options;
}

static void init_shell(std::shared_ptr<mysqlsh::Command_line_shell> shell) {
  mysqlsh::global_init();

#ifdef ENABLE_SESSION_RECORDING
  init_debug_shell(shell);
#endif

  if (!shell->options().dbug_options.empty()) {
    const auto &opts = shell->options().dbug_options;
    if (opts[0] == '{' || opts[0] == '[') {
      // testutil.injectFault() style handled in debug_shell.cc
    } else {
      // dbug style
      DBUG_SET_INITIAL(opts.c_str());
    }
  }
}

static void finalize_shell(mysqlsh::Command_line_shell *shell) {
#ifdef ENABLE_SESSION_RECORDING
  finalize_debug_shell(shell);
#endif
  // Calls restore print to make the cached output to get printed
  shell->restore_print();

  // shell needs to be destroyed before global_end() is called, because it
  // needs to call destructors of JS contexts before V8 is shut down
  delete shell;

  mysqlsh::global_end();
}

#ifdef _WIN32
int wmain(int argc, wchar_t **wargv) {
  std::vector<std::string> sargv;
  std::vector<char *> cargv;

  sargv.reserve(argc);
  cargv.reserve(argc + 1);

  for (int i = 0; i < argc; ++i) {
    sargv.emplace_back(shcore::wide_to_utf8(wargv[i]));
    cargv.emplace_back(&sargv.back()[0]);
  }

  cargv.emplace_back(nullptr);

  char **argv = &cargv[0];

#else
int main(int argc, char **argv) {
#endif
  tzset();
  std::string mysqlsh_path = shcore::get_binary_path();
  g_mysqlsh_path = mysqlsh_path.c_str();

#ifdef _WIN32
  UINT origcp = GetConsoleCP();
  UINT origocp = GetConsoleOutputCP();

  // Enable UTF-8 input and output
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);

  auto restore_cp = shcore::on_leave_scope([origcp, origocp]() {
    // Restore original codepage
    SetConsoleCP(origcp);
    SetConsoleOutputCP(origocp);
  });

#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
  {
    // Try to enable VT100 escapes...
    // Supported in Windows 10 command window and some other terminals.
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(handle, &mode);
    SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
#endif  // ENABLE_VIRTUAL_TERMINAL_PROCESSING
#else
  auto locale = std::setlocale(LC_ALL, "en_US.UTF-8");
  // logger is not initialized yet here
  if (!locale)
    fprintf(stderr, "Cannot set LC_ALL to locale en_US.UTF-8: %s\n",
            strerror(errno));
  // set the environment variable as well, this ensures that locale is not
  // reset using setlocale(LC_XXX, "") call by any of our dependencies
  shcore::setenv("LC_ALL", "en_US.UTF-8");
#endif  // _WIN32

  // Has to be called once in main so internal static variable is properly set
  // with the main thread id.
  mysqlshdk::utils::in_main_thread();
  int ret_val = 0;
  Interrupt_helper sighelper;

  mysqlsh::Scoped_ssh_manager ssh_manager(
      std::make_shared<mysqlshdk::ssh::Ssh_manager>());
  mysqlsh::Scoped_interrupt interrupt_handler(
      shcore::Interrupts::create(&sighelper));

  std::shared_ptr<mysqlsh::Shell_options> shell_options =
      process_args(&argc, &argv);
  const mysqlsh::Shell_options::Storage &options = shell_options->get();

  if (options.exit_code != 0) return options.exit_code;

  mysqlsh::Scoped_shell_options scoped_shell_options(shell_options);

  std::shared_ptr<shcore::Logger> logger;
  try {
    // Setup logging
    logger = shcore::Logger::create_instance(
        options.log_file.empty() ? nullptr : options.log_file.c_str(),
        options.log_to_stderr, options.log_level);
  } catch (const std::exception &e) {
    fprintf(stderr, "%s\n", e.what());
    exit(1);
  }

  mysqlsh::Scoped_logger scoped_logger(logger);

  log_info("%s", version_string(argv[0], true).c_str());
  mysqlsh::Scoped_log_sql log_sql(std::make_shared<shcore::Log_sql>());
  shcore::current_log_sql()->push("main");

  std::shared_ptr<mysqlsh::Command_line_shell> shell;
#ifdef HAVE_PYTHON
  shcore::Scoped_callback cleanup([&shell] {
    shell.reset();
    shcore::Python_init_singleton::destroy_python();
  });
#endif

  try {
    bool interrupted = false;
    if (!options.interactive) {
      shcore::current_interrupt()->push_handler([&interrupted]() {
        interrupted = true;
        return false;
      });
    }

    bool valid_color_capability = detect_color_capability();

    // The Json_shell mode is enabled when this env variable is defined
    char *json_shell = getenv("MYSQLSH_JSON_SHELL");
    if (json_shell) {
      // The variable needs to be remvoved in case AAPI sandbox operations are
      // executed, this is because the launched shell instance will also use the
      // variable, breaking the output parsing
      shcore::unsetenv("MYSQLSH_JSON_SHELL");

      // When shell is running as MYSQL_JSON_SHELL binary data is truncated at
      // 257 bytes, eventually this should be determined by a shell command ilne
      // argument, i.e. --binary-limit
      shell_options.get()->set_binary_limit(256);
      shell.reset(new mysqlsh::Json_shell(shell_options), finalize_shell);
    } else {
      shell.reset(new mysqlsh::Command_line_shell(shell_options),
                  finalize_shell);
    }

    init_shell(shell);

    // Since log initialization errors are not critical but just warnings, they
    // get printed in a delayed way to have them properly formatted based on the
    // ourput format
    if (const auto warning = logger->get_initialization_warning();
        !warning.empty()) {
      mysqlsh::current_console()->print_warning(warning);
    }

#ifdef _WIN32
    Interrupt_windows_helper whelper;
#endif

    log_debug("Using color mode %i",
              static_cast<int>(mysqlshdk::textui::get_color_capability()));

    if (shell_options->action_print_version()) {
      std::string version_msg =
          version_string(argv[0], shell_options->action_print_version_extra());

#ifdef ENABLE_SESSION_RECORDING
      version_msg.append(" + session_recorder");
#endif
      mysqlsh::current_console()->println(version_msg);
      ret_val = options.exit_code;
    } else if (shell_options->action_print_help()) {
      shell->print_cmd_line_helper();
      ret_val = options.exit_code;
    } else {
      // The banner is printed only when in a real interactive session.
      if (options.interactive) {
        // The call to quiet_print will cause any information printed to be
        // cached.
        // When the interactive shell starts a restore_print will be called
        // which will cause the cached information to be printed unless
        // --quiet-start=2 was used.
        shell->quiet_print();

        // The shell banner is printed only if quiet-start was not used
        if (options.quiet_start == mysqlsh::Shell_options::Quiet_start::NOT_SET)
          shell->print_banner();
      }

      // Open the default shell session
      if (options.has_connection_data()) {
        try {
          auto restore_print_on_error =
              shcore::Scoped_callback([shell]() { shell->restore_print(); });

          mysqlshdk::db::Connection_options target =
              options.connection_options();

          if ((target.has_password() || options.is_mfa(true)) &&
              !options.no_password) {
            mysqlsh::current_console()->print_warning(
                "Using a password on the command line interface can be "
                "insecure.");
          }

          // Connect to the requested instance
          shell->connect(target, options.recreate_database);

          // If redirect is requested, then reconnect to the right instance
          handle_redirect(shell, options.redirect_session);
        } catch (const mysqlshdk::db::Error &e) {
          std::string error = "MySQL Error ";
          error.append(std::to_string(e.code()));

          if (e.sqlstate() && *e.sqlstate())
            error.append(" (").append(e.sqlstate()).append(")");

          error.append(": ").append(e.what());

          mysqlsh::current_console()->print_error(error);

          return 1;
        } catch (const shcore::Exception &e) {
          std::cerr << e.format() << "\n";
          return 1;
        } catch (const std::exception &e) {
          std::cerr << e.what() << "\n";
          return 1;
        }
      } else {
        shell->restore_print();

        if (mysqlsh::Shell_options::Storage::Redirect_to::None !=
            options.redirect_session) {
          mysqlsh::current_console()->print_error(
              "The " + option_for(options.redirect_session) +
              " option requires a session to a member of an InnoDB cluster or "
              "ReplicaSet.");
          return 1;
        }
      }

      try {
        // initialize globals requested via command line (i.e. --cluster,
        // --replicaset)
        shell->init_extra_globals();
      } catch (const shcore::Exception &e) {
        mysqlsh::current_console()->print_error(e.format());
        return 1;
      }

      if (valid_color_capability) shell->load_prompt_theme(pick_prompt_theme());

      const auto shell_cli_operation = shell_options->get_shell_cli_operation();

      if (shell_cli_operation) {
        try {
          shell->print_result(shell_cli_operation->execute());
        } catch (const std::invalid_argument &e) {
          mysqlsh::current_console()->print_error(e.what());
          ret_val = 10;
        } catch (const shcore::Error &e) {
          mysqlsh::current_console()->print_error(e.what());
          ret_val = 1;
        } catch (const std::exception &e) {
          mysqlsh::current_console()->print_error(e.what());
          ret_val = 1;
        }
      } else if (!options.execute_statement.empty()) {
        std::stringstream stream(options.execute_statement);
        ret_val = shell->process_stream(stream, "(command line)", {});
      } else if (!options.execute_dba_statement.empty()) {
        if (options.initial_mode != shcore::IShell_core::Mode::JavaScript &&
            options.initial_mode != shcore::IShell_core::Mode::None) {
          mysqlsh::current_console()->raw_print(
              "The --dba option can only be used in JavaScript mode\n",
              mysqlsh::Output_stream::STDERR);
          ret_val = 1;
        } else {
          ret_val = execute_dba_command(shell, options.execute_dba_statement);
        }
      } else if (!options.run_file.empty()) {
        ret_val = shell->process_file(options.run_file, options.script_argv);
      } else if (!options.run_module.empty()) {
        ret_val = shell->run_module(options.run_module, options.script_argv);
      } else if (!options.import_args.empty()) {
        ret_val = execute_import_command(shell.get(), options.import_args,
                                         options.import_opts);
      } else if (options.interactive) {
        shell->load_state();

        shell->pre_command_loop();

        shell->command_loop();

        shell->save_state();

        ret_val = 0;
      } else {
        ret_val = shell->process_stream(std::cin, "STDIN", {});
      }
    }
    if (interrupted) {
#ifdef _WIN32
      ret_val = 130;
#else
      signal(SIGINT, SIG_DFL);
      kill(getpid(), SIGINT);
#endif
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    ret_val = 1;
  }

  return ret_val;
}
