/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "common/process_launcher/process_launcher.h"
#include "utils/utils_file.h"

using namespace mysh;
using namespace mysh::dba;
using namespace shcore;

ProvisioningInterface::ProvisioningInterface(shcore::Interpreter_delegate* deleg) :
_delegate(deleg) {}

ProvisioningInterface::~ProvisioningInterface() {}

int ProvisioningInterface::execute_mysqlprovision(const std::string &cmd, const std::vector<const char *> &args,
                                     const std::vector<std::string> &passwords,
                                     std::string &errors, bool verbose) {
  std::vector<const char *> args_script;
  std::string buf;
  char c;
  int exit_code;
  std::string full_output;
  std::string error_output;

  // set _local_mysqlprovision_path if empty
  if (_local_mysqlprovision_path.empty()) {
    _local_mysqlprovision_path = (*shcore::Shell_core_options::get())[SHCORE_GADGETS_PATH].as_string();

    // If is not set, we have to assume that it's located on the PATH
    if (_local_mysqlprovision_path.empty())
      _local_mysqlprovision_path = "mysqlprovision";
  }

  if (_local_mysqlprovision_path.find(".py") == _local_mysqlprovision_path.size() - 3)
    args_script.push_back("python");

  args_script.push_back(_local_mysqlprovision_path.c_str());
  args_script.push_back(cmd.c_str());

  args_script.insert(args_script.end(), args.begin(), args.end());

  {
    std::string cmdline;
    for (auto s : args_script) {
      if (s)
        cmdline.append(s).append(" ");
    }
    log_info("DBA: mysqlprovision: Executing %s", cmdline.c_str());
  }

  ngcommon::Process_launcher p(args_script[0], &args_script[0]);
  p.start();

  if (!passwords.empty()) {
    try {
      for (size_t i = 0; i < passwords.size(); i++) {
        p.write(passwords[i].c_str(), passwords[i].length());
      }
    } catch (std::system_error &e) {
      log_warning("DBA: %s while executing mysqlprovision", e.what());
    }
  }
  int rc;
  try {
    while ((rc = p.read(&c, 1)) > 0) {
      buf += c;
      if (c == '\n') {
        if (verbose)
          _delegate->print(_delegate->user_data, buf.c_str());
        log_debug("DBA: mysqlprovision: %s", buf.c_str());
        if ((buf.find("ERROR") != std::string::npos) || (buf.find("mysqlprovision: error") != std::string::npos))
            error_output.append(buf);
        full_output.append(buf);
        buf = "";
      }
    }
  } catch (std::system_error &e) {
    log_warning("DBA: %s while reading from mysqlprovision", e.what());
  }
  if (!buf.empty()) {
    if (verbose)
      _delegate->print(_delegate->user_data, buf.c_str());
    log_debug("DBA: mysqlprovision: %s", buf.c_str());
    if ((buf.find("ERROR") != std::string::npos) || (buf.find("mysqlprovision: error") != std::string::npos))
      full_output.append(buf);
  }
  exit_code = p.wait();

  /*
   * process launcher returns 128 if an ENOENT happened.
   */
  if (exit_code == 128) {
    if (!verbose) {
      _delegate->print(_delegate->user_data, full_output.c_str());
    }
    throw shcore::Exception::runtime_error("Please install mysqlprovision. If already installed, set its path using the environment variable: MYSQLPROVISION");
  }

  /*
   * mysqlprovision returns 1 as exit-code for internal behaviour errors.
   * The logged message starts with "ERROR: "
   */
  else if (exit_code == 1) {
    _delegate->print_error(_delegate->user_data,
                     ("mysqlprovision exited with error code " + std::to_string(exit_code) + "\n").c_str());
    if (!verbose) {
      _delegate->print(_delegate->user_data, full_output.c_str());
    }
    std::string remove_me = "ERROR: Error executing the '" + cmd + "' command:";

    std::string::size_type i = full_output.find(remove_me);

    if (i != std::string::npos)
      full_output.erase(i, remove_me.length());

    errors = full_output;

    /*
     * mysqlprovision returns 2 as exit-code for parameters parsing errors
     * The logged message starts with "mysqlprovision: error: "
     */
  } else if (exit_code == 2) {
    std::string remove_me = "mysqlprovision: error:";

    std::string::size_type i = full_output.find(remove_me);

    if (i != std::string::npos)
      full_output.erase(i, remove_me.length());

    errors = full_output;
  }
  if (errors.empty() && exit_code != 0) {
    errors = "Error while executing mysqlprovision (return " + std::to_string(exit_code) + ")";
  }
  log_info("DBA: mysqlprovision: Command returned exit code %i", exit_code);

  return exit_code;
}

int ProvisioningInterface::check(const std::string &user, const std::string &host, int port,
                                 const std::string &password, std::string &errors, bool verbose) {
  std::string instance_param = "--instance=" + user + "@" + host + ":" + std::to_string(port);
  std::vector<std::string> passwords;
  std::string pwd = password;

  pwd += "\n";
  passwords.push_back(pwd);

  std::vector<const char *> args;
  args.push_back(instance_param.c_str());
  args.push_back("--stdin");
  args.push_back(NULL);

  return execute_mysqlprovision("check", args, passwords, errors, verbose);
}

