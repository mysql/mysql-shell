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

#include "mod_mysqlx_metadata_storage.h"
#include "modules/adminapi/metadata-model_definitions.h"
#include "mysqlx_connection.h"
#include "xerrmsg.h"

#include "utils/utils_file.h"
#include "utils/utils_general.h"

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

MetadataStorage::MetadataStorage(const std::shared_ptr<AdminSession> &admin_session) :
_admin_session(admin_session)
{
}

MetadataStorage::~MetadataStorage()
{
}

std::shared_ptr< ::mysqlx::Result> MetadataStorage::execute_sql(const std::string &sql) const
{
  std::shared_ptr< ::mysqlx::Result> ret_val;
  try
  {
    ret_val = _admin_session->get_session().execute_sql(sql);
    ret_val->wait();
  }
  catch (::mysqlx::Error &e) {
    if (CR_SERVER_GONE_ERROR == e.error() || ER_X_BAD_PIPE == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

  return ret_val;
}

bool MetadataStorage::metadata_schema_exists()
{
  std::string type = "Schema";
  std::string search_name = "farm_metadata_schema";
  std::string found = _admin_session->get_session().db_object_exists(type, search_name, "");

  if (found.empty()) return false;

  return true;
}

void MetadataStorage::create_metadata_schema()
{
  if (!metadata_schema_exists())
  {
    std::string query = shcore::md_model_sql;

    size_t pos = 0;
    std::string token, delimiter = ";\n";
    while ((pos = query.find(delimiter)) != std::string::npos) {
      token = query.substr(0, pos);

      try { _admin_session->get_session().execute_sql(token); }
      catch (::mysqlx::Error &e) { throw; }
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

uint64_t MetadataStorage::get_farm_id(const std::string &farm_name)
{
  std::string query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;
  uint64_t farm_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm ID

  query = "SELECT farm_id from farm_metadata_schema.farms where farm_name = '" + farm_name + "'";
  result = execute_sql(query);

  row = result->next();

  if (row)
    farm_id = row->uIntField(0);

  result->flush();

  return farm_id;
}

uint64_t MetadataStorage::get_host_id(std::string host_name)
{
  std::string query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;
  uint64_t host_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm ID

  query = "SELECT host_id from farm_metadata_schema.hosts where host_name = '" + host_name + "'";
  result = execute_sql(query);

  row = result->next();

  if (row)
    host_id = row->uIntField(0);

  result->flush();

  return host_id;
}

uint64_t MetadataStorage::get_farm_id(uint64_t rs_id)
{
  std::string query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;
  uint64_t farm_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm ID

  query = "SELECT farm_id from farm_metadata_schema.replicasets where replicaset_id = '" + std::to_string(rs_id) + "'";
  result = execute_sql(query);

  row = result->next();

  if (row)
    farm_id = row->uIntField(0);

  result->flush();

  return farm_id;
}

bool MetadataStorage::farm_exists(const std::string &farm_name)
{
  /*
   * To check if the farm exists, we can use get_farm_id
   * and simply check for its return value.
   * If zero, it means the farm does not exist (farm_id cannot be zero)
   */

  if (get_farm_id(farm_name))
    return true;

  return false;
}

void MetadataStorage::insert_farm(const std::shared_ptr<Farm> &farm)
{
  std::string farm_name, query, farm_admin_type, farm_description,
              instance_admin_user, instance_admin_user_password,
              farm_read_user, farm_read_user_password, replication_user,
              replication_user_password;

  farm_name = farm->get_member("name").as_string();
  farm_admin_type = farm->get_member("adminType").as_string();
  instance_admin_user = farm->get_instance_admin_user();
  instance_admin_user_password = farm->get_instance_admin_user_password();
  farm_read_user = farm->get_farm_reader_user();
  farm_read_user_password = farm->get_farm_reader_user_password();
  replication_user = farm->get_replication_user();
  replication_user_password = farm->get_replication_user_password();

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Check if the Farm has some description
  farm_description = farm->get_description();
  query = "INSERT INTO farm_metadata_schema.farms (farm_name, description, mysql_user_accounts, options, attributes) "
          "VALUES ('" + farm_name + "', '" + farm_description + "', '{\"instanceAdmin\": {\"username\": \"" + instance_admin_user +
          "\", \"password\": \"" + instance_admin_user_password + "\"}, \"farmReader\": {\"username\": \"" + farm_read_user +
          "\", \"password\": \"" + farm_read_user_password + "\"}, \"replicationUser\": {\"username\": \"" + replication_user +
          "\", \"password\": \"" + replication_user_password +  "\"}}','{\"farmAdminType\": \"" + farm_admin_type + "\"}', '{\"default\": true}')";

  // Insert the Farm on the farms table
  try
  {
    auto result = execute_sql(query);
    farm->set_id(result->lastInsertId());
  }
  catch (::mysqlx::Error &e)
  {
    if (e.what() == "Duplicate entry '" + farm_name + "' for key 'farm_name'")
      throw Exception::argument_error("A Farm with the name '" + farm_name + "' already exists.");
    else
      throw;
  }
}

void MetadataStorage::insert_default_replica_set(const std::shared_ptr<Farm> &farm)
{
  std::string query;
  uint64_t farm_id;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  farm_id = farm->get_id();

  // Insert the default ReplicaSet on the replicasets table
  query = "INSERT INTO farm_metadata_schema.replicasets (farm_id, replicaset_type, replicaset_name, active) VALUES (" +
        std::to_string(farm_id) + ", 'gr', 'default', 1)";

  result = execute_sql(query);

  // Update the replicaset_id
  uint64_t rs_id = 0;
  rs_id = result->lastInsertId();

  // Update the farm entry with the replicaset_id
  farm->get_default_replicaset()->set_id(rs_id);

  // Insert the default ReplicaSet on the replicasets table
  query = "UPDATE farm_metadata_schema.farms SET default_replicaset = " + std::to_string(rs_id) +
        " WHERE farm_id = " + std::to_string(farm_id) + "";

  execute_sql(query);
}

void MetadataStorage::insert_host(const shcore::Argument_list &args)
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

  execute_sql(query);
}

void MetadataStorage::insert_instance(const shcore::Argument_list &args, uint64_t host_id, uint64_t rs_id)
{
  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string mysql_server_uuid;
  std::string instance_name;
  std::string role;
  std::string mode;
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

  if (options->has_key("mysql_server_uuid"))
    mysql_server_uuid = (*options)["mysql_server_uuid"].as_string();
  else // TODO: remove this, get uuid properly
  {
    std::random_device rd;
    std::string random_text;
    const char *alphabet = "1234567890abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<int> dist(0, strlen(alphabet) - 1);

    for (int i = 0; i < 6; i++)
      random_text += alphabet[dist(rd)];

    mysql_server_uuid = random_text;
  }

  if (options->has_key("instance_name"))
    instance_name = (*options)["instance_name"].as_string();
  else //TODO: remove this, get instance_name properly
  {
    std::random_device rd;
    std::string random_text;
    const char *alphabet = "1234567890abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<int> dist(0, strlen(alphabet) - 1);

    for (int i = 0; i < 6; i++)
      random_text += alphabet[dist(rd)];

    instance_name = random_text;
  }

  if (options->has_key("role"))
    role = (*options)["role"].as_string();

  if (options->has_key("mode"))
    mode = (*options)["mode"].as_string();

  //if (options->has_key("weight"))
  //  weight = (*options)["weight"].as_float();

  if (options->has_key("addresses"))
    addresses = (*options)["addresses"].as_string();

  if (options->has_key("attributes"))
    attributes = (*options)["attributes"].as_map();

  if (options->has_key("version_token"))
    version_token = (*options)["version_token"].as_int();

  if (options->has_key("description"))
    mode = (*options)["description"].as_string();

  // Insert the default ReplicaSet on the replicasets table
  query = "INSERT INTO farm_metadata_schema.instances (host_id, replicaset_id, mysql_server_uuid, instance_name,\
  role, mode, addresses) VALUES ('" +
        std::to_string(host_id) + "', '" + std::to_string(rs_id) + "', '" + mysql_server_uuid + "', '" +
        instance_name + "', '" + role + "', '" +  mode + "', '{\"mysqlClassic\": \"" + addresses + "\"}')";

  execute_sql(query);
}

void MetadataStorage::drop_farm(const std::string &farm_name)
{
  std::string query, ret;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Check if the Farm exists
  if (!farm_exists(farm_name))
    throw Exception::logic_error("The farm with the name '" + farm_name + "' does not exist.");

  // It exists, so let's get the farm_id and move on
  else
  {
    uint64_t farm_id = get_farm_id(farm_name);

    // Check if the Farm is empty
    query = "SELECT * from farm_metadata_schema.replicasets where farm_id = " + std::to_string(farm_id) + "";

    result = execute_sql(query);

    row = result->next();

    result->flush();

    if (row)
      throw Exception::logic_error("The farm with the name '" + farm_name + "' is not empty.");

    // OK the farm exists and is empty, we can remove it
    query = "DELETE from farm_metadata_schema.farms where farm_id = " + std::to_string(farm_id) + "";

    execute_sql(query);
  }
}

bool MetadataStorage::farm_has_default_replicaset_only(const std::string &farm_name)
{
  std::string query, ret;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm ID
  uint64_t farm_id = get_farm_id(farm_name);

  // Check if the Farm has only one replicaset
  query = "SELECT count(*) FROM farm_metadata_schema.replicasets WHERE farm_id = " + std::to_string(farm_id) + " AND replicaset_name <> 'default'";

  result = execute_sql(query);

  row = result->next();

  result->flush();

  int count = 0;
  if (row)
    count = row->sIntField(0);

  return count == 0;
}

void MetadataStorage::drop_default_replicaset(const std::string &farm_name)
{
  std::string query;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  uint64_t farm_id = get_farm_id(farm_name);

  // Set the default_replicaset as NULL
  execute_sql("UPDATE farm_metadata_schema.farms SET default_replicaset = NULL WHERE farm_id = " + std::to_string(farm_id) + "");

  // Delete the default_replicaset
  execute_sql("delete from farm_metadata_schema.replicasets where farm_id = " + std::to_string(farm_id) + "");
}

uint64_t MetadataStorage::get_farm_default_rs_id(const std::string &farm_name)
{
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;
  uint64_t default_rs_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Default ReplicaSet ID

  result = execute_sql("SELECT default_replicaset from farm_metadata_schema.farms where farm_name = '" + farm_name + "'");

  row = result->next();

  if (!row) return default_rs_id;

  // It exists, so let's get the farm_id
  else
  {
    while (row)
    {
      if (row->isNullField(0))
        break;
      else
      {
        default_rs_id = row->uIntField(0);
        result->flush();
        break;
      }
    }
  }

  return default_rs_id;
}

std::string MetadataStorage::get_replicaset_name(uint64_t rs_id)
{
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;
  std::string rs_name;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the ReplicaSet name
  result = execute_sql("SELECT replicaset_name from farm_metadata_schema.replicasets where replicaset_id = '" + std::to_string(rs_id) + "'");

  row = result->next();

  if (!row) return NULL;

  // It exists, so let's get the replicaset_name
  else
  {
    while (row)
    {
      rs_name = row->stringField(0);
      result->flush();
      break;
    }
  }

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

std::shared_ptr<Farm> MetadataStorage::get_farm(const std::string &farm_name)
{
  // Check if the Farm exists
  if (!farm_exists(farm_name))
    throw Exception::logic_error("The farm with the name '" + farm_name + "' does not exist.");

  std::shared_ptr<Farm> farm(new Farm(farm_name, shared_from_this()));

  // Update the farm_id
  uint64_t farm_id = get_farm_id(farm_name);
  farm->set_id(farm_id);

  // Get the Farm's Default replicaSet ID
  uint64_t default_rs_id = get_farm_default_rs_id(farm_name);

  // Check if the Farm has a Default ReplicaSet
  if (default_rs_id != 0)
  {
    // Get the default replicaset from the Farm
    std::shared_ptr<ReplicaSet> default_rs = get_replicaset(default_rs_id);

    // Update the default replicaset_id
    farm->set_default_replicaset(default_rs);
  }

  return farm;
}

bool MetadataStorage::has_default_farm()
{
  bool ret_val = false;

  if (metadata_schema_exists())
  {
    auto result = execute_sql("SELECT farm_id from farm_metadata_schema.farms WHERE attributes->\"$.default\" = true");

    auto row = result->next();

    if (row)
      ret_val = true;;
  }

  return ret_val;
}

std::string MetadataStorage::get_default_farm_name()
{
  std::string rs_name;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Default Farm name
  auto result = execute_sql("SELECT farm_name from farm_metadata_schema.farms WHERE attributes->\"$.default\" = true");

  auto row = result->next();

  result->flush();

  if (row)
    rs_name = row->stringField(0);

  return rs_name;
}

bool MetadataStorage::is_replicaset_empty(uint64_t rs_id)
{
  auto result = execute_sql("SELECT COUNT(*) FROM farm_metadata_schema.instances WHERE replicaset_id = '" + std::to_string(rs_id) + "'");

  auto row = result->next();

  result->flush();

  uint64_t count = 0;
  if (row)
    count = row->sInt64Field(0);

  return count == 0;
}

std::string MetadataStorage::get_instance_admin_user(uint64_t rs_id)
{
  std::string instance_admin_user, query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  uint64_t farm_id = get_farm_id(rs_id);

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm instanceAdminUser

  query = "SELECT JSON_UNQUOTE(mysql_user_accounts->\"$.instanceAdmin.username\") FROM farm_metadata_schema.farms WHERE farm_id = '" + std::to_string(farm_id) + "'";
  result = execute_sql(query);

  row = result->next();

  if (row)
    instance_admin_user = row->stringField(0);

  result->flush();

  return instance_admin_user;
}

std::string MetadataStorage::get_instance_admin_user_password(uint64_t rs_id)
{
  std::string instance_admin_user_password, query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  uint64_t farm_id = get_farm_id(rs_id);

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm instanceAdminUser

  query = "SELECT JSON_UNQUOTE(mysql_user_accounts->\"$.instanceAdmin.password\") FROM farm_metadata_schema.farms WHERE farm_id = '" + std::to_string(farm_id) + "'";
  result = execute_sql(query);

  row = result->next();

  if (row)
    instance_admin_user_password = row->stringField(0);

  result->flush();

  return instance_admin_user_password;
}

std::string MetadataStorage::get_replication_user_password(uint64_t rs_id)
{
  std::string replication_user_password, query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  uint64_t farm_id = get_farm_id(rs_id);

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm instanceAdminUser

  query = "SELECT JSON_UNQUOTE(mysql_user_accounts->\"$.replicationUser.password\") FROM farm_metadata_schema.farms WHERE farm_id = '" + std::to_string(farm_id) + "'";
  result = execute_sql(query);

  row = result->next();

  if (row)
    replication_user_password = row->stringField(0);

  result->flush();

  return replication_user_password;
}

std::string MetadataStorage::get_seed_instance(uint64_t rs_id)
{
  std::string seed_address, query;
  std::shared_ptr< ::mysqlx::Result> result;
  std::shared_ptr< ::mysqlx::Row> row;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm instanceAdminUser

  query = "SELECT JSON_UNQUOTE(addresses->\"$.mysqlClassic\") FROM farm_metadata_schema.instances WHERE replicaset_id = '" + std::to_string(rs_id) + "' AND role = 'master'";
  result = execute_sql(query);

  row = result->next();

  if (row)
    seed_address = row->stringField(0);

  result->flush();

  return seed_address;
}
