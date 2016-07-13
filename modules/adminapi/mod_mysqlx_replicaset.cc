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
_name(name)
{
  init();
}

ReplicaSet::~ReplicaSet()
{
}

bool ReplicaSet::operator == (const Object_bridge &other) const
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
 * \li name: returns a String object with the name of this ReplicaSet object.
 */
#else
/**
* Returns the name of this ReplicaSet object.
* \return the name as an String object.
*/
#if DOXYGEN_JS
String ReplicaSet::getName(){}
#elif DOXYGEN_PY
str ReplicaSet::get_name(){}
#endif
#endif
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

#if DOXYGEN_CPP
/**
 * Use this function to add a Node to the ReplicaSet object
 * \param args : A list of values to be used to add a Node to the ReplicaSet.
 *
 * This function returns an empty Value.
 */
#else
/**
* Adds a Node to the ReplicaSet
* \param conn The Connection String or URI of the Node to be added
*/
#if DOXYGEN_JS
Undefined ReplicaSet::addNode(String conn){}
#elif DOXYGEN_PY
None ReplicaSet::add_node(str conn){}
#endif
/**
* Adds a Node to the ReplicaSet
* \param doc The Document representing the Node to be added
*/
#if DOXYGEN_JS
Undefined ReplicaSet::addNode(Document doc){}
#elif DOXYGEN_PY
None ReplicaSet::add_node(Document doc){}
#endif
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

  if(options->size() == 0)
    throw shcore::Exception::argument_error("Connection data empty.");

  if (options->has_key("host"))
    host = (*options)["host"].as_string();

  if (options->has_key("port"))
    port = (*options)["port"].as_int();

  if (options->has_key("socket"))
    sock = (*options)["socket"].as_string();

  if (options->has_key("schema"))
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  if (options->has_key("user") || options->has_key("dbUser"))
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  if (options->has_key("password") || options->has_key("dbPassword"))
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

  // TODO: validate additional data.

  std::string sock_port = (port == 0) ? sock : boost::lexical_cast<std::string>(port);

  // Handle empty required values
  if (!options->has_key("host"))
    throw shcore::Exception::argument_error("Missing required value for hostname.");

  // Add the Node on the Metadata Schema
  // TODO!

  return Value();
}

#if DOXYGEN_CPP
/**
 * Use this function to remove a Node from the ReplicaSet object
 * \param args : A list of values to be used to remove a Node to the Farm.
 *
 * This function returns an empty Value.
 */
#else
/**
* Removes a Node from the ReplicaSet
* \param name The name of the Node to be removed
*/
#if DOXYGEN_JS
Undefined ReplicaSet::removeNode(String name){}
#elif DOXYGEN_PY
None ReplicaSet::remove_node(str name){}
#endif

/**
* Removes a Node from the ReplicaSet
* \param doc The Document representing the Node to be removed
*/
#if DOXYGEN_JS
Undefined ReplicaSet::removeNode(Document doc){}
#elif DOXYGEN_PY
None ReplicaSet::remove_node(Document doc){}
#endif
#endif

shcore::Value ReplicaSet::remove_node(const shcore::Argument_list &args)
{
  args.ensure_count(1, "ReplicaSet.removeNode");

  // TODO!

  return Value();
}
