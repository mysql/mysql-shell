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

#include "modules/adminapi/common/dba_errors.h"
#include "modules/mod_utils.h"
#include "modules/util/json_importer.h"
#include "my_dbug.h"
#include "mysqlsh/cmdline_shell.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/document_parser.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/shell_init.h"

#ifdef WITH_OCI
#include "modules/util/oci_setup.h"
#endif

#include <sys/stat.h>
#include <clocale>
#include <cstdio>
#include <iostream>
#include <sstream>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef ENABLE_SESSION_RECORDING
void handle_debug_options(int *argc, char ***argv);
void init_debug_shell(std::shared_ptr<mysqlsh::Command_line_shell> shell);
void finalize_debug_shell(mysqlsh::Command_line_shell *shell);
#endif

const char *g_mysqlsh_path;
static mysqlsh::Command_line_shell *g_shell_ptr;

#ifdef WIN32
#include <windows.h>

class Interrupt_helper : public shcore::Interrupt_helper {
 public:
  void setup() override {
    // if we're being executed using CreateProcess() with
    // CREATE_NEW_PROCESS_GROUP flag set, an implicit call to
    // SetConsoleCtrlHandler(NULL, TRUE) is made, need to revert that
    SetConsoleCtrlHandler(nullptr, FALSE);
    // set our own handler
    SetConsoleCtrlHandler(windows_ctrl_handler, TRUE);
  }

  void block() override {}

  void unblock(bool) override {}

 private:
  static BOOL windows_ctrl_handler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
      case CTRL_C_EVENT:
      case CTRL_BREAK_EVENT:
        interrupt();
        // Don't let the default handler terminate us
        return TRUE;
      case CTRL_CLOSE_EVENT:
      case CTRL_LOGOFF_EVENT:
      case CTRL_SHUTDOWN_EVENT:
        // TODO: Add proper exit handling if needed
        break;
    }
    // Pass signal to the next control handler function.
    return FALSE;
  }

  static void interrupt() {
    try {
      // we're being called from another thread, first notify the interrupt
      // handlers, so they update their state before main thread resumes
      shcore::Interrupts::interrupt();
      // notify the event, potentially waking up the main thread
      shcore::Sigint_event::get().notify();
    } catch (const std::exception &e) {
      log_error("Unhandled exception in SIGINT handler: %s", e.what());
    }
  }
};

#else  // !WIN32

#include <signal.h>

/**
SIGINT signal handler.

@description
This function handles SIGINT (Ctrl-C). It will call a shell function
which should handle the interruption in an appropriate fashion.
If the interrupt() function returns false, it will interret as a signal
that the shell itself should abort immediately.

@param [IN]               Signal number
*/

static void handle_ctrlc_signal(int /* sig */) {
  int errno_save = errno;
  try {
    shcore::Interrupts::interrupt();
  } catch (const std::exception &e) {
    log_error("Unhandled exception in SIGINT handler: %s", e.what());
  }
  if (shcore::Interrupts::propagates_interrupt()) {
    // propagate the ^C to the caller of the shell
    // this is the usual handling when we're running in batch mode
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
  }
  errno = errno_save;
}

static void install_signal_handler() {
  // install signal handler
  signal(SIGINT, handle_ctrlc_signal);
  // allow system calls to be interrupted
  siginterrupt(SIGINT, 1);
  // Ignore broken pipe signal
  signal(SIGPIPE, SIG_IGN);
}

class Interrupt_helper : public shcore::Interrupt_helper {
 public:
  virtual void setup() { install_signal_handler(); }

  virtual void block() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);
  }

  virtual void unblock(bool clear_pending) {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);

    if (clear_pending && sigpending(&sigset)) {
      struct sigaction ign, old;
      // set to ignore SIGINT
      ign.sa_handler = SIG_IGN;
      ign.sa_flags = 0;
      sigaction(SIGINT, &ign, &old);
      // unblock and let SIGINT be delivered (and ignored)
      sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
      // restore original SIGINT handler
      sigaction(SIGINT, &old, nullptr);
    } else {
      sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
    }
  }
};

#endif  //! WIN32

