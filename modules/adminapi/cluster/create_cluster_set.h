/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_CREATE_CLUSTER_SET_H_
#define MODULES_ADMINAPI_CLUSTER_CREATE_CLUSTER_SET_H_

#include <string>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {
namespace clusterset {

class Create_cluster_set : public Command_interface {
 public:
  Create_cluster_set(Cluster_impl *cluster, const std::string &domain_name,
                     const Create_cluster_set_options &options);

  ~Create_cluster_set() override;

  /**
   * Prepare the Create_cluster_set command for execution.
   */
  void prepare() override;

  /**
   * Execute the Create_cluster_set command.
   *
   * @return shcore::Value with the new ClusterSet object created.
   */
  shcore::Value execute() override;

 private:
  Cluster_impl *m_cluster = nullptr;
  const std::string m_domain_name;
  Create_cluster_set_options m_options;

  /**
   * Check if all cluster members are running the minimum required version
   */
  void check_version_requirement();

  /**
   * Check if the cluster has illegal replication channels configured
   */
  void check_illegal_channels();

  /**
   * Validate the requirements for the Group Replication configuration:
   *
   * - Check if the cluster has group_replication_view_change_uuid set and
   *   stored in the Metadata
   */
  void check_gr_configuration();

  /**
   * Resolve the ClusterSet SSL-mode
   */
  void resolve_ssl_mode();
};

}  // namespace clusterset
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_CREATE_CLUSTER_SET_H_
