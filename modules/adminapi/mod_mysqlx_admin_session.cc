/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "utils/utils_sqlstring.h"
#include "mod_mysqlx_admin_session.h"
#include "../mysqlxtest_utils.h"

#include "logger/logger.h"

#include <boost/bind.hpp>

using namespace mysh;
using namespace shcore;
using namespace mysh::mysqlx;

AdminSession::AdminSession()
{
  init();
}

bool AdminSession::is_connected() const
{
  return _session.is_connected();
}

AdminSession::AdminSession(const AdminSession& s) : ShellAdminSession(s)
{
  init();
}

void AdminSession::init()
{
  _clusters.reset(new shcore::Value::Map_type);

  add_method("close", boost::bind(&AdminSession::close, this, _1), "data");
}

Value AdminSession::connect(const Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("connect").c_str());

  try
  {
    // Retrieves the connection data, whatever the source is
    load_connection_data(args);

    _session.open(_host, _port, _schema, _user, _password, _ssl_ca, _ssl_cert, _ssl_key, 10000, _auth_method, true);
  }
  CATCH_AND_TRANSLATE();

  return Value::Null();
}

void AdminSession::set_option(const char *option, int value)
{
  if (strcmp(option, "trace_protocol") == 0 && _session.is_connected())
    _session.enable_protocol_trace(value != 0);
  else
    throw shcore::Exception::argument_error(std::string("Unknown option ").append(option));
}

uint64_t AdminSession::get_connection_id() const
{
  return _connection_id;
}

#ifdef DOXYGEN
/**
* \brief Closes the session.
* After closing the session it is still possible to make read only operation to gather metadata info, like getTable(name) or getSchemas().
*/
Undefined BaseSession::close(){}
#endif
Value AdminSession::close(const shcore::Argument_list &args)
{
  args.ensure_count(0, get_function_name("close").c_str());

  // Connection must be explicitly closed, we can't rely on the
  // automatic destruction because if shared across different objects
  // it may remain open
  reset_session();

  return shcore::Value();
}

void AdminSession::reset_session()
{
  try
  {
    log_warning("Closing admin session: %s", _uri.c_str());

    _session.reset();
  }
  catch (std::exception &e)
  {
    log_warning("Error occurred closing admin session: %s", e.what());
  }
}

#ifdef DOXYGEN
/**
* Retrieves the Cluster configured as default on this Metadata instance.
* \return A Cluster object or Null
*/
Cluster AdminSession::getDefaultCluster(){}

/**
* Retrieves the connection data for this session in string format.
* \return A string representing the connection data.
*/
String AdminSession::getUri(){}
#endif
Value AdminSession::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is a schema and attempt loading it as such
  if (ShellAdminSession::has_member(prop))
    ret_val = ShellAdminSession::get_member(prop);
  else if (prop == "uri")
    ret_val = Value(_uri);
  else if (prop == "defaultCluster")
  {
    if (!_default_cluster.empty())
    {
      shcore::Argument_list args;
      args.push_back(shcore::Value(_default_cluster));
      ret_val = get_cluster(args);
    }
    else
      ret_val = Value::Null();
  }

  return ret_val;
}

#ifdef DOXYGEN
/**
* Retrieves a Cluster object from the current session through it's name.
* \param name The name of the Schema object to be retrieved.
* \return The Cluster object with the given name.
* \exception An exception is thrown if the given name is not a valid schema on the AdminSession.
* \sa Schema
*/
Schema BaseSession::getCluster(String name){}
#endif
shcore::Value AdminSession::get_cluster(const shcore::Argument_list &args) const
{
  args.ensure_count(1, get_function_name("getCluster").c_str());
  shcore::Value ret_val;

  return ret_val;
}

#ifdef DOXYGEN
/**
* Retrieves the Schemas available on the session.
* \return A List containing the Schema objects available o the session.
*/
List BaseSession::getClusters(){}
#endif
shcore::Value AdminSession::get_clusters(const shcore::Argument_list &args) const
{
  args.ensure_count(0, get_function_name("getClusters").c_str());

  shcore::Value::Array_type_ref clusters(new shcore::Value::Array_type);

  return shcore::Value(clusters);
}

shcore::Value AdminSession::get_capability(const std::string& name)
{
  return _session.get_capability(name);
}

shcore::Value AdminSession::get_status(const shcore::Argument_list &args)
{
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  (*status)["SESSION_TYPE"] = shcore::Value("Admin");

  shcore::Value node_type = get_capability("node_type");
  if (node_type)
    (*status)["NODE_TYPE"] = node_type;

  (*status)["DEFAULT_CLUSTER"] = shcore::Value(_default_cluster);

  return shcore::Value(status);
}

boost::shared_ptr<shcore::Object_bridge> AdminSession::create(const shcore::Argument_list &args)
{
  return connect_session(args, mysh::Application);
}