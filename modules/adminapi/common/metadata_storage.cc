/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/metadata_storage.h"

#include <mysqld_error.h>
#include <algorithm>

#include <list>
#include <string_view>

#include "adminapi/common/cluster_types.h"
#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "utils/version.h"

namespace mysqlsh {
namespace dba {

using mysqlshdk::utils::Version;

namespace {
Router_metadata unserialize_router(const mysqlshdk::db::Row_ref_by_name &row) {
  Router_metadata router;

  router.id = row.get_uint("router_id");
  router.hostname = row.get_string("host_name");
  router.name = row.get_string("router_name");

  if (!row.is_null("ro_port")) {
    router.ro_port = row.get_string("ro_port");
  }
  if (!row.is_null("rw_port")) {
    router.rw_port = row.get_string("rw_port");
  }
  if (!row.is_null("ro_x_port")) {
    router.ro_x_port = row.get_string("ro_x_port");
  }
  if (!row.is_null("rw_x_port")) {
    router.rw_x_port = row.get_string("rw_x_port");
  }
  if (row.has_field("rw_split_port") && !row.is_null("rw_split_port")) {
    router.rw_split_port = row.get_string("rw_split_port");
  }
  if (!row.is_null("last_check_in")) {
    router.last_checkin = row.get_string("last_check_in");
  }
  if (!row.is_null("version")) {
    router.version = row.get_string("version");
  }
  if (row.has_field("tags") && !row.is_null("tags")) {
    router.tags = shcore::Value::parse(row.get_string("tags", "{}")).as_map();
  }
  if (row.has_field("bootstrap_target_type") &&
      !row.is_null("bootstrap_target_type")) {
    router.bootstrap_target_type = row.get_string("bootstrap_target_type");
  }
  if (row.has_field("targetCluster") && !row.is_null("targetCluster")) {
    router.target_cluster = row.get_string("targetCluster");
  }

  return router;
}

constexpr const char *k_list_routers_1_0_1 =
    "SELECT r.router_id, r.router_name, h.host_name,"
    " r.attributes->>'$.ROEndpoint' AS ro_port,"
    " r.attributes->>'$.RWEndpoint' AS rw_port,"
    " r.attributes->>'$.ROXEndpoint' AS ro_x_port,"
    " r.attributes->>'$.RWXEndpoint' AS rw_x_port,"
    " r.attributes->>'$.RWSplitEndpoint' AS rw_split_port,"
    " NULL as last_check_in,"
    " r.attributes->>'$.version' AS version"
    " FROM mysql_innodb_cluster_metadata.routers r"
    " JOIN mysql_innodb_cluster_metadata.hosts h "
    " ON r.host_id = h.host_id";

constexpr const char *k_list_routers =
    "SELECT r.router_id, r.router_name, r.address as host_name,"
    " r.attributes->>'$.ROEndpoint' AS ro_port,"
    " r.attributes->>'$.RWEndpoint' AS rw_port,"
    " r.attributes->>'$.ROXEndpoint' AS ro_x_port,"
    " r.attributes->>'$.RWXEndpoint' AS rw_x_port,"
    " r.attributes->>'$.RWSplitEndpoint' AS rw_split_port,"
    " r.attributes->>'$.bootstrapTargetType' AS bootstrap_target_type,"
    " r.last_check_in, r.version,"
    " r.options->>'$.target_cluster' as targetCluster,"
    " r.options->'$.tags' as tags"
    " FROM mysql_innodb_cluster_metadata.v2_routers r";

const char *get_router_query(const Version &md_version) {
  if (md_version.get_major() == 1)
    return k_list_routers_1_0_1;
  else
    return k_list_routers;
}

static constexpr const char *k_select_cluster_metadata_2_1_0 =
    R"*(SELECT c.* FROM (
SELECT cluster_type, primary_mode, cluster_id, cluster_name,
      description, NULL as group_name, async_topology_type,
      NULL as clusterset_id, 0 as invalidated,
      attributes->'$.tags' as tags
FROM mysql_innodb_cluster_metadata.v2_ar_clusters
UNION ALL
SELECT c.cluster_type, c.primary_mode, c.cluster_id, c.cluster_name,
      c.description, c.group_name, NULL as async_topology_type, c.clusterset_id,
      coalesce(cv.invalidated, 0) as invalidated,
      c.attributes->'$.tags' as tags
FROM mysql_innodb_cluster_metadata.v2_gr_clusters c
  LEFT JOIN mysql_innodb_cluster_metadata.v2_cs_members cv
  ON c.cluster_id = cv.cluster_id
) as c)*";

static constexpr const char *k_select_cluster_metadata_2_0_0 =
    R"*(SELECT c.* FROM (
SELECT cluster_type, primary_mode, cluster_id, cluster_name,
      description, NULL as group_name, async_topology_type, 0 as invalidated
FROM mysql_innodb_cluster_metadata.v2_ar_clusters
UNION ALL
SELECT cluster_type, primary_mode, cluster_id, cluster_name,
      description, group_name, NULL as async_topology_type, 0 as invalidated
FROM mysql_innodb_cluster_metadata.v2_gr_clusters
) as c)*";

static constexpr const char *k_select_cluster_metadata_1_0_1 =
    R"*(SELECT r.topology_type, c.cluster_id, c.cluster_name, c.description,
 r.attributes->>'$.group_replication_group_name' as group_name, 0 as invalidated
 FROM mysql_innodb_cluster_metadata.clusters c
 JOIN mysql_innodb_cluster_metadata.replicasets r
  ON c.cluster_id = r.cluster_id)*";

std::string get_cluster_query(const Version &md_version) {
  if (md_version.get_major() == 1) {
    return k_select_cluster_metadata_1_0_1;
  } else if (md_version.get_major() == 2 && md_version.get_minor() == 0) {
    return k_select_cluster_metadata_2_0_0;
  } else {
    return k_select_cluster_metadata_2_1_0;
  }
}

constexpr const char *k_base_instance_query =
    "SELECT i.instance_id, i.cluster_id, c.group_name, "
    " am.master_instance_id, am.master_member_id, am.member_role, am.view_id,"
    " i.label, i.mysql_server_uuid, i.address,"
    " i.endpoint, i.xendpoint, ii.addresses->>'$.grLocal' as grendpoint,"
    " CAST(ii.attributes->'$.server_id' AS UNSIGNED) server_id,"
    " IFNULL(CAST(ii.attributes->'$.tags._hidden' AS UNSIGNED), false)"
    " hidden_from_router,"
    " ii.attributes->'$.tags' as tags, i.instance_type"
    " FROM mysql_innodb_cluster_metadata.v2_instances i"
    " LEFT JOIN mysql_innodb_cluster_metadata.instances ii"
    "   ON ii.instance_id = i.instance_id"
    " LEFT JOIN mysql_innodb_cluster_metadata.v2_gr_clusters c"
    "   ON c.cluster_id = i.cluster_id"
    " LEFT JOIN mysql_innodb_cluster_metadata.v2_ar_members am"
    "   ON am.instance_id = i.instance_id";

constexpr const char *k_base_instance_query_1_0_1 =
    "SELECT i.instance_id, r.cluster_id, i.role,"
    " r.attributes->>'$.group_replication_group_name' group_name,"
    " i.instance_name label, i.mysql_server_uuid, "
    " i.addresses->>'$.mysqlClassic' endpoint,"
    " i.addresses->>'$.mysqlX' xendpoint,"
    " i.addresses->>'$.grLocal' grendpoint,"
    " CAST(i.attributes->'$.server_id' AS UNSIGNED) server_id"
    " FROM mysql_innodb_cluster_metadata.instances i"
    " LEFT JOIN mysql_innodb_cluster_metadata.replicasets r"
    "   ON r.replicaset_id = i.replicaset_id";

constexpr const char *k_base_instance_query_2_0_0 =
    "SELECT i.instance_id, i.cluster_id, c.group_name, "
    " am.master_instance_id, am.master_member_id, am.member_role, am.view_id,"
    " i.label, i.mysql_server_uuid, i.address,"
    " i.endpoint, i.xendpoint, ii.addresses->>'$.grLocal' as grendpoint,"
    " CAST(ii.attributes->'$.server_id' AS UNSIGNED) server_id,"
    " IFNULL(CAST(ii.attributes->'$.tags._hidden' AS UNSIGNED), false)"
    " hidden_from_router,"
    " ii.attributes->'$.tags' as tags"
    " FROM mysql_innodb_cluster_metadata.v2_instances i"
    " LEFT JOIN mysql_innodb_cluster_metadata.instances ii"
    "   ON ii.instance_id = i.instance_id"
    " LEFT JOIN mysql_innodb_cluster_metadata.v2_gr_clusters c"
    "   ON c.cluster_id = i.cluster_id"
    " LEFT JOIN mysql_innodb_cluster_metadata.v2_ar_members am"
    "   ON am.instance_id = i.instance_id";

std::string get_instance_query(const Version &md_version) {
  if (md_version.get_major() == 1) {
    return k_base_instance_query_1_0_1;
  } else if (md_version.get_major() == 2 && md_version.get_minor() < 2) {
    return k_base_instance_query_2_0_0;
  } else {
    return k_base_instance_query;
  }
}

// In Metadata schema versions higher than 1.0.1
// instances.mysql_server_uuid uses the collation ascii_general_ci which
// then when doing comparisons with the sysvar @@server_uuid will results
// in an illegal mix of collations. For that reason, we must do the right
// cast of @@server_uuid to ascii_general_ci
constexpr const char *k_all_online_check_query_2_0_0 =
    "SELECT (SELECT COUNT(*) "
    "FROM !.instances "
    "WHERE cluster_id = (SELECT cluster_id "
    "FROM !.instances "
    "WHERE CAST(mysql_server_uuid AS CHAR CHARACTER SET ASCII) = "
    "CAST(@@server_uuid AS CHAR CHARACTER SET ASCII))) "
    "= (SELECT count(*) FROM performance_schema.replication_group_members "
    "WHERE member_state = 'ONLINE') as all_online";

constexpr const char *k_all_online_check_query_1_0_1 =
    "SELECT (SELECT COUNT(*) "
    "FROM !.instances "
    "WHERE replicaset_id = (SELECT replicaset_id "
    "FROM !.instances "
    "WHERE mysql_server_uuid=@@server_uuid)) = (SELECT count(*) "
    "FROM performance_schema.replication_group_members "
    "WHERE member_state = 'ONLINE') as all_online";

constexpr const char *k_all_online_check_query =
    "SELECT (SELECT COUNT(*) "
    "FROM !.instances "
    "WHERE instance_type <> 'read-replica' AND cluster_id = (SELECT "
    "cluster_id FROM !.instances "
    "WHERE mysql_server_uuid = @@server_uuid)) "
    "= (SELECT count(*) FROM performance_schema.replication_group_members "
    "WHERE member_state = 'ONLINE') as all_online";

std::string get_all_online_check_query(const Version &md_version) {
  if (md_version.get_major() == 1) {
    return k_all_online_check_query_1_0_1;
  } else if (md_version.get_major() == 2 && md_version.get_minor() < 2) {
    return k_all_online_check_query_2_0_0;
  } else {
    return k_all_online_check_query;
  }
}

constexpr const char *k_base_instance_in_cluster_query =
    "SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances "
    "WHERE cluster_id = ? AND LOWER(addresses->>'$.mysqlClassic') = LOWER(?)";

constexpr const char *k_base_instance_in_cluster_query_1_0_1 =
    "SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances, "
    "mysql_innodb_cluster_metadata.replicasets "
    "WHERE cluster_id = ? AND LOWER(addresses->>'$.mysqlClassic') = LOWER(?)";

std::string get_instance_in_cluster_query(const Version &md_version) {
  if (md_version.get_major() == 1)
    return k_base_instance_in_cluster_query_1_0_1;
  else
    return k_base_instance_in_cluster_query;
}

constexpr const char *k_base_cluster_size_query =
    "SELECT COUNT(*) as count "
    "FROM mysql_innodb_cluster_metadata.instances "
    "WHERE cluster_id = ?";

constexpr const char *k_cluster_size_query_1_0_1 =
    "SELECT COUNT(*) as count "
    "FROM mysql_innodb_cluster_metadata.instances, "
    "mysql_innodb_cluster_metadata.replicasets "
    "WHERE cluster_id = ?";

std::string get_cluster_size_query(const Version &md_version) {
  if (md_version.get_major() == 1)
    return k_cluster_size_query_1_0_1;
  else
    return k_base_cluster_size_query;
}

constexpr const char *k_base_topology_mode_query =
    "SELECT primary_mode FROM mysql_innodb_cluster_metadata.clusters "
    "WHERE cluster_id = ?";

constexpr const char *k_base_topology_mode_query_1_0_1 =
    "SELECT topology_type FROM mysql_innodb_cluster_metadata.replicasets "
    "WHERE cluster_id = ?";

std::string get_topology_mode_query(const Version &md_version) {
  if (md_version.get_major() == 1)
    return k_base_topology_mode_query_1_0_1;
  else
    return k_base_topology_mode_query;
}

constexpr std::string_view k_select_router_options =
    R"*(SELECT
CONCAT(CONVERT(r.address using utf8mb4), '::', r.router_name) AS router_label,
JSON_MERGE_PATCH(
  COALESCE(r.attributes->>'$.Configuration', '{}'),
  COALESCE( ( SELECT cs.router_options->>'$.ConfigurationChanges' FROM mysql_innodb_cluster_metadata.clustersets cs where (cs.clusterset_id = r.clusterset_id) ), '{}' ),
  COALESCE( ( SELECT c.router_options->>'$.ConfigurationChanges' FROM mysql_innodb_cluster_metadata.clusters c where (c.cluster_id = r.cluster_id) ), '{}' ),
  COALESCE( r.attributes->>'$.ConfigurationChanges', '{}' ),
  JSON_OBJECT(
    "routing_rules",
    (
      WITH ro AS (
        SELECT JSON_MERGE_PATCH(
          COALESCE((SELECT cs.router_options FROM mysql_innodb_cluster_metadata.clustersets cs where cs.clusterset_id = r.clusterset_id), '{}'),
          COALESCE((SELECT c.router_options FROM mysql_innodb_cluster_metadata.clusters c where c.cluster_id = r.cluster_id), '{}'),
            COALESCE(r.options, '{}')
        ) AS merged_options
      )
      SELECT JSON_MERGE_PATCH("{}",
        JSON_OBJECT(
          "tags", JSON_EXTRACT(ro.merged_options, "$.tags"),
          "target_cluster", JSON_EXTRACT(ro.merged_options, "$.target_cluster"),
          "use_replica_primary_as_rw", JSON_EXTRACT(ro.merged_options, "$.use_replica_primary_as_rw"),
          "stats_updates_frequency", JSON_EXTRACT(ro.merged_options, "$.stats_updates_frequency"),
          "read_only_targets", JSON_EXTRACT(ro.merged_options, "$.read_only_targets"),
          "unreachable_quorum_allowed_traffic", JSON_EXTRACT(ro.merged_options, "$.unreachable_quorum_allowed_traffic"),
          "invalidated_cluster_policy", JSON_EXTRACT(ro.merged_options, "$.invalidated_cluster_policy")
        )
      )
      FROM ro
    )
  )
) AS router_configuration
FROM mysql_innodb_cluster_metadata.routers r)*";

constexpr std::string_view k_select_router_versions =
    R"*(SELECT versions.version
  FROM mysql_innodb_cluster_metadata.!,
  JSON_TABLE(
    JSON_KEYS(
      JSON_EXTRACT(router_options, '$.Configuration')
    ),
    '$[*]' COLUMNS (version VARCHAR(9) PATH '$'))
  AS versions
  )*";

constexpr std::string_view k_select_default_router_options =
    R"*(SELECT
  JSON_MERGE_PATCH(
    IFNULL(
    JSON_EXTRACT(
      JSON_EXTRACT(
        JSON_EXTRACT(c.router_options, '$.Configuration'),
        ?),
    '$.Defaults'), '{}'),
    JSON_OBJECT(
      "routing_rules",
      (
        SELECT
          JSON_MERGE_PATCH(
            '{}',
            JSON_OBJECT(
              "tags",
              JSON_EXTRACT(c.router_options, "$.tags"),
              "target_cluster",
              JSON_EXTRACT(c.router_options, "$.target_cluster"),
              "use_replica_primary_as_rw",
              JSON_EXTRACT(c.router_options, "$.use_replica_primary_as_rw"),
              "stats_updates_frequency",
              JSON_EXTRACT(c.router_options, "$.stats_updates_frequency"),
              "read_only_targets",
              JSON_EXTRACT(c.router_options, "$.read_only_targets"),
              "unreachable_quorum_allowed_traffic",
              JSON_EXTRACT(c.router_options, "$.unreachable_quorum_allowed_traffic"),
              "invalidated_cluster_policy",
              JSON_EXTRACT(c.router_options, "$.invalidated_cluster_policy")
            )
          ) AS routing_rules
      )
    )
  ))*";

