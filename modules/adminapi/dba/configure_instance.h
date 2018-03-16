/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_DBA_CONFIGURE_INSTANCE_H_
#define MODULES_ADMINAPI_DBA_CONFIGURE_INSTANCE_H_

#include <map>
#include <memory>
#include <string>

#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "modules/command_interface.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "scripting/lang_base.h"

namespace mysqlsh {
namespace dba {

class Configure_instance : public Command_interface {
 public:
  Configure_instance(
      mysqlshdk::mysql::IInstance *target_instance,
      const std::string &mycnf_path, const std::string &output_mycnf_path,
      const std::string &cluster_admin,
      const mysqlshdk::utils::nullable<std::string> &cluster_admin_password,
      mysqlshdk::utils::nullable<bool> clear_read_only, const bool interactive,
      mysqlshdk::utils::nullable<bool> restart,
      std::shared_ptr<ProvisioningInterface> provisioning_interface,
      std::shared_ptr<mysqlsh::IConsole> console_handler);
  ~Configure_instance();

  void prepare() override;
  shcore::Value execute() override;
  void rollback() override;
  void finish() override;

 protected:
  void ensure_configuration_change_possible(bool needs_mycnf_change);

  void ensure_instance_address_usable();
  bool check_config_path_for_update();
  bool check_persisted_globals_load();
  void ensure_user_privileges();
  void check_create_admin_user();
  void create_admin_user();
  bool check_configuration_updates(bool *restart, bool *dynamic_sysvar_change,
                                   bool *config_file_change);
  void perform_configuration_updates(bool use_set_persist);
  void handle_mp_op_result(const shcore::Dictionary_t &result);

  bool clear_super_read_only();
  void restore_super_read_only();

 protected:
  // The following may change from user-interaction
  std::string m_mycnf_path;
  std::string m_output_mycnf_path;
  std::string m_cluster_admin;
  mysqlshdk::utils::nullable<std::string> m_cluster_admin_password;
  std::string m_current_user;
  std::string m_current_host;
  const bool m_interactive;
  bool m_local_target = false;
  mysqlshdk::utils::nullable<bool> m_clear_read_only;
  mysqlshdk::utils::nullable<bool> m_restart;

  // By default, the clusterAdmin account will be created unless
  // it's not specified on the command options or it already exists
  bool m_create_cluster_admin = true;

  bool m_can_set_persist = false;
  bool m_can_restart = false;
  bool m_can_update_mycnf = false;

  bool m_needs_configuration_update = false;  //< config changes is general
  bool m_needs_update_mycnf = false;  //< changes that have to be done in my.cnf
  bool m_needs_restart = false;       //< server restart needed

  // TODO(someone): as soon as MP is fully rewritten to C++,we won't need
  // this private member since we'll rely on the provisioning library
  mysqlshdk::mysql::IInstance *m_target_instance;
  std::shared_ptr<ProvisioningInterface> m_provisioning_interface;
  std::shared_ptr<mysqlsh::IConsole> m_console;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_CONFIGURE_INSTANCE_H_
