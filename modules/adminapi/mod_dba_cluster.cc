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

#include "mod_dba_cluster.h"

#include "common/uuid/include/uuid_gen.h"
#include <sstream>
#include <iostream>
#include <iomanip>

#include "my_aes.h"

#include "mod_dba_replicaset.h"
#include "mod_dba_metadata_storage.h"
#include "../mysqlxtest_utils.h"

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

Cluster::Cluster(const std::string &name, std::shared_ptr<MetadataStorage> metadata_storage) :
_name(name), _metadata_storage(metadata_storage), _json_mode(JSON_STANDARD_OUTPUT)
{
  init();
}

Cluster::~Cluster()
{
}

std::string &Cluster::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const
{
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

bool Cluster::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function.
 * The content of the returned value depends on the property being requested.
 * The next list shows the valid properties as well as the returned value for each of them:
 *
 * \li name: returns a String object with the name of this Cluster object.
 * \li adminType: returns the admin Type for this Cluster object.
 */
#else
/**
* Returns the name of this Cluster object.
* \return the name as an String object.
*/
#if DOXYGEN_JS
String Cluster::getName(){}
#elif DOXYGEN_PY
str Cluster::get_name(){}
#endif
/**
* Returns the admin type of this Cluster object.
* \return the admin type as an String object.
*/
#if DOXYGEN_JS
Cluster Cluster::getAdminType(){}
#elif DOXYGEN_PY
Cluster Cluster::get_admin_type(){}
#endif
#endif
shcore::Value Cluster::get_member(const std::string &prop) const
{
  shcore::Value ret_val;
  if (prop == "name")
    ret_val = shcore::Value(_name);
  else if (prop == "adminType")
    ret_val = (*_options)[OPT_ADMIN_TYPE];
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);

  return ret_val;
}

void Cluster::init()
{
  add_property("name", "getName");
  add_property("adminType", "getAdminType");
  add_method("addInstance", std::bind(&Cluster::add_instance, this, _1), "data");
  add_method("rejoinInstance", std::bind(&Cluster::rejoin_instance, this, _1), "data");
  add_method("removeInstance", std::bind(&Cluster::remove_instance, this, _1), "data");
  add_method("getReplicaSet", std::bind(&Cluster::get_replicaset, this, _1), "name", shcore::String, NULL);
  add_method("describe", std::bind(&Cluster::describe, this, _1), NULL);
  add_method("status", std::bind(&Cluster::status, this, _1), NULL);
}

/**
* Retrieves the name of the Cluster object
* \return The Cluster name
*/
#if DOXYGEN_JS
String Cluster::getName(){}
#elif DOXYGEN_PY
str Cluster::get_name(){}
#endif

/**
* Retrieves the Administration type of the Cluster object
* \return The Administration type
*/
#if DOXYGEN_JS
String Cluster::getAdminType(){}
#elif DOXYGEN_PY
str Cluster::get_admin_type(){}
#endif

//#if DOXYGEN_CPP
///**
// * Use this function to add a Seed Instance to the Cluster object
// * \param args : A list of values to be used to add a Seed Instance to the Cluster.
// *
// * This function creates the Default ReplicaSet implicitly and adds the Instance to it
// * This function returns an empty Value.
// */
//#else
///**
//* Adds a Seed Instance to the Cluster
//* \param conn The Connection String or URI of the Instance to be added
//*/
//#if DOXYGEN_JS
//Undefined addSeedInstance(String conn){}
//#elif DOXYGEN_PY
//None add_seed_instance(str conn){}
//#endif
///**
//* Adds a Seed Instance to the Cluster
//* \param doc The Document representing the Instance to be added
//*/
//#if DOXYGEN_JS
//Undefined addSeedInstance(Document doc){}
//#elif DOXYGEN_PY
//None add_seed_instance(Document doc){}
//#endif
//#endif

shcore::Value Cluster::add_seed_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  //args.ensure_count(1, 2, (class_name() + ".addSeedInstance").c_str());

  //try
  //{
  MetadataStorage::Transaction tx(_metadata_storage);
  std::string default_replication_user = "rpl_user"; // Default for V1.0 is rpl_user
  std::shared_ptr<ReplicaSet> default_rs = get_default_replicaset();

  // Check if we have a Default ReplicaSet, if so it means we already added the Seed Instance
  if (default_rs != NULL)
  {
    uint64_t rs_id = default_rs->get_id();
    if (!_metadata_storage->is_replicaset_empty(rs_id))
      throw shcore::Exception::logic_error("Default ReplicaSet already initialized. Please use: addInstance() to add more Instances to the ReplicaSet.");
  }
  else
  {
    // Create the Default ReplicaSet and assign it to the Cluster's default_replica_set var
    _default_replica_set.reset(new ReplicaSet("default", _metadata_storage));

    _default_replica_set->set_cluster(shared_from_this());

    // If we reached here without errors we can update the Metadata

    // Update the Cluster table with the Default ReplicaSet on the Metadata
    _metadata_storage->insert_default_replica_set(shared_from_this());
  }

  // Add the Instance to the Default ReplicaSet
  ret_val = _default_replica_set->add_instance(args);
  tx.commit();
  //}
  //CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addSeedInstance"));

  return ret_val;
}

#if DOXYGEN_CPP
/**
 * Use this function to add a Instance to the Cluster object
 * \param args : A list of values to be used to add a Instance to the Cluster.
 *
 * This function calls ReplicaSet::add_instance(args).
 * This function returns an empty Value.
 */
#else
/**
* Adds a Instance to the Cluster
* \param conn The Connection String or URI of the Instance to be added
*/
#if DOXYGEN_JS
Undefined addInstance(String conn){}
#elif DOXYGEN_PY
None add_instance(str conn){}
#endif
/**
* Adds a Instance to the Cluster
* \param doc The Document representing the Instance to be added
*/
#if DOXYGEN_JS
Undefined addInstance(Document doc){}
#elif DOXYGEN_PY
None add_instance(Document doc){}
#endif
#endif

shcore::Value Cluster::add_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("addInstance").c_str());

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  // Add the Instance to the Default ReplicaSet
  try
  {
    ret_val = _default_replica_set->add_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  return ret_val;
}

#if DOXYGEN_CPP
/**
 * Use this function to rejoin an Instance in its replicaset
 * \param conn : The hostname:port of the instance to be rejoined
 *
 * This function returns an empty Value.
 */
#else
/**
* Rejoins an Instance to the Cluster
* \param conn The Connection String or URI of the Instance to be rejoined
*/
#if DOXYGEN_JS
Undefined rejoinInstance(String conn){}
#elif DOXYGEN_PY
None rejoin_instance(str conn){}
#endif
#endif
shcore::Value Cluster::rejoin_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  args.ensure_count(1, 2, get_function_name("rejoinInstance").c_str());
  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");
  // rejoin the Instance to the Default ReplicaSet
  try
  {
    // if not, call mysqlprovision to join the instance to its own group
    ret_val = _default_replica_set->rejoin_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  return ret_val;
}

#if DOXYGEN_CPP
/**
 * Use this function to remove a Instance from the Cluster object
 * \param args : A list of values to be used to remove a Instance to the Cluster.
 *
 * This function calls ReplicaSet::remove_instance(args).
 * This function returns an empty Value.
 */
#else
/**
* Removes a Instance from the Cluster
* \param name The name of the Instance to be removed
*/
#if DOXYGEN_JS
Undefined removeInstance(String name){}
#elif DOXYGEN_PY
None remove_instance(str name){}
#endif
/**
* Removes a Instance from the Cluster
* \param doc The Document representing the Instance to be removed
*/
#if DOXYGEN_JS
Undefined removeInstance(Document doc){}
#elif DOXYGEN_PY
None remove_instance(Document doc){}
#endif
#endif

shcore::Value Cluster::remove_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("removeInstance").c_str());

  // Remove the Instance from the Default ReplicaSet
  try
  {
    _default_replica_set->remove_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeInstance"));

  return Value();
}

/**
* Returns the ReplicaSet of the given name.
* \sa ReplicaSet
* \param name the name of the ReplicaSet to look for.
* \return the ReplicaSet object matching the name.
*
* Verifies if the requested Collection exist on the metadata schema, if exists, returns the corresponding ReplicaSet object.
*/
#if DOXYGEN_JS
ReplicaSet Cluster::getReplicaSet(String name){}
#elif DOXYGEN_PY
ReplicaSet Cluster::get_replica_set(str name){}
#endif

shcore::Value Cluster::get_replicaset(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  if (args.size() == 0)
    ret_val = shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(_default_replica_set));

  else
  {
    args.ensure_count(1, get_function_name("getReplicaSet").c_str());
    std::string name = args.string_at(0);

    if (name == "default")
      ret_val = shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(_default_replica_set));

    else
    {
      /*
       * Retrieve the ReplicaSet from the ReplicaSets array
       */
      //if (found)

      //else
      //  throw shcore::Exception::runtime_error("The ReplicaSet " + _name + "." + name + " does not exist");
    }
  }

  return ret_val;
}

void Cluster::set_default_replicaset(std::shared_ptr<ReplicaSet> default_rs)
{
  _default_replica_set = default_rs;

  if (_default_replica_set)
    _default_replica_set->set_cluster(shared_from_this());
};

