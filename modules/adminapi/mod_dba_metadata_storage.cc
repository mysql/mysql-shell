/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_dba_metadata_storage.h"
#include "modules/adminapi/metadata-model_definitions.h"
#include "modules/base_session.h"
#include "mysqlx_connection.h"
#include "xerrmsg.h"

#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include <random>

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

MetadataStorage::MetadataStorage(Dba* dba) :
_dba(dba)
{
}

MetadataStorage::~MetadataStorage()
{
}

std::shared_ptr<ShellBaseResult> MetadataStorage::execute_sql(const std::string &sql) const
{
  shcore::Value ret_val;

  auto session = _dba->get_active_session();
  if (!session)
    throw Exception::metadata_error("The Metadata is inaccessible");

  try
  {
    ret_val = session->execute_sql(sql, shcore::Argument_list());
  }
  catch (shcore::Exception& e)
  {
    if (CR_SERVER_GONE_ERROR == e.code() || ER_X_BAD_PIPE == e.code())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else if (CR_SQLSTATE == e.code())
      throw Exception::metadata_error("The Metadata session is invalid. A R/W session is required.");
    else
      throw;
  }

  return ret_val.as_object<ShellBaseResult>();
}

void MetadataStorage::start_transaction()
{
  auto session = _dba->get_active_session();
  session->start_transaction();
}

void MetadataStorage::commit()
{
  auto session = _dba->get_active_session();
  session->commit();
}

void MetadataStorage::rollback()
{
  auto session = _dba->get_active_session();
  session->rollback();
}

bool MetadataStorage::metadata_schema_exists()
{
  std::string found_object;
  std::string type = "Schema";
  std::string search_name = "mysql_innodb_cluster_metadata";
  auto session = _dba->get_active_session();

  if (session)
    found_object = session->db_object_exists(type, search_name, "");
  else
    throw shcore::Exception::logic_error("");

  return !found_object.empty();
}

void MetadataStorage::create_metadata_schema()
{
  if (!metadata_schema_exists())
  {
    std::string query = shcore::md_model_sql;

    size_t pos = 0;
    std::string token, delimiter = ";\n";
    auto session = _dba->get_active_session();
    while ((pos = query.find(delimiter)) != std::string::npos) {
      token = query.substr(0, pos);

      session->execute_sql(token, shcore::Argument_list());

      query.erase(0, pos + delimiter.length());
    }
  }
  else
  {
    // Check the Schema version and update the schema accordingly
  }
}

void MetadataStorage::drop_metadata_schema()
{
  execute_sql("DROP SCHEMA mysql_innodb_cluster_metadata");
}

uint64_t MetadataStorage::get_cluster_id(const std::string &cluster_name)
{
  uint64_t cluster_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster ID

  auto result = execute_sql("SELECT cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name = '" + cluster_name + "'");

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
    cluster_id = row.as_object<Row>()->get_member(0).as_uint();

  //result->flush();

  return cluster_id;
}

uint64_t MetadataStorage::get_cluster_id(uint64_t rs_id)
{
  uint64_t cluster_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster ID

  auto result = execute_sql("SELECT cluster_id from mysql_innodb_cluster_metadata.replicasets where replicaset_id = '" + std::to_string(rs_id) + "'");

  auto row = result->call("fetchOne", shcore::Argument_list());

  //result->flush();

  if (row)
    cluster_id = row.as_object<Row>()->get_member(0).as_uint();

  return cluster_id;
}

bool MetadataStorage::cluster_exists(const std::string &cluster_name)
{
  /*
   * To check if the cluster exists, we can use get_cluster_id
   * and simply check for its return value.
   * If zero, it means the cluster does not exist (cluster_id cannot be zero)
   */

  if (get_cluster_id(cluster_name))
    return true;

  return false;
}