constexpr std::string_view
    k_select_default_router_configuration_changes_schema =
        R"*(SELECT JSON_EXTRACT(
            JSON_EXTRACT(
                JSON_EXTRACT(
                    JSON_EXTRACT(router_options, '$.Configuration'),
                    ?),
                '$.ConfigurationChangesSchema'),
            '$.properties')
      )*";

}  // namespace

std::string to_string(const Instance_type instance_type) {
  switch (instance_type) {
    case Instance_type::GROUP_MEMBER:
      return "GROUP-MEMBER";
    case Instance_type::ASYNC_MEMBER:
      return "ASYNC-MEMBER";
    case Instance_type::READ_REPLICA:
      return "READ-REPLICA";
    default:
      throw std::logic_error("Unexpected instance_type");
  }
}

Instance_type to_instance_type(const std::string &instance_type) {
  if (shcore::str_caseeq("GROUP-MEMBER", instance_type)) {
    return Instance_type::GROUP_MEMBER;
  } else if (shcore::str_caseeq("ASYNC-MEMBER", instance_type)) {
    return Instance_type::ASYNC_MEMBER;
  } else if (shcore::str_caseeq("READ-REPLICA", instance_type)) {
    return Instance_type::READ_REPLICA;
  } else {
    throw std::runtime_error("Unsupported instance_type value: " +
                             instance_type);
  }
}

MetadataStorage::MetadataStorage(const std::shared_ptr<Instance> &instance)
    : m_md_server(instance), m_owns_md_server(true) {
  log_debug("Metadata operations will use %s", instance->descr().c_str());
  instance->retain();
}

MetadataStorage::MetadataStorage(const Instance &instance)
    : m_md_server(std::make_shared<Instance>(instance)),
      m_owns_md_server(false) {
  log_debug("Metadata operations will use %s", instance.descr().c_str());
}

MetadataStorage::~MetadataStorage() {
  if (m_md_server && m_owns_md_server) m_md_server->release();
}

bool MetadataStorage::check_version(
    mysqlshdk::utils::Version *out_version) const {
  if (m_md_state == mysqlsh::dba::metadata::State::NONEXISTING ||
      m_md_state == mysqlsh::dba::metadata::State::FAILED_UPGRADE ||
      m_md_state == mysqlsh::dba::metadata::State::UPGRADING) {
    try {
      m_md_state = mysqlsh::dba::metadata::check_installed_schema_version(
          m_md_server, &m_md_version, &m_real_md_version, &m_md_version_schema);
    } catch (const shcore::Error &e) {
      throw shcore::Exception::mysql_error_with_code(
          shcore::str_format(
              "Failed to execute query on Metadata server %s: %s",
              m_md_server->descr().c_str(), e.what()),
          e.code());
    }
  }

  if (m_md_version != mysqlsh::dba::metadata::kNotInstalled) {
    if (out_version) *out_version = m_md_version;
    return true;
  }

  return false;
}

bool MetadataStorage::is_valid() const {
  if (m_md_server) {
    if (m_md_server->get_session() && m_md_server->get_session()->is_open()) {
      return m_md_state != mysqlsh::dba::metadata::State::NONEXISTING;
    }
  }

  return false;
}

bool MetadataStorage::check_instance_type(
    const std::string & /*uuid*/,
    const mysqlshdk::utils::Version & /*md_version*/, Cluster_type *out_type,
    Replica_type *out_replica_type) const {
  if (real_version() != metadata::kNotInstalled) {
    if (m_real_md_version.get_major() == 1) {
      try {
        auto query = shcore::sqlstring(
            "select count(*) from !.instances where "
            "mysql_server_uuid = @@server_uuid",
            0);
        query << m_md_version_schema;
        query.done();

        auto result = execute_sql(query);
        auto row = result->fetch_one();
        if (row) {
          if (row->get_int(0) != 0) {
            log_debug(
                "Instance type check: %s: Cluster metadata record found "
                "(metadata %s)",
                m_md_server->descr().c_str(),
                m_real_md_version.get_full().c_str());
            if (out_type) *out_type = Cluster_type::GROUP_REPLICATION;
            return true;
          } else {
            log_debug(
                "Instance type check: %s: Metadata record not found (metadata "
                "%s)",
                m_md_server->descr().c_str(),
                m_real_md_version.get_full().c_str());
          }
        }
      } catch (const shcore::Exception &error) {
        log_debug("Error querying metadata: %s: version %s: %i %s",
                  m_md_server->descr().c_str(),
                  m_real_md_version.get_full().c_str(), error.code(),
                  error.what());
        throw;
      }
    } else {
      try {
        shcore::sqlstring query;
        std::string instance_type;
        bool get_instance_type = false;

        if (supports_read_replicas()) {
          get_instance_type = true;
          query = shcore::sqlstring(
              "select cluster_type, instance_type from !.v2_this_instance", 0);
        } else {
          query = shcore::sqlstring(
              "select cluster_type from !.v2_this_instance", 0);
        }

        query << m_md_version_schema;
        query.done();

        auto result = execute_sql(query);
        auto row = result->fetch_one();

        if (row && !row->is_null(0)) {
          std::string type = row->get_string(0);
          if (get_instance_type) {
            instance_type = row->get_string(1, "");
          }
          if (type == "ar") {
            log_debug(
                "Instance type check: %s: ReplicaSet metadata record found "
                "(metadata %s)",
                m_md_server->descr().c_str(),
                m_real_md_version.get_full().c_str());
            if (out_type) *out_type = Cluster_type::ASYNC_REPLICATION;
            return true;
          } else if (type == "gr") {
            log_debug(
                "Instance type check: %s: Cluster metadata record found "
                "(metadata %s)",
                m_md_server->descr().c_str(),
                m_real_md_version.get_full().c_str());
            if (out_type) *out_type = Cluster_type::GROUP_REPLICATION;

            if (out_replica_type) {
              if (!instance_type.empty() && to_instance_type(instance_type) ==
                                                Instance_type::READ_REPLICA) {
                *out_replica_type = Replica_type::READ_REPLICA;
              } else {
                *out_replica_type = Replica_type::GROUP_MEMBER;
              }
            }

            return true;
          } else {
            throw shcore::Exception(
                "Unexpected cluster type in metadata " + type,
                SHERR_DBA_METADATA_INVALID);
          }
        } else {
          log_debug(
              "Instance type check: %s: Metadata record not found (metadata "
              "%s)",
              m_md_server->descr().c_str(),
              m_real_md_version.get_full().c_str());
        }
      } catch (const shcore::Exception &error) {
        log_debug("Error querying metadata: %s: version %s: %i %s",
                  m_md_server->descr().c_str(),
                  m_real_md_version.get_full().c_str(), error.code(),
                  error.what());
        throw;
      }
    }
  }
  return false;
}

bool MetadataStorage::check_all_members_online() const {
  // If the number of members that belong to the same replicaset in the
  // instances table is the same as the number of ONLINE members in
  // replication_group_members then ALL the members in the cluster are
  // ONLINE
  bool ret_val = false;
  if (real_version() != metadata::kNotInstalled) {
    auto query_base = get_all_online_check_query(m_real_md_version);

    auto result = m_md_server->queryf(query_base, m_md_version_schema.c_str(),
                                      m_md_version_schema.c_str());

    const mysqlshdk::db::IRow *row = result->fetch_one();

    if (row && row->get_int(0, 0)) ret_val = true;
  }

  return ret_val;
}

std::shared_ptr<mysqlshdk::db::IResult> MetadataStorage::execute_sql(
    const std::string &sql) const {
  std::shared_ptr<mysqlshdk::db::IResult> ret_val;

  try {
    ret_val = m_md_server->query(sql);
  } catch (const shcore::Error &err) {
    log_warning("While querying metadata: %s\n\t%s", err.format().c_str(),
                sql.c_str());
    if (CR_SERVER_GONE_ERROR == err.code()) {
      log_debug("The Metadata server is inaccessible");
      throw shcore::Exception::metadata_error("The Metadata is inaccessible");
    } else if (err.code() == ER_OPTION_PREVENTS_STATEMENT) {
      auto console = mysqlsh::current_console();

      console->print_error(
          m_md_server->descr() +
          ": Error updating cluster metadata: " + err.format());
      throw shcore::Exception("Metadata cannot be updated: " + err.format(),
                              SHERR_DBA_METADATA_READ_ONLY);
    } else {
      std::string err_msg = "Failed to execute query on Metadata server " +
                            m_md_server->descr() + ": ";
      err_msg.append(err.what());
      throw shcore::Exception::mysql_error_with_code(err_msg, err.code());
    }
  }

  return ret_val;
}

Cluster_metadata MetadataStorage::unserialize_cluster_metadata(
    const mysqlshdk::db::Row_ref_by_name &row, const Version &version) const {
  Cluster_metadata rs;
  std::string topology_type_md;

  if (version.get_major() == 1) {
    rs.cluster_id = std::to_string(row.get_uint("cluster_id"));
    topology_type_md = row.get_string("topology_type", "");
    rs.type = Cluster_type::GROUP_REPLICATION;
  } else {
    rs.cluster_id = row.get_string("cluster_id");

    if (row.has_field("clusterset_id") && !row.is_null("clusterset_id")) {
      rs.cluster_set_id = row.get_string("clusterset_id");
    }

    topology_type_md = row.get_string("primary_mode", "");

    if (row.get_string("cluster_type") == "ar") {
      rs.type = Cluster_type::ASYNC_REPLICATION;
    } else {
      rs.type = Cluster_type::GROUP_REPLICATION;
    }

    if (row.has_field("async_topology_type") &&
        !row.is_null("async_topology_type")) {
      rs.async_topology_type =
          to_topology_type(row.get_string("async_topology_type"));
    }
  }

  // Set cluster_topology_type
  if (!topology_type_md.empty()) {
    if (topology_type_md == "pm") {
      rs.cluster_topology_type = mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY;
    } else if (topology_type_md == "mm") {
      rs.cluster_topology_type = mysqlshdk::gr::Topology_mode::MULTI_PRIMARY;
    } else {
      throw shcore::Exception::metadata_error(
          "Unexpected topology mode found in Metadata: " + topology_type_md);
    }
  }

  rs.cluster_name = row.get_string("cluster_name");
  rs.description = row.get_string("description", "");
  rs.group_name = row.get_string("group_name", "");
  if (row.has_field("view_uuid"))
    rs.view_change_uuid = row.get_string("view_uuid", "");

  if (row.has_field("tags") && !row.is_null("tags"))
    rs.tags = shcore::Value::parse(row.get_string("tags", "{}")).as_map();

  return rs;
}

bool MetadataStorage::get_cluster(const Cluster_id &cluster_id,
                                  Cluster_metadata *out_cluster) {
  auto result = execute_sqlf(
      get_cluster_query(real_version()) + " WHERE c.cluster_id = ?",
      cluster_id);
  if (auto row = result->fetch_one_named()) {
    *out_cluster = unserialize_cluster_metadata(row, m_md_version);
    return true;
  }

  return false;
}

bool MetadataStorage::query_cluster_attribute(const Cluster_id &cluster_id,
                                              std::string_view attribute,
                                              shcore::Value *out_value) const {
  auto stmt = shcore::str_format(
      "SELECT attributes->'$.%.*s' FROM mysql_innodb_cluster_metadata.clusters "
      "WHERE cluster_id = ?",
      static_cast<int>(attribute.length()), attribute.data());

  auto result = execute_sql(shcore::sqlstring(stmt, 0) << cluster_id);

  if (auto row = result->fetch_one()) {
    if (!row->is_null(0)) {
      *out_value = shcore::Value::parse(row->get_as_string(0));
      return true;
    }
  }
  return false;
}

void MetadataStorage::remove_cluster_attribute(const Cluster_id &cluster_id,
                                               std::string_view attribute) {
  auto stmt = shcore::str_format(
      "UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = "
      "json_remove(attributes, '$.%.*s') WHERE cluster_id = ?",
      static_cast<int>(attribute.length()), attribute.data());
  shcore::sqlstring query(stmt, 0);
  query << cluster_id;
  query.done();

  execute_sql(query);
}

void MetadataStorage::update_cluster_attribute(const Cluster_id &cluster_id,
                                               std::string_view attribute,
                                               const shcore::Value &value,
                                               bool store_null) {
  if (value) {
    auto stmt = shcore::str_format(
        "UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = "
        "json_set(attributes, '$.%.*s', CAST(? as JSON)) WHERE cluster_id = ?",
        static_cast<int>(attribute.length()), attribute.data());

    shcore::sqlstring query(stmt, 0);
    query << value.repr() << cluster_id;
    query.done();

    execute_sql(query);
    return;
  }

  // value is null

  if (store_null) {
    auto stmt = shcore::str_format(
        "UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = "
        "json_set(attributes, '$.%.*s', CAST(NULL as JSON)) WHERE "
        "cluster_id = ?",
        static_cast<int>(attribute.length()), attribute.data());

    shcore::sqlstring query(stmt, 0);
    query << cluster_id;
    query.done();

    execute_sql(query);
    return;
  }

  auto stmt = shcore::str_format(
      "UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = "
      "json_remove(attributes, '$.%.*s') WHERE cluster_id = ?",
      static_cast<int>(attribute.length()), attribute.data());

  shcore::sqlstring query(stmt, 0);
  query << cluster_id;
  query.done();

  execute_sql(query);
}

void MetadataStorage::update_clusters_attribute(std::string_view attribute,
                                                const shcore::Value &value) {
  if (value) {
    shcore::sqlstring query(
        shcore::str_format(
            "UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = "
            "json_set(attributes, '$.%.*s', CAST(? as JSON))",
            static_cast<int>(attribute.length()), attribute.data()),
        0);

    query << value.repr();
    query.done();
    execute_sql(query);
  } else {
    shcore::sqlstring query(
        shcore::str_format("UPDATE mysql_innodb_cluster_metadata.clusters SET "
                           "attributes = json_remove(attributes, '$.%.*s')",
                           static_cast<int>(attribute.length()),
                           attribute.data()),
        0);
    query.done();
    execute_sql(query);
  }
}

bool MetadataStorage::query_cluster_capability(const Cluster_id &cluster_id,
                                               std::string_view capability,
                                               shcore::Value *out_value) const {
  auto attribute = shcore::str_format("%s.%.*s", k_cluster_capabilities,
                                      static_cast<int>(capability.length()),
                                      capability.data());

  return query_cluster_attribute(cluster_id, attribute, out_value);
}

void MetadataStorage::update_cluster_capability(
    const Cluster_id &cluster_id, const std::string &capability,
    const std::string &value, const std::set<std::string> &allowed_operations) {
  shcore::Dictionary_t cap = shcore::make_dict();

  if (!value.empty()) {
    shcore::Dictionary_t value_and_allow = shcore::make_dict();
    shcore::Array_t allowed_ops = shcore::make_array();

    value_and_allow->set("value", shcore::Value(value));

    for (const auto &op : allowed_operations) {
      allowed_ops->push_back(shcore::Value(op));
    }

    value_and_allow->set("allow", shcore::Value(allowed_ops));

    cap->set(capability, shcore::Value(value_and_allow));
  } else {
    cap->set(capability, shcore::Value(nullptr));
  }

  shcore::sqlstring query(
      "UPDATE mysql_innodb_cluster_metadata.clusters"
      " SET attributes = JSON_MERGE_PATCH(attributes, CAST('{\"" +
          std::string(k_cluster_capabilities) + "\": " +
          shcore::Value(cap).repr() + "}' as JSON)) WHERE cluster_id = ?",
      0);

  query << cluster_id;
  query.done();
  execute_sql(query);
}