int ProvisioningInterface::exec_sandbox_op(const std::string &op, int port, int portx, const std::string &sandbox_dir,
                                           const std::string &password,
                                           const std::vector<std::string> &extra_args,
                                           std::string &errors, bool verbose) {
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

#ifdef _WIN32
    sandbox_args.push_back("\"" + sandbox_dir + "\"");
#else
    sandbox_args.push_back(sandbox_dir);
#endif
  } else if (shcore::Shell_core_options::get()->has_key(SHCORE_SANDBOX_DIR)) {
    std::string dir = (*shcore::Shell_core_options::get())[SHCORE_SANDBOX_DIR].as_string();
    sandbox_args.push_back("--sandboxdir");
#ifdef _WIN32
    sandbox_args.push_back("\"" + dir + "\"");
#else
    sandbox_args.push_back(dir);
#endif
  }

  if (!pwd.empty()) {
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
  args.push_back(NULL);

  return execute_mysqlprovision("sandbox", args, passwords, errors, verbose);
}

int ProvisioningInterface::deploy_sandbox(int port, int portx, const std::string &sandbox_dir,
                                          const std::string &password,
                                          const shcore::Value &mycnf_options,
                                          std::string &errors, bool verbose) {
  std::vector<std::string> extra_args;
  if (mycnf_options) {
    for (auto s : *mycnf_options.as_array()) {
      extra_args.push_back("--opt=" + s.as_string());
    }
  }
  return exec_sandbox_op("start", port, portx, sandbox_dir, password,
                         extra_args, errors, verbose);
}

int ProvisioningInterface::delete_sandbox(int port, const std::string &sandbox_dir,
                                          std::string &errors, bool verbose) {
  return exec_sandbox_op("delete", port, 0, sandbox_dir, "",
                         std::vector<std::string>(), errors, verbose);
}

int ProvisioningInterface::kill_sandbox(int port, const std::string &sandbox_dir,
                                        std::string &errors, bool verbose) {
  return exec_sandbox_op("kill", port, 0, sandbox_dir, "",
                         std::vector<std::string>(), errors, verbose);
}

int ProvisioningInterface::stop_sandbox(int port, const std::string &sandbox_dir,
                                        std::string &errors, bool verbose) {
  return exec_sandbox_op("stop", port, 0, sandbox_dir, "",
                         std::vector<std::string>(), errors, verbose);
}

int ProvisioningInterface::start_replicaset(const std::string &instance_url, const std::string &repl_user,
                                      const std::string &super_user_password, const std::string &repl_user_password,
                                      bool multi_master,
                                      std::string &errors, bool verbose) {
  std::vector<std::string> passwords;
  std::string instance_args, repl_user_args;
  std::string super_user_pwd = super_user_password;
  std::string repl_user_pwd = repl_user_password;

  instance_args = "--instance=" + instance_url;
  repl_user_args = "--replication-user=" + repl_user;

  super_user_pwd += "\n";
  passwords.push_back(super_user_pwd);

  repl_user_pwd += "\n";
  passwords.push_back(repl_user_pwd);

  std::vector<const char *> args;
  args.push_back(instance_args.c_str());
  args.push_back(repl_user_args.c_str());
  if (multi_master)
    args.push_back("--single-primary=OFF");
  args.push_back("--stdin");
  args.push_back(NULL);

  return execute_mysqlprovision("start-replicaset", args, passwords, errors, verbose);
}

int ProvisioningInterface::join_replicaset(const std::string &instance_url, const std::string &repl_user,
                                      const std::string &peer_instance_url, const std::string &super_user_password,
                                      const std::string &repl_user_password,
                                      bool multi_master,
                                      std::string &errors, bool verbose) {
  std::vector<std::string> passwords;
  std::string instance_args, peer_instance_args, repl_user_args;
  std::string super_user_pwd = super_user_password;
  std::string repl_user_pwd = repl_user_password;

  instance_args = "--instance=" + instance_url;
  repl_user_args = "--replication-user=" + repl_user;

  super_user_pwd += "\n";
  // password for instance being joined
  passwords.push_back(super_user_pwd);
  // password for the replication user
  repl_user_pwd += "\n";
  passwords.push_back(repl_user_pwd);
  // we need to enter the super-user password again for the peer instance
  passwords.push_back(super_user_pwd);

  peer_instance_args = "--peer-instance=" + peer_instance_url;

  std::vector<const char *> args;
  args.push_back(instance_args.c_str());
  args.push_back(repl_user_args.c_str());
  args.push_back(peer_instance_args.c_str());

  args.push_back("--stdin");
  args.push_back(NULL);

  return execute_mysqlprovision("join-replicaset", args, passwords, errors, verbose);
}

int ProvisioningInterface::leave_replicaset(const std::string &instance_url, const std::string &super_user_password,
                                            std::string &errors, bool verbose) {
  std::vector<std::string> passwords;
  std::string instance_args, repl_user_args;
  std::string super_user_pwd = super_user_password;

  instance_args = "--instance=" + instance_url;

  super_user_pwd += "\n";
  passwords.push_back(super_user_pwd);

  std::vector<const char *> args;
  args.push_back(instance_args.c_str());

  args.push_back("--stdin");
  args.push_back(NULL);

  return execute_mysqlprovision("leave-replicaset", args, passwords, errors, verbose);
}
