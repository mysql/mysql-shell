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
#include "mod_dba.h"
#include "utils/utils_general.h"
#include "shellcore/object_factory.h"
#include "shellcore/shell_core_options.h"
#include "../mysqlxtest_utils.h"
#include <random>

#include "logger/logger.h"

#include "mod_dba_farm.h"
#include "mod_dba_metadata_storage.h"
#include <boost/lexical_cast.hpp>

#include "common/process_launcher/process_launcher.h"

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

#define PASSWORD_LENGHT 16

std::set<std::string> Dba::_deploy_instance_opts = { "port", "portx", "sandboxDir", "password", "dbPassword" };

Dba::Dba(IShell_core* owner) :
_shell_core(owner)
{
  init();
}

bool Dba::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}

void Dba::init()
{
  // In case we are going to keep a cache of Farms
  // If not, _farms can be removed
  _farms.reset(new shcore::Value::Map_type);

  add_property("defaultFarm", "getDefaultFarm");

  // Pure functions
  add_method("resetSession", std::bind(&Dba::reset_session, this, _1), "session", shcore::Object, NULL);
  add_method("createFarm", std::bind(&Dba::create_farm, this, _1), "farmName", shcore::String, NULL);
  add_method("dropFarm", std::bind(&Dba::drop_farm, this, _1), "farmName", shcore::String, NULL);
  add_method("getFarm", std::bind(&Dba::get_farm, this, _1), "farmName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("validateInstance", std::bind(&Dba::validate_instance, this, _1), "data", shcore::Map, NULL);
  add_method("deployLocalInstance", std::bind(&Dba::deploy_local_instance, this, _1), "data", shcore::Map, NULL);

  _metadata_storage.reset(new MetadataStorage(this));

  std::string python_path = "";
  std::string gadgets_path;

  if (getenv("MYSQLPROVISION") != NULL)
    gadgets_path = std::string(getenv("MYSQLPROVISION")); // should be set to the mysqlprovision root dir

  if (!gadgets_path.empty())
  {
    gadgets_path += "/gadgets/python";

    std::string sep;
#ifdef WIN32
    sep = ";";
#else
    sep = ":";
#endif

    if (getenv("PYTHONPATH") != NULL)
      python_path = std::string(getenv("PYTHONPATH") + sep + gadgets_path);
    else
      python_path = gadgets_path;

#ifdef WIN32
    _putenv_s("PYTHONPATH", python_path.c_str());
#else
    setenv("PYTHONPATH", python_path.c_str(), true);
#endif
  }
}

std::string Dba::generate_password(int password_lenght)
{
  std::random_device rd;
  std::string pwd;
  const char *alphabet = "1234567890abcdefghijklmnopqrstuvwxyz";
  std::uniform_int_distribution<int> dist(0, strlen(alphabet) - 1);

  for (int i = 0; i < password_lenght; i++)
    pwd += alphabet[dist(rd)];

  return pwd;
}

std::shared_ptr<ShellDevelopmentSession> Dba::get_active_session()
{
  std::shared_ptr<ShellDevelopmentSession> ret_val;
  if (_custom_session)
    ret_val = _custom_session;
  else
    ret_val = _shell_core->get_dev_session();

  if (!ret_val)
    throw shcore::Exception::logic_error("The Metadata is inaccessible, an active session is required");

  return ret_val;
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
String Dba::getUri(){}
#elif DOXYGEN_PY
str Dba::get_uri(){}
#endif
/**
* Retrieves the Farm configured as default on this Metadata instance.
* \return A Farm object or Null
*/
#if DOXYGEN_JS
Farm Dba::getDefaultFarm(){}
#elif DOXYGEN_PY
Farm Dba::get_default_farm(){}
#endif
#endif

Value Dba::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is a schema and attempt loading it as such
  if (prop == "defaultFarm")
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
  else if (Cpp_object_bridge::has_member(prop))
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

/**
* Retrieves a Farm object from the current session through it's name.
* \param name The name of the Farm object to be retrieved.
* \return The Farm object with the given name.
* \sa Farm
*/
#if DOXYGEN_JS
Farm Dba::getFarm(String name){}
#elif DOXYGEN_PY
Farm Dba::get_farm(str name){}
#endif

shcore::Value Dba::get_farm(const shcore::Argument_list &args) const
{
  Value ret_val;
  args.ensure_count(1, get_function_name("getFarm").c_str());

  try
  {
    std::string farm_name = args.string_at(0);

    if (farm_name.empty())
      throw Exception::argument_error("The Farm name cannot be empty.");

    //if (!_session.is_connected())
    //  throw Exception::metadata_error("Not connected to the Metadata Storage.");

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
Farm Dba::createFarm(String name, String farmAdminPassword, JSON options){}
#elif DOXYGEN_PY
Farm Dba::create_farm(str name, str farm_admin_password, JSON options){}
#endif
shcore::Value Dba::create_farm(const shcore::Argument_list &args)
{
  Value ret_val;
  args.ensure_count(2, 3, get_function_name("createFarm").c_str());

  // Available options
  std::string farm_admin_type = "local"; // Default is local
  std::string instance_admin_user = "instance_admin"; // Default is instance_admin
  std::string farm_reader_user = "farm_reader"; // Default is farm_reader
  std::string replication_user = "replication_user"; // Default is replication_user

  std::string instance_admin_user_password;

  try
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
      std::vector<std::string> valid_options = { "farmAdminType", "instanceAdminUser", "instanceAdminPassword" };

      for (shcore::Value::Map_type::iterator i = options->begin(); i != options->end(); ++i)
      {
        if ((std::find(valid_options.begin(), valid_options.end(), i->first) == valid_options.end()))
          throw shcore::Exception::argument_error("Unexpected argument " + i->first + " on connection data.");
      }

      if (options->has_key("farmAdminType"))
        farm_admin_type = (*options)["farmAdminType"].as_string();

      std::cout << "farm admin type: " << farm_admin_type << "\n";

      if (farm_admin_type != "local" ||
          farm_admin_type != "guided" ||
          farm_admin_type != "manual" ||
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
    farm->set_replication_user(replication_user);
    farm->set_replication_user_password(generate_password(PASSWORD_LENGHT));

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
Undefined Dba::dropFarm(String name){}
#elif DOXYGEN_PY
None Dba::drop_farm(str name){}
#endif

shcore::Value Dba::drop_farm(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("dropFarm").c_str());

  try
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
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropFarm"))

  return Value();
}

/**
 * Drops the Metadata Schema.
 * \return nothing.
 */
#if DOXYGEN_JS
Undefined Dba::dropMetadataSchema(){}
#elif DOXYGEN_PY
None Dba::drop_metadata_schema(){}
#endif

shcore::Value Dba::drop_metadata_schema(const shcore::Argument_list &args)
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
      _metadata_storage->drop_metadata_schema();

      // If it reaches here, it means there are no exceptions and we can reset the farms cache
      if (_farms->size() > 0)
        _farms.reset(new shcore::Value::Map_type);

      _default_farm = "";
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))
  }

  return Value();
}

shcore::Value Dba::reset_session(const shcore::Argument_list &args)
{
  args.ensure_count(0, 1, get_function_name("resetSession").c_str());

  try
  {
    if (args.size())
    {
      // TODO: Review the case when using a Global_session
      _custom_session = args[0].as_object<ShellDevelopmentSession>();

      if (!_custom_session)
        throw shcore::Exception::argument_error("Invalid session object.");
    }
    else
      _custom_session.reset();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("resetSession"))

  return Value();
}

shcore::Value Dba::validate_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, "validateInstance");

  shcore::Value ret_val;

  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string protocol;
  std::string user;
  std::string host;
  int port = 0;
  std::string sock;
  std::string schema;
  std::string ssl_ca;
  std::string ssl_cert;
  std::string ssl_key;

  std::vector<std::string> valid_options = { "host", "port", "user", "dbUser", "password", "dbPassword", "socket", "ssl_ca", "ssl_cert", "ssl_key", "ssl_key" };

  try
  {
    // Identify the type of connection data (String or Document)
    if (args[0].type == shcore::String)
    {
      uri = args.string_at(0);
      options = shcore::get_connection_data(uri, false);
    }

    // Connection data comes in a dictionary
    else if (args[0].type == shcore::Map)
      options = args.map_at(0);

    else
      throw shcore::Exception::argument_error("Unexpected argument on connection data.");

    if (options->size() == 0)
      throw shcore::Exception::argument_error("Connection data empty.");

    for (shcore::Value::Map_type::iterator i = options->begin(); i != options->end(); ++i)
    {
      if ((std::find(valid_options.begin(), valid_options.end(), i->first) == valid_options.end()))
        throw shcore::Exception::argument_error("Unexpected argument '" + i->first + "' on connection data.");
    }

    if (options->has_key("host"))
      host = (*options)["host"].as_string();

    if (options->has_key("port"))
      port = (*options)["port"].as_int();

    // Sets a default user if not specified
    if (options->has_key("user"))
      user = options->get_string("user");
    else if (options->has_key("dbUser"))
      user = options->get_string("dbUser");

    if (options->has_key("password"))
      user = options->get_string("password");
    else if (options->has_key("dbPassword"))
      user = options->get_string("dbPassword");

    if (options->has_key("socket"))
      sock = (*options)["socket"].as_string();

    if (options->has_key("ssl_ca"))
      ssl_ca = (*options)["ssl_ca"].as_string();

    if (options->has_key("ssl_cert"))
      ssl_cert = (*options)["ssl_cert"].as_string();

    if (options->has_key("ssl_key"))
      ssl_key = (*options)["ssl_key"].as_string();

    if (port == 0 && sock.empty())
      port = get_default_instance_port();

    // TODO: validate additional data.

    std::string sock_port = (port == 0) ? sock : boost::lexical_cast<std::string>(port);

    // Handle empty required values
    if (!options->has_key("host"))
      throw shcore::Exception::argument_error("Missing required value for hostname.");

    std::string gadgets_path = (*shcore::Shell_core_options::get())[SHCORE_GADGETS_PATH].as_string();

    if (gadgets_path.empty())
      throw shcore::Exception::logic_error("Please set the mysqlprovision path using the environment variable: MYSQLPROVISION.");

    std::string instance_cmd = "--instance=" + user + "@" + host + ":" + std::to_string(port);
    const char *args_script[] = { "python", gadgets_path.c_str(), "check", instance_cmd.c_str(), "--stdin", NULL };

    ngcommon::Process_launcher p("python", args_script);

    std::string buf;
    char c;
    std::string success("Operation completed with success.");
    std::string password = "root\n"; // TODO: interactive wrapper to query it
    std::string error;

#ifdef WIN32
    success += "\r\n";
#else
    success += "\n";
#endif

    try
    {
      p.write(password.c_str(), password.length());
    }
    catch (shcore::Exception &e)
    {
      throw shcore::Exception::runtime_error(e.what());
    }

    bool read_error = false;

    while (p.read(&c, 1) > 0)
    {
      buf += c;
      if (c == '\n')
      {
        if ((buf.find("ERROR") != std::string::npos))
          read_error = true;

        if (read_error)
          error += buf;

        if (strcmp(success.c_str(), buf.c_str()) == 0)
        {
          std::string s_out = "The instance: " + host + ":" + std::to_string(port) + " is valid for Farm usage";
          ret_val = shcore::Value(s_out);
          break;
        }
        buf = "";
      }
    }

    if (read_error)
    {
      // Remove unnecessary gadgets output
      std::string remove_me = "ERROR: Error executing the 'check' command:";

      std::string::size_type i = error.find(remove_me);

      if (i != std::string::npos)
        error.erase(i, remove_me.length());

      throw shcore::Exception::logic_error(error);
    }

    p.wait();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION("validateInstance");

  return ret_val;
}