void MetadataStorage::update_cluster_set_attribute(
    const Cluster_set_id &clusterset_id, std::string_view attribute,
    const shcore::Value &value) {
  shcore::sqlstring query;

  if (value) {
    auto stmt = shcore::str_format(
        "UPDATE mysql_innodb_cluster_metadata.clustersets SET attributes = "
        "json_set(attributes, '$.%.*s', CAST(? as JSON)) WHERE clusterset_id = "
        "?",
        static_cast<int>(attribute.length()), attribute.data());

    query = shcore::sqlstring(stmt, 0);
    query << value.repr() << clusterset_id;
  } else {
    auto stmt = shcore::str_format(
        "UPDATE mysql_innodb_cluster_metadata.clustersets SET attributes = "
        "json_remove(attributes, '$.%.*s') WHERE clusterset_id = ?",
        static_cast<int>(attribute.length()), attribute.data());

    query = shcore::sqlstring(stmt, 0);
    query << clusterset_id;
  }

  query.done();
  execute_sql(query);
}

bool MetadataStorage::query_cluster_set_attribute(
    const Cluster_set_id &clusterset_id, std::string_view attribute,
    shcore::Value *out_value) const {
  auto stmt = shcore::str_format(
      "SELECT attributes->'$.%.*s' FROM "
      "mysql_innodb_cluster_metadata.clustersets WHERE clusterset_id=?",
      static_cast<int>(attribute.length()), attribute.data());

  auto result = execute_sql(shcore::sqlstring(stmt, 0) << clusterset_id);

  if (auto row = result->fetch_one()) {
    if (!row->is_null(0)) {
      *out_value = shcore::Value::parse(row->get_as_string(0));
      return true;
    }
  }
  return false;
}

std::vector<Cluster_metadata> MetadataStorage::get_all_clusters(
    bool include_invalidated) {
  std::vector<Cluster_metadata> l;

  if (real_version() != metadata::kNotInstalled) {
    std::string query(get_cluster_query(m_real_md_version));

    // If a different schema is provided, uses it
    if (m_md_version_schema != metadata::kMetadataSchemaName)
      query = shcore::str_replace(query, metadata::kMetadataSchemaName,
                                  m_md_version_schema);

    auto result = execute_sqlf(query);

    while (auto row = result->fetch_one_named()) {
      if (include_invalidated || row.get_int("invalidated", 0) == 0)
        l.push_back(unserialize_cluster_metadata(row, m_real_md_version));
    }
  }

  return l;
}

bool MetadataStorage::get_cluster_for_server_uuid(
    const std::string &server_uuid, Cluster_metadata *out_cluster) const {
  std::shared_ptr<mysqlshdk::db::IResult> result;

  std::string query = get_cluster_query(real_version());

  std::string instances_table("v2_instances");
  std::string target_table("c");
  std::string field_name("cluster_id");

  if (m_md_version.get_major() == 1) {
    instances_table = "instances";
    target_table = "r";
    field_name = "replicaset_id";
  }

  result = execute_sqlf(query +
                            " JOIN mysql_innodb_cluster_metadata.! i"
                            "  ON i.! = !.!"
                            " WHERE i.mysql_server_uuid = ?",
                        instances_table, field_name, target_table, field_name,
                        server_uuid);

  if (auto row = result->fetch_one_named()) {
    *out_cluster = unserialize_cluster_metadata(row, m_md_version);
    return true;
  }

  return false;
}

bool MetadataStorage::get_cluster_for_cluster_name(
    const std::string &name, Cluster_metadata *out_cluster,
    bool allow_invalidated) const {
  std::string domain_name;
  std::string cluster_name;
  parse_fully_qualified_cluster_name(name, &domain_name, nullptr,
                                     &cluster_name);
  if (domain_name.empty()) {
    domain_name = k_default_domain_name;
  }

  auto result = execute_sqlf(
      get_cluster_query(real_version()) + " WHERE c.cluster_name = ?",
      cluster_name);
  if (auto row = result->fetch_one_named()) {
    if (allow_invalidated || row.get_int("invalidated", 0) == 0)
      *out_cluster = unserialize_cluster_metadata(row, m_md_version);
    else
      throw shcore::Exception("Cluster '" + name + "' is invalidated",
                              SHERR_DBA_ASYNC_MEMBER_INVALIDATED);
    return true;
  }

  return false;
}

/**
 * Create MD schema record for a newly created cluster.
 *
 * GR, cluster and inter-cluster topology validations are assumed to have
 * already been done before.
 *
 * If we're creating a replicated cluster, then the metadata server session
 * must be to the current primary cluster.
 */
Cluster_id MetadataStorage::create_cluster_record(Cluster_impl *cluster,
                                                  bool adopted, bool recreate) {
  Cluster_id cluster_id;

  try {
    if (!recreate) {
      cluster_id = m_md_server->generate_uuid();
    } else {
      cluster_id = cluster->get_id();
    }

    auto result = execute_sqlf(
        "INSERT INTO mysql_innodb_cluster_metadata.clusters "
        "(cluster_id, cluster_name, description,"
        " cluster_type, primary_mode, attributes)"
        " VALUES (?, ?, ?, 'gr', ?,"
        " JSON_OBJECT('adopted', ?,"
        "      'group_replication_group_name', ?))",
        cluster_id, cluster->cluster_name(), cluster->get_description(),
        (cluster->get_cluster_topology_type() ==
                 mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY
             ? "pm"
             : "mm"),
        adopted, cluster->get_group_name());
  } catch (const shcore::Exception &e) {
    if (e.code() == ER_DUP_ENTRY) {
      log_info("DBA: A Cluster with the name '%s' already exists: %s",
               cluster->get_name().c_str(), e.format().c_str());

      throw shcore::Exception::argument_error("A Cluster with the name '" +
                                              cluster->get_name() +
                                              "' already exists.");
    }
    throw;
  }

  cluster->set_id(cluster_id);

  return cluster_id;
}

Instance_id MetadataStorage::insert_instance(const Instance_metadata &instance,
                                             Transaction_undo *undo) {
  auto addresses = ("'mysqlClassic', ?"_sql << instance.endpoint).str();
  if (!instance.xendpoint.empty())
    addresses += (", 'mysqlX', ?"_sql << instance.xendpoint).str();
  if (!instance.grendpoint.empty())
    addresses += (", 'grLocal', ?"_sql << instance.grendpoint).str();

  auto attributes = ("'server_id', ?"_sql << instance.server_id).str();
  if (!instance.cert_subject.empty())
    attributes += ("'cert_subject', ?"_sql << instance.cert_subject).str();

  shcore::sqlstring query;

  if (supports_read_replicas()) {
    query = shcore::sqlstring(
        "INSERT INTO mysql_innodb_cluster_metadata.instances "
        "(cluster_id, address, mysql_server_uuid, "
        "instance_name, addresses, attributes, instance_type) "
        "VALUES (?, ?, ?, ?, json_object(" +
            addresses + "), json_object(" + attributes + "), ?)",
        0);
  } else {
    query = shcore::sqlstring(
        "INSERT INTO mysql_innodb_cluster_metadata.instances "
        "(cluster_id, address, mysql_server_uuid, "
        "instance_name, addresses, attributes) "
        "VALUES (?, ?, ?, ?, json_object(" +
            addresses + "), json_object(" + attributes + "))",
        0);
  }

  query << instance.cluster_id;
  query << instance.address;
  query << instance.uuid;
  query << instance.label;
  if (supports_read_replicas()) query << to_string(instance.instance_type);
  query.done();

  if (undo) {
    auto undo_query = shcore::sqlstring(
        "DELETE FROM mysql_innodb_cluster_metadata.instances "
        "WHERE mysql_server_uuid = ?",
        0);
    undo_query << instance.uuid;
    undo_query.done();

    undo->add_sql(undo_query.str());
  }

  auto result = execute_sql(query);

  return result->get_auto_increment_value();
}

void MetadataStorage::update_instance(const Instance_metadata &instance) {
  auto addresses = ("'mysqlClassic', ?"_sql << instance.endpoint).str();
  if (!instance.xendpoint.empty())
    addresses += (", 'mysqlX', ?"_sql << instance.xendpoint).str();
  if (!instance.grendpoint.empty())
    addresses += (", 'grLocal', ?"_sql << instance.grendpoint).str();

  auto attributes = ("'server_id', ?"_sql << instance.server_id).str();
  if (!instance.cert_subject.empty())
    attributes += ("'cert_subject', ?"_sql << instance.cert_subject).str();

  bool lookup_by_address = instance.id == 0 || instance.cluster_id.empty();

  shcore::sqlstring query;
  query = shcore::sqlstring(
      "UPDATE mysql_innodb_cluster_metadata.instances "
      "SET cluster_id = ?, address = ?, mysql_server_uuid = ?, " +
          std::string(instance.label.empty() ? "" : "instance_name = ?, ") +
          "addresses = json_merge_patch(addresses, json_object(" + addresses +
          ")), attributes = json_merge_patch(attributes, json_object(" +
          attributes + ")) WHERE " +
          (lookup_by_address ? "address = ?"
                             : "cluster_id = ? AND instance_id = ?"),
      0);

  query << instance.cluster_id;
  query << instance.address;
  query << instance.uuid;

  if (!instance.label.empty()) {
    query << instance.label;
  }
  if (lookup_by_address) {
    query << instance.address;
  } else {
    query << instance.cluster_id;
    query << instance.id;
  }
  query.done();

  execute_sql(query);
}

bool MetadataStorage::query_instance_attribute(std::string_view uuid,
                                               std::string_view attribute,
                                               shcore::Value *out_value) const {
  auto stmt = shcore::str_format(
      "SELECT attributes->'$.%.*s' FROM "
      "mysql_innodb_cluster_metadata.instances WHERE mysql_server_uuid = ?",
      static_cast<int>(attribute.length()), attribute.data());

  auto result = execute_sql(shcore::sqlstring(stmt, 0) << uuid);

  if (auto row = result->fetch_one(); row) {
    if (!row->is_null(0)) {
      *out_value = shcore::Value::parse(row->get_as_string(0));
      return true;
    }
  }

  return false;
}

void MetadataStorage::remove_instance_attribute(std::string_view uuid,
                                                std::string_view attribute) {
  auto stmt = shcore::str_format(
      "UPDATE mysql_innodb_cluster_metadata.instances SET attributes = "
      "json_remove(attributes, '$.%.*s') WHERE mysql_server_uuid = ?",
      static_cast<int>(attribute.length()), attribute.data());

  shcore::sqlstring query(stmt, 0);
  query << uuid;
  query.done();

  execute_sql(query);
}

void MetadataStorage::update_instance_attribute(std::string_view uuid,
                                                std::string_view attribute,
                                                const shcore::Value &value,
                                                bool store_null,
                                                Transaction_undo *undo) {
  if (undo) {
    undo->add_snapshot_for_update(
        "mysql_innodb_cluster_metadata.instances", "attributes",
        *get_md_server(), shcore::sqlformat("mysql_server_uuid = ?", uuid));
  }

  if (value) {
    auto stmt = shcore::str_format(
        "UPDATE mysql_innodb_cluster_metadata.instances SET attributes = "
        "json_set(attributes, '$.%.*s', CAST(? as JSON)) WHERE "
        "mysql_server_uuid = ?",
        static_cast<int>(attribute.length()), attribute.data());

    shcore::sqlstring query(stmt, 0);
    query << value.repr() << uuid;
    query.done();

    execute_sql(query);
    return;
  }

  // value is null

  if (store_null) {
    auto stmt = shcore::str_format(
        "UPDATE mysql_innodb_cluster_metadata.instances SET attributes = "
        "json_set(attributes, '$.%.*s', CAST(NULL as JSON)) WHERE "
        "mysql_server_uuid = ?",
        static_cast<int>(attribute.length()), attribute.data());

    shcore::sqlstring query(stmt, 0);
    query << uuid;
    query.done();

    execute_sql(query);
    return;
  }

  auto stmt = shcore::str_format(
      "UPDATE mysql_innodb_cluster_metadata.instances SET attributes = "
      "json_remove(attributes, '$.%.*s') WHERE mysql_server_uuid = ?",
      static_cast<int>(attribute.length()), attribute.data());

  shcore::sqlstring query(stmt, 0);
  query << uuid;
  query.done();

  execute_sql(query);
}

void MetadataStorage::update_instance_addresses(std::string_view uuid,
                                                std::string_view address,
                                                const shcore::Value &value,
                                                Transaction_undo *undo) {
  if (undo) {
    undo->add_snapshot_for_update(
        "mysql_innodb_cluster_metadata.instances", "addresses",
        *get_md_server(), shcore::sqlformat("mysql_server_uuid = ?", uuid));
  }

  if (value) {
    auto stmt = shcore::str_format(
        "UPDATE mysql_innodb_cluster_metadata.instances SET addresses = "
        "json_set(addresses, '$.%.*s', CAST(? as JSON)) WHERE "
        "mysql_server_uuid = ?",
        static_cast<int>(address.length()), address.data());

    shcore::sqlstring query(stmt, 0);
    query << value.repr() << uuid;
    query.done();

    execute_sql(query);
  } else {
    auto stmt = shcore::str_format(
        "UPDATE mysql_innodb_cluster_metadata.instances SET addresses = "
        "json_remove(addresses, '$.%.*s') WHERE mysql_server_uuid = ?",
        static_cast<int>(address.length()), address.data());

    shcore::sqlstring query(stmt, 0);
    query << uuid;
    query.done();

    execute_sql(query);
  }
}

void MetadataStorage::set_instance_tag(const std::string &uuid,
                                       const std::string &tagname,
                                       const shcore::Value &value) {
  // XXX add validation for tagname chars
  set_table_tag("instances", "mysql_server_uuid", uuid, tagname, value);
}

void MetadataStorage::set_cluster_tag(const std::string &uuid,
                                      const std::string &tagname,
                                      const shcore::Value &value) {
  set_table_tag("clusters", "cluster_id", uuid, tagname, value);
}

void MetadataStorage::set_router_tag(Cluster_type type,
                                     const std::string &cluster_id,
                                     const std::string &router,
                                     const std::string &tagname,
                                     const shcore::Value &value) {
  shcore::Value curtag;
  if (router.empty()) {
    curtag = get_global_routing_option(type, cluster_id, "tags");
  } else {
    curtag = get_routing_option(type, cluster_id, router, "tags");
  }

  shcore::Dictionary_t tags;
  if (curtag && curtag.get_type() == shcore::Map) {
    tags = curtag.as_map();
    tags->set(tagname, value);
  } else {
    tags = shcore::make_dict(tagname, value);
  }

  if (router.empty()) {
    set_global_routing_option(type, cluster_id, "tags", shcore::Value(tags));
  } else {
    set_routing_option(type, router, cluster_id, "tags", shcore::Value(tags));
  }
}

void MetadataStorage::set_table_tag(std::string_view tablename,
                                    std::string_view uuid_column_name,
                                    std::string_view uuid,
                                    std::string_view tagname,
                                    const shcore::Value &value) {
  if (value) {
    shcore::sqlstring query(
        shcore::str_format(
            "UPDATE mysql_innodb_cluster_metadata.%.*s"
            " SET attributes = JSON_MERGE_PATCH(attributes,"
            " JSON_OBJECT('tags', JSON_OBJECT(?, CAST(? as JSON))))"
            " WHERE %.*s = ?",
            static_cast<int>(tablename.size()), tablename.data(),
            static_cast<int>(uuid_column_name.size()), uuid_column_name.data()),
        0);
    query << tagname << value.repr() << uuid;

    query.done();
    execute_sql(query);
  } else {
    auto query = shcore::str_format(
        "UPDATE mysql_innodb_cluster_metadata.%.*s SET attributes"
        " = JSON_REMOVE(attributes, '$.tags.%.*s')  WHERE %.*s = ?",
        static_cast<int>(tablename.size()), tablename.data(),
        static_cast<int>(tagname.size()), tagname.data(),
        static_cast<int>(uuid_column_name.size()), uuid_column_name.data());
    execute_sqlf(query, uuid);
  }
}

namespace {
std::string repl_account_user_key(Cluster_type type,
                                  Replica_type replica_type) {
  if (type == Cluster_type::GROUP_REPLICATION) {
    if (replica_type == Replica_type::GROUP_MEMBER) {
      return "$.recoveryAccountUser";
    } else if (replica_type == Replica_type::READ_REPLICA) {
      return "$.readReplicaReplicationAccountUser";
    }
  }
  return "$.replicationAccountUser";
}

std::string repl_account_host_key(Cluster_type type,
                                  Replica_type replica_type) {
  if (type == Cluster_type::GROUP_REPLICATION) {
    if (replica_type == Replica_type::GROUP_MEMBER) {
      return "$.recoveryAccountHost";
    } else if (replica_type == Replica_type::READ_REPLICA) {
      return "$.readReplicaReplicationAccountHost";
    }
  }
  return "$.replicationAccountHost";
}
}  // namespace

