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

#include "mod_mysqlx_replicaset.h"
#include "mod_mysqlx_metadata_storage.h"

#include "common/uuid/include/uuid_gen.h"
#include "utils/utils_general.h"
#include "../mysqlxtest_utils.h"
#include "xerrmsg.h"
#include "mysqlx_connection.h"
#include "shellcore/shell_core_options.h"
#include "common/process_launcher/process_launcher.h"
#include "../mod_mysql_session.h"

#include <sstream>
#include <iostream>
#include <iomanip>

#include <boost/lexical_cast.hpp>

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

ReplicaSet::ReplicaSet(const std::string &name, std::shared_ptr<MetadataStorage> metadata_storage) :
_name(name), _metadata_storage(metadata_storage)
{
  init();
}

ReplicaSet::~ReplicaSet()
{
}

std::string &ReplicaSet::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const
{
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
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
  add_method("addInstance", std::bind(&ReplicaSet::add_instance_, this, _1), "data");
  add_method("removeInstance", std::bind(&ReplicaSet::remove_instance, this, _1), "data");
}

#if DOXYGEN_CPP
/**
 * Use this function to add a Instance to the ReplicaSet object
 * \param args : A list of values to be used to add a Instance to the ReplicaSet.
 *
 * This function returns an empty Value.
 */
