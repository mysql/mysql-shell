/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <utility>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/replicaset/replicaset_describe.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {

Replicaset_describe::Replicaset_describe(const ReplicaSet &replicaset)
    : m_replicaset(replicaset) {}

Replicaset_describe::~Replicaset_describe() {}

void Replicaset_describe::prepare() {
  // Save the reference of the cluster object
  m_cluster = m_replicaset.get_cluster();

  // Sanity checks
  {
    // Verify if the cluster is still registed in the Metadata
    if (!m_cluster->get_metadata_storage()->cluster_exists(
            m_cluster->get_name()))
      throw shcore::Exception::runtime_error(
          "The cluster '" + m_cluster->get_name() +
          "' is no longer registered in the Metadata.");

    // Verify if the topology type changed and issue an error if needed.
    m_replicaset.sanity_check();
  }

  // Get the current members list
  m_instances = m_replicaset.get_instances();
}

void Replicaset_describe::feed_metadata_info(
    shcore::Dictionary_t dict, const ReplicaSet::Instance_info &info) {
  (*dict)["address"] = shcore::Value(info.classic_endpoint);
  (*dict)["role"] = shcore::Value(info.role);
  (*dict)["label"] = shcore::Value(info.name);
}

void Replicaset_describe::feed_member_info(
    shcore::Dictionary_t dict, const mysqlshdk::gr::Member &member) {
  if (!member.version.empty()) {
    (*dict)["version"] = shcore::Value(member.version);
  }
}

shcore::Array_t Replicaset_describe::get_topology(
    const std::vector<mysqlshdk::gr::Member> &member_info) {
  shcore::Array_t instances_list = shcore::make_array();

  auto get_member = [&member_info](const std::string &uuid) {
    for (const auto &m : member_info) {
      if (m.uuid == uuid) return m;
    }
    return mysqlshdk::gr::Member();
  };

  for (const auto &inst : m_instances) {
    shcore::Dictionary_t member = shcore::make_dict();
    mysqlshdk::gr::Member minfo(get_member(inst.uuid));

    feed_metadata_info(member, inst);
    feed_member_info(member, minfo);

    instances_list->push_back(shcore::Value(member));
  }

  return instances_list;
}

shcore::Dictionary_t Replicaset_describe::collect_replicaset_description() {
  shcore::Dictionary_t tmp = shcore::make_dict();
  shcore::Dictionary_t ret = shcore::make_dict();

  auto group_session = m_cluster->get_group_session();

  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  std::string gr_primary_uuid =
      mysqlshdk::gr::get_group_primary_uuid(group_session, nullptr);

  std::string topology_mode =
      !gr_primary_uuid.empty()
          ? mysqlshdk::gr::to_string(
                mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY)
          : mysqlshdk::gr::to_string(
                mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

  // Set ReplicaSet name
  (*ret)["name"] = shcore::Value(m_replicaset.get_name());
  (*ret)["topologyMode"] = shcore::Value(topology_mode);

  mysqlshdk::mysql::Instance group_instance(group_session);

  bool single_primary;
  std::vector<mysqlshdk::gr::Member> member_info(
      mysqlshdk::gr::get_members(group_instance, &single_primary));

  (*ret)["topology"] = shcore::Value(get_topology(member_info));

  return ret;
}

shcore::Value Replicaset_describe::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();

  // Get the ReplicaSet description
  shcore::Dictionary_t replicaset_dict;

  replicaset_dict = collect_replicaset_description();

  // Check if the ReplicaSet group session is established to an instance with a
  // state different than
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
    auto group_session = m_cluster->get_group_session();

    auto state =
        get_replication_group_state(mysqlshdk::mysql::Instance(group_session),
                                    get_gr_instance_type(group_session));

    bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                    state.source_state != ManagedInstance::OnlineRO);
    if (warning) {
      std::string warning =
          "The cluster description may be inaccurate as it was generated from "
          "an instance in ";
      warning.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*replicaset_dict)["warning"] = shcore::Value(warning);
    }
  }

  return shcore::Value(replicaset_dict);
}

void Replicaset_describe::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Replicaset_describe::finish() {
  // Reset all auxiliary (temporary) data used for the operation execution.
  m_instances.clear();
}

}  // namespace dba
}  // namespace mysqlsh