void MetadataStorage::insert_cluster(const std::shared_ptr<Cluster> &cluster)
{
  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Check if the Cluster has some description
  std::string query = "INSERT INTO mysql_innodb_cluster_metadata.clusters (cluster_name, description, mysql_user_accounts, options, attributes) "\
                      "VALUES ('" + cluster->get_name() + "', '" + cluster->get_description() + "', '" + cluster->get_accounts() + "'"\
                      ",'" + cluster->get_options() + "', '" + cluster->get_attributes() + "')";

  // Insert the Cluster on the cluster table
  try
  {
    auto result = execute_sql(query);
    cluster->set_id(result->get_member("autoIncrementValue").as_int());
  }
  catch (shcore::Exception &e)
  {
    if (e.what() == "Duplicate entry '" + cluster->get_name() + "' for key 'cluster_name'")
      throw Exception::argument_error("A Cluster with the name '" + cluster->get_name() + "' already exists.");
    else
      throw;
  }
}

void MetadataStorage::insert_default_replica_set(const std::shared_ptr<Cluster> &cluster)
{
  std::string query;
  uint64_t cluster_id;

  cluster_id = cluster->get_id();

  // Insert the default ReplicaSet on the replicasets table
  query = "INSERT INTO mysql_innodb_cluster_metadata.replicasets (cluster_id, replicaset_type, replicaset_name, active) VALUES (" +
        std::to_string(cluster_id) + ", 'gr', 'default', 1)";

  auto result = execute_sql(query);

  // Update the replicaset_id
  uint64_t rs_id = 0;
  rs_id = result->get_member("autoIncrementValue").as_uint();

  // Update the cluster entry with the replicaset_id
  cluster->get_default_replicaset()->set_id(rs_id);

  // Insert the default ReplicaSet on the replicasets table
  query = "UPDATE mysql_innodb_cluster_metadata.clusters SET default_replicaset = " + std::to_string(rs_id) +
        " WHERE cluster_id = " + std::to_string(cluster_id) + "";

  execute_sql(query);
}

std::shared_ptr<ShellBaseResult> MetadataStorage::insert_host(const shcore::Argument_list &args)
{
  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string host_name;
  std::string ip_address;
  std::string location;
  //shcore::Value::Map_type_ref attributes, admin_user_account;

  std::string query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  // Identify the type of args data (String or Document)
  if (args[0].type == String)
  {
    uri = args.string_at(0);
    options = get_connection_data(uri, false);
  }

  // Connection data comes in a dictionary
  else if (args[0].type == Map)
    options = args.map_at(0);

  if (options->has_key("host"))
    host_name = (*options)["host"].as_string();

  if (options->has_key("id_address"))
    ip_address = (*options)["id_address"].as_string();
  else
    ip_address = (*options)["host"].as_string();

  if (options->has_key("location"))
    location = (*options)["location"].as_string();
  else
    location = "TODO";

  // Insert the default ReplicaSet on the replicasets table
  query = "INSERT INTO mysql_innodb_cluster_metadata.hosts (host_name, ip_address, location) VALUES ('" +
        host_name + "', '" + ip_address + "', '" + location + "')";

  return execute_sql(query);
}

void MetadataStorage::insert_instance(const shcore::Argument_list &args, uint64_t host_id, uint64_t rs_id)
{
  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string mysql_server_uuid;
  std::string instance_name;
  std::string role;
  float weight;
  shcore::Value::Map_type_ref attributes;
  std::string addresses;
  int version_token;
  std::string description;

  std::string query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  // args data comes in a dictionary
  options = args.map_at(0);

  mysql_server_uuid = (*options)["mysql_server_uuid"].as_string();
  instance_name = (*options)["instance_name"].as_string();

  if (options->has_key("role"))
    role = (*options)["role"].as_string();

  //if (options->has_key("weight"))
  //  weight = (*options)["weight"].as_float();

  if (options->has_key("addresses"))
    addresses = (*options)["addresses"].as_string();

  if (options->has_key("attributes"))
    attributes = (*options)["attributes"].as_map();

  if (options->has_key("version_token"))
    version_token = (*options)["version_token"].as_int();

  if (options->has_key("description"))
    description = (*options)["description"].as_string();

  // Insert the default ReplicaSet on the replicasets table
  query = "INSERT INTO mysql_innodb_cluster_metadata.instances (host_id, replicaset_id, mysql_server_uuid, instance_name,\
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          role, addresses) VALUES ('" +
        std::to_string(host_id) + "', '" + std::to_string(rs_id) + "', '" + mysql_server_uuid + "', '" +
        instance_name + "', '" + role + "', '{\"mysqlClassic\": \"" + addresses + "\"}')";

  execute_sql(query);
}

