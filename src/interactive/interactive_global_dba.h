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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef SRC_INTERACTIVE_INTERACTIVE_GLOBAL_DBA_H_
#define SRC_INTERACTIVE_INTERACTIVE_GLOBAL_DBA_H_

#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/interactive_object_wrapper.h"

namespace shcore {
class Global_dba : public Interactive_object_wrapper {
 public:
  explicit Global_dba(Shell_core &shell_core)
      : Interactive_object_wrapper("dba", shell_core) {
    init();
  }

  void init();

  // create and start
  shcore::Value deploy_sandbox_instance(const shcore::Argument_list &args,
                                        const std::string &fname);
  shcore::Value stop_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value delete_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value kill_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value start_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value reboot_cluster_from_complete_outage(
      const shcore::Argument_list &args);

  shcore::Value create_cluster(const shcore::Argument_list &args);
  shcore::Value get_cluster(const shcore::Argument_list &args);
  shcore::Value drop_metadata_schema(const shcore::Argument_list &args);

 private:
  mysqlsh::dba::Cluster_check_info check_preconditions(
      std::shared_ptr<mysqlshdk::db::ISession> group_session,
      const std::string &function_name) const;
  std::vector<std::pair<std::string, std::string>>
  get_replicaset_instances_status(
      std::shared_ptr<mysqlsh::dba::Cluster> cluster,
      const shcore::Value::Map_type_ref &options) const;
  void validate_instances_status_reboot_cluster(
      std::shared_ptr<mysqlsh::dba::Cluster> cluster,
      std::shared_ptr<mysqlshdk::db::ISession> member_session,
      shcore::Value::Map_type_ref options) const;
  shcore::Argument_list check_instance_op_params(
      const shcore::Argument_list &args, const std::string &function_name);
  shcore::Value perform_instance_operation(const shcore::Argument_list &args,
                                           const std::string &fname,
                                           const std::string &progressive,
                                           const std::string &past);
  bool resolve_cnf_path(
      const mysqlshdk::db::Connection_options &connection_args,
      const shcore::Value::Map_type_ref &extra_options);

  std::string prompt_confirmed_password();
  int prompt_menu(const std::vector<std::string> &options, int defopt);
};
}  // namespace shcore

#endif  // SRC_INTERACTIVE_INTERACTIVE_GLOBAL_DBA_H_
