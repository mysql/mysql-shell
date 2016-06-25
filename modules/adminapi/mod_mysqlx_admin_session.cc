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

#include "utils/utils_sqlstring.h"
#include "mod_mysqlx_admin_session.h"
#include "shellcore/object_factory.h"
#include "../mysqlxtest_utils.h"

#include "logger/logger.h"

#include <boost/bind.hpp>
#include <boost/pointer_cast.hpp>

#include "mod_mysqlx_farm.h"

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

REGISTER_OBJECT(mysqlx, AdminSession);

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
  // In case we are going to keep a cache of Farms
  // If not, _farms can be removed
  _farms.reset(new shcore::Value::Map_type);

  // Note this one is a function that has a property equivalent: getDefaultFarm/defaultFarm
  add_method("getDefaultFarm", boost::bind(&AdminSession::get_member_method, this, _1, "getDefaultFarm", "defaultFarm"), NULL);

  // Pure functions
  add_method("createFarm", boost::bind(&AdminSession::create_farm, this, _1), "farmName", shcore::String, NULL);
  add_method("dropFarm", boost::bind(&AdminSession::drop_farm, this, _1), "farmName", shcore::String, NULL);
  add_method("getFarm", boost::bind(&AdminSession::get_farm, this, _1), "farmName", shcore::String, NULL);
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

/*
 * This function verifies if the given object exist in the database, works for schemas, tables, views and collections.
 * The check for tables, views and collections is done is done based on the type.
 * If type is not specified and an object with the name is found, the type will be returned.
 *
 * Returns the name of the object as exists in the database.
 */
std::string AdminSession::db_object_exists(std::string &type, const std::string &name, const std::string& owner) const
{
  return _session.db_object_exists(type, name, owner);
}


#if DOXYGEN_JS
/**
* \brief Closes the session.
* After closing the session it is still possible to make read only operation to gather metadata info, like getTable(name) or getSchemas().
*/
Undefined AdminSession::close(){}
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

bool AdminSession::has_member(const std::string &prop) const
{
  if (ShellAdminSession::has_member(prop))
    return true;
  if (prop == "defaultFarm" || prop == "uri")
    return true;

  return false;
}

std::vector<std::string> AdminSession::get_members() const
{
  std::vector<std::string> members(ShellAdminSession::get_members());
  members.push_back("defaultFarm");
  return members;
}


#if DOXYGEN_JS
/**
* Retrieves the Farm configured as default on this Metadata instance.
* \return A Farm object or Null
*/
Farm AdminSession::getDefaultFarm(){}

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
  else if (prop == "defaultFarm")
  {
    // TODO: If there is a default farm and we have the name, retrieve it with the next call
    if (/*!_default_farm.empty()*/0)
    {
      //shcore::Argument_list args;
      //args.push_back(shcore::Value(_default_farm));
      //ret_val = get_farm(args);
    }
    else
      ret_val = Value::Null();
  }

  return ret_val;
}

#if DOXYGEN_JS
/**
* Retrieves a Farm object from the current session through it's name.
* \param name The name of the Farm object to be retrieved.
* \return The Farm object with the given name.
* \sa Farm
*/
Farm AdminSession::getFarm(String name){}
#endif
shcore::Value AdminSession::get_farm(const shcore::Argument_list &args) const
{
  args.ensure_count(1, "AdminSession.getFarm");

  // TODO: just do it!

  return shcore::Value();
}

#ifdef DOXYGEN
/**
 * Creates a Farm object.
 * \param name The name of the Farm object to be retrieved.
 * \return The created Farm object.
 * \sa Farm
 */
Farm AdminSession::createFarm(String name){}
#endif
shcore::Value AdminSession::create_farm(const shcore::Argument_list &args)
{
  Value ret_val;
  args.ensure_count(1, "AdminSession.createFarm");

  if (!_session.is_connected())
    throw Exception::logic_error("Not connected.");
  else
  {
    std::string name = args.string_at(0);

    if (name.empty())
      throw Exception::argument_error("The Farm name cannot be empty.");
    else
    {
      boost::shared_ptr<Farm> farm (new Farm(name));

      // If reaches this point it indicates the Farm was created successfully
      ret_val = Value(boost::static_pointer_cast<Object_bridge>(farm));
      (*_farms)[name] = ret_val;

      // Now we need to create the Farm on the Metadata Schema: TODO!
      //std::string query = "CREATE ...";
      //_session.execute_sql(query);
    }
  }

  return ret_val;
}

#if DOXYGEN_JS
/**
 * Drops a Farm object.
 * \param name The name of the Farm object to be dropped.
 * \return nothing.
 * \sa Farm
 */
None minSession::dropFarm(String name){}
#endif
shcore::Value AdminSession::drop_farm(const shcore::Argument_list &args)
{
  args.ensure_count(1, "AdminSession.dropFarm");

  //TODO: just do it!

  return shcore::Value();
}

shcore::Value AdminSession::get_capability(const std::string& name)
{
  return _session.get_capability(name);
}


// TODO: Careful wit this one, this means the status printed on the shell with the \s command
shcore::Value AdminSession::get_status(const shcore::Argument_list &args)
{
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  (*status)["SESSION_TYPE"] = shcore::Value("Admin");

  shcore::Value node_type = get_capability("node_type");
  if (node_type)
    (*status)["NODE_TYPE"] = node_type;

  // TODO: Uncomment or Delete
  // (*status)["DEFAULT_FARM"] = shcore::Value(_default_farm);

  return shcore::Value(status);
}

boost::shared_ptr<shcore::Object_bridge> AdminSession::create(const shcore::Argument_list &args)
{
  return connect_admin_session(args);
}