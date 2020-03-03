/*
 * Copyright (c) 2016, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include <mysql.h>
#include <mysqld_error.h>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {
namespace {
Router_metadata unserialize_router(const mysqlshdk::db::Row_ref_by_name &row) {
  Router_metadata router;

  router.id = row.get_uint("router_id");
  router.hostname = row.get_string("host_name");
  router.name = row.get_string("router_name");
  if (!row.is_null("ro_port"))
    router.ro_port = std::stoi(row.get_string("ro_port"));
  if (!row.is_null("rw_port"))
    router.rw_port = std::stoi(row.get_string("rw_port"));
  if (!row.is_null("ro_x_port"))
    router.ro_x_port = std::stoi(row.get_string("ro_x_port"));
  if (!row.is_null("rw_x_port"))
    router.rw_x_port = std::stoi(row.get_string("rw_x_port"));
  if (!row.is_null("last_check_in"))
    router.last_checkin = row.get_string("last_check_in");
  if (!row.is_null("version")) router.version = row.get_string("version");

  return router;
}

constexpr const char *k_list_routers_1_0_1 =
    "SELECT r.router_id, r.router_name, h.host_name,"
    " r.attributes->>'$.ROEndpoint' AS ro_port,"
    " r.attributes->>'$.RWEndpoint' AS rw_port,"
    " r.attributes->>'$.ROXEndpoint' AS ro_x_port,"
    " r.attributes->>'$.RWXEndpoint' AS rw_x_port,"
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
    " r.last_check_in, r.version"
    " FROM mysql_innodb_cluster_metadata.v2_routers r";

const char *get_router_query(const mysqlshdk::utils::Version &md_version) {
  if (md_version.get_major() == 1)
    return k_list_routers_1_0_1;
  else
    return k_list_routers;
}

static constexpr const char *k_select_cluster_metadata =
    R"*(SELECT * FROM (
SELECT cluster_type, primary_mode, cluster_id, cluster_name,
      description, NULL as group_name, async_topology_type
FROM mysql_innodb_cluster_metadata.v2_ar_clusters
UNION ALL
SELECT cluster_type, primary_mode, cluster_id, cluster_name,
      description, group_name, NULL as async_topology_type
FROM mysql_innodb_cluster_metadata.v2_gr_clusters
) as c)*";

static constexpr const char *k_select_cluster_metadata_1_0_1 =
    R"*(SELECT r.topology_type, c.cluster_id, c.cluster_name, c.description,
 r.attributes->>'$.group_replication_group_name' as group_name
 FROM mysql_innodb_cluster_metadata.clusters c
 JOIN mysql_innodb_cluster_metadata.replicasets r
  ON c.cluster_id = r.cluster_id)*";

std::string get_cluster_query(const mysqlshdk::utils::Version &md_version) {
  if (md_version.get_major() == 1)
    return k_select_cluster_metadata_1_0_1;
  else
    return k_select_cluster_metadata;
}

constexpr const char *k_base_instance_query =
    "SELECT i.instance_id, i.cluster_id, c.group_name,"
    " am.master_instance_id, am.master_member_id, am.member_role, am.view_id,"
    " i.label, i.mysql_server_uuid, i.address,"
    " i.endpoint, i.xendpoint, '' as grendpoint"
    " FROM mysql_innodb_cluster_metadata.v2_instances i"
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
    " i.addresses->>'$.grEndpoint' grendpoint"
    " FROM mysql_innodb_cluster_metadata.instances i"
    " LEFT JOIN mysql_innodb_cluster_metadata.replicasets r"
    "   ON r.replicaset_id = i.replicaset_id";

std::string get_instance_query(const mysqlshdk::utils::Version &md_version) {
  if (md_version.get_major() == 1)
    return k_base_instance_query_1_0_1;
  else
    return k_base_instance_query;
}

constexpr const char *k_all_online_check_query =
    "SELECT (SELECT COUNT(*) "
    "FROM !.instances "
    "WHERE cluster_id = (SELECT cluster_id "
    "FROM !.instances "
    "WHERE mysql_server_uuid=@@server_uuid)) = (SELECT count(*) "
    "FROM performance_schema.replication_group_members "
    "WHERE member_state = 'ONLINE') as all_online";

constexpr const char *k_all_online_check_query_1_0_1 =
    "SELECT (SELECT COUNT(*) "
    "FROM !.instances "
    "WHERE replicaset_id = (SELECT replicaset_id "
    "FROM !.instances "
    "WHERE mysql_server_uuid=@@server_uuid)) = (SELECT count(*) "
    "FROM performance_schema.replication_group_members "
    "WHERE member_state = 'ONLINE') as all_online";

std::string get_all_online_check_query(
    const mysqlshdk::utils::Version &md_version) {
  if (md_version.get_major() == 1)
    return k_all_online_check_query_1_0_1;
  else
    return k_all_online_check_query;
}

constexpr const char *k_base_instance_in_cluster_query =
    "SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances "
    "WHERE cluster_id = ? AND addresses->'$.mysqlClassic' = ?";

constexpr const char *k_base_instance_in_cluster_query_1_0_1 =
    "SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances, "
    "mysql_innodb_cluster_metadata.replicasets "
    "WHERE cluster_id = ? AND addresses->'$.mysqlClassic' = ?";

std::string get_instance_in_cluster_query(
    const mysqlshdk::utils::Version &md_version) {
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

std::string get_cluster_size_query(
    const mysqlshdk::utils::Version &md_version) {
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

std::string get_topology_mode_query(
    const mysqlshdk::utils::Version &md_version) {
  if (md_version.get_major() == 1)
    return k_base_topology_mode_query_1_0_1;
  else
    return k_base_topology_mode_query;
}

// Constants with the names used to lock instances.
static constexpr char k_lock[] = "AdminAPI_lock";
static constexpr char k_lock_name_metadata[] = "AdminAPI_metadata";

// Timeout for the Metadata lock (60 sec).
constexpr const int k_lock_timeout = 60;

}  // namespace

class MetadataStorage::Transaction {
 public:
  explicit Transaction(MetadataStorage *md) : _md(md) {
    md->execute_sql("START TRANSACTION");
  }

  ~Transaction() {
    try {
      if (_md) _md->execute_sql("ROLLBACK");
    } catch (const std::exception &e) {
      log_error("Error implicitly rolling back transaction: %s", e.what());
    }
  }

  void commit() {
    if (_md) {
      _md->execute_sql("COMMIT");
      _md = nullptr;
    }
  }

 private:
  MetadataStorage *_md;
};

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
    m_md_state = mysqlsh::dba::metadata::check_installed_schema_version(
        m_md_server, &m_md_version, &m_real_md_version, &m_md_version_schema);
  }

  if (m_md_version != mysqlsh::dba::metadata::kNotInstalled) {
    if (out_version) *out_version = m_md_version;
    return true;
  }

  return false;
}

bool MetadataStorage::check_instance_type(
    const std::string & /*uuid*/,
    const mysqlshdk::utils::Version & /*md_version*/,
    Cluster_type *out_type) const {
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
            *out_type = Cluster_type::GROUP_REPLICATION;
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
        auto query =
            shcore::sqlstring("select cluster_type from !.v2_this_instance", 0);
        query << m_md_version_schema;
        query.done();

        auto result = execute_sql(query);
        auto row = result->fetch_one();

        if (row && !row->is_null(0)) {
          std::string type = row->get_string(0);
          if (type == "ar") {
            log_debug(
                "Instance type check: %s: ReplicaSet metadata record found "
                "(metadata %s)",
                m_md_server->descr().c_str(),
                m_real_md_version.get_full().c_str());
            *out_type = Cluster_type::ASYNC_REPLICATION;
            return true;
          } else if (type == "gr") {
            log_debug(
                "Instance type check: %s: Cluster metadata record found "
                "(metadata %s)",
                m_md_server->descr().c_str(),
                m_real_md_version.get_full().c_str());
            *out_type = Cluster_type::GROUP_REPLICATION;
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
    log_warning("While querying metadata: %s", err.format().c_str());
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
    const mysqlshdk::db::Row_ref_by_name &row,
    const mysqlshdk::utils::Version &version) {
  Cluster_metadata rs;

  if (version.get_major() == 1) {
    rs.cluster_id = std::to_string(row.get_uint("cluster_id"));
    rs.topology_type = row.get_string("topology_type", "");
    rs.type = Cluster_type::GROUP_REPLICATION;
  } else {
    rs.cluster_id = row.get_string("cluster_id");
    rs.topology_type = row.get_string("primary_mode", "");
    if (row.get_string("cluster_type") == "ar")
      rs.type = Cluster_type::ASYNC_REPLICATION;
    else
      rs.type = Cluster_type::GROUP_REPLICATION;
    if (!row.is_null("async_topology_type"))
      rs.async_topology_type =
          to_topology_type(row.get_string("async_topology_type"));
  }

  rs.cluster_name = row.get_string("cluster_name");
  rs.description = row.get_string("description", "");
  rs.group_name = row.get_string("group_name", "");

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
                                              const std::string &attribute,
                                              shcore::Value *out_value) {
  auto result = execute_sql(
      shcore::sqlstring("SELECT attributes->'$." + attribute +
                            "' FROM mysql_innodb_cluster_metadata.clusters"
                            " WHERE cluster_id=?",
                        0)
      << cluster_id);

  if (auto row = result->fetch_one()) {
    if (!row->is_null(0)) {
      *out_value = shcore::Value::parse(row->get_as_string(0));
      return true;
    }
  }
  return false;
}

void MetadataStorage::update_cluster_attribute(const Cluster_id &cluster_id,
                                               const std::string &attribute,
                                               const shcore::Value &value) {
  if (value) {
    shcore::sqlstring query(
        "UPDATE mysql_innodb_cluster_metadata.clusters"
        " SET attributes = json_set(attributes, '$." +
            attribute +
            "', CAST(? as JSON))"
            " WHERE cluster_id = ?",
        0);

    query << value.repr() << cluster_id;
    query.done();
    execute_sql(query);
  } else {
    shcore::sqlstring query(
        "UPDATE mysql_innodb_cluster_metadata.clusters"
        " SET attributes = json_remove(attributes, '$." +
            attribute +
            "')"
            " WHERE cluster_id = ?",
        0);
    query << cluster_id;
    query.done();
    execute_sql(query);
  }
}

std::vector<Cluster_metadata> MetadataStorage::get_all_clusters() {
  std::vector<Cluster_metadata> l;

  if (real_version() != metadata::kNotInstalled) {
    std::string query(get_cluster_query(m_real_md_version));

    // If a different schema is provided, uses it
    if (m_md_version_schema != metadata::kMetadataSchemaName)
      query = shcore::str_replace(query, metadata::kMetadataSchemaName,
                                  m_md_version_schema);

    auto result = execute_sqlf(query);

    while (auto row = result->fetch_one_named()) {
      l.push_back(unserialize_cluster_metadata(row, m_real_md_version));
    }
  }

  return l;
}

bool MetadataStorage::get_cluster_for_server_uuid(
    const std::string &server_uuid, Cluster_metadata *out_cluster) {
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
    const std::string &name, Cluster_metadata *out_cluster) {
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
    *out_cluster = unserialize_cluster_metadata(row, m_md_version);
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
                                                  bool adopted) {
  Cluster_id cluster_id;

  try {
    auto result = execute_sqlf("SELECT uuid()");
    cluster_id = result->fetch_one_or_throw()->get_string(0);

    result = execute_sqlf(
        "INSERT INTO mysql_innodb_cluster_metadata.clusters "
        "(cluster_id, cluster_name, description,"
        " cluster_type, primary_mode, attributes)"
        " VALUES (?, ?, ?, 'gr', ?,"
        " JSON_OBJECT('adopted', ?,"
        "      'group_replication_group_name', ?))",
        cluster_id, cluster->cluster_name(), cluster->get_description(),
        cluster->get_topology_type(), adopted, cluster->get_group_name());
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

Instance_id MetadataStorage::insert_instance(
    const Instance_metadata &instance) {
  std::string addresses;
  addresses = shcore::sqlstring("'mysqlClassic', ?", 0) << instance.endpoint;
  if (!instance.xendpoint.empty())
    addresses += shcore::sqlstring(", 'mysqlX', ?", 0) << instance.xendpoint;
  if (!instance.grendpoint.empty())
    addresses += shcore::sqlstring(", 'grLocal', ?", 0) << instance.grendpoint;

  shcore::sqlstring query;
  query = shcore::sqlstring(
      "INSERT INTO mysql_innodb_cluster_metadata.instances "
      "(cluster_id, address, mysql_server_uuid, "
      "instance_name, addresses, attributes)"
      "VALUES (?, ?, ?, ?, json_object(" +
          addresses + "), '{}')",
      0);

  query << instance.cluster_id;
  query << instance.address;
  query << instance.uuid;
  query << instance.label;
  query.done();

  auto result = execute_sql(query);

  return result->get_auto_increment_value();
}

bool MetadataStorage::query_instance_attribute(const std::string &uuid,
                                               const std::string &attribute,
                                               shcore::Value *out_value) {
  auto result = execute_sql(
      shcore::sqlstring("SELECT attributes->'$." + attribute +
                            "' FROM mysql_innodb_cluster_metadata.instances"
                            " WHERE mysql_server_uuid=?",
                        0)
      << uuid);

  if (auto row = result->fetch_one()) {
    if (!row->is_null(0)) {
      *out_value = shcore::Value::parse(row->get_as_string(0));
      return true;
    }
  }
  return false;
}

void MetadataStorage::update_instance_attribute(const std::string &uuid,
                                                const std::string &attribute,
                                                const shcore::Value &value) {
  if (value) {
    shcore::sqlstring query(
        "UPDATE mysql_innodb_cluster_metadata.instances"
        " SET attributes = json_set(attributes, '$." +
            attribute +
            "', CAST(? as JSON))"
            " WHERE mysql_server_uuid = ?",
        0);

    query << value.repr() << uuid;
    query.done();
    execute_sql(query);
  } else {
    shcore::sqlstring query(
        "UPDATE mysql_innodb_cluster_metadata.instances"
        " SET attributes = json_remove(attributes, '$." +
            attribute +
            "')"
            " WHERE mysql_server_uuid = ?",
        0);
    query << uuid;
    query.done();
    execute_sql(query);
  }
}

void MetadataStorage::update_instance_recovery_account(
    const std::string &instance_uuid, const std::string &recovery_account_user,
    const std::string &recovery_account_host) {
  shcore::sqlstring query;

  if (!recovery_account_user.empty()) {
    query = shcore::sqlstring(
        "UPDATE mysql_innodb_cluster_metadata.instances"
        " SET attributes = json_set(COALESCE(attributes, '{}'),"
        "  '$.recoveryAccountUser', ?,"
        "  '$.recoveryAccountHost', ?)"
        " WHERE mysql_server_uuid = ?",
        0);
    query << recovery_account_user;
    query << recovery_account_host;
    query << instance_uuid;
    query.done();
  } else {
    // if recovery_account user is empty, clear existing recovery attributes
    // of the instance from the metadata.
    query = shcore::sqlstring(
        "UPDATE mysql_innodb_cluster_metadata.instances"
        " SET attributes = json_remove(attributes,"
        "  '$.recoveryAccountUser', '$.recoveryAccountHost')"
        " WHERE mysql_server_uuid = ?",
        0);
    query << instance_uuid;
    query.done();
  }
  execute_sql(query);
}

std::pair<std::string, std::string>
MetadataStorage::get_instance_recovery_account(
    const std::string &instance_uuid) {
  shcore::sqlstring query = shcore::sqlstring{
      "SELECT (attributes->>'$.recoveryAccountUser') as recovery_user,"
      " (attributes->>'$.recoveryAccountHost') as recovery_host"
      " FROM mysql_innodb_cluster_metadata.instances "
      " WHERE mysql_server_uuid = ?",
      0};
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

void MetadataStorage::remove_instance_recovery_account(
    const std::string &instance_uuid,
    const std::string &recovery_account_user) {
  shcore::sqlstring query;

  if (!recovery_account_user.empty()) {
    query = shcore::sqlstring(
        "UPDATE mysql_innodb_cluster_metadata.instances "
        "SET attributes = json_remove(attributes, "
        "'$.recoveryAccountUser') WHERE mysql_server_uuid = ? AND "
        "attributes->>'$.recoveryAccountUser' = ?",
        0);
    query << instance_uuid;
    query << recovery_account_user;
    query.done();
    execute_sql(query);
  }
}

bool MetadataStorage::is_recovery_account_unique(
    const std::string &recovery_account_user) {
  shcore::sqlstring query;

  if (!recovery_account_user.empty()) {
    query = shcore::sqlstring(
        "SELECT COUNT(*) as count FROM "
        "mysql_innodb_cluster_metadata.instances "
        "WHERE attributes->'$.recoveryAccountUser' = ?",
        0);
    query << recovery_account_user;
    query.done();

    auto result = execute_sql(query);
    auto row = result->fetch_one();
    int count = 0;

    if (row) {
      count = row->get_int(0);
    }

    return count == 1;
  }

  return false;
}

void MetadataStorage::remove_instance(const std::string &instance_address) {
  shcore::sqlstring query;

  // Remove the instance
  query = shcore::sqlstring(
      "DELETE FROM mysql_innodb_cluster_metadata.instances "
      "WHERE addresses->'$.mysqlClassic' = ?",
      0);
  query << instance_address;
  query.done();

  execute_sql(query);
}

void MetadataStorage::drop_cluster(const std::string &cluster_name) {
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

  Transaction tx(this);

  // It exists, so let's get the cluster_id and move on
  Cluster_id cluster_id = get_cluster_id(cluster_name);

  execute_sqlf(
      "DELETE FROM mysql_innodb_cluster_metadata.instances "
      "WHERE cluster_id = ?",
      cluster_id);

  // Remove the cluster
  execute_sqlf(
      "DELETE from mysql_innodb_cluster_metadata.clusters "
      "WHERE cluster_id = ?",
      cluster_id);

  tx.commit();
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
    const mysqlshdk::db::Row_ref_by_name &row,
    mysqlshdk::utils::Version *mdver_in) const {
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
        (!row.has_field("group_name") || row.is_null("group_name")))
      instance.invalidated = true;
  }

  instance.id = row.get_uint("instance_id");

  if (row.has_field("group_name"))
    instance.group_name = row.get_string("group_name", "");
  instance.uuid = row.get_string("mysql_server_uuid", "");
  instance.label = row.get_string("label", "");
  instance.endpoint = row.get_string("endpoint", "");
  instance.xendpoint = row.get_string("xendpoint", "");
  if (row.has_field("grendpoint"))
    instance.grendpoint = row.get_string("grendpoint", "");

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
    Cluster_id cluster_id) {
  std::vector<Instance_metadata> ret_val;

  if (real_version() != metadata::kNotInstalled) {
    std::string query(get_instance_query(m_real_md_version));

    // If a different schema is provided, uses it
    if (m_md_version_schema != metadata::kMetadataSchemaName)
      query = shcore::str_replace(query, metadata::kMetadataSchemaName,
                                  m_md_version_schema);

    std::shared_ptr<mysqlshdk::db::IResult> result;

    if (cluster_id.empty()) {
      result = execute_sql(query);
    } else {
      if (m_real_md_version.get_major() == 1) {
        result = execute_sqlf(query + " WHERE r.cluster_id = ?",
                              std::atoi(cluster_id.c_str()));
      } else {
        result = execute_sqlf(query + " WHERE i.cluster_id = ?", cluster_id);
      }
    }

    while (auto row = result->fetch_one_named()) {
      ret_val.push_back(unserialize_instance(row, &m_real_md_version));
    }
  }

  return ret_val;
}

constexpr const char *k_replica_set_instances_query =
    "SELECT i.instance_id, i.cluster_id,"
    " am.master_instance_id, am.master_member_id,"
    " am.member_role, am.view_id, "
    " i.label, i.mysql_server_uuid, i.address, i.endpoint, i.xendpoint"
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
    const std::string &uuid) const {
  auto result = execute_sqlf(
      get_instance_query(real_version()) + " WHERE i.mysql_server_uuid = ?",
      uuid);

  if (auto row = result->fetch_one_named()) {
    return unserialize_instance(row);
  }

  throw shcore::Exception("Metadata for instance " + uuid + " not found",
                          SHERR_DBA_MEMBER_METADATA_MISSING);
}

Instance_metadata MetadataStorage::get_instance_by_address(
    const std::string &instance_address) {
  auto md_version = real_version();
  auto query(get_instance_query(md_version));

  if (md_version.get_major() == 1)
    query += " WHERE i.addresses->>'$.mysqlClassic' = ?";
  else
    query += " WHERE i.address = ?";

  auto result = execute_sqlf(query, instance_address);

  if (auto row = result->fetch_one_named()) {
    return unserialize_instance(row);
  }

  throw shcore::Exception(
      "Metadata for instance " + instance_address + " not found",
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

void MetadataStorage::get_lock_exclusive() const {
  auto console = current_console();

  // Use a predefined timeout value (60 sec) to try to acquire the lock.
  int timeout = k_lock_timeout;
  mysqlshdk::mysql::Lock_mode mode = mysqlshdk::mysql::Lock_mode::EXCLUSIVE;

  DBUG_EXECUTE_IF("dba_locking_timeout_one", { timeout = 1; });

  // Try to acquire the specified lock.
  // NOTE: Only one lock per namespace is used because lock release is
  //       performed by namespace.
  if (mysqlshdk::mysql::has_lock_service_udfs(*m_md_server)) {
    // No need install Locking Service UDFs, already done at a higher level
    // (instance) if needed/supported. Thus, we only try to acquire the lock if
    // the lock UDFs are available (skipped otherwise).

    try {
      log_debug("Acquiring %s lock ('%s', '%s') on %s.",
                mysqlshdk::mysql::to_string(mode).c_str(), k_lock_name_metadata,
                k_lock, m_md_server->descr().c_str());
      mysqlshdk::mysql::get_lock(*m_md_server, k_lock_name_metadata, k_lock,
                                 mode, timeout);
    } catch (const shcore::Exception &err) {
      // Abort the operation in case the required lock cannot be acquired.
      log_debug("Failed to get %s lock ('%s', '%s'): %s",
                mysqlshdk::mysql::to_string(mode).c_str(), k_lock_name_metadata,
                k_lock, err.what());
      console->print_error(
          "Cannot update the metadata because the maximum wait time to "
          "acquire a write lock has been reached.");
      console->print_info(
          "Other operations requiring "
          "exclusive access to the metadata are running concurrently, please "
          "wait for those operations to finish and try again.");

      throw shcore::Exception(
          "Failed to acquire lock to update the metadata on instance '" +
              m_md_server->descr() + "', wait timeout exceeded",
          SHERR_DBA_LOCK_GET_TIMEOUT);
    }
  }
}

void MetadataStorage::release_lock(bool no_throw) const {
  // Release all metadata locks in the k_lock_name_metadata namespace.
  // NOTE: Only perform the operation if the lock service UDFs are available
  //       otherwise do nothing (ignore if concurrent execution is not
  //       supported, e.g., lock service plugin not available).
  try {
    if (mysqlshdk::mysql::has_lock_service_udfs(*m_md_server)) {
      log_debug("Releasing locks for '%s' on %s.", k_lock_name_metadata,
                m_md_server->descr().c_str());
      mysqlshdk::mysql::release_lock(*m_md_server, k_lock_name_metadata);
    }
  } catch (const shcore::Error &error) {
    if (no_throw) {
      // Ignore any error trying to release locks (e.g., might have not been
      // previously acquired due to lack of permissions).
      log_error("Unable to release '%s' locks for '%s': %s",
                k_lock_name_metadata, m_md_server->descr().c_str(),
                error.what());
    } else {
      throw;
    }
  }
}

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
  Transaction tx(this);

  Cluster_id cluster_id;
  try {
    auto result = execute_sqlf("SELECT uuid()");
    cluster_id = result->fetch_one_or_throw()->get_string(0);

    result = execute_sqlf(
        "INSERT INTO mysql_innodb_cluster_metadata.clusters "
        "(cluster_id, cluster_name, description,"
        " cluster_type, primary_mode, attributes)"
        " VALUES (?, ?, ?, ?, ?,"
        " JSON_OBJECT('adopted', ?))",
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
      " cluster_id, view_id, topology_type,"
      " view_change_reason, view_change_time, view_change_info,"
      " attributes "
      ") VALUES (?, 1, ?, ?, NOW(6), "
      "JSON_OBJECT('user', USER(),"
      "   'source', @@server_uuid),"
      "'{}')",
      cluster_id, to_string(cluster->get_async_topology_type()),
      k_async_view_change_reason_create);

  cluster->set_id(cluster_id);

  tx.commit();

  return cluster_id;
}

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
uint32_t MetadataStorage::inc_view_failover_counter(uint32_t view_id) const {
  uint32_t res = view_id >> 16;
  res += 1;
  res <<= 16;
  return res;
}

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
      " (cluster_id, view_id, topology_type, "
      " view_change_reason, view_change_time, view_change_info, "
      " attributes"
      ") SELECT"
      " cluster_id, ?, topology_type, ?,"
      " NOW(6), JSON_OBJECT('user', USER(),"
      "   'source', @@server_uuid),"
      " attributes"
      " FROM mysql_innodb_cluster_metadata.async_cluster_views"
      " WHERE cluster_id = ? AND view_id = ?",
      aclvid, operation, cluster_id, last_aclvid);

  *out_aclvid = aclvid;
  *out_last_aclvid = last_aclvid;
}

Instance_id MetadataStorage::record_async_member_added(
    const Instance_metadata &member) {
  // Acquire required lock on the Metadata (try at most during 60 sec).
  get_lock_exclusive();

  // Always release locks at the end, when leaving the function scope.
  auto finally = shcore::on_leave_scope([this]() { release_lock(); });

  Transaction tx(this);

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

  tx.commit();

  return member_id;
}

void MetadataStorage::record_async_member_rejoined(
    const Instance_metadata &member) {
  // Acquire required lock on the Metadata (try at most during 60 sec).
  get_lock_exclusive();

  // Always release locks at the end, when leaving the function scope.
  auto finally = shcore::on_leave_scope([this]() { release_lock(); });

  Transaction tx(this);

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

  tx.commit();
}

void MetadataStorage::record_async_member_removed(const Cluster_id &cluster_id,
                                                  Instance_id instance_id) {
  // Acquire required lock on the Metadata (try at most during 60 sec).
  get_lock_exclusive();

  // Always release locks at the end, when leaving the function scope.
  auto finally = shcore::on_leave_scope([this]() { release_lock(); });

  uint32_t aclvid;
  uint32_t last_aclvid;

  Transaction tx(this);

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

  tx.commit();
}

void MetadataStorage::record_async_primary_switch(Instance_id new_primary_id) {
  // NO need to acquire a Metadata lock because an exclusive locks is already
  // acquired (with a higher level) on all instances of the replica set for
  // set_primary_instance().

  // In a safe active switch, we copy over the whole view of the cluster
  // at once, at the beginning.
  uint32_t aclvid;
  uint32_t last_aclvid;

  Transaction tx(this);

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

    tx.commit();
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

  Transaction tx(this);

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

    tx.commit();
  } else {
    log_error("Async cluster for instance %i has metadata errors",
              new_primary_id);
    throw shcore::Exception("Invalid metadata for async cluster found",
                            SHERR_DBA_METADATA_INCONSISTENT);
  }
}

std::vector<Router_metadata> MetadataStorage::get_routers(
    const Cluster_id &cluster_id) {
  std::vector<Router_metadata> ret_val;
  std::string query(get_router_query(real_version()));

  std::shared_ptr<mysqlshdk::db::IResult> result;

  if (m_md_version.get_major() >= 2 && !cluster_id.empty()) {
    query.append(" WHERE r.cluster_id = ?");
    result = execute_sqlf(query, cluster_id);
  } else {
    result = execute_sqlf(query);
  }

  while (auto row = result->fetch_one_named()) {
    auto router = unserialize_router(row);
    ret_val.push_back(router);
  }
  return ret_val;
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

  // Acquire required lock on the Metadata (try at most during 60 sec).
  get_lock_exclusive();

  // Always release locks at the end, when leaving the function scope.
  auto finally = shcore::on_leave_scope([this]() { release_lock(); });

  // This has to support MD versions 1.0 and 2.0, so that we can remove older
  // router versions while upgrading the MD.
  if (real_version().get_major() == 1) {
    auto result = execute_sqlf(
        "SELECT router_id FROM mysql_innodb_cluster_metadata.routers r"
        " JOIN mysql_innodb_cluster_metadata.hosts h ON r.host_id = h.host_id"
        " WHERE r.router_name = ? AND h.host_name = ?",
        name, address);

    if (auto row = result->fetch_one()) {
      int router_id = row->get_int(0);

      execute_sqlf(
          "DELETE FROM mysql_innodb_cluster_metadata.routers"
          " WHERE router_id = ?",
          router_id);

      return true;
    } else {
      return false;
    }
  } else {
    auto result = execute_sqlf(
        "SELECT router_id FROM mysql_innodb_cluster_metadata.routers r"
        " WHERE r.router_name = ? AND r.address = ?",
        name, address);

    if (auto row = result->fetch_one()) {
      int router_id = row->get_int(0);

      execute_sqlf(
          "DELETE FROM mysql_innodb_cluster_metadata.routers"
          " WHERE router_id = ?",
          router_id);

      return true;
    } else {
      return false;
    }
  }
}

}  // namespace dba
}  // namespace mysqlsh
