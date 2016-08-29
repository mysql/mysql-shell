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

namespace mysh {
namespace mysqlx {
/**
* Represents an Interface to the mysqlprovision utility
*/
class ProvisioningInterface {
 public:
  ProvisioningInterface();
  ~ProvisioningInterface();

  int check(const std::string &user, const std::string &host, int port, const std::string &password,
            std::string &errors, bool verbose);

  int deploy_sandbox(int port, int portx, const std::string &sandbox_dir,
                     const std::string &password, std::string &errors, bool verbose);

  //int stop_sandbox(int port, int portx, const std::string &sandbox_dir,
  //                 const std::string &password, std::string &errors, bool verbose);

  int delete_sandbox(int port, int portx, const std::string &sandbox_dir,
                     std::string &errors, bool verbose);

  int kill_sandbox(int port, int portx, const std::string &sandbox_dir,
                   std::string &errors, bool verbose);

  int start_replicaset(const std::string &instance_url, const std::string &repl_user,
                 const std::string &super_user_password, const std::string &repl_user_password,
                 std::string &errors, bool verbose);

  int join_replicaset(const std::string &instance_url, const std::string &repl_user,
                 const std::string &peer_instance_url, const std::string &super_user_password,
                 const std::string &repl_user_password, std::string &errors, bool verbose);

  int leave_replicaset(const std::string &instance_url, const std::string &super_user_password,
                       std::string &errors, bool verbose);

 private:
  std::string _local_mysqlprovision_path;

  int executeMp(std::string cmd, std::vector<const char *> args, const std::vector<std::string> &passwords,
                std::string &errors, bool verbose);

  int exec_sandbox_op(std::string op, int port, int portx, const std::string &sandbox_dir,
                     const std::string &password, std::string &errors, bool verbose);
};
}  // namespace mysqlx
}  // namespace mysh

#endif  // MODULES_ADMINAPI_MOD_DBA_PROVISIONING_INTERFACE_H_
