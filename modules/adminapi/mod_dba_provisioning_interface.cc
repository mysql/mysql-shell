/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <cstring>
#include <string>
#include <system_error>
#include <vector>

#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"

extern const char *g_mysqlsh_argv0;


using namespace mysqlsh;
using namespace mysqlsh::dba;
using namespace shcore;
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
               mysqlshdk::db::replay::mysqlprovision_recording_path().c_str(),
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

ProvisioningInterface::ProvisioningInterface(
    shcore::Interpreter_delegate *deleg)
    : _verbose(0), _delegate(deleg) {}

ProvisioningInterface::~ProvisioningInterface() {}

int ProvisioningInterface::execute_mysqlprovision(
    const std::string &cmd,
    const shcore::Argument_list &args,
    const shcore::Argument_map &kwargs,
    shcore::Value::Array_type_ref *errors, int verbose) {
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

  // set _local_mysqlprovision_path if empty
  if (_local_mysqlprovision_path.empty()) {
    // 1st try from global options (programmatically set by main program)
    // 2nd check if the binary is in the same dir as ourselves
    // 3rd set it to mysqlprovision and hope that it will be in $PATH

    if (!mysqlsh::Base_shell::options().gadgets_path.empty())
      _local_mysqlprovision_path = mysqlsh::Base_shell::options().gadgets_path;

    if (_local_mysqlprovision_path.empty()) {
      std::string tmp(get_mysqlx_home_path());

      // If MYSQLX_HOME is unknown we try to get the binary dir
      if (tmp.empty())
        tmp = get_binary_folder();

#ifdef _WIN32
      tmp.append("\\share\\mysqlsh\\mysqlprovision.zip");
#else
      tmp.append("/share/mysqlsh/mysqlprovision.zip");
#endif
      if (file_exists(tmp))
        _local_mysqlprovision_path = tmp;
    }
    assert(!_local_mysqlprovision_path.empty());
  }

  args_script.push_back(g_mysqlsh_argv0);
  args_script.push_back("--py");
  args_script.push_back("-f");

  args_script.push_back(_local_mysqlprovision_path.c_str());
  args_script.push_back(cmd.c_str());
  args_script.push_back(NULL);

  setup_recorder_environment(cmd);

  std::string message =
      "DBA: mysqlprovision: Executing " +
      shcore::str_join(&args_script[0], &args_script[args_script.size() - 1],
                       " ");
  log_info("%s", message.c_str());

  if (verbose > 1 ||
      (getenv("TEST_DEBUG") && strcmp(getenv("TEST_DEBUG"), "2") == 0)) {
    message += "\n";
    _delegate->print(_delegate->user_data, message.c_str());
  }

  if (verbose) {
    std::string title = " MySQL Provision Output ";
    std::string half_header((78 - title.size()) / 2, '=');
    std::string header = half_header + title + half_header + "\n";
    _delegate->print(_delegate->user_data, header.c_str());
  }

  std::string format = mysqlsh::Base_shell::options().output_format;
  std::string stage_action;

  shcore::Process_launcher p(&args_script[0]);
  try {
    stage_action = "starting";
    p.start();

    shcore::Value wrapped_args(shcore::Value::new_array());
    Argument_map kwargs_(kwargs);
    kwargs_["verbose"] = shcore::Value(verbose);
    wrapped_args.as_array()->push_back(value_from_argmap(kwargs_));
    for (int i = 0; i < args.size(); i++)
      wrapped_args.as_array()->push_back(args[i]);

    {
      stage_action = "executing";
      std::string json = wrapped_args.json();
      json += "\n.\n";
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
          if (!buf.empty() && verbose)
            _delegate->print(_delegate->user_data, buf.c_str());

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
            _delegate->print(_delegate->user_data, buf.c_str());
            // throw shcore::Exception::parser_error(error);

            log_error("DBA: mysqlprovision: %s", error.c_str());
          }

          if (raw_data && raw_data.type == shcore::Map) {
            auto data = raw_data.as_map();

            std::string type = data->get_string("type");
            std::string info;

            if (type == "WARNING" || type == "ERROR") {
              if (!(*errors))
                (*errors).reset(new shcore::Value::Array_type());

              (*errors)->push_back(raw_data);
              info = type + ": ";
            } else if (type == "DEBUG") {
              info = type + ": ";
            }

            info += data->get_string("msg") + "\n";

            if (verbose &&
                info.find("Enter the password for") == std::string::npos) {
              if (format.find("json") == std::string::npos)
                _delegate->print(_delegate->user_data, info.c_str());
              else
                _delegate->print_value(_delegate->user_data, raw_data,
                                       "mysqlprovision");
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
      if (verbose)
        _delegate->print(_delegate->user_data, buf.c_str());

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
    footer.append("\n");
    _delegate->print(_delegate->user_data, footer.c_str());
  }

  if ((getenv("TEST_DEBUG") && strcmp(getenv("TEST_DEBUG"), "2") == 0)) {
    _delegate->print(
        _delegate->user_data,
        shcore::str_format("mysqlprovision exited with code %i\n", exit_code)
            .c_str());
    _delegate->print(_delegate->user_data, full_output.c_str());
  }

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

void ProvisioningInterface::set_ssl_args(
    const std::string &prefix,
    const mysqlshdk::db::Connection_options &instance,
    std::vector<const char *> *args) {
  std::string ssl_ca, ssl_cert, ssl_key;

  auto instance_ssl = instance.get_ssl_options();

  if (instance_ssl.has_ca())
    ssl_ca = "--" + prefix + "-ssl-ca=" + instance_ssl.get_ca();

  if (instance_ssl.has_cert())
    ssl_cert = "--" + prefix + "-ssl-cert=" + instance_ssl.get_cert();

  if (instance_ssl.has_key())
    ssl_key = "--" + prefix + "-ssl-key=" + instance_ssl.get_key();

  if (!ssl_ca.empty())
    args->push_back(strdup(ssl_ca.c_str()));
  if (!ssl_cert.empty())
    args->push_back(strdup(ssl_cert.c_str()));
  if (!ssl_key.empty())
    args->push_back(strdup(ssl_key.c_str()));
}

int ProvisioningInterface::check(
    const mysqlshdk::db::Connection_options &connection_options,
    const std::string &cnfpath, bool update,
    shcore::Value::Array_type_ref *errors) {
  shcore::Argument_map kwargs;

  {
    auto map = get_connection_map(connection_options);
    if (map->has_key("password"))
      (*map)["passwd"] = (*map)["password"];
    kwargs["server"] = shcore::Value(map);
  }
  if (!cnfpath.empty()) {
    kwargs["option_file"] = shcore::Value(cnfpath);
  }
  if (update) {
    kwargs["update"] = shcore::Value::True();
  }

  return execute_mysqlprovision("check", shcore::Argument_list(), kwargs,
                                errors, _verbose);
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
    std::string dir = mysqlsh::Base_shell::options().sandbox_directory;

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
    bool ignore_ssl_error, shcore::Value::Array_type_ref *errors) {
  shcore::Argument_map kwargs;
  if (mycnf_options) {
    kwargs["opt"] = mycnf_options;
  }

  if (ignore_ssl_error)
    kwargs["ignore_ssl_error"] = shcore::Value::True();

  if (start)
    kwargs["start"] = shcore::Value::True();

  if (!password.empty())
    kwargs["passwd"] = shcore::Value(password);

  return exec_sandbox_op("create", port, portx, sandbox_dir, kwargs,
                         errors);
}

int ProvisioningInterface::delete_sandbox(
    int port, const std::string &sandbox_dir,
    shcore::Value::Array_type_ref *errors) {
  return exec_sandbox_op("delete", port, 0, sandbox_dir,
                         shcore::Argument_map(), errors);
}

int ProvisioningInterface::kill_sandbox(int port,
                                        const std::string &sandbox_dir,
                                        shcore::Value::Array_type_ref *errors) {
  return exec_sandbox_op("kill", port, 0, sandbox_dir,
                         shcore::Argument_map(), errors);
}

int ProvisioningInterface::stop_sandbox(int port,
                                        const std::string &sandbox_dir,
                                        const std::string &password,
                                        shcore::Value::Array_type_ref *errors) {
  shcore::Argument_map kwargs;
  if (!password.empty())
    kwargs["passwd"] = shcore::Value(password);
  return exec_sandbox_op("stop", port, 0, sandbox_dir, kwargs, errors);
}

int ProvisioningInterface::start_sandbox(
    int port, const std::string &sandbox_dir,
    shcore::Value::Array_type_ref *errors) {
  return exec_sandbox_op("start", port, 0, sandbox_dir,
                         shcore::Argument_map(), errors);
}

int ProvisioningInterface::start_replicaset(
    const mysqlshdk::db::Connection_options &instance,
    const std::string &repl_user, const std::string &super_user_password,
    const std::string &repl_user_password, bool multi_master,
    const std::string &ssl_mode, const std::string &ip_whitelist,
    const std::string &group_name,
    const std::string &gr_local_address,
    const std::string &gr_group_seeds,
    shcore::Value::Array_type_ref *errors) {
  shcore::Argument_map kwargs;
  shcore::Argument_list args;

  {
    auto map = get_connection_map(instance);
    (*map)["passwd"] = shcore::Value(super_user_password);
    args.push_back(shcore::Value(map));
  }

  kwargs["rep_user_passwd"] = shcore::Value(repl_user_password);
  kwargs["replication_user"] = shcore::Value(repl_user);

  if (multi_master) {
    kwargs["single_primary"] = shcore::Value("OFF");
  }
  if (!ssl_mode.empty()) {
    kwargs["ssl_mode"] = shcore::Value(ssl_mode);
  }
  if (!ip_whitelist.empty()) {
    kwargs["ip_whitelist"] = shcore::Value(ip_whitelist);
  }
  if (!group_name.empty()) {
    kwargs["group_name"] = shcore::Value(group_name);
  }
  if (!gr_local_address.empty()) {
    kwargs["gr_address"] = shcore::Value(gr_local_address);
  }
  if (!gr_group_seeds.empty()) {
    kwargs["group_seeds"] = shcore::Value(gr_group_seeds);
  }

  return execute_mysqlprovision("start-replicaset", args, kwargs, errors,
                                _verbose);
}

int ProvisioningInterface::join_replicaset(
    const mysqlshdk::db::Connection_options &instance,
    const mysqlshdk::db::Connection_options &peer, const std::string &repl_user,
    const std::string &super_user_password,
    const std::string &repl_user_password, const std::string &ssl_mode,
    const std::string &ip_whitelist,
    const std::string &gr_local_address,
    const std::string &gr_group_seeds,
    bool skip_rpl_user, shcore::Value::Array_type_ref *errors) {
  shcore::Argument_map kwargs;
  shcore::Argument_list args;

  {
    auto map = get_connection_map(instance);
    (*map)["passwd"] = shcore::Value(super_user_password);
    args.push_back(shcore::Value(map));
  }
  {
    auto map = get_connection_map(peer);
    (*map)["passwd"] = shcore::Value(super_user_password);
    args.push_back(shcore::Value(map));
  }

  if (!repl_user.empty()) {
    kwargs["rep_user_passwd"] = shcore::Value(repl_user_password);
    kwargs["replication_user"] = shcore::Value(repl_user);
  }
  if (!ssl_mode.empty()) {
    kwargs["ssl_mode"] = shcore::Value(ssl_mode);
  }
  if (!ip_whitelist.empty()) {
    kwargs["ip_whitelist"] = shcore::Value(ip_whitelist);
  }

  if (!gr_local_address.empty()) {
    kwargs["gr_address"] = shcore::Value(gr_local_address);
  }
  if (!gr_group_seeds.empty()) {
    kwargs["group_seeds"] = shcore::Value(gr_group_seeds);
  }
  if (skip_rpl_user) {
    kwargs["skip_rpl_user"] = shcore::Value::True();
  }

  return execute_mysqlprovision("join-replicaset", args, kwargs, errors,
                                _verbose);
}

int ProvisioningInterface::leave_replicaset(
    const mysqlshdk::db::Connection_options &connection_options,
    shcore::Value::Array_type_ref *errors) {
  shcore::Argument_map kwargs;
  shcore::Argument_list args;

  {
    auto map = get_connection_map(connection_options);
    if (map->has_key("password"))
      (*map)["passwd"] = (*map)["password"];
    args.push_back(shcore::Value(map));
  }

  return execute_mysqlprovision("leave-replicaset", args, kwargs, errors,
                                _verbose);
}