void MetadataStorage::update_instance_repl_account(
    std::string_view instance_uuid, Cluster_type type,
    Replica_type replica_type, std::string_view recovery_account_user,
    std::string_view recovery_account_host, Transaction_undo *undo) const {
  shcore::sqlstring query;

  if (undo)
    undo->add_snapshot_for_update(
        "mysql_innodb_cluster_metadata.instances", "attributes",
        *get_md_server(),
        shcore::sqlformat("mysql_server_uuid = ?", instance_uuid));

  if (!recovery_account_user.empty()) {
    query =
        "UPDATE mysql_innodb_cluster_metadata.instances"
        " SET attributes = json_set(COALESCE(attributes, '{}'),"
        "  ?, ?,  ?, ?)"
        " WHERE mysql_server_uuid = ?"_sql;
    query << repl_account_user_key(type, replica_type);
    query << recovery_account_user;
    query << repl_account_host_key(type, replica_type);
    query << recovery_account_host;
    query << instance_uuid;
    query.done();
  } else {
    // if recovery_account user is empty, clear existing recovery attributes
    // of the instance from the metadata.
    query =
        "UPDATE mysql_innodb_cluster_metadata.instances"
        " SET attributes = json_remove(attributes, ?, ?)"
        " WHERE mysql_server_uuid = ?"_sql;
    query << repl_account_user_key(type, replica_type);
    query << repl_account_host_key(type, replica_type);
    query << instance_uuid;
    query.done();
  }
  execute_sql(query);
}

void MetadataStorage::update_instance_uuid(Cluster_id cluster_id,
                                           Instance_id instance_id,
                                           std::string_view instance_uuid) {
  auto query =
      "UPDATE mysql_innodb_cluster_metadata.instances SET mysql_server_uuid = "
      "? WHERE (cluster_id = ?) AND (instance_id = ?);"_sql
      << instance_uuid << cluster_id << instance_id;
  query.done();

  execute_sql(query);
}

std::pair<std::string, std::string> MetadataStorage::get_instance_repl_account(
    const std::string &instance_uuid, Cluster_type type,
    Replica_type replica_type) {
  shcore::sqlstring query = shcore::sqlstring{
      "SELECT (attributes->>?) as recovery_user,"
      " (attributes->>?) as recovery_host"
      " FROM mysql_innodb_cluster_metadata.instances "
      " WHERE mysql_server_uuid = ?",
      0};
  query << repl_account_user_key(type, replica_type);
  query << repl_account_host_key(type, replica_type);
  query << instance_uuid;
  query.done();
  std::string recovery_user, recovery_host;

  auto res = execute_sql(query);
  auto row = res->fetch_one();
  if (row) {
    recovery_user = row->get_string(0, "");
    recovery_host = row->get_string(1, "");
  }
  return std::make_pair(recovery_user, recovery_host);
}

std::string MetadataStorage::get_instance_repl_account_user(
    std::string_view instance_uuid, Cluster_type type,
    Replica_type replica_type) {
  auto query =
      "SELECT (attributes->>?) "
      " FROM mysql_innodb_cluster_metadata.instances "
      " WHERE mysql_server_uuid = ?"_sql;

  query << repl_account_user_key(type, replica_type) << instance_uuid;
  query.done();

  auto res = execute_sql(query);

  if (auto row = res->fetch_one(); row) {
    return row->get_string(0, "");
  }

  return {};
}

void MetadataStorage::update_cluster_repl_account(
    const Cluster_id &cluster_id, const std::string &repl_account_user,
    const std::string &repl_account_host, Transaction_undo *undo) {
  shcore::sqlstring query;

  if (undo)
    undo->add_snapshot_for_update(
        "mysql_innodb_cluster_metadata.clusters", "attributes",
        *get_md_server(), shcore::sqlformat("cluster_id = ?", cluster_id));

  if (!repl_account_user.empty()) {
    query = shcore::sqlstring(
        "UPDATE mysql_innodb_cluster_metadata.clusters"
        " SET attributes = json_set(COALESCE(attributes, '{}'),"
        "  '$.replicationAccountUser', ?,"
        "  '$.replicationAccountHost', ?)"
        " WHERE cluster_id = ?",
        0);
    query << repl_account_user;
    query << repl_account_host;
  } else {
    // if repl_account user is empty, clear existing recovery attributes
    // of the instance from the metadata.
    query = shcore::sqlstring(
        "UPDATE mysql_innodb_cluster_metadata.clusters"
        " SET attributes = json_remove(attributes,"
        "  '$.replicationAccountUser', '$.replicationAccountHost')"
        " WHERE cluster_id = ?",
        0);
  }

  query << cluster_id;
  query.done();

  execute_sql(query);
}

void MetadataStorage::update_read_replica_repl_account(
    const std::string &instance_uuid, const std::string &repl_account_user,
    const std::string &repl_account_host, Transaction_undo *undo) {
  shcore::sqlstring query;

  if (undo) {
    undo->add_snapshot_for_update(
        "mysql_innodb_cluster_metadata.instances", "attributes",
        *get_md_server(),
        shcore::sqlformat("mysql_server_uuid = ?", instance_uuid));
  }

  if (!repl_account_user.empty()) {
    query = shcore::sqlstring(
        "UPDATE mysql_innodb_cluster_metadata.instances"
        " SET attributes = json_set(COALESCE(attributes, '{}'),"
        " '$.readReplicaReplicationAccountUser', ?,"
        " '$.readReplicaReplicationAccountHost', ?)"
        " WHERE mysql_server_uuid = ?",
        0);
    query << repl_account_user;
    query << repl_account_host;
    query << instance_uuid;
    query.done();
  } else {
    // if repl_account_user user is empty, clear existing recovery attributes
    // of the instance from the metadata.
    query = shcore::sqlstring(
        "UPDATE mysql_innodb_cluster_metadata.instances"
        " SET attributes = json_remove(attributes, "
        " '$.readReplicaReplicationAccountUser', "
        " '$.readReplicaReplicationAccountHost')"
        " WHERE mysql_server_uuid = ?",
        0);
    query << instance_uuid;
    query.done();
  }
  execute_sql(query);
}

std::pair<std::string, std::string> MetadataStorage::get_cluster_repl_account(
    const Cluster_id &cluster_id) const {
  shcore::sqlstring query = shcore::sqlstring{
      "SELECT (attributes->>'$.replicationAccountUser') as replication_user,"
      " (attributes->>'$.replicationAccountHost') as replication_host"
      " FROM mysql_innodb_cluster_metadata.clusters "
      " WHERE cluster_id = ?",
      0};
  query << cluster_id;
  query.done();
  std::string recovery_user, recovery_host;

  auto res = execute_sql(query);
  auto row = res->fetch_one();
  if (row) {
    recovery_user = row->get_string(0, "");
    recovery_host = row->get_string(1, "");
  }
  return std::make_pair(recovery_user, recovery_host);
}

std::string MetadataStorage::get_cluster_repl_account_user(
    const Cluster_id &cluster_id) const {
  auto query =
      "SELECT (attributes->>'$.replicationAccountUser') as replication_user "
      " FROM mysql_innodb_cluster_metadata.clusters "
      " WHERE cluster_id = ?"_sql;

  query << cluster_id;
  query.done();

  auto res = execute_sql(query);
  if (auto row = res->fetch_one(); row) {
    return row->get_string(0, std::string{""});
  }

  return {};
}

std::pair<std::string, std::string>
MetadataStorage::get_read_replica_repl_account(
    const std::string &instance_uuid) const {
  shcore::sqlstring query = shcore::sqlstring{
      "SELECT (attributes->>'$.readReplicaReplicationAccountUser') as "
      "replication_user,"
      " (attributes->>'$.readReplicaReplicationAccountHost') as "
      "replication_host"
      " FROM mysql_innodb_cluster_metadata.instances "
      " WHERE mysql_server_uuid = ?",
      0};
  query << instance_uuid;
  query.done();
  std::string replication_user, replication_host;

  auto res = execute_sql(query);
  auto row = res->fetch_one();
  if (row) {
    replication_user = row->get_string(0, "");
    replication_host = row->get_string(1, "");
  }
  return std::make_pair(replication_user, replication_host);
}

std::map<std::string, std::string>
MetadataStorage::get_instances_with_recovery_accounts(
    const Cluster_id &cluster_id) const {
  shcore::sqlstring query(
      "SELECT mysql_server_uuid, IFNULL("
      "attributes->>'$.recoveryAccountUser','') FROM "
      "mysql_innodb_cluster_metadata.instances "
      "WHERE cluster_id = ?",
      0);
  query << cluster_id;
  query.done();
  auto result = execute_sql(query);
  std::map<std::string, std::string> ret_val;
  while (auto row = result->fetch_one()) {
    ret_val.insert({row->get_string(0), row->get_string(1)});
  }
  return ret_val;
}

int MetadataStorage::count_recovery_account_uses(
    const std::string &recovery_account_user, bool clusterset_account) const {
  shcore::sqlstring query;

  if (!recovery_account_user.empty()) {
    if (!clusterset_account) {
      query = shcore::sqlstring(
          "SELECT COUNT(*) as count FROM "
          "mysql_innodb_cluster_metadata.instances "
          "WHERE attributes->'$.recoveryAccountUser' = ?",
          0);
    } else {
      query = shcore::sqlstring(
          "SELECT COUNT(*) as count FROM "
          "mysql_innodb_cluster_metadata.clusters "
          "WHERE attributes->'$.replicationAccountUser' = ?",
          0);
    }

    query << recovery_account_user;
    query.done();

    auto result = execute_sql(query);
    auto row = result->fetch_one();
    int count = 0;

    if (row) {
      count = row->get_int(0);
    }

    return count;
  }

  return 0;
}

std::vector<std::string> MetadataStorage::get_recovery_account_users() {
  auto result = execute_sql(
      "SELECT attributes->>'$.recoveryAccountUser' FROM "
      "mysql_innodb_cluster_metadata.instances");

  std::vector<std::string> users;
  while (auto row = result->fetch_one())
    if (!row->is_null(0)) users.push_back(row->get_string(0));

  return users;
}

size_t MetadataStorage::iterate_recovery_account(
    const std::function<bool(uint32_t, std::string)> &cb) {
  auto query =
      "SELECT CAST(attributes->>'$.server_id' AS UNSIGNED), "
      "attributes->>'$.recoveryAccountUser' FROM "
      "mysql_innodb_cluster_metadata.instances WHERE "
      "(COALESCE(CAST(attributes->>'$.server_id' AS UNSIGNED), 0) > 0) AND "
      "(attributes->>'$.recoveryAccountUser' IS NOT NULL)"_sql;

  auto result = execute_sql(query);

  size_t num_accounts{0};
  while (const auto row = result->fetch_one()) {
    num_accounts++;
    if (!cb(static_cast<uint32_t>(row->get_int(0)), row->get_string(1))) break;
  }

  return num_accounts;
}

void MetadataStorage::remove_instance(std::string_view instance_address,
                                      Transaction_undo *undo) {
  // Remove the instance
  auto query = ("DELETE FROM mysql_innodb_cluster_metadata.instances "
                "WHERE LOWER(addresses->>'$.mysqlClassic') = LOWER(?)"_sql
                << instance_address)
                   .str();

  if (undo) {
    undo->add_snapshot_for_delete(
        "mysql_innodb_cluster_metadata.instances", *get_md_server(),
        shcore::sqlformat("LOWER(addresses->>'$.mysqlClassic') = LOWER(?)",
                          instance_address));
  }

  execute_sql(query);
}

void MetadataStorage::drop_cluster(const std::string &cluster_name,
                                   Transaction_undo *undo) {
  auto get_cluster_id = [this](const std::string &full_cluster_name) {
    Cluster_id cluster_id;
    shcore::sqlstring query;

    std::string domain;
    std::string cluster;
    parse_fully_qualified_cluster_name(full_cluster_name, &domain, nullptr,
                                       &cluster);
    if (domain.empty()) {
      domain = k_default_domain_name;
    }

    // Get the Cluster ID
    query = shcore::sqlstring(
        "SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters "
        "WHERE cluster_name = ?",
        0);
    query << cluster;
    query.done();

    auto result = execute_sql(query);
    auto row = result->fetch_one();
    if (row) {
      cluster_id = row->get_string(0);
    } else {
      throw shcore::Exception::argument_error("The cluster with the name '" +
                                              full_cluster_name +
                                              "' does not exist.");
    }
    if (result->fetch_one()) {
      throw shcore::Exception::argument_error(
          "Ambiguous cluster name specification '" + full_cluster_name +
          "': please use the fully qualified cluster name");
    }
    return cluster_id;
  };

  // It exists, so let's get the cluster_id and move on
  Cluster_id cluster_id = get_cluster_id(cluster_name);

  if (undo)
    undo->add_snapshot_for_delete(
        "mysql_innodb_cluster_metadata.instances", *get_md_server(),
        shcore::sqlformat("cluster_id = ?", cluster_id));
  execute_sqlf(
      "DELETE FROM mysql_innodb_cluster_metadata.instances "
      "WHERE cluster_id = ?",
      cluster_id);

  // Remove the cluster
  if (undo)
    undo->add_snapshot_for_delete(
        "mysql_innodb_cluster_metadata.clusters", *get_md_server(),
        shcore::sqlformat("cluster_id = ?", cluster_id));
  execute_sqlf(
      "DELETE from mysql_innodb_cluster_metadata.clusters "
      "WHERE cluster_id = ?",
      cluster_id);
}

void MetadataStorage::update_cluster_name(const Cluster_id &cluster_id,
                                          const std::string &new_cluster_name) {
  shcore::sqlstring query;

  query = shcore::sqlstring(
      "UPDATE mysql_innodb_cluster_metadata.clusters SET "
      "cluster_name = ? WHERE cluster_id = ?",
      0);
  query << new_cluster_name;
  query << cluster_id;
  query.done();

  execute_sql(query);
}

/**
 * Count the number of instances in a cluster.
 *
 * @param rs_id Integer with the ID of the target cluster.
 *
 * @return An integer with the number of instances in the cluster.
 */
size_t MetadataStorage::get_cluster_size(const Cluster_id &cluster_id) const {
  shcore::sqlstring query;
  // TODO update
  query = shcore::sqlstring(get_cluster_size_query(real_version()), 0);
  query << cluster_id;
  query.done();

  auto result = execute_sql(query);

  auto row = result->fetch_one();
  size_t count = 0;
  if (row) {
    count = row->get_int(0);
  }
  return count;
}

bool MetadataStorage::is_instance_on_cluster(const Cluster_id &cluster_id,
                                             const std::string &address) {
  shcore::sqlstring query;

  query = shcore::sqlstring(get_instance_in_cluster_query(real_version()), 0);
  query << cluster_id;
  query << address;
  query.done();

  auto result = execute_sql(query);

  auto row = result->fetch_one();
  size_t count = 0;
  if (row) {
    count = row->get_int(0);
  }
  return count == 1;
}

bool MetadataStorage::is_instance_label_unique(const Cluster_id &cluster_id,
                                               const std::string &label) const {
  shcore::sqlstring query = shcore::sqlstring{
      "SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances "
      "WHERE cluster_id = ? AND instance_name = ?",
      0};
  query << cluster_id;
  query << label;
  query.done();

  return 0 == execute_sql(query)->fetch_one_or_throw()->get_int(0);
}

void MetadataStorage::set_instance_label(const Cluster_id &cluster_id,
                                         const std::string &label,
                                         const std::string &new_label) {
  shcore::sqlstring query;

  // Check if the label exists
  if (is_instance_label_unique(cluster_id, label)) {
    throw shcore::Exception::logic_error("The instance with the label '" +
                                         label + "' does not exist.");
  } else {
    query = shcore::sqlstring(
        "UPDATE mysql_innodb_cluster_metadata.instances SET "
        "instance_name = ? WHERE cluster_id = ? AND instance_name = ?",
        0);
    query << new_label;
    query << cluster_id;
    query << label;
    query.done();

    execute_sql(query);
  }
}