#else
/**
* Adds a Instance to the ReplicaSet
* \param conn The Connection String or URI of the Instance to be added
*/
#if DOXYGEN_JS
Undefined ReplicaSet::addInstance(String conn){}
#elif DOXYGEN_PY
None ReplicaSet::add_instance(str conn){}
#endif
/**
* Adds a Instance to the ReplicaSet
* \param doc The Document representing the Instance to be added
*/
#if DOXYGEN_JS
Undefined ReplicaSet::addInstance(Document doc){}
#elif DOXYGEN_PY
None ReplicaSet::add_instance(Document doc){}
#endif
#endif
shcore::Value ReplicaSet::add_instance_(const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  args.ensure_count(1, get_function_name("addInstance").c_str());

  // Check if the ReplicaSet is empty
  if (!_metadata_storage->is_replicaset_empty(get_id()))
    throw shcore::Exception::logic_error("ReplicaSet not initialized. Please add the Seed Instance using: addSeedInstance().");

  // Add the Instance to the Default ReplicaSet
  try
  {
    ret_val = add_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  return ret_val;
}

shcore::Value ReplicaSet::add_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  bool seed_instance = false;
  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string protocol;
  std::string user;
  std::string password;
  std::string host;
  int port = 0;
  std::string sock;
  std::string ssl_ca;
  std::string ssl_cert;
  std::string ssl_key;

  std::vector<std::string> valid_options = { "host", "port", "dbUser", "socket", "ssl_ca", "ssl_cert", "ssl_key", "ssl_key" };

  // NOTE: This function is called from either the add_instance_ on this class
  //       or the add_instance in Farm class, hence this just throws exceptions
  //       and the proper handling is done on the caller functions (to append the called function name)

  // Check if we're on a addSeedInstance or not
  if (_metadata_storage->is_replicaset_empty(_id)) seed_instance = true;

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

  if (options->has_key("dbUser"))
    user = (*options)["dbUser"].as_string();

  if (options->has_key("socket"))
    sock = (*options)["socket"].as_string();

  if (options->has_key("ssl_ca"))
    ssl_ca = (*options)["ssl_ca"].as_string();

  if (options->has_key("ssl_cert"))
    ssl_cert = (*options)["ssl_cert"].as_string();

  if (options->has_key("ssl_key"))
    ssl_key = (*options)["ssl_key"].as_string();

  if (port == 0 && sock.empty())
    port = get_default_port();

  // TODO: validate additional data.

  std::string sock_port = (port == 0) ? sock : boost::lexical_cast<std::string>(port);

  // Handle empty required values
  if (!options->has_key("host"))
    throw shcore::Exception::argument_error("Missing required value for hostname.");

  // Check if the instance was already added
  std::string instance_address = host + ":" + std::to_string(port);

  if(_metadata_storage->is_instance_on_replicaset(get_id(), instance_address))
    throw shcore::Exception::logic_error("The instance '" + instance_address + "'' already belongs to the ReplicaSet: '" + get_member("name").as_string() + "'.");

  std::string instance_admin_user = _metadata_storage->get_instance_admin_user(get_id());
  std::string instance_admin_user_password = _metadata_storage->get_instance_admin_user_password(get_id());
  std::string replication_user_password = _metadata_storage->get_replication_user_password(get_id());

  // check if we have to create the user on the Instance
  // TODO: what if someone already created the instance_admin user?
  if (instance_admin_user == "instance_admin")
  {
    shcore::Argument_list args;
    Value::Map_type_ref options_session(new shcore::Value::Map_type);

    (*options_session)["host"] = shcore::Value(host);
    (*options_session)["port"] = shcore::Value(port);

    if (!user.empty())
      (*options_session)["dbUser"] = shcore::Value(user);
    else
      (*options_session)["dbUser"] = shcore::Value("root");

    (*options_session)["dbPassword"] = shcore::Value("root"); // TODO: create interactive mode to query for the password

    args.push_back(shcore::Value(options_session));

    auto session = mysh::connect_session(args, mysh::Classic);
    mysh::mysql::ClassicSession *classic = dynamic_cast<mysh::mysql::ClassicSession*>(session.get());

    std::string query = "CREATE USER IF NOT EXISTS 'instance_admin'@'" + host + "' IDENTIFIED BY '" + instance_admin_user_password + "'";

    args.clear();
    args.push_back(shcore::Value("SET sql_log_bin = 0"));
    classic->run_sql(args);

    args.clear();
    args.push_back(shcore::Value(query));

    classic->run_sql(args);

    query = "GRANT PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE ON *.* TO 'instance_admin'@'" + host + "'";

    args.clear();
    args.push_back(shcore::Value(query));

    classic->run_sql(args);

    query = "CREATE USER IF NOT EXISTS 'replication_user'@'%' IDENTIFIED BY '" + replication_user_password + "'";

    args.clear();
    args.push_back(shcore::Value(query));

    classic->run_sql(args);

    query = "GRANT REPLICATION SLAVE ON *.* to 'replication_user'@'%'";

    args.clear();
    args.push_back(shcore::Value(query));

    classic->run_sql(args);

    args.clear();
    args.push_back(shcore::Value("SET sql_log_bin = 1"));
    classic->run_sql(args);
  }

  // Gadgets handling
  std::string buf;
  char c;
  std::string success("Operation completed with success.");
  std::string super_user_password = "root\n"; // TODO: interactive wrapper to query it
  std::string replication_password = replication_user_password + "\n";
  std::string error;

  std::string gadgets_path = (*shcore::Shell_core_options::get())[SHCORE_GADGETS_PATH].as_string();

#ifdef WIN32
  success += "\r\n";
#else
  success += "\n";
#endif

  // Call the gadget to bootstrap the group with this instance
  if (seed_instance)
  {
    // Call mysqlprovision to bootstrap the group using "start"
    std::string instance_cmd = "--instance=" + user + "@" + host + ":" + std::to_string(port);
    std::string replication_user = "--replication-user=replication_user";
    const char *args_script[] = { "python", gadgets_path.c_str(), "start-replicaset", instance_cmd.c_str(), replication_user.c_str(), "--stdin", NULL };

    ngcommon::Process_launcher p("python", args_script);

    try
    {
      p.write(super_user_password.c_str(), super_user_password.length());
    }
    catch (shcore::Exception &e)
    {
      throw shcore::Exception::runtime_error(e.what());
    }

    try
    {
      p.write(replication_password.c_str(), replication_password.length());
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
          std::string s_out = "The instance '" + host + ":" + std::to_string(port) + "' was successfully added as seeding instance to the MySQL Farm.";
          ret_val = shcore::Value(s_out);
          break;
        }
        buf = "";
      }
    }

    if (read_error)
      throw shcore::Exception::logic_error(error);

    p.wait();

    // OK, if we reached here without errors we can update the metadata with the host
    _metadata_storage->insert_host(args);

    // And the instance
    uint64_t host_id = _metadata_storage->get_host_id(host);

    shcore::Argument_list args_instance;
    Value::Map_type_ref options_instance(new shcore::Value::Map_type);
    args_instance.push_back(shcore::Value(options_instance));

    (*options_instance)["role"] = shcore::Value("master");
    (*options_instance)["mode"] = shcore::Value("rw");

    // TODO: construct properly the addresses
    std::string address = host + ":" + std::to_string(port);
    (*options_instance)["addresses"] = shcore::Value(address);

    _metadata_storage->insert_instance(args_instance, host_id, get_id());
  }

  else
  {
    // We need to retrieve a peer instance, so let's use the Seed one
    std::string peer_instance = _metadata_storage->get_seed_instance(get_id());

    // Call mysqlprovision to join the instance on the group using "join"
    std::string instance_cmd = "--instance=" + user + "@" + host + ":" + std::to_string(port);
    std::string replication_user = "--replication-user=replication_user";
    std::string peer_cmd = "--peer-instance=" + user + "@" + peer_instance;
    const char *args_script[] = { "python", gadgets_path.c_str(), "join-replicaset", instance_cmd.c_str(), replication_user.c_str(), peer_cmd.c_str(), "--stdin", NULL };

    ngcommon::Process_launcher p("python", args_script);

    try
    {
      p.write(super_user_password.c_str(), super_user_password.length());
    }
    catch (shcore::Exception &e)
    {
      throw shcore::Exception::runtime_error(e.what());
    }

    try
    {
      p.write(replication_password.c_str(), replication_password.length());
    }
    catch (shcore::Exception &e)
    {
      throw shcore::Exception::runtime_error(e.what());
    }

    try
    {
      p.write(super_user_password.c_str(), super_user_password.length());
    }
    catch (shcore::Exception &e)
    {
      throw shcore::Exception::runtime_error(e.what());
    }

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
          std::string s_out = "The instance '" + host + ":" + std::to_string(port) + "' was successfully added to the MySQL Farm.";
          ret_val = shcore::Value(s_out);
          break;
        }
        buf = "";
      }
    }

    if (read_error)
      throw shcore::Exception::logic_error(error);

    // wait for termination
    p.wait();

    // OK, if we reached here without errors we can update the metadata with the host
    _metadata_storage->insert_host(args);

    // And the instance
    uint64_t host_id = _metadata_storage->get_host_id(host);

    shcore::Argument_list args_instance;
    Value::Map_type_ref options_instance(new shcore::Value::Map_type);
    args_instance.push_back(shcore::Value(options_instance));

    (*options_instance)["role"] = shcore::Value("hotSpare");
    (*options_instance)["mode"] = shcore::Value("ro");

    // TODO: construct properly the addresses
    std::string address = host + ":" + std::to_string(port);
    (*options_instance)["addresses"] = shcore::Value(address);

    _metadata_storage->insert_instance(args_instance, host_id, get_id());
  }

  return ret_val;
}

#if DOXYGEN_CPP
/**
 * Use this function to remove a Instance from the ReplicaSet object
 * \param args : A list of values to be used to remove a Instance to the Farm.
 *
 * This function returns an empty Value.
 */
#else
/**
* Removes a Instance from the ReplicaSet
* \param name The name of the Instance to be removed
*/
#if DOXYGEN_JS
Undefined ReplicaSet::removeInstance(String name){}
#elif DOXYGEN_PY
None ReplicaSet::remove_instance(str name){}
#endif

/**
* Removes a Instance from the ReplicaSet
* \param doc The Document representing the Instance to be removed
*/
#if DOXYGEN_JS
Undefined ReplicaSet::removeInstance(Document doc){}
#elif DOXYGEN_PY
None ReplicaSet::remove_instance(Document doc){}
#endif
#endif

shcore::Value ReplicaSet::remove_instance(const shcore::Argument_list &args)
{
  args.ensure_count(1, get_function_name("removeInstance").c_str());

  // TODO!

  return Value();
}
