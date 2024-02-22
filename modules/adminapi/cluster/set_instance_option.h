/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_SET_INSTANCE_OPTION_H_
#define MODULES_ADMINAPI_CLUSTER_SET_INSTANCE_OPTION_H_

#include <optional>
#include <string>

#include "adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"
#include "mysqlshdk/libs/config/config.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Set_instance_option final : public Command_interface {
 public:
  Set_instance_option(
      const Cluster_impl &cluster,
      const mysqlshdk::db::Connection_options &instance_cnx_opts,
      const std::string &option, const std::string &value);
  Set_instance_option(
      const Cluster_impl &cluster,
      const mysqlshdk::db::Connection_options &instance_cnx_opts,
      const std::string &option, int64_t value);

  ~Set_instance_option() override;

  /**
   * Prepare the Cluster set_instance_option command for execution.
   * Validates parameters and others, more specifically:
   * - Validate if the option is valid, being the accepted values:
   *     - label
   *     - exitStateAction
   *     - memberWeight
   * - Validate the connection options
   * - Verify if the target instance belongs to the cluster
   * - Verify if the target cluster member is ONLINE
   * - Verify user privileges to execute operation;
   * - Verify if the target cluster member supports the option
   * - Verify if the cluster has quorum
   */
  void prepare() override;

  /**
   * Execute the Cluster set_instance_option command.
   *
   * @return An empty shcore::Value.
   */
  shcore::Value execute() override;

  /**
   * Rollback the command.
   *
   * NOTE: Not currently used.
   */
  void rollback() override {}

  /**
   * Finalize the command execution.
   */
  void finish() override {}

  /**
   * The current active instance
   */
  mysqlsh::dba::Instance *get_target_instance() const {
    return m_target_instance.get();
  }

 private:
  const Cluster_impl &m_cluster;
  mysqlshdk::db::Connection_options m_instance_cnx_opts;
  std::string m_target_instance_address;
  std::string m_address_in_metadata;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  // Configuration object (to read and set instance configurations).
  std::unique_ptr<mysqlshdk::config::Config> m_cfg;
  std::string m_option;
  std::optional<std::string> m_value_str;
  std::optional<int64_t> m_value_int;

  void ensure_option_valid();
  void ensure_instance_belong_to_cluster();
  void ensure_target_member_reachable();
  void ensure_option_supported_target_member();
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_SET_INSTANCE_OPTION_H_