Instance_metadata MetadataStorage::unserialize_instance(
    const mysqlshdk::db::Row_ref_by_name &row, Version *mdver_in) const {
  Instance_metadata instance;

  // If no input version is provided it will use the version attribute on this
  // class to determine the format the unserialization is done
  if (!mdver_in) mdver_in = &m_md_version;

  if (mdver_in->get_major() == 1) {
    instance.cluster_id = std::to_string(row.get_uint("cluster_id"));
  } else {
    instance.cluster_id = row.get_string("cluster_id");
    instance.address = row.get_string("address", "");
    instance.master_id = row.get_uint("master_instance_id", 0);
    instance.master_uuid = row.get_string("master_member_id", "");
    instance.primary_master = row.get_string("member_role", "") == "PRIMARY";
    if (row.is_null("member_role") &&
        (!row.has_field("group_name") || row.is_null("group_name"))) {
      instance.invalidated = true;
    }
  }

  instance.id = row.get_uint("instance_id");

  if (row.has_field("group_name")) {
    instance.group_name = row.get_string("group_name", "");
  }

  instance.uuid = row.get_string("mysql_server_uuid", "");
  instance.label = row.get_string("label", "");
  instance.endpoint = row.get_string("endpoint", "");
  instance.xendpoint = row.get_string("xendpoint", "");
  if (row.has_field("grendpoint")) {
    instance.grendpoint = row.get_string("grendpoint", "");
  }

  instance.server_id = row.get_uint("server_id", 0);
  if (row.has_field("cert_subject"))
    instance.cert_subject = row.get_string("cert_subject", 0);

  if (row.has_field("hidden_from_router"))
    instance.hidden_from_router = row.get_uint("hidden_from_router", 0);

  if (row.has_field("tags"))
    instance.tags = shcore::Value::parse(row.get_string("tags", "{}")).as_map();

  if (row.has_field("instance_type")) {
    instance.instance_type =
        to_instance_type(row.get_string("instance_type", ""));
  }

  return instance;
}

std::vector<Instance_metadata> MetadataStorage::get_replica_set_members(
    const Cluster_id &cluster_id, uint64_t *out_view_id) {
  std::vector<Instance_metadata> members;

  std::string alias = "i";
  if (m_md_version.get_major() == 1) alias = "r";

  auto result = execute_sqlf(
      get_instance_query(real_version()) + " WHERE !.cluster_id = ?", alias,
      cluster_id);

  if (out_view_id) *out_view_id = 0;

  while (auto row = result->fetch_one_named()) {
    // NOTE: view_id will be NULL for invalidated members
    if (out_view_id && *out_view_id == 0)
      *out_view_id = row.get_uint("view_id", 0);
    members.push_back(unserialize_instance(row));
  }

  return members;
}

void MetadataStorage::get_replica_set_primary_info(const Cluster_id &cluster_id,
                                                   std::string *out_primary_id,
                                                   uint64_t *out_view_id) {
  auto result = execute_sqlf(
      "SELECT view_id, member_id "
      "FROM  mysql_innodb_cluster_metadata.v2_ar_members "
      "WHERE cluster_id = ? AND member_role = 'PRIMARY'",
      cluster_id);
  auto row = result->fetch_one_named();
  if (row) {
    if (out_view_id) *out_view_id = row.get_uint("view_id");
    if (out_primary_id) *out_primary_id = row.get_string("member_id");
  } else {
    throw shcore::Exception(
        "Metadata information on PRIMARY not found for replicaset (" +
            cluster_id + ")",
        SHERR_DBA_METADATA_INFO_MISSING);
  }
}

std::vector<Instance_metadata> MetadataStorage::get_all_instances(
    Cluster_id cluster_id, bool include_read_replicas) {
  if (real_version() == metadata::kNotInstalled) return {};

  std::string query(get_instance_query(m_real_md_version));

  // If a different schema is provided, uses it
  if (m_md_version_schema != metadata::kMetadataSchemaName)
    query = shcore::str_replace(query, metadata::kMetadataSchemaName,
                                m_md_version_schema);

  bool supports_read_replicas =
      m_real_md_version >= mysqlshdk::utils::Version(2, 2, 0);

  std::shared_ptr<mysqlshdk::db::IResult> result;

  if (cluster_id.empty()) {
    if (!include_read_replicas && supports_read_replicas) {
      result = execute_sql(query + " WHERE i.instance_type <> 'read-replica'");
    } else {
      result = execute_sql(query);
    }

  } else {
    if (m_real_md_version.get_major() == 1) {
      result = execute_sqlf(query + " WHERE r.cluster_id = ?",
                            std::atoi(cluster_id.c_str()));
    } else {
      if (!include_read_replicas && supports_read_replicas) {
        result = execute_sqlf(query +
                                  " WHERE i.cluster_id = ? AND "
                                  "i.instance_type <> 'read-replica'",
                              cluster_id);
      } else {
        result = execute_sqlf(query + " WHERE i.cluster_id = ?", cluster_id);
      }
    }
  }

  std::vector<Instance_metadata> ret_val;

  while (auto row = result->fetch_one_named()) {
    auto instance_md = unserialize_instance(row, &m_real_md_version);

    ret_val.push_back(instance_md);
  }

  return ret_val;
}

constexpr const char *k_replica_set_instances_query =
    "SELECT i.instance_id, i.cluster_id,"
    " am.master_instance_id, am.master_member_id,"
    " am.member_role, am.view_id, "
    " i.label, i.mysql_server_uuid, i.address, i.endpoint, i.xendpoint, "
    " CAST(i.attributes->'$.server_id' AS UNSIGNED) server_id,"
    " IFNULL(CAST(i.attributes->'$.tags._hidden' AS UNSIGNED), false)"
    " hidden_from_router"
    " FROM mysql_innodb_cluster_metadata.v2_instances i"
    " LEFT JOIN mysql_innodb_cluster_metadata.v2_ar_members am"
    "   ON am.instance_id = i.instance_id"
    " WHERE i.cluster_id = ?";

std::vector<Instance_metadata> MetadataStorage::get_replica_set_instances(
    const Cluster_id &rs_id) {
  std::vector<Instance_metadata> ret_val;

  auto result = execute_sqlf(k_replica_set_instances_query, rs_id);

  while (auto row = result->fetch_one_named()) {
    ret_val.push_back(unserialize_instance(row));
  }

  return ret_val;
}

Instance_metadata MetadataStorage::get_instance_by_uuid(
    const std::string &uuid, const Cluster_id &cluster_id) const {
  std::string query =
      get_instance_query(real_version()) + " WHERE i.mysql_server_uuid = ?";

  std::shared_ptr<mysqlshdk::db::IResult> result;

  if (cluster_id.empty()) {
    result = execute_sqlf(query, uuid);
  } else {
    query += " AND i.cluster_id = ?";
    result = execute_sqlf(query, uuid, cluster_id);
  }

  if (auto row = result->fetch_one_named()) {
    return unserialize_instance(row);
  }

  throw shcore::Exception("Metadata for instance " + uuid + " not found",
                          SHERR_DBA_MEMBER_METADATA_MISSING);
}

Instance_metadata MetadataStorage::get_instance_by_address(
    std::string_view instance_address) const {
  auto md_version = real_version();
  auto query(get_instance_query(md_version));

  if (md_version.get_major() == 1)
    query += " WHERE LOWER(i.addresses->>'$.mysqlClassic') = LOWER(?)";
  else
    query += " WHERE i.address = ?";

  auto result = execute_sqlf(query, instance_address);

  if (auto row = result->fetch_one_named()) {
    return unserialize_instance(row);
  }

  throw shcore::Exception(
      shcore::str_format("Metadata for instance %.*s not found",
                         static_cast<int>(instance_address.length()),
                         instance_address.data()),
      SHERR_DBA_MEMBER_METADATA_MISSING);
}

Instance_metadata MetadataStorage::get_instance_by_label(
    std::string_view label) const {
  auto query(get_instance_query(real_version()));
  query += " WHERE i.label = ?";

  auto result = execute_sqlf(query, label);

  if (auto row = result->fetch_one_named()) {
    return unserialize_instance(row);
  }

  throw shcore::Exception(
      shcore::str_format("Metadata for instance with label '%.*s' not found",
                         static_cast<int>(label.length()), label.data()),
      SHERR_DBA_MEMBER_METADATA_MISSING);
}

mysqlshdk::gr::Topology_mode MetadataStorage::get_cluster_topology_mode(
    const Cluster_id &cluster_id) {
  // Execute query to obtain the topology mode from the metadata.
  shcore::sqlstring query =
      shcore::sqlstring{get_topology_mode_query(real_version()), 0};
  query << cluster_id;
  query.done();

  std::string topology_mode =
      execute_sql(query)->fetch_one_or_throw()->get_string(0);

  // Convert topology mode string from metadata to enumeration value.
  if (topology_mode == "pm") {
    return mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY;
  } else if (topology_mode == "mm") {
    return mysqlshdk::gr::Topology_mode::MULTI_PRIMARY;
  } else {
    throw shcore::Exception::metadata_error(
        "Unexpected topology mode found in Metadata: " + topology_mode);
  }
}

void MetadataStorage::update_cluster_topology_mode(
    const Cluster_id &cluster_id,
    const mysqlshdk::gr::Topology_mode &topology_mode) {
  // Convert topology mode to metadata value.
  std::string topology_mode_str;
  if (topology_mode == mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY) {
    topology_mode_str = "pm";
  } else if (topology_mode == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY) {
    topology_mode_str = "mm";
  }

  // Execute query to update topology mode on metadata.
  shcore::sqlstring query = shcore::sqlstring{
      "UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = ?"
      " WHERE cluster_id = ?",
      0};
  query << topology_mode_str;
  query << cluster_id;
  query.done();

  execute_sql(query);
}

// ----------------------------------------------------------------------------

constexpr const char *k_async_view_change_reason_create = "CREATE";
constexpr const char *k_async_view_change_reason_switch_primary =
    "SWITCH_ACTIVE";
constexpr const char *k_async_view_change_reason_force_primary = "FORCE_ACTIVE";

constexpr const char *k_async_view_change_reason_add_instance = "ADD_INSTANCE";
constexpr const char *k_async_view_change_reason_rejoin_instance =
    "REJOIN_INSTANCE";
constexpr const char *k_async_view_change_reason_remove_instance =
    "REMOVE_INSTANCE";

Cluster_id MetadataStorage::create_async_cluster_record(
    Replica_set_impl *cluster, bool adopted) {
  Cluster_id cluster_id;
  try {
    cluster_id = m_md_server->generate_uuid();

    execute_sqlf(
        "INSERT INTO mysql_innodb_cluster_metadata.clusters (cluster_id, "
        "cluster_name, description, cluster_type, primary_mode, attributes) "
        "VALUES (?, ?, ?, ?, ?, JSON_OBJECT('adopted', ?))",
        cluster_id, cluster->cluster_name(), cluster->get_description(), "ar",
        cluster->get_topology_type(), adopted);

  } catch (const shcore::Error &e) {
    if (e.code() == ER_DUP_ENTRY) {
      log_info("Duplicate cluster entry for %s: %s",
               cluster->get_name().c_str(), e.format().c_str());
      throw shcore::Exception::argument_error("A Cluster with the name '" +
                                              cluster->get_name() +
                                              "' already exists.");
    } else {
      throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
    }
  }

  execute_sqlf(
      "INSERT INTO mysql_innodb_cluster_metadata.async_cluster_views ("
      " cluster_id, view_id, topology_type, view_change_reason, "
      "view_change_time, view_change_info, attributes ) VALUES (?, 1, ?, ?, "
      "NOW(6), JSON_OBJECT('user', USER(),   'source', @@server_uuid), '{}')",
      cluster_id, to_string(cluster->get_async_topology_type()),
      k_async_view_change_reason_create);

  cluster->set_id(cluster_id);

  return cluster_id;
}

namespace {
/**
 * Increment failover (leftmost) sub-counter for Metadata view ID.
 *
 * The Metadata view ID is (internally) composed by two sub-counter
 * (16 bits each): one (leftmost) used for failover operations and the other
 * (rightmost) for the remaining operations. This function increments the
 * failover sub-counter (reseting the rightmost counter to 0).
 *
 * Examples:
 *  0x0001 0000 [65536] = inc_view_failover_counter(0x0000 0001 [1])
 *  0x0005 0000 [327680] = inc_view_failover_counter(0x0004 0006 [262150])
 *
 * NOTE: In decimal, this operation is arithmetically equivalent to:
 *       (trunc(view_id / (MAX_UINT16 + 1), 0) + 1) * (MAX_UINT16 + 1)
 * @param counter
 * @return
 */
uint32_t inc_view_failover_counter(uint32_t view_id) {
  uint32_t res = view_id >> 16;
  res += 1;
  res <<= 16;
  return res;
}
}  // namespace

void MetadataStorage::begin_acl_change_record(const Cluster_id &cluster_id,
                                              const char *operation,
                                              uint32_t *out_aclvid,
                                              uint32_t *out_last_aclvid) {
  auto res = execute_sqlf(
      "SELECT MAX(view_id)"
      " FROM mysql_innodb_cluster_metadata.async_cluster_views"
      " WHERE cluster_id = ?",
      cluster_id);
  auto row = res->fetch_one_or_throw();

  uint32_t last_aclvid = row->get_uint(0);

  // The view_id is (internally) composed by 2 sub-counters:
  // - left counter (leftmost 16 bits): incremented by failover operations.
  //   Examples: 0x0001 0000 = inc_view_failover_counter(0x0000 0001)
  //             0x0005 0000 = inc_view_failover_counter(0x0004 0006)
  // - right counter (rightmost 16 bits): incremented by all other operations.
  //   Examples: 0x0000 0002 = 0x0000 0001 + 1
  //             0x0004 0007 = 0x0004 0006 + 1
  // IMPORTANT NOTE: This additional failover sub-counter is required to ensure
  // the view_id is incremented even if the previous view information was lost
  // (not replicated to any slave) due to the primary failure, avoiding the
  // same view_id value to be reused (by incrementing an outdated value).
  uint32_t aclvid;
  if (operation == k_async_view_change_reason_force_primary) {
    // Increment leftmost counter for failover.
    aclvid = inc_view_failover_counter(last_aclvid);
  } else {
    // Increment rightmost counter for other operations.
    aclvid = last_aclvid + 1;
  }

  log_debug("Updating metadata for async cluster %s view %s,%s", operation,
            cluster_id.c_str(), std::to_string(aclvid).c_str());

  execute_sqlf(
      "INSERT INTO mysql_innodb_cluster_metadata.async_cluster_views"
      " (cluster_id, view_id, topology_type, view_change_reason, "
      "view_change_time, view_change_info, attributes) SELECT cluster_id, ?, "
      "topology_type, ?, NOW(6), JSON_OBJECT('user', USER(), 'source', "
      "@@server_uuid), attributes FROM "
      "mysql_innodb_cluster_metadata.async_cluster_views WHERE cluster_id = ? "
      "AND view_id = ?",
      aclvid, operation, cluster_id, last_aclvid);

  *out_aclvid = aclvid;
  *out_last_aclvid = last_aclvid;
}

Instance_id MetadataStorage::record_async_member_added(
    const Instance_metadata &member) {
  Instance_id member_id = insert_instance(member);

  uint32_t aclvid;
  uint32_t last_aclvid;

  begin_acl_change_record(member.cluster_id,
                          k_async_view_change_reason_add_instance, &aclvid,
                          &last_aclvid);

  // copy the current member list
  execute_sqlf(
      "INSERT INTO mysql_innodb_cluster_metadata.async_cluster_members"
      " (cluster_id, view_id, instance_id, master_instance_id, "
      "   primary_master, attributes)"
      " SELECT cluster_id, ?, instance_id, master_instance_id, "
      "   primary_master, attributes"
      " FROM mysql_innodb_cluster_metadata.async_cluster_members"
      " WHERE cluster_id = ? AND view_id = ?",
      aclvid, member.cluster_id, last_aclvid);

  // add the new member
  // we also add some info from the instance to the attributes object, so that
  // we leave behind some information about removed instances
  execute_sqlf(
      "INSERT INTO mysql_innodb_cluster_metadata.async_cluster_members ("
      " cluster_id, view_id, instance_id, master_instance_id, primary_master,"
      " attributes)"
      " VALUES (?, ?, ?, IF(?=0, NULL, ?), ?, "
      "   (SELECT JSON_OBJECT('instance.mysql_server_uuid', mysql_server_uuid,"
      "       'instance.address', address)"
      "     FROM mysql_innodb_cluster_metadata.instances"
      "     WHERE instance_id = ?)"
      " )",
      member.cluster_id, aclvid, member_id, member.master_id, member.master_id,
      member.primary_master, member_id);

  return member_id;
}

