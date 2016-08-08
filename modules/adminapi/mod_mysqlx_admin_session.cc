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
#include <random>

#include "logger/logger.h"

#include "mod_mysqlx_farm.h"
#include "mod_mysqlx_metadata_storage.h"

#include "common/process_launcher/process_launcher.h"

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

#define PASSWORD_LENGHT 16

REGISTER_OBJECT(mysqlx, AdminSession);

AdminSession::AdminSession()
{
  init();
}

bool AdminSession::is_connected() const
{
  return _session.is_connected();
}

SessionHandle AdminSession::get_session() const
{
  return _session;
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
  add_method("getDefaultFarm", std::bind(&AdminSession::get_member_method, this, _1, "getDefaultFarm", "defaultFarm"), NULL);

  // Pure functions
  add_method("createFarm", std::bind(&AdminSession::create_farm, this, _1), "farmName", shcore::String, NULL);
  add_method("dropFarm", std::bind(&AdminSession::drop_farm, this, _1), "farmName", shcore::String, NULL);
  add_method("getFarm", std::bind(&AdminSession::get_farm, this, _1), "farmName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&AdminSession::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("close", std::bind(&AdminSession::close, this, _1), NULL);
}

Value AdminSession::connect(const Argument_list &args)
{
  args.ensure_count(1, 2, "AdminSession.connect");

  try
  {
    // Retrieves the connection data, whatever the source is
    load_connection_data(args);

    _session.open(_host, _port, _schema, _user, _password, _ssl_ca, _ssl_cert, _ssl_key, 10000, _auth_method, true);

    // initializes the MetadataStorage
    _metadata_storage.reset(new MetadataStorage(shared_from_this()));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION("AdminSession.connect");

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

/**
* \brief Closes the session.
* After closing the session it is still possible to make read only operation to gather metadata info, like getTable(name) or getSchemas().
*/
#if DOXYGEN_JS
Undefined AdminSession::close(){}
#elif DOXYGEN_PY
None AdminSession::close(){}
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

std::string AdminSession::generate_password(int password_lenght)
{
  std::random_device rd;
  std::string pwd;
  const char *alphabet = "1234567890abcdefghijklmnopqrstuvwxyz";
  std::uniform_int_distribution<int> dist(0, strlen(alphabet)-1);

  for (int i = 0; i < password_lenght ; i++)
    pwd += alphabet[dist(rd)];

  return pwd;
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

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function.
 * The content of the returned value depends on the property being requested.
 * The next list shows the valid properties as well as the returned value for each of them:
 *
 * \li uri: returns a String object with a string representing the connection data for this session.
 * \li defaultFarm: returns the default Farm object.
 */
#else
/**
* Retrieves the connection data for this session in string format.
* \return A string representing the connection data.
*/
#if DOXYGEN_JS
String AdminSession::getUri(){}
#elif DOXYGEN_PY
str AdminSession::get_uri(){}
#endif
/**
* Retrieves the Farm configured as default on this Metadata instance.
* \return A Farm object or Null
*/
#if DOXYGEN_JS
Farm AdminSession::getDefaultFarm(){}
#elif DOXYGEN_PY
Farm AdminSession::get_default_farm(){}
#endif
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
    // If there is a default farm and we have the name, retrieve it with the next call
    if (!_default_farm.empty())
    {
      shcore::Argument_list args;
      args.push_back(shcore::Value(_default_farm));
      ret_val = get_farm(args);
    }
    // For V1 we only support one Farm. Check if there's a Farm on the MD and update _default_farm to it.
    else if (_metadata_storage->has_default_farm())
    {
      _default_farm = _metadata_storage->get_default_farm_name();

      shcore::Argument_list args;
      args.push_back(shcore::Value(_default_farm));

      ret_val = get_farm(args);
    }
    else
      throw Exception::logic_error("There is no default Farm.");
  }

  return ret_val;
}

/**
* Retrieves a Farm object from the current session through it's name.
* \param name The name of the Farm object to be retrieved.
* \return The Farm object with the given name.
* \sa Farm
*/
#if DOXYGEN_JS
Farm AdminSession::getFarm(String name){}
#elif DOXYGEN_PY
Farm AdminSession::get_farm(str name){}
#endif

shcore::Value AdminSession::get_farm(const shcore::Argument_list &args) const
{
  Value ret_val;
  args.ensure_count(1, get_function_name("getFarm").c_str());

  try
  {
    if (!_session.is_connected())
      throw Exception::metadata_error("Not connected to the Metadata Storage.");

    std::string farm_name = args.string_at(0);

    if (farm_name.empty())
      throw Exception::argument_error("The Farm name cannot be empty.");

    if (!_farms->has_key(farm_name))
      (*_farms)[farm_name] = shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(_metadata_storage->get_farm(farm_name)));

    ret_val = (*_farms)[farm_name];
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getFarm"))

  return ret_val;
}

/**
 * Creates a Farm object.
 * \param name The name of the Farm object to be created
 * \param farmAdminPassword The Farm Administration password
 * \param options Options
 * \return The created Farm object.
 * \sa Farm
 */
#if DOXYGEN_JS
Farm AdminSession::createFarm(String name, String farmAdminPassword, JSON options){}
#elif DOXYGEN_PY
Farm AdminSession::create_farm(str name, str farm_admin_password, JSON options){}
#endif
shcore::Value AdminSession::create_farm(const shcore::Argument_list &args)
{
  Value ret_val;
  args.ensure_count(2, 3, get_function_name("createFarm").c_str());

  // Available options
  std::string farm_admin_type = "local"; // Default is local
  std::string instance_admin_user = "instance_admin"; // Default is instance_admin
  std::string farm_reader_user = "farm_reader"; // Default is farm_reader

  std::string instance_admin_user_password;

  try
  {
    if (!_session.is_connected())
      throw Exception::metadata_error("Not connected to the Metadata Storage.");
    else
    {
      std::string farm_name = args.string_at(0);

      if (farm_name.empty())
        throw Exception::argument_error("The Farm name cannot be empty.");

      std::string farm_password = args.string_at(1);

      // Check if we have a valid password
      if (farm_password.empty())
        throw Exception::argument_error("The Farm password cannot be empty.");

      if (args.size() > 2)
      {
        // Map with the options
        shcore::Value::Map_type_ref options = args.map_at(2);

        // Verify if the options are valid
        std::vector<std::string> valid_options = {"farmAdminType", "instanceAdminUser", "instanceAdminPassword"};

        for (shcore::Value::Map_type::iterator i = options->begin(); i != options->end(); ++i)
        {
          if ((std::find(valid_options.begin(), valid_options.end(), i->first) == valid_options.end()))
            throw shcore::Exception::argument_error("Unexpected argument " + i->first + " on connection data.");
        }

        if (options->has_key("farmAdminType"))
          farm_admin_type = (*options)["farmAdminType"].as_string();

        if (farm_admin_type != "local" &&
            farm_admin_type != "guided" &&
            farm_admin_type != "manual" &&
            farm_admin_type != "ssh")
        {
          throw shcore::Exception::argument_error("Farm Administration Type invalid. Valid types are: 'local', 'guided', 'manual', 'ssh'");
        }

        if (options->has_key("instanceAdminUser"))
        {
          instance_admin_user = (*options)["instanceAdminUser"].as_string();

          if (instance_admin_user.empty())
            throw Exception::argument_error("The instanceAdminUser option cannot be empty.");

          if (!options->has_key("instanceAdminPassword"))
            throw shcore::Exception::argument_error("instanceAdminUser password not provided.");
          else
            instance_admin_user_password = (*options)["instanceAdminPassword"].as_string();
        }
      }

      /*
       * For V1.0 we only support one single Farm. That one shall be the default Farm.
       * We must check if there's already a Default Farm assigned, and if so thrown an exception.
       * And we must check if there's already one Farm on the MD and if so assign it to Default
       */
      bool has_default_farm = _metadata_storage->has_default_farm();

      if ((!_default_farm.empty()) || has_default_farm)
        throw Exception::argument_error("There is already one Farm initialized. Only one Farm is supported.");

      // First we need to create the Metadata Schema, or update it if already exists
      _metadata_storage->create_metadata_schema();

      std::shared_ptr<Farm> farm(new Farm(farm_name, _metadata_storage));

      // Check if we have the instanceAdminUser password or we need to generate it
      if (instance_admin_user_password.empty())
        instance_admin_user_password = generate_password(PASSWORD_LENGHT);

      // Update the properties
      farm->set_admin_type(farm_admin_type);
      farm->set_password(farm_password);
      farm->set_instance_admin_user(instance_admin_user);
      farm->set_instance_admin_user_password(instance_admin_user_password);
      farm->set_farm_reader_user(farm_reader_user);
      farm->set_farm_reader_user_password(generate_password(PASSWORD_LENGHT));

      // For V1.0, let's see the Farm's description to "default"
      farm->set_description("Default Farm");

      // Insert Farm on the Metadata Schema
      _metadata_storage->insert_farm(farm);

      // If it reaches here, it means there are no exceptions
      ret_val = Value(std::static_pointer_cast<Object_bridge>(farm));
      (*_farms)[farm_name] = ret_val;

      // Update the default_farm
      _default_farm = farm_name;
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createFarm"))

  return ret_val;
}

/**
 * Drops a Farm object.
 * \param name The name of the Farm object to be dropped.
 * \return nothing.
 * \sa Farm
 */
#if DOXYGEN_JS
Undefined AdminSession::dropFarm(String name){}
#elif DOXYGEN_PY
None AdminSession::drop_farm(str name){}
#endif

shcore::Value AdminSession::drop_farm(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("dropFarm").c_str());

  try
  {
    if (!_session.is_connected())
      throw Exception::metadata_error("Not connected to the Metadata Storage.");
    else
    {
      std::string farm_name = args.string_at(0);

      if (farm_name.empty())
        throw Exception::argument_error("The Farm name cannot be empty.");

      shcore::Value::Map_type_ref options; // Map with the options
      bool drop_default_rs = false;

      // Check for options
      if (args.size() == 2)
      {
        options = args.map_at(1);

        if (options->has_key("dropDefaultReplicaSet"))
          drop_default_rs = (*options)["dropDefaultReplicaSet"].as_bool();
      }

      if (!drop_default_rs)
      {
        _metadata_storage->drop_farm(farm_name);

        // If it reaches here, it means there are no exceptions
        if (_farms->has_key(farm_name))
          _farms->erase(farm_name);
      }
      else
      {
        // check if the Farm has more replicaSets than the default one
        if (!_metadata_storage->farm_has_default_replicaset_only(farm_name))
          throw Exception::logic_error("Cannot drop Farm: The farm with the name '"
              + farm_name + "' has more replicasets than the default replicaset.");

        // drop the default ReplicaSet and call drop_farm again
        _metadata_storage->drop_default_replicaset(farm_name);
        _metadata_storage->drop_farm(farm_name);

        // If it reaches here, it means there are no exceptions
        if (_farms->has_key(farm_name))
          _farms->erase(farm_name);
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropFarm"))

  return Value();
}

/**
 * Drops the Metadata Schema.
 * \return nothing.
 */
#if DOXYGEN_JS
Undefined AdminSession::dropMetadataSchema(){}
#elif DOXYGEN_PY
None AdminSession::drop_metadata_schema(){}
#endif

shcore::Value AdminSession::drop_metadata_schema(const shcore::Argument_list &args)
{
  args.ensure_count(1, get_function_name("dropMetadataSchema").c_str());

  bool enforce = false;

  // Map with the options
  shcore::Value::Map_type_ref options = args.map_at(0);

  if (options->has_key("enforce"))
        enforce = (*options)["enforce"].as_bool();

  if (enforce)
  {
    try
    {
      if (!_session.is_connected())
        throw Exception::metadata_error("Not connected to the Metadata Storage.");
      else
      {
        _metadata_storage->drop_metadata_schema();

        // If it reaches here, it means there are no exceptions and we can reset the farms cache
        if (_farms->size() > 0)
          _farms.reset(new shcore::Value::Map_type);

        _default_farm = "";
      }
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))
  }

  return Value();
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

  shcore::Value instance_type = get_capability("instance_type");
  if (instance_type)
    (*status)["NODE_TYPE"] = instance_type;

  // TODO: Uncomment or Delete
  (*status)["DEFAULT_FARM"] = shcore::Value(_default_farm);

  return shcore::Value(status);
}

std::shared_ptr<shcore::Object_bridge> AdminSession::create(const shcore::Argument_list &args)
{
  return connect_admin_session(args);
}