shcore::Value Dba::deploy_local_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, "deployLocalInstance");

  shcore::Value ret_val;

  shcore::Value::Map_type_ref options; // Map with the connection data

  int port = 0;
  int portx = 0;
  std::string password;
  std::string sandbox_dir;

  try
  {
    options = args.map_at(0);

    // Verification of invalid attributes on the instance deployment data
    auto invalids = shcore::get_additional_keys(options, _deploy_instance_opts);
    if (invalids.size())
    {
      std::string error = "The instance data contains the next invalid attributes: ";
      error += shcore::join_strings(invalids, ", ");
      throw shcore::Exception::argument_error(error);
    }

    // Verification of required attributes on the instance deployment data
    auto missing = shcore::get_missing_keys(options, { "password|dbPassword" });

    if (missing.size())
    {
      if (args.size() == 2)
        password = args.string_at(1);
      else
        throw shcore::Exception::argument_error("Missing root password for the deployed instance");
    }
    else
    {
      if (options->has_key("password"))
        password = options->get_string("password");
      else if (options->has_key("dbPassword"))
        password = options->get_string("dbPassword");
    }

    password += "\n";

    if (options->has_key("port"))
      port = (*options)["port"].as_int();

    if (options->has_key("portx"))
      portx = (*options)["portx"].as_int();

    if (options->has_key("sandboxDir"))
      sandbox_dir = (*options)["sandboxDir"].as_string();

    std::string gadgets_path = (*shcore::Shell_core_options::get())[SHCORE_GADGETS_PATH].as_string();

    if (gadgets_path.empty())
      throw shcore::Exception::logic_error("Please set the mysqlprovision path using the environment variable: MYSQLPROVISION.");

    std::vector<std::string> sandbox_args;
    std::string arg;

    if (port != 0)
    {
      arg = "--port=" + std::to_string(port);
      sandbox_args.push_back(arg);
    }

    if (portx != 0)
    {
      arg = "--mysqlx-port=" + std::to_string(portx);
      sandbox_args.push_back(arg);
    }

    if (!sandbox_dir.empty())
    {
      arg = "--sandboxdir=" + sandbox_dir;
      sandbox_args.push_back(arg);
    }

    char **args_script = new char*[10];
    args_script[0] = const_cast<char*>("python");
    args_script[1] = const_cast<char*>(gadgets_path.c_str());
    args_script[2] = const_cast<char*>("sandbox");
    args_script[3] = const_cast<char*>("start");

    int i;

    for (i = 0; i < sandbox_args.size(); i++)
      args_script[i + 4] = const_cast<char*>(sandbox_args[i].c_str());

    args_script[i++ + 4] = const_cast<char*>("--stdin");
    args_script[i++ + 4] = NULL;

    ngcommon::Process_launcher p("python", const_cast<const char**>(args_script));

    std::string buf, error, answer;
    char c;
    std::string success("Operation completed with success.");

#ifdef WIN32
    success += "\r\n";
#else
    success += "\n";
#endif

    try
    {
      p.write(password.c_str(), password.length());
    }
    catch (shcore::Exception &e)
    {
      throw shcore::Exception::runtime_error(e.what());
    }

    bool read_error = false;

    while (p.read(&c, 1) > 0)
    {
      buf += c;
      if (c == '\n')
      {
        if ((buf.find("ERROR") != std::string::npos))
          read_error = true;

        if (read_error)
          error += buf;

        if (strcmp(success.c_str(), buf.c_str()) == 0)
        {
          std::string s_out = "Instance: localhost:" + std::to_string(port) + " successfully deployed.";
          ret_val = shcore::Value(s_out);
          break;
        }
        buf = "";
      }
    }

    if (read_error)
    {
      // Remove unnecessary gadgets output
      std::string remove_me = "ERROR: Error executing the 'sandbox' command:";

      std::string::size_type i = error.find(remove_me);

      if (i != std::string::npos)
        error.erase(i, remove_me.length());

      throw shcore::Exception::logic_error(error);
    }

    p.wait();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION("deployLocalInstance");

  return ret_val;
}