static int enable_x_protocol(
    std::shared_ptr<mysqlsh::Command_line_shell> shell) {
  // clang-format off
  static const char *script =
      R"*(function enableXProtocol() {
  try {
    if (session.uri.indexOf("mysqlx://") == 0) {
      var mysqlx_port = session.sql('select @@mysqlx_port').execute().fetchOne();
      println('enableXProtocol: X Protocol plugin is already enabled and listening for connections on port '+mysqlx_port[0]);
      return 0;
    }
    var mysqlx_port = session.runSql('select @@mysqlx_port').fetchOne();
    println('enableXProtocol: X Protocol plugin is already enabled and listening for connections on port '+mysqlx_port[0]);
    return 0;
  } catch (error) {
    if (error["code"] != 1193) { // unknown system variable
      println('enableXProtocol: Error checking for X Protocol plugin: '+error["message"]);
      return 1;
    }
  }
  println('enableXProtocol: Installing plugin mysqlx...');
  var row = session.runSql("select @@version_compile_os, substr(@@version, 1, instr(@@version, '-')-1)").fetchOne();
  var version = row[1].split(".");
  var vernum = parseInt(version[0]) * 10000 + parseInt(version[1]) * 100 + parseInt(version[2]);
  try {
    if (row[0] == "Win32" || row[0] == "Win64") {
      var r = session.runSql("install plugin mysqlx soname 'mysqlx.dll';");
      if (vernum >= 80004)
        var r = session.runSql("install plugin mysqlx_cache_cleaner soname 'mysqlx.dll';");
    } else {
      var r = session.runSql("install plugin mysqlx soname 'mysqlx.so';")
      if (vernum >= 80004)
        var r = session.runSql("install plugin mysqlx_cache_cleaner soname 'mysqlx.so';");
    }
    println("enableXProtocol: done");
  } catch (error) {
    println('enableXProtocol: Error installing the X Plugin: '+error['message']);
  }
}
enableXProtocol();
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
    shcore::Document_reader_options roptions;

    shcore::Dictionary_t options = shcore::make_dict();
    for (auto &option : import_opts)
      shcore::Shell_cli_operation::add_option(options, option);

    shcore::Option_unpacker unpacker(options);
    mysqlsh::unpack_json_import_flags(&unpacker, &roptions);
    unpacker.end();

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
  std::string path = shcore::get_user_config_path();
  path.append("prompt.json");

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

static int handle_redirect(std::shared_ptr<mysqlsh::Command_line_shell> shell,
                           const mysqlsh::Shell_options::Storage &options) {
  switch (options.redirect_session) {
    case mysqlsh::Shell_options::Storage::None:
      break;
    case mysqlsh::Shell_options::Storage::Primary: {
      try {
        if (!shell->redirect_session_if_needed(false)) {
          std::cerr << "NOTE: --redirect-primary ignored because target is "
                       "already a PRIMARY\n";
        }
      } catch (const shcore::Exception &e) {
        std::cerr << "While handling --redirect-primary:\n";
        if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
          std::cerr << "ERROR: The cluster appears to be under a partial "
                       "or total outage and the PRIMARY cannot be "
                       "selected.\n"
                    << e.what() << "\n";
          return 1;
        }
        throw;
      } catch (...) {
        std::cerr << "While handling --redirect-primary:\n";
        throw;
      }
      break;
    }
    case mysqlsh::Shell_options::Storage::Secondary: {
      try {
        if (!shell->redirect_session_if_needed(true)) {
          std::cerr << "NOTE: --redirect-secondary ignored because target is "
                       "already a SECONDARY\n";
        }
      } catch (const shcore::Exception &e) {
        std::cerr << "While handling --redirect-secondary:\n";
        if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
          std::cerr << "ERROR: The cluster appears to be under a partial "
                       "or total outage and an ONLINE SECONDARY cannot "
                       "be selected.\n"
                    << e.what() << "\n";
          return 1;
        }
        throw;
      } catch (...) {
        std::cerr << "While handling --redirect-secondary:\n";
        throw;
      }
      break;
    }
  }
  return 0;
}

static void show_cluster_info(
    std::shared_ptr<mysqlsh::Command_line_shell> /* shell */,
    std::shared_ptr<mysqlsh::dba::Cluster> cluster) {
  // cluster->diagnose();
  auto console = mysqlsh::current_console();
  console->println("You are connected to a member of cluster '" +
                   cluster->impl()->get_name() + "'.");
  console->println(
      "Variable 'cluster' is set.\nUse cluster.status() in scripting mode to "
      "get status of this cluster or cluster.help() for more commands.");
}

bool stdin_is_tty = false;
bool stdout_is_tty = false;

