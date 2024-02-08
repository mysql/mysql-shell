/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_ADMINAPI_COMMON_PROVISIONING_INTERFACE_H_
#define MODULES_ADMINAPI_COMMON_PROVISIONING_INTERFACE_H_

#include <string>
#include <vector>

#include "modules/adminapi/common/group_replication_options.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "scripting/lang_base.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

constexpr int k_start_wait_timeout = 30;

namespace mysqlsh {
namespace dba {
#if DOXYGEN_CPP
/**
 * Represents an Interface to the mysqlprovision utility
 */
#endif
class ProvisioningInterface {
 public:
  ProvisioningInterface();
  ~ProvisioningInterface();

  int create_sandbox(int port, int portx, const std::string &sandbox_dir,
                     const std::string &password,
                     const shcore::Value &mycnf_options, bool start,
                     bool ignore_ssl_error, int timeout,
                     const std::string &mysqld_path,
                     shcore::Value::Array_type_ref *errors);
  int delete_sandbox(int port, const std::string &sandbox_dir,
                     bool ignore_sandbox_not_exists,
                     shcore::Value::Array_type_ref *errors);
  int kill_sandbox(int port, const std::string &sandbox_dir,
                   shcore::Value::Array_type_ref *errors);
  int stop_sandbox(int port, const std::string &sandbox_dir,
                   const std::string &password,
                   shcore::Value::Array_type_ref *errors);
  int start_sandbox(int port, const std::string &sandbox_dir,
                    shcore::Value::Array_type_ref *errors,
                    int timeout = k_start_wait_timeout);

  void set_verbose(int verbose) { _verbose = verbose; }
  int get_verbose() const { return _verbose; }

 private:
  int _verbose = 0;

  int execute_mysqlprovision(const std::string &cmd,
                             const shcore::Argument_list &args,
                             const shcore::Argument_map &kwargs,
                             shcore::Value::Array_type_ref *errors,
                             int verbose);
  int exec_sandbox_op(const std::string &op, int port, int portx,
                      const std::string &sandbox_dir,
                      const shcore::Argument_map &extra_kwargs,
                      shcore::Value::Array_type_ref *errors);
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_PROVISIONING_INTERFACE_H_
