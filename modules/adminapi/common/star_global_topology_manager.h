/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_STAR_GLOBAL_TOPOLOGY_MANAGER_H_
#define MODULES_ADMINAPI_COMMON_STAR_GLOBAL_TOPOLOGY_MANAGER_H_

#include <list>
#include "modules/adminapi/common/global_topology_manager.h"

namespace mysqlsh {
namespace dba {

class Star_global_topology_manager : public Global_topology_manager {
 public:
  explicit Star_global_topology_manager(
      uint64_t rclid, std::unique_ptr<topology::Global_topology> topology)
      : Global_topology_manager(rclid, std::move(topology)) {}

  Global_topology_type get_topology_type() const override {
    return Global_topology_type::SINGLE_PRIMARY_TREE;
  }

  void validate_adopt_cluster(const topology::Node **out_primary) override;

  void validate_add_replica(
      const topology::Node *master_node, mysqlshdk::mysql::IInstance *instance,
      const Async_replication_options &repl_options) override;

  void validate_rejoin_replica(mysqlshdk::mysql::IInstance *instance) override;

  void validate_remove_replica(mysqlshdk::mysql::IInstance *master,
                               mysqlshdk::mysql::IInstance *instance,
                               bool force, bool *out_repl_working) override;

  void validate_switch_primary(
      mysqlshdk::mysql::IInstance *master,
      mysqlshdk::mysql::IInstance *promoted,
      const std::list<std::shared_ptr<Instance>> &instances) override;

  void validate_force_primary(
      mysqlshdk::mysql::IInstance *master,
      const std::list<std::shared_ptr<Instance>> &instances) override;

 private:
  void validate_active_unavailable();
  void validate_force_primary_target(const topology::Node *promoted_node);

  Instance *pick_master_instance() const;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_STAR_GLOBAL_TOPOLOGY_MANAGER_H_
