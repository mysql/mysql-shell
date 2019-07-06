/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/dba_errors.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_sqlstring.h"
#include "utils/utils_string.h"

namespace mysqlsh {
namespace dba {

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
    : m_md_server(instance) {
  log_debug("Metadata operations will use %s", instance->descr().c_str());

  // TODO(alfredo) instance->retain();
}

MetadataStorage::~MetadataStorage() {
  // TODO(alfredo)  if (m_md_server) m_md_server->release();
}

bool MetadataStorage::check_exists(
    mysqlshdk::utils::Version *out_version) const {
  if (m_md_version != mysqlshdk::utils::Version()) {
    if (out_version) *out_version = m_md_version;
    return true;
  }

  try {
    auto result = execute_sql(
        "select major, minor, patch from "
        "mysql_innodb_cluster_metadata.schema_version");
    auto row = result->fetch_one_or_throw();
    int major = row->get_int(0);
    int minor = row->get_int(1);
    int patch = row->get_int(2);

    m_md_version = mysqlshdk::utils::Version(major, minor, patch);
    if (out_version) *out_version = m_md_version;
  } catch (const shcore::Exception &error) {
    log_debug("Error querying metadata: %s: %s", m_md_server->descr().c_str(),
              error.format().c_str());

    // Ignore error table does not exist (error 1146) for 5.7 or database
    // does not exist (error 1049) for 8.0, when metadata is not available.
    if (error.code() != ER_NO_SUCH_TABLE && error.code() != ER_BAD_DB_ERROR)
      throw;

    return false;
  }
  return true;
}

std::shared_ptr<mysqlshdk::db::IResult> MetadataStorage::execute_sql(
    const std::string &sql) const {
  std::shared_ptr<mysqlshdk::db::IResult> ret_val;

  try {
    ret_val = m_md_server->query(sql);
  } catch (mysqlshdk::db::Error &err) {
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
      throw shcore::Exception::mysql_error_with_code_and_state(
          err.what(), err.code(), err.sqlstate());
    }
  }

  return ret_val;
}

Cluster_metadata MetadataStorage::unserialize_cluster_metadata(
    const mysqlshdk::db::Row_ref_by_name &row) {
  Cluster_metadata rs;

  rs.cluster_id = row.get_uint("cluster_id");
  rs.cluster_name = row.get_string("cluster_name");
  rs.description = row.get_string("description", "");
  rs.topology_type = row.get_string("topology_type", "");

  rs.group_name = row.get_string("group_name", "");

  return rs;
}

static constexpr const char *k_select_cluster_metadata =
    R"*(SELECT r.topology_type, c.cluster_id, c.cluster_name, c.description,
 r.attributes->>'$.group_replication_group_name' as group_name
 FROM mysql_innodb_cluster_metadata.clusters c
 JOIN mysql_innodb_cluster_metadata.replicasets r
  ON c.cluster_id = r.cluster_id)*";

bool MetadataStorage::get_cluster(Cluster_id cluster_id,
                                  Cluster_metadata *out_cluster) {
  auto result = execute_sqlf(
      std::string(k_select_cluster_metadata) + " WHERE c.cluster_id = ?",
      cluster_id);
  if (auto row = result->fetch_one_named()) {
    *out_cluster = unserialize_cluster_metadata(row);
    return true;
  }

  return false;
}

