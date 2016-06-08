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

#include "mod_mysqlx_replicaset.h"

#include "common/uuid/include/uuid_gen.h"
#include "utils/utils_general.h"

#include <sstream>
#include <iostream>
#include <iomanip>

#include <boost/lexical_cast.hpp>

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

ReplicaSet::ReplicaSet(const std::string &name) :
_uuid(new_uuid()), _name(name)
{
  init();
}

ReplicaSet::~ReplicaSet()
{
}

std::string ReplicaSet::new_uuid()
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

bool ReplicaSet::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}

shcore::Value ReplicaSet::get_member(const std::string &prop) const
{
  shcore::Value ret_val;
  if (prop == "name")
    ret_val = shcore::Value(_name);
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);

  return ret_val;
}

void ReplicaSet::init()
{
  add_property("name", "getName");
  add_method("addNode", boost::bind(&ReplicaSet::add_node, this, _1), "data");
  add_method("removeNode", boost::bind(&ReplicaSet::remove_node, this, _1), "data");
}

#ifdef DOXYGEN
/**
* Retrieves the name of the ReplicaSet object
* \return The ReplicaSet name
*/
String ReplicaSet::get_name(){}

/**
* Adds a Node to the ReplicaSet
* \param conn The Connection String or URI of the Node to be added
*/
None addNode(std::string conn){}

/**
* Adds a Node to the ReplicaSet
* \param doc The Document representing the Node to be added
*/
None addNode(Document doc){}
#endif

shcore::Value ReplicaSet::add_node(const shcore::Argument_list &args)
{
  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string protocol;
  std::string user;
  std::string password;
  std::string host;
  int port = 0;
  std::string sock;
  std::string schema;
  std::string ssl_ca;
  std::string ssl_cert;
  std::string ssl_key;

  args.ensure_count(1, (class_name() + ".addNode").c_str());

  // Identify the type of connection data (String or Document)
  if (args[0].type == String)
  {
    uri = args.string_at(0);
    options = get_connection_data(uri, false);
  }

  // Connection data comes in a dictionary
  else if (args[0].type == Map)
    options = args.map_at(0);

  else
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  if (options->has_key("host"))
    host = (*options)["host"].as_string();

  if (options->has_key("port"))
    port = (*options)["port"].as_int();

  if (options->has_key("socket"))
    sock = (*options)["socket"].as_string();

  if (options->has_key("schema"))
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  if (options->has_key("user"))
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  if (options->has_key("password"))
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  if (options->has_key("ssl_ca"))
    ssl_ca = (*options)["ssl_ca"].as_string();

  if (options->has_key("ssl_cert"))
    ssl_cert = (*options)["ssl_cert"].as_string();

  if (options->has_key("ssl_key"))
    ssl_key = (*options)["ssl_key"].as_string();

  if (options->has_key("authMethod"))
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  if (port == 0 && sock.empty())
    port = get_default_port();

  std::string sock_port = (port == 0) ? sock : boost::lexical_cast<std::string>(port);

  // Add the Node on the Metadata Schema
  // TODO!

  return Value();
}

#ifdef DOXYGEN
/**
* Removes a Node from the ReplicaSet
* \param name The name of the Node to be removed
*/
None removeNode(std::string name){}

/**
* Removes a Node from the ReplicaSet
* \param doc The Document representing the Node to be removed
*/
None removeNode(Document doc){}
#endif

shcore::Value ReplicaSet::remove_node(const shcore::Argument_list &args)
{
  args.ensure_count(1, "ReplicaSet.removeNode");

  // TODO!

  return Value();
}
