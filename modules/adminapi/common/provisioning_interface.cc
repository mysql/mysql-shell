/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <cstring>
#include <string>
#include <system_error>
#include <vector>

#include "modules/adminapi/common/provisioning_interface.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"

extern const char *g_mysqlsh_path;

namespace mysqlsh {
namespace dba {

using mysqlshdk::db::uri::formats::user_transport;

namespace {
void setup_recorder_environment(const std::string &cmd) {
  static char mode[512];
  static char prefix[512];

  snprintf(mode, sizeof(mode), "MYSQLSH_RECORDER_MODE=");
  snprintf(prefix, sizeof(prefix), "MYSQLSH_RECORDER_PREFIX=");

  // If session recording is wanted, we need to append a mysqlprovision specific
  // suffix to the output path, which also has to be different for each call
  if (mysqlshdk::db::replay::g_replay_mode !=
      mysqlshdk::db::replay::Mode::Direct) {
    if (cmd != "sandbox") {  // don't record sandbox ops
      if (mysqlshdk::db::replay::g_replay_mode ==
          mysqlshdk::db::replay::Mode::Record) {
        snprintf(mode, sizeof(mode), "MYSQLSH_RECORDER_MODE=record");
      } else {
        snprintf(mode, sizeof(mode), "MYSQLSH_RECORDER_MODE=replay");
      }
      snprintf(prefix, sizeof(prefix), "MYSQLSH_RECORDER_PREFIX=%s_%s",
               mysqlshdk::db::replay::external_recording_path("mp").c_str(),
               cmd.c_str());
    }
  }
  putenv(mode);
  putenv(prefix);
}

shcore::Value value_from_argmap(const shcore::Argument_map &argmap) {
  shcore::Value map(shcore::Value::new_map());
  *map.as_map() = argmap.as_map();
  return map;
}

}  // namespace

ProvisioningInterface::ProvisioningInterface(const std::string &provision_path)
    : _verbose(0), _local_mysqlprovision_path(provision_path) {}

ProvisioningInterface::~ProvisioningInterface() {}

int ProvisioningInterface::execute_mysqlprovision(
    const std::string &cmd, const shcore::Argument_list &args,
    const shcore::Argument_map &kwargs, shcore::Value::Array_type_ref *errors,
    int verbose) {
  std::vector<const char *> args_script;
  std::string buf;
  char c;
  int exit_code = 0;
  std::string full_output;

  // suppress ^C propagation, mp should handle ^C itself and signal us about it
  shcore::Interrupt_handler intr([]() {
    // don't propagate up the ^C
    return false;
  });

  std::string log_level = "--log-level=";
  if (mysqlsh::current_shell_options()->get().log_to_stderr)
    log_level.append("@");
  log_level.append(std::to_string(
      static_cast<int>(shcore::Logger::singleton()->get_log_level())));

  args_script.push_back(g_mysqlsh_path);
  args_script.push_back(log_level.c_str());
  args_script.push_back("--py");
  args_script.push_back("-f");

  args_script.push_back(_local_mysqlprovision_path.c_str());
  args_script.push_back(cmd.c_str());
  args_script.push_back(NULL);

  setup_recorder_environment(cmd);

  // Wrap arguments to be passed to mysqlprovision
  shcore::Value wrapped_args(shcore::Value::new_array());
  shcore::Argument_map kwargs_(kwargs);
  kwargs_["verbose"] = shcore::Value(verbose);
  wrapped_args.as_array()->push_back(value_from_argmap(kwargs_));
  // Use a different wrapped args list with hidden passwords to show on the log
  shcore::Value logged_wrapped_args(shcore::Value::new_array());
  shcore::Argument_map logged_kwargs(kwargs_);
  if (logged_kwargs.has_key("passwd"))
    logged_kwargs["passwd"] = shcore::Value("****");
  if (logged_kwargs.has_key("rep_user_passwd"))
    logged_kwargs["rep_user_passwd"] = shcore::Value("****");
  if (logged_kwargs.has_key("server")) {
    shcore::Argument_map logged_server_opt(*logged_kwargs["server"].as_map());
    if (logged_server_opt.has_key("passwd"))
      logged_server_opt["passwd"] = shcore::Value("****");
    if (logged_server_opt.has_key("password"))
      logged_server_opt["password"] = shcore::Value("****");
    logged_kwargs["server"] = value_from_argmap(logged_server_opt);
  }
  logged_wrapped_args.as_array()->push_back(value_from_argmap(logged_kwargs));
  for (size_t i = 0; i < args.size(); i++) {
    wrapped_args.as_array()->push_back(args[i]);
    if (args[i].type == shcore::Map) {
      shcore::Argument_map logged_args(*args[i].as_map());
      if (logged_args.has_key("passwd"))
        logged_args["passwd"] = shcore::Value("****");
      if (logged_args.has_key("password"))
        logged_args["password"] = shcore::Value("****");
      logged_wrapped_args.as_array()->push_back(value_from_argmap(logged_args));
    } else {
      logged_wrapped_args.as_array()->push_back(args[i]);
    }
  }
  // Create JSON string with wrapped arguments
  std::string json = wrapped_args.json();
  json += "\n.\n";

  // Create JSON string with wrapped arguments to show in log (no passwords)
  std::string logged_json = logged_wrapped_args.json();
#ifndef NDEBUG
#ifdef _WIN32
  logged_json = "(echo(" + logged_json + "^&echo(.^&echo.)";
#else
  logged_json = "printf '" + logged_json + "\\n.\\n'";
#endif
#else
  logged_json = "'" + logged_json + "\\n.\\n'";
#endif

  std::string message =
      "DBA: mysqlprovision: Executing " + logged_json + " | " +
      shcore::str_join(&args_script[0], &args_script[args_script.size() - 1],
                       " ");
  log_info("%s", message.c_str());

#ifndef NDEBUG
  if (getenv("TEST_DEBUG") && strcmp(getenv("TEST_DEBUG"), "2") >= 0) {
    std::cerr << message << "\n" << value_from_argmap(kwargs).repr() << "\n";
    for (const auto &arg : args) std::cerr << arg.repr() << "\n";
  }
#endif
  auto console = mysqlsh::current_console();

  if (verbose > 1) {
    console->println(message);
  }

  if (verbose) {
    std::string title = " MySQL Provision Output ";
    std::string half_header((78 - title.size()) / 2, '=');
    std::string header = half_header + title + half_header;
    console->println(header);
  }

  std::string stage_action;

  shcore::Process_launcher p(&args_script[0]);
  try {
    stage_action = "starting";
    p.start();

    {
      stage_action = "executing";
      p.write(json.c_str(), json.size());
    }

    stage_action = "reading from";

    bool last_closed = false;
    bool json_started = false;
    while (p.read(&c, 1) > 0) {
      // Ignores the initial output (most likely prompts)
      // Until the first { is found, indicating the start of JSON data
      if (!json_started) {
        if (c == '{') {
          json_started = true;

          // Prints any initial data
          if (!buf.empty() && verbose) console->print(buf);

          buf.clear();
        } else {
          buf += c;
          continue;
        }
      }

      if (c == '\n') {
        // TODO(paulo): We may need to also filter other messages about
        //              password retrieval

        if (last_closed) {
          shcore::Value raw_data;
          try {
            raw_data = shcore::Value::parse(buf);
          } catch (shcore::Exception &e) {
            std::string error = e.what();
            error += ": ";
            error += buf;

            // Prints the bad formatted buffer, instead of trowing an exception
            // and aborting This is because despite the problam parsing the MP
            // output The work may have been completed there.
            console->print(buf);
            // throw shcore::Exception::parser_error(error);

            log_error("DBA: mysqlprovision: %s", error.c_str());
          }

          if (raw_data && raw_data.type == shcore::Map) {
            auto data = raw_data.as_map();

            std::string type = data->get_string("type");
            std::string info;

            if (type == "WARNING" || type == "ERROR") {
              if (!(*errors)) (*errors).reset(new shcore::Value::Array_type());

              (*errors)->push_back(raw_data);
              info = type + ": ";
            } else if (type == "DEBUG") {
              info = type + ": ";
            }

            info += data->get_string("msg") + "\n";

            // Password prompts are ignored
            if (verbose &&
                info.find("Enter the password for") == std::string::npos) {
              console->print(info);
            }
          }

          log_debug("DBA: mysqlprovision: %s", buf.c_str());

          full_output.append(buf);
          buf = "";
        } else {
          buf += c;
        }
      } else if (c == '\r') {
        buf += c;
      } else {
        buf += c;

        last_closed = c == '}';
      }
    }

    if (!buf.empty()) {
      if (verbose) console->print(buf);

      log_debug("DBA: mysqlprovision: %s", buf.c_str());

      full_output.append(buf);
    }
    stage_action = "terminating";
  } catch (const std::system_error &e) {
    log_warning("DBA: %s while %s mysqlprovision", e.what(),
                stage_action.c_str());
  }

  exit_code = p.wait();

  if (verbose) {
    std::string footer(78, '=');
    console->println(footer);
  }

#ifndef NDEBUG
  if ((getenv("TEST_DEBUG") && strcmp(getenv("TEST_DEBUG"), "2") >= 0)) {
    std::cerr << "mysqlprovision exited with code " << exit_code << ":\n"
              << "\t"
              << shcore::str_join(shcore::str_split(full_output, "\n"), "\n\t")
              << "\n";
  }
#endif
  /*
   * process launcher returns 128 if an ENOENT happened.
   */
  if (exit_code == 128) {
    throw shcore::Exception::runtime_error(
        "mysqlprovision not found. Please verify that mysqlsh is installed "
        "correctly.");
    /*
     * mysqlprovision returns 1 as exit-code for internal behaviour errors.
     * The logged message starts with "ERROR: "
     */
  } else if (exit_code == 1) {
    // Print full output if it wasn't already printed before because of verbose
    log_error("DBA: mysqlprovision exited with error code (%s) : %s ",
              std::to_string(exit_code).c_str(), full_output.c_str());

    /*
     * mysqlprovision returns 2 as exit-code for parameters parsing errors
     * The logged message starts with "mysqlprovision: error: "
     */
  } else if (exit_code == 2) {
    log_error("DBA: mysqlprovision exited with error code (%s) : %s ",
              std::to_string(exit_code).c_str(), full_output.c_str());

    // This error implies a wrong integratio nbetween the chell and MP
    std::string log_path = shcore::get_user_config_path();
    log_path += "mysqlsh.log";

    throw shcore::Exception::runtime_error(
        "Error calling mysqlprovision. For more details look at the log at: " +
        log_path);
  } else {
    log_info("DBA: mysqlprovision: Command returned exit code %i", exit_code);
  }
  return exit_code;
}

int ProvisioningInterface::exec_sandbox_op(
    const std::string &op, int port, int portx, const std::string &sandbox_dir,
    const shcore::Argument_map &extra_kwargs,
    shcore::Value::Array_type_ref *errors) {
  shcore::Argument_map kwargs(extra_kwargs);

  kwargs["sandbox_cmd"] = shcore::Value(op);
  kwargs["port"] = shcore::Value(std::to_string(port));
  if (portx != 0) {
    kwargs["mysqlx_port"] = shcore::Value(std::to_string(portx));
  }

  if (!sandbox_dir.empty()) {
    kwargs["sandbox_base_dir"] = shcore::Value(sandbox_dir);
  } else {
    std::string dir = mysqlsh::current_shell_options()->get().sandbox_directory;

    try {
      shcore::ensure_dir_exists(dir);

      kwargs["sandbox_base_dir"] = shcore::Value(dir);
    } catch (std::runtime_error &error) {
      log_warning("DBA: Unable to create default sandbox directory at %s.",
                  dir.c_str());
    }
  }

  return execute_mysqlprovision("sandbox", shcore::Argument_list(), kwargs,
                                errors, _verbose);
}

int ProvisioningInterface::create_sandbox(
    int port, int portx, const std::string &sandbox_dir,
    const std::string &password, const shcore::Value &mycnf_options, bool start,
    bool ignore_ssl_error, int timeout, shcore::Value::Array_type_ref *errors) {
  shcore::Argument_map kwargs;
  if (mycnf_options) {
    kwargs["opt"] = mycnf_options;
  }

  if (ignore_ssl_error) kwargs["ignore_ssl_error"] = shcore::Value::True();

  if (start) kwargs["start"] = shcore::Value::True();

  if (!password.empty()) kwargs["passwd"] = shcore::Value(password);

  if (timeout > 0) kwargs["timeout"] = shcore::Value(timeout);

  return exec_sandbox_op("create", port, portx, sandbox_dir, kwargs, errors);
}

int ProvisioningInterface::delete_sandbox(
    int port, const std::string &sandbox_dir,
    shcore::Value::Array_type_ref *errors) {
  return exec_sandbox_op("delete", port, 0, sandbox_dir, shcore::Argument_map(),
                         errors);
}

int ProvisioningInterface::kill_sandbox(int port,
                                        const std::string &sandbox_dir,
                                        shcore::Value::Array_type_ref *errors) {
  return exec_sandbox_op("kill", port, 0, sandbox_dir, shcore::Argument_map(),
                         errors);
}

int ProvisioningInterface::stop_sandbox(int port,
                                        const std::string &sandbox_dir,
                                        const std::string &password,
                                        shcore::Value::Array_type_ref *errors) {
  shcore::Argument_map kwargs;
  if (!password.empty()) kwargs["passwd"] = shcore::Value(password);
  return exec_sandbox_op("stop", port, 0, sandbox_dir, kwargs, errors);
}

int ProvisioningInterface::start_sandbox(
    int port, const std::string &sandbox_dir,
    shcore::Value::Array_type_ref *errors) {
  return exec_sandbox_op("start", port, 0, sandbox_dir, shcore::Argument_map(),
                         errors);
}

}  // namespace dba
}  // namespace mysqlsh