bool MetadataStorage::query_cluster_attribute(Cluster_id cluster_id,
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

void MetadataStorage::update_cluster_attribute(Cluster_id cluster_id,
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

bool MetadataStorage::get_cluster_for_group_name(
    const std::string &group_name, Cluster_metadata *out_cluster) {
  auto result = execute_sqlf(
      std::string(k_select_cluster_metadata) + " WHERE c.group_name = ?",
      group_name);
  if (auto row = result->fetch_one_named()) {
    *out_cluster = unserialize_cluster_metadata(row);
    return true;
  }

  return false;
}

bool MetadataStorage::get_cluster_for_server_uuid(
    const std::string &server_uuid, Cluster_metadata *out_cluster) {
  auto result =
      execute_sqlf(std::string(k_select_cluster_metadata) +
                       " JOIN mysql_innodb_cluster_metadata.instances i"
                       "  ON i.replicaset_id = r.replicaset_id"
                       " WHERE i.mysql_server_uuid = ?",
                   server_uuid);
  if (auto row = result->fetch_one_named()) {
    *out_cluster = unserialize_cluster_metadata(row);
    return true;
  }

  return false;
}

bool MetadataStorage::get_cluster_for_cluster_name(
    const std::string &name, Cluster_metadata *out_cluster) {
  auto result = execute_sqlf(
      std::string(k_select_cluster_metadata) + " WHERE c.cluster_name = ?",
      name);
  if (auto row = result->fetch_one_named()) {
    *out_cluster = unserialize_cluster_metadata(row);
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

  Transaction tx(this);

  try {
    auto result = execute_sqlf(
        "INSERT INTO mysql_innodb_cluster_metadata.clusters "
        "(cluster_name, description, attributes)"
        " VALUES (?, ?, '{}')",
        cluster->get_name(), cluster->get_description());

    cluster_id = result->get_auto_increment_value();
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

  shcore::sqlstring query(
      "INSERT INTO mysql_innodb_cluster_metadata.replicasets "
      "(cluster_id, replicaset_type, topology_type, replicaset_name, "
      "active, attributes) "
      "VALUES (?, ?, ?, ?, ?,"
      "JSON_OBJECT('adopted', ?, "
      "            'group_replication_group_name', ?))",
      0);

  // Insert the default ReplicaSet on the replicasets table
  query << cluster_id << "gr" << cluster->get_topology_type()
        << cluster->get_default_replicaset()->get_name() << 1;
  query << (adopted ? "1" : "0");
  query << cluster->get_group_name();
  query.done();

  auto result = execute_sql(query);
  auto rs_id = result->get_auto_increment_value();

  // Insert the default ReplicaSet on the replicasets table
  query = shcore::sqlstring(
      "UPDATE mysql_innodb_cluster_metadata.clusters "
      "SET default_replicaset = ? WHERE cluster_id = ?",
      0);
  query << rs_id << cluster_id;
  query.done();

  execute_sql(query);

  tx.commit();

  return cluster_id;
}

Instance_id MetadataStorage::insert_instance(
    const Instance_metadata &instance) {
  auto insert_host = [this](const std::string &host,
                            const std::string &ip_address,
                            const std::string &location) -> uint32_t {
    shcore::sqlstring query;

    // check if the host is already registered
    {
      query = shcore::sqlstring(
          "SELECT host_id, host_name, ip_address"
          " FROM mysql_innodb_cluster_metadata.hosts"
          " WHERE host_name = ? OR (ip_address <> '' AND ip_address = ?)",
          0);
      query << host << ip_address;
      query.done();
      auto result(execute_sql(query));
      if (result) {
        auto row = result->fetch_one();
        if (row) {
          uint32_t host_id = row->get_uint(0);

          log_info("Found host entry %u in metadata for host %s (%s)", host_id,
                   host.c_str(), ip_address.c_str());
          return host_id;
        }
      }
    }

    // Insert the default ReplicaSet on the replicasets table
    query = shcore::sqlstring(
        "INSERT INTO mysql_innodb_cluster_metadata.hosts "
        "(host_name, ip_address, location) VALUES (?, ?, ?)",
        0);
    query << host;
    query << ip_address;
    query << location;
    query.done();

    auto result(execute_sql(query));
    return result->get_auto_increment_value();
  };

  shcore::sqlstring query;

  Transaction tx(this);

  // Insert the default ReplicaSet on the replicasets table
  auto host_id = insert_host(instance.address, "", "");

  auto result = execute_sqlf(
      "SELECT c.default_replicaset"
      " FROM mysql_innodb_cluster_metadata.clusters c"
      " WHERE c.cluster_id = ?",
      instance.cluster_id);
  uint32_t rs_id = result->fetch_one_or_throw()->get_uint(0);

  // If the x-plugin is disabled, we do not story any value for mysqlX
  if (!instance.xendpoint.empty()) {
    query = shcore::sqlstring(
        "INSERT INTO mysql_innodb_cluster_metadata.instances "
        "(host_id, replicaset_id, mysql_server_uuid, "
        "instance_name, role, addresses, attributes)"
        "VALUES (?, ?, ?, ?, ?, json_object('mysqlClassic', ?, "
        "'mysqlX', ?, 'grLocal', ?), '{}')",
        0);
  } else {
    query = shcore::sqlstring(
        "INSERT INTO mysql_innodb_cluster_metadata.instances "
        "(host_id, replicaset_id, mysql_server_uuid, "
        "instance_name, role, addresses, attributes) "
        "VALUES (?, ?, ?, ?, ?, json_object('mysqlClassic', ?, 'grLocal', ?), "
        "'{}')",
        0);
  }

  query << host_id;
  query << rs_id;
  query << instance.uuid;
  query << instance.label;
  query << instance.role_type;
  query << instance.endpoint;

  if (!instance.xendpoint.empty()) query << instance.xendpoint;

  query << instance.grendpoint;
  query.done();

  result = execute_sql(query);

  tx.commit();

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
        "SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances "
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
    Cluster_id cluster_id = 0;
    uint64_t rs_id = 0;
    shcore::sqlstring query;

    // Get the Cluster ID
    query = shcore::sqlstring(
        "SELECT cluster_id, default_replicaset FROM "
        "mysql_innodb_cluster_metadata.clusters "
        "WHERE cluster_name = ?",
        0);
    query << full_cluster_name;
    query.done();

    auto result = execute_sql(query);
    auto row = result->fetch_one();
    if (row) {
      cluster_id = row->get_uint(0);
      rs_id = row->get_uint(1);
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
    return std::make_pair(cluster_id, rs_id);
  };

  Transaction tx(this);

  // It exists, so let's get the cluster_id and move on
  Cluster_id cluster_id;
  uint64_t rs_id;
  std::tie(cluster_id, rs_id) = get_cluster_id(cluster_name);

  {
    // Set the default_replicaset to NULL (needed to avoid foreign key
    // constraint error when removing replicasets data).
    execute_sqlf(
        "UPDATE mysql_innodb_cluster_metadata.clusters SET "
        "default_replicaset = NULL WHERE cluster_id = ?",
        cluster_id);

    execute_sqlf(
        "DELETE FROM mysql_innodb_cluster_metadata.instances "
        "WHERE replicaset_id = ?",
        rs_id);

    execute_sqlf(
        "DELETE FROM mysql_innodb_cluster_metadata.replicasets "
        "WHERE cluster_id = ?",
        cluster_id);
  }

  // Remove the cluster
  execute_sqlf(
      "DELETE from mysql_innodb_cluster_metadata.clusters "
      "WHERE cluster_id = ?",
      cluster_id);

  tx.commit();
}

void MetadataStorage::update_cluster_name(Cluster_id cluster_id,
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
size_t MetadataStorage::get_cluster_size(Cluster_id cluster_id) const {
  shcore::sqlstring query;

  query = shcore::sqlstring(
      "SELECT COUNT(*) as count"
      " FROM mysql_innodb_cluster_metadata.instances i"
      " JOIN mysql_innodb_cluster_metadata.replicasets r"
      " ON i.replicaset_id = r.replicaset_id"
      " WHERE r.cluster_id = ?",
      0);
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

bool MetadataStorage::is_instance_on_cluster(Cluster_id cluster_id,
                                             const std::string &address) {
  shcore::sqlstring query;

  query = shcore::sqlstring(
      "SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances i"
      " JOIN mysql_innodb_cluster_metadata.replicasets r"
      " ON i.replicaset_id = r.replicaset_id"
      " WHERE r.cluster_id = ? AND i.addresses->'$.mysqlClassic' = ?",
      0);
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

bool MetadataStorage::is_instance_label_unique(Cluster_id cluster_id,
                                               const std::string &label) const {
  shcore::sqlstring query = shcore::sqlstring{
      "SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances i"
      " JOIN mysql_innodb_cluster_metadata.replicasets r"
      " ON i.replicaset_id = r.replicaset_id"
      " WHERE r.cluster_id = ? AND i.instance_name = ?",
      0};
  query << cluster_id;
  query << label;
  query.done();

  return 0 == execute_sql(query)->fetch_one_or_throw()->get_int(0);
}

void MetadataStorage::set_instance_label(Cluster_id cluster_id,
                                         const std::string &label,
                                         const std::string &new_label) {
  shcore::sqlstring query;

  // Check if the label exists
  if (is_instance_label_unique(cluster_id, label)) {
    throw shcore::Exception::logic_error("The instance with the label '" +
                                         label + "' does not exist.");
  } else {
    auto result = execute_sqlf(
        "SELECT default_replicaset"
        " FROM mysql_innodb_cluster_metadata.clusters"
        " WHERE cluster_id = ?",
        cluster_id);
    auto rs_id = result->fetch_one_or_throw()->get_uint(0);

    query = shcore::sqlstring(
        "UPDATE mysql_innodb_cluster_metadata.instances SET "
        "instance_name = ? WHERE replicaset_id = ? AND instance_name = ?",
        0);
    query << new_label;
    query << rs_id;
    query << label;
    query.done();

    execute_sql(query);
  }
}

namespace {
Instance_metadata unserialize_instance(
    const mysqlshdk::db::Row_ref_by_name &row) {
  Instance_metadata instance;

  instance.id = row.get_uint("instance_id");
  instance.cluster_id = row.get_uint("cluster_id");
  if (row.has_field("group_name"))
    instance.group_name = row.get_string("group_name", "");
  instance.uuid = row.get_string("mysql_server_uuid", "");
  instance.label = row.get_string("label", "");
  instance.endpoint = row.get_string("endpoint", "");
  instance.xendpoint = row.get_string("xendpoint", "");
  if (row.has_field("grendpoint"))
    instance.grendpoint = row.get_string("grendpoint", "");
  instance.role_type = row.get_string("role", "");

  return instance;
}
}  // namespace

constexpr const char *k_base_instance_query =
    "SELECT i.instance_id, r.cluster_id, i.role,"
    " r.attributes->>'$.group_replication_group_name' group_name,"
    " i.instance_name label, i.mysql_server_uuid, "
    " i.addresses->>'$.mysqlClassic' endpoint,"
    " i.addresses->>'$.mysqlX' xendpoint,"
    " i.addresses->>'$.grEndpoint' grendpoint"
    " FROM mysql_innodb_cluster_metadata.instances i"
    " LEFT JOIN mysql_innodb_cluster_metadata.replicasets r"
    "   ON r.replicaset_id = i.replicaset_id";

std::vector<Instance_metadata> MetadataStorage::get_all_instances(
    Cluster_id cluster_id) {
  std::vector<Instance_metadata> ret_val;

  auto result = cluster_id == 0
                    ? execute_sql(k_base_instance_query)
                    : execute_sqlf(std::string(k_base_instance_query) +
                                       " WHERE r.cluster_id = ?",
                                   cluster_id);

  while (auto row = result->fetch_one_named()) {
    ret_val.push_back(unserialize_instance(row));
  }

  return ret_val;
}

Instance_metadata MetadataStorage::get_instance_by_uuid(
    const std::string &uuid) {
  auto result = execute_sqlf(
      std::string(k_base_instance_query) + " WHERE i.mysql_server_uuid = ?",
      uuid);

  if (auto row = result->fetch_one_named()) {
    return unserialize_instance(row);
  }

  throw shcore::Exception("Metadata for instance " + uuid + " not found",
                          SHERR_DBA_MEMBER_METADATA_MISSING);
}

Instance_metadata MetadataStorage::get_instance_by_endpoint(
    const std::string &instance_address) {
  auto result = execute_sqlf(std::string(k_base_instance_query) +
                                 " WHERE i.addresses->>'$.mysqlClassic' = ?",
                             instance_address);

  if (auto row = result->fetch_one_named()) {
    return unserialize_instance(row);
  }

  throw shcore::Exception(
      "Metadata for instance " + instance_address + " not found",
      SHERR_DBA_MEMBER_METADATA_MISSING);
}

mysqlshdk::gr::Topology_mode MetadataStorage::get_cluster_topology_mode(
    Cluster_id cluster_id) {
  // Execute query to obtain the topology mode from the metadata.
  shcore::sqlstring query = shcore::sqlstring{
      "SELECT topology_type FROM mysql_innodb_cluster_metadata.replicasets "
      "WHERE cluster_id = ?",
      0};
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
    Cluster_id cluster_id, const mysqlshdk::gr::Topology_mode &topology_mode) {
  // Convert topology mode to metadata value.
  std::string topology_mode_str;
  if (topology_mode == mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY) {
    topology_mode_str = "pm";
  } else if (topology_mode == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY) {
    topology_mode_str = "mm";
  }

  // Execute query to update topology mode on metadata.
  shcore::sqlstring query = shcore::sqlstring{
      "UPDATE mysql_innodb_cluster_metadata.replicasets SET topology_type = ?"
      " WHERE cluster_id = ?",
      0};
  query << topology_mode_str;
  query << cluster_id;
  query.done();

  execute_sql(query);
}

}  // namespace dba
}  // namespace mysqlsh