void MetadataStorage::record_async_member_rejoined(
    const Instance_metadata &member) {
  uint32_t aclvid;
  uint32_t last_aclvid;
  begin_acl_change_record(member.cluster_id,
                          k_async_view_change_reason_rejoin_instance, &aclvid,
                          &last_aclvid);

  // Copy the current member list excluding the rejoining member
  execute_sqlf(
      "INSERT INTO mysql_innodb_cluster_metadata.async_cluster_members"
      " (cluster_id, view_id, instance_id, master_instance_id, "
      "   primary_master, attributes)"
      " SELECT cluster_id, ?, instance_id, master_instance_id, "
      "   primary_master, attributes"
      " FROM mysql_innodb_cluster_metadata.async_cluster_members"
      " WHERE cluster_id = ? AND view_id = ? AND instance_id <> ?",
      aclvid, member.cluster_id, last_aclvid, member.id);

  // Add the rejoining member
  // we also add some info from the instance to the attributes object, so that
  // we leave behind some information about removed instances
  execute_sqlf(
      "INSERT INTO mysql_innodb_cluster_metadata.async_cluster_members ("
      " cluster_id, view_id, instance_id, master_instance_id, primary_master,"
      " attributes)"
      " VALUES (?, ?, ?, IF(?=0, NULL, ?), ?, "
      "   (SELECT JSON_OBJECT('instance.mysql_server_uuid', mysql_server_uuid,"
      "       'instance.address', address)"
      "     FROM mysql_innodb_cluster_metadata.instances"
      "     WHERE instance_id = ?)"
      " )",
      member.cluster_id, aclvid, member.id, member.master_id, member.master_id,
      member.primary_master, member.id);
}

void MetadataStorage::record_async_member_removed(const Cluster_id &cluster_id,
                                                  Instance_id instance_id) {
  uint32_t aclvid;
  uint32_t last_aclvid;
  begin_acl_change_record(cluster_id,
                          k_async_view_change_reason_remove_instance, &aclvid,
                          &last_aclvid);

  // copy the current member list without the removed one
  execute_sqlf(
      "INSERT INTO mysql_innodb_cluster_metadata.async_cluster_members"
      " (cluster_id, view_id, instance_id, master_instance_id, primary_master,"
      " attributes)"
      " SELECT cluster_id, ?, instance_id, master_instance_id, primary_master,"
      " attributes"
      " FROM mysql_innodb_cluster_metadata.async_cluster_members"
      " WHERE cluster_id = ? AND view_id = ? AND instance_id <> ?",
      aclvid, cluster_id, last_aclvid, instance_id);

  // delete the instance record
  execute_sqlf(
      "DELETE FROM mysql_innodb_cluster_metadata.instances "
      "WHERE instance_id = ?",
      instance_id);
}

void MetadataStorage::record_async_primary_switch(Instance_id new_primary_id) {
  // NO need to acquire a Metadata lock because an exclusive locks is already
  // acquired (with a higher level) on all instances of the replica set for
  // set_primary_instance().

  // In a safe active switch, we copy over the whole view of the cluster
  // at once, at the beginning.
  uint32_t aclvid;
  uint32_t last_aclvid;

  auto res = execute_sqlf(
      "SELECT c.cluster_id, c.async_topology_type"
      " FROM mysql_innodb_cluster_metadata.v2_ar_clusters c"
      " JOIN mysql_innodb_cluster_metadata.v2_ar_members m"
      "   ON c.cluster_id = m.cluster_id"
      " WHERE m.instance_id = ?",
      new_primary_id);
  auto row = res->fetch_one();
  if (row) {
    Cluster_id cluster_id = row->get_string(0);
    std::string topo_s = row->get_string(1);
    Instance_id old_primary_id;

    {
      res = execute_sqlf(
          "SELECT m.instance_id"
          " FROM mysql_innodb_cluster_metadata.v2_ar_members m"
          " WHERE m.cluster_id = ? AND m.member_role = 'PRIMARY'",
          cluster_id);
      row = res->fetch_one();
      if (row) {
        old_primary_id = row->get_uint(0);
      } else {
        throw shcore::Exception("PRIMARY instance not defined in metadata",
                                SHERR_DBA_ASYNC_PRIMARY_UNDEFINED);
      }
    }
    begin_acl_change_record(cluster_id,
                            k_async_view_change_reason_switch_primary, &aclvid,
                            &last_aclvid);

    switch (to_topology_type(topo_s)) {
      case Global_topology_type::SINGLE_PRIMARY_TREE:
        // insert record for the promoted instance
        execute_sqlf(
            "INSERT INTO"
            " mysql_innodb_cluster_metadata.async_cluster_members"
            " (cluster_id, view_id, instance_id, master_instance_id, "
            "   primary_master, attributes)"
            " SELECT cluster_id, ?, instance_id, NULL, 1, attributes"
            " FROM mysql_innodb_cluster_metadata.async_cluster_members"
            " WHERE cluster_id = ? AND view_id = ? AND instance_id = ?",
            aclvid, cluster_id, last_aclvid, new_primary_id);

        // insert record for the demoted instance
        execute_sqlf(
            "INSERT INTO"
            " mysql_innodb_cluster_metadata.async_cluster_members"
            " (cluster_id, view_id, instance_id, master_instance_id, "
            "   primary_master, attributes)"
            " SELECT cluster_id, ?, instance_id, ?, 0, attributes"
            " FROM mysql_innodb_cluster_metadata.async_cluster_members"
            " WHERE cluster_id = ? AND view_id = ? AND instance_id = ?",
            aclvid, new_primary_id, cluster_id, last_aclvid, old_primary_id);

        // update everything else
        execute_sqlf(
            "INSERT INTO "
            "mysql_innodb_cluster_metadata.async_cluster_members"
            " (cluster_id, view_id, instance_id, master_instance_id, "
            "   primary_master, attributes)"
            " SELECT cluster_id, ?, instance_id, ?,"
            "     IF(instance_id = ?, 1, 0), attributes"
            " FROM mysql_innodb_cluster_metadata.async_cluster_members"
            " WHERE cluster_id = ? AND view_id = ?"
            "   AND instance_id NOT IN (?, ?)",
            aclvid, new_primary_id, new_primary_id, cluster_id, last_aclvid,
            new_primary_id, old_primary_id);
        break;

      case Global_topology_type::NONE:
        // not supposed to hit this
        assert(0);
        throw std::logic_error("Internal error");
    }
  } else {
    log_error("Async cluster for instance %i has metadata errors",
              new_primary_id);
    throw shcore::Exception("Invalid metadata for async cluster found",
                            SHERR_DBA_METADATA_INCONSISTENT);
  }
}

void MetadataStorage::record_async_primary_forced_switch(
    Instance_id new_primary_id, const std::list<Instance_id> &invalidated) {
  // NO need to acquire a Metadata lock because an exclusive locks is already
  // acquired (with a higher level) on all instances of the replica set for
  // set_primary_instance().

  uint32_t aclvid;
  uint32_t last_aclvid;

  auto res = execute_sqlf(
      "SELECT c.cluster_id, c.async_topology_type"
      " FROM mysql_innodb_cluster_metadata.v2_ar_clusters c"
      " JOIN mysql_innodb_cluster_metadata.v2_ar_members m"
      "   ON c.cluster_id = m.cluster_id"
      " WHERE m.instance_id = ?",
      new_primary_id);
  auto row = res->fetch_one();
  if (row) {
    Cluster_id cluster_id = row->get_string(0);
    std::string topo_s = row->get_string(1);
    Instance_id old_primary_id;

    {
      res = execute_sqlf(
          "SELECT m.instance_id"
          " FROM mysql_innodb_cluster_metadata.v2_ar_members m"
          " WHERE m.cluster_id = ? AND m.member_role = 'PRIMARY'",
          cluster_id);
      row = res->fetch_one();
      if (row) {
        old_primary_id = row->get_uint(0);
      } else {
        throw shcore::Exception("PRIMARY instance not defined in metadata",
                                SHERR_DBA_ASYNC_PRIMARY_UNDEFINED);
      }
    }
    begin_acl_change_record(cluster_id,
                            k_async_view_change_reason_force_primary, &aclvid,
                            &last_aclvid);

    std::string invalidated_ids;
    invalidated_ids.append(", ").append(std::to_string(old_primary_id));
    for (auto id : invalidated) {
      invalidated_ids.append(", ").append(std::to_string(id));
    }

    switch (to_topology_type(topo_s)) {
      case Global_topology_type::SINGLE_PRIMARY_TREE:
        // insert record for the promoted instance
        execute_sqlf(
            "INSERT INTO"
            " mysql_innodb_cluster_metadata.async_cluster_members"
            " (cluster_id, view_id, instance_id, master_instance_id, "
            "   primary_master, attributes)"
            " SELECT cluster_id, ?, instance_id, NULL, 1, attributes"
            " FROM mysql_innodb_cluster_metadata.async_cluster_members"
            " WHERE cluster_id = ? AND view_id = ? AND instance_id = ?",
            aclvid, cluster_id, last_aclvid, new_primary_id);

        // update everything else
        execute_sqlf(
            "INSERT INTO"
            " mysql_innodb_cluster_metadata.async_cluster_members"
            " (cluster_id, view_id, instance_id, master_instance_id, "
            "   primary_master, attributes)"
            " SELECT cluster_id, ?, instance_id, ?,"
            "     IF(instance_id = ?, 1, 0), attributes"
            " FROM mysql_innodb_cluster_metadata.async_cluster_members"
            " WHERE cluster_id = ? AND view_id = ?"
            "   AND instance_id NOT IN (?" +
                invalidated_ids + ")",
            aclvid, new_primary_id, new_primary_id, cluster_id, last_aclvid,
            new_primary_id);
        break;

      case Global_topology_type::NONE:
        // not supposed to hit this
        assert(0);
        throw std::logic_error("Internal error");
    }
  } else {
    log_error("Async cluster for instance %i has metadata errors",
              new_primary_id);
    throw shcore::Exception("Invalid metadata for async cluster found",
                            SHERR_DBA_METADATA_INCONSISTENT);
  }
}

std::vector<Router_metadata> MetadataStorage::get_routers(
    const Cluster_id &cluster_id) {
  std::string query(get_router_query(real_version()));

  std::shared_ptr<mysqlshdk::db::IResult> result;
  if (m_md_version.get_major() >= 2 && !cluster_id.empty()) {
    query.append(" WHERE r.cluster_id = ?");
    result = execute_sqlf(query, cluster_id);
  } else {
    result = execute_sqlf(query);
  }

  std::vector<Router_metadata> ret_val;
  while (auto row = result->fetch_one_named()) {
    auto router = unserialize_router(row);
    ret_val.push_back(router);
  }
  return ret_val;
}

std::vector<Router_metadata> MetadataStorage::get_clusterset_routers(
    const Cluster_set_id &cs) {
  std::string query(get_router_query(real_version()));

  query.append(" WHERE r.clusterset_id = ?");
  auto result = execute_sqlf(query, cs);

  std::vector<Router_metadata> ret_val;
  while (auto row = result->fetch_one_named()) {
    auto router = unserialize_router(row);
    ret_val.push_back(router);
  }
  return ret_val;
}

namespace {
template <class TOptions>
std::string router_opt_select_items(std::string_view prefix,
                                    const TOptions &options) {
  std::string ret;
  for (const auto &opt : options) {
    if (opt == k_router_option_target_cluster) {
      ret += ", JSON_QUOTE(IF(";
      ret.append(prefix);
      ret +=
          "->>'$.target_cluster'='primary', 'primary', "
          "(SELECT cluster_name FROM mysql_innodb_cluster_metadata.clusters "
          "c "
          "WHERE c.attributes->>'$.group_replication_group_name' = ";
      ret.append(prefix);
      ret += "->>'$.target_cluster' limit 1))) AS target_cluster";
    } else {
      ret += ", ";
      ret.append(prefix);
      ret += "->'$.";
      ret += opt;
      ret += "' as ";
      ret += opt;
    }
  }
  return ret + " ";
}

Routing_options_metadata fetch_router_options(
    const std::shared_ptr<mysqlshdk::db::IResult> &result,
    const std::map<std::string, shcore::Value> &option_defaults) {
  Routing_options_metadata ret;

  while (auto row = result->fetch_one_named()) {
    std::map<std::string, shcore::Value> rom;
    bool is_global = row.is_null("router_label");

    for (const auto &option : option_defaults) {
      if (!row.is_null(option.first)) {
        rom.emplace(option.first,
                    shcore::Value::parse(row.get_string(option.first)));
      } else {
        // fill-in default values for global options
        if (is_global) rom.emplace(option.first, option.second);
      }
    }
    if (!is_global) {
      ret.routers[row.get_string("router_label")] = std::move(rom);
    } else {
      ret.global = std::move(rom);
    }
  }
  return ret;
}
}  // namespace

