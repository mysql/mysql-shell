/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <utility>
#include <string>
#include <vector>

#include "modules/interactive_object_wrapper.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "mysqlshdk/include/shellcore/console.h"

namespace shcore {
class Global_dba : public Interactive_object_wrapper {
 public:
  explicit Global_dba(
      Shell_core& shell_core,
      std::shared_ptr<mysqlsh::IConsole> console_handler) :
    Interactive_object_wrapper("dba", shell_core, console_handler),
    _delegate(console_handler) { init(); }

  void init();
  // virtual void resolve() const;

  // create and start
  shcore::Value deploy_sandbox_instance(const shcore::Argument_list &args,
                                        const std::string& fname);
  shcore::Value stop_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value delete_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value kill_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value start_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value reboot_cluster_from_complete_outage(
        const shcore::Argument_list &args);

  shcore::Value create_cluster(const shcore::Argument_list &args);
  shcore::Value get_cluster(const shcore::Argument_list &args);
  shcore::Value drop_metadata_schema(const shcore::Argument_list &args);
  shcore::Value check_instance_configuration(const shcore::Argument_list &args);
  shcore::Value configure_local_instance(const shcore::Argument_list &args);

 private:
  std::shared_ptr<mysqlsh::IConsole> _delegate;

 private:
  mysqlsh::dba::Cluster_check_info check_preconditions(
        std::shared_ptr<mysqlshdk::db::ISession> group_session,
        const std::string& function_name) const;
  std::vector<std::pair<std::string, std::string>>
      get_replicaset_instances_status(
            std::shared_ptr<mysqlsh::dba::Cluster> cluster,
            const shcore::Value::Map_type_ref &options) const;
  void validate_instances_status_reboot_cluster(
        std::shared_ptr<mysqlsh::dba::Cluster> cluster,
        std::shared_ptr<mysqlshdk::db::ISession> member_session,
        shcore::Value::Map_type_ref options) const;
  shcore::Argument_list check_instance_op_params(
        const shcore::Argument_list &args, const std::string& function_name);
  shcore::Value perform_instance_operation(
        const shcore::Argument_list &args, const std::string &fname,
        const std::string& progressive, const std::string& past);
  void dump_table(const std::vector<std::string> &column_names,
                  const std::vector<std::string> &column_labels,
                  shcore::Value::Array_type_ref documents);
  void print_validation_results(const shcore::Value::Map_type_ref& result);
  bool resolve_cnf_path(
      const mysqlshdk::db::Connection_options &connection_args,
      const shcore::Value::Map_type_ref &extra_options);

  bool ensure_admin_account_usable(
      std::shared_ptr<mysqlshdk::db::ISession> session,
      const std::string &user, const std::string &host,
      std::string *out_create_account);

  std::string prompt_confirmed_password();
  int prompt_menu(const std::vector<std::string> &options, int defopt);
  bool prompt_super_read_only(
      std::shared_ptr<mysqlshdk::db::ISession> session,
      const shcore::Value::Map_type_ref &options);
};
}  // namespace shcore

#endif  // SRC_INTERACTIVE_INTERACTIVE_GLOBAL_DBA_H_
