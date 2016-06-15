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

#include <boost/bind.hpp>

#include "mod_mysqlx_farm.h"

#include "common/uuid/include/uuid_gen.h"
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

Farm::Farm(const std::string &name) :
_uuid(new_uuid()), _name(name), _default_replica_set(new ReplicaSet("default"))
{
  init();
}

Farm::~Farm()
{
}

std::string Farm::new_uuid()
{
  uuid_type uuid;
  generate_uuid(uuid);

  std::stringstream str;
  str << std::hex << std::noshowbase << std::setfill('0') << std::setw(2);

  str << (int)uuid[0] << std::setw(2) << (int)uuid[1] << std::setw(2) << (int)uuid[2] << std::setw(2) << (int)uuid[3];
  str << "-" << std::setw(2) << (int)uuid[4] << std::setw(2) << (int)uuid[5];
  str << "-" << std::setw(2) << (int)uuid[6] << std::setw(2) << (int)uuid[7];
  str << "-" << std::setw(2) << (int)uuid[8] << std::setw(2) << (int)uuid[9];
  str << "-" << std::setw(2) << (int)uuid[10] << std::setw(2) << (int)uuid[11]
    << std::setw(2) << (int)uuid[12] << std::setw(2) << (int)uuid[13]
    << std::setw(2) << (int)uuid[14] << std::setw(2) << (int)uuid[15];

  return str.str();
}

bool Farm::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}

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
  add_method("addNode", boost::bind(&Farm::add_node, this, _1), "data");
  add_method("removeNode", boost::bind(&Farm::remove_node, this, _1), "data");
  add_method("getReplicaSet", boost::bind(&Farm::get_replicaset, this, _1), "name", shcore::String, NULL);
}

#ifdef DOXYGEN
/**
* Retrieves the name of the Farm object
* \return The Farm name
*/
String Farm::getName(){}

/**
* Retrieves the Administration type of the Farm object
* \return The Administration type
*/
String Farm::getAdminType(){}

/**
* Adds a Node to the Farm
* \param conn The Connection String or URI of the Node to be added
*/
None addNode(std::string conn){}

/**
* Adds a Node to the Farm
* \param doc The Document representing the Node to be added
*/
None addNode(Document doc){}
#endif

shcore::Value Farm::add_node(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + ".addNode").c_str());

  // Add the Node to the Default ReplicaSet
  _default_replica_set->add_node(args);

  return Value();
}

#ifdef DOXYGEN
/**
* Removes a Node from the From
* \param name The name of the Node to be removed
*/
None removeNode(std::string name){}

/**
* Removes a Node from the From
* \param doc The Document representing the Node to be removed
*/
None removeNode(Document doc){}
#endif

shcore::Value Farm::remove_node(const shcore::Argument_list &args)
{
  args.ensure_count(1, (class_name() + ".removeNode").c_str());

  // Remove the Node from the Default ReplicaSet
  _default_replica_set->remove_node(args);

  return Value();
}

#ifdef DOXYGEN
/**
* Returns the ReplicaSet of the given name.
* \sa ReplicaSet
* \param name the name of the ReplicaSet to look for.
* \return the ReplicaSet object matching the name.
*
* Verifies if the requested Collection exist on the metadata schema, if exists, returns the corresponding ReplicaSet object.
*/
ReplicaSet Farm::getReplicaSet(String name){}
#endif

shcore::Value Farm::get_replicaset(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  if (args.size() == 0)
    ret_val = shcore::Value(_default_replica_set);

  else
  {
    args.ensure_count(1, (class_name() + ".getReplicaSet").c_str());
    std::string name = args.string_at(0);

    if (name == "default")
      ret_val = shcore::Value(boost::dynamic_pointer_cast<shcore::Object_bridge>(_default_replica_set));

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