Routing_options_metadata MetadataStorage::get_routing_options(
    Cluster_type type, const std::string &id) {
  std::string query;
  std::map<std::string, shcore::Value> option_defaults;

  switch (type) {
    case Cluster_type::REPLICATED_CLUSTER:
      option_defaults = {k_default_clusterset_router_options.begin(),
                         k_default_clusterset_router_options.end()};

      if (installed_version() < mysqlshdk::utils::Version(2, 2)) {
        assert(option_defaults.find(k_router_option_stats_updates_frequency) !=
               option_defaults.end());
        option_defaults[k_router_option_stats_updates_frequency] =
            shcore::Value(0);
      }

      query =
          "SELECT concat(r.address, '::', r.router_name) AS router_label" +
          router_opt_select_items("r.options", k_clusterset_router_options) +
          "FROM mysql_innodb_cluster_metadata.routers AS r UNION SELECT "
          "NULL" +
          router_opt_select_items("cs.router_options",
                                  k_clusterset_router_options) +
          "FROM mysql_innodb_cluster_metadata.clustersets AS cs WHERE "
          "clusterset_id = ?";
      break;
    case Cluster_type::GROUP_REPLICATION:
      option_defaults = {k_default_cluster_router_options.begin(),
                         k_default_cluster_router_options.end()};

      query = "SELECT concat(r.address, '::', r.router_name) AS router_label" +
              router_opt_select_items("r.options", k_cluster_router_options) +
              "FROM mysql_innodb_cluster_metadata.routers AS r UNION SELECT "
              "NULL" +
              router_opt_select_items("c.router_options",
                                      k_cluster_router_options) +
              "FROM mysql_innodb_cluster_metadata.clusters AS c WHERE "
              "cluster_id = ?";
      break;
    case Cluster_type::ASYNC_REPLICATION:
      option_defaults = {k_default_replicaset_router_options.begin(),
                         k_default_replicaset_router_options.end()};

      query =
          "SELECT concat(r.address, '::', r.router_name) AS router_label" +
          router_opt_select_items("r.options", k_replicaset_router_options) +
          "FROM mysql_innodb_cluster_metadata.routers AS r UNION SELECT "
          "NULL" +
          router_opt_select_items("c.router_options",
                                  k_replicaset_router_options) +
          "FROM mysql_innodb_cluster_metadata.clusters AS c WHERE "
          "cluster_id = ?";
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  return fetch_router_options(execute_sqlf(query, id), option_defaults);
}

shcore::Value MetadataStorage::get_router_options(Cluster_type type,
                                                  const std::string &id,
                                                  const std::string &router) {
  std::shared_ptr<mysqlshdk::db::IResult> result;
  auto router_options = shcore::make_dict();

  std::string query = std::string(k_select_router_options);

  switch (type) {
    case Cluster_type::ASYNC_REPLICATION:
    case Cluster_type::GROUP_REPLICATION:
      query += " WHERE cluster_id = ?";
      break;
    case Cluster_type::REPLICATED_CLUSTER:
      query += " WHERE clusterset_id = ?";
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  if (!router.empty()) {
    query +=
        " AND CONCAT(CONVERT(r.address using utf8mb4), '::', r.router_name) = "
        "?";
  }

  if (!router.empty()) {
    result = execute_sqlf(query, id, router);
  } else {
    result = execute_sqlf(query, id);
  }

  auto row = result->fetch_one_named();

  if (!row) {
    return shcore::Value::Null();
  }

  while (row) {
    auto label = row.get_string("router_label");
    auto options = row.get_string("router_configuration");
    auto options_parsed = shcore::Value::parse(options);

    router_options->set(std::move(label),
                        shcore::Value(std::move(options_parsed)));

    row = result->fetch_one_named();
  }

  return shcore::Value(std::move(router_options));
}

mysqlshdk::utils::Version MetadataStorage::get_router_version(
    Cluster_type type, const std::string &id, const std::string &router_name) {
  std::shared_ptr<mysqlshdk::db::IResult> result;
  Version version;

  std::string_view type_id;

  switch (type) {
    case Cluster_type::ASYNC_REPLICATION:
    case Cluster_type::GROUP_REPLICATION:
      type_id = "cluster_id = ?";
      break;
    case Cluster_type::REPLICATED_CLUSTER:
      type_id = "clusterset_id = ?";
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  std::string query =
      "SELECT DISTINCT version FROM mysql_innodb_cluster_metadata.routers r "
      "WHERE concat(r.address, '::', r.router_name) = ? AND ";

  query.append(type_id);

  result = execute_sqlf(query, router_name, id);

  while (auto row = result->fetch_one()) {
    version = mysqlshdk::utils::Version(row->get_string(0));
  }

  if (!version) throw std::logic_error("internal error");

  return version;
}

mysqlshdk::utils::Version
MetadataStorage::get_highest_bootstrapped_router_version(
    Cluster_type type, const std::string &id) {
  std::shared_ptr<mysqlshdk::db::IResult> result;
  Version max_version;

  std::string_view type_id;

  switch (type) {
    case Cluster_type::ASYNC_REPLICATION:
    case Cluster_type::GROUP_REPLICATION:
      type_id = "cluster_id = ?";
      break;
    case Cluster_type::REPLICATED_CLUSTER:
      type_id = "clusterset_id = ?";
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  std::string query =
      "SELECT DISTINCT version FROM mysql_innodb_cluster_metadata.routers "
      "WHERE ";
  query.append(type_id);

  result = execute_sqlf(query, id);

  while (auto row = result->fetch_one()) {
    max_version =
        std::max(max_version, mysqlshdk::utils::Version(row->get_string(0)));
  }

  if (!max_version) return Version(0, 0, 0);

  return max_version;
}

mysqlshdk::utils::Version
MetadataStorage::get_highest_router_configuration_schema_version(
    Cluster_type type, const std::string &id) {
  std::shared_ptr<mysqlshdk::db::IResult> result;
  Version max_version;

  std::string_view type_str, type_id;

  switch (type) {
    case Cluster_type::ASYNC_REPLICATION:
    case Cluster_type::GROUP_REPLICATION:
      type_str = "clusters";
      type_id = "cluster_id = ?";
      break;
    case Cluster_type::REPLICATED_CLUSTER:
      type_str = "clustersets";
      type_id = "clusterset_id = ?";
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  std::string query =
      shcore::sqlformat(std::string(k_select_router_versions), type_str);

  query.append("WHERE ").append(type_id);

  result = execute_sqlf(query, id);

  while (auto row = result->fetch_one()) {
    max_version =
        std::max(max_version, mysqlshdk::utils::Version(row->get_string(0)));
  }

  if (!max_version) return Version(0, 0, 0);

  return max_version;
}

shcore::Value MetadataStorage::get_default_router_options(
    Cluster_type type, const std::string &id,
    const mysqlshdk::utils::Version &max_version) {
  std::shared_ptr<mysqlshdk::db::IResult> result;
  auto default_router_options = shcore::make_dict();

  std::string version_str =
      shcore::str_format("$.\"%s\"", max_version.get_full().c_str());

  std::string query = shcore::sqlformat(
      std::string(k_select_default_router_options), version_str);

  switch (type) {
    case Cluster_type::ASYNC_REPLICATION:
    case Cluster_type::GROUP_REPLICATION:
      query +=
          " FROM mysql_innodb_cluster_metadata.clusters c WHERE cluster_id = "
          "?";
      break;
    case Cluster_type::REPLICATED_CLUSTER:
      query +=
          " FROM mysql_innodb_cluster_metadata.clustersets c WHERE "
          "clusterset_id = ?";
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  result = execute_sqlf(query, id);

  if (auto row = result->fetch_one(); row && !row->is_null(0)) {
    return shcore::Value::parse(row->get_string(0));
  }

  return {};
}

shcore::Value MetadataStorage::get_router_configuration_changes_schema(
    Cluster_type type, const std::string &id,
    const mysqlshdk::utils::Version &max_version) {
  std::shared_ptr<mysqlshdk::db::IResult> result;

  std::string version_str =
      shcore::str_format("$.\"%s\"", max_version.get_full().c_str());

  std::string query = shcore::sqlformat(
      std::string(k_select_default_router_configuration_changes_schema),
      version_str);

  switch (type) {
    case Cluster_type::ASYNC_REPLICATION:
    case Cluster_type::GROUP_REPLICATION:
      query +=
          " FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_id = ?";
      break;
    case Cluster_type::REPLICATED_CLUSTER:
      query +=
          " FROM mysql_innodb_cluster_metadata.clustersets WHERE clusterset_id "
          "= ?";
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  result = execute_sqlf(query, id);

  if (auto row = result->fetch_one(); row && !row->is_null(0)) {
    return shcore::Value::parse(row->get_string(0));
  }

  return shcore::Value::Null();
}

shcore::Value MetadataStorage::get_global_routing_option(
    Cluster_type type, const std::string &cluster_id,
    const std::string &option) {
  std::shared_ptr<mysqlshdk::db::IResult> result;
  switch (type) {
    case Cluster_type::REPLICATED_CLUSTER:
      result = execute_sqlf(
          "SELECT router_options->?"
          " FROM mysql_innodb_cluster_metadata.clustersets"
          " WHERE clusterset_id = ?",
          "$." + option, cluster_id);
      break;
    case Cluster_type::ASYNC_REPLICATION:
    case Cluster_type::GROUP_REPLICATION:
      result = execute_sqlf(
          "SELECT router_options->?"
          " FROM mysql_innodb_cluster_metadata.clusters"
          " WHERE cluster_id = ?",
          "$." + option, cluster_id);
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  if (auto row = result->fetch_one()) {
    if (row->is_null(0)) return {};
    return shcore::Value::parse(row->get_string(0));
  }

  return {};
}

shcore::Value MetadataStorage::get_routing_option(Cluster_type type,
                                                  const std::string &cluster_id,
                                                  const std::string &router,
                                                  const std::string &option) {
  std::shared_ptr<mysqlshdk::db::IResult> result;
  switch (type) {
    case Cluster_type::REPLICATED_CLUSTER:
      result = execute_sqlf(
          "SELECT router_options->?"
          " FROM mysql_innodb_cluster_metadata.v2_cs_router_options"
          " WHERE clusterset_id = ? AND router_label = ?",
          "$." + option, cluster_id, router);
      break;
    case Cluster_type::GROUP_REPLICATION:
    case Cluster_type::ASYNC_REPLICATION:
      result = execute_sqlf(
          R"*(SELECT JSON_EXTRACT(
                      JSON_MERGE_PATCH((SELECT COALESCE(router_options, '{}')
                          FROM mysql_innodb_cluster_metadata.clusters
                          WHERE cluster_id = r.cluster_id),
                        COALESCE(r.options, '{}')), ?) as router_options
            FROM mysql_innodb_cluster_metadata.routers r
            WHERE cluster_id = ?
                AND concat(r.address, '::', r.router_name) = ?)*",
          "$." + option, cluster_id, router);
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  if (auto row = result->fetch_one()) {
    if (row->is_null(0)) return {};
    return shcore::Value::parse(row->get_string(0));
  }

  return {};
}

std::string MetadataStorage::get_cluster_name(
    const std::string &group_replication_group_name) {
  constexpr auto query =
      "SELECT cluster_name from mysql_innodb_cluster_metadata.clusters WHERE "
      "attributes->>'$.group_replication_group_name' = ?";

  auto result = execute_sqlf(query, group_replication_group_name);

  if (auto row = result->fetch_one()) {
    if (!row->is_null(0)) {
      return row->get_as_string(0);
    }
  }

  throw shcore::Exception::logic_error(
      "No cluster found with group_replication_group_name '" +
      group_replication_group_name + "'");
}

std::string MetadataStorage::get_cluster_group_name(
    const std::string &cluster_name) {
  constexpr auto query =
      "SELECT attributes->>'$.group_replication_group_name' as group_name "
      "from "
      "mysql_innodb_cluster_metadata.clusters WHERE "
      "cluster_name = ?";

  auto result = execute_sqlf(query, cluster_name);

  if (auto row = result->fetch_one()) {
    if (!row->is_null(0)) {
      return row->get_as_string(0);
    }
  }

  throw shcore::Exception("No cluster found with name '" + cluster_name + "'",
                          SHERR_DBA_METADATA_MISSING);
}

static void parse_router_definition(const std::string &router_def,
                                    std::string *out_address,
                                    std::string *out_name) {
  auto pos = router_def.find("::");
  if (pos == std::string::npos) {
    *out_address = router_def;
    *out_name = "";  // default is ""
  } else {
    *out_address = router_def.substr(0, pos);
    *out_name = router_def.substr(pos + 2);
  }
}

bool MetadataStorage::remove_router(const std::string &router_def) {
  std::string address;
  std::string name;
  parse_router_definition(router_def, &address, &name);

  // This has to support MD versions 1.0 and 2.x, so that we can remove older
  // router versions while upgrading the MD.
  std::shared_ptr<mysqlshdk::db::IResult> result;
  if (real_version().get_major() == 1) {
    result = execute_sqlf(
        "SELECT router_id FROM mysql_innodb_cluster_metadata.routers r JOIN "
        "mysql_innodb_cluster_metadata.hosts h ON r.host_id = h.host_id WHERE "
        "r.router_name = ? AND LOWER(h.host_name) = LOWER(?)",
        name, address);

    auto row = result->fetch_one();
    if (!row) return false;

    execute_sqlf(
        "DELETE FROM mysql_innodb_cluster_metadata.routers WHERE router_id = ?",
        row->get_int(0));

    return true;
  }

  // For versions of Metadata >= 2.0, Routers running 8.4.0+ will also store
  // the Default Configurations Document in the topology table. If the Router
  // being removed is the last of that version, the corresponding document
  // must be removed too
  result = execute_sqlf(
      "SELECT * FROM mysql_innodb_cluster_metadata.routers r WHERE "
      "r.router_name = ? AND r.address = ?",
      name, address);

  auto row = result->fetch_one_named();
  if (!row) return false;

  std::string topology_id, bootstrap_target_type, router_version_str;
  uint64_t router_id;

  router_version_str = row.get_string("version");
  router_id = row.get_uint("router_id");

  if (row.has_field("attributes") && !row.is_null("attributes")) {
    auto attr = shcore::Value::parse(row.get_string("attributes")).as_map();
    bootstrap_target_type = attr->get_string("bootstrapTargetType");
  }

  if (bootstrap_target_type == "cluster" && row.has_field("cluster_id") &&
      !row.is_null("cluster_id")) {
    topology_id = row.get_string("cluster_id");
  } else if (bootstrap_target_type == "clusterset" &&
             row.has_field("clusterset_id") && !row.is_null("clusterset_id")) {
    topology_id = row.get_string("clusterset_id");
  }

  // Delete the Router
  execute_sqlf(
      "DELETE FROM mysql_innodb_cluster_metadata.routers WHERE "
      "router_id = ?",
      router_id);

  // If we cannot determine the topology type we're done and cannot do anything
  // else so return right away
  if (bootstrap_target_type.empty() || topology_id.empty()) return true;

  auto cluster_type = bootstrap_target_type == "clusterset"
                          ? Cluster_type::REPLICATED_CLUSTER
                          : Cluster_type::GROUP_REPLICATION;

  // Check which is the highest version of Router bootstrapped in this
  // topology
  auto highest_bootstrapped_router_version =
      get_highest_bootstrapped_router_version(cluster_type, topology_id);

  auto highest_router_configuration_document_version =
      get_highest_router_configuration_schema_version(cluster_type,
                                                      topology_id);

  // If there are no configuration documents we're done
  if (!highest_router_configuration_document_version) {
    return true;
  }

  // If the highest version of the configuration document is higher than the
  // Router with highest version bootstrapped in this topology, the document
  // must be removed. Same if there are no more routers in the Metadata.
  if (!highest_bootstrapped_router_version ||
      highest_router_configuration_document_version >
          highest_bootstrapped_router_version) {
    if (cluster_type == Cluster_type::REPLICATED_CLUSTER) {
      std::string query = shcore::str_format(
          "UPDATE mysql_innodb_cluster_metadata.clustersets SET router_options "
          "= JSON_REMOVE(router_options, '$.Configuration.\"%s\"')",
          Version(router_version_str).get_full().c_str());
      execute_sqlf(query);
    } else {
      std::string query = shcore::str_format(
          "UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = "
          "JSON_REMOVE(router_options, '$.Configuration.\"%s\"')",
          Version(router_version_str).get_full().c_str());
      execute_sqlf(query);
    }
  }

  return true;
}

void MetadataStorage::set_target_cluster_for_all_routers(
    const Cluster_id &cluster_id, const std::string &target_cluster) {
  shcore::sqlstring query(
      "UPDATE mysql_innodb_cluster_metadata.routers"
      " SET options = json_set(options, '$.targetCluster', ?)"
      " WHERE cluster_id = ?",
      0);

  query << target_cluster << cluster_id;
  query.done();
  execute_sql(query);
}

void MetadataStorage::migrate_routers_to_clusterset(
    const Cluster_id &cluster_id, const Cluster_set_id &cluster_set_id) {
  shcore::sqlstring query(
      "UPDATE mysql_innodb_cluster_metadata.routers"
      " SET clusterset_id = ?, cluster_id = NULL "
      " WHERE cluster_id = ?",
      0);

  query << cluster_set_id << cluster_id;
  query.done();
  execute_sql(query);
}

void MetadataStorage::migrate_read_only_targets_to_clusterset(
    const Cluster_id &cluster_id, const Cluster_set_id &cluster_set_id) {
  auto result = execute_sql(
      ("SELECT router_options->>'$.read_only_targets' FROM mysql_innodb_cluster_metadata.clusters WHERE cluster_id = ?"_sql
       << cluster_id)
          .str());

  auto row = result->fetch_one();

  std::string read_only_targets;
  if (!row || row->is_null(0)) {
    // If the value is not set for the Cluster, assume the default
    read_only_targets = k_default_router_option_read_only_targets;
  } else {
    read_only_targets = row->get_string(0);
  }

  execute_sql(
      ("UPDATE mysql_innodb_cluster_metadata.clustersets "
       "SET router_options = JSON_SET(router_options, '$.read_only_targets', "
       "?) WHERE clusterset_id = ?"_sql
       << read_only_targets << cluster_set_id)
          .str());
}

std::vector<Router_metadata>
MetadataStorage::get_routers_using_cluster_as_target(
    const std::string &target_cluster_group_name) {
  std::string query(get_router_query(real_version()));

  std::shared_ptr<mysqlshdk::db::IResult> result;

  if (m_md_version.get_major() >= 2 && m_md_version.get_minor() >= 1 &&
      !target_cluster_group_name.empty()) {
    query.append(" WHERE r.options->>'$.target_cluster' = ?");
    result = execute_sqlf(query, target_cluster_group_name);
  } else {
    throw std::logic_error(
        "Operation not support on Metadata schema versions < 2.1.0");
  }

  std::vector<Router_metadata> ret_val;
  while (auto row = result->fetch_one_named()) {
    auto router = unserialize_router(row);
    ret_val.push_back(router);
  }
  return ret_val;
}

shcore::Value MetadataStorage::get_router_info(const std::string &router_name) {
  auto res = execute_sqlf(
      "SELECT attributes FROM mysql_innodb_cluster_metadata.v2_routers "
      " WHERE router_name = ?",
      router_name);
  if (auto row = res->fetch_one()) {
    return shcore::Value::parse(row->get_string(0, ""));
  }

  throw shcore::Exception("Invalid router name '" + router_name + "'",
                          SHERR_DBA_METADATA_MISSING);
}

namespace {
void throw_router_not_found(std::string error) {
  const auto pos = error.find("Router");
  if (pos != std::string::npos) error = error.substr(pos);
  throw shcore::Exception::argument_error(error);
}
}  // namespace

void MetadataStorage::set_global_routing_option(Cluster_type type,
                                                const std::string &cluster_id,
                                                const std::string &option,
                                                const shcore::Value &value) {
  const std::map<std::string, shcore::Value> *option_defaults = nullptr;
  switch (type) {
    case Cluster_type::REPLICATED_CLUSTER:
      option_defaults = &k_default_clusterset_router_options;
      break;
    case Cluster_type::GROUP_REPLICATION:
      option_defaults = &k_default_cluster_router_options;
      break;
    case Cluster_type::ASYNC_REPLICATION:
      option_defaults = &k_default_replicaset_router_options;
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  shcore::Value value_copy = value;
  std::string option_name = option;

  if (value.get_type() == shcore::Null) {
    if ((type == Cluster_type::REPLICATED_CLUSTER) &&
        (option == k_router_option_stats_updates_frequency) &&
        (installed_version() < mysqlshdk::utils::Version(2, 2))) {
      value_copy = shcore::Value(0);
    } else {
      value_copy = option_defaults->at(option);
    }
  }

  if (type == Cluster_type::REPLICATED_CLUSTER) {
    if (value_copy.get_type() == shcore::Null) {
      execute_sqlf(R"*(UPDATE mysql_innodb_cluster_metadata.clustersets
        SET router_options = JSON_REMOVE(router_options, concat('$.', ?))
        WHERE clusterset_id = ?)*",
                   option_name, cluster_id);
    } else {
      execute_sqlf(R"*(UPDATE mysql_innodb_cluster_metadata.clustersets
        SET router_options =
          JSON_SET(IFNULL(router_options, '{}'), concat('$.', ?), CAST(? as JSON))
        WHERE clusterset_id = ?)*",
                   option_name, value_copy.json(false), cluster_id);
    }
  } else {
    if (value_copy.get_type() == shcore::Null) {
      execute_sqlf(R"*(UPDATE mysql_innodb_cluster_metadata.clusters
        SET router_options = JSON_REMOVE(router_options, concat('$.', ?))
        WHERE cluster_id = ?)*",
                   option_name, cluster_id);
    } else {
      execute_sqlf(R"*(UPDATE mysql_innodb_cluster_metadata.clusters
        SET router_options =
          JSON_SET(IFNULL(router_options, '{}'), concat('$.', ?), CAST(? as JSON))
        WHERE cluster_id = ?)*",
                   option_name, value_copy.json(false), cluster_id);
    }
  }
}

void MetadataStorage::set_routing_option(Cluster_type type,
                                         const std::string &router,
                                         const std::string &cluster_id,
                                         const std::string &option,
                                         const shcore::Value &value) {
  std::string option_name = option;

  auto [router_address, router_name] = shcore::str_partition(router, "::");
  auto res = execute_sqlf(
      R"*(SELECT count(*) FROM mysql_innodb_cluster_metadata.routers r
        WHERE (r.clusterset_id = ? or r.cluster_id = ?) and
          r.address = ? and r.router_name = ?)*",
      cluster_id, cluster_id, router_address, router_name);
  if (auto row = res->fetch_one(); row->get_int(0) == 0) {
    throw_router_not_found("Router '" + router +
                           "' is not part of this topology");
  }

  if (value == shcore::Value::Null()) {
    if (type == Cluster_type::REPLICATED_CLUSTER) {
      execute_sqlf(R"*(UPDATE mysql_innodb_cluster_metadata.routers r
        SET r.options = JSON_REMOVE(r.options, concat('$.', ?))
        WHERE r.clusterset_id = ?
          AND r.address = ? AND r.router_name = ?)*",
                   option_name, cluster_id, router_address, router_name);
    } else {
      execute_sqlf(R"*(UPDATE mysql_innodb_cluster_metadata.routers r
        SET r.options = JSON_REMOVE(r.options, concat('$.', ?))
        WHERE r.cluster_id = ?
          AND r.address = ? AND r.router_name = ?)*",
                   option_name, cluster_id, router_address, router_name);
    }
  } else {
    if (type == Cluster_type::REPLICATED_CLUSTER) {
      execute_sqlf(R"*(UPDATE mysql_innodb_cluster_metadata.routers r
        SET r.options =
          JSON_SET(IFNULL(r.options, '{}'), concat('$.', ?), CAST(? as JSON))
        WHERE r.clusterset_id = ?
          AND r.address = ? AND r.router_name = ?)*",
                   option_name, value.json(false), cluster_id, router_address,
                   router_name);
    } else {
      execute_sqlf(R"*(UPDATE mysql_innodb_cluster_metadata.routers r
        SET r.options =
          JSON_SET(IFNULL(r.options, '{}'), concat('$.', ?), CAST(? as JSON))
        WHERE r.cluster_id = ?
          AND r.address = ? AND r.router_name = ?)*",
                   option_name, value.json(false), cluster_id, router_address,
                   router_name);
    }
  }
}