static std::shared_ptr<mysqlsh::Shell_options> process_args(int *argc,
                                                            char ***argv) {
#ifdef ENABLE_SESSION_RECORDING
  handle_debug_options(argc, argv);
#endif
  auto shell_options = std::make_shared<mysqlsh::Shell_options>(
      *argc, *argv,
      shcore::path::join_path(shcore::get_user_config_path(), "options.json"));
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
    DBUG_SET_INITIAL(shell->options().dbug_options.c_str());
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

int main(int argc, char **argv) {
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
  if (!locale) log_error("Failed to set locale to en_US.UTF-8");
  // set the environment variable as well, this ensures that locale is not
  // reset using setlocale(LC_XXX, "") call by any of our dependencies
  shcore::setenv("LC_ALL", "en_US.UTF-8");
#endif  // _WIN32

  int ret_val = 0;
  Interrupt_helper sighelper;
  shcore::Interrupts::init(&sighelper);

  std::shared_ptr<mysqlsh::Shell_options> shell_options =
      process_args(&argc, &argv);
  const mysqlsh::Shell_options::Storage &options = shell_options->get();

  if (options.exit_code != 0) return options.exit_code;

  // Setup logging
  {
    std::string log_path =
        shcore::path::join_path(shcore::get_user_config_path(), "mysqlsh.log");
    shcore::Logger::setup_instance(log_path.c_str(), options.log_to_stderr,
                                   options.log_level);
  }

  std::shared_ptr<mysqlsh::Command_line_shell> shell;
  try {
    bool interrupted = false;
    if (!options.interactive) {
      shcore::Interrupts::push_handler([&interrupted]() {
        interrupted = true;
        return false;
      });
    }

    bool valid_color_capability = detect_color_capability();

    shell.reset(new mysqlsh::Command_line_shell(shell_options), finalize_shell);

    init_shell(shell);

    log_debug("Using color mode %i",
              static_cast<int>(mysqlshdk::textui::get_color_capability()));

    std::string version_msg;
    version_msg.resize(1024);

    if (shell_options->action_print_version()) {
      if (*MYSH_BUILD_ID && shell_options->action_print_version_extra()) {
        snprintf(&version_msg[0], version_msg.size(), "%s   %s - build %s",
                 argv[0], shcore::get_long_version(), MYSH_BUILD_ID);
        version_msg.resize(strlen(&version_msg[0]));
        if (*MYSH_COMMIT_ID) {
          version_msg.append(" - commit_id ");
          version_msg.append(MYSH_COMMIT_ID);
        }
      } else {
        snprintf(&version_msg[0], version_msg.size(), "%s   %s", argv[0],
                 shcore::get_long_version());
        version_msg.resize(strlen(&version_msg[0]));
      }
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

          if (target.has_password()) {
            mysqlsh::current_console()->print_warning(
                "Using a password on the command line interface can be "
                "insecure.");
          }

          // Connect to the requested instance
          shell->connect(target, options.recreate_database);

          // If redirect is requested, then reconnect to the right instance
          ret_val = handle_redirect(shell, options);
          if (ret_val != 0) return ret_val;
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
          ret_val = 1;
          goto end;
        }
      } else {
        shell->restore_print();
        if (options.redirect_session) {
          mysqlsh::current_console()->print_error(
              "--redirect option requires a session to a member of an InnoDB "
              "cluster");
          return 1;
        }
      }

      std::shared_ptr<mysqlsh::dba::Cluster> default_cluster;

#ifdef WITH_OCI
      if (options.oci_wizard) {
        if (options.interactive) {
          mysqlsh::oci::load_profile(options.oci_profile,
                                     shell->shell_context());
        } else {
          mysqlsh::current_console()->print_warning(
              "Option --oci requires interactive mode, ignoring option.");
        }
      }
#endif

      // If default cluster specified on the cmdline, set cluster global var

      auto shell_cli_operation = shell_options->get_shell_cli_operation();
      if (options.default_cluster_set && !shell_cli_operation) {
        try {
          default_cluster = shell->set_default_cluster(options.default_cluster);
        } catch (const shcore::Exception &e) {
          mysqlsh::current_console()->print_warning(
              "Option --cluster requires a session to a member of a InnoDB "
              "cluster.");
          mysqlsh::current_console()->print_error(e.format());
          return 1;
        }
      }

      g_shell_ptr = shell.get();
      if (valid_color_capability) shell->load_prompt_theme(pick_prompt_theme());

      if (shell_cli_operation && !shell_cli_operation->empty()) {
        try {
          shell->print_result(shell_cli_operation->execute());
        } catch (const shcore::Shell_cli_operation::Mapping_error &e) {
          mysqlsh::current_console()->print_error(e.what());
          ret_val = 10;
        } catch (const shcore::Exception &e) {
          mysqlsh::current_console()->print_value(shcore::Value(e.error()),
                                                  "error");
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
      } else if (!options.import_args.empty()) {
        ret_val = execute_import_command(shell.get(), options.import_args,
                                         options.import_opts);
      } else if (options.interactive) {
        shell->load_state();

        if (default_cluster) {
          show_cluster_info(shell, default_cluster);
        }

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

end:
  return ret_val;
}
