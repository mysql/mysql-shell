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

uint64_t MetadataStorage::get_farm_id(const std::string &farm_name)
{
  uint64_t farm_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm ID

  auto result = execute_sql("SELECT farm_id from farm_metadata_schema.farms where farm_name = '" + farm_name + "'");

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("farm_id"));
    farm_id = row.as_object<Row>()->get_field(args).as_uint();
  }

  //result->flush();

  return farm_id;
}

uint64_t MetadataStorage::get_farm_id(uint64_t rs_id)
{
  uint64_t farm_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm ID

  auto result = execute_sql("SELECT farm_id from farm_metadata_schema.replicasets where replicaset_id = '" + std::to_string(rs_id) + "'");

  auto row = result->call("fetchOne", shcore::Argument_list());

  //result->flush();

  if (row)
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("farm_id"));
    farm_id = row.as_object<Row>()->get_field(args).as_uint();
  }

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
          "\", \"password\": \"" + replication_user_password + "\"}}','{\"farmAdminType\": \"" + farm_admin_type + "\"}', '{\"default\": true}')";

  // Insert the Farm on the farms table
  try
  {
    auto result = execute_sql(query);
    farm->set_id(result->get_member("autoIncrementValue").as_int());
  }
  catch (shcore::Exception &e)
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

  farm_id = farm->get_id();

  // Insert the default ReplicaSet on the replicasets table
  query = "INSERT INTO farm_metadata_schema.replicasets (farm_id, replicaset_type, replicaset_name, active) VALUES (" +
        std::to_string(farm_id) + ", 'gr', 'default', 1)";

  auto result = execute_sql(query);

  // Update the replicaset_id
  uint64_t rs_id = 0;
  rs_id = result->get_member("autoIncrementValue").as_uint();

  // Update the farm entry with the replicaset_id
  farm->get_default_replicaset()->set_id(rs_id);

  // Insert the default ReplicaSet on the replicasets table
  query = "UPDATE farm_metadata_schema.farms SET default_replicaset = " + std::to_string(rs_id) +
        " WHERE farm_id = " + std::to_string(farm_id) + "";

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

void MetadataStorage::drop_farm(const std::string &farm_name)
{
  std::string query, ret;

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

    auto result = execute_sql(query);

    auto row = result->call("fetchOne", shcore::Argument_list());

    //result->flush();

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

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm ID
  uint64_t farm_id = get_farm_id(farm_name);

  // Check if the Farm has only one replicaset
  query = "SELECT count(*) as count FROM farm_metadata_schema.replicasets WHERE farm_id = " + std::to_string(farm_id) + " AND replicaset_name <> 'default'";

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
  uint64_t default_rs_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Default ReplicaSet ID

  auto result = execute_sql("SELECT default_replicaset from farm_metadata_schema.farms where farm_name = '" + farm_name + "'");

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

    auto row = result->call("fetchOne", shcore::Argument_list());

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

  auto row = result->call("fetchOne", shcore::Argument_list());

  if (row)
  {
    auto real_row = row.as_object<Row>();
    shcore::Argument_list args;
    args.push_back(shcore::Value("farm_name"));
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

  uint64_t farm_id = get_farm_id(rs_id);

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm instanceAdminUser

  query = "SELECT JSON_UNQUOTE(mysql_user_accounts->\"$.instanceAdmin.username\")  as user FROM farm_metadata_schema.farms WHERE farm_id = '" + std::to_string(farm_id) + "'";

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

  uint64_t farm_id = get_farm_id(rs_id);

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm instanceAdminUser

  query = "SELECT JSON_UNQUOTE(mysql_user_accounts->\"$.instanceAdmin.password\")  as password FROM farm_metadata_schema.farms WHERE farm_id = '" + std::to_string(farm_id) + "'";

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

  uint64_t farm_id = get_farm_id(rs_id);

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm instanceAdminUser

  query = "SELECT JSON_UNQUOTE(mysql_user_accounts->\"$.replicationUser.password\")  as password FROM farm_metadata_schema.farms WHERE farm_id = '" + std::to_string(farm_id) + "'";

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

  // Get the Farm instanceAdminUser

  query = "SELECT JSON_UNQUOTE(addresses->\"$.mysqlClassic\")  as address FROM farm_metadata_schema.instances WHERE replicaset_id = '" + std::to_string(rs_id) + "' AND role = 'master'";

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