/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_DBA_CREATE_CLUSTER_H_
#define MODULES_ADMINAPI_DBA_CREATE_CLUSTER_H_

#include <memory>
#include <string>

#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/dba/api_options.h"
#include "modules/command_interface.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/config/config.h"

namespace mysqlsh {
namespace dba {

class Create_cluster : public Command_interface {
 public:
  Create_cluster(const std::shared_ptr<Instance> &target_instance,
                 const std::string &cluster_name,
                 const Create_cluster_options &options);

  Create_cluster(const std::shared_ptr<MetadataStorage> &metadata,
                 const std::shared_ptr<Instance> &target_instance,
                 const std::shared_ptr<Instance> &primary_master,
                 const std::string &cluster_name,
                 const Create_cluster_options &options);

  ~Create_cluster() override;

  /**
   * Prepare the create_cluster command for execution.
   * Validates parameters and others, more specifically:
   * - Validate the cluster name;
   * - Validate the GR options (values and options combinations);
   * - Validate the Clone options (values and options combinations);
   *   NOTE: This includes handling the multiPrimary option (and prompt);
   * - If not an adopted cluster:
   *     - ensure the instance configuration is valid;
   *     - print autoRejoin warning if needed;
   *     - force exitStateAction default to READ_ONLY;
   *     - Validate replication filters;
   *     - Resolve the SSL Mode (based on option and server support);
   *     - Ensure target instance does not belong to a replicaset;
   *     - Get the report host value (to be used by GR and Metadata);
   *     - Resolve the GR local address;
   *     - Generate the GR group name to use if needed;
   * - else:
   *     - Determine the topology mode used by the adopted group;
   * - Validate super_read_only;
   * - If not an adopted cluster:
   *     - Prepare Config object;
   */
  void prepare() override;

  /**
   * Execute the create_cluster command.
   * More specifically:
   * - Log common information (e.g., used GR options/settings if not adopted);
   * - Install GR plugin (if needed);
   * - If not an adopted cluster:
   *     - start replicaset (bootstrap GR);
   *     - create recovery (replication) user;
   *     - Set GR recovery user;
   * - Create Metadata schema (if needed);
   * - Create and initialize Cluster object;
   * - Insert Cluster into the Metadata;
   * - Set default Replicaset and insert it into the Metadata;
   * - If adopted cluster:
   *     - Add all cluster instances into the Metadata;
   *   else:
   *     - Add the target instance into the Metadata (if needed);
   *
   * @return shcore::Value with the new cluster.
   */
  shcore::Value execute() override;

  /**
   * Rollback the command.
   *
   * NOTE: Not currently used (does nothing).
   */
  void rollback() override;

  /**
   * Finalize the command execution.
   *
   * NOTE: Do nothing currently.
   */
  void finish() override;

  /**
   * Returns the communication stack with which the cluster will be created.
   *
   * NOTE: this should only be called after calling prepare() to make sure
   * that, if the default is "mysql" (e.g.: the version allows it), it is
   * properly returned.
   *
   * @return The communication stack intended for the new cluster.
   */
  std::string get_communication_stack() const;

 private:
  std::shared_ptr<MetadataStorage> m_metadata;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  std::shared_ptr<mysqlsh::dba::Instance> m_primary_master;
  const std::string m_cluster_name;
  Create_cluster_options m_options;
  bool m_retrying = false;
  std::string m_address_in_metadata;
  bool m_create_replica_cluster = false;
  mysqlshdk::mysql::Lock_scoped_list m_instance_locks;

  // Configuration object (to read and set instance configurations).
  std::unique_ptr<mysqlshdk::config::Config> m_cfg;

  void validate_create_cluster_options();
  void resolve_ssl_mode();
  void prepare_metadata_schema();
  void create_recovery_account(mysqlshdk::mysql::IInstance *primary,
                               mysqlshdk::mysql::IInstance *target,
                               Cluster_impl *cluster = nullptr,
                               std::string *out_username = nullptr);
  void store_recovery_account_metadata(mysqlshdk::mysql::IInstance *target,
                                       const Cluster_impl &cluster);
  void reset_recovery_all(Cluster_impl *cluster);
  void persist_sro_all(Cluster_impl *cluster);
  void lock_all_instances();

  /**
   * This method validates the use of IPv6 addresses on the localAddress of the
   * seed instance.
   */
  void validate_local_address_ip_compatibility() const;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_CREATE_CLUSTER_H_
