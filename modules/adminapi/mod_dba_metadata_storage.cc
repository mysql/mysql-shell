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
    else
      throw;
  }

  return ret_val.as_object<ShellBaseResult>();
}

void MetadataStorage::start_transaction()
{
  execute_sql("start transaction");
}

void MetadataStorage::commit()
{
  execute_sql("commit");
}

void MetadataStorage::rollback()
{
  execute_sql("rollback");
}

bool MetadataStorage::metadata_schema_exists()
{
  std::string found_object;
  std::string type = "Schema";
  std::string search_name = "farm_metadata_schema";
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
  execute_sql("DROP SCHEMA farm_metadata_schema");
}

uint64_t MetadataStorage::get_cluster_id(const std::string &cluster_name)
{
  uint64_t cluster_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster ID

  auto result = execute_sql("SELECT farm_id from farm_metadata_schema.farms where farm_name = '" + cluster_name + "'");

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

  auto result = execute_sql("SELECT farm_id from farm_metadata_schema.replicasets where replicaset_id = '" + std::to_string(rs_id) + "'");

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
  std::string cluster_name, query, cluster_admin_type, cluster_description,
              instance_admin_user, instance_admin_user_password,
              cluster_read_user, cluster_read_user_password, replication_user,
              replication_user_password;

  cluster_name = cluster->get_member("name").as_string();
  cluster_admin_type = cluster->get_member("adminType").as_string();
  instance_admin_user = cluster->get_instance_admin_user();
  instance_admin_user_password = cluster->get_instance_admin_user_password();
  cluster_read_user = cluster->get_cluster_reader_user();
  cluster_read_user_password = cluster->get_cluster_reader_user_password();
  replication_user = cluster->get_replication_user();
  replication_user_password = cluster->get_replication_user_password();

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Check if the Cluster has some description
  cluster_description = cluster->get_description();
  query = "INSERT INTO farm_metadata_schema.farms (farm_name, description, mysql_user_accounts, options, attributes) "
          "VALUES ('" + cluster_name + "', '" + cluster_description + "', '{\"instanceAdmin\": {\"username\": \"" + instance_admin_user +
          "\", \"password\": \"" + instance_admin_user_password + "\"}, \"farmReader\": {\"username\": \"" + cluster_read_user +
          "\", \"password\": \"" + cluster_read_user_password + "\"}, \"replicationUser\": {\"username\": \"" + replication_user +
          "\", \"password\": \"" + replication_user_password + "\"}}','{\"clusterAdminType\": \"" + cluster_admin_type + "\"}', '{\"default\": true}')";

  // Insert the Cluster on the cluster table
  try
  {
    auto result = execute_sql(query);
    cluster->set_id(result->get_member("autoIncrementValue").as_int());
  }
  catch (shcore::Exception &e)
  {
    if (e.what() == "Duplicate entry '" + cluster_name + "' for key 'cluster_name'")
      throw Exception::argument_error("A Cluster with the name '" + cluster_name + "' already exists.");
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
  query = "INSERT INTO farm_metadata_schema.replicasets (farm_id, replicaset_type, replicaset_name, active) VALUES (" +
        std::to_string(cluster_id) + ", 'gr', 'default', 1)";

  auto result = execute_sql(query);

  // Update the replicaset_id
  uint64_t rs_id = 0;
  rs_id = result->get_member("autoIncrementValue").as_uint();

  // Update the cluster entry with the replicaset_id
  cluster->get_default_replicaset()->set_id(rs_id);

  // Insert the default ReplicaSet on the replicasets table
  query = "UPDATE farm_metadata_schema.farms SET default_replicaset = " + std::to_string(rs_id) +
        " WHERE farm_id = " + std::to_string(cluster_id) + "";

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
  query = "INSERT INTO farm_metadata_schema.hosts (host_name, ip_address, location) VALUES ('" +
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
  query = "INSERT INTO farm_metadata_schema.instances (host_id, replicaset_id, mysql_server_uuid, instance_name,\
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        role, addresses) VALUES ('" +
        std::to_string(host_id) + "', '" + std::to_string(rs_id) + "', '" + mysql_server_uuid + "', '" +
        instance_name + "', '" + role + "', '{\"mysqlClassic\": \"" + addresses + "\"}')";

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
    query = "SELECT * from farm_metadata_schema.replicasets where farm_id = " + std::to_string(cluster_id) + "";

    auto result = execute_sql(query);

    auto row = result->call("fetchOne", shcore::Argument_list());

    //result->flush();

    if (row)
      throw Exception::logic_error("The cluster with the name '" + cluster_name + "' is not empty.");

    // OK the cluster exists and is empty, we can remove it
    query = "DELETE from farm_metadata_schema.farms where farm_id = " + std::to_string(cluster_id) + "";

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
  query = "SELECT count(*) as count FROM farm_metadata_schema.replicasets WHERE farm_id = " + std::to_string(cluster_id) + " AND replicaset_name <> 'default'";

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

  // Set the default_replicaset as NULL
  execute_sql("UPDATE farm_metadata_schema.farms SET default_replicaset = NULL WHERE farm_id = " + std::to_string(cluster_id) + "");

  // Delete the default_replicaset
  execute_sql("delete from farm_metadata_schema.replicasets where farm_id = " + std::to_string(cluster_id) + "");
}

uint64_t MetadataStorage::get_cluster_default_rs_id(const std::string &cluster_name)
{
  uint64_t default_rs_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Default ReplicaSet ID

  auto result = execute_sql("SELECT default_replicaset from farm_metadata_schema.farms where farm_name = '" + cluster_name + "'");

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("default_replicaset"));
    auto field = row.as_object<Row>()->get_field(args);

    if (field)
      default_rs_id = field.as_uint();
  }

  //result->flush();

  return default_rs_id;
}

