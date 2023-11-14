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

#include "modules/adminapi/cluster/describe.h"

#include "adminapi/common/common.h"
#include "modules/adminapi/common/sql.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

void Describe::prepare() {
  // Sanity check: Verify if the topology type changed and issue an error if
  // needed.
  m_cluster.sanity_check();

  // Get the current members list
  m_instances = m_cluster.get_all_instances();
}

void Describe::feed_metadata_info(shcore::Dictionary_t dict,
                                  const Instance_metadata &info) {
  (*dict)["address"] = shcore::Value(info.endpoint);
  (*dict)["label"] = shcore::Value(info.label);

  if (info.hidden_from_router) {
    (*dict)["hiddenFromRouter"] = shcore::Value::True();
  }

  if (info.instance_type == Instance_type::READ_REPLICA) {
    (*dict)["role"] = shcore::Value("READ_REPLICA");

    // Get the replicationSources
    shcore::Array_t source_list = shcore::make_array();

    Replication_sources replication_sources =
        m_cluster.get_read_replica_replication_sources(info.uuid);

    if (replication_sources.source_type == Source_type::PRIMARY) {
      source_list->push_back(
          shcore::Value(shcore::str_upper(kReplicationSourcesAutoPrimary)));
    } else if (replication_sources.source_type == Source_type::SECONDARY) {
      source_list->push_back(
          shcore::Value(shcore::str_upper(kReplicationSourcesAutoSecondary)));
    } else {
      for (const auto &source : replication_sources.replication_sources) {
        source_list->push_back(shcore::Value(source.to_string()));
      }
    }

    (*dict)["replicationSources"] = shcore::Value(source_list);

  } else {
    (*dict)["role"] = shcore::Value("HA");
  }
}

shcore::Array_t Describe::get_topology() {
  shcore::Array_t instances_list = shcore::make_array();

  for (const auto &instance_def : m_instances) {
    shcore::Dictionary_t member = shcore::make_dict();
    feed_metadata_info(member, instance_def);

    instances_list->push_back(shcore::Value(std::move(member)));
  }

  return instances_list;
}

shcore::Dictionary_t Describe::collect_replicaset_description() {
  shcore::Dictionary_t ret = shcore::make_dict();

  (*ret)["name"] = shcore::Value("default");
  (*ret)["topologyMode"] = shcore::Value(m_cluster.get_topology_type());

  // Get and set the topology (all instances)
  (*ret)["topology"] = shcore::Value(get_topology());

  return ret;
}

shcore::Value Describe::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();
  (*dict)["clusterName"] = shcore::Value(m_cluster.get_name());
  (*dict)["defaultReplicaSet"] =
      shcore::Value(collect_replicaset_description());

  return shcore::Value(std::move(dict));
}

void Describe::finish() {
  // Reset all auxiliary (temporary) data used for the operation execution.
  m_instances.clear();
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
