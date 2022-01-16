/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/provisioning_interface.h"
#include "modules/adminapi/dba/api_options.h"
#include "modules/command_interface.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/repl_config.h"
#include "scripting/lang_base.h"

namespace mysqlsh {
namespace dba {

class Configure_instance : public Command_interface {
 public:
  Configure_instance(
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      const Configure_instance_options &options, TargetType::Type instance_type,
      Cluster_type purpose);

  virtual ~Configure_instance() = default;

  void prepare() override;
  shcore::Value execute() override;

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
  void validate_applier_worker_threads();

  void ensure_instance_address_usable();
  bool check_config_path_for_update();
  void check_create_admin_user();
  void create_admin_user();
  bool check_configuration_updates(bool *restart, bool *dynamic_sysvar_change,
                                   bool *config_file_change);

  bool clear_super_read_only();
  void restore_super_read_only();
  void check_lock_service();

 protected:
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  std::string m_current_user;
  std::string m_current_host;
  bool m_local_target = false;
  Configure_instance_options m_options;

  // By default, the clusterAdmin account will be created unless
  // it's not specified on the command options or it already exists
  bool m_create_cluster_admin = true;
  bool m_install_lock_service_udfs = false;

  mysqlshdk::utils::nullable<bool> m_can_set_persist;
  bool m_can_restart = false;

  bool m_needs_configuration_update = false;  //< config changes is general
  bool m_needs_update_mycnf = false;  //< changes that have to be done in my.cnf
  bool m_needs_restart = false;       //< server restart needed
  bool m_is_sandbox = false;          // true if the instance is a sandbox
  bool m_is_cnf_from_sandbox = false;  // true if mycnfpath was originally empty
                                       // but was automatically filled because
                                       // the instance is a sandbox.

  bool m_set_applier_worker_threads = false;

  // Configuration object (to read and set instance configurations).
  std::unique_ptr<mysqlshdk::config::Config> m_cfg;

  // List of invalid (incompatible) configurations to update.
  std::vector<mysqlshdk::mysql::Invalid_config> m_invalid_cfgs;

 private:
  const Cluster_type m_purpose;
  const TargetType::Type m_instance_type;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_CONFIGURE_INSTANCE_H_
