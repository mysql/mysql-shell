/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "utils/utils_sqlstring.h"
#include "mod_dba_metadata_storage.h"
#include "modules/adminapi/metadata-model_definitions.h"
//#include "modules/adminapi/mod_dba_instance.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mysql_connection.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "mysqlx_connection.h" // for error codes
#include "mysqlxtest/password_hasher.h"

#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include <random>

// How many times to retry a query if it fails because it's SUPER_READ_ONLY
static const int kMaxReadOnlyRetries = 10;

using namespace mysqlsh;
using namespace mysqlsh::dba;
using namespace shcore;

MetadataStorage::MetadataStorage(std::shared_ptr<mysqlsh::ShellDevelopmentSession> session) :
  _session(session) {}

MetadataStorage::~MetadataStorage() {}

std::shared_ptr<mysql::ClassicResult> MetadataStorage::execute_sql(const std::string &sql, bool retry, const std::string &log_sql) const {
  shcore::Value ret_val;

  if (log_sql.empty())
    log_debug("DBA: execute_sql('%s'", sql.c_str());
  else
    log_debug("DBA: execute_sql('%s'", log_sql.c_str());

  if (!_session)
    throw Exception::metadata_error("The Metadata is inaccessible");

  int retry_count = kMaxReadOnlyRetries;
  while (retry_count > 0) {
    try {
      ret_val = _session->execute_sql(sql, shcore::Argument_list());

      // If reached here it means there were no errors
      retry_count = 0;
    } catch (shcore::Exception& e) {
      if (CR_SERVER_GONE_ERROR == e.code()) {
        log_debug("%s", e.format().c_str());
        log_debug("DBA: The Metadata is inaccessible");
        throw Exception::metadata_error("The Metadata is inaccessible");
      } else if (retry && retry_count > 0 && e.code() == 1290) { // SUPER_READ_ONLY enabled
        log_info("%s: retrying after 1s...\n", e.format().c_str());
#ifdef HAVE_SLEEP
        sleep(1);
#elif defined(WIN32)
        Sleep(1000);
#endif
        retry_count--;
      } else {
        log_debug("%s", e.format().c_str());
        throw;
      }
    }
  }

  return ret_val.as_object<mysql::ClassicResult>();
}

void MetadataStorage::set_session(std::shared_ptr<mysqlsh::ShellDevelopmentSession> session){
  _session = session;
};

void MetadataStorage::start_transaction() {
  _session->start_transaction();
}

void MetadataStorage::commit() {
  _session->commit();
}

void MetadataStorage::rollback() {
  _session->rollback();
}

bool MetadataStorage::metadata_schema_exists() {
  std::string found_object;
  std::string type = "Schema";
  std::string search_name = "mysql_innodb_cluster_metadata";

  if (_session)
    found_object = _session->db_object_exists(type, search_name, "");
  else
    throw shcore::Exception::logic_error("");

  return !found_object.empty();
}

void MetadataStorage::create_metadata_schema() {
  if (!metadata_schema_exists()) {
    std::string query = shcore::md_model_sql;

    size_t pos = 0;
    std::string token, delimiter = ";\n";

    while ((pos = query.find(delimiter)) != std::string::npos) {
      token = query.substr(0, pos);

      execute_sql(token);

      query.erase(0, pos + delimiter.length());
    }
  } else {
    // Check the Schema version and update the schema accordingly
  }
}

void MetadataStorage::drop_metadata_schema() {
  execute_sql("DROP SCHEMA mysql_innodb_cluster_metadata");
}

uint64_t MetadataStorage::get_cluster_id(const std::string &cluster_name) {
  uint64_t cluster_id = 0;
  shcore::sqlstring query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster ID
  query = shcore::sqlstring("SELECT cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name = ?", 0);
  query << cluster_name;
  query.done();

  auto result = execute_sql(query);
  auto row = result->fetch_one();
  if (row)
    cluster_id = row->get_value(0).as_uint();
  return cluster_id;
}

