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

#include <string>
#include <vector>
#include <system_error>

#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "modules/base_session.h"
#include "utils/utils_general.h"
#include "common/process_launcher/process_launcher.h"
#include "utils/utils_file.h"

static const char *kRequiredMySQLProvisionInterfaceVersion = "2.1";

using namespace mysqlsh;
using namespace mysqlsh::dba;
using namespace shcore;

ProvisioningInterface::ProvisioningInterface(shcore::Interpreter_delegate* deleg) :
_verbose(0), _delegate(deleg) {}

ProvisioningInterface::~ProvisioningInterface() {}

int ProvisioningInterface::execute_mysqlprovision(const std::string &cmd, const std::vector<const char *> &args,
                                     const std::vector<std::string> &passwords,
                                     shcore::Value::Array_type_ref &errors, int verbose) {
  std::vector<const char *> args_script;
  std::string buf;
  char c;
  int exit_code = 0;
  std::string full_output;

  // set _local_mysqlprovision_path if empty
  if (_local_mysqlprovision_path.empty()) {
    // 1st try from global options (programmatically set by main program)
    // 2nd check if the binary is in the same dir as ourselves
    // 3rd set it to mysqlprovision and hope that it will be in $PATH

    if ((*shcore::Shell_core_options::get()).has_key(SHCORE_GADGETS_PATH))
      _local_mysqlprovision_path = (*shcore::Shell_core_options::get())[SHCORE_GADGETS_PATH].as_string();

    if (_local_mysqlprovision_path.empty()) {
      std::string tmp(get_binary_folder());
#ifdef _WIN32
      tmp.append("\\mysqlprovision.cmd");
#else
      tmp.append("/mysqlprovision");
#endif
      if (file_exists(tmp))
        _local_mysqlprovision_path = tmp;
    }

    // If is not set, we have to assume that it's located on the PATH
    if (_local_mysqlprovision_path.empty())
      _local_mysqlprovision_path = "mysqlprovision";
  }

  args_script.push_back(_local_mysqlprovision_path.c_str());
  args_script.push_back(cmd.c_str());

  args_script.insert(args_script.end(), args.begin(), args.end());
  // API version check for mysqlprovision
  args_script.push_back("--log-format=json");
  args_script.push_back("-xV");

  args_script.push_back(kRequiredMySQLProvisionInterfaceVersion);

  if (verbose > 1)
    args_script.push_back("--verbose");

  args_script.push_back(NULL);

  std::string cmdline;
  for (auto s : args_script) {
    if (s)
      cmdline.append(s).append(" ");
  }
  std::string message = "DBA: mysqlprovision: Executing " + cmdline;
  log_info("%s", message.c_str());

  if (verbose > 1) {
    message += "\n";
    _delegate->print(_delegate->user_data, message.c_str());
  }

  if (verbose) {
    std::string title = " MySQL Provision Output ";
    std::string half_header((78 - title.size()) / 2, '=');
    std::string header = half_header + title + half_header + "\n";
    _delegate->print(_delegate->user_data, header.c_str());
  }

  std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();
  std::string stage_action;

  ngcommon::Process_launcher p(&args_script[0]);
  try {
    stage_action = "starting";
    p.start();

    if (!passwords.empty()) {
      stage_action = "executing";
      for (size_t i = 0; i < passwords.size(); i++) {
        p.write(passwords[i].c_str(), passwords[i].length());
      }
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
        // TODO: We may need to also filter other messages about
        //       password retrieval

        if (last_closed) {
          shcore::Value raw_data;
          try {
            raw_data = shcore::Value::parse(buf);
          } catch (shcore::Exception &e) {
            std::string error = e.what();
            error += ": ";
            error += buf;

            // Prints the bad formatted buffer, instead of trowing an exception and aborting
            // This is because despite the problam parsing the MP output
            // The work may have been completed there.
            _delegate->print(_delegate->user_data, buf.c_str());
            //throw shcore::Exception::parser_error(error);

            log_error("DBA: mysqlprovision: %s", error.c_str());
          }

          if (raw_data && raw_data.type == shcore::Map) {
            auto data = raw_data.as_map();

            std::string type = data->get_string("type");
            std::string info;

            if (type == "WARNING" || type == "ERROR") {
              if (!errors)
                errors.reset(new shcore::Value::Array_type());

              errors->push_back(raw_data);
              info = type + ": ";
            } else if (type == "DEBUG") {
              info = type + ": ";
            }

            info += data->get_string("msg") + "\n";

            if (verbose && info.find("Enter the password for") == std::string::npos) {
              if (format.find("json") == std::string::npos)
                _delegate->print(_delegate->user_data, info.c_str());
              else
                _delegate->print_value(_delegate->user_data, raw_data, "mysqlprovision");
            }
          }

          log_debug("DBA: mysqlprovision: %s", buf.c_str());

          full_output.append(buf);
          buf = "";
        } else
          buf += c;
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
    log_warning("DBA: %s while %s mysqlprovision", e.what(), stage_action.c_str());
  }

  exit_code = p.wait();

  if (verbose) {
    std::string footer(78, '=');
    footer.append("\n");
    _delegate->print(_delegate->user_data, footer.c_str());
  }

  /*
   * process launcher returns 128 if an ENOENT happened.
   */
  if (exit_code == 128)
    throw shcore::Exception::runtime_error("mysqlprovision not found. Please verify that mysqlsh is installed correctly.");

  /*
   * mysqlprovision returns 1 as exit-code for internal behaviour errors.
   * The logged message starts with "ERROR: "
   */
  else if (exit_code == 1) {
    // Print full output if it wasn't already printed before because of verbose
    log_error("DBA: mysqlprovision exited with error code (%s) : %s ", std::to_string(exit_code).c_str(), full_output.c_str());

    /*
     * mysqlprovision returns 2 as exit-code for parameters parsing errors
     * The logged message starts with "mysqlprovision: error: "
     */
  } else if (exit_code == 2) {
    log_error("DBA: mysqlprovision exited with error code (%s) : %s ", std::to_string(exit_code).c_str(), full_output.c_str());

    // This error implies a wrong integratio nbetween the chell and MP
    std::string log_path = shcore::get_user_config_path();
    log_path += "mysqlsh.log";

    throw shcore::Exception::runtime_error("Error calling mysqlprovision. For more details look at the log at: " + log_path);
  } else
    log_info("DBA: mysqlprovision: Command returned exit code %i", exit_code);

  return exit_code;
}


void ProvisioningInterface::set_ssl_args(const std::string &prefix,
                                         const shcore::Value::Map_type_ref &instance_ssl,
                                         std::vector<const char *> &args){
  std::string ssl_ca, ssl_cert, ssl_key;

#ifdef _WIN32
  if (instance_ssl->has_key("sslCa"))
      ssl_ca = "--" + prefix + "-ssl-ca=\"" + instance_ssl->get_string("sslCa") + "\"";
    if (instance_ssl->has_key("sslCert"))
      ssl_cert = "--" + prefix + "-ssl-cert=\"" + instance_ssl->get_string("sslCert") + "\"";
    if (instance_ssl->has_key("sslKey"))
      ssl_key = "--" + prefix + "-ssl-key=\"" + instance_ssl->get_string("sslKey") + "\"";
#else
  if (instance_ssl->has_key("sslCa"))
    ssl_ca = "--" + prefix + "-ssl-ca=" + instance_ssl->get_string("sslCa");
  if (instance_ssl->has_key("sslCert"))
    ssl_cert = "--" + prefix + "-ssl-cert=" + instance_ssl->get_string("sslCert");
  if (instance_ssl->has_key("sslKey"))
    ssl_key = "--" + prefix + "-ssl-key=" + instance_ssl->get_string("sslKey");
#endif

  if (!ssl_ca.empty())
    args.push_back(strdup(ssl_ca.c_str()));
  if (!ssl_cert.empty())
    args.push_back(strdup(ssl_cert.c_str()));
  if (!ssl_key.empty())
    args.push_back(strdup(ssl_key.c_str()));
}

int ProvisioningInterface::check(const std::string &user, const std::string &host, int port,
                                 const std::string &password,
                                 const shcore::Value::Map_type_ref &instance_ssl,
                                 const std::string &cnfpath, bool update,
                                 shcore::Value::Array_type_ref &errors) {
  std::string instance_param = "--instance=" + user + "@" + host + ":" + std::to_string(port);
  std::vector<std::string> passwords;
  std::string pwd = password;

  pwd += "\n";
  passwords.push_back(pwd);

  std::vector<const char *> args;
  args.push_back(instance_param.c_str());
  set_ssl_args("instance", instance_ssl, args);

  std::string path(cnfpath);

  if (!path.empty()) {
    args.push_back("--defaults-file");

#ifdef _WIN32
    path = "\"" + path + "\"";
#endif
    args.push_back(path.c_str());
  }

  if (update)
    args.push_back("--update");

  args.push_back("--stdin");

  return execute_mysqlprovision("check", args, passwords, errors, _verbose);
}

int ProvisioningInterface::exec_sandbox_op(const std::string &op, int port, int portx, const std::string &sandbox_dir,
                                           const std::string &password,
                                           const std::vector<std::string> &extra_args,
                                           shcore::Value::Array_type_ref &errors) {
  std::vector<std::string> sandbox_args, passwords;
  std::string arg, pwd = password;

  if (port != 0) {
    arg = "--port=" + std::to_string(port);
    sandbox_args.push_back(arg);
  }

  if (portx != 0) {
    arg = "--mysqlx-port=" + std::to_string(portx);
    sandbox_args.push_back(arg);
  }

  if (!sandbox_dir.empty()) {
    sandbox_args.push_back("--sandboxdir");

    sandbox_args.push_back(sandbox_dir);
  } else if (shcore::Shell_core_options::get()->has_key(SHCORE_SANDBOX_DIR)) {
    std::string dir = (*shcore::Shell_core_options::get())[SHCORE_SANDBOX_DIR].as_string();

    try {
      shcore::ensure_dir_exists(dir);

      sandbox_args.push_back("--sandboxdir");
      sandbox_args.push_back(dir);
    } catch (std::runtime_error &error) {
      log_warning("DBA: Unable to create default sandbox directory at %s.", dir.c_str());
    }
  }

  // Inserts the indicated password
  if (op == "create" || op == "stop") {
    pwd += "\n";
    passwords.push_back(pwd);
  }

  std::vector<const char *> args;
  args.push_back(op.c_str());
  for (size_t i = 0; i < sandbox_args.size(); i++)
    args.push_back(sandbox_args[i].c_str());
  for (size_t i = 0; i < extra_args.size(); i++)
    args.push_back(extra_args[i].c_str());
  if (!pwd.empty())
    args.push_back("--stdin");

  return execute_mysqlprovision("sandbox", args, passwords, errors, _verbose);
}

int ProvisioningInterface::create_sandbox(int port, int portx, const std::string &sandbox_dir,
                                          const std::string &password,
                                          const shcore::Value &mycnf_options,
                                          bool start,
                                          bool ignore_ssl_error,
                                          shcore::Value::Array_type_ref &errors) {
  std::vector<std::string> extra_args;
  if (mycnf_options) {
    for (auto s : *mycnf_options.as_array()) {
      extra_args.push_back("--opt=" + s.as_string());
    }
  }

  if (ignore_ssl_error)
    extra_args.push_back("--ignore-ssl-error");

  if(start)
    extra_args.push_back("--start");

  return exec_sandbox_op("create", port, portx, sandbox_dir, password,
                         extra_args, errors);
}

int ProvisioningInterface::delete_sandbox(int port, const std::string &sandbox_dir,
                                          shcore::Value::Array_type_ref &errors) {
  return exec_sandbox_op("delete", port, 0, sandbox_dir, "",
                         std::vector<std::string>(), errors);
}

int ProvisioningInterface::kill_sandbox(int port, const std::string &sandbox_dir,
                                        shcore::Value::Array_type_ref &errors) {
  return exec_sandbox_op("kill", port, 0, sandbox_dir, "",
                         std::vector<std::string>(), errors);
}

int ProvisioningInterface::stop_sandbox(int port, const std::string &sandbox_dir,
                                        const std::string &password,
                                        shcore::Value::Array_type_ref &errors) {
  return exec_sandbox_op("stop", port, 0, sandbox_dir, password,
                         std::vector<std::string>(), errors);
}

int ProvisioningInterface::start_sandbox(int port, const std::string &sandbox_dir,
                                         shcore::Value::Array_type_ref &errors) {
  return exec_sandbox_op("start", port, 0, sandbox_dir, "",
                         std::vector<std::string>(), errors);
}

int ProvisioningInterface::start_replicaset(const std::string &instance_url,
                                      const shcore::Value::Map_type_ref &instance_ssl,
                                      const std::string &repl_user,
                                      const std::string &super_user_password, const std::string &repl_user_password,
                                      bool multi_master, const std::string &ssl_mode,
                                      const std::string &ip_whitelist,
                                      const std::string &group_name,
                                      shcore::Value::Array_type_ref &errors) {
  std::vector<std::string> passwords;
  std::string instance_args, repl_user_args;
  std::string super_user_pwd = super_user_password;
  std::string repl_user_pwd = repl_user_password;
  std::string ssl_mode_opt;
  std::string ip_whitelist_opt;
  std::string group_name_opt;

  instance_args = "--instance=" + instance_url;
  repl_user_args = "--replication-user=" + repl_user;

  super_user_pwd += "\n";
  passwords.push_back(super_user_pwd);

  repl_user_pwd += "\n";
  passwords.push_back(repl_user_pwd);

  std::vector<const char *> args;
  args.push_back(instance_args.c_str());
  set_ssl_args("instance", instance_ssl, args);
  if (!repl_user.empty())
    args.push_back(repl_user_args.c_str());
  if (multi_master)
    args.push_back("--single-primary=OFF");
  if (!ssl_mode.empty()) {
    ssl_mode_opt = "--ssl-mode=" + ssl_mode;
    args.push_back(ssl_mode_opt.c_str());
  }
  if (!ip_whitelist.empty()) {
    ip_whitelist_opt = "--ip-whitelist=" + ip_whitelist;
    args.push_back(ip_whitelist_opt.c_str());
  }
  if (!group_name.empty()) {
    group_name_opt = "--group-name=" + group_name;
    args.push_back(group_name_opt.c_str());
  }

  args.push_back("--stdin");

  return execute_mysqlprovision("start-replicaset", args, passwords, errors, _verbose);
}

int ProvisioningInterface::join_replicaset(const std::string &instance_url,
                                      const shcore::Value::Map_type_ref &instance_ssl,
                                      const std::string &repl_user,
                                      const std::string &peer_instance_url,
                                      const shcore::Value::Map_type_ref &peer_instance_ssl,
                                      const std::string &super_user_password,
                                      const std::string &repl_user_password,
                                      const std::string &ssl_mode,
                                      const std::string &ip_whitelist,
                                      const std::string &gr_group_seeds,
                                      bool skip_rpl_user,
                                      shcore::Value::Array_type_ref &errors) {
  std::vector<std::string> passwords;
  std::string instance_args, peer_instance_args, repl_user_args;
  std::string super_user_pwd = super_user_password;
  std::string repl_user_pwd = repl_user_password;
  std::string ssl_mode_opt;
  std::string ip_whitelist_opt;
  std::string gr_group_seeds_opt;

  instance_args = "--instance=" + instance_url;
  repl_user_args = "--replication-user=" + repl_user;

  super_user_pwd += "\n";
  // password for instance being joined
  passwords.push_back(super_user_pwd);
  if (!repl_user.empty()) {
    // password for the replication user
    repl_user_pwd += "\n";
    passwords.push_back(repl_user_pwd);
  }
  // we need to enter the super-user password again for the peer instance
  passwords.push_back(super_user_pwd);

  peer_instance_args = "--peer-instance=" + peer_instance_url;

  std::vector<const char *> args;
  args.push_back(instance_args.c_str());
  set_ssl_args("instance", instance_ssl, args);
  if (!repl_user.empty())
    args.push_back(repl_user_args.c_str());
  args.push_back(peer_instance_args.c_str());
  set_ssl_args("peer-instance", peer_instance_ssl, args);
  if (!ssl_mode.empty()) {
    ssl_mode_opt = "--ssl-mode=" + ssl_mode;
    args.push_back(ssl_mode_opt.c_str());
  }
  if (!ip_whitelist.empty()) {
    ip_whitelist_opt = "--ip-whitelist=" + ip_whitelist;
    args.push_back(ip_whitelist_opt.c_str());
  }

  if (!gr_group_seeds.empty()) {
    gr_group_seeds_opt = "--group-seeds=" + gr_group_seeds;
    args.push_back(gr_group_seeds_opt.c_str());
  }

  if (skip_rpl_user)
    args.push_back("--skip-replication-user");

  args.push_back("--stdin");

  return execute_mysqlprovision("join-replicaset", args, passwords, errors, _verbose);
}

int ProvisioningInterface::leave_replicaset(const std::string &instance_url,
                                            const shcore::Value::Map_type_ref &instance_ssl,
                                            const std::string &super_user_password,
                                            shcore::Value::Array_type_ref &errors) {
  std::vector<std::string> passwords;
  std::string instance_args, repl_user_args;
  std::string super_user_pwd = super_user_password;

  instance_args = "--instance=" + instance_url;

  super_user_pwd += "\n";
  passwords.push_back(super_user_pwd);

  std::vector<const char *> args;
  args.push_back(instance_args.c_str());
  set_ssl_args("instance", instance_ssl, args);

  args.push_back("--stdin");

  return execute_mysqlprovision("leave-replicaset", args, passwords, errors, _verbose);
}
