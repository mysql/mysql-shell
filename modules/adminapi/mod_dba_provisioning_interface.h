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

#ifndef MODULES_ADMINAPI_MOD_DBA_PROVISIONING_INTERFACE_H_
#define MODULES_ADMINAPI_MOD_DBA_PROVISIONING_INTERFACE_H_

#include <string>
#include <vector>

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/shell_core_options.h"
#include "shellcore/lang_base.h"

namespace mysh {
namespace dba {
#if DOXYGEN_CPP
/**
* Represents an Interface to the mysqlprovision utility
*/
#endif
class ProvisioningInterface {
public:
  ProvisioningInterface(shcore::Interpreter_delegate* deleg);
  ~ProvisioningInterface();

  int check(const std::string &user, const std::string &host, int port, const std::string &password,
            std::string &errors, bool verbose);

  int create_sandbox(int port, int portx, const std::string &sandbox_dir,
                     const std::string &password,
                     const shcore::Value &mycnf_options,
                     std::string &errors);
  int delete_sandbox(int port, const std::string &sandbox_dir,
                     std::string &errors);
  int kill_sandbox(int port, const std::string &sandbox_dir,
                   std::string &errors);
  int stop_sandbox(int port, const std::string &sandbox_dir,
                   std::string &errors);
  int start_sandbox(int port, const std::string &sandbox_dir,
                   std::string &errors);
  int start_replicaset(const std::string &instance_url, const std::string &repl_user,
                 const std::string &super_user_password, const std::string &repl_user_password,
                 bool multi_master,
                 std::string &errors);
  int join_replicaset(const std::string &instance_url, const std::string &repl_user,
                 const std::string &peer_instance_url, const std::string &super_user_password,
                 const std::string &repl_user_password,
                 bool multi_master,
                 std::string &errors);

  int leave_replicaset(const std::string &instance_url, const std::string &super_user_password,
                       std::string &errors);

  void set_verbose(bool verbose) { _verbose = verbose; }
  bool get_verbose() { return _verbose; }

private:
  bool _verbose;
  shcore::Interpreter_delegate *_delegate;
  std::string _local_mysqlprovision_path;

  int execute_mysqlprovision(const std::string &cmd, const std::vector<const char *> &args,
                const std::vector<std::string> &passwords,
                std::string &errors, bool verbose);
  int exec_sandbox_op(const std::string &op, int port, int portx, const std::string &sandbox_dir,
                     const std::string &password,
                     const std::vector<std::string> &extra_args,
                     std::string &errors);
};
}  // namespace mysqlx
}  // namespace mysh

#endif  // MODULES_ADMINAPI_MOD_DBA_PROVISIONING_INTERFACE_H_
