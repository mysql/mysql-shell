/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/global_topology_check.h"
#include "modules/adminapi/common/dba_errors.h"
#include "mysqlshdk/include/shellcore/console.h"

namespace mysqlsh {
namespace dba {

void validate_node_status(const topology::Node *node) {
  if (!node)
    throw shcore::Exception("Unable to find instance in the topology.",
                            SHERR_DBA_ASYNC_MEMBER_TOPOLOGY_MISSING);

  switch (node->status()) {
    case topology::Node_status::UNREACHABLE:
      throw shcore::Exception(node->label + " is unreachable",
                              SHERR_DBA_ASYNC_MEMBER_UNREACHABLE);

    case topology::Node_status::INVALIDATED:
      throw shcore::Exception(node->label + " was invalidated by a failover",
                              SHERR_DBA_ASYNC_MEMBER_INVALIDATED);

    case topology::Node_status::ERROR:
      if (node->get_primary_member()->master_channel)
        mysqlsh::current_console()->print_error(
            "Replication or configuration errors at " + node->label + ": " +
            mysqlshdk::mysql::format_status(
                node->get_primary_member()->master_channel->info));
      else
        mysqlsh::current_console()->print_error(
            "Replication or configuration error at " + node->label);
      throw shcore::Exception(
          "Replication or configuration errors at " + node->label,
          SHERR_DBA_REPLICATION_ERROR);

    case topology::Node_status::OFFLINE:
      throw shcore::Exception("Replication is stopped at " + node->label,
                              SHERR_DBA_ASYNC_MEMBER_NOT_REPLICATING);

    case topology::Node_status::INCONSISTENT:
      throw shcore::Exception(
          "Transaction set at " + node->label +
              " is inconsistent with the rest of the replicaset.",
          SHERR_DBA_ASYNC_MEMBER_INCONSISTENT);

    case topology::Node_status::ONLINE:
      break;
  }
}

namespace {
void validate_star_topology_consistent(
    const topology::Global_topology & /*topology*/) {
  // Ensures that the master cluster is available and all slaves are replicating
  // from the right source. Unavailable slaves are ignored.
  // XXX
}

}  // namespace

void validate_global_topology_active_cluster_available(
    const topology::Global_topology &topology) {
  auto console = current_console();

  if (topology.is_single_active()) {
    const topology::Node *master_node = topology.get_primary_master_node();

    validate_node_status(master_node);
  } else {
    assert(0);
  }
}

void validate_global_topology_consistent(
    const topology::Global_topology &topology) {
  validate_global_topology_active_cluster_available(topology);

  if (topology.type() == Global_topology_type::SINGLE_PRIMARY_TREE) {
    validate_star_topology_consistent(topology);
  } else {
    assert(0);
  }

  //   // check the cluster that will be our master
  //   *out_master_group = topology.try_get_group(master_group_name);
  //   if (!*out_master_group) {
  //     log_error("No cluster info for master group %s",
  //     master_group_name.c_str()); throw std::logic_error("internal error");
  //   }
  //   log_info("Cluster %s will be the master cluster",
  //            (*out_master_group)->label.c_str());
  //   topology.validate_group_strict(*out_master_group);

  //   // check the cluster that is the current slave of the master, which will
  //   // become our slave
  //   *out_slave_group = topology.try_get_group_slave_for(*out_master_group);
  //   if (*out_slave_group) {
  //     log_info("Cluster %s will be the slave cluster",
  //              (*out_slave_group)->label.c_str());
  //     topology.validate_group_strict(*out_slave_group);
  //   } else {
  //     log_info("Master cluster has no slaves");
  //   }
}

}  // namespace dba
}  // namespace mysqlsh
