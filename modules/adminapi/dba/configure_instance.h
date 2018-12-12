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
#include <vector>

#include "modules/adminapi/common/provisioning_interface.h"
#include "modules/command_interface.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
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
      mysqlshdk::utils::nullable<bool> restart);
  ~Configure_instance();

  void prepare() override;
  shcore::Value execute() override;
  void rollback() override;
  void finish() override;

 protected:
  void ensure_configuration_change_possible(bool needs_mycnf_change);

  /**
   * Prepare the internal mysqlshdk::config::Config object to use.
   *
   * Initialize the mysqlshdk::config::Config object adding the proper
   * configuration handlers based on the target instance settings (e.g., server
   * version) and operation input parameters.
   */
  void prepare_config_object();

  /**
   * This method validates the mycnfPath parameter.
   * If the mycnfPath parameter is empty and the instance is a sandbox,
   * it automatically fills it.
   * It issues an error if the file provided for mycnfPath doesn't exist.
   * @throws std::runtime_error if the file provided as mycnfPath doesn't exist.
   */
  void validate_config_path();

  void ensure_instance_address_usable();
  bool check_config_path_for_update();
  void check_create_admin_user();
  void create_admin_user();
  bool check_configuration_updates(bool *restart, bool *dynamic_sysvar_change,
                                   bool *config_file_change);

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

  mysqlshdk::utils::nullable<bool> m_can_set_persist;
  bool m_can_restart = false;

  bool m_needs_configuration_update = false;  //< config changes is general
  bool m_needs_update_mycnf = false;  //< changes that have to be done in my.cnf
  bool m_needs_restart = false;       //< server restart needed
  bool m_is_sandbox = false;          // true if the instance is a sandbox
  bool m_is_cnf_from_sandbox = false;  // true if mycnfpath was originally empty
                                       // but was automatically filled because
                                       // the instance is a sandbox.

  // Configuration object (to read and set instance configurations).
  std::unique_ptr<mysqlshdk::config::Config> m_cfg;

  // List of invalid (incompatible) configurations to update.
  std::vector<mysqlshdk::gr::Invalid_config> m_invalid_cfgs;

  // Target instance to configure.
  mysqlshdk::mysql::IInstance *m_target_instance;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_CONFIGURE_INSTANCE_H_