std::string MetadataStorage::get_replicaset_name(uint64_t rs_id)
{
  std::string rs_name;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the ReplicaSet name
  auto result = execute_sql("SELECT replicaset_name from farm_metadata_schema.replicasets where replicaset_id = '" + std::to_string(rs_id) + "'");

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
  std::string query = "SELECT farm_id, farm_name, default_replicaset, description, JSON_UNQUOTE(JSON_EXTRACT(options, '$.clusterAdminType')) as admin_type " \
    "from farm_metadata_schema.farms " \
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
      cluster->set_admin_type(real_row->get_member(4).as_string());
      cluster->set_description(real_row->get_member(3).as_string());

      int rsetid = real_row->get_member(2).as_int();
      if (rsetid)
        cluster->set_default_replicaset(get_replicaset(rsetid));
    }
  }
  catch (shcore::Exception &e)
  {
    std::string error = e.what();

    if (error == "Table 'farm_metadata_schema.farms' doesn't exist")
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
  std::shared_ptr<Cluster> cluster = get_cluster_matching("farm_name = '" + cluster_name + "'");

  if (!cluster)
    throw Exception::logic_error("The cluster with the name '" + cluster_name + "' does not exist.");

  return cluster;
}

bool MetadataStorage::has_default_cluster()
{
  bool ret_val = false;

  if (metadata_schema_exists())
  {
    auto result = execute_sql("SELECT farm_id from farm_metadata_schema.farms WHERE attributes->\"$.default\" = true");

    auto row = result->call("fetchOne", shcore::Argument_list());

    if (row)
      ret_val = true;;
  }

  return ret_val;
}

std::string MetadataStorage::get_default_cluster_name()
{
  std::string rs_name;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Default Cluster name
  auto result = execute_sql("SELECT farm_name from farm_metadata_schema.farms WHERE attributes->\"$.default\" = true");

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
  {
    auto real_row = row.as_object<Row>();
    shcore::Argument_list args;
    args.push_back(shcore::Value("cluster_name"));
    rs_name = row.as_object<Row>()->get_field(args).as_string();
  }

  return rs_name;
}

bool MetadataStorage::is_replicaset_empty(uint64_t rs_id)
{
  auto result = execute_sql("SELECT COUNT(*) as count FROM farm_metadata_schema.instances WHERE replicaset_id = '" + std::to_string(rs_id) + "'");

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
  auto result = execute_sql("SELECT COUNT(*) as count FROM farm_metadata_schema.instances WHERE replicaset_id = '" +
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

std::string MetadataStorage::get_instance_admin_user(uint64_t rs_id)
{
  std::string instance_admin_user, query;

  uint64_t cluster_id = get_cluster_id(rs_id);

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster instanceAdminUser

  query = "SELECT JSON_UNQUOTE(mysql_user_accounts->\"$.instanceAdmin.username\")  as user FROM farm_metadata_schema.farms WHERE farm_id = '" + std::to_string(cluster_id) + "'";

  auto result = execute_sql(query);

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("user"));
    instance_admin_user = row.as_object<Row>()->get_field(args).as_string();
  }

  // result->flush();

  return instance_admin_user;
}

std::string MetadataStorage::get_instance_admin_user_password(uint64_t rs_id)
{
  std::string instance_admin_user_password, query;

  uint64_t cluster_id = get_cluster_id(rs_id);

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster instanceAdminUser

  query = "SELECT JSON_UNQUOTE(mysql_user_accounts->\"$.instanceAdmin.password\")  as password FROM farm_metadata_schema.farms WHERE farm_id = '" + std::to_string(cluster_id) + "'";

  auto result = execute_sql(query);

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("password"));
    instance_admin_user_password = row.as_object<Row>()->get_field(args).as_string();
  }

  //result->flush();

  return instance_admin_user_password;
}

std::string MetadataStorage::get_replication_user_password(uint64_t rs_id)
{
  std::string replication_user_password, query;

  uint64_t cluster_id = get_cluster_id(rs_id);

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster instanceAdminUser

  query = "SELECT JSON_UNQUOTE(mysql_user_accounts->\"$.replicationUser.password\")  as password FROM farm_metadata_schema.farms WHERE farm_id = '" + std::to_string(cluster_id) + "'";

  auto result = execute_sql(query);

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("password"));
    replication_user_password = row.as_object<Row>()->get_field(args).as_string();
  }

  //result->flush();

  return replication_user_password;
}

std::string MetadataStorage::get_seed_instance(uint64_t rs_id)
{
  std::string seed_address, query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Cluster instanceAdminUser

  query = "SELECT JSON_UNQUOTE(addresses->\"$.mysqlClassic\")  as address FROM farm_metadata_schema.instances WHERE replicaset_id = '" + std::to_string(rs_id) + "' AND role = 'HA'";

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