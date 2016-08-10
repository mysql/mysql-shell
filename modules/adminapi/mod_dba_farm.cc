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

#include "mod_dba_farm.h"

#include "common/uuid/include/uuid_gen.h"
#include <sstream>
#include <iostream>
#include <iomanip>

#include "mod_dba_replicaset.h"
#include "mod_dba_metadata_storage.h"
#include "../mysqlxtest_utils.h"

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

Farm::Farm(const std::string &name, std::shared_ptr<MetadataStorage> metadata_storage) :
_name(name), _metadata_storage(metadata_storage)
{
  init();
}

Farm::~Farm()
{
}

std::string &Farm::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const
{
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

bool Farm::operator == (const Object_bridge &other) const
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
 * \li name: returns a String object with the name of this Farm object.
 * \li adminType: returns the admin Type for this Farm object.
 */
#else
/**
* Returns the name of this Farm object.
* \return the name as an String object.
*/
#if DOXYGEN_JS
String Farm::getName(){}
#elif DOXYGEN_PY
str Farm::get_name(){}
#endif
/**
* Returns the admin type of this Farm object.
* \return the admin type as an String object.
*/
#if DOXYGEN_JS
Farm Farm::getAdminType(){}
#elif DOXYGEN_PY
Farm Farm::get_admin_type(){}
#endif
#endif
shcore::Value Farm::get_member(const std::string &prop) const
{
  shcore::Value ret_val;
  if (prop == "name")
    ret_val = shcore::Value(_name);
  else if (prop == "adminType")
    ret_val = shcore::Value(_admin_type);
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);

  return ret_val;
}

void Farm::init()
{
  add_property("name", "getName");
  add_property("adminType", "getAdminType");
  add_method("addSeedInstance", std::bind(&Farm::add_seed_instance, this, _1), "data");
  add_method("addInstance", std::bind(&Farm::add_instance, this, _1), "data");
  add_method("removeInstance", std::bind(&Farm::remove_instance, this, _1), "data");
  add_method("getReplicaSet", std::bind(&Farm::get_replicaset, this, _1), "name", shcore::String, NULL);
}

/**
* Retrieves the name of the Farm object
* \return The Farm name
*/
#if DOXYGEN_JS
String Farm::getName(){}
#elif DOXYGEN_PY
str Farm::get_name(){}
#endif

/**
* Retrieves the Administration type of the Farm object
* \return The Administration type
*/
#if DOXYGEN_JS
String Farm::getAdminType(){}
#elif DOXYGEN_PY
str Farm::get_admin_type(){}
#endif

#if DOXYGEN_CPP
/**
 * Use this function to add a Seed Instance to the Farm object
 * \param args : A list of values to be used to add a Seed Instance to the Farm.
 *
 * This function creates the Default ReplicaSet implicitly and adds the Instance to it
 * This function returns an empty Value.
 */
#else
/**
* Adds a Seed Instance to the Farm
* \param conn The Connection String or URI of the Instance to be added
*/
#if DOXYGEN_JS
Undefined addSeedInstance(String conn){}
#elif DOXYGEN_PY
None add_seed_instance(str conn){}
#endif
/**
* Adds a Seed Instance to the Farm
* \param doc The Document representing the Instance to be added
*/
#if DOXYGEN_JS
Undefined addSeedInstance(Document doc){}
#elif DOXYGEN_PY
None add_seed_instance(Document doc){}
#endif
#endif

shcore::Value Farm::add_seed_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  args.ensure_count(1, (class_name() + ".addSeedInstance").c_str());

  try
  {
    std::string default_replication_user = "rpl_user"; // Default for V1.0 is rpl_user
    std::shared_ptr<ReplicaSet> default_rs = get_default_replicaset();

    // Check if we have a Default ReplicaSet, if so it means we already added the Seed Instance
    if (default_rs != NULL)
    {
      uint64_t rs_id = default_rs->get_id();
      if (!_metadata_storage->is_replicaset_empty(rs_id))
        throw shcore::Exception::logic_error("Default ReplicaSet already initialized. Please use: addInstance() to add more Instances to the ReplicaSet.");
    }

    // Create the Default ReplicaSet and assign it to the Farm's default_replica_set var
    _default_replica_set.reset(new ReplicaSet("default", _metadata_storage));

    _default_replica_set->set_replication_user(default_replication_user);

    // If we reached here without errors we can update the Metadata

    // Update the Farm table with the Default ReplicaSet on the Metadata
    _metadata_storage->insert_default_replica_set(shared_from_this());

    // Add the Instance to the Default ReplicaSet
    ret_val = _default_replica_set->add_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addSeedInstance"));

  return ret_val;
}

#if DOXYGEN_CPP
/**
 * Use this function to add a Instance to the Farm object
 * \param args : A list of values to be used to add a Instance to the Farm.
 *
 * This function calls ReplicaSet::add_instance(args).
 * This function returns an empty Value.
 */
#else
/**
* Adds a Instance to the Farm
* \param conn The Connection String or URI of the Instance to be added
*/
#if DOXYGEN_JS
Undefined addInstance(String conn){}
#elif DOXYGEN_PY
None add_instance(str conn){}
#endif
/**
* Adds a Instance to the Farm
* \param doc The Document representing the Instance to be added
*/
#if DOXYGEN_JS
Undefined addInstance(Document doc){}
#elif DOXYGEN_PY
None add_instance(Document doc){}
#endif
#endif

shcore::Value Farm::add_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  args.ensure_count(1, get_function_name("addInstance").c_str());

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized. Please add the Seed Instance using: addSeedInstance().");

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
 * Use this function to remove a Instance from the Farm object
 * \param args : A list of values to be used to remove a Instance to the Farm.
 *
 * This function calls ReplicaSet::remove_instance(args).
 * This function returns an empty Value.
 */
#else
/**
* Removes a Instance from the Farm
* \param name The name of the Instance to be removed
*/
#if DOXYGEN_JS
Undefined removeInstance(String name){}
#elif DOXYGEN_PY
None remove_instance(str name){}
#endif
/**
* Removes a Instance from the Farm
* \param doc The Document representing the Instance to be removed
*/
#if DOXYGEN_JS
Undefined removeInstance(Document doc){}
#elif DOXYGEN_PY
None remove_instance(Document doc){}
#endif
#endif

shcore::Value Farm::remove_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, get_function_name("removeInstance").c_str());

  // Remove the Instance from the Default ReplicaSet
  _default_replica_set->remove_instance(args);

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
ReplicaSet Farm::getReplicaSet(String name){}
#elif DOXYGEN_PY
ReplicaSet Farm::get_replica_set(str name){}
#endif

shcore::Value Farm::get_replicaset(const shcore::Argument_list &args)
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