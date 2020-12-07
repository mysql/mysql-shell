/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates.
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

#include "unittest/test_utils/mod_testutils.h"

#include <cassert>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <system_error>
#include <utility>

#ifdef _WIN32
// clang-format off
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Iphlpapi.h>
#include <windows.h>
// clang-format on
// for GetIpAddrTable()
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/include/scripting/obj_date.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_process.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"
#ifndef ENABLE_SESSION_RECORDING
#include "unittest/test_utils.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/shell_test_env.h"
#endif
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "mysqlshdk/libs/mysql/lock_service.h"
#include "mysqlshdk/libs/storage/backend/oci_object_storage.h"
#include "mysqlshdk/libs/storage/compressed_file.h"

// clang-format off
#ifndef _WIN32
#include <net/if.h>
#endif
// clang-format on

// TODO(anyone)
// - make destroySandbox() expect that the final state of the sandbox is the
//   same as in the beginning
// - make a deployCluster() with reusable cluster
// - make a destroyCluster() that expects the cluster is in the same state
//   as original

extern int g_test_trace_scripts;
extern int g_test_color_output;
extern bool g_bp;

namespace tests {
constexpr int k_wait_sandbox_dead_timeout = 120;
constexpr int k_wait_sandbox_alive_timeout = 120;
constexpr int k_wait_repl_connection_error = 300;
constexpr int k_wait_member_timeout = 120;
constexpr int k_wait_delayed_gr_start_timeout = 300;
const char *k_boilerplate_root_password = "root";

Testutils::Testutils(const std::string &sandbox_dir, bool dummy_mode,
                     std::shared_ptr<mysqlsh::Command_line_shell> shell,
                     const std::string &mysqlsh_path)
    : mysqlsh::Extensible_object("testutil", "testutil", true),
      _shell(shell),
      _mysqlsh_path(mysqlsh_path) {
  _use_boilerplate = true;
  // When boilerplate is created, _sandbox_dir is replaced in start/stop script
  // files with placeholders. It cannot end with a path separator character,
  // because this may result in an unusable script.
  _sandbox_dir = shcore::str_rstrip(sandbox_dir, "\\/");
  _skip_server_interaction = dummy_mode;
  if (g_test_trace_scripts > 0 && dummy_mode)
    std::cerr << "testutils using dummy sandboxes\n";

  expose("deploySandbox", &Testutils::deploy_sandbox, "port", "rootpass",
         "?myCnfOptions", "?options");
  expose("deployRawSandbox", &Testutils::deploy_raw_sandbox, "port", "rootpass",
         "?myCnfOptions", "?options");
  expose("destroySandbox", &Testutils::destroy_sandbox, "port", "?quiet_kill",
         false);
  expose("startSandbox", &Testutils::start_sandbox, "port", "?options");
  expose("stopSandbox", &Testutils::stop_sandbox, "port", "?options");
  expose("killSandbox", &Testutils::kill_sandbox, "port", "?quiet", false);
  expose("restartSandbox", &Testutils::restart_sandbox, "port");
  expose("waitSandboxAlive", &Testutils::wait_sandbox_alive, "port_or_uri");
  expose("snapshotSandboxConf", &Testutils::snapshot_sandbox_conf, "port");
  expose("beginSnapshotSandboxErrorLog",
         &Testutils::begin_snapshot_sandbox_error_log, "port");
  expose("endSnapshotSandboxErrorLog",
         &Testutils::end_snapshot_sandbox_error_log, "port");
  expose("changeSandboxConf", &Testutils::change_sandbox_conf, "port", "option",
         "value", "?section");
  expose("upgradeSandbox", &Testutils::upgrade_sandbox, "port");
  expose("removeFromSandboxConf", &Testutils::remove_from_sandbox_conf, "port",
         "option", "?section");
  expose("getSandboxConfPath", &Testutils::get_sandbox_conf_path, "port");
  expose("getSandboxLogPath", &Testutils::get_sandbox_log_path, "port");
  expose("getSandboxPath", &Testutils::get_sandbox_path, "?port", "?filename");

  expose("isTcpPortListening", &Testutils::is_tcp_port_listening, "host",
         "port");

  expose("dumpData", &Testutils::dump_data, "uri", "filename", "schemas",
         "?options");
  expose("importData", &Testutils::import_data, "uri", "filename",
         "?default_schema", "?default_charset");

  expose("getShellLogPath", &Testutils::get_shell_log_path);

  expose("waitMemberState", &Testutils::wait_member_state, "port", "states",
         "?direct");
  expose("waitMemberTransactions", &Testutils::wait_member_transactions,
         "dest_port", "?source_port");
  expose("waitForReplConnectionError",
         &Testutils::wait_for_repl_connection_error, "port", "?channel",
         "group_replication_recovery");
  expose("waitForRplApplierError", &Testutils::wait_for_rpl_applier_error,
         "port", "?channel", "");

  expose("expectPrompt", &Testutils::expect_prompt, "prompt", "value");
  expose("expectPassword", &Testutils::expect_password, "prompt", "value");
  expose("assertNoPrompts", &Testutils::assert_no_prompts);
  expose("fetchCapturedStdout", &Testutils::fetch_captured_stdout, "?eatOne");
  expose("fetchCapturedStderr", &Testutils::fetch_captured_stderr, "?eatOne");
#ifndef ENABLE_SESSION_RECORDING
  expose("fetchDbaSqlLog", &Testutils::fetch_dba_sql_log, "?flush");
#endif

  expose("callMysqlsh", &Testutils::call_mysqlsh, "args", "?stdInput", "?envp",
         "?execPath");
  expose("callMysqlshAsync", &Testutils::call_mysqlsh_async, "args",
         "?stdInput", "?envp", "?execPath");
  expose("waitMysqlshAsync", &Testutils::wait_mysqlsh_async, "pid", "?seconds",
         60);

  expose("makeFileReadOnly", &Testutils::make_file_readonly, "path");
  expose("grepFile", &Testutils::grep_file, "path", "pattern");
  expose("catFile", &Testutils::cat_file, "path");
  expose("wipeFileContents", &Testutils::wipe_file_contents, "path");
  expose("preprocessFile", &Testutils::preprocess_file, "inPath", "variables",
         "outPath", "?skipSections");

  expose("isReplaying", &Testutils::is_replaying);
  expose("fail", &Testutils::fail, "context");
  expose("skip", &Testutils::skip, "reason");
  expose("versionCheck", &Testutils::version_check, "v1", "op", "v2");
  expose("waitForDelayedGRStart", &Testutils::wait_for_delayed_gr_start, "port",
         "rootpass", "?timeout",
         static_cast<int>(k_wait_delayed_gr_start_timeout));
  expose("mkdir", &Testutils::mk_dir, "name", "?recursive");
  expose("rmdir", &Testutils::rm_dir, "name", "?recursive");
  expose("rename", &Testutils::rename, "path", "newpath");
  expose("chmod", &Testutils::ch_mod, "path", "mode");
  expose("cpfile", &Testutils::cp_file, "source", "target");
  expose("rmfile", &Testutils::rm_file, "target");
  expose("touch", &Testutils::touch, "file");
  expose("createFile", &Testutils::create_file, "path", "content");
  expose("enableExtensible", &Testutils::enable_extensible);
  expose("dbugSet", &Testutils::dbug_set, "dbug");
  expose("dprint", &Testutils::dprint, "s");
  expose("setTrap", &Testutils::set_trap, "type", "?conditions", "?options");
  expose("resetTraps", &Testutils::reset_traps, "?type");
  expose("clearTraps", &Testutils::clear_traps, "?type");
  expose("getUserConfigPath", &Testutils::get_user_config_path);
  expose("setenv", &Testutils::setenv, "variable", "?value");
  expose("getCurrentMetadataVersion",
         &Testutils::get_current_metadata_version_string);
  expose("getInstalledMetadataVersion",
         &Testutils::get_installed_metadata_version_string);
  expose("wipeAllOutput", &Testutils::wipe_all_output);

  expose("getExclusiveLock", &Testutils::get_exclusive_lock, "classic_session",
         "name_space", "name", "?timeout");
  expose("getSharedLock", &Testutils::get_shared_lock, "classic_session",
         "name_space", "name", "?timeout");
  expose("releaseLocks", &Testutils::release_locks, "classic_session",
         "name_space");

  // expose("slowify", &Testutils::slowify, "port", "start");
  expose("bp", &Testutils::bp, "flag");

  expose("sslCreateCa", &Testutils::ssl_create_ca, "name");
  expose("sslCreateCerts", &Testutils::ssl_create_certs, "sbport", "caname",
         "servercn", "clientcn");

  expose("validateOciConfig", &Testutils::validate_oci_config);
  expose("getOciConfig", &Testutils::get_oci_config);
  expose("uploadOciObject", &Testutils::upload_oci_object, "bucket", "object",
         "filePath");
  expose("downloadOciObject", &Testutils::download_oci_object, "namespace",
         "bucket", "object", "filePath");
  expose("createOciObject", &Testutils::create_oci_object, "bucket", "name",
         "content");
  expose("deleteOciObject", &Testutils::delete_oci_object, "bucket", "name");
  expose("anycopy", &Testutils::anycopy, "from", "to");
}

//!<  @name Testing Utilities
///@{
/**
 * Gets the metadata version supported by this version of the shell
 */
#if DOXYGEN_JS
String Testutils::getCurrentMetadataVersion();
#elif DOXYGEN_PY
str Testutils::get_current_metadata_version();
#endif
///@}
std::string Testutils::get_current_metadata_version_string() {
  return mysqlsh::dba::metadata::current_version().get_base();
}

//!<  @name Testing Utilities
///@{
/**
 * Gets the metadata version available on the instance where the global session
 * is connected.
 */
#if DOXYGEN_JS
String Testutils::getInstalledMetadataVersion();
#elif DOXYGEN_PY
str Testutils::get_installed_metadata_version();
#endif
///@}
std::string Testutils::get_installed_metadata_version_string() {
  if (auto shell = _shell.lock()) {
    auto dev_session = shell->shell_context()->get_dev_session();
    if (!dev_session) throw std::runtime_error("No active shell session");
    auto session = dev_session->get_core_session();
    if (!session || !session->is_open()) {
      throw std::logic_error(
          "The testutil.getInstalledMetadataVersion method uses the active "
          "shell session and requires it to be open.");
    } else {
      return mysqlsh::dba::metadata::installed_version(
                 std::make_shared<mysqlsh::dba::Instance>(session))
          .get_base();
    }
  } else {
    throw std::logic_error("Lost reference to shell object");
  }

  return "";
}

std::string Testutils::get_mysqld_version(const std::string &mysqld_path) {
  std::string mysqld_active_path{"mysqld"};
  if (!mysqld_path.empty()) mysqld_active_path = mysqld_path;

  const char *argv[] = {mysqld_active_path.c_str(), "--version", nullptr};
  std::string version = mysqlshdk::utils::run_and_catch_output(argv, true);
  if (!version.empty()) {
    auto tokens = shcore::str_split(version, " ", -1, true);
    if (tokens[1] == "Ver") {
      return tokens[2];
    }
  }
  return "";
}

void Testutils::dbug_set(const std::string &s) {
  (void)s;
  DBUG_SET(s.c_str());
}

void Testutils::bp(bool flag) {
  // Use for setting conditional breakpoints, like:
  //
  // In gdb/lldb:
  // break set -n __cxa_throw -c g_bp
  // And in the test:
  // lots of code
  // testutil.bp(true);
  // code where bp should be enabled
  g_bp = flag;
}

/** Sets up a debug trap with the given conditions and action.
 *
 * @param type type of trap
 * @param condition array of conditions to match for the trap to be triggered
 * @param options options for the trap
 *
 * ## The following conditions are supported:
 * opt == str
 * opt != str
 * opt !regex str
 * opt regex pattern
 * opt > int
 * ++match_counter == int
 * ++match_counter > int
 *
 * opt is one of the condition options defined by the trap, described below.
 * Conditions are evaluated in the order they appear. Evaluation stops if
 * a condition is false.
 *
 * ++match_counter is a counter that increments every time it's evaluated.
 *
 * ## The following types of debug traps are supported:
 *
 * ### mysql, mysqlx
 * Traps that stop execution when a SQL stmt is executed in a classic or X
 * protocol session.
 *
 * #### Condition options:
 * sql - the sql statement about to be executed
 * uri - uri_endpoint() of the target server
 *
 * #### Trap options:
 * abort:int - if present and != 0, it will abort() on match, optional
 * code:int - MySQL error code to be reported, optional
 * msg:string - MySQL error msg to be reported, required unless abort:1
 *
 * If only msg is given, a std::logic_error will be thrown with the given msg,
 * instead of a db::Error.
 */
void Testutils::set_trap(const std::string &type,
                         const shcore::Array_t &conditions,
                         const shcore::Dictionary_t &options) {
#ifdef DBUG_OFF
  (void)type;
  (void)conditions;
  (void)options;
  throw std::logic_error("set_trap() not available in DBUG_OFF builds");
#else
  mysqlshdk::utils::FI::Conditions conds;

  if (conditions) {
    for (const auto &cond : *conditions) {
      conds.add(cond.as_string());
    }
  }

  mysqlshdk::utils::FI::Trap_options opts;
  if (options) {
    for (const auto &opt : *options) {
      opts[opt.first] = opt.second.descr();
    }
  }

  mysqlshdk::utils::FI::set_trap(type, conds, opts);
#endif
}

void Testutils::clear_traps(const std::string &type) {
#ifdef DBUG_OFF
  (void)type;
  throw std::logic_error("clear_traps() not available in DBUG_OFF builds");
#else
  mysqlshdk::utils::FI::clear_traps(type);
#endif
}

void Testutils::reset_traps(const std::string &type) {
#ifdef DBUG_OFF
  (void)type;
  throw std::logic_error("reset_traps() not available in DBUG_OFF builds");
#else
  mysqlshdk::utils::FI::reset_traps(type);
#endif
}

void Testutils::enable_extensible() {
  register_help("Gives access to general testing functions and properties.",
                {"Gives access to general testing functions and properties."},
                true);

  register_function_help(
      "registerModule", "Registers a test module.",
      {"@param parent The module that will contain the new module.",
       "@param name The name of the new module.",
       "@param options Optional options with help information for the "
       "module."},
      {"Registers a new test module into an existing test module.",
       "The parent module should be an existing module, in the format of: "
       "testutil[.<name>]*]",
       "The name should be a valid identifier.",
       "The object will be registered using the same name in both JavaScript "
       "and Python.",
       "The options parameter accepts the following options:",
       "@li brief string. Short description of the new module.",
       "@li details array. Detailed description of the module.",
       "Each entry in the details array will be turned into a paragraph on "
       "the module help.",
       "Only strings are allowed in the details array."});

  register_function_help(
      "registerFunction", "Registers a test utility function.",
      {"@param parent The module that will contain the new function.",
       "@param name The name of the new function in camelCase format.",
       "@param function The function callback to be executed when the new "
       "function is called.",
       "@param definition Optional Options containing additional function "
       "definition details."},
      {"Registers a new function into an existing test module.",
       "The name should be a valid identifier.",
       "It should be an existing module, in the format of: "
       "testutil[.<name>]*]",
       "The function will be registered following the respective naming "
       "convention for JavaScript and Python.",
       "The definition parameter accepts the following options:",
       "@li brief string. Short description of the new function.",
       "@li details array. Detailed description of the new function.",
       "@li parameters array. List of parameters the new function receives.",
       "Each entry in the details array will be turned into a paragraph on "
       "the module help.",
       "Only strings are allowed in the details array.",
       "Each parameter is defined as a dictionary, the following properties "
       "are allowed:",
       "@li name: defines the name of the parameter",
       "@li brief: a short description of the parameter",
       "@li details: an array with the detailed description of the parameter",
       "@li type: the data type of the parameter",
       "@li required: a boolean indicating if the parameter is required or "
       "not",
       "The supported parameter types are:",
       "@li string",
       "@li integer",
       "@li float",
       "@li array",
       "@li object",
       "@li dictionary",
       "Parameters of type 'string' may contain a 'values' property with the "
       "list of values allowed for the parameter.",
       "Parameters of type 'object' may contain either:",
       "@li a 'class' property with the type of object the parameter must "
       "be.",
       "@li a 'classes' property with a list of types of objects that the "
       "parameter can be.",
       "Parameters of type 'dictionary' may contain an 'options' list "
       "defining the options that are allowed for the parameter.",
       "Each option on the 'options' list follows the same structure as the "
       "parameter definition."});

  // Sample dynamic object created from C++
  // This object will be hun under testutils and will be used for testing
  // purposes
  register_object(
      "sampleModuleJS", "Sample module exported from C++",
      {"Exploring the posibility to dynamically create objects fron C++"},
      "module");

  register_object(
      "sampleModulePY", "Sample module exported from C++",
      {"Exploring the posibility to dynamically create objects fron C++"},
      "module");
}

namespace {
/** Iterates through all the file lines calling the given function on each of
 * them.
 * @param file Path to the file whose lines we want to process.
 * @param functor Function that is called on each of the file's lines
 */
void process_file_lines(
    const std::string &file,
    const std::function<std::string(const std::string &line)> &functor) {
  std::vector<std::string> lines;
  std::ifstream ifile(file);
  if (!ifile.good()) {
    throw std::runtime_error("Could not open file '" + file +
                             "': " + shcore::errno_to_string(errno));
  }

  {
    std::string line;
    while (std::getline(ifile, line)) {
      lines.push_back(functor(line));
    }
  }
  ifile.close();

  std::ofstream ofile(file);

  if (!ofile.good()) {
    throw std::runtime_error("Could not open file '" + file +
                             "': " + shcore::errno_to_string(errno));
  }

  for (const auto &line : lines) {
    ofile << line << '\n';
  }

  ofile.close();
}

void replace_file_text(const std::string &file, const std::string &from,
                       const std::string &to) {
  process_file_lines(file, [&from, &to](const std::string &line) {
    return shcore::str_replace(line, from, to);
  });
}
}  // namespace

void Testutils::set_sandbox_snapshot_dir(const std::string &dir) {
  _sandbox_snapshot_dir = dir;
}

void Testutils::set_test_execution_context(const std::string &file, int line,
                                           Shell_test_env *env) {
  _test_file = file;
  _test_line = line;
  _test_env = env;
}

//!<  @name Sandbox Operations
///@{
/**
 * Gets the path to the configuration file for the specific sandbox.
 * @param port The port of the sandbox owning the configuration file being
 * searched.
 *
 * This function will return the path to the configuration file for the sandbox
 * listening at the specified port.
 */
#if DOXYGEN_JS
String Testutils::getSandboxConfPath(Integer port);
#elif DOXYGEN_PY
str Testutils::get_sandbox_conf_path(int port);
#endif
///@}
std::string Testutils::get_sandbox_conf_path(int port) {
  return shcore::path::join_path(
      {_sandbox_dir, std::to_string(port), "my.cnf"});
}

std::string Testutils::get_sandbox_datadir(int port) {
  std::string path =
      shcore::path::join_path(_sandbox_dir, std::to_string(port), "data");
  if (!shcore::is_folder(path))
    path = shcore::path::join_path(_sandbox_dir, std::to_string(port),
                                   "sandboxdata");
  return path;
}

//!<  @name Sandbox Operations
///@{
/**
 * Gets the path to the error log for the specific sandbox.
 * @param port The port of the sandbox which error log file path will be
 * retrieved.
 *
 * This function will return the path to the error log for the sandbox
 * listening at the specified port.
 */
#if DOXYGEN_JS
String Testutils::getSandboxLogPath(Integer port);
#elif DOXYGEN_PY
str Testutils::get_sandbox_log_path(int port);
#endif
///@}
std::string Testutils::get_sandbox_log_path(int port) {
  return shcore::path::join_path(get_sandbox_datadir(port), "error.log");
}

//!<  @name Sandbox Operations
///@{
/**
 * Gets the path to any file contained on the sandbox datadir.
 * @param port Optional port of the sandbox for which a path will be retrieved.
 * @param name Optional name of the file path to be retrieved.
 *
 * This function will return path related to the sandboxes.
 *
 * If port is NOT specified, it will retrieve the path to the sandbox home
 * folder.
 *
 * If port is specified it will retrieve a path for that specific sandbox.
 *
 * If file is not specified, it will retrieve the path to the sandbox folder.
 *
 * If file is specified it will retrieve the path to that file only if it is a
 * a valid file:
 * - my.cnf is a valid file.
 * - Any other file, will be valid if it exists on the sandboxdata dir.
 */
#if DOXYGEN_JS
String Testutils::getSandboxPath(Integer port, String name);
#elif DOXYGEN_PY
str Testutils::get_sandbox_path(int port, str name);
#endif
///@}
std::string Testutils::get_sandbox_path(int port, const std::string &file) {
  std::string path;

  if (port == 0) {
    path = _sandbox_dir;
  } else {
    if (file.empty()) {
      path = shcore::path::join_path({_sandbox_dir, std::to_string(port)});
    } else {
      if (file == "my.cnf") {
        path = shcore::path::join_path(
            {_sandbox_dir, std::to_string(port), "my.cnf"});
      } else {
        path = shcore::path::join_path(get_sandbox_datadir(port), file);
      }
    }
  }

  if (!shcore::path::exists(path)) path.clear();

  return path;
}

//!<  @name Misc Utilities
///@{
/**
 * Dumps a list of schemas from a MySQL instance into a file
 * @param uri URI of the instance. Must be classic protocol.
 * @param path filename of the dump file to write to
 * @param schemaList array of schema names to dump
 * @param options Optional dictionary with options that affect the produced dump
 *
 * Calls mysqldump to dump the given schemas.
 *
 * The supported options include:
 *
 * @li noData: boolean, if true, excludes schema data
 * @li skipComments: boolean, if true, excludes all the comments
 * @li skipDumpDate: boolean, if true, excludes the dump date comment
 */
#if DOXYGEN_JS
Undefined Testutils::dumpData(String uri, String path, Array schemaList);
#elif DOXYGEN_PY
None Testutils::dump_data(str uri, str path, list schemaList);
#endif
///@}
void Testutils::dump_data(const std::string &uri, const std::string &path,
                          const std::vector<std::string> &schemas,
                          const shcore::Dictionary_t &opts) {
  bool no_data = false;
  bool skip_dump_date = false;
  bool skip_comments = false;
  if (opts) {
    shcore::Option_unpacker(opts)
        .optional("noData", &no_data)
        .optional("skipComments", &skip_comments)
        .optional("skipDumpDate", &skip_dump_date)
        .end();
  }
  // When replaying we don't need to do anything
  mysqlshdk::db::replay::No_replay dont_record;
  if (_skip_server_interaction) return;

  // use mysqldump for now, until we support dumping internally
  std::string mysqldump = shcore::path::search_stdpath("mysqldump");
  if (mysqldump.empty()) {
    throw std::runtime_error("mysqldump executable not found in PATH");
  }

  auto options = mysqlshdk::db::Connection_options(uri);

  std::string sport =
      options.has_port() ? std::to_string(options.get_port()) : "3306";
  std::vector<const char *> argv;
  argv.push_back(mysqldump.c_str());
  argv.push_back("-u");
  argv.push_back(options.get_user().c_str());
  argv.push_back("-h");
  argv.push_back(options.get_host().c_str());
  if (options.has_port()) {
    argv.push_back("--protocol=TCP");
    argv.push_back("-P");
    argv.push_back(sport.c_str());
  }
  std::string password;
  if (options.has_password()) {
    password = "--password=" + options.get_password();
    // NOTE: If this ever becomes a public (non-test) function, pwd passing must
    // be done via stdin or temporary file
    argv.push_back(password.c_str());
  }
  argv.push_back("-r");
  argv.push_back(path.c_str());
  argv.push_back("--set-gtid-purged=OFF");
  argv.push_back("--databases");
  for (const auto &s : schemas) argv.push_back(s.c_str());
  if (g_test_trace_scripts > 0) {
    std::cerr << shcore::str_join(argv, " ") << "\n";
  }
  if (no_data) argv.push_back("--no-data");
  if (skip_comments) argv.push_back("--skip-comments");
  if (skip_dump_date) argv.push_back("--skip-dump-date");
  argv.push_back(nullptr);
  shcore::Process dump(&argv[0]);
  dump.start();
  std::string output;
  bool eof = false;
  for (;;) {
    std::string line = dump.read_line(&eof);
    std::cerr << line;
    if (!line.empty()) output = line;
    if (eof) break;
  }
  int rc = dump.wait();
  if (rc != 0) {
    throw std::runtime_error("mysqldump exited with code " +
                             std::to_string(rc) + ": " + output);
  }
}

//!<  @name Misc Utilities
///@{
/**
 * Loads a MySQL dump script from a file
 * @param uri URI of the instance. Must be classic protocol.
 * @param path filename of the dump file to write to
 * @param defaultSchema (optional) Default schema name to use during import.
 *
 * Loads a SQL script from a file using mysql cli.
 */
#if DOXYGEN_JS
Undefined Testutils::importData(String uri, String path, String defaultSchema,
                                String defaultCharset);
#elif DOXYGEN_PY
None Testutils::import_data(str uri, str path, str defaultSchema,
                            str defaultCharset);
#endif
///@}
void Testutils::import_data(const std::string &uri, const std::string &path,
                            const std::string &default_schema,
                            const std::string &default_charset) {
  // When replaying we don't need to do anything
  mysqlshdk::db::replay::No_replay dont_record;
  if (_skip_server_interaction) return;

  // use mysql for now, until we support efficient import internally
  std::string mysql = shcore::path::search_stdpath("mysql");
  if (mysql.empty()) {
    throw std::runtime_error("mysql executable not found in PATH");
  }

  auto options = mysqlshdk::db::Connection_options(uri);
  std::string sport =
      options.has_port() ? std::to_string(options.get_port()) : "3306";
  std::vector<const char *> argv;
  argv.push_back(mysql.c_str());
  argv.push_back("-u");
  argv.push_back(options.get_user().c_str());
  argv.push_back("-h");
  argv.push_back(options.get_host().c_str());
  std::string password;
  if (options.has_password()) {
    password = "--password=" + options.get_password();
    // NOTE: If this ever becomes a public (non-test) function, pwd passing must
    // be done via stdin or temporary file
    argv.push_back(password.c_str());
  }
  if (options.has_port()) {
    argv.push_back("--protocol=TCP");
    argv.push_back("-P");
    argv.push_back(sport.c_str());
  }
  std::string defcharset;
  if (!default_charset.empty()) {
    defcharset = "--default-character-set=" + default_charset;
    argv.push_back(defcharset.c_str());
  }
  if (!default_schema.empty()) argv.push_back(default_schema.c_str());

  if (g_test_trace_scripts > 0) {
    std::cerr << shcore::str_join(argv, " ") << "\n";
  }
  argv.push_back(nullptr);

  shcore::Process dump(&argv[0]);
#ifdef _WIN32
  dump.set_create_process_group();
#endif  // _WIN32
  dump.enable_reader_thread();
  dump.redirect_file_to_stdin(path);
  dump.start();

  const auto rc = dump.wait();

  if (rc != 0) {
    throw std::runtime_error("mysql exited with code " + std::to_string(rc) +
                             ": " + dump.read_all());
  }
}

//!<  @name InnoDB Cluster Utilities
///@{
/**
 * Upon starting a mysql server, if the group_replication_start_on_boot flag is
 * enabled, the GR plugin is started. While the plugin is starting, the user
 * cannot run any operations to modify GR variables as it results in an error.
 *
 * The purpose of this method is upon starting a server, to work as
 * synchronization point for the tests, waiting until the GR plugin starts.
 *
 * @param port The port of the instance where we will wait until the GR plugin
 * starts.
 * @param rootpass The password to be assigned to the root user.
 * @param timeout How many seconds we will wait for the plugin to start. If
 *        timeout value is 0, the method waits indefinitely. By default
 *        100 seconds. Negative timeout values will be converted to 0.
 *
 */
#if DOXYGEN_JS
Undefined Testutils::waitForDelayedGRStart(Integer port, String rootpass,
                                           Integer timeout = 60);
#elif DOXYGEN_PY
None Testutils::wait_for_delayed_gr_start(int port, str rootpass,
                                          int timeout = 60)
#endif
///@}
void Testutils::wait_for_delayed_gr_start(int port,
                                          const std::string &root_pass,
                                          int timeout) {
  mysqlshdk::db::Connection_options cnx_opt;
  cnx_opt.set_user("root");
  cnx_opt.set_password(root_pass);
  cnx_opt.set_host("localhost");
  cnx_opt.set_port(port);
  std::shared_ptr<mysqlshdk::db::ISession> session;
  session = mysqlshdk::db::mysql::Session::create();
  session->connect(cnx_opt);
  mysqlshdk::mysql::Instance instance{session};

  int elapsed_time = 0;
  bool is_starting = false;
  const uint32_t sleep_time = mysqlshdk::db::replay::g_replay_mode ==
                                      mysqlshdk::db::replay::Mode::Replay
                                  ? 1
                                  : 1000;
  // Convert negative timeout values to 0
  timeout = timeout >= 0 ? timeout : 0;
  while (!timeout || elapsed_time < timeout) {
    is_starting =
        mysqlshdk::gr::is_group_replication_delayed_starting(instance);
    if (!is_starting) {
      break;
    }
    elapsed_time += 1;
    shcore::sleep_ms(sleep_time);
  }
  session->close();
  if (is_starting) {
    throw std::runtime_error(
        "Timeout waiting for the Group Replication Plugin to start/stop");
  }
}

//!<  @name Misc Utilities
///@{
/**
 * Gets the path to the shell log.
 */
#if DOXYGEN_JS
String Testutils::getShellLogPath();
#elif DOXYGEN_PY
str Testutils::get_shell_log_path();
#endif
///@}
std::string Testutils::get_shell_log_path() {
  return shcore::current_logger()->logfile_name();
}

#ifndef ENABLE_SESSION_RECORDING
//!<  @name Misc Utilities
///@{
/**
 * Gets (and optionally flushes) the contents of the dba SQL log.
 */
#if DOXYGEN_JS
Array Testutils::fetchDbaSqlLog(Boolean);
#elif DOXYGEN_PY
list Testutils::fetch_dba_sql_log(bool);
#endif
///@}
shcore::Array_t Testutils::fetch_dba_sql_log(bool flush) {
  shcore::Array_t log = dynamic_cast<Shell_core_test_wrapper *>(_test_env)
                            ->output_handler.dba_sql_log;
  if (flush)
    dynamic_cast<Shell_core_test_wrapper *>(_test_env)
        ->output_handler.dba_sql_log.reset();
  return log;
}
#endif

//!<  @name Testing Utilities
///@{
/**
 * Identifies if the test suite is being executed in reply mode.
 */
#if DOXYGEN_JS
Bool Testutils::isReplaying();
#elif DOXYGEN_PY
bool Testutils::is_replaying();
#endif
///@}
bool Testutils::is_replaying() {
  return mysqlshdk::db::replay::g_replay_mode ==
         mysqlshdk::db::replay::Mode::Replay;
}

//!<  @name Testing Utilities
///@{
/**
 * Causes the test to fail.
 *
 * This function can be used directly on the test script to cause a failure,
 * it is useful if the test validation should be done using pure code rather
 * than falling to the standard validation methods.
 */
#if DOXYGEN_JS
Undefined Testutils::fail();
#elif DOXYGEN_PY
None Testutils::fail();
#endif
///@}
void Testutils::fail(const std::string &context) {
  // TODO(alfredo) make and replace with a markup text processor
  std::string text = context;

  if (g_test_color_output) {
    text = shcore::str_replace(text, "<red>", "\x1b[31m");
    text = shcore::str_replace(text, "</red>", "\x1b[0m");
    text = shcore::str_replace(text, "<yellow>", "\x1b[33m");
    text = shcore::str_replace(text, "</yellow>", "\x1b[0m");
    text = shcore::str_replace(text, "<b>", "\x1b[1m");
    text = shcore::str_replace(text, "</b>", "\x1b[0m");
  } else {
    text = shcore::str_replace(text, "<red>", "");
    text = shcore::str_replace(text, "</red>", "");
    text = shcore::str_replace(text, "<yellow>", "");
    text = shcore::str_replace(text, "</yellow>", "");
    text = shcore::str_replace(text, "<b>", "");
    text = shcore::str_replace(text, "</b>", "");
  }
#ifdef ENABLE_SESSION_RECORDING
  throw std::logic_error("method not available");
#else
  ADD_FAILURE_AT(_test_file.c_str(), _test_line) << text << "\n";
#endif
}

///@{
/**
 * Causes the test to skip.
 *
 * Call from test script when the rest of the test should be skipped.
 */
#if DOXYGEN_JS
Undefined Testutils::skip();
#elif DOXYGEN_PY
None Testutils::skip();
#endif
///@}
void Testutils::skip(const std::string &reason) { _test_skipped = reason; }

#if 0
///@{
/**
 * Slow down replication applier rate by locking and unlock tables in a loop.
 *
 * @param port
 * @param start true to start, false to stop.
 */
#if DOXYGEN_JS
Undefined Testutils::slowify(Integer port, Boolean start);
#elif DOXYGEN_PY
None Testutils::skip(int port, bool start);
endif
///@}
void Testutils::slowify(int port, bool start) {
  if (_slower_threads.find(port) != _slower_threads.end()) {
    if (start)
      throw std::logic_error("slowify already active for " +
                             std::to_string(port));
  } else {
    if (!start)
      throw std::logic_error("slowify not active for " + std::to_string(port));
  }

  if (start) {
    Slower_thread *info(new Slower_thread());
    _slower_threads.emplace(port, info);

    info->thread = std::thread([this, port]() {
      mysqlsh::thread_init();

      try {
        auto session = connect_to_sandbox(port);
        while (!_slower_threads[port]->stop) {
          session->execute("FLUSH TABLES WITH READ LOCK");
          shcore::sleep_ms(500);
          session->execute("UNLOCK TABLES");
        }
        session->close();
      } catch (std::exception &e) {
        dprint("ERROR in slower thread: " + std::string(e.what()));
      }

      mysqlsh::thread_end();
    });
  } else {
    _slower_threads[port]->stop = true;
    _slower_threads[port]->thread.join();
    _slower_threads.erase(port);
  }
}
#endif
#endif

///@{
/**
 * Saves a snapshot of the my.cnf file for a sandbox. On replay mode, the
 * snapshot that was saved will be restored.
 *
 * If multiple snapshots are taken, they will be all stored, then restored
 * in the same order.
 */
#if DOXYGEN_JS
Undefined Testutils::snapshotSandboxConf(Integer port);
#elif DOXYGEN_PY
None Testutils::snapshot_sandbox_conf(int port);
#endif
///@}
void Testutils::snapshot_sandbox_conf(int port) {
  if (mysqlshdk::db::replay::g_replay_mode !=
      mysqlshdk::db::replay::Mode::Direct) {
    if (_sandbox_snapshot_dir.empty()) {
      throw std::logic_error("set_sandbox_snapshot_dir() not called");
    }
    std::string sandbox_cnf_path = get_sandbox_conf_path(port);
    std::string sandbox_cnf_bkpath =
        _sandbox_snapshot_dir + "/sandbox_" + std::to_string(port) + "_" +
        std::to_string(_snapshot_conf_serial++) + "_my.cnf";
    if (!_skip_server_interaction) {
      // copy the config file from the sandbox dir to the snapshot dir
      shcore::copy_file(sandbox_cnf_path, sandbox_cnf_bkpath);
    } else {
      // copy the config file from the snapshot dir to the sandbox dir, creating
      // it if needed
      shcore::create_directory(shcore::path::dirname(sandbox_cnf_path));
      shcore::copy_file(sandbox_cnf_bkpath, sandbox_cnf_path);
      if (g_test_trace_scripts)
        std::cerr << "Copied " << sandbox_cnf_bkpath << " to "
                  << sandbox_cnf_path << "\n";
    }
  }
}

///@{
/**
 * Checks whether the given TCP port is listening for connections.
 *
 * The result of the check will be recorded in the trace, so that the same
 * result is returned during a replay.
 */
#if DOXYGEN_JS
Bool Testutils::isTcpPortListening(String host, Integer port);
#elif DOXYGEN_PY
bool Testutils::is_tcp_port_listening(str host, int port);
#endif
///@}
bool Testutils::is_tcp_port_listening(const std::string &host, int port) {
  std::string port_check_file = _sandbox_snapshot_dir + ".portchk_" +
                                std::to_string(_tcp_port_check_serial++);

  switch (mysqlshdk::db::replay::g_replay_mode) {
    case mysqlshdk::db::replay::Mode::Replay: {
      auto info = shcore::Value::parse(shcore::get_text_file(port_check_file));
      auto dict = info.as_map();
      if (dict->get_string("host") != host || dict->get_int("port") != port) {
        std::cerr << "Mismatched TCP port check params:\n"
                  << "Expected: " << info.descr() << "\nActual: " << host
                  << ", " << port << "\n";
        throw std::runtime_error("TCP port check mismatch");
      }
      return dict->get_bool("result");
    }

    case mysqlshdk::db::replay::Mode::Record: {
      bool r = false;
      try {
        r = mysqlshdk::utils::Net::is_port_listening(host, port);
      } catch (std::exception &) {
        r = false;
      }
      auto dict = shcore::make_dict();
      dict->set("port", shcore::Value(port));
      dict->set("host", shcore::Value(host));
      dict->set("result", shcore::Value(r));
      shcore::create_file(port_check_file, shcore::Value(dict).repr());
      return r;
    }

    case mysqlshdk::db::replay::Mode::Direct:
    default:
      try {
        return mysqlshdk::utils::Net::is_port_listening(host, port);
      } catch (std::exception &) {
        return false;
      }
  }
}

void Testutils::begin_snapshot_sandbox_error_log(int port) {
  if (_sandbox_snapshot_dir.empty()) {
    if (mysqlshdk::db::replay::g_replay_mode !=
        mysqlshdk::db::replay::Mode::Direct)
      throw std::logic_error("set_sandbox_snapshot_dir() not called");
    return;
  }
  std::string sandbox_log_path = get_sandbox_log_path(port);
  std::string sandbox_log_bkpath = shcore::path::join_path(
      _sandbox_snapshot_dir, "sandbox_" + std::to_string(port) + "_" +
                                 std::to_string(_snapshot_log_index) +
                                 "_error.log");
  if (_skip_server_interaction) {
    _snapshot_log_index++;
    // copy the log file from the snapshot dir to the sandbox dir, creating
    // it if needed
    shcore::create_directory(shcore::path::dirname(sandbox_log_path));
    shcore::copy_file(sandbox_log_bkpath, sandbox_log_path);
    if (g_test_trace_scripts)
      std::cerr << "Copied " << sandbox_log_bkpath << " to " << sandbox_log_path
                << "\n";
  }
}

void Testutils::end_snapshot_sandbox_error_log(int port) {
  if (_sandbox_snapshot_dir.empty()) {
    if (mysqlshdk::db::replay::g_replay_mode !=
        mysqlshdk::db::replay::Mode::Direct)
      throw std::logic_error("set_sandbox_snapshot_dir() not called");
    return;
  }
  std::string sandbox_log_path = get_sandbox_log_path(port);
  std::string sandbox_log_bkpath = shcore::path::join_path(
      _sandbox_snapshot_dir, "sandbox_" + std::to_string(port) + "_" +
                                 std::to_string(_snapshot_log_index) +
                                 "_error.log");
  if (!_skip_server_interaction) {
    _snapshot_log_index++;
    // copy the log file from the sandbox dir to the snapshot dir
    shcore::copy_file(sandbox_log_path, sandbox_log_bkpath);
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Deploys a sandbox using the indicated password and port
 * @param port The port where the sandbox wlil be listening for mysql protocol
 * connections.
 * @param pwd The password to be assigned to the root user.
 * @param options Additional options to be set on the sandbox configuration
 * file.
 *
 * This functions works when using either --record or --direct mode of the test
 * suite. It is an improved version of the deploySandboxInstance function of the
 * Admin API which will speed up the process of deploying a new sandbox.
 *
 * First time it is called, it will create a boilerplate sandbox using the
 * normal sandbox deployment procedure.
 *
 * It creates a new sandbox by copying the data on the boilerplate sandbox.
 *
 * When using --replay mode, the function does nothing.
 *
 * Sandboxes are pre-configured for group replication and thus are not good
 * for testing AdminAPI. For testing AdminAPI features, specially those that
 * check or configure MySQL instances, use deployRawSandbox() instead.
 */
#if DOXYGEN_JS
Undefined Testutils::deploySandbox(Integer port, String pwd,
                                   Dictionary options);
#elif DOXYGEN_PY
None Testutils::deploy_sandbox(int port, str pwd, Dictionary options);
#endif
///@}
void Testutils::deploy_sandbox(int port, const std::string &rootpass,
                               const shcore::Dictionary_t &my_cnf_options,
                               const shcore::Dictionary_t &opts) {
  bool create_remote_root{true};
  bool keyring_plugin{false};
  std::string mysqld_path;

  if (opts) {
    create_remote_root = opts->get_bool("createRemoteRoot", true);
    mysqld_path = opts->get_string("mysqldPath", "");
    keyring_plugin = opts->get_bool("enableKeyringPlugin", false);
  }

  if (keyring_plugin) {
    std::string plugin_file("keyring_file");
#ifdef _WIN32
    plugin_file.append(".dll");
#else
    plugin_file.append(".so");
#endif

    (*my_cnf_options)["early-plugin-load"] = shcore::Value(plugin_file);

    if (!my_cnf_options->has_key("keyring_file_data")) {
      auto keyring_file_data = shcore::path::join_path(
          _sandbox_dir, std::to_string(port), "keyring", "keyfile");
      (*my_cnf_options)["keyring_file_data"] = shcore::Value(keyring_file_data);
    }
  }

  _passwords[port] = rootpass;
  mysqlshdk::db::replay::No_replay dont_record;
  if (!_skip_server_interaction) {
    wait_sandbox_dead(port);

    // Sandbox from a boilerplate
    if (k_boilerplate_root_password == rootpass) {
      prepare_sandbox_boilerplate(rootpass, port, mysqld_path);
      if (!deploy_sandbox_from_boilerplate(port, my_cnf_options, false,
                                           mysqld_path)) {
        std::cerr << "Unable to deploy boilerplate sandbox\n";
        abort();
      }
      handle_remote_root_user(rootpass, port, create_remote_root);
    } else {
      prepare_sandbox_boilerplate(rootpass, port, mysqld_path);
      deploy_sandbox_from_boilerplate(port, my_cnf_options, false, mysqld_path);
      handle_remote_root_user(rootpass, port, create_remote_root);
    }
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Deploys a raw sandbox using the indicated password and port
 *
 * Basically the same as deploySandbox, except:
 * - configurations are closer to default (all replication options are removed)
 * - datadir is called "data" instead of "sandboxdata", which makes it look like
 * an ordinary instance to the AdminAPI.
 */
#if DOXYGEN_JS
Undefined Testutils::deployRawSandbox(Integer port, String pwd,
                                      Dictionary options);
#elif DOXYGEN_PY
None Testutils::deploy_raw_sandbox(int port, str pwd, Dictionary options);
#endif
///@}
void Testutils::deploy_raw_sandbox(int port, const std::string &rootpass,
                                   const shcore::Dictionary_t &my_cnf_opts,
                                   const shcore::Dictionary_t &opts) {
  bool create_remote_root{true};
  std::string mysqld_path;

  if (opts) {
    create_remote_root = opts->get_bool("createRemoteRoot", true);
    mysqld_path = opts->get_string("mysqldPath", "");
  }

  _passwords[port] = rootpass;
  mysqlshdk::db::replay::No_replay dont_record;
  if (!_skip_server_interaction) {
    wait_sandbox_dead(port);
    // Sandbox from a boilerplate
    if (k_boilerplate_root_password == rootpass) {
      prepare_sandbox_boilerplate(rootpass, port, mysqld_path);
      wait_sandbox_dead(port);
      if (!deploy_sandbox_from_boilerplate(port, my_cnf_opts, true,
                                           mysqld_path)) {
        std::cerr << "Unable to deploy boilerplate sandbox\n";
        abort();
      }
      handle_remote_root_user(rootpass, port, create_remote_root);
    } else {
      prepare_sandbox_boilerplate(rootpass, port, mysqld_path);
      wait_sandbox_dead(port);
      deploy_sandbox_from_boilerplate(port, my_cnf_opts, true, mysqld_path);
      handle_remote_root_user(rootpass, port, create_remote_root);
    }
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Destroys the sandbox listening at the indicated port
 * @param port The port where the sandbox is listening for mysql protocol
 * connections.
 *
 * This function also works when using the --direct and --record modes of the
 * test suite.
 *
 * It will delete the sandbox listening at the indicated port. This function
 * must be called after stopping or killing the sandbox.
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
Undefined Testutils::destroySandbox(Integer port);
#elif DOXYGEN_PY
None Testutils::destroy_sandbox(int port);
#endif
///@}
void Testutils::destroy_sandbox(int port, bool quiet_kill) {
  mysqlshdk::db::replay::No_replay dont_record;
  kill_sandbox(port, quiet_kill);
#ifdef _WIN32
  // Make config file (and backups) readable in case it was made RO by some test
  std::string dirname = shcore::path::dirname(get_sandbox_conf_path(port));
  if (shcore::path::exists(dirname)) {
    for (const std::string &name : shcore::listdir(dirname)) {
      const std::string path = shcore::path::join_path({dirname, name});
      const auto wide_path = shcore::utf8_to_wide(path);
      auto attributes = GetFileAttributesW(wide_path.c_str());
      if (attributes != INVALID_FILE_ATTRIBUTES) {
        attributes &= ~FILE_ATTRIBUTE_READONLY;
        SetFileAttributesW(wide_path.c_str(), attributes);
      }
    }
  }
#endif
  if (!_skip_server_interaction) {
    shcore::Value::Array_type_ref errors;
    _mp.delete_sandbox(port, _sandbox_dir, true, &errors);
    if (errors && !errors->empty())
      std::cerr << "During delete of " << port << ": "
                << shcore::Value(errors).descr() << "\n";
  } else {
    if (!_sandbox_dir.empty()) {
      std::string sandbox_path =
          shcore::path::dirname(get_sandbox_conf_path(port));
      if (shcore::is_folder(sandbox_path))
        shcore::remove_directory(sandbox_path, true);
    }
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Starts the sandbox created at the indicated port
 * @param port The port where the sandbox listens for mysql protocol
 * connections.
 * @param options: Optional dictionary that accepts the following options:
 *        timeout: integer value with the timeout in seconds for the start
 *                 operation.
 *
 * This function also works when using the --direct and --record modes of the
 * test suite.
 *
 * It will retry up to 5 times starting the sandbox at the indicated port,
 * improving the success rate of the operation.
 *
 * This function will verify that any other sandbox running previously at the
 * same port is completely dead before each start attempt.
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
Undefined Testutils::startSandbox(Integer port, Dictionary options);
#elif DOXYGEN_PY
None Testutils::start_sandbox(int port, dict options);
#endif
///@}
void Testutils::start_sandbox(int port, const shcore::Dictionary_t &opts) {
  // Initialize to default value (30 sec)
  int timeout = k_start_wait_timeout;
  shcore::Option_unpacker(opts).optional("timeout", &timeout).end();

  if (!_skip_server_interaction) {
    try {
      wait_sandbox_dead(port);

      shcore::Value::Array_type_ref errors;
      _mp.start_sandbox(port, _sandbox_dir, &errors, timeout);
      if (errors && !errors->empty()) {
        for (auto err : *errors) {
          if ((*err.as_map())["type"].get_string() == "ERROR") {
            std::cerr << "During start of " << port << ": "
                      << shcore::Value(errors).descr() << "\n";
            throw std::runtime_error("Could not start sandbox instance " +
                                     std::to_string(port));
          }
        }
      }
    } catch (const std::runtime_error &error) {
      // print the error log contents
      std::string error_log_path = get_sandbox_log_path(port);
      try {
        dprint(cat_file(error_log_path));
      } catch (const std::exception &e) {
        dprint("Could not dump " + error_log_path + ": " + e.what());
      }
      throw;
    }
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Stops the sandbox listening at the indicated port
 * @param port The port where the sandbox listens for mysql protocol
 * connections.
 *
 * This function works when using the --direct and --record modes of the test
 * suite.
 *
 * This function performs the normal stop sandbox operation of the Admin API
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
Undefined Testutils::stopSandbox(Integer port, Object options);
#elif DOXYGEN_PY
None Testutils::stop_sandbox(int port, dict options);
#endif
///@}
void Testutils::stop_sandbox(int port, const shcore::Dictionary_t &opts) {
  bool wait = false;
  shcore::Option_unpacker(opts).optional("wait", &wait).end();

  mysqlshdk::db::replay::No_replay dont_record;
  if (!_skip_server_interaction) {
    try {
      auto session = connect_to_sandbox(port);
      session->execute("shutdown");
      session->close();

      if (wait) wait_sandbox_dead(port);
    } catch (const std::runtime_error &error) {
      // print the error log contents
      std::string error_log_path = get_sandbox_log_path(port);
      try {
        dprint(cat_file(error_log_path));
      } catch (const std::exception &e) {
        dprint("Could not dump " + error_log_path + ": " + e.what());
      }
      throw;
    }
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Restarts the sandbox listening at the specified port.
 * @param port The port where the sandbox listens for mysql protocol
 * connections.
 *
 * This function works when using the --direct and --record modes of the test
 * suite.
 *
 * This function executes the stop sandbox operation of this module followed
 * by the start sandbox operation.
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
Undefined Testutils::restartSandbox(Integer port);
#elif DOXYGEN_PY
None Testutils::restart_sandbox(int port);
#endif
///@}
void Testutils::restart_sandbox(int port) {
  stop_sandbox(port, shcore::make_dict("wait", shcore::Value(1)));
  start_sandbox(port);
}

//!<  @name Sandbox Operations
///@{
/**
 * Kills the sandbox listening at the indicated port
 * @param port The port where the sandbox listens for mysql protocol
 * connections.
 *
 * This function works when using the --direct and --record modes of the test
 * suite.
 *
 * This function performs the normal kill sandbox operation of the Admin API but
 * also verifies that the sandbox is completely dead.
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
Undefined Testutils::killSandbox(Integer port);
#elif DOXYGEN_PY
None Testutils::kill_sandbox(int port);
#endif
///@}
void Testutils::kill_sandbox(int port, bool quiet) {
  if (!_skip_server_interaction) {
    shcore::Value::Array_type_ref errors;
    _mp.kill_sandbox(port, _sandbox_dir, &errors);
    // Only output errors to stderr if quiet mode is disabled (default).
    if (!quiet && errors && !errors->empty())
      std::cerr << "During kill of " << port << ": "
                << shcore::Value(errors).descr() << "\n";
    wait_sandbox_dead(port);
  }
}

#ifndef WIN32
static int os_file_lock(int fd) {
  struct flock lk;

  lk.l_type = F_WRLCK;
  lk.l_whence = SEEK_SET;
  lk.l_start = lk.l_len = 0;

  if (fcntl(fd, F_SETLK, &lk) == -1) {
    if (errno == EAGAIN || errno == EACCES) {
      return 1;  // already locked
    }
    return (-1);  // error
  }
  return (0);  // not locked
}
#endif

void Testutils::wait_sandbox_dead(int port) {
  log_info("Waiting for ports (%d)...", port);
  // wait until classic, x and Xcom ports are free
  if (port * 10 + 1 < 65535) {
    int retries = (k_wait_sandbox_dead_timeout * 1000) / 500;
    while (!is_port_available_for_sandbox_to_bind(port * 10 + 1)) {
      if (--retries < 0)
        throw std::runtime_error("Timeout waiting for sandbox " +
                                 std::to_string(port) + " to shutdown");
      shcore::sleep_ms(500);
    }
  }
  if (port * 10 < 65535) {
    int retries = (k_wait_sandbox_dead_timeout * 1000) / 500;
    while (!is_port_available_for_sandbox_to_bind(port * 10)) {
      if (--retries < 0)
        throw std::runtime_error("Timeout waiting for sandbox " +
                                 std::to_string(port) + " to shutdown");
      shcore::sleep_ms(500);
    }
  }
  {
    int retries = (k_wait_sandbox_dead_timeout * 1000) / 500;
    while (!is_port_available_for_sandbox_to_bind(port)) {
      if (--retries < 0)
        throw std::runtime_error("Timeout waiting for sandbox " +
                                 std::to_string(port) + " to shutdown");
      shcore::sleep_ms(500);
    }
  }

  log_info("Waiting for lock file...");

  int retries = (k_wait_sandbox_dead_timeout * 1000) / 500;
#ifndef _WIN32
  while (mysqlshdk::utils::check_lock_file(get_sandbox_datadir(port) +
                                           "/mysqld.sock.lock")) {
    if (--retries < 0)
      throw std::runtime_error("Timeout waiting for sandbox " +
                               std::to_string(port) + " to shutdown");
    shcore::sleep_ms(500);
  }

  retries = (k_wait_sandbox_dead_timeout * 1000) / 500;
  while (mysqlshdk::utils::check_lock_file(
      get_sandbox_datadir(port) + "/mysqlx.sock.lock", "X%zd")) {
    if (--retries < 0)
      throw std::runtime_error("Timeout waiting for sandbox " +
                               std::to_string(port) + " to shutdown");
    shcore::sleep_ms(500);
  }
  retries = (k_wait_sandbox_dead_timeout * 1000) / 500;
#endif
  std::string pidfile = shcore::path::join_path(get_sandbox_path(port),
                                                std::to_string(port) + ".pid");
  // Wait for the pid file to be deleted, which is one the last things the
  // server does before exiting and is done right before logging
  // "Shutdown complete".
  // Obviously this won't be reliable if the sandbox is killed, tho.
  log_info("Waiting for pid file '%s' to be deleted by mysqld...",
           pidfile.c_str());
  while (shcore::path::exists(pidfile)) {
    if (--retries < 0)
      throw std::runtime_error("Timeout waiting for sandbox pid file " +
                               pidfile + " to be deleted after shutdown");
    shcore::sleep_ms(500);
  }
  log_info("Finished waiting");
}

void Testutils::wait_sandbox_alive(const shcore::Value &port_or_uri) {
  if (shcore::Value_type::Integer == port_or_uri.type) {
    const auto port = port_or_uri.as_int();

    wait_sandbox_alive([this, port]() { return connect_to_sandbox(port); },
                       std::to_string(port));
  } else {
    const auto co = mysqlsh::get_connection_options(port_or_uri);

    wait_sandbox_alive(
        [&co]() { return mysqlsh::establish_session(co, false); },
        co.as_uri(mysqlshdk::db::uri::formats::full()));
  }
}

void Testutils::wait_sandbox_alive(
    const std::function<std::shared_ptr<mysqlshdk::db::ISession>()> &connect,
    const std::string &context) {
  log_info("Waiting for sandbox (%s) to accept connections...",
           context.c_str());
  int retries = (k_wait_sandbox_alive_timeout * 1000) / 500;
  std::shared_ptr<mysqlshdk::db::ISession> session;
  while (retries > 0) {
    try {
      session = connect();
    } catch (const std::exception &err) {
      --retries;
      shcore::sleep_ms(500);
      continue;
    }
    session->close();
    break;
  }
  if (retries <= 0) {
    throw std::runtime_error("Timeout waiting for sandbox " + context +
                             " to accept connections");
  }
}

bool Testutils::is_port_available_for_sandbox_to_bind(int port) const {
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo *info = nullptr, *p = nullptr;
  // Get the list of network address structures for all IPs (0.0.0.0) on the
  // given service port
  int result =
      getaddrinfo("0.0.0.0", std::to_string(port).c_str(), &hints, &info);

  if (result != 0) throw mysqlshdk::utils::net_error(gai_strerror(result));

  if (info != nullptr) {
    std::unique_ptr<addrinfo, void (*)(addrinfo *)> deleter{info, freeaddrinfo};
    // initialize socket
    int sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
#ifndef _WIN32
    const int socket_error = -1;
#else
    const int socket_error = INVALID_SOCKET;
#endif
    if (sock == socket_error) {
      throw std::runtime_error("Could not create socket: " +
                               shcore::errno_to_string(errno));
    }
    for (p = info; p != nullptr; p = p->ai_next) {
#ifndef _WIN32
      const int opt_val = 1;
#else
      const char opt_val = 0;
#endif
      // set socket as reusable for non windows systems since according to
      // sql/conn_handler/socket_connection.cc:383 mysqld doesn't set
      // SO_REUSEADDR for Windows.
      if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val) <
          0) {
        throw std::runtime_error("Could not set socket as reusable: " +
                                 shcore::errno_to_string(errno));
      }
      if (bind(sock, p->ai_addr, p->ai_addrlen) != 0) {
        // cannot bind
        return false;
      } else {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
      }
    }
    // could bind on all
    return true;
  } else {
    throw std::runtime_error("Could not resolve address 0.0.0.0");
  }
}

void Testutils::wait_until_file_lock_released(const std::string &filepath,
                                              int timeout) const {
  int retries = timeout * 1000 / 500;
  log_info("Waiting for lock of file '%s' to be released...", filepath.c_str());
  // Note this method should not throw any error if the file doesn't exist.
#ifdef _WIN32
  // In Windows, it should be enough to see if the file is locked
  while (true) {
    FILE *f = fopen(filepath.c_str(), "a");
    if (f) {
      fclose(f);
      break;
    }
    if (errno == ENOENT) break;
    if (--retries < 0)
      throw std::runtime_error("Timeout waiting for lock of file '" + filepath +
                               "' to be released.");
    shcore::sleep_ms(500);
  }
#else
  int file_fd = open(filepath.c_str(), O_RDWR);
  if (file_fd > 0) {
    while (os_file_lock(file_fd) > 0) {
      if (--retries < 0)
        throw std::runtime_error("Timeout waiting for lock of file '" +
                                 filepath + "' to be released.");
      shcore::sleep_ms(500);
    }
    ::close(file_fd);
  }
#endif
}

bool is_configuration_option(const std::string &option,
                             const std::string &line) {
  std::string normalized = shcore::str_replace(line, " ", "");
  return (normalized == option || normalized.find(option + "=") == 0);
}

bool is_section_line(const std::string &line, const std::string &section = "") {
  bool ret_val = false;

  if (!line.empty()) {
    std::string normalized = shcore::str_strip(line);
    if (normalized[0] == '[' && normalized[normalized.length() - 1] == ']') {
      ret_val = true;

      if (!section.empty()) ret_val = normalized == "[" + section + "]";
    }
  }

  return ret_val;
}

//!<  @name Sandbox Operations
///@{
/**
 * Delete lines with the option from the given config file.
 * @param port The port of the sandbox where the configuration will be updated.
 * @param option The option name that will be removed from the configuration
 * file.
 * @param section The section from which the option will be removed.
 *
 * This function will remove any occurrence of the configuration option on the
 * indicated section.
 *
 * If the section is not specified the operation will be done on every section
 * of the file.
 */
#if DOXYGEN_JS
Undefined Testutils::removeFromSandboxConf(Integer port, String option,
                                           String section);
#elif DOXYGEN_PY
None Testutils::remove_from_sandbox_conf(int port, str option, str section);
#endif
///@}
void Testutils::remove_from_sandbox_conf(int port, const std::string &option,
                                         const std::string &section) {
  std::string cfgfile_path = get_sandbox_conf_path(port);

  bool file_exists = shcore::is_file(cfgfile_path);
  if (!file_exists && _skip_server_interaction) return;

  mysqlshdk::config::Config_file cfg;
  if (file_exists) {
    // Read the file, only if it exists.
    cfg.read(cfgfile_path);
  } else {
    throw std::logic_error{
        "Cannot remove an option from a file that does not"
        "exist. File: " +
        cfgfile_path};
  }

  if (section.empty()) {
    // Remove option from all groups (sections) if the section is not specified
    // (empty).
    std::vector<std::string> groups = cfg.groups();
    for (auto const &group : groups) {
      cfg.remove_option(group, option);
    }
  } else {
    cfg.remove_option(section, option);
  }

  // Apply changes (write) to option file.
  cfg.write(cfgfile_path);
}

//!<  @name Sandbox Operations
///@{
/**
 * Change sandbox config option and add it if it's not in the my.cnf yet
 * @param port The port of the sandbox where the configuration will be updated.
 * @param option The option to be updated or added.
 * @param value The new value for the option.
 * @param section The section on which the option will be added or updated.
 *
 * This function will replace the value of the configuration option from the
 * indicated section of the configuration file.
 *
 * If the option does not exist it will be added.
 *
 * If the section is not indicated, mysqld is assumed.
 * * will change all sections.
 */
#if DOXYGEN_JS
Undefined Testutils::changeSandboxConf(Integer port, String option,
                                       String value, String section);
#elif DOXYGEN_PY
None Testutils::change_sandbox_conf(int port, str option, str value,
                                    str section);
#endif
///@}
void Testutils::change_sandbox_conf(int port, const std::string &option,
                                    const std::string &value,
                                    const std::string &section_) {
  if (option == "server-uuid" || option == "server_uuid") {
    change_sandbox_uuid(port, value);
    return;
  }

  std::string section = section_.empty() ? "mysqld" : section_;

  std::string cfgfile_path = get_sandbox_conf_path(port);
  bool file_exists = shcore::is_file(cfgfile_path);
  if (!file_exists && _skip_server_interaction) return;

  mysqlshdk::config::Config_file cfg;
  if (file_exists) {
    // Read the file, only if it exists.
    cfg.read(cfgfile_path);
  }

  if (section == "*") {
    // Change all groups (sections) if * is used for the group.
    std::vector<std::string> groups = cfg.groups();
    for (auto const &group : groups) {
      cfg.set(group, option, mysqlshdk::utils::nullable<std::string>(value));
    }
  } else {
    if (!cfg.has_group(section)) {
      // Create the group (section) if it does not exist.
      cfg.add_group(section);
    }
    cfg.set(section, option, mysqlshdk::utils::nullable<std::string>(value));
  }

  // Apply change to option file.
  cfg.write(cfgfile_path);
}

/**
 * Searches $PATH for the given executable and returns its canonical path.
 * @param exec_name The name of the executable we want to find
 * @return the canonical path of the executable found on $PATH
 */
std::string Testutils::get_executable_path(const std::string &exec_name) {
  std::string exec_path = shcore::path::search_stdpath(exec_name);

  if (exec_path.empty()) {
    throw ::std::runtime_error(shcore::str_format(
        "Unable to find executable '%s' on PATH.", exec_name.c_str()));
  }

  // Make sure we're using canonical path;
  return shcore::path::get_canonical_path(exec_path);
}

//!<  @name Sandbox Operations
///@{
/**
 * Upgrade the mysqld executable used by the sandbox with the one found in
 * $PATH.
 * @param port The port of the sandbox to upgrade.
 *
 * This function will replace the value of the basedir in the sandbox
 * configuration file and update the start script.
 *
 */
#if DOXYGEN_JS
Undefined Testutils::upgradeSandbox(Integer port);
#elif DOXYGEN_PY
None Testutils::upgrade_sandbox(int port);
#endif
///@}
void Testutils::upgrade_sandbox(int port) {
  const std::string mysqld_path = get_executable_path("mysqld");
  log_info("Upgrading sandbox (%d) using mysqld executable %s ...", port,
           mysqld_path.c_str());
#ifdef _WIN32
  std::string start("start.bat");
#else
  std::string start("start.sh");
#endif

  // When replaying we don't need to do anything
  mysqlshdk::db::replay::No_replay dont_record;
  if (_skip_server_interaction) return;

  // Change basedir to point to the basedir of the new executable
  // Do the same MP does to find the basedir from the executable path, assuming
  // it is inside a bin or sbin folder
  const std::string path_sep = std::string(1, shcore::path::path_separator);
  auto mysqld_path_vec = shcore::split_string(mysqld_path, path_sep);

  auto bin_pos = std::find_if(mysqld_path_vec.rbegin(), mysqld_path_vec.rend(),
                              [](const std::string &s) {
                                return (shcore::str_lower(s) == "bin" ||
                                        shcore::str_lower(s) == "sbin");
                              });

  // if no bin/sbin folder is found, throw an error
  if (bin_pos == mysqld_path_vec.rend()) {
    throw ::std::runtime_error("Unable to find basedir of mysqld executable " +
                               mysqld_path);
  }
  const std::string basedir =
      shcore::str_join(mysqld_path_vec.begin(), bin_pos.base() - 1, path_sep);

  // Update the basedir on the config file
  change_sandbox_conf(port, "basedir", basedir, "mysqld");

  // Remove mysqlx plugin load since it is installed by default on 8.0
  remove_from_sandbox_conf(port, "plugin_load", "mysqld");

  // Update the start script. We can leave the stop script since even
  // the oldest mysqladmin versions that the shell supports are able to
  // shutdown 5.7 or 8.0 versions of MySQL.
  std::string sandbox_basedir =
      shcore::path::join_path(_sandbox_dir, std::to_string(port));
  // create lambda function to replace old mysqld executable with canonical
  // mysqld path.
  auto replace_func = [&mysqld_path](const std::string &line) {
    auto found = line.find(" --defaults-file=");
    if (found != std::string::npos) {
      std::string line_copy{line};
      line_copy.replace(0, found, mysqld_path);
      return line_copy;
    } else {
      return line;
    }
  };
  // Replace the old mysqld executable on the start script with the new
  // mysqld_path
  process_file_lines(shcore::path::join_path(sandbox_basedir, start),
                     replace_func);
}

/**
 * Change sandbox server UUID.
 *
 * This function will set the server UUID of a sandbox by changing it in the
 * corresponding auto.cnf file. If the auto.cnf file does not exist then it
 * will be created.
 *
 * Note: The sandbox need to be restarted for the change to take effect if
 *       already running.
 *
 * @param port The port of the sandbox where the configuration will be updated.
 * @param server_uuid A string with the server UUID value to set.
 */
void Testutils::change_sandbox_uuid(int port, const std::string &server_uuid) {
  // Get path of the auto.cnf file (containing the server_uuid information).
  std::string autocnf_path =
      shcore::path::join_path(get_sandbox_datadir(port), "auto.cnf");

  if (!shcore::is_file(autocnf_path) && _skip_server_interaction) return;

  // Check if the auto.cnf file exists (only created on the first server start).
  if (shcore::is_file(autocnf_path)) {
    // Replace the server-uuid with the new value in the auto.cnf file.
    std::string new_autocnf_path = autocnf_path + ".new";
    std::ofstream new_autocnf_file(new_autocnf_path);
    std::ifstream autocnf_file(autocnf_path);
    std::string line;
    bool in_section = false;

    while (std::getline(autocnf_file, line)) {
      if (is_section_line(line, "auto")) {
        // As soon as the "auto" section is found adds the new server-uuid
        // value.
        in_section = true;
        new_autocnf_file << line << std::endl;
        new_autocnf_file << "server-uuid=" << server_uuid << std::endl;
        continue;
      } else if (is_section_line(line)) {
        in_section = false;
      }

      // If we are in the "auto" section and the server-uuid option is found,
      // it is removed since it will be the old value.
      if (in_section) {
        if (is_configuration_option("server-uuid", line)) continue;
      }
      new_autocnf_file << line << std::endl;
    }
    autocnf_file.close();
    new_autocnf_file.close();
    shcore::delete_file(autocnf_path);
    try_rename(new_autocnf_path, autocnf_path);
  } else {
    // File does not exist, create a new one with the desired server UUID.
    std::string file_contents = "[auto]\nserver-uuid=" + server_uuid + "\n";
    shcore::create_file(autocnf_path, file_contents);
  }
}

//!<  @name InnoDB Cluster Utilities
///@{
/**
 * Waits until a cluster member reaches one of the specified states.
 * @param port The port of the instance to be polled listens for MySQL
 * connections.
 * @param states An array containing the states that would cause the poll cycle
 * to finish.
 * @param direct If true, opens a direct session to the member to be observed.
 * @returns The state of the member.
 *
 * This function is to be used with the members of a cluster.
 *
 * It will start a polling cycle verifying the member state, the cycle will end
 * when one of the expected states is reached or if the timeout of
 * k_wait_member_timeout seconds occurs.
 */
#if DOXYGEN_JS
String Testutils::waitMemberState(Integer port, String[] states,
                                  bool direct = false);
#elif DOXYGEN_PY
str Testutils::wait_member_state(int port, str[] states, bool direct = False);
#endif
///@}
std::string Testutils::wait_member_state(int member_port,
                                         const std::string &states,
                                         bool direct) {
  if (states.empty())
    throw std::invalid_argument(
        "states argument for wait_member_state() can't be empty");

  std::string current_state;
  int curtime = 0;
  std::shared_ptr<mysqlshdk::db::ISession> session;
  if (direct) {
    session = connect_to_sandbox(member_port);
  } else {
    // Use the shell's active session
    if (auto shell = _shell.lock()) {
      if (!shell->shell_context()->get_dev_session())
        throw std::runtime_error("No active shell session");
      session = shell->shell_context()->get_dev_session()->get_core_session();
      if (!session || !session->is_open()) {
        throw std::logic_error(
            "The testutil.waitMemberState method uses the active shell "
            "session and requires it to be open.");
      }
    } else {
      throw std::logic_error("Lost reference to shell object");
    }
  }

  while (curtime < k_wait_member_timeout) {
    auto result = session->query(
        "SELECT member_state FROM "
        "performance_schema.replication_group_members "
        "WHERE member_port = " +
        std::to_string(member_port));
    current_state = "(MISSING)";
    if (auto row = result->fetch_one()) {
      current_state = row->get_string(0);
    }
    if (states.find(current_state) != std::string::npos) {
      if (direct) session->close();
      return current_state;
    }

    if (mysqlshdk::db::replay::g_replay_mode ==
        mysqlshdk::db::replay::Mode::Replay) {
      // in replay mode we can wait much less (or not at all)
      shcore::sleep_ms(1);
    } else {
      shcore::sleep_ms(1000);
    }
    curtime++;
  }

  // Print some debugging info
  auto result = session->query(
      "SELECT member_id, member_host, member_port, member_state FROM "
      "performance_schema.replication_group_members");
  std::cout << "replication_group_members as seen from the monitoring session: "
            << session->get_connection_options().as_uri(
                   mysqlshdk::db::uri::formats::only_transport())
            << "\n";
  while (auto row = result->fetch_one()) {
    std::cout << row->get_as_string(0) << "\t" << row->get_as_string(1) << "\t"
              << row->get_as_string(2) << "\t" << row->get_as_string(3) << "\n";
  }
  // If we were waiting for the instance to be online and it got stuck in
  // recovering, try to get some helpful information
  if (states.find("ONLINE") != std::string::npos &&
      (current_state == "RECOVERING")) {
    std::shared_ptr<mysqlshdk::db::ISession> direct_session;
    direct_session = connect_to_sandbox(member_port);
    result = direct_session->query(
        "SELECT CHANNEL_NAME, SERVICE_STATE, LAST_ERROR_NUMBER, "
        "LAST_ERROR_MESSAGE FROM "
        "performance_schema.replication_connection_status");
    std::cout << "replication_connection_status seen from the stuck instance: "
              << direct_session->get_connection_options().as_uri(
                     mysqlshdk::db::uri::formats::only_transport())
              << ": \n";
    while (auto row = result->fetch_one()) {
      std::cout << row->get_as_string(0) << "\t" << row->get_as_string(1)
                << "\t" << row->get_as_string(2) << "\t"
                << row->get_as_string(3) << "\n";
    }

    std::cout << "MySQL users with passwords as seen from the monitoring "
                 "session: "
              << session->get_connection_options().as_uri(
                     mysqlshdk::db::uri::formats::only_transport())
              << ": \n";
    result = session->query(
        "SELECT user, host, authentication_string from mysql.user");
    while (auto row = result->fetch_one()) {
      std::cout << row->get_as_string(0) << "\t" << row->get_as_string(1)
                << "\t" << row->get_as_string(2) << "\n";
    }

    std::cout << "MySQL recovery information as seen from the stuck instance: "
              << direct_session->get_connection_options().as_uri(
                     mysqlshdk::db::uri::formats::only_transport())
              << ": \n";
    result = direct_session->query(
        "SELECT User_name, User_password FROM mysql.slave_master_info");
    while (auto row = result->fetch_one()) {
      std::cout << row->get_as_string(0) << "\t" << row->get_as_string(1)
                << "\n";
    }

    direct_session->close();
  }
  if (direct) session->close();

  throw std::runtime_error(
      "Timeout while waiting for cluster member to become one of " + states +
      ": seems to be stuck as " + current_state);
}

///@{
/**
 * Waits until the given repl connection channel errors out.
 */
#if DOXYGEN_JS
Undefined Testutils::waitForReplConnectionError(Integer port, String channel);
#elif DOXYGEN_PY
None Testutils::wait_for_repl_connection_error(int port, str channel);
#endif
///@}
int Testutils::wait_for_repl_connection_error(int port,
                                              const std::string &channel) {
  int current_error_number = 0;
  int elapsed_time = 0;
  std::shared_ptr<mysqlshdk::db::ISession> session = connect_to_sandbox(port);

  const uint32_t sleep_time = mysqlshdk::db::replay::g_replay_mode ==
                                      mysqlshdk::db::replay::Mode::Replay
                                  ? 1
                                  : 1000;

  int timeout = k_wait_repl_connection_error;
  while (elapsed_time < timeout) {
    auto result = session->queryf(
        "SELECT LAST_ERROR_NUMBER FROM "
        "performance_schema.replication_connection_status "
        "WHERE channel_name = ?",
        channel);

    if (auto row = result->fetch_one()) {
      current_error_number = row->get_int(0);
      if (current_error_number != 0) break;
    }

    elapsed_time += 1;
    shcore::sleep_ms(sleep_time);
  }

  session->close();
  if (current_error_number == 0) {
    throw std::runtime_error(
        "Timeout waiting for a connection error in replication channel");
  }
  return current_error_number;
}

///@{
/**
 * Waits until the given rpl applier channel stops and errors out.
 */
#if DOXYGEN_JS
Undefined Testutils::waitForRplApplierError(Integer port, String channel);
#elif DOXYGEN_PY
None Testutils::wait_for_rpl_applier_error(int port, str channel);
#endif
///@}
int Testutils::wait_for_rpl_applier_error(int port,
                                          const std::string &channel) {
  int elapsed_time = 0;
  int last_error_num = 0;
  std::shared_ptr<mysqlshdk::db::ISession> session = connect_to_sandbox(port);
  const mysqlshdk::db::IRow *row = nullptr;

  const uint32_t sleep_time = mysqlshdk::db::replay::g_replay_mode ==
                                      mysqlshdk::db::replay::Mode::Replay
                                  ? 1
                                  : 1000;

  int timeout = k_wait_repl_connection_error;
  std::string service_state = "ON";
  std::shared_ptr<mysqlshdk::db::IResult> result;

  while (elapsed_time < timeout) {
    // NOTE: It is assumed that MTS is not used (checking status of one worker).
    result = session->queryf(
        "SELECT service_state, last_error_number, last_error_message, "
        "last_error_timestamp "
        "FROM performance_schema.replication_applier_status_by_worker "
        "WHERE channel_name = ?",
        channel);

    row = result->fetch_one();
    if (row) {
      service_state = row->get_string(0);
      // Stop if applier is of (error found)
      if (service_state == "OFF") {
        last_error_num = row->get_int(1);
        break;
      }
    }

    elapsed_time += 1;
    shcore::sleep_ms(sleep_time);
  }

  if (last_error_num == 0) {
    // Print some debug information if timeout is reached.
    if (row) {
      std::cout << "Applier error information for channel '" << channel.c_str()
                << "' at sandbox " << std::to_string(port).c_str() << ":"
                << std::endl;
      std::cout << "\t service_state: " << row->get_string(0).c_str()
                << std::endl;
      if (!row->is_null(1)) {
        std::cout << "\t last_error_number: "
                  << std::to_string(row->get_int(1)).c_str() << std::endl;
        std::cout << "\t last_error_message: " << row->get_string(2).c_str()
                  << std::endl;
        std::cout << "\t last_error_timestamp: "
                  << row->get_as_string(3).c_str() << std::endl;
      } else {
        std::cout << "\t No last error information (last_error_number is NULL)"
                  << std::endl;
      }
    } else {
      std::cout << "No applier error information found for channel '"
                << channel.c_str() << "' at sandbox "
                << std::to_string(port).c_str() << "!" << std::endl;
    }

    throw std::runtime_error(
        "Timeout waiting for applier error in replication channel");
  }
  return last_error_num;
}

///@{
/**
 * Waits until destination sandbox finishes applying transactions that
 * were executed in the source sandbox. This is checked using
 * WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS()
 * @param dest_port sandbox port waiting for the txs
 * @param source_port sandbox port where the txs originate from (the primary).
 *            If 0, it will use the active shell session.
 * @returns true on success, fail on timeout
 *
 * Throws exception on error (e.g. GR not running)
 */
#if DOXYGEN_JS
Boolean Testutils::waitMemberTransactions(Integer destPort,
                                          Integer sourcePort = 0);
#elif DOXYGEN_PY
bool Testutils::wait_member_transactions(int destPort, int sourcePort = 0);
#endif
///@}
bool Testutils::wait_member_transactions(int dest_port, int source_port) {
  std::shared_ptr<mysqlshdk::db::ISession> source;

  if (source_port == 0) {
    if (auto shell = _shell.lock()) {
      if (!shell->shell_context()->get_dev_session())
        throw std::runtime_error("No active shell session");
      source = shell->shell_context()->get_dev_session()->get_core_session();
      if (!source || !source->is_open()) {
        throw std::logic_error(
            "The testutil.waitMemberState method uses the active shell "
            "session and requires it to be open.");
      }
    } else {
      throw std::logic_error("Lost reference to shell object");
    }
  } else {
    source = connect_to_sandbox(source_port);
  }
  // Must get the value of the 'gtid_executed' variable with GLOBAL scope to get
  // the GTID of ALL transactions, otherwise only a set of transactions written
  // to the cache in the current session might be returned.
  std::string gtid_set = source->query("select @@global.gtid_executed")
                             ->fetch_one()
                             ->get_string(0);

  std::shared_ptr<mysqlshdk::db::ISession> dest;
  dest = connect_to_sandbox(dest_port);

  auto result = dest->queryf("select WAIT_FOR_EXECUTED_GTID_SET(?, ?)",
                             gtid_set, k_wait_member_timeout);
  auto row = result->fetch_one();
  // NOTE: WAIT_FOR_EXECUTED_GTID_SET() does not return NULL like
  // WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS(), instead an error is generated.
  // 0 is returned for success and 1 for timeout.
  return row->get_int(0) == 0;
}

//!<  @name Misc Utilities
///@{
/**
 * Changes access attributes to a file to be read only.
 * @param path The path to the file to be made read only.
 * @returns 0 on success, -1 on failure
 */
#if DOXYGEN_JS
Integer Testutils::makeFileReadonly(String path);
#elif DOXYGEN_PY
int Testutils::make_file_readonly(str path);
#endif
///@}
int Testutils::make_file_readonly(const std::string &path) {
  return shcore::make_file_readonly(path);
}

//!< @name File Operations
///@{
/**
 * Creates a folder with the given name.
 * @param path The name of the folder to be created.
 * @param recursive To create the complete directory hierarchy if the given name
 * contains subdirectories.
 */
#if DOXYGEN_JS
Undefined Testutils::mkdir(String path, Boolean recursive);
#elif DOXYGEN_PY
None Testutils::mkdir(str path, bool recursive);
#endif
///@}
void Testutils::mk_dir(const std::string &path, bool recursive) {
  shcore::create_directory(path, recursive);
}

//!< @name File Operations
///@{
/**
 * Deletes a folder with the given name.
 * @param path The name of the folder to be deleted.
 * @param recursive To remove the folder and its contents.
 */
#if DOXYGEN_JS
Undefined Testutils::rmdir(String path, Boolean recursive);
#elif DOXYGEN_PY
None Testutils::rmdir(str path, bool recursive);
#endif
///@}
void Testutils::rm_dir(const std::string &path, bool recursive) {
  shcore::remove_directory(path, recursive);
}

//!< @name File Operations
///@{
/**
 * Changes the file permissions of the indicated path.
 * @param path The path of the file/folder which will be updated.
 * @param mode The User/Group/Other permissions to be set.
 *
 * Permissions should be given in binary mask format, this is
 * as a number in the format of 0UGO
 *
 * Where:
 *  @li U is the binary mask for User's permissions
 *  @li G is the binary mask for User Group's permissions
 *  @li O is the binary mask for Other's permissions
 *
 * Each binary mask contains the a permission combination that includes:
 * @li 0x001: Indicates execution permission
 * @li 0x010: Indicates write permission
 * @li 0x100: Indicates read permission
 *
 * On Windows, only the User permissions are considered:
 * @li if the user write permission is OFF, the file will be set as
 * Read Only.
 * @li if the user write permission is ON, the Read Only flag will be
 * removed from the file.
 *
 * This function does not work with Windows folders.
 */
#if DOXYGEN_JS
Integer Testutils::chmod(String path, Integer mode);
#elif DOXYGEN_PY
int Testutils::chmod(str path, int mode);
#endif
///@}
int Testutils::ch_mod(const std::string &path, int mode) {
  return shcore::ch_mod(path, mode);
}

//!< @name File Operations
///@{
/**
 * Creates a copy of a file.
 * @param source The name of the file to be copied
 * @param target The name of the new file.
 */
#if DOXYGEN_JS
Undefined Testutils::cpfile(String source, String target);
#elif DOXYGEN_PY
None Testutils::cpfile(str source, str target);
#endif
///@}
void Testutils::cp_file(const std::string &source, const std::string &target) {
  shcore::copy_file(source, target, true);
}

//!< @name File Operations
///@{
/**
 * Deletes a file matching a glob pattern
 * @param path The name of the file to be deleted.
 */
#if DOXYGEN_JS
Undefined Testutils::rmfile(String path);
#elif DOXYGEN_PY
None Testutils::rmfile(str path);
#endif
///@}
void Testutils::rm_file(const std::string &target) {
  std::string dir = shcore::path::dirname(target);
  std::string file = shcore::path::basename(target);
  shcore::iterdir(dir, [=](const std::string &f) {
    if (shcore::match_glob(file, f)) {
      shcore::delete_file(shcore::path::join_path(dir, f), false);
    }
    return true;
  });
}

//!< @name File Operations
///@{
/**
 * Renames a file or directory.
 * @param path The name of the file to be renamed.
 * @param newpath The new name of the file.
 */
#if DOXYGEN_JS
Undefined Testutils::rename(String path, String newpath);
#elif DOXYGEN_PY
None Testutils::rename(str path, str newpath);
#endif
///@}
void Testutils::rename(const std::string &path, const std::string &newpath) {
  shcore::rename_file(path, newpath);
}

//!< @name File Operations
///@{
/**
 * If file does not exists is created empty.
 */
#if DOXYGEN_JS
Undefined Testutils::touch(String file);
#elif DOXYGEN_PY
None Testutils::touch(str file);
#endif
///@}
void Testutils::touch(const std::string &file) {
  if (!shcore::is_file(file)) {
    make_empty_file(file);
  }
}

//!< @name File Operations
///@{
/**
 * Create empty file specified by path and create parent directories needed by
 * path if they do not already exists.
 */
#if DOXYGEN_JS
Undefined Testutils::makeEmptyFile(String path);
#elif DOXYGEN_PY
None Testutils::make_empty_file(str path);
#endif
///@}
void Testutils::make_empty_file(const std::string &path) {
  auto directory = shcore::path::dirname(path);
  shcore::create_directory(directory, true);
  shcore::create_file(path, "");
}

//!< @name File Operations
///@{
/**
 * Create file specified by path with the specified content.
 * Create parent directories needed by path if they do not already exists.
 */
#if DOXYGEN_JS
Undefined Testutils::createFile(String path, String content);
#elif DOXYGEN_PY
None Testutils::create_file(str path, str content);
#endif
///@}
void Testutils::create_file(const std::string &path,
                            const std::string &content) {
  auto directory = shcore::path::dirname(path);
  shcore::create_directory(directory, true);
  shcore::create_file(path, content);
}

//!< @name File Operations
///@{
/**
 * Search for a pattern on a file.
 * @param path The path to the file where the pattern will be searched.
 * @param pattern The pattern to be searched on the file.
 * @returns Array containing the matching lines.
 *
 * This function will read each line of the file and match it using the provided
 * glob-like pattern using backtracking.
 *
 * Note: works with ASCII only, no utf8 support.
 *
 * This function will return all the lines that matched the given pattern.
 */
#if DOXYGEN_JS
List Testutils::grepFile(String path, String pattern);
#elif DOXYGEN_PY
list Testutils::grep_file(str path, str pattern);
#endif
///@}
shcore::Array_t Testutils::grep_file(const std::string &path,
                                     const std::string &pattern) {
  std::ifstream f(path);
  if (!f.good())
    throw std::runtime_error("grep error: " + path + ": " + strerror(errno));
  shcore::Array_t result = shcore::make_array();
  while (!f.eof()) {
    std::string line;
    std::getline(f, line);
    if (shcore::match_glob("*" + pattern + "*", line))
      result->push_back(shcore::Value(line));
  }
  f.close();
  return result;
}

//!< @name File Operations
///@{
/**
 * Reads a file line by line into an array, much like Unix's cat tool.
 * @param path The path to the file.
 * @returns Array containing the contents of the file.
 *
 * This function will read each line of the file and store it in the result
 * array.
 *
 * This function will return all the content of the given file.
 */
#if DOXYGEN_JS
List Testutils::catFile(String path);
#elif DOXYGEN_PY
list Testutils::cat_file(str path);
#endif
///@}
std::string Testutils::cat_file(const std::string &path) {
  std::ifstream in(path, std::ios::in | std::ios::binary);
  if (!in.good()) {
    throw std::runtime_error("cat error: " + path + ": " + strerror(errno));
  } else {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return (contents);
  }
}

//!<  @name Misc Utilities
///@{
/**
 * Wipes the contents of a file.
 * @param path The path to the file.
 *
 * This function will truncate a file, wiping its content.
 */
#if DOXYGEN_JS
List Testutils::wipeFileContents(String path);
#elif DOXYGEN_PY
list Testutils::wipe_file_contents(str path);
#endif
///@}
void Testutils::wipe_file_contents(const std::string &path) {
  std::ifstream f(path);
  if (!f.good())
    throw std::runtime_error("wipe_file_contents error: " + path + ": " +
                             strerror(errno));
  f.close();
  std::ofstream out(path, std::ios::out | std::ios::trunc);  // truncating file
  out.close();
}

//!<  @name Misc Utilities
///@{
/**
 * Copy file replacing ${VARIABLE}s
 */
#if DOXYGEN_JS
List Testutils::preprocessFile(String path);
#elif DOXYGEN_PY
list Testutils::preprocess_file(str path);
#endif
///@}
void Testutils::preprocess_file(const std::string &in_path,
                                const std::vector<std::string> &vars,
                                const std::string &out_path,
                                const std::vector<std::string> &skip_sections) {
  std::string data = shcore::get_text_file(in_path);

  data = shcore::str_subvars(
      data,
      [&vars](const std::string &s) {
        for (const auto &v : vars) {
          auto p = v.find('=');
          if (p != std::string::npos) {
            if (v.substr(0, p) == s) {
              return v.substr(p + 1);
            }
          }
        }
        return s;
      },
      "${", "}");

  // Removes sections enclosed by a specific indicator
  if (!skip_sections.empty()) {
    for (const auto &section : skip_sections) {
      bool found = false;
      auto split1 = shcore::str_partition(data, section, &found);
      if (found) {
        auto split2 = shcore::str_partition(split1.second, section, &found);
        if (found) {
          data = split1.first + split2.second;
        }
      }
    }
  }

  shcore::create_file(out_path, data);
}

//!<  @name Testing Utilities
///@{
/**
 * Sets an expected prompt as well as the response to be given.
 * @param prompt The prompt to be expected.
 * @param answer The answer to be given when the prompt is received.
 *
 * Some of the interative functions of the shell require information from the
 * user, this is done through prompts.
 *
 * This function can be used to identify an expected prompt as well as defining
 * the response that should be given to that prompt.
 *
 * If something different than the expected is prompted, will cause the test to
 * fail.
 *
 * If the prompt matches the expected prompt, the answer associated to the
 * prompt will be given.
 *
 * Use * on the prompt to cause any prompt to be valid (bypass the expected
 * prompt validation).
 */
#if DOXYGEN_JS
Undefined Testutils::expectPrompt(String prompt, String answer);
#elif DOXYGEN_PY
None Testutils::expect_prompt(str prompt, str answer);
#endif
///@}
void Testutils::expect_prompt(const std::string &prompt,
                              const std::string &text) {
  _feed_prompt(prompt, text);
}

//!<  @name Testing Utilities
///@{
/**
 * Verifies that all the expected prompts were consumed.
 *
 * This function can be used at the end of the execution of a test chunk to
 * ensure all the expected prompts and passwords were consumed.
 */
#if DOXYGEN_JS
Undefined Testutils::assertNoPrompts();
#elif DOXYGEN_PY
None Testutils::assert_no_prompts();
#endif
///@}
void Testutils::assert_no_prompts() { _assert_no_prompts(); }

//!<  @name Testing Utilities
///@{
/**
 * Cleans up the output accumulated so far from STDOUT, STDERR and LOG
 */
#if DOXYGEN_JS
Undefined Testutils::wipeAllOutput();
#elif DOXYGEN_PY
None Testutils::wipe_all_output();
#endif
///@}
void Testutils::wipe_all_output() { _wipe_all_output(); }

//!<  @name Testing Utilities
///@{
/**
 * Sets an expected password prompt as well as the password to be given as
 * response.
 * @param prompt The prompt to be expected.
 * @param password The password to be given when the password prompt is
 * received.
 *
 * Some of the interative functions of the shell require a password from the
 * user, this is done through password prompts.
 *
 * This function can be used to identify an expected password prompt as well as
 * defining the password that should be given to that prompt.
 *
 * If something different than the expected is prompted, will cause the test to
 * fail.
 *
 * If the password prompt matches the expected password prompt, the password
 * associated to the prompt will be given.
 *
 * Use * on the prompt to cause any password prompt to be valid (bypass the
 * expected password prompt validation).
 */
#if DOXYGEN_JS
Undefined Testutils::expectPassword(String prompt, String password);
#elif DOXYGEN_PY
None Testutils::expect_password(str prompt, str password);
#endif
///@}
void Testutils::expect_password(const std::string &prompt,
                                const std::string &text) {
  _feed_password(prompt, text);
}

//!<  @name Testing Utilities
///@{
/**
 * Prints a string for debugging, without affecting the checked test output.
 *
 * Output is enabled if tracing is enabled.
 */
#if DOXYGEN_JS
Undefined Testutils::dprint(String s);
#elif DOXYGEN_PY
None Testutils::dprint(str s);
#endif
///@}
void Testutils::dprint(const std::string &s) {
#ifdef ENABLE_SESSION_RECORDING
  fprintf(stderr, "%s\n", s.c_str());
#else
  if (g_test_trace_scripts && _test_env) _test_env->debug_print(makegreen(s));
#endif
}

//!<  @name Testing Utilities
///@{
/**
 * Generates a named CA that can be used to create certificates.
 *
 * @param name a name for the CA to be created
 */
#if DOXYGEN_JS
Undefined Testutils::sslCreateCa(String s);
#elif DOXYGEN_PY
None Testutils::ssl_create_ca(str s);
#endif
///@}
void Testutils::ssl_create_ca(const std::string &name) {
  if (!_skip_server_interaction) {
    std::string basedir = shcore::path::join_path(_sandbox_dir, "ssl");
    if (!shcore::path_exists(basedir)) shcore::create_directory(basedir);

    std::string key_path = shcore::path::join_path(basedir, name + "-key.pem");
    std::string req_path = shcore::path::join_path(basedir, name + "-req.pem");
    std::string ca_path = shcore::path::join_path(basedir, name + ".pem");

    std::string cmd;

    std::string cav3_ext = shcore::path::join_path(basedir, "cav3.ext");
    std::string certv3_ext = shcore::path::join_path(basedir, "certv3.ext");
    shcore::delete_file(cav3_ext);
    shcore::delete_file(certv3_ext);

    shcore::create_file(cav3_ext, "basicConstraints=CA:TRUE\n");
    shcore::create_file(certv3_ext, "basicConstraints=CA:TRUE\n");

    std::string keyfmt =
        "openssl req -newkey rsa:2048 -days 3650 -nodes -keyout $ca-key.pem$ "
        "-subj /CN=A_test_CA_called_$cn$ -out $ca-req.pem$ "
        "&& openssl rsa -in $ca-key.pem$ -out $ca-key.pem$";

    cmd = shcore::str_replace(keyfmt, "$ca-key.pem$", key_path.c_str());
    cmd = shcore::str_replace(cmd, "$cn$", name.c_str());
    cmd = shcore::str_replace(cmd, "$ca-req.pem$", req_path.c_str());

    dprint("-> " + cmd);
    if (system(cmd.c_str()) != 0)
      throw std::runtime_error("CA creation failed");

    std::string certfmt =
        "openssl x509 -sha256 -days 3650 -extfile $cav3.ext$ -CAcreateserial "
        "-req -in $ca-req.pem$ -signkey $ca-key.pem$ -out $ca.pem$";
    cmd = shcore::str_replace(certfmt, "$ca-key.pem$", key_path.c_str());
    cmd = shcore::str_replace(cmd, "$ca-req.pem$", req_path.c_str());
    cmd = shcore::str_replace(cmd, "$ca.pem$", ca_path.c_str());
    cmd = shcore::str_replace(cmd, "$cav3.ext$", cav3_ext.c_str());

    dprint("-> " + cmd);
    if (system(cmd.c_str()) != 0)
      throw std::runtime_error("CA creation failed");

    shcore::delete_file(req_path);
    shcore::delete_file(cav3_ext);
    shcore::delete_file(certv3_ext);
  }
}

//!<  @name Testing Utilities
///@{
/**
 * Generats client and server certs signed by the named CA.
 *
 * @param sbport port of the sandbox that will use the certificates
 * @param caname the name of the CA as given to sslCreateCa
 * @param servercn the CN to be used for the server certificate
 * @param clientcn the CN to be used for the client certificate
 *
 * Both client and server certificates signed by the CA that was given will
 * be created and replace the equivalent ones in the given sandbox.
 *
 * The sandbox must be restarted for the changes to take effect.
 *
 * Output is enabled if tracing is enabled.
 */
#if DOXYGEN_JS
Undefined Testutils::sslCreateCerts(Integer sbport, String caname,
                                    String servercn, String clientcn);
#elif DOXYGEN_PY
None Testutils::ssl_create_certs(int sbport, str caname, str servercn,
                                 str clientcn);
#endif
///@}
void Testutils::ssl_create_certs(int sbport, const std::string &caname,
                                 const std::string &servercn,
                                 const std::string &clientcn) {
  if (!_skip_server_interaction) {
    std::string basedir = shcore::path::join_path(_sandbox_dir, "ssl");
    std::string sbdir = get_sandbox_datadir(sbport);

    std::string ca_key_path =
        shcore::path::join_path(basedir, caname + "-key.pem");
    std::string ca_path = shcore::path::join_path(basedir, caname + ".pem");

    // first copy the CA to the target dir
    shcore::copy_file(ca_path, shcore::path::join_path(sbdir, "ca.pem"));
    // Note: in reality, I don't think we would deploy the CA key too, but we do
    // to overwrite the one generated by default
    shcore::copy_file(ca_path, shcore::path::join_path(sbdir, "ca-key.pem"));

    auto mkcert = [&](const std::string &name, const std::string &cn) {
      std::string key_path = shcore::path::join_path(sbdir, name + "-key.pem");
      std::string req_path = shcore::path::join_path(sbdir, name + "-req.pem");
      std::string cert_path =
          shcore::path::join_path(sbdir, name + "-cert.pem");

      std::string cmd;

      std::string cav3_ext = shcore::path::join_path(sbdir, "cav3.ext");
      std::string certv3_ext = shcore::path::join_path(sbdir, "certv3.ext");
      shcore::delete_file(cav3_ext);
      shcore::delete_file(certv3_ext);

      shcore::create_file(cav3_ext, "basicConstraints=CA:TRUE\n");
      shcore::create_file(certv3_ext, "basicConstraints=CA:TRUE\n");

      std::string keyfmt =
          "openssl req -newkey rsa:2048 -days 3650 -nodes -keyout "
          "$cert-key.pem$ "
          "-subj '/CN=$cn$' -out $cert-req.pem$ "
          "&& openssl rsa -in $cert-key.pem$ -out $cert-key.pem$";

      cmd = shcore::str_replace(keyfmt, "$cert-key.pem$", key_path.c_str());
      cmd = shcore::str_replace(cmd, "$cn$", cn.c_str());
      cmd = shcore::str_replace(cmd, "$cert-req.pem$", req_path.c_str());

      dprint("-> " + cmd);
      if (system(cmd.c_str()) != 0)
        throw std::runtime_error("CA creation failed");

      std::string certfmt =
          "openssl x509 -sha256 -days 3650 -extfile $cav3.ext$ "
          "-req -in $cert-req.pem$ -CA $ca.pem$ -CAkey $ca-key.pem$ "
          "-CAcreateserial -out $cert.pem$";
      cmd = shcore::str_replace(certfmt, "$ca-key.pem$", ca_key_path.c_str());
      cmd = shcore::str_replace(cmd, "$ca.pem$", ca_path.c_str());
      cmd = shcore::str_replace(cmd, "$cert-req.pem$", req_path.c_str());
      cmd = shcore::str_replace(cmd, "$cert.pem$", cert_path.c_str());
      cmd = shcore::str_replace(cmd, "$cav3.ext$", cav3_ext.c_str());

      dprint("-> " + cmd);
      if (system(cmd.c_str()) != 0)
        throw std::runtime_error("CA creation failed");

      shcore::delete_file(req_path);
      shcore::delete_file(cav3_ext);
      shcore::delete_file(certv3_ext);
    };

    // then generate the server certs
    mkcert("server", servercn);

    // and finally the client certs
    mkcert("client", clientcn);
  }
}

std::string Testutils::fetch_captured_stdout(bool eat_one) {
  return _fetch_stdout(eat_one);
}

std::string Testutils::fetch_captured_stderr(bool eat_one) {
  return _fetch_stderr(eat_one);
}

//!<  @name Testing Utilities
///@{
/**
 * Sets or clears the value of an environment variable.
 */
#if DOXYGEN_JS
Undefined Testutils::setenv(String var, String value);
#elif DOXYGEN_PY
None Testutils::setenv(str var, str value);
#endif
///@}
void Testutils::setenv(const std::string &var, const std::string &value) {
  if (value.empty())
    shcore::unsetenv(var);
  else
    shcore::setenv(var, value);
}

void Testutils::handle_sandbox_encryption(const std::string &path) const {
  shcore::create_directory(path);
  shcore::ch_mod(path, 0750);
}

void Testutils::handle_remote_root_user(const std::string &rootpass, int port,
                                        bool create_remote_root) const {
  // Connect to sandbox using default root@localhost account.
  auto session = mysqlshdk::db::mysql::Session::create();
  auto options = mysqlshdk::db::Connection_options("root@localhost");
  options.set_port(port);
  options.set_password(rootpass);
  session->connect(options);

  // Create root user to allow access using other hostnames, otherwise only
  // access through 'localhost' will be allowed leading to access denied
  // errors if the reported replication host (from the metadata) is used.
  // By default, create_remote_root = true.
  if (create_remote_root) {
    session->execute("SET sql_log_bin = 0");

    std::string remote_root = "%";
    shcore::sqlstring create_user(
        "CREATE USER IF NOT EXISTS root@? IDENTIFIED BY ?", 0);
    create_user << remote_root << rootpass;
    create_user.done();
    session->execute(create_user);

    shcore::sqlstring grant("GRANT ALL ON *.* TO root@? WITH GRANT OPTION", 0);
    grant << remote_root;
    grant.done();
    session->execute(grant);

    session->execute("SET sql_log_bin = 1");
  } else {
    // Drop the user in case it already exists (copied from boilerplate).
    session->execute("SET sql_log_bin = 0");
    session->execute("DROP USER IF EXISTS 'root'@'%'");
    session->execute("SET sql_log_bin = 1");
  }

  session->close();
}

void Testutils::prepare_sandbox_boilerplate(const std::string &rootpass,
                                            int port,
                                            const std::string &mysqld_path) {
  if (g_test_trace_scripts) std::cerr << "Preparing sandbox boilerplate...\n";

  std::string mysqld_version = get_mysqld_version(mysqld_path);

  std::string bp_folder{"myboilerplate"};
  if (!mysqld_path.empty()) bp_folder.append("-" + mysqld_version);
  std::string boilerplate = shcore::path::join_path(_sandbox_dir, bp_folder);
  if (shcore::is_folder(boilerplate) && _use_boilerplate) {
    if (g_test_trace_scripts)
      std::cerr << "Reusing existing sandbox boilerplate at " << boilerplate
                << "\n";
    return;
  }

  // Create a sandbox, shut it down and then keep a copy of its basedir
  // to be reused for future deployments

  shcore::Value::Array_type_ref errors;
  shcore::Value mycnf_options = shcore::Value::new_array();
  mycnf_options.as_array()->push_back(shcore::Value("innodb_log_file_size=1M"));
  mycnf_options.as_array()->push_back(
      shcore::Value("innodb_log_buffer_size=1M"));
  mycnf_options.as_array()->push_back(
      shcore::Value("innodb_data_file_path=ibdata1:10M:autoextend"));

  _mp.create_sandbox(port, port * 10, _sandbox_dir, rootpass, mycnf_options,
                     true, true, 60, mysqld_path, &errors);
  if (errors && !errors->empty()) {
    std::cerr << "Error deploying sandbox:\n";
    for (auto &v : *errors) std::cerr << v.descr() << "\n";
    throw std::runtime_error("Error deploying sandbox");
  }

  shcore::create_file(shcore::path::join_path(
                          _sandbox_dir, std::to_string(port), "version.txt"),
                      mysqld_version);

  stop_sandbox(port);

  wait_sandbox_dead(port);

  change_sandbox_conf(port, "port", "<PLACEHOLDER>", "*");
  remove_from_sandbox_conf(port, "server_id", "mysqld");
  remove_from_sandbox_conf(port, "datadir", "mysqld");
  remove_from_sandbox_conf(port, "log_error", "mysqld");
  remove_from_sandbox_conf(port, "pid_file", "mysqld");
  remove_from_sandbox_conf(port, "secure_file_priv", "mysqld");
  remove_from_sandbox_conf(port, "loose_mysqlx_port", "mysqld");
  remove_from_sandbox_conf(port, "report_port", "mysqld");

  if (shcore::is_folder(boilerplate)) {
    shcore::remove_directory(boilerplate);
  }

#ifdef _WIN32
  // We'll wait up to ~1 minute trying to do the folder rename
  // in case the server has not stopped.
  int attempts = 30;
  bool retry = true;
  while (retry && attempts) {
    try {
      shcore::rename_file(
          shcore::path::join_path(_sandbox_dir, std::to_string(port)),
          boilerplate);
      retry = false;
    } catch (const std::exception &err) {
      std::cout << "Failed attempt creating boilerplate sandbox: " << err.what()
                << std::endl;
      std::string message = err.what();
      if (message.find("Permission denied") != std::string::npos) {
        attempts--;
        shcore::sleep_ms(2000);
      } else {
        retry = false;
      }
    }
  }
#else
  shcore::rename_file(
      shcore::path::join_path(_sandbox_dir, std::to_string(port)), boilerplate);
#endif

  std::string sbdir = shcore::path::join_path(boilerplate, "sandboxdata");

  shcore::delete_file(shcore::path::join_path(sbdir, "auto.cnf"));
  shcore::delete_file(shcore::path::join_path(sbdir, "general.log"));
  shcore::delete_file(shcore::path::join_path(sbdir, "mysqld.sock"));
  shcore::delete_file(shcore::path::join_path(sbdir, "mysqlx.sock"));
  shcore::delete_file(shcore::path::join_path(sbdir, "error.log"));

  // Delete all SSL related files to force them to be re-created
  shcore::iterdir(sbdir, [&sbdir](const std::string &name) {
    if (shcore::str_endswith(name, ".pem"))
      shcore::delete_file(shcore::path::join_path(sbdir, name));
    return true;
  });

#ifdef _WIN32
  std::string start("start.bat");
  std::string stop("stop.bat");
#else
  std::string start("start.sh");
  std::string stop("stop.sh");
#endif

  // Replaces the original port on the start and stop scripts by a <SBPORT>
  // token
  replace_file_text(shcore::path::join_path(boilerplate, start),
                    std::to_string(port), "<SBPORT>");

  replace_file_text(shcore::path::join_path(boilerplate, stop),
                    std::to_string(port), "<SBPORT>");

  replace_file_text(shcore::path::join_path(boilerplate, start), _sandbox_dir,
                    "<SBDIR>");

  replace_file_text(shcore::path::join_path(boilerplate, stop), _sandbox_dir,
                    "<SBDIR>");

  if (g_test_trace_scripts)
    std::cerr << "Created sandbox boilerplate at " << boilerplate << "\n";
}

void copy_boilerplate_sandbox(const std::string &from, const std::string &to) {
  shcore::create_directory(to);
  shcore::iterdir(from, [from, to](const std::string &name) {
    try {
      std::string item_from = shcore::path::join_path(from, name);
      std::string item_to = shcore::path::join_path(to, name);

      if (shcore::is_folder(item_from)) {
        copy_boilerplate_sandbox(item_from, item_to);
      } else if (shcore::is_file(item_from)) {
#ifndef _WIN32
        if (name == "mysqld") {
          if (symlink(item_from.c_str(), item_to.c_str()) != 0) {
            throw std::runtime_error(shcore::str_format(
                "Unable to create symlink %s to %s: %s", item_from.c_str(),
                item_to.c_str(), strerror(errno)));
          }
          return true;
        }
#endif
        shcore::copy_file(item_from, item_to, true);
      }
    } catch (std::runtime_error &e) {
      if (errno != ENOENT) throw;
    }
    return true;
  });
}

void Testutils::validate_boilerplate(const std::string &sandbox_dir,
                                     bool delete_if_expired) {
  // TODO(rennox): Verify if we should be passing a specific path here
  std::string version = get_mysqld_version("");
  delete_if_expired = true;
  std::string basedir = shcore::path::join_path(sandbox_dir, "myboilerplate");
  bool expired = false;
  if (shcore::is_folder(basedir)) {
    std::string bversion;
    try {
      bversion = shcore::get_text_file(
          shcore::path::join_path(basedir, "version.txt"));
    } catch (const std::runtime_error &e) {
      std::cerr << "ERROR: " << e.what() << "\n";
      expired = true;
    }
    if (bversion != version) {
      std::cerr << "Sandbox boilerplate was created for " << bversion
                << " but mysqld in PATH is " << version << "\n";
      expired = true;
    }
    if (expired) {
      if (delete_if_expired) {
        std::cerr << "Deleting expired boilerplate at " << basedir << "...\n";
        log_info("DELET SANDBOX BOILER %s %s %s\n", basedir.c_str(),
                 version.c_str(), bversion.c_str());
        shcore::remove_directory(basedir);
      }
    }
  }
}

bool Testutils::deploy_sandbox_from_boilerplate(
    int port, const shcore::Dictionary_t &opts, bool raw,
    const std::string &mysqld_path) {
  if (g_test_trace_scripts) {
    if (raw)
      std::cerr << "Deploying raw sandbox " << port << " from boilerplate\n";
    else
      std::cerr << "Deploying sandbox " << port << " from boilerplate\n";
  }
  std::string mysqld_version = get_mysqld_version(mysqld_path);
  std::string bp_folder{"myboilerplate"};
  if (!mysqld_path.empty()) bp_folder.append("-" + mysqld_version);

  std::string boilerplate = shcore::path::join_path(_sandbox_dir, bp_folder);

  std::string basedir =
      shcore::path::join_path(_sandbox_dir, std::to_string(port));

  // Copy basics
  try {
    copy_boilerplate_sandbox(boilerplate, basedir);
  } catch (std::exception &e) {
    std::cerr << "Error copying boilerplate for sandbox " << port << ": "
              << e.what() << "\n";
    throw std::logic_error("During lazy deployment of sandbox " +
                           std::to_string(port) + ": " + e.what());
  }

#ifdef _WIN32
  std::string start("start.bat");
  std::string stop("stop.bat");
#else
  std::string start("start.sh");
  std::string stop("stop.sh");
#endif

  // Replaces the <SBPORT> token on the start and stop scripts for the real
  // sandbox port
  replace_file_text(shcore::path::join_path(basedir, start), "<SBPORT>",
                    std::to_string(port));
  replace_file_text(shcore::path::join_path(basedir, stop), "<SBPORT>",
                    std::to_string(port));

  replace_file_text(shcore::path::join_path(basedir, start), "<SBDIR>",
                    _sandbox_dir);
  replace_file_text(shcore::path::join_path(basedir, stop), "<SBDIR>",
                    _sandbox_dir);

  // Customize
  change_sandbox_conf(port, "port", std::to_string(port), "*");
  change_sandbox_conf(port, "server_id", std::to_string(port + 12345),
                      "mysqld");
  change_sandbox_conf(
      port, "datadir",
      shcore::str_replace(
          shcore::path::join_path(basedir, raw ? "data" : "sandboxdata"), "\\",
          "/"),
      "mysqld");
  change_sandbox_conf(
      port, "log_error",
      shcore::str_replace(
          shcore::path::join_path(basedir, raw ? "data" : "sandboxdata",
                                  "error.log"),
          "\\", "/"),
      "mysqld");
  change_sandbox_conf(
      port, "pid_file",
      shcore::str_replace(
          shcore::path::join_path(basedir, std::to_string(port) + ".pid"), "\\",
          "/"),
      "mysqld");
  change_sandbox_conf(port, "secure_file_priv",
                      shcore::path::join_path(basedir, "mysql-files"),
                      "mysqld");
  change_sandbox_conf(port, "loose_mysqlx_port", std::to_string(port * 10),
                      "mysqld");
  change_sandbox_conf(port, "report_port", std::to_string(port), "mysqld");
  change_sandbox_conf(port, "general_log", "1", "mysqld");

  if (raw) {
    try_rename(shcore::path::join_path(basedir, "sandboxdata"),
               shcore::path::join_path(basedir, "data"));

    // remove all pre-configured options related to replication
    static const char *options[] = {"transaction_write_set_extraction",
                                    "disabled_storage_engines",
                                    "binlog_checksum",
                                    "gtid_mode",
                                    "server_id",
                                    "auto_increment_offset",
                                    "log_bin",
                                    "log_slave_updates",
                                    "relay_log_info_repository",
                                    "master_info_repository",
                                    "binlog_format",
                                    "enforce_gtid_consistency",
                                    "report_port",
                                    "binlog_transaction_dependency_tracking",
                                    "slave_preserve_commit_order",
                                    "slave_parallel_type",
                                    "slave_parallel_workers",
                                    NULL};

    for (const char **opt = options; *opt; ++opt) {
      remove_from_sandbox_conf(port, *opt);
    }
  }

  if (opts && !opts->empty()) {
    // Handle server-uuid (or server_uuid) option.
    // It is a special case that cannot be set in the configuration file,
    // it has to be changed in another specific file ($DATADIR/auto.cnf).
    if (opts->has_key("server-uuid")) {
      change_sandbox_uuid(port, opts->get_string("server-uuid"));
      opts->erase("server-uuid");
    }
    if (opts->has_key("server_uuid")) {
      change_sandbox_uuid(port, opts->get_string("server_uuid"));
      opts->erase("server_uuid");
    }
    for (const auto &option : (*opts)) {
      change_sandbox_conf(port, option.first, option.second.descr(), "mysqld");
    }
  }

  if (opts && opts->has_key("keyring_file_data")) {
    auto keyring_path =
        shcore::path::basename(opts->get_string("keyring_file_data"));
    shcore::create_directory(keyring_path);
    shcore::ch_mod(keyring_path, 0570);
  }

  start_sandbox(port);

  return true;
}

//!<  @name Misc Utilities
///@{
/**
 * Compares two version strings.
 *
 * @param v1 the first version to be compared.
 * @param op the comparison operator to be used.
 * @param v2 the second version to be compared.
 *
 * This function performs the comparison operation at op.
 *
 * If op is empty, an == operation will be done
 */
#if DOXYGEN_JS
Boolean Testutils::versionCheck(String v1, String op, String v2);
#elif DOXYGEN_PY
bool Testutils::version_check(str v1, str op, str v2);
#endif
///@}
bool Testutils::version_check(const std::string &v1, const std::string &op,
                              const std::string &v2) {
  bool ret_val = true;

  mysqlshdk::utils::Version ver1, ver2;

  ver1 = mysqlshdk::utils::Version(v1);
  ver2 = mysqlshdk::utils::Version(v2);

  if (ret_val) {
    if (op.empty() || op == "==")
      ret_val = ver1 == ver2;
    else if (op == "!=")
      ret_val = ver1 != ver2;
    else if (op == ">=")
      ret_val = ver1 >= ver2;
    else if (op == "<=")
      ret_val = ver1 <= ver2;
    else if (op == ">")
      ret_val = ver1 > ver2;
    else if (op == "<")
      ret_val = ver1 < ver2;
    else
      throw std::logic_error(get_function_name("versionCheck") +
                             ": Invalid operator: " + op);
  }

  return ret_val;
}

//!<  @name Testing Utilities
///@{
/**
 * Get an exclusive lock.
 *
 * @param classic_session ClassicSession object to get the lock.
 * @param name_space string with the name space used to identify the lock.
 * @param name string with the name of the lock.
 * @param timeout time in seconds to wait to be able to get the lock before
 *                failing with an error.
 */
#if DOXYGEN_JS
Undefined Testutils::getExclusiveLock(ClassicSession classic_session,
                                      string name_space, string name,
                                      number timeout);
#elif DOXYGEN_PY
None Testutils::get_exclusive_lock(ClassicSession classic_session,
                                   str name_space, str name, int timeout);
#endif
///@}
void Testutils::get_exclusive_lock(const shcore::Value &classic_session,
                                   const std::string name_space,
                                   const std::string name,
                                   unsigned int timeout) {
  std::shared_ptr<mysqlsh::mysql::ClassicSession> session_obj =
      classic_session.as_object<mysqlsh::mysql::ClassicSession>();
  mysqlshdk::mysql::Instance instance(session_obj->get_core_session());

  if (!mysqlshdk::mysql::has_lock_service_udfs(instance)) {
    mysqlshdk::mysql::install_lock_service_udfs(&instance);
  }

  mysqlshdk::mysql::get_lock(instance, name_space, name,
                             mysqlshdk::mysql::Lock_mode::EXCLUSIVE, timeout);
}

//!<  @name Testing Utilities
///@{
/**
 * Get a shared lock.
 *
 * @param classic_session ClassicSession object to get the lock.
 * @param name_space string with the name space used to identify the lock.
 * @param name string with the name of the lock.
 * @param timeout time in seconds to wait to be able to get the lock before
 *                failing with an error.
 */
#if DOXYGEN_JS
Undefined Testutils::getSharedLock(ClassicSession classic_session,
                                   string name_space, string name,
                                   number timeout);
#elif DOXYGEN_PY
None Testutils::get_shared_lock(ClassicSession classic_session, str name_space,
                                str name, int timeout);
#endif
///@}
void Testutils::get_shared_lock(const shcore::Value &classic_session,
                                const std::string name_space,
                                const std::string name, unsigned int timeout) {
  std::shared_ptr<mysqlsh::mysql::ClassicSession> session_obj =
      classic_session.as_object<mysqlsh::mysql::ClassicSession>();
  mysqlshdk::mysql::Instance instance(session_obj->get_core_session());

  if (!mysqlshdk::mysql::has_lock_service_udfs(instance)) {
    mysqlshdk::mysql::install_lock_service_udfs(&instance);
  }

  mysqlshdk::mysql::get_lock(instance, name_space, name,
                             mysqlshdk::mysql::Lock_mode::SHARED, timeout);
}

/**
 * Release locks.
 *
 * @param classic_session ClassicSession object to release the locks.
 * @param name_space string identifying the name space to release all locks.
 */
#if DOXYGEN_JS
Undefined Testutils::releaseLocks(ClassicSession classic_session,
                                  string name_space);
#elif DOXYGEN_PY
None Testutils::release_locks(ClassicSession classic_session, str name_space);
#endif
///@}
void Testutils::release_locks(const shcore::Value &classic_session,
                              const std::string name_space) {
  std::shared_ptr<mysqlsh::mysql::ClassicSession> session_obj =
      classic_session.as_object<mysqlsh::mysql::ClassicSession>();
  mysqlshdk::mysql::Instance instance(session_obj->get_core_session());

  mysqlshdk::mysql::release_lock(instance, name_space);
}

/**  @name Sandbox Operations
 *
 * Utilities that provide a reliable handling of sandboxes.
 */
///@{
///@}

/**  @name InnoDB Cluster Utilities
 *
 * Utilities specific for InnoDB Cluster Tests.
 */
///@{
///@}

/**  @name Testing Utilities
 *
 * Utilities related to the testing framework.
 */
///@{
///@}

/**  @name Misc Utilities
 *
 * Other utilities.
 */
///@{
///@}

mysqlshdk::db::Connection_options Testutils::sandbox_connection_options(
    int port, const std::string &password) {
  mysqlshdk::db::Connection_options copts;
  copts.set_scheme("mysql");
  copts.set_host("localhost");
  copts.set_port(port);
  copts.set_user("root");
  copts.set_password(password);
  return copts;
}

namespace {
void setup_recorder_environment() {
  std::string mode;
  std::string prefix;

  // If session recording is wanted, we need to append a mysqlprovision specific
  // suffix to the output path, which also has to be different for each call
  if (mysqlshdk::db::replay::g_replay_mode !=
      mysqlshdk::db::replay::Mode::Direct) {
    if (mysqlshdk::db::replay::g_replay_mode ==
        mysqlshdk::db::replay::Mode::Record) {
      mode = "record";
    } else {
      mode = "replay";
    }

    prefix = mysqlshdk::db::replay::external_recording_path("mysqlsh");
  }

  shcore::setenv("MYSQLSH_RECORDER_MODE", mode);
  shcore::setenv("MYSQLSH_RECORDER_PREFIX", prefix);
}

std::vector<const char *> prepare_mysqlsh_cmdline(
    const std::vector<std::string> &args, const std::string &executable_path,
    const std::string &mysqlsh_path, std::string *mysqlsh_found_path) {
  std::vector<const char *> full_argv;

  setup_recorder_environment();

  if (executable_path == "mysqlshrec") {
    mysqlsh_found_path->assign(
        shcore::path::join_path(shcore::get_binary_folder(), "mysqlshrec"));
  } else if (executable_path.empty()) {
    if (mysqlshdk::db::replay::g_replay_mode !=
        mysqlshdk::db::replay::Mode::Direct) {
      // use mysqlshrec unless in direct mode
      mysqlsh_found_path->assign(
          shcore::path::join_path(shcore::get_binary_folder(), "mysqlshrec"));
    } else {
      mysqlsh_found_path->assign(mysqlsh_path);
    }
  } else {
    mysqlsh_found_path->assign(executable_path);
  }
  full_argv.push_back(mysqlsh_found_path->c_str());
  assert(strlen(full_argv.front()) > 0);

  for (const std::string &arg : args) {
    full_argv.push_back(arg.c_str());
  }
  if (g_test_trace_scripts) {
    std::cerr << shcore::str_join(full_argv, " ") << "\n";
  }
  full_argv.push_back(nullptr);
  return full_argv;
}
}  // namespace

int Testutils::call_mysqlsh(const shcore::Array_t &args,
                            const std::string &std_input,
                            const shcore::Array_t &env,
                            const std::string &executable_path) {
  return call_mysqlsh_c(
      shcore::Value(args).to_string_vector(), std_input,
      env ? shcore::Value(env).to_string_vector() : std::vector<std::string>{},
      executable_path);
}

int Testutils::call_mysqlsh_c(const std::vector<std::string> &args,
                              const std::string &std_input,
                              const std::vector<std::string> &env,
                              const std::string &executable_path) {
  char c;
  int exit_code = 1;
  std::string output;
  std::string mysqlsh_found_path;
  auto full_argv = prepare_mysqlsh_cmdline(args, executable_path, _mysqlsh_path,
                                           &mysqlsh_found_path);

  shcore::Process_launcher process(&full_argv[0]);

  if (!env.empty()) process.set_environment(env);

#ifdef _WIN32
  process.set_create_process_group();
#endif
  std::shared_ptr<mysqlsh::Command_line_shell> shell(_shell.lock());
  try {
    // Starts the process
    process.start();

    if (!std_input.empty()) {
      process.write(&std_input[0], std_input.size());
      process.finish_writing();  // Reader will see EOF
    }

    // // The password should be provided when it is expected that the Shell
    // // will prompt for it, in such case, we give it on the stdin
    // if (password) {
    //   std::string pwd(password);
    //   pwd.append("\n");
    //   process->write(pwd.c_str(), pwd.size());
    // }

    // Reads all produced output, until stdout is closed
    while (process.read(&c, 1) > 0) {
      if (g_test_trace_scripts && !shell) std::cout << c << std::flush;
      if (c == '\r') continue;
      if (c == '\n') {
        if (shell) mysqlsh::current_console()->println(output);
        output.clear();
      } else {
        output += c;
      }
    }
    if (!output.empty()) {
      if (shell) mysqlsh::current_console()->println(output);
    }
    // Wait until it finishes
    exit_code = process.wait();
  } catch (const std::system_error &e) {
    output = e.what();
    if (shell)
      mysqlsh::current_console()->println(
          ("Exception calling mysqlsh: " + output).c_str());
    exit_code = 256;  // This error code will indicate an error happened
                      // launching the process
  }
  return exit_code;
}

Testutils::Async_mysqlsh_run::Async_mysqlsh_run(
    const std::vector<std::string> &cmdline_, const std::string &stdin_,
    const std::vector<std::string> &env_, const std::string &mysqlsh_path,
    const std::string &executable_path)
    : cmdline(cmdline_),
      mysqlsh_found_path(),
      argv(prepare_mysqlsh_cmdline(cmdline, executable_path, mysqlsh_path,
                                   &mysqlsh_found_path)),
      process(&argv[0]),
      output(),
      std_in(stdin_),
      env(env_),
      task(std::async(std::launch::async,
                      &Testutils::Async_mysqlsh_run::run_mysqlsh_in_background,
                      this)) {}

int Testutils::Async_mysqlsh_run::run_mysqlsh_in_background() {
  char c;
  int exit_code = 1;
  if (!env.empty()) process.set_environment(env);

#ifdef _WIN32
  process.set_create_process_group();
#endif
  try {
    // Starts the process
    process.start();

    if (!std_in.empty()) {
      process.write(&std_in[0], std_in.size());
      process.finish_writing();  // Reader will see EOF
    }

    // Reads all produced output, until stdout is closed
    while (process.read(&c, 1) > 0) {
      if (g_test_trace_scripts) std::cout << c << std::flush;
      if (c != '\r') output += c;
    }

    // Wait until it finishes
    exit_code = process.wait();
  } catch (const std::system_error &e) {
    output += "Exception calling mysqlsh: ";
    output += e.what();
    exit_code = 256;  // This error code will indicate an error happened
                      // launching the process
  }
  return exit_code;
}

int Testutils::call_mysqlsh_async(const shcore::Array_t &args,
                                  const std::string &std_input,
                                  const shcore::Array_t &env,
                                  const std::string &executable_path) {
  return call_mysqlsh_c_async(
      shcore::Value(args).to_string_vector(), std_input,
      env ? shcore::Value(env).to_string_vector() : std::vector<std::string>{},
      executable_path);
}

int Testutils::call_mysqlsh_c_async(const std::vector<std::string> &args,
                                    const std::string &std_input,
                                    const std::vector<std::string> &env,
                                    const std::string &executable_path) {
  m_shell_runs.emplace_back(std::make_unique<Testutils::Async_mysqlsh_run>(
      args, std_input, env, _mysqlsh_path, executable_path));
  return m_shell_runs.size() - 1;
}

int Testutils::wait_mysqlsh_async(int id, int seconds) {
  if (id >= static_cast<int>(m_shell_runs.size()) || !m_shell_runs[id])
    throw std::logic_error("No process found with id: " + std::to_string(id));

  auto run = m_shell_runs[id].get();

  auto status = run->task.wait_for(std::chrono::seconds(seconds));
  std::shared_ptr<mysqlsh::Command_line_shell> shell(_shell.lock());
  if (status != std::future_status::ready) {
    auto msg = "Process '" + run->mysqlsh_found_path + " " +
               shcore::str_join(run->cmdline, " ") +
               "' did not finish in time and will be killed";
    if (shell)
      mysqlsh::current_console()->println(msg);
    else
      std::cout << msg << std::endl;
    run->process.kill();
  }

  int return_code = run->task.get();
  if (shell)
    mysqlsh::current_console()->println(run->output);
  else
    std::cout << run->output << std::endl;
  m_shell_runs[id].reset();
  return return_code;
}

void Testutils::try_rename(const std::string &source,
                           const std::string &target) {
#ifdef _WIN32
  // rename datadir
  // Sometimes windows delays releasing the file handles used on
  // the operations above, causig a Permission Denied error
  // We introduce this loop to give it some time
  int attempts = 10;
  while (attempts) {
    try {
      shcore::rename_file(source, target);
      break;
    } catch (const std::exception &err) {
      if (attempts) {
        std::cout << "Failed renaming " << source.c_str() << " to "
                  << target.c_str() << err.what() << std::endl;
        attempts--;
        shcore::sleep_ms(500);
      } else {
        throw;
      }
    }
  }
#else
  shcore::rename_file(source, target);
#endif
}

std::shared_ptr<mysqlshdk::db::ISession> Testutils::connect_to_sandbox(
    int port) {
  mysqlshdk::db::Connection_options cnx_opt;
  cnx_opt.set_user("root");
  {
    auto pass = _passwords.find(port);

    if (_passwords.end() == pass) {
      // maybe it's an X port, try again
      pass = _passwords.find(port / 10);
    }

    cnx_opt.set_password(_passwords.end() == pass ? "root" : pass->second);
  }
  cnx_opt.set_host("127.0.0.1");
  cnx_opt.set_port(port);
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(cnx_opt);
  return session;
}

std::string Testutils::get_user_config_path() {
  return shcore::get_user_config_path();
}

bool Testutils::validate_oci_config() {
  const auto oci_config_path =
      mysqlsh::current_shell_options()->get().oci_config_file;
  const auto oci_config_profile =
      mysqlsh::current_shell_options()->get().oci_profile;

  bool ret_val = false;

  mysqlshdk::config::Config_file config(mysqlshdk::config::Case::SENSITIVE,
                                        mysqlshdk::config::Escape::NO);
  try {
    config.read(oci_config_path);

    ret_val = config.has_group(oci_config_profile);
  } catch (const std::runtime_error &error) {
    // OK If the file does not exist
    if (!shcore::str_beginswith(error.what(), "Cannot open file")) throw;
  }
  return ret_val;
}

shcore::Dictionary_t Testutils::get_oci_config() {
  const auto oci_config_path =
      mysqlsh::current_shell_options()->get().oci_config_file;
  const auto oci_config_profile =
      mysqlsh::current_shell_options()->get().oci_profile;

  mysqlshdk::config::Config_file config(mysqlshdk::config::Case::SENSITIVE,
                                        mysqlshdk::config::Escape::NO);
  config.read(oci_config_path);

  shcore::Dictionary_t ret_val;

  if (config.has_group(oci_config_profile)) {
    ret_val = shcore::make_dict();

    auto set_option = [&ret_val, &config,
                       &oci_config_profile](const std::string &option) {
      if (config.has_option(oci_config_profile, option)) {
        auto value = config.get(oci_config_profile, option);

        if (value.is_null())
          (*ret_val)[option] = shcore::Value::Null();
        else
          (*ret_val)[option] = shcore::Value(*value);
      }
    };

    set_option("fingerprint");
    set_option("key_file");
    set_option("pass_phrase");
    set_option("tenancy");
    set_option("region");
    set_option("user");
  }

  return ret_val;
}

void Testutils::upload_oci_object(const std::string &bucket,
                                  const std::string &name,
                                  const std::string &path) {
  if (!validate_oci_config())
    throw std::runtime_error(
        "This function is ONLY available when the OCI configuration is in "
        "place");

  std::ifstream ifile(path, std::ifstream::binary);
  if (!ifile.good()) {
    throw std::runtime_error("Could not open file '" + path +
                             "': " + shcore::errno_to_string(errno));
  }

  mysqlshdk::oci::Oci_options options;
  options.os_bucket_name = bucket;
  options.check_option_values();
  mysqlshdk::storage::backend::oci::Object object(options, name);
  object.open(mysqlshdk::storage::Mode::WRITE);

  std::string buffer;
  buffer.reserve(1024 * 1024 * 10);
  while (ifile) {
    ifile.read(&buffer[0], 1024 * 1024 * 10);
    if (ifile) {
      object.write(&buffer[0], 1024 * 1024 * 10);
    } else {
      object.write(&buffer[0], ifile.gcount());
    }
  }

  object.close();
  ifile.close();
}

void Testutils::download_oci_object(const std::string &ns,
                                    const std::string &bucket,
                                    const std::string &name,
                                    const std::string &path) {
  if (!validate_oci_config())
    throw std::runtime_error(
        "This function is ONLY available when the OCI configuration is in "
        "place");

  std::ofstream ofile(path, std::ifstream::binary);
  if (!ofile.good()) {
    throw std::runtime_error("Could not open file '" + path +
                             "': " + shcore::errno_to_string(errno));
  }

  mysqlshdk::oci::Oci_options options;
  options.os_namespace = ns;
  options.os_bucket_name = bucket;
  options.check_option_values();
  mysqlshdk::storage::backend::oci::Object object(options, name);
  object.open(mysqlshdk::storage::Mode::READ);

  const size_t buffer_size = 10485760;
  std::string buffer;
  buffer.reserve(buffer_size);
  auto count = object.read(&buffer[0], buffer_size);
  while (count) {
    ofile.write(buffer.data(), count);
    count = object.read(&buffer[0], buffer_size);
  }

  object.close();
  ofile.close();
}

void Testutils::create_oci_object(const std::string &bucket,
                                  const std::string &name,
                                  const std::string &content) {
  if (!validate_oci_config())
    throw std::runtime_error(
        "This function is ONLY available when the OCI configuration is in "
        "place");

  mysqlshdk::oci::Oci_options options;
  options.os_bucket_name = bucket;
  options.check_option_values();
  mysqlshdk::storage::backend::oci::Object object(options, name);
  object.open(mysqlshdk::storage::Mode::WRITE);
  object.write(content.c_str(), content.size());
  object.close();
}

void Testutils::delete_oci_object(const std::string &bucket_name,
                                  const std::string &name) {
  if (!validate_oci_config())
    throw std::runtime_error(
        "This function is ONLY available when the OCI configuration is in "
        "place");

  mysqlshdk::oci::Oci_options options;
  options.os_bucket_name = bucket_name;
  options.check_option_values();
  mysqlshdk::storage::backend::oci::Bucket bucket(options);
  bucket.delete_object(name);
}

namespace {

std::unique_ptr<mysqlshdk::storage::IFile> file(const shcore::Value &location) {
  if (location.type == shcore::String) {
    return mysqlshdk::storage::make_file(location.as_string());
  } else if (location.type == shcore::Map) {
    mysqlshdk::oci::Oci_option_unpacker<
        mysqlshdk::oci::Oci_options::OBJECT_STORAGE>
        oci_option_pack;
    std::string name;
    mysqlshdk::oci::Oci_options oci_options;

    auto options = location.as_map();

    shcore::Option_unpacker unpacker;
    unpacker.set_options(options);
    unpacker.required("name", &name);
    oci_option_pack.options().unpack(&unpacker, &oci_option_pack);
    unpacker.end();

    oci_options = oci_option_pack;
    if (oci_options) {
      oci_options.check_option_values();
      return mysqlshdk::storage::make_file(name, oci_options);
    } else {
      throw std::runtime_error("map arg must be a OCI-OS object");
    }
  } else {
    throw std::runtime_error("arg must be a string or map");
  }
}

}  // namespace

void Testutils::anycopy(const shcore::Value &from, const shcore::Value &to) {
  std::unique_ptr<mysqlshdk::storage::IFile> from_file = file(from);
  std::unique_ptr<mysqlshdk::storage::IFile> to_file = file(to);

  // handle compression/decompression
  mysqlshdk::storage::Compression from_compr;
  mysqlshdk::storage::Compression to_compr;
  try {
    from_compr = mysqlshdk::storage::from_extension(
        std::get<1>(shcore::path::split_extension(from_file->filename())));
  } catch (...) {
    from_compr = mysqlshdk::storage::Compression::NONE;
  }

  try {
    to_compr = mysqlshdk::storage::from_extension(
        std::get<1>(shcore::path::split_extension(to_file->filename())));
  } catch (...) {
    to_compr = mysqlshdk::storage::Compression::NONE;
  }

  if (from_compr != to_compr) {
    from_file = mysqlshdk::storage::make_file(std::move(from_file), from_compr);
    to_file = mysqlshdk::storage::make_file(std::move(to_file), to_compr);
  }

  from_file->open(mysqlshdk::storage::Mode::READ);
  to_file->open(mysqlshdk::storage::Mode::WRITE);

  std::string buffer;

  buffer.resize(2 * 1024 * 1024);

  for (;;) {
    auto c = from_file->read(&buffer[0], buffer.size());
    if (c <= 0) break;
    to_file->write(&buffer[0], c);
  }

  from_file->close();
  to_file->close();
}

}  // namespace tests