uint64_t MetadataStorage::get_cluster_id(uint64_t rs_id) {
  uint64_t cluster_id = 0;
  shcore::sqlstring query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster ID
  query = shcore::sqlstring("SELECT cluster_id from mysql_innodb_cluster_metadata.replicasets where replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  auto result = execute_sql(query);
  auto row = result->fetch_one();
  if (row)
    cluster_id = row->get_value(0).as_uint();
  return cluster_id;
}

bool MetadataStorage::cluster_exists(const std::string &cluster_name) {
  /*
   * To check if the cluster exists, we can use get_cluster_id
   * and simply check for its return value.
   * If zero, it means the cluster does not exist (cluster_id cannot be zero)
   */

  if (get_cluster_id(cluster_name))
    return true;

  return false;
}

void MetadataStorage::insert_cluster(const std::shared_ptr<Cluster> &cluster) {
  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Check if the Cluster has some description
  shcore::sqlstring query("INSERT INTO mysql_innodb_cluster_metadata.clusters (cluster_name, description, options, attributes) "\
                          "VALUES (?, ?, ?, ?)", 0);
  query << cluster->get_name()
        << cluster->get_description()
        << cluster->get_options()
        << cluster->get_attributes();
  query.done();
  // Insert the Cluster on the cluster table
  try {
    auto result = execute_sql(query);
    cluster->set_id(result->get_member("autoIncrementValue").as_int());
  } catch (shcore::Exception &e) {
    if (e.what() == "Duplicate entry '" + cluster->get_name() + "' for key 'cluster_name'") {
      log_debug("DBA: A Cluster with the name '%s' already exists", (cluster->get_name()).c_str());
      throw Exception::argument_error("A Cluster with the name '" + cluster->get_name() + "' already exists.");
    } else
      throw;
  }
}

void MetadataStorage::insert_replica_set(std::shared_ptr<ReplicaSet> replicaset,
    bool is_default, bool is_adopted) {
  shcore::sqlstring query("INSERT INTO mysql_innodb_cluster_metadata.replicasets "
                          "(cluster_id, replicaset_type, topology_type, replicaset_name, active, attributes) "
                          "VALUES (?, ?, ?, ?, ?, IF(?, JSON_OBJECT('adopted', 'true'), '{}'))", 0);
  uint64_t cluster_id;

  cluster_id = replicaset->get_cluster()->get_id();

  // Insert the default ReplicaSet on the replicasets table
  query << cluster_id << "gr" << replicaset->get_topology_type() << "default" << 1;
  query << (is_adopted ? "1" : "0");
  query.done();

  auto result = execute_sql(query);

  // Update the replicaset_id
  uint64_t rs_id = 0;
  rs_id = result->get_member("autoIncrementValue").as_uint();

  // Update the cluster entry with the replicaset_id
  replicaset->set_id(rs_id);

  // Insert the default ReplicaSet on the replicasets table
  query = shcore::sqlstring("UPDATE mysql_innodb_cluster_metadata.clusters SET default_replicaset = ?"
                            " WHERE cluster_id = ?", 0);
  query << rs_id << cluster_id;
  query.done();

  execute_sql(query);
}

uint32_t MetadataStorage::insert_host(const shcore::Value::Map_type_ref &options) {
  std::string uri;

  std::string host_name;
  std::string ip_address;
  std::string location;

  shcore::sqlstring query;

  if (options->has_key("host"))
    host_name = (*options)["host"].as_string();

  if (options->has_key("id_address"))
    ip_address = (*options)["id_address"].as_string();

  if (options->has_key("location"))
    location = (*options)["location"].as_string();

  // check if the host is already registered
  {
    query = shcore::sqlstring("SELECT host_id, host_name, ip_address"
        " FROM mysql_innodb_cluster_metadata.hosts"
        " WHERE host_name = ? OR (ip_address <> '' AND ip_address = ?)", 0);
    query << host_name << ip_address;
    query.done();
    auto result(execute_sql(query, false, ""));
    if (result) {
      auto row = result->fetch_one();
      if (row) {
        uint32_t host_id = static_cast<uint32_t>(row->get_value(0).as_uint());

        log_info("Found host entry %u in metadata for host %s (%s)",
                  host_id, host_name.c_str(), ip_address.c_str());
        return host_id;
      }
    }
  }

  // Insert the default ReplicaSet on the replicasets table
  query = shcore::sqlstring("INSERT INTO mysql_innodb_cluster_metadata.hosts (host_name, ip_address, location) VALUES (?, ?, ?)", 0);
  query << host_name;
  query << ip_address;
  query << location;
  query.done();

  // execute and keep retrying if the server is super-readonly
  // possibly because it's recovering
  auto result(execute_sql(query, true));
  return static_cast<uint32_t>(result->get_member("autoIncrementValue").as_int());
}

void MetadataStorage::insert_instance(const shcore::Value::Map_type_ref& options, uint64_t host_id, uint64_t rs_id) {
  std::string mysql_server_uuid;
  std::string instance_label;
  std::string role;
  shcore::Value::Map_type_ref attributes;
  std::string endpoint;
  std::string xendpoint;
  std::string grendpoint;

  shcore::sqlstring query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  if (host_id == 0)
    host_id = (*options)["host_id"].as_uint();

  if (rs_id == 0)
    rs_id = (*options)["replicaset_id"].as_uint();

  mysql_server_uuid = (*options)["mysql_server_uuid"].as_string();
  instance_label = (*options)["label"].as_string();

  if (options->has_key("role"))
    role = (*options)["role"].as_string();

  if (options->has_key("endpoint"))
    endpoint = (*options)["endpoint"].as_string();

  if (options->has_key("xendpoint"))
    xendpoint = (*options)["xendpoint"].as_string();

  if (options->has_key("grendpoint"))
    grendpoint = (*options)["grendpoint"].as_string();

  if (options->has_key("attributes"))
    attributes = (*options)["attributes"].as_map();

  query = shcore::sqlstring(
        "INSERT INTO mysql_innodb_cluster_metadata.instances "
        "(host_id, replicaset_id, mysql_server_uuid, "
        "instance_name, role, addresses) "
        "VALUES (?, ?, ?, ?, ?, json_object('mysqlClassic', ?, "
        "'mysqlX', ?, 'grLocal', ?))", 0);

  query << host_id;
  query << rs_id;
  query << mysql_server_uuid;
  query << instance_label;
  query << role;
  query << endpoint;
  query << xendpoint;
  query << grendpoint;
  query.done();

  execute_sql(query);
}

void MetadataStorage::remove_instance(const std::string &instance_address) {
  shcore::sqlstring query;

  // Remove the instance
  query = shcore::sqlstring("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE addresses->\"$.mysqlClassic\" = ?", 0);
  query << instance_address;
  query.done();

  execute_sql(query);
}

void MetadataStorage::drop_cluster(const std::string &cluster_name) {
  shcore::sqlstring query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Check if the Cluster exists
  if (!cluster_exists(cluster_name))
    throw Exception::logic_error("The cluster with the name '" + cluster_name + "' does not exist.");

  // It exists, so let's get the cluster_id and move on
  else {
    uint64_t cluster_id = get_cluster_id(cluster_name);

    // Check if the Cluster is empty
    query = shcore::sqlstring("SELECT * from mysql_innodb_cluster_metadata.replicasets where cluster_id = ?", 0);
    query << cluster_id;
    query.done();

    auto result = execute_sql(query);

    auto row = result->fetch_one();

    //result->flush();

    if (row)
      throw Exception::logic_error("The cluster with the name '" + cluster_name + "' is not empty.");

    // OK the cluster exists and is empty, we can remove it
    query = shcore::sqlstring("DELETE from mysql_innodb_cluster_metadata.clusters where cluster_id = ?", 0);
    query << cluster_id;
    query.done();

    execute_sql(query);
  }
}

bool MetadataStorage::cluster_has_default_replicaset_only(const std::string &cluster_name) {
  shcore::sqlstring query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster ID
  uint64_t cluster_id = get_cluster_id(cluster_name);

  // Check if the Cluster has only one replicaset
  query = shcore::sqlstring("SELECT count(*) as count FROM mysql_innodb_cluster_metadata.replicasets WHERE cluster_id = ? AND replicaset_name <> 'default'", 0);
  query << cluster_id;
  query.done();

  auto result = execute_sql(query);

  auto row = result->fetch_one();
  int count = 0;
  if (row) {
    count = row->get_value(0).as_int();
  }

  return count == 0;
}

bool MetadataStorage::is_cluster_empty(uint64_t cluster_id) {
  shcore::sqlstring query;

  query = shcore::sqlstring("SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.replicasets WHERE cluster_id = ?", 0);
  query << cluster_id;
  query.done();

  auto result = execute_sql(query);

  auto row = result->fetch_one();
  uint64_t count = 0;
  if (row) {
    count = row->get_value(0).as_int();
  }

  return count == 0;
}

void MetadataStorage::drop_replicaset(uint64_t rs_id) {
  shcore::sqlstring query;
  bool default_rs = false;
  std::string rs_name;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  Transaction tx(shared_from_this());

  // Check if this is the Default replicaset
  query = shcore::sqlstring("SELECT replicaset_name FROM mysql_innodb_cluster_metadata.replicasets WHERE replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  auto result = execute_sql(query);

  auto row = result->fetch_one();
  if (row) {
    rs_name = row->get_value_as_string(0);
  }

  if (rs_name == "default")
    default_rs = true;

  if (default_rs) {
    // Set the default_replicaset as NULL
    uint64_t cluster_id = get_cluster_id(rs_id);
    query = shcore::sqlstring("UPDATE mysql_innodb_cluster_metadata.clusters SET default_replicaset = NULL WHERE cluster_id = ?", 0);
    query << cluster_id;
    query.done();
    execute_sql(query);
  }

  // Delete the associated instances
  query = shcore::sqlstring("delete from mysql_innodb_cluster_metadata.instances where replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  execute_sql(query);

  // Delete the replicaset
  query = shcore::sqlstring("delete from mysql_innodb_cluster_metadata.replicasets where replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  execute_sql(query);

  tx.commit();
}

void MetadataStorage::disable_replicaset(uint64_t rs_id) {
  shcore::sqlstring query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Set active as False
  query = shcore::sqlstring("UPDATE mysql_innodb_cluster_metadata.replicasets SET active = ?"
                            " WHERE replicaset_id = ?", 0);
  query << 0 << rs_id;
  query.done();

  execute_sql(query);
}

bool MetadataStorage::is_replicaset_active(uint64_t rs_id) {
  shcore::sqlstring query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  query = shcore::sqlstring("SELECT active FROM mysql_innodb_cluster_metadata.replicasets WHERE replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  auto result = execute_sql(query);

  auto row = result->fetch_one();
  int active = 0;
  if (row) {
    active = row->get_value(0).as_int();
  }

  return active == 1;
}

std::string MetadataStorage::get_replicaset_group_name() {
  std::string group_name;

  std::string query("SELECT @@group_replication_group_name");

  // Any error will bubble up right away
  auto result = execute_sql(query);
  auto row = result->fetch_one();
  if (row) {
    group_name = row->get_value_as_string(0);
  }
  return group_name;
}

void MetadataStorage::set_replicaset_group_name(std::shared_ptr<ReplicaSet> replicaset,
      const std::string &group_name) {
  uint64_t rs_id;

  rs_id = replicaset->get_id();

  shcore::sqlstring query("UPDATE mysql_innodb_cluster_metadata.replicasets SET attributes = json_set(attributes, '$.group_replication_group_name', ?)"
                          " WHERE replicaset_id = ?", 0);

  query << group_name << rs_id;
  query.done();

  execute_sql(query);
}

std::shared_ptr<ReplicaSet> MetadataStorage::get_replicaset(uint64_t rs_id) {
  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  shcore::sqlstring query("SELECT replicaset_name, topology_type"
                          " FROM mysql_innodb_cluster_metadata.replicasets"
                          " WHERE replicaset_id = ?", 0);
  query << rs_id;
  auto result = execute_sql(query);
  auto row = result->fetch_one();
  if (row) {
    std::string rs_name = row->get_value(0).as_string();
    std::string topo = row->get_value(1).as_string();

    // Create a ReplicaSet Object to match the Metadata
    std::shared_ptr<ReplicaSet> rs(new ReplicaSet("name", topo, shared_from_this()));
    // Get and set the Metadata data
    rs->set_id(rs_id);
    rs->set_name(rs_name);
    return rs;
  }
  throw Exception::metadata_error("Unknown replicaset " + std::to_string(rs_id));
}

std::shared_ptr<Cluster> MetadataStorage::get_cluster_from_query(const std::string &query) {
  std::shared_ptr<Cluster> cluster;

  try {
    auto result = execute_sql(query);
    auto row = result->fetch_one();

    if (row) {
      shcore::Argument_list args;

      cluster.reset(new Cluster(row->get_value(1).as_string(), shared_from_this()));

      cluster->set_id(row->get_value(0).as_int());
      cluster->set_description(row->get_value(3).as_string());
      cluster->set_options(row->get_value(4).as_string());
      cluster->set_attributes(row->get_value(5).as_string());

      auto rsetid_val = row->get_value(2);
      if (rsetid_val)
        cluster->set_default_replicaset(get_replicaset(rsetid_val.as_int()));
    }
  } catch (shcore::Exception &e) {
    std::string error = e.what();

    if (error == "Table 'mysql_innodb_cluster_metadata.clusters' doesn't exist") {
      log_debug("Metadata Schema does not exist.");
      throw Exception::metadata_error("Metadata Schema does not exist.");
    } else {
      throw;
    }
  }

  return cluster;
}

std::shared_ptr<Cluster> MetadataStorage::get_cluster_matching(const std::string &condition, const std::string &value) {
  shcore::sqlstring query;
  std::string raw_query;

  raw_query = "SELECT cluster_id, cluster_name, default_replicaset, description, options, attributes " \
              "FROM mysql_innodb_cluster_metadata.clusters " \
              "WHERE " + condition + " = ?";

  query = shcore::sqlstring(raw_query.c_str(), 0);
  query << value;
  query.done();

  return get_cluster_from_query(query);
}

std::shared_ptr<Cluster> MetadataStorage::get_cluster_matching(const std::string &condition, bool value) {
  std::string query;
  std::string str_value = value ? "true" : "false";

  query = "SELECT cluster_id, cluster_name, default_replicaset, description, options, attributes " \
          "FROM mysql_innodb_cluster_metadata.clusters " \
          "WHERE " + condition + " = " + str_value;

  return get_cluster_from_query(query);
}

std::shared_ptr<Cluster> MetadataStorage::get_default_cluster() {
  return get_cluster_matching("attributes->'$.default'", true);
}

std::shared_ptr<Cluster> MetadataStorage::get_cluster(const std::string &cluster_name) {
  std::shared_ptr<Cluster> cluster = get_cluster_matching("cluster_name", cluster_name);

  if (!cluster)
    throw Exception::logic_error("The cluster with the name '" + cluster_name + "' does not exist.");

  return cluster;
}

bool MetadataStorage::has_default_cluster() {
  bool ret_val = false;

  if (metadata_schema_exists()) {
    auto result = execute_sql("SELECT cluster_id from mysql_innodb_cluster_metadata.clusters WHERE attributes->\"$.default\" = true");

    auto row = result->fetch_one();
    if (row)
      ret_val = true;
  }
  return ret_val;
}

bool MetadataStorage::is_replicaset_empty(uint64_t rs_id) {
  shcore::sqlstring query;

  query = shcore::sqlstring("SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances WHERE replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  auto result = execute_sql(query);

  auto row = result->fetch_one();
  uint64_t count = 0;
  if (row) {
    count = row->get_value(0).as_int();
  }

  return count == 0;
}

bool MetadataStorage::is_instance_on_replicaset(uint64_t rs_id, const std::string &address) {
  shcore::sqlstring query;

  query = shcore::sqlstring("SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances WHERE replicaset_id = ? AND addresses->\"$.mysqlClassic\" = ?", 0);
  query << rs_id;
  query << address;
  query.done();

  auto result = execute_sql(query);

  auto row = result->fetch_one();
  uint64_t count = 0;
  if (row) {
    count = row->get_value(0).as_int();
  }
  return count == 1;
}

std::string MetadataStorage::get_seed_instance(uint64_t rs_id) {
  std::string seed_address, query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster instanceAdminUser

  //query = "SELECT JSON_UNQUOTE(addresses->\"$.mysqlClassic\")  as address FROM mysql_innodb_cluster_metadata.instances WHERE replicaset_id = '" + std::to_string(rs_id) + "' AND role = 'HA'";
  query = "SELECT JSON_UNQUOTE(i.addresses->\"$.mysqlClassic\") as address "
          " FROM performance_schema.replication_group_members g"
          " JOIN mysql_innodb_cluster_metadata.instances i ON g.member_id = i.mysql_server_uuid"
          " WHERE g.member_state = 'ONLINE'";

  auto result = execute_sql(query);

  auto row = result->fetch_one();
  if (row) {
    seed_address = row->get_value_as_string(0);
  }
  return seed_address;
}

std::shared_ptr<shcore::Value::Array_type> MetadataStorage::get_replicaset_instances(uint64_t rs_id) {
  shcore::sqlstring query;

  query = shcore::sqlstring("select mysql_server_uuid, instance_name, role,"
                            " JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) as host"
                            " from mysql_innodb_cluster_metadata.instances"
                            " where replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  auto result = execute_sql(query);
  auto raw_instances = result->call("fetchAll", shcore::Argument_list());
  auto instances = raw_instances.as_array();

  return instances;
}

std::shared_ptr<shcore::Value::Array_type> MetadataStorage::get_replicaset_online_instances(uint64_t rs_id) {
  shcore::sqlstring query;

  query = shcore::sqlstring("SELECT mysql_server_uuid, instance_name, role,"
                            " JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) as host"
                            " FROM performance_schema.replication_group_members g"
                            " JOIN mysql_innodb_cluster_metadata.instances i ON g.member_id = i.mysql_server_uuid"
                            " WHERE g.member_state = 'ONLINE'"
                            " AND replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  auto result = execute_sql(query);
  auto raw_instances = result->call("fetchAll", shcore::Argument_list());
  auto instances = raw_instances.as_array();

  return instances;
}

shcore::Value::Map_type_ref MetadataStorage::get_instance(
      const std::string &instance_address) {
  shcore::sqlstring query;
  Value::Map_type_ref ret_val(new shcore::Value::Map_type);
  uint64_t host_id, replicaset_id;
  // int version_token;
  std::string mysql_server_uuid, instance_name, role,
              endpoint, xendpoint, grendpoint;
  // std::string weight, description;
  shcore::Value::Map_type_ref attributes;

  query = shcore::sqlstring(
      "SELECT host_id, replicaset_id, mysql_server_uuid, "\
      "instance_name, role, weight, "\
      "JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) as endpoint, "\
      "JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlX\")) as xendpoint, "\
      "JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.grLocal\")) as grendpoint, "\
      "addresses, attributes, version_token, description "
      "FROM mysql_innodb_cluster_metadata.instances "\
      "WHERE addresses->\"$.mysqlClassic\" = ?", 0);

  query << instance_address;
  query.done();

  auto result = execute_sql(query);
  auto row = result->fetch_one();

  if (row) {
    host_id = row->get_value(0);
    replicaset_id = row->get_value(1);
    mysql_server_uuid = row->get_value(2).as_string();
    instance_name = row->get_value(3).as_string();
    role = row->get_value(4).as_string();
    // weight = row->get_value(5).as_float();
    endpoint = row->get_value(6).as_string();
    xendpoint = row->get_value(7).as_string();
    grendpoint = row->get_value(8).as_string();
    // attributes = row->get_value(10).as_map();
    // version_token = row->get_value(11).as_int();
    // description = row->get_value(12).as_string();

    (*ret_val)["host_id"] = Value(host_id);
    (*ret_val)["replicaset_id"] = Value(replicaset_id);
    (*ret_val)["mysql_server_uuid"] = Value(mysql_server_uuid);
    (*ret_val)["label"] = Value(instance_name);
    (*ret_val)["role"] = Value(role);
    // (*ret_val)["weight"] = Value(weight);
    (*ret_val)["endpoint"] = Value(endpoint);
    (*ret_val)["xendpoint"] = Value(xendpoint);
    (*ret_val)["grendpoint"] = Value(grendpoint);
    // (*ret_val)["attributes"] = Value(attributes);
    // (*ret_val)["version_token"] = Value(version_token);
    // (*ret_val)["description"] = Value(description);

    return ret_val;
  }

  throw Exception::metadata_error("The instance with the address '" +
                                  instance_address + "' does not exist.");
}

void MetadataStorage::create_account(const std::string &username,
                                     const std::string &password,
                                     const std::string &hostname,
                                     bool password_hashed) {
  std::string query_to_escape, query_log;
  shcore::sqlstring query;

  query_to_escape.append("CREATE USER IF NOT EXISTS ?@?");

  query_log = query_to_escape;

  query_to_escape.append(" IDENTIFIED ");
  query_to_escape.append((password_hashed ?
                          "WITH mysql_native_password AS ?" : "BY ?"));

  query_log + std::string(password.length(), '*');
  query_log.append("'");

  query = shcore::sqlstring(query_to_escape.c_str(), 0);
  query << username;
  query << hostname;
  query << password;
  query.done();

  execute_sql(query, false, query_log);
}

// generate a replication user account + password for an instance
// This account will be replicated to all instances in the replicaset, so that
// the newly joining instance can connect to any of them for recovery.
void MetadataStorage::create_repl_account(std::string &username,
                                          std::string &password) {
  shcore::sqlstring query;
  bool account_created = false;
  // Generate a password
  password = generate_password();

  MySQL_timer timer;
  std::string tstamp = std::to_string(timer.get_time());
  std::string base_user = "mysql_innodb_cluster_rplusr";
  username = base_user.substr(0, 32 - tstamp.size()) + tstamp;

  // TODO: Replication accounts should be created with grants for the joining
  // instance only
  // However, we don't have a reliable way of getting the external IP and/or
  // fully qualified domain name
  std::string hostname = "%";

  Transaction tx(shared_from_this());

  query = shcore::sqlstring("DROP USER IF EXISTS ?@?", 0);
  query << username;
  query << hostname;
  query.done();

  execute_sql(query);

  // Try to create an account with the generated password,
  // and if fails retry for 100 times with newly generated passwords
  int retry_count = 100;
  while (retry_count > 0) {
    try {
      // Create the account without using an hashed password
      create_account(username, password, hostname, false);

      // If it reached here it means there were no errors
      retry_count = 0;
      account_created = true;
    } catch (shcore::Exception &e) {
      // If the error is: ERROR 1819 (HY000): Your password does not satisfy
      // the current policy requirements
      // We regenerate the password to retry
      if (e.code() == ER_NOT_VALID_PASSWORD) {
        password = generate_password();
        retry_count--;
      } else {
        throw;
      }
    }
  }

  // Try to create an account using mysql_native_password with the
  // hashed password to avoid validate_password verification.
  if (!account_created) {
    log_warning("Failed to create replication user account. "
                "Trying to create the account using a SHA1 hashed "
                "password to avoid the validate_password verification");
    try {
      // Compute the password SHA1 hash
      std::string hashed_password =
          Password_hasher::compute_password_hash(password);

      // Create the account using an hashed password
      create_account(username, hashed_password, hostname, true);
    } catch (shcore::Exception &e) {
      // If the error is: ERROR 1524 (HY000): Plugin 'mysql_native_password'
      // is not loaded, we have failed all the attempts so we suggest the user
      // to change to validate_password rules
      if (e.code() == ER_PLUGIN_IS_NOT_LOADED) {
        throw shcore::Exception::runtime_error(
              "Failed to create replication user account. Try to decrease the "
              "validate_password rules and try the operation again.");
      } else {
        throw;
      }
    }
  }

  query = shcore::sqlstring("GRANT REPLICATION SLAVE ON *.* to ?@?", 0);
  query << username;
  query << hostname;
  query.done();

  execute_sql(query);

  tx.commit();
}
