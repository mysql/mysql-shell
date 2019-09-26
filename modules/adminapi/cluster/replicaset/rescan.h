/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_REPLICASET_RESCAN_H_
#define MODULES_ADMINAPI_REPLICASET_RESCAN_H_

#include <memory>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/replicaset/replicaset.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {

class Rescan : public Command_interface {
 public:
  Rescan(
      bool interactive, mysqlshdk::utils::nullable<bool> update_topology_mode,
      bool auto_add_instances, bool auto_remove_instances,
      const std::vector<mysqlshdk::db::Connection_options> &add_instances_list,
      const std::vector<mysqlshdk::db::Connection_options>
          &remove_instances_list,
      GRReplicaSet *replicaset);

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
  const bool m_interactive = false;
  mysqlshdk::utils::nullable<bool> m_update_topology_mode;
  const bool m_auto_add_instances = false;
  const bool m_auto_remove_instances = false;
  std::vector<mysqlshdk::db::Connection_options> m_add_instances_list;
  std::vector<mysqlshdk::db::Connection_options> m_remove_instances_list;
  GRReplicaSet *m_replicaset = nullptr;

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
   * @return a Map with the differences between the running replicaset and the
   *         information registered in the metadata (i.e., new instances, not
   *         available instances, and topology mode changes).
   */
  shcore::Value::Map_type_ref get_rescan_report() const;

  /**
   * Auxiliary function to add new instances or remove unavailable instances
   * from the metadata.
   *
   * @param found_instances list of new or unavailable instances reported by
   *                        get_rescan_report().
   * @param auto_opt boolean indicating if "auto" was used for the corresponding
   *                 to add or remove the instances.
   * @param instances_list_opt list of connection options used for the
   *                           corresponding option to add or remove the
   *                           instances.
   * @param to_add boolean value to indicate if the instances will be added to
   *               the metadata (true) or removed from the metadata (false).
   */
  void update_instances_list(
      std::shared_ptr<shcore::Value::Array_type> found_instances, bool auto_opt,
      std::vector<mysqlshdk::db::Connection_options> *instances_list_opt,
      bool to_add);

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

  /**
   * Remove the given instance from the metadata.
   *
   * NOTE: No connection will be performed to the instance to remove.
   *
   * @param instance_address string with the address of the instance to remove
   *                         from the metadata.
   */
  void remove_instance_from_metadata(const std::string &instance_address);
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICASET_RESCAN_H_