void MetadataStorage::remove_instance(const std::string &instance_name)
{
  std::string query;

  // Remove the instance
  query = "DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name = '" + instance_name + "'";

  execute_sql(query);
}

void MetadataStorage::drop_cluster(const std::string &cluster_name)
{
  std::string query, ret;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Check if the Cluster exists
  if (!cluster_exists(cluster_name))
    throw Exception::logic_error("The cluster with the name '" + cluster_name + "' does not exist.");

  // It exists, so let's get the cluster_id and move on
  else
  {
    uint64_t cluster_id = get_cluster_id(cluster_name);

    // Check if the Cluster is empty
    query = "SELECT * from mysql_innodb_cluster_metadata.replicasets where cluster_id = " + std::to_string(cluster_id) + "";

    auto result = execute_sql(query);

    auto row = result->call("fetchOne", shcore::Argument_list());

    //result->flush();

    if (row)
      throw Exception::logic_error("The cluster with the name '" + cluster_name + "' is not empty.");

    // OK the cluster exists and is empty, we can remove it
    query = "DELETE from mysql_innodb_cluster_metadata.clusters where cluster_id = " + std::to_string(cluster_id) + "";

    execute_sql(query);
  }
}

bool MetadataStorage::cluster_has_default_replicaset_only(const std::string &cluster_name)
{
  std::string query, ret;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster ID
  uint64_t cluster_id = get_cluster_id(cluster_name);

  // Check if the Cluster has only one replicaset
  query = "SELECT count(*) as count FROM mysql_innodb_cluster_metadata.replicasets WHERE cluster_id = " + std::to_string(cluster_id) + " AND replicaset_name <> 'default'";

  auto result = execute_sql(query);

  auto row = result->call("fetchOne", shcore::Argument_list());

  //result->flush();

  int count = 0;
  if (row)
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("count"));
    count = row.as_object<Row>()->get_field(args).as_uint();
  }

  return count == 0;
}

void MetadataStorage::drop_default_replicaset(const std::string &cluster_name)
{
  std::string query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  uint64_t cluster_id = get_cluster_id(cluster_name);

  Transaction tx(shared_from_this());
  // Set the default_replicaset as NULL
  execute_sql("UPDATE mysql_innodb_cluster_metadata.clusters SET default_replicaset = NULL WHERE cluster_id = " + std::to_string(cluster_id) + "");

  // Delete the default_replicaset
  execute_sql("delete from mysql_innodb_cluster_metadata.replicasets where cluster_id = " + std::to_string(cluster_id) + "");

  tx.commit();
}

std::string MetadataStorage::get_replicaset_name(uint64_t rs_id)
{
  std::string rs_name;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the ReplicaSet name
  auto result = execute_sql("SELECT replicaset_name from mysql_innodb_cluster_metadata.replicasets where replicaset_id = '" + std::to_string(rs_id) + "'");

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("replicaset_name"));
    rs_name = row.as_object<Row>()->get_field(args).as_string();
  }

  //result->flush();

  return rs_name;
}

std::shared_ptr<ReplicaSet> MetadataStorage::get_replicaset(uint64_t rs_id)
{
  // Create a ReplicaSet Object to match the Metadata
  std::shared_ptr<ReplicaSet> rs(new ReplicaSet("name", shared_from_this()));
  std::string rs_name;

  // Get and set the Metadata data
  rs->set_id(rs_id);

  rs_name = get_replicaset_name(rs_id);

  rs->set_name(rs_name);

  return rs;
}

