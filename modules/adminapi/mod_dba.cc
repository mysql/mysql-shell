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

#include "mod_dba_cluster.h"
#include "mod_dba_metadata_storage.h"
#include <boost/lexical_cast.hpp>

#include "common/process_launcher/process_launcher.h"

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

#define PASSWORD_LENGHT 16

std::set<std::string> Dba::_deploy_instance_opts = { "portx", "sandboxDir", "password", "dbPassword" };

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
  // In case we are going to keep a cache of Clusters
  // If not, _clusters can be removed
  _clusters.reset(new shcore::Value::Map_type);

  // Pure functions
  add_method("resetSession", std::bind(&Dba::reset_session, this, _1), "session", shcore::Object, NULL);
  add_method("createCluster", std::bind(&Dba::create_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("dropCluster", std::bind(&Dba::drop_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("getCluster", std::bind(&Dba::get_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("validateInstance", std::bind(&Dba::validate_instance, this, _1), "data", shcore::Map, NULL);
  add_method("deployLocalInstance", std::bind(&Dba::deploy_local_instance, this, _1), "data", shcore::Map, NULL);
  add_varargs_method("help", std::bind(&Dba::help, this, _1));

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
* Retrieves a Cluster object from the Metadata Schema.
* \ param name Optional parameter to specify the name of the Cluster to be returned.
*
* If the name is provided the Cluster with the given name will be returned. If a Cluster with the given name does not exist
* an error will be raised.
*
* If no name is provided, the Cluster configured as "default" will be returned.
* \return The Cluster configured as default.
*/
#else
/**
* Retrieves the default Cluster from the Metadata Schema.
* \return The Cluster configured as default.
*/
#if DOXYGEN_JS
Cluster Dba::getCluster(){}
#elif DOXYGEN_PY
Cluster Dba::get_cluster(){}
#endif
/**
* Retrieves a Cluster object from the Metadata schema.
* \param name The name of the Cluster object to be retrieved.
* \return The Cluster object with the given name.
*
* If a Cluster with the given name does not exist an error will be raised.
*/
#if DOXYGEN_JS
Cluster Dba::getCluster(String name){}
#elif DOXYGEN_PY
Cluster Dba::get_cluster(str name){}
#endif
#endif
shcore::Value Dba::get_cluster(const shcore::Argument_list &args) const
{
  Value ret_val;
  args.ensure_count(0, 2, get_function_name("getCluster").c_str());

  std::shared_ptr<mysh::mysqlx::Cluster> cluster;
  bool get_default_cluster = false;
  std::string cluster_name;
  std::string master_key;
  shcore::Value::Map_type_ref options;

  try
  {
    // gets the cluster_name and/or options
    if (args.size())
    {
      if (args.size() == 1)
      {
        if (args[0].type == shcore::String)
          cluster_name = args[0].as_string();
        else if (args[0].type == shcore::Map)
        {
          options = args[0].as_map();
          get_default_cluster = true;
        }
        else
          throw shcore::Exception::argument_error("Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options");
      }
      else
      {
        cluster_name = args.string_at(0);
        options = args[1].as_map();
      }
    }
    else
      get_default_cluster = true;

    // Validates the options for invalid and missing required attributes
    if (options)
    {
      // Verification of invalid attributes on the instance creation options
      auto invalids = shcore::get_additional_keys(options, { "masterKey" });
      if (invalids.size())
      {
        std::string error = "The instance options contain the following invalid attributes: ";
        error += shcore::join_strings(invalids, ", ");
        throw shcore::Exception::argument_error(error);
      }

      // Verification of required attributes on the instance deployment data
      auto missing = shcore::get_missing_keys(options, { "masterKey" });

      if (missing.size())
        throw shcore::Exception::argument_error("Missing the administrative MASTER key for the cluster");
      else
        master_key = options->get_string("masterKey");
    }

    if (get_default_cluster)
    {
      if (!_default_cluster)
        _default_cluster = _metadata_storage->get_default_cluster();

      cluster = _default_cluster;
    }
    else
    {
      if (cluster_name.empty())
        throw Exception::argument_error("The Cluster name cannot be empty.");

      if (!_clusters->has_key(cluster_name))
        cluster = _metadata_storage->get_cluster(cluster_name);
      else
        ret_val = (*_clusters)[cluster_name];
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getCluster"));

  if (cluster)
  {
    // Caches the master key on the loaded cluster
    cluster->set_password(master_key);

    if (!_clusters->has_key(cluster->get_name()))
       (*_clusters)[cluster->get_name()] = shcore::Value(std::dynamic_pointer_cast<Object_bridge>(cluster));

    ret_val = (*_clusters)[cluster->get_name()];
  }

  return ret_val;
}

/**
 * Creates a Cluster object.
 * \param name The name of the Cluster object to be created
 * \param clusterAdminPassword The Cluster Administration password
 * \param options Options
 * \return The created Cluster object.
 * \sa Cluster
 */
#if DOXYGEN_JS
Cluster Dba::createCluster(String name, String clusterAdminPassword, JSON options){}
#elif DOXYGEN_PY
Cluster Dba::create_cluster(str name, str cluster_admin_password, JSON options){}
#endif
shcore::Value Dba::create_cluster(const shcore::Argument_list &args)
{
  Value ret_val;
  args.ensure_count(2, 3, get_function_name("createCluster").c_str());

  // Available options
  std::string cluster_admin_type = "local"; // Default is local
  std::string instance_admin_user = "mysql_innodb_cluster_admin"; // Default is mysql_innodb_cluster_admin
  std::string cluster_reader_user = "mysql_innodb_cluster_reader"; // Default is mysql_innodb_cluster_reader
  std::string replication_user = "mysql_innodb_cluster_rpl_user"; // Default is mysql_innodb_cluster_rpl_user

  std::string mysql_innodb_cluster_admin_pwd;

  try
  {
    std::string cluster_name = args.string_at(0);

    if (cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

    std::string cluster_password = args.string_at(1);

    // Check if we have a valid password
    if (cluster_password.empty())
      throw Exception::argument_error("The Cluster password cannot be empty.");

    if (args.size() > 2)
    {
      // Map with the options
      shcore::Value::Map_type_ref options = args.map_at(2);

      // Verification of invalid attributes on the instance creation options
      auto invalids = shcore::get_additional_keys(options, { "clusterAdminType" });
      if (invalids.size())
      {
        std::string error = "The instance options contain the following invalid attributes: ";
        error += shcore::join_strings(invalids, ", ");
        throw shcore::Exception::argument_error(error);
      }

      if (options->has_key("clusterAdminType"))
        cluster_admin_type = (*options)["clusterAdminType"].as_string();

      if (cluster_admin_type != "local" ||
          cluster_admin_type != "guided" ||
          cluster_admin_type != "manual" ||
          cluster_admin_type != "ssh")
      {
        throw shcore::Exception::argument_error("Cluster Administration Type invalid. Valid types are: 'local', 'guided', 'manual', 'ssh'");
      }
    }

    MetadataStorage::Transaction tx(_metadata_storage);

    /*
     * For V1.0 we only support one single Cluster. That one shall be the default Cluster.
     * We must check if there's already a Default Cluster assigned, and if so thrown an exception.
     * And we must check if there's already one Cluster on the MD and if so assign it to Default
     */
    bool has_default_cluster = _metadata_storage->has_default_cluster();

    if (_default_cluster || has_default_cluster)
      throw Exception::argument_error("Cluster is already initialized. Use getCluster() to access it.");

    // First we need to create the Metadata Schema, or update it if already exists
    _metadata_storage->create_metadata_schema();

    // Check if we have the instanceAdminUser password or we need to generate it
    if (mysql_innodb_cluster_admin_pwd.empty())
      mysql_innodb_cluster_admin_pwd = cluster_password;

    _default_cluster.reset(new Cluster(cluster_name, _metadata_storage));

    // Update the properties
    _default_cluster->set_password(cluster_password);
    _default_cluster->set_description("Default Cluster");

    _default_cluster->set_account_user(ACC_INSTANCE_ADMIN, instance_admin_user);
    _default_cluster->set_account_password(ACC_INSTANCE_ADMIN, mysql_innodb_cluster_admin_pwd);
    _default_cluster->set_account_user(ACC_CLUSTER_READER, cluster_reader_user);
    _default_cluster->set_account_password(ACC_CLUSTER_READER, generate_password(PASSWORD_LENGHT));
    _default_cluster->set_account_user(ACC_REPLICATION_USER, replication_user);
    _default_cluster->set_account_password(ACC_REPLICATION_USER, generate_password(PASSWORD_LENGHT));

    _default_cluster->set_option(OPT_ADMIN_TYPE, shcore::Value(cluster_admin_type));

    _default_cluster->set_attribute(ATT_DEFAULT, shcore::Value::True());

    // For V1.0, let's see the Cluster's description to "default"

    // Insert Cluster on the Metadata Schema
    _metadata_storage->insert_cluster(_default_cluster);

    auto session = get_active_session();

    shcore::Argument_list args;
    args.push_back(shcore::Value(session->uri()));
    args.push_back(shcore::Value(session->get_password()));

    _default_cluster->add_seed_instance(args);

    // If it reaches here, it means there are no exceptions
    ret_val = Value(std::static_pointer_cast<Object_bridge>(_default_cluster));
    (*_clusters)[cluster_name] = ret_val;

    tx.commit();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createCluster"))

  return ret_val;
}

/**
 * Drops a Cluster object.
 * \param name The name of the Cluster object to be dropped.
 * \return nothing.
 * \sa Cluster
 */
#if DOXYGEN_JS
Undefined Dba::dropCluster(String name){}
#elif DOXYGEN_PY
None Dba::drop_cluster(str name){}
#endif

shcore::Value Dba::drop_cluster(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, get_function_name("dropCluster").c_str());

  try
  {
    std::string cluster_name = args.string_at(0);

    if (cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

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
      _metadata_storage->drop_cluster(cluster_name);

      // If it reaches here, it means there are no exceptions
      if (_clusters->has_key(cluster_name))
        _clusters->erase(cluster_name);
    }
    else
    {
      // check if the Cluster has more replicaSets than the default one
      if (!_metadata_storage->cluster_has_default_replicaset_only(cluster_name))
        throw Exception::logic_error("Cannot drop Cluster: The cluster with the name '"
            + cluster_name + "' has more replicasets than the default replicaset.");

      // drop the default ReplicaSet and call drop_cluster again
      _metadata_storage->drop_default_replicaset(cluster_name);
      _metadata_storage->drop_cluster(cluster_name);

      // If it reaches here, it means there are no exceptions
      if (_clusters->has_key(cluster_name))
        _clusters->erase(cluster_name);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropCluster"))

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

      // If it reaches here, it means there are no exceptions and we can reset the clusters cache
      if (_clusters->size() > 0)
        _clusters.reset(new shcore::Value::Map_type);

      _default_cluster.reset();
      _default_cluster_name = "";
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
          std::string s_out = "The instance: " + host + ":" + std::to_string(port) + " is valid for Cluster usage";
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

  int port = args.int_at(0);
  int portx = 0;
  std::string password;
  std::string sandbox_dir;

  try
  {
    if (args.size() == 2)
    {
      options = args.map_at(1);

      // Verification of invalid attributes on the instance deployment data
      auto invalids = shcore::get_additional_keys(options, _deploy_instance_opts);
      if (invalids.size())
      {
        std::string error = "The instance data contains the following invalid attributes: ";
        error += shcore::join_strings(invalids, ", ");
        throw shcore::Exception::argument_error(error);
      }

      // Verification of required attributes on the instance deployment data
      auto missing = shcore::get_missing_keys(options, { "password|dbPassword" });

      if (missing.size())
          throw shcore::Exception::argument_error("Missing root password for the deployed instance");
      else
      {
        if (options->has_key("password"))
          password = options->get_string("password");
        else if (options->has_key("dbPassword"))
          password = options->get_string("dbPassword");
      }
    }
    else
      throw shcore::Exception::argument_error("Missing root password for the deployed instance");

    password += "\n";

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
    else if (shcore::Shell_core_options::get()->has_key(SHCORE_SANDBOX_DIR))
    {
      std::string dir = (*shcore::Shell_core_options::get())[SHCORE_SANDBOX_DIR].as_string();
      arg = "--sandboxdir="+dir;
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
          break;

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

std::string Dba::get_help_text(const std::string& topic)
{
  std::string ret_val;
  std::map<std::string, std::string> help_data;

  if (topic == "__brief__")
    ret_val = "Enables cluster administration operations.";
  else if (topic == get_function_name("createCluster", false))
    ret_val = "Creates a Cluster.";
  else if (topic == get_function_name("dropCluster", false))
    ret_val = "Deletes a Cluster.";
  else if (topic == get_function_name("getCluster", false))
    ret_val = "Retrieves an InnoDB cluster from the Metadata Store.\n\n"\
    "SYNTAX:\n\n"\
    "   " + get_function_name("getCluster", false) + "([name])\n\n"\
    "WHERE:\n\n"\
    "   name: is an optional parameter to specify name of cluster to be returned.\n\n"\
    "   If name is not specified, the default cluster will be returned.\n\n"\
    "   If name is specified, and no cluster with the indicated name is found,\n"\
    "   an error will be raised.\n";
  else if (topic == get_function_name("dropMetadataSchema", false))
    ret_val = "Destroys the Cluster configuration data.";
  else if (topic == get_function_name("validateInstance", false))
    ret_val = "Validates an instance.";
  else if (topic == get_function_name("deployLocalInstance", false))
    ret_val = "Creates a new MySQL Server instance.";

  return ret_val;
}
