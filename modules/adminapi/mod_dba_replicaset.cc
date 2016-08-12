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

#include "mod_dba_replicaset.h"
#include "mod_dba_metadata_storage.h"

#include "common/uuid/include/uuid_gen.h"
#include "utils/utils_general.h"
#include "../mysqlxtest_utils.h"
#include "xerrmsg.h"
#include "mysqlx_connection.h"
#include "shellcore/shell_core_options.h"
#include "common/process_launcher/process_launcher.h"
#include "../mod_mysql_session.h"
#include "../mod_mysql_resultset.h"

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

void ReplicaSet::append_json(shcore::JSON_dumper& dumper) const
{
  std::shared_ptr<mysh::ShellBaseResult>result1 = _metadata_storage->execute_sql("show status like 'group_replication_primary_member'");

  auto row = result1->call("fetchOne", shcore::Argument_list());

  std::string master_uuid;
  if (row)
    master_uuid = row.as_object<Row>()->get_member(1).as_string();

  std::string query = "select mysql_server_uuid, instance_name, role, MEMBER_STATE, JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) as host from farm_metadata_schema.instances left join performance_schema.replication_group_members on `mysql_server_uuid`=`MEMBER_ID` where replicaset_id = " + std::to_string(_id);
  auto result2 = _metadata_storage->execute_sql(query);

  auto raw_instances = result2->call("fetchAll", shcore::Argument_list());

  // First we identify the master instance
  auto instances = raw_instances.as_array();

  std::shared_ptr<mysh::Row> master;
  for (auto value : *instances.get())
  {
    auto row = value.as_object<mysh::Row>();
    if (row->get_member(0).as_string() == master_uuid)
    {
      master = row;
      break;
    }
  }

  dumper.start_object();
  dumper.append_string("status", "<Some Status>");

  // Creates the instances element
  dumper.append_string("instances");
  dumper.start_array();

  dumper.start_object();
  dumper.append_string("name", master->get_member(1).as_string());
  dumper.append_string("host", master->get_member(4).as_string());
  dumper.append_string("role", master->get_member(2).as_string());
  dumper.append_string("mode", "R/W");
  dumper.append_string("status", master->get_member(3).as_string() == "ONLINE" ? "ONLINE" : "OFFLINE");
  dumper.end_object();

  for (auto value : *instances.get())
  {
    auto row = value.as_object<mysh::Row>();
    if (row != master)
    {
      dumper.start_object();
      dumper.append_string("name", row->get_member(1).as_string());
      dumper.append_string("host", row->get_member(4).as_string());
      dumper.append_string("role", row->get_member(2).as_string());
      dumper.append_string("mode", "R/O");
      dumper.append_string("status", row->get_member(3).as_string() == "ONLINE" ? "ONLINE" : "OFFLINE");
      dumper.end_object();
    }
  }
  dumper.end_array();

  dumper.append_string("topology");

  dumper.start_object();
  dumper.append_string("name", master->get_member(1).as_string());
  dumper.append_string("role", master->get_member(2).as_string());
  dumper.append_string("mode", "R/W");

  dumper.append_string("leaves");
  dumper.start_array();
  for (auto value : *instances.get())
  {
    auto row = value.as_object<mysh::Row>();
    if (row != master)
    {
      dumper.start_object();
      dumper.append_string("name", row->get_member(1).as_string());
      dumper.append_string("role", row->get_member(2).as_string());
      dumper.append_string("mode", "R/O");
      dumper.append_string("leaves");
      dumper.start_array();
      dumper.end_array();
      dumper.end_object();
    }
  }

  dumper.end_array();
  dumper.end_object();

  dumper.end_object();
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
  args.ensure_count(2, 3, get_function_name("addInstance").c_str());

  // Check if the ReplicaSet is empty
  if (_metadata_storage->is_replicaset_empty(get_id()))
    throw shcore::Exception::logic_error("ReplicaSet not initialized. Please add the Seed Instance using: addSeedInstance().");

  // Add the Instance to the Default ReplicaSet
  try
  {
    ret_val = add_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  return ret_val;
}

std::set<std::string> ReplicaSet::_valid_attributes = { "name", "host", "port", "user", "dbUser", "password", "dbPassword", "socket", "ssl_ca", "ssl_cert", "ssl_key", "ssl_key" };

std::set<std::string> ReplicaSet::get_invalid_attributes(const std::set<std::string> input)
{
  std::set<std::string> diff;

  std::set_difference(input.begin(), input.end(), _valid_attributes.begin(), _valid_attributes.end(), std::inserter(diff, diff.begin()));

  return diff;
}

shcore::Value ReplicaSet::add_instance(const shcore::Argument_list &args)
{
  shcore::Value ret_val;

  bool seed_instance = false;
  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string user;
  std::string host;
  int port = 0;

  // NOTE: This function is called from either the add_instance_ on this class
  //       or the add_instance in Farm class, hence this just throws exceptions
  //       and the proper handling is done on the caller functions (to append the called function name)

  // Check if we're on a addSeedInstance or not
  if (_metadata_storage->is_replicaset_empty(_id)) seed_instance = true;

  std::string farm_admin_password = args.string_at(0);

  // Check if we have a valid password
  if (farm_admin_password.empty())
    throw Exception::argument_error("The MASTER Farm password cannot be empty.");

  // Identify the type of connection data (String or Document)
  if (args[1].type == String)
  {
    uri = args.string_at(1);
    options = get_connection_data(uri, false);
  }

  // Connection data comes in a dictionary
  else if (args[1].type == Map)
    options = args.map_at(1);

  else
    throw shcore::Exception::argument_error("Invalid connection options, expected either a URI or a Dictionary.");

  // Gets the list of incoming attributes
  std::set<std::string> attributes;
  for (auto option : *options)
    attributes.insert(option.first);

  auto invalids = mysh::mysqlx::ReplicaSet::get_invalid_attributes(attributes);

  // Verification of invalid attributes on the connection data
  if (invalids.size())
  {
    std::string error = "The connection data contains the next invalid attributes: ";
    error += *invalids.begin();

    invalids.erase(invalids.begin());

    for (auto attribute : invalids)
      error += ", " + attribute;

    throw shcore::Exception::argument_error(error);
  }

  if (options->has_key("port"))
    port = options->get_int("port");
  else
    port = get_default_port();

  // Sets a default user if not specified
  if (options->has_key("user"))
    user = options->get_string("user");
  else if (options->has_key("dbUser"))
      user = options->get_string("dbUser");
  else
  {
    user = "root";
    (*options)["dbUser"] = shcore::Value(user);
  }

  // Handle empty required values
  if (!options->has_key("host"))
    throw shcore::Exception::argument_error("Missing required attribute: host.");
  else
    host = options->get_string("host");

  std::string password;
  if (!options->has_key("password") && !options->has_key("dbPassword"))
  {
    if (args.size() == 3)
      (*options)["dbPassword"] = shcore::Value(args.string_at(2));
    else
      throw shcore::Exception::argument_error("Missing password for " + build_connection_string(options, false));
  }

  // Check if the instance was already added
  std::string instance_address = options->get_string("host") + ":" + std::to_string(options->get_int("port"));

  if (_metadata_storage->is_instance_on_replicaset(get_id(), instance_address))
    throw shcore::Exception::logic_error("The instance '" + instance_address + "'' already belongs to the ReplicaSet: '" + get_member("name").as_string() + "'.");

  std::string instance_admin_user = _metadata_storage->get_instance_admin_user(get_id());
  std::string instance_admin_user_password = _metadata_storage->get_instance_admin_user_password(get_id());
  std::string replication_user_password = _metadata_storage->get_replication_user_password(get_id());

  // Drop and create the instanceAdmin user
  shcore::Argument_list temp_args;

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(options));
  auto session = mysh::connect_session(new_args, mysh::Classic);
  mysh::mysql::ClassicSession *classic = dynamic_cast<mysh::mysql::ClassicSession*>(session.get());

  temp_args.clear();
  temp_args.push_back(shcore::Value("SET sql_log_bin = 0"));
  classic->run_sql(temp_args);

  std::string query = "DROP USER IF EXISTS 'instance_admin'@'" + host + "'";

  temp_args.clear();
  temp_args.push_back(shcore::Value(query));

  classic->run_sql(temp_args);

  query = "CREATE USER 'instance_admin'@'" + host + "' IDENTIFIED BY '" + instance_admin_user_password + "'";

  temp_args.clear();
  temp_args.push_back(shcore::Value(query));

  classic->run_sql(temp_args);

  query = "GRANT PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE ON *.* TO 'instance_admin'@'" + host + "'";

  temp_args.clear();
  temp_args.push_back(shcore::Value(query));

  classic->run_sql(temp_args);

  query = "DROP USER IF EXISTS 'replication_user'@'" + host + "'";

  temp_args.clear();
  temp_args.push_back(shcore::Value(query));

  query = "CREATE USER IF NOT EXISTS 'replication_user'@'%' IDENTIFIED BY '" + replication_user_password + "'";

  temp_args.clear();
  temp_args.push_back(shcore::Value(query));

  classic->run_sql(temp_args);

  query = "GRANT REPLICATION SLAVE ON *.* to 'replication_user'@'%'";

  temp_args.clear();
  temp_args.push_back(shcore::Value(query));

  classic->run_sql(temp_args);

  temp_args.clear();
  temp_args.push_back(shcore::Value("SET sql_log_bin = 1"));
  classic->run_sql(temp_args);

  temp_args.clear();
  temp_args.push_back(shcore::Value("SELECT @@server_uuid"));
  auto uuid_raw_result = classic->run_sql(temp_args);
  auto uuid_result = uuid_raw_result.as_object<mysh::mysql::ClassicResult>();

  auto uuid_row = uuid_result->fetch_one(shcore::Argument_list());

  std::string mysql_server_uuid;
  if (uuid_row)
    mysql_server_uuid = uuid_row.as_object<mysh::Row>()->get_member(0).as_string();

  // Gadgets handling
  std::string buf;
  char c;
  std::string success("Operation completed with success.");
  std::string super_user_password = "root\n"; // TODO: interactive wrapper to query it
  std::string replication_password = replication_user_password + "\n";
  std::string error;

  std::string gadgets_path = (*shcore::Shell_core_options::get())[SHCORE_GADGETS_PATH].as_string();

  if (gadgets_path.empty())
      throw shcore::Exception::logic_error("Please set the mysqlprovision path using the environment variable: MYSQLPROVISION.");

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
    {
      // Remove unnecessary gadgets output
      std::string remove_me = "ERROR: Error executing the 'start-replicaset' command:";

      std::string::size_type i = error.find(remove_me);

      if (i != std::string::npos)
        error.erase(i, remove_me.length());

      throw shcore::Exception::logic_error(error);
    }

    p.wait();

    // OK, if we reached here without errors we can update the metadata with the host
    auto result = _metadata_storage->insert_host(args);

    // And the instance
    uint64_t host_id = result->get_member("autoIncrementValue").as_int();

    shcore::Argument_list args_instance;
    Value::Map_type_ref options_instance(new shcore::Value::Map_type);
    args_instance.push_back(shcore::Value(options_instance));

    (*options_instance)["role"] = shcore::Value("HA");

    // TODO: construct properly the addresses
    std::string address = host + ":" + std::to_string(port);
    shcore::Value val_address = shcore::Value(address);
    (*options_instance)["addresses"] = val_address;
    (*options_instance)["mysql_server_uuid"] = shcore::Value(mysql_server_uuid);

    if (options->has_key("name"))
      (*options_instance)["instance_name"] = (*options)["name"];
    else
      (*options_instance)["instance_name"] = val_address;

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
    {
      // Remove unnecessary gadgets output
      std::string remove_me = "ERROR: Error executing the 'join-replicaset' command:";

      std::string::size_type i = error.find(remove_me);

      if (i != std::string::npos)
        error.erase(i, remove_me.length());

      throw shcore::Exception::logic_error(error);
    }

    // wait for termination
    p.wait();

    // OK, if we reached here without errors we can update the metadata with the host
    auto result = _metadata_storage->insert_host(args);

    // And the instance
    uint64_t host_id = result->get_member("autoIncrementValue").as_int();

    shcore::Argument_list args_instance;
    Value::Map_type_ref options_instance(new shcore::Value::Map_type);
    args_instance.push_back(shcore::Value(options_instance));

    (*options_instance)["role"] = shcore::Value("HA");

    std::string address = host + ":" + std::to_string(port);
    shcore::Value val_address = shcore::Value(address);
    (*options_instance)["addresses"] = val_address;

    (*options_instance)["mysql_server_uuid"] = shcore::Value(mysql_server_uuid);

    if (options->has_key("name"))
      (*options_instance)["instance_name"] = (*options)["name"];
    else
      (*options_instance)["instance_name"] = val_address;

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