// ----------------------------------------------------------------------------

bool MetadataStorage::cluster_sets_supported() const {
  return real_version() >= Version(2, 1, 0);
}

namespace {
Cluster_set_member_metadata unserialize_clusterset_member_metadata(
    const mysqlshdk::db::Row_ref_by_name &row) {
  Cluster_set_member_metadata csmd;

  csmd.cluster_set_id = row.get_string("clusterset_id", "");
  csmd.master_cluster_id = row.get_string("master_cluster_id", "");
  csmd.primary_cluster = row.get_string("member_role", "") == "PRIMARY";
  csmd.invalidated = row.get_int("invalidated", 0);

  return csmd;
}
}  // namespace

bool MetadataStorage::get_cluster_set(
    const Cluster_set_id &cs_id, bool allow_invalidated,
    Cluster_set_metadata *out_cs,
    std::vector<Cluster_set_member_metadata> *out_cs_members,
    uint64_t *out_view_id) const {
  Cluster_id cluster_id;

  auto result = execute_sqlf(
      "SELECT clusterset_id, domain_name"
      " FROM mysql_innodb_cluster_metadata.clustersets"
      " WHERE clusterset_id = ?",
      cs_id);
  if (auto row = result->fetch_one_named()) {
    if (out_cs) {
      out_cs->id = row.get_string("clusterset_id");
      out_cs->domain_name = row.get_string("domain_name");
    }

    if (out_cs_members) {
      result = execute_sqlf(
          "SELECT c.*, m.view_id, m.member_role, m.master_cluster_id,"
          "  m.invalidated,"
          "  c.attributes->>'$.group_replication_view_change_uuid' as "
          "view_uuid"
          " FROM mysql_innodb_cluster_metadata.v2_cs_members m"
          " JOIN mysql_innodb_cluster_metadata.v2_gr_clusters c"
          "  ON c.cluster_id = m.cluster_id"
          " WHERE m.clusterset_id = ?" +
              std::string(allow_invalidated ? "" : " AND m.invalidated = 0"),
          cs_id);

      while (auto mrow = result->fetch_one_named()) {
        Cluster_set_member_metadata cmd =
            unserialize_clusterset_member_metadata(mrow);
        cmd.cluster = unserialize_cluster_metadata(mrow, m_md_version);

        out_cs_members->emplace_back(std::move(cmd));

        if (out_view_id) {
          *out_view_id = mrow.get_uint("view_id");
        }
      }
    }
    return true;
  }

  return false;
}

bool MetadataStorage::get_cluster_set_member_for_cluster_name(
    const std::string &name, Cluster_set_member_metadata *out_cluster,
    bool allow_invalidated) const {
  bool ret_val = false;

  auto result = execute_sqlf(
      "SELECT c.*, m.view_id, m.member_role, m.master_cluster_id,"
      "  m.invalidated,"
      "  c.attributes->>'$.group_replication_view_change_uuid' as view_uuid"
      " FROM mysql_innodb_cluster_metadata.v2_cs_members m"
      " JOIN mysql_innodb_cluster_metadata.v2_gr_clusters c"
      "  ON c.cluster_id = m.cluster_id"
      " WHERE c.cluster_name = ?",
      name);

  auto mrow = result->fetch_one_named();
  if (out_cluster && mrow) {
    *out_cluster = unserialize_clusterset_member_metadata(mrow);
    out_cluster->cluster = unserialize_cluster_metadata(mrow, m_md_version);

    if (out_cluster->invalidated && !allow_invalidated) {
      throw shcore::Exception("Cluster '" + name + "' is invalidated",
                              SHERR_DBA_ASYNC_MEMBER_INVALIDATED);
    }

    ret_val = true;
  }

  return ret_val;
}

bool MetadataStorage::get_cluster_set_member(
    const Cluster_id &cluster_id,
    Cluster_set_member_metadata *out_cs_member) const {
  if (!cluster_sets_supported()) return false;

  auto result = execute_sqlf(
      "SELECT c.*, m.view_id, m.member_role, m.master_cluster_id,"
      "  m.invalidated,"
      "  c.attributes->>'$.group_replication_view_change_uuid' as view_uuid"
      " FROM mysql_innodb_cluster_metadata.v2_cs_members m"
      " JOIN mysql_innodb_cluster_metadata.v2_gr_clusters c"
      "  ON c.cluster_id = m.cluster_id"
      " WHERE c.cluster_id = ?",
      cluster_id);

  auto mrow = result->fetch_one_named();
  if (out_cs_member && mrow) {
    *out_cs_member = unserialize_clusterset_member_metadata(mrow);
    out_cs_member->cluster = unserialize_cluster_metadata(mrow, m_md_version);

    return true;
  }

  return false;
}

void MetadataStorage::cleanup_for_cluster(Cluster_id cluster_id) {
  // truncate clusterset_views tables
  execute_sqlf("DELETE FROM mysql_innodb_cluster_metadata.clusterset_members");
  execute_sqlf("DELETE FROM mysql_innodb_cluster_metadata.clusterset_views");

  // delete instances from other clusters
  execute_sqlf(
      "DELETE FROM mysql_innodb_cluster_metadata.instances"
      " WHERE cluster_id <> ?",
      cluster_id);

  // delete clusters
  execute_sqlf(
      "DELETE FROM mysql_innodb_cluster_metadata.clusters"
      " WHERE cluster_id <> ?",
      cluster_id);

  // delete clustersets
  execute_sqlf("DELETE FROM mysql_innodb_cluster_metadata.clustersets");
}

Cluster_set_id MetadataStorage::create_cluster_set_record(
    Cluster_set_impl *clusterset, Cluster_id seed_cluster_id,
    shcore::Dictionary_t seed_attributes) {
  execute_sqlf(
      "CALL mysql_innodb_cluster_metadata.v2_cs_created(?, ?, ?, @_cs_id)",
      clusterset->get_name(), seed_cluster_id,
      shcore::Value(seed_attributes).json());

  Cluster_set_id cs_id =
      execute_sqlf("select @_cs_id")->fetch_one()->get_string(0, "");
  clusterset->set_id(cs_id);

  return cs_id;
}

void MetadataStorage::record_cluster_set_member_added(
    const Cluster_set_member_metadata &cluster) {
  execute_sqlf(
      "CALL mysql_innodb_cluster_metadata.v2_cs_member_added(?, ?, ?, '{}')",
      cluster.cluster_set_id, cluster.cluster.cluster_id,
      cluster.master_cluster_id);
}

void MetadataStorage::record_cluster_set_member_removed(
    const Cluster_set_id &cs_id, const Cluster_id &cluster_id) {
  execute_sqlf("CALL mysql_innodb_cluster_metadata.v2_cs_member_removed(?, ?)",
               cs_id, cluster_id);
}

void MetadataStorage::record_cluster_set_member_rejoined(
    const Cluster_set_id &cs_id, const Cluster_id &cluster_id,
    const Cluster_id &master_cluster_id) {
  execute_sqlf(
      "CALL mysql_innodb_cluster_metadata.v2_cs_member_rejoined(?, ?, ?, "
      "'{}')",
      cs_id, cluster_id, master_cluster_id);
}

void MetadataStorage::record_cluster_set_primary_switch(
    const Cluster_set_id &cs_id, const Cluster_id &new_primary_id,
    const std::list<Cluster_id> &invalidated) {
  execute_sqlf(
      "CALL mysql_innodb_cluster_metadata.v2_cs_primary_changed(?, ?, "
      "'{}')",
      cs_id, new_primary_id);

  for (const auto &c : invalidated) {
    execute_sqlf(
        "CALL mysql_innodb_cluster_metadata.v2_cs_add_invalidated_member(?, "
        "?)",
        cs_id, c);
  }
}

void MetadataStorage::record_cluster_set_primary_failover(
    const Cluster_set_id &cs_id, const Cluster_id &cluster_id,
    const std::list<Cluster_id> &invalidated) {
  execute_sqlf(
      "CALL mysql_innodb_cluster_metadata.v2_cs_primary_force_changed(?, ?, "
      "'{}')",
      cs_id, cluster_id);

  for (const auto &c : invalidated) {
    execute_sqlf(
        "CALL mysql_innodb_cluster_metadata.v2_cs_add_invalidated_member(?, "
        "?)",
        cs_id, c);
  }
}

bool MetadataStorage::check_metadata(mysqlshdk::utils::Version *out_version,
                                     Cluster_type *out_type,
                                     Replica_type *out_replica_type) const {
  if (check_version(out_version)) {
    auto target_server = get_md_server();
    log_debug("Instance type check: %s: Metadata version %s found",
              target_server->descr().c_str(), out_version->get_full().c_str());

    if (!check_instance_type(target_server->get_uuid(), *out_version, out_type,
                             out_replica_type)) {
      *out_type = Cluster_type::NONE;
      log_debug("Instance %s is not managed",
                target_server->get_uuid().c_str());
    } else {
      log_debug("Instance %s is managed for %s",
                target_server->get_uuid().c_str(),
                to_string(*out_type).c_str());
    }

    return true;
  }
  return false;
}

bool MetadataStorage::check_cluster_set(
    const mysqlshdk::mysql::IInstance *target_instance, uint64_t *out_view_id,
    std::string *out_cs_domain_name, Cluster_set_id *out_cluster_set_id) const {
  bool ret_val = false;

  if (!cluster_sets_supported()) {
    return false;
  }

  auto target = target_instance ? target_instance : m_md_server.get();

  std::string csid;

  auto instance_address = target->get_canonical_address();
  try {
    auto result = m_md_server->queryf(
        "SELECT clusterset_id from mysql_innodb_cluster_metadata.clusters c, "
        "mysql_innodb_cluster_metadata.instances i WHERE "
        "c.cluster_id=i.cluster_id AND i.address = ?",
        instance_address);

    auto row = result->fetch_one();

    ret_val = row && !row->is_null(0);

    if (ret_val) csid = row->get_string(0);

    if (ret_val && (out_cs_domain_name || out_cluster_set_id)) {
      result = execute_sqlf(
          "SELECT domain_name, clusterset_id"
          " FROM mysql_innodb_cluster_metadata.clustersets"
          " WHERE clusterset_id = ?",
          row->get_as_string(0));

      auto domain_name = result->fetch_one();

      if (out_cs_domain_name)
        *out_cs_domain_name = domain_name->get_as_string(0);
      if (out_cluster_set_id)
        *out_cluster_set_id = domain_name->get_as_string(1);
    }
  } catch (const shcore::Error &e) {
    // ER_BAD_FIELD_ERROR Would be raised if metadata schema is not 2.1.0
    // ER_BAD_DB_ERROR Would be raised in a metadata upgrade failure in which
    // case the state doesn't really matter
    // ER_NO_SUCH_TABLE Would be raised with 5.7 servers
    if (e.code() != ER_BAD_FIELD_ERROR && e.code() != ER_BAD_DB_ERROR &&
        e.code() != ER_NO_SUCH_TABLE) {
      log_error(
          "Error while verifying if instance '%s' belongs to a ClusterSet: "
          "%s",
          instance_address.c_str(), e.what());

      throw shcore::Exception::runtime_error(
          "Unable to determine if the instance '" + instance_address +
          "' belongs to a ClusterSet");
    }
  }

  if (out_view_id) {
    auto result = m_md_server->queryf(
        "SELECT MAX(view_id)"
        " FROM mysql_innodb_cluster_metadata.clusterset_views"
        " WHERE clusterset_id = ?",
        csid);
    if (auto row = result->fetch_one()) {
      *out_view_id = row->get_uint(0, 0);
    } else {
      *out_view_id = 0;
    }
  }

  return ret_val;
}

bool MetadataStorage::supports_cluster_set() const {
  return real_version() >= Version(2, 1, 0);
}

bool MetadataStorage::supports_read_replicas() const {
  return real_version() >= mysqlshdk::utils::Version(2, 2, 0);
}

}  // namespace dba
}  // namespace mysqlsh
