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

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

MetadataStorage::MetadataStorage(boost::shared_ptr<AdminSession> admin_session) :
_admin_session(admin_session)
{
}

MetadataStorage::~MetadataStorage()
{
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
    std::string token, delimiter=";\n";
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

uint64_t MetadataStorage::get_farm_id(std::string farm_name)
{
  std::string query;
  boost::shared_ptr< ::mysqlx::Result> result;
  boost::shared_ptr< ::mysqlx::Row> row;
  uint64_t farm_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm ID

  try {
    query = "SELECT farm_id from farm_metadata_schema.farms where farm_name = '" + farm_name + "'";
    result = _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

  row = result->next();

  if (!row) return farm_id;

  // It exists, so let's get the farm_id
  else
  {
    while (row)
    {
      farm_id = row->uIntField(0);
      result->flush();
      break;
    }
  }

  return farm_id;
}

bool MetadataStorage::farm_exists(std::string farm_name)
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

void MetadataStorage::insert_farm(boost::shared_ptr<Farm> farm)
{
  std::string farm_name, query;
  boost::shared_ptr< ::mysqlx::Result> result;
  boost::shared_ptr< ::mysqlx::Row> row;

  farm_name = farm->get_member("name").as_string();

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Insert the Farm on the farms table
  query = "INSERT INTO farm_metadata_schema.farms (farm_name) VALUES ('" + farm_name + "')";

  try
  {
    result = _admin_session->get_session().execute_sql(query);
    result->wait();
    result->flush();
  }
  catch (::mysqlx::Error &e)
  {
    if (e.what() == "Duplicate entry '" + farm_name + "' for key 'farm_name'")
      throw Exception::argument_error("A Farm with the name '" + farm_name + "' already exists.");

    else if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");

    else
      throw;
  }

  // Update the farm_id
  uint64_t farm_id = get_farm_id(farm_name);
  farm->set_id(farm_id);

  // For V1.0 we only support one Farm, so let's mark it as 'default' on the attributed column
  query = "UPDATE farm_metadata_schema.farms SET attributes = '{\"default\": true}' WHERE farm_id = '" + std::to_string(farm_id) + "'";

  try {
    _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

  // Insert the default ReplicaSet on the replicasets table
  query = "INSERT INTO farm_metadata_schema.replicasets (farm_id, replicaset_type, replicaset_name, active) VALUES (" +
        std::to_string(farm_id) + ", 'gr', 'default', 1)";

  try {
    _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

  // Update the replicaset_id
  uint64_t rs_id = 0;
  query = "SELECT replicaset_id FROM farm_metadata_schema.replicasets WHERE farm_id = '" + std::to_string(farm_id) + "'";

  try {
    result = _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

  row = result->next();

  while (row)
  {
    if (!row->isNullField(0))
    {
      rs_id = row->uIntField(0);
      result->flush();
      break;
    }

    row = result->next();
  }

  // Update the farm entry with the replicaset_id
  farm->get_default_replicaset()->set_id(rs_id);

  // Insert the default ReplicaSet on the replicasets table
  query = "UPDATE farm_metadata_schema.farms SET default_replicaset = "+ std::to_string(rs_id) +
        " WHERE farm_id = " + std::to_string(farm_id) + "";

  try {
    _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }
}

void MetadataStorage::drop_farm(std::string farm_name)
{
  std::string query, ret;
  boost::shared_ptr< ::mysqlx::Result> result;
  boost::shared_ptr< ::mysqlx::Row> row;

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

    try {
      result = _admin_session->get_session().execute_sql(query);
    }
    catch (::mysqlx::Error &e) {
      if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
        throw Exception::metadata_error("The Metadata is inaccessible");
      else
        throw;
    }

    row = result->next();

    if (row)
      throw Exception::logic_error("The farm with the name '" + farm_name + "' is not empty.");

    // OK the farm exists and is empty, we can remove it
    else
    {
      result->flush();

      query = "DELETE from farm_metadata_schema.farms where farm_id = " + std::to_string(farm_id) + "";

      try {
        _admin_session->get_session().execute_sql(query);
      }
      catch (::mysqlx::Error &e) {
        if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
          throw Exception::metadata_error("The Metadata is inaccessible");
        else
          throw;
      }
    }
  }
}

bool MetadataStorage::farm_has_default_replicaset_only(std::string farm_name)
{
  std::string query, ret;
  boost::shared_ptr< ::mysqlx::Result> result;
  boost::shared_ptr< ::mysqlx::Row> row;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Farm ID
  uint64_t farm_id = get_farm_id(farm_name);

  // Check if the Farm has only one replicaset
  query = "SELECT COUNT(replicaset_name) FROM farm_metadata_schema.replicasets WHERE farm_id = " + std::to_string(farm_id) + "";

  try {
    result = _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

  row = result->next();

  if (!row)
    return true;

  // Count the replicasets
  else
  {
    int count;

    while (row)
    {
      count = row->sIntField(0);
      result->flush();
      break;
    }

    if (count > 1) return false;
  }

  // Confirm that is the default replicaset
  query = "SELECT replicaset_id FROM farm_metadata_schema.replicasets WHERE farm_id = " + std::to_string(farm_id) + " AND replicaset_name = 'default'";

  try {
    result = _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

  row = result->next();

  if (!row) return false;

  return true;
}

void MetadataStorage::drop_default_replicaset(std::string farm_name)
{
  std::string query, ret;
  boost::shared_ptr< ::mysqlx::Result> result;
  boost::shared_ptr< ::mysqlx::Row> row;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  uint64_t farm_id = get_farm_id(farm_name);

  // Set the default_replicaset as NULL
  query = "UPDATE farm_metadata_schema.farms SET default_replicaset = NULL WHERE farm_id = " + std::to_string(farm_id) + "";

  try {
    result = _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

  // Delete the default_replicaset
  query = "delete from farm_metadata_schema.replicasets where farm_id = " + std::to_string(farm_id) + "";

  try {
    result = _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }
}

uint64_t MetadataStorage::get_farm_default_rs_id(std::string farm_name)
{
  std::string query;
  boost::shared_ptr< ::mysqlx::Result> result;
  boost::shared_ptr< ::mysqlx::Row> row;
  uint64_t default_rs_id = 0;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the Default ReplicaSet ID

  try {
    query = "SELECT default_replicaset from farm_metadata_schema.farms where farm_name = '" + farm_name + "'";
    result = _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

  row = result->next();

  if (!row) return default_rs_id;

  // It exists, so let's get the farm_id
  else
  {
    while (row)
    {
      default_rs_id = row->uIntField(0);
      result->flush();
      break;
    }
  }

  return default_rs_id;
}

std::string MetadataStorage::get_replicaset_name(uint64_t rs_id)
{
  std::string query;
  boost::shared_ptr< ::mysqlx::Result> result;
  boost::shared_ptr< ::mysqlx::Row> row;
  std::string rs_name;

  if (!metadata_schema_exists())
    throw Exception::metadata_error("Metadata Schema does not exist.");

  // Get the ReplicaSet name

  try {
    query = "SELECT replicaset_name from farm_metadata_schema.replicasets where replicaset_id = '" + std::to_string(rs_id) + "'";
    result = _admin_session->get_session().execute_sql(query);
  }
  catch (::mysqlx::Error &e) {
    if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
      throw Exception::metadata_error("The Metadata is inaccessible");
    else
      throw;
  }

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

boost::shared_ptr<ReplicaSet> MetadataStorage::get_replicaset(uint64_t rs_id)
{
  // Create a ReplicaSet Object to match the Metadata
  boost::shared_ptr<ReplicaSet> rs (new ReplicaSet("name"));
  std::string rs_name;

  // Get and set the Metadata data
  rs->set_id(rs_id);

  rs_name = get_replicaset_name(rs_id);

  rs->set_name(rs_name);

  return rs;
}

boost::shared_ptr<Farm> MetadataStorage::get_farm(std::string farm_name)
{
  // Check if the Farm exists
  if (!farm_exists(farm_name))
    throw Exception::logic_error("The farm with the name '" + farm_name + "' does not exist.");

  boost::shared_ptr<Farm> farm (new Farm(farm_name));

  // Update the farm_id
  uint64_t farm_id = get_farm_id(farm_name);
  farm->set_id(farm_id);

  // Get the Farm's Default replicaSet ID
  uint64_t default_rs_id = get_farm_default_rs_id(farm_name);

  // Get the default replicaset from the Farm
  boost::shared_ptr<ReplicaSet> default_rs = get_replicaset(default_rs_id);

  // Update the default replicaset_id
  farm->set_default_replicaset(default_rs);

  return farm;
}

bool MetadataStorage::has_default_farm()
{
  std::string query;
  boost::shared_ptr< ::mysqlx::Result> result;
  boost::shared_ptr< ::mysqlx::Row> row;

  if (metadata_schema_exists())
  {
    try {
      query = "SELECT farm_id from farm_metadata_schema.farms WHERE attributes->\"$.default\" = true";
      result = _admin_session->get_session().execute_sql(query);
    }
    catch (::mysqlx::Error &e) {
      if ((CR_SERVER_GONE_ERROR || ER_X_BAD_PIPE) == e.error())
        throw Exception::metadata_error("The Metadata is inaccessible");
      else
        throw;
    }

    row = result->next();

    if (!row) return false;

    return true;
  }
  return false;
}
