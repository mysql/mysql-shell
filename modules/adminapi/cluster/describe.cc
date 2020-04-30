/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/describe.h"
#include "modules/adminapi/common/sql.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Describe::Describe(const Cluster_impl &cluster) : m_cluster(cluster) {}

Describe::~Describe() {}

void Describe::prepare() {
  // Sanity check: Verify if the topology type changed and issue an error if
  // needed.
  m_cluster.sanity_check();

  // Get the current members list
  m_instances = m_cluster.get_instances();
}

void Describe::feed_metadata_info(shcore::Dictionary_t dict,
                                  const Instance_metadata &info) {
  (*dict)["address"] = shcore::Value(info.endpoint);
  (*dict)["role"] = shcore::Value("HA");
  (*dict)["label"] = shcore::Value(info.label);
}

shcore::Array_t Describe::get_topology() {
  std::vector<Instance_metadata> instance_defs = m_cluster.get_instances();

  shcore::Array_t instances_list = shcore::make_array();

  for (const auto &instance_def : instance_defs) {
    shcore::Dictionary_t member = shcore::make_dict();
    feed_metadata_info(member, instance_def);

    instances_list->push_back(shcore::Value(member));
  }

  return instances_list;
}

shcore::Dictionary_t Describe::collect_replicaset_description() {
  shcore::Dictionary_t tmp = shcore::make_dict();
  shcore::Dictionary_t ret = shcore::make_dict();

  // Set Cluster name and topologyMode
  (*ret)["name"] = shcore::Value("default");

  // Get and set the topology mode from the Metadata
  auto group_instance = m_cluster.get_target_server();

  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  std::string gr_primary_uuid =
      mysqlshdk::gr::get_group_primary_uuid(*group_instance, nullptr);

  std::string topology_mode =
      !gr_primary_uuid.empty()
          ? mysqlshdk::gr::to_string(
                mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY)
          : mysqlshdk::gr::to_string(
                mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

  (*ret)["topologyMode"] = shcore::Value(topology_mode);

  // Get and set the topology (all instances)
  (*ret)["topology"] = shcore::Value(get_topology());

  return ret;
}

shcore::Value Describe::get_default_replicaset_description() {
  shcore::Dictionary_t dict = shcore::make_dict();

  // Get the Default Cluster description
  shcore::Dictionary_t replicaset_dict;

  replicaset_dict = collect_replicaset_description();

  // Check if the Cluster group session is established to an instance with
  // a state different than
  //   - Online R/W
  //   - Online R/O
  //
  // Possibly with the state:
  //
  //   - RECOVERING
  //   - OFFLINE
  //   - ERROR
  //
  // If that's the case, a warning must be added to the resulting JSON object
  {
    auto group_instance = m_cluster.get_target_server();

    auto state = get_replication_group_state(
        *group_instance, get_gr_instance_type(*group_instance));

    bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                    state.source_state != ManagedInstance::OnlineRO);
    if (warning) {
      std::string warning_msg =
          "The cluster description may be inaccurate as it was generated from "
          "an instance in ";
      warning_msg.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      warning_msg.append(" state");
      (*replicaset_dict)["warning"] = shcore::Value(warning_msg);
    }
  }

  return shcore::Value(replicaset_dict);
}

shcore::Value Describe::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["clusterName"] = shcore::Value(m_cluster.get_name());

  // Get the default replicaSet description
  (*dict)["defaultReplicaSet"] =
      shcore::Value(get_default_replicaset_description());

  // Iterate all replicasets and get the description for each one
  // NOTE: to be done only when multiple replicasets are supported

  return shcore::Value(dict);
}

void Describe::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Describe::finish() {
  // Reset all auxiliary (temporary) data used for the operation execution.
  m_instances.clear();
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
