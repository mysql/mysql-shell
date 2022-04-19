/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_RESCAN_H_
#define MODULES_ADMINAPI_CLUSTER_RESCAN_H_

#include <memory>
#include <string>
#include <vector>

#include "adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster/api_options.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Rescan : public Command_interface {
 public:
  Rescan(const Rescan_options &options, Cluster_impl *cluster);

  /**
   * Prepare the rescan command for execution.
   * Validates parameters and others, more specifically:
   * - Validate duplicates in the list of instances (to add or remove);
   * - All instances in the list to add must be active member of the GR group;
   * - All instances in the list to remove cannot be active member of the GR
   *   group;
   */
  void prepare() override;

  /**
   * Execute the rescan command.
   * More specifically:
   * - Get the rescan report (Map with needed changes, returned at the end).
   * - Use the result from the rescan report to:
   *   - Add newly discovered instances to the metadata (if needed);
   *   - Remove obsolete instances from the metadata (if needed);
   *   - Update the topology mode in the metadata (if needed);
   *     - Auto-increment setting will also be updated for all instance when the
   *       topology mode is updated.
   * - Return the result from the first step.
   *
   * @return an Map with the result of the rescan report.
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
   * NOTE: Does nothing in this case.
   */
  void finish() override;

 private:
  Rescan_options m_options;
  Cluster_impl *m_cluster = nullptr;
  bool m_is_view_change_uuid_supported = false;

  /**
   * Validate existence of duplicates for the addInstances and removeInstances
   * options.
   */
  void validate_list_duplicates() const;

  /**
   * Goes through the list of unavailable instances, and checks if they are
   * running auto-rejoin. If they are print a warning to let the user know and
   * remove the instance from the list. Also, if they are auto-rejoining,
   * remove them from the list received as parameter.
   * @param unavailable_instances vector with the unavailable instances.
   *        Any instances found on the vector that are doing auto-rejoin will be
   *        removed.
   */
  void ensure_unavailable_instances_not_auto_rejoining(
      std::vector<MissingInstanceInfo> &unavailable_instances) const;

  /**
   * Detect instance not in the desired state (active or not).
   *
   * @param instances_list target list of instances to check.
   * @param is_active boolean indicating the type of check to perform: if it is
   *                  active (true) or not (false).
   * @return a list of strings with the instances that do not match the active
   *         state.
   */
  std::vector<std::string> detect_invalid_members(
      const std::vector<mysqlshdk::db::Connection_options> &instances_list,
      bool is_active);

  /**
   * Get a report for the rescan.
   *
   * This function will determine all the differences in the topology (compared
   * to the metadata information) that can be addressed by the rescan operation,
   * namely: newly discovered instances, unavailable (obsolete) instances, and
   * change of the topology mode. A map (JSON like) will be returned with the
   * result.
   *
   * @return a Map with the differences between the running cluster and the
   *         information registered in the metadata (i.e., new instances, not
   *         available instances, and topology mode changes).
   */
  shcore::Value::Map_type_ref get_rescan_report() const;

  /**
   * Handles the addition of new instances to the the metadata.
   *
   * @param instance_list list of new instances reported by get_rescan_report().
   */
  void add_metadata_for_instances(
      const std::shared_ptr<shcore::Value::Array_type> &instance_list);

  /**
   * Handles the removal of unavailable instances from the the metadata.
   *
   * @param instance_list list of unavailable instances reported by
   * get_rescan_report().
   */
  void remove_metadata_for_instances(
      const std::shared_ptr<shcore::Value::Array_type> &instance_list);

  /**
   * Handles the update of modified instances on the the metadata.
   *
   * @param instance_list list of updated instances reported by
   * get_rescan_report().
   */
  void update_metadata_for_instances(
      const std::shared_ptr<shcore::Value::Array_type> &instance_list);

  /**
   * Update the topology mode in the metadata.
   *
   * @param topology_mode mysqlshdk::gr::Topology_mode to set.
   */
  void update_topology_mode(const mysqlshdk::gr::Topology_mode &topology_mode);

  /**
   * Add the given instance to the metadata.
   *
   * NOTE: A connection will be performed to the instance to add.
   *
   * @param instance_cnx_opts Object with the connection options of the instance
   *                          to add to the metadata.
   */
  void add_instance_to_metadata(
      const mysqlshdk::db::Connection_options &instance_cnx_opts);

  void update_metadata_for_instance(
      const mysqlshdk::db::Connection_options &instance_cnx_opts,
      Instance_id instance_id, const std::string &label);
  /**
   * Remove the given instance from the metadata.
   *
   * NOTE: No connection will be performed to the instance to remove.
   *
   * @param instance_address string with the address of the instance to remove
   *                         from the metadata.
   */
  void remove_instance_from_metadata(const std::string &instance_address);

  void upgrade_comm_protocol();

  /**
   * Ensures group_replication_view_change_uuid is set on all Cluster members
   * and stored in the Metadata schema.
   */
  void ensure_view_change_uuid_set();

  /**
   * Ensures group_replication_view_change_uuid is stored in the Metadata
   * schema.
   */
  void ensure_view_change_uuid_set_stored_metadata(
      const std::string &view_change_uuid = "");

  /**
   * Ensures the recovery accounts in use by each cluster member match the one
   * created for itself
   */
  void ensure_recovery_accounts_match();

  void check_mismatched_hostnames(shcore::Array_t instances) const;
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_RESCAN_H_