std::shared_ptr<Cluster> MetadataStorage::get_cluster_matching(const std::string& condition)
{
  std::shared_ptr<Cluster> cluster;
  std::string query = "SELECT cluster_id, cluster_name, default_replicaset, description, mysql_user_accounts, options, attributes " \
    "from mysql_innodb_cluster_metadata.clusters " \
    "WHERE ";

  query += condition;

  try
  {
    auto result = execute_sql(query);

    auto row = result->call("fetchOne", shcore::Argument_list());

    if (row)
    {
      auto real_row = row.as_object<Row>();
      shcore::Argument_list args;

      cluster.reset(new Cluster(real_row->get_member(1).as_string(), shared_from_this()));

      cluster->set_id(real_row->get_member(0).as_int());
      cluster->set_description(real_row->get_member(3).as_string());
      cluster->set_accounts(real_row->get_member(4).as_string());
      cluster->set_options(real_row->get_member(5).as_string());
      cluster->set_attributes(real_row->get_member(6).as_string());

      auto rsetid_val = real_row->get_member(2);
      if (rsetid_val)
        cluster->set_default_replicaset(get_replicaset(rsetid_val.as_int()));
    }
  }
  catch (shcore::Exception &e)
  {
    std::string error = e.what();

    if (error == "Table 'mysql_innodb_cluster_metadata.clusters' doesn't exist")
      throw Exception::metadata_error("Metadata Schema does not exist.");
    else
      throw;
  }

  return cluster;
}

std::shared_ptr<Cluster> MetadataStorage::get_default_cluster()
{
  return get_cluster_matching("attributes->'$.default' = true");
}

std::shared_ptr<Cluster> MetadataStorage::get_cluster(const std::string &cluster_name)
{
  std::shared_ptr<Cluster> cluster = get_cluster_matching("cluster_name = '" + cluster_name + "'");

  if (!cluster)
    throw Exception::logic_error("The cluster with the name '" + cluster_name + "' does not exist.");

  return cluster;
}

bool MetadataStorage::has_default_cluster()
{
  bool ret_val = false;

  if (metadata_schema_exists())
  {
    auto result = execute_sql("SELECT cluster_id from mysql_innodb_cluster_metadata.clusters WHERE attributes->\"$.default\" = true");

    auto row = result->call("fetchOne", shcore::Argument_list());

    if (row)
      ret_val = true;;
  }

  return ret_val;
}

bool MetadataStorage::is_replicaset_empty(uint64_t rs_id)
{
  auto result = execute_sql("SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances WHERE replicaset_id = '" + std::to_string(rs_id) + "'");

  auto row = result->call("fetchOne", shcore::Argument_list());

  //result->flush();

  uint64_t count = 0;
  if (row)
  {
    auto real_row = row.as_object<Row>();
    shcore::Argument_list args;
    args.push_back(shcore::Value("count"));
    count = row.as_object<Row>()->get_field(args).as_int();
  }

  return count == 0;
}

bool MetadataStorage::is_instance_on_replicaset(uint64_t rs_id, std::string address)
{
  auto result = execute_sql("SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances WHERE replicaset_id = '" +
                            std::to_string(rs_id) + "' AND addresses->\"$.mysqlClassic\"='" + address + "'");

  auto row = result->call("fetchOne", shcore::Argument_list());

  //result->flush();

  uint64_t count = 0;
  if (row)
  {
    auto real_row = row.as_object<Row>();
    shcore::Argument_list args;
    args.push_back(shcore::Value("count"));
    count = row.as_object<Row>()->get_field(args).as_int();
  }

  return count == 1;
}

std::string MetadataStorage::get_seed_instance(uint64_t rs_id)
{
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

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("address"));
    seed_address = row.as_object<Row>()->get_field(args).as_string();
  }

  //result->flush();

  return seed_address;
}