void Cluster::append_json(shcore::JSON_dumper& dumper) const
{
  if (_json_mode)
  {
    dumper.start_object();
    dumper.append_string("clusterName", _name);

    if (!_default_replica_set)
      dumper.append_null("defaultReplicaSet");
    else
    {
      if (_json_mode == JSON_TOPOLOGY_OUTPUT)
        dumper.append_string("adminType", (*_options)[OPT_ADMIN_TYPE].as_string());

      _default_replica_set->set_json_mode(_json_mode);
      dumper.append_value("defaultReplicaSet", shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(_default_replica_set)));
      _default_replica_set->set_json_mode(JSON_STANDARD_OUTPUT);
    }

    dumper.end_object();
  }
  else
    Cpp_object_bridge::append_json(dumper);
}

/**
* Returns a formatted JSON describing the structure of the Cluster
*/
#if DOXYGEN_JS
String Cluster::describe(){}
#elif DOXYGEN_PY
str Cluster::describe(){}
#endif
shcore::Value Cluster::describe(const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  _json_mode = JSON_TOPOLOGY_OUTPUT;
  shcore::Value myself = shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(shared_from_this()));
  ret_val = shcore::Value(myself.json(true));
  _json_mode = JSON_STANDARD_OUTPUT;

  return ret_val;
}

/**
* Returns a formatted JSON describing the status of the Cluster
*/
#if DOXYGEN_JS
String Cluster::status(){}
#elif DOXYGEN_PY
str Cluster::status(){}
#endif
shcore::Value Cluster::status(const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  _json_mode = JSON_STATUS_OUTPUT;
  shcore::Value myself = shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(shared_from_this()));
  ret_val = shcore::Value(myself.json(true));
  _json_mode = JSON_STANDARD_OUTPUT;

  return ret_val;
}

void Cluster::set_account_data(const std::string& account, const std::string& key, const std::string& value)
{
  if (!_accounts)
    _accounts.reset(new shcore::Value::Map_type());

  if (!_accounts->has_key(account))
    (*_accounts)[account] = shcore::Value::new_map();

  auto account_data = (*_accounts)[account].as_map();
  (*account_data)[key] = shcore::Value(value);
}

std::string Cluster::get_account_data(const std::string& account, const std::string& key)
{
  std::string ret_val;

  if (_accounts && _accounts->has_key(account))
  {
    ret_val = _accounts->get_map(account)->get_string(key);
  }
  return ret_val;
}

std::string Cluster::get_accounts_data()
{
  shcore::Value::Map_type_ref accounts = _accounts;

  // Hide the MASTER key
  auto account_data = (*accounts)["clusterAdmin"].as_map();
  account_data->erase("password");

  std::string data(shcore::Value(accounts).json(false));

  std::string dest;
  size_t rounded_size = myaes::my_aes_get_size(static_cast<uint32_t>(data.length()),
                                    myaes::my_aes_128_ecb);
  if (data.length() < rounded_size)
    data.append(rounded_size - data.length(), ' ');
  dest.resize(rounded_size);

  if (myaes::my_aes_encrypt(reinterpret_cast<const unsigned char*>(data.data()),
                 static_cast<uint32_t>(data.length()),
                 reinterpret_cast<unsigned char*>(&dest[0]),
                 reinterpret_cast<const unsigned char*>(_master_key.data()),
                 static_cast<uint32_t>(_master_key.length()),
                 myaes::my_aes_128_ecb, NULL, false) < 0)
    throw shcore::Exception::logic_error("Error encrypting account information");
  return dest;
}

/** Set cluster account data as stored in metadata table
 *
 * The account data is expected to a JSON string, AES encrypted.
 */
void Cluster::set_accounts_data(const std::string& encrypted_json)
{
  std::string decrypted_data;
  decrypted_data.resize(encrypted_json.length());
  int len;
  if ((len = myaes::my_aes_decrypt(reinterpret_cast<const unsigned char*>(encrypted_json.data()),
                     static_cast<uint32_t>(encrypted_json.length()),
                     reinterpret_cast<unsigned char*>(&decrypted_data[0]),
                     reinterpret_cast<const unsigned char*>(_master_key.data()),
                     static_cast<uint32_t>(_master_key.length()),
                     myaes::my_aes_128_ecb, NULL, false)) < 0)
    throw shcore::Exception::logic_error("Error decrypting account information");

  decrypted_data.resize(len);

  try
  {
    _accounts = shcore::Value::parse(decrypted_data).as_map();
  }
  catch (shcore::Exception &e)
  {
    if (e.is_parser())
    {
      log_info("DBA: Error parsing account data for cluster '%s': %s",
              _name.c_str(), e.format().c_str());
      throw Exception::logic_error("Unable to decrypt account information");
    }
    throw;
  }
}

void Cluster::set_option(const std::string& option, const shcore::Value& value)
{
  if (!_options)
    _options.reset(new shcore::Value::Map_type());

  (*_options)[option] = value;
}

void Cluster::set_attribute(const std::string& attribute, const shcore::Value& value)
{
  if (!_attributes)
    _attributes.reset(new shcore::Value::Map_type());

  (*_attributes)[attribute] = value;
}
