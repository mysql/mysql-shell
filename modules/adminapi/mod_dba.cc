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
using namespace mysh::dba;
using namespace shcore;

#define PASSWORD_LENGHT 16

std::set<std::string> Dba::_deploy_instance_opts = { "portx", "sandboxDir", "password", "dbPassword", "verbose" };
std::set<std::string> Dba::_validate_instance_opts = { "host", "port", "user", "dbUser", "password", "dbPassword", "socket", "ssl_ca", "ssl_cert", "ssl_key", "ssl_key", "verbose" };

Dba::Dba(IShell_core* owner) :
_shell_core(owner) {
  init();
}

bool Dba::operator == (const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Dba::init() {
  // Pure functions
  add_method("resetSession", std::bind(&Dba::reset_session, this, _1), "session", shcore::Object, NULL);
  add_method("createCluster", std::bind(&Dba::create_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("getCluster", std::bind(&Dba::get_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("validateInstance", std::bind(&Dba::validate_instance, this, _1), "data", shcore::Map, NULL);
  add_method("deployLocalInstance", std::bind(&Dba::deploy_local_instance, this, _1, "deployLocalInstance"), "data", shcore::Map, NULL);
  add_varargs_method("startLocalInstance", std::bind(&Dba::deploy_local_instance, this, _1, "startLocalInstance"));
  add_method("stopLocalInstance", std::bind(&Dba::stop_local_instance, this, _1), "data", shcore::Map, NULL);
  add_method("restartLocalInstance", std::bind(&Dba::restart_local_instance, this, _1), "data", shcore::Map, NULL);
  add_method("deleteLocalInstance", std::bind(&Dba::delete_local_instance, this, _1), "data", shcore::Map, NULL);
  add_method("killLocalInstance", std::bind(&Dba::kill_local_instance, this, _1), "data", shcore::Map, NULL);
  add_varargs_method("help", std::bind(&Dba::help, this, _1));

  _metadata_storage.reset(new MetadataStorage(this));
  _provisioning_interface.reset(new ProvisioningInterface(_shell_core->get_delegate()));

  std::string python_path = "";
  std::string local_mysqlprovision_path;

  if (getenv("MYSQL_ORCHESTRATOR") != NULL)
    local_mysqlprovision_path = std::string(getenv("MYSQL_ORCHESTRATOR")); // should be set to the mysql-orchestrator root dir

  if (!local_mysqlprovision_path.empty()) {
    local_mysqlprovision_path += "/gadgets/python";

    std::string sep;
#ifdef WIN32
    sep = ";";
#else
    sep = ":";
#endif

    if (getenv("PYTHONPATH") != NULL)
      python_path = std::string(getenv("PYTHONPATH") + sep + local_mysqlprovision_path);
    else
      python_path = local_mysqlprovision_path;

#ifdef WIN32
    _putenv_s("PYTHONPATH", python_path.c_str());
#else
    setenv("PYTHONPATH", python_path.c_str(), true);
#endif
  }
}

std::string Dba::generate_password(int password_lenght) {
  std::random_device rd;
  std::string pwd;
  const char *alphabet = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~@#%$^&*()-_=+]}[{|;:.>,</?";
  std::uniform_int_distribution<int> dist(0, strlen(alphabet) - 1);

  for (int i = 0; i < password_lenght; i++)
    pwd += alphabet[dist(rd)];

  return pwd;
}

std::shared_ptr<ShellDevelopmentSession> Dba::get_active_session() const {
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
Cluster Dba::getCluster() {}
#elif DOXYGEN_PY
Cluster Dba::get_cluster() {}
#endif
/**
* Retrieves a Cluster object from the Metadata schema.
* \param name The name of the Cluster object to be retrieved.
* \param options Dictionary containing the masterKey value.
* \return The Cluster object with the given name.
*
* If a Cluster with the given name does not exist an error will be raised.
*/
#if DOXYGEN_JS
Cluster Dba::getCluster(String name) {}
#elif DOXYGEN_PY
Cluster Dba::get_cluster(str name) {}
#endif
#endif
shcore::Value Dba::get_cluster(const shcore::Argument_list &args) const {
  Value ret_val;

  validate_session(get_function_name("getCluster"));

  args.ensure_count(0, 2, get_function_name("getCluster").c_str());

  std::shared_ptr<mysh::dba::Cluster> cluster;
  bool get_default_cluster = false;
  std::string cluster_name;
  std::string master_key;
  shcore::Value::Map_type_ref options;

  try {
    // gets the cluster_name and/or options
    if (args.size()) {
      if (args.size() == 1) {
        if (args[0].type == shcore::String)
          cluster_name = args.string_at(0);
        else if (args[0].type == shcore::Map) {
          options = args.map_at(0);
          get_default_cluster = true;
        } else
          throw shcore::Exception::argument_error("Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options");
      } else {
        cluster_name = args.string_at(0);
        options = args.map_at(1);
      }
    } else
      get_default_cluster = true;

    // Validates the options for invalid and missing required attributes
    if (options) {
      // Verification of invalid attributes on the instance creation options
      auto invalids = shcore::get_additional_keys(options, { "masterKey" });
      if (invalids.size()) {
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

    if (get_default_cluster) {
      // Reloads the cluster (to avoid losing _default_cluster in case of error)
      cluster = _metadata_storage->get_default_cluster(master_key);

      // Loaded ok, now it becomes the default cluster
      _default_cluster = cluster;

      cluster = _default_cluster;
    } else {
      if (cluster_name.empty())
        throw Exception::argument_error("The Cluster name cannot be empty.");

      cluster = _metadata_storage->get_cluster(cluster_name, master_key);
    }

    if (cluster)
      ret_val = shcore::Value(std::dynamic_pointer_cast<Object_bridge>(cluster));
    else {
      std::string message;
      if (get_default_cluster)
        message = "No default cluster is configured.";
      else
        message = "The cluster '" + cluster_name + "' is not configured.";

      throw shcore::Exception::logic_error(message);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getCluster"));

  return ret_val;
}

/**
 * Creates a Cluster object.
 * \param name The name of the Cluster object to be created
 * \param masterKey The Cluster master key
 * \param options optional dictionary with options that modify the behavior of this function
 * \return The created Cluster object.
 *
 *  The options dictionary can contain the next values:
 *
 *  - clusterAdminType: determines the type of management to be done on the cluster instances.
 *    Valid values include: local, manual, guided or ssh.
 *    At the moment only local is supported and used as default value if not specified.
 *  - multiMaster: boolean value that indicates whether the group as a singe or multiple master (R/W) nodes.
 *    If not specified false is assigned.
 */
#if DOXYGEN_JS
Cluster Dba::createCluster(String name, String masterKey, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::create_cluster(str name, str masterKey, Dictionary options) {}
#endif
shcore::Value Dba::create_cluster(const shcore::Argument_list &args) {
  Value ret_val;

  validate_session(get_function_name("createCluster"));

  args.ensure_count(2, 3, get_function_name("createCluster").c_str());

  // Available options
  std::string cluster_admin_type = "local"; // Default is local
  std::string instance_admin_user = "mysql_innodb_instance_admin"; // Default is mysql_innodb_cluster_admin
  std::string cluster_reader_user = "mysql_innodb_cluster_reader"; // Default is mysql_innodb_cluster_reader
  std::string replication_user = "mysql_innodb_cluster_rpl_user"; // Default is mysql_innodb_cluster_rpl_user
  std::string cluster_admin_user = "mysql_innodb_cluster_admin";

  std::string mysql_innodb_cluster_admin_pwd;
  bool multi_master = false; // Default single/primary master
  bool verbose = false; // Default is false

  try {
    std::string cluster_name = args.string_at(0);

    if (cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

    if (!shcore::is_valid_identifier(cluster_name))
      throw Exception::argument_error("The Cluster name must be a valid identifier.");

    std::string cluster_password = args.string_at(1);

    // Check if we have a valid password
    if (cluster_password.empty())
      throw Exception::argument_error("The Cluster password cannot be empty.");

    if (args.size() > 2) {
      // Map with the options
      shcore::Value::Map_type_ref options = args.map_at(2);

      // Verification of invalid attributes on the instance creation options
      auto invalids = shcore::get_additional_keys(options, { "clusterAdminType", "multiMaster", "verbose" });
      if (invalids.size()) {
        std::string error = "The instance options contain the following invalid attributes: ";
        error += shcore::join_strings(invalids, ", ");
        throw shcore::Exception::argument_error(error);
      }

      if (options->has_key("clusterAdminType"))
        cluster_admin_type = options->get_string("clusterAdminType");

      if (options->has_key("multiMaster"))
        multi_master = options->get_bool("multiMaster");

      if (cluster_admin_type != "local" &&
          cluster_admin_type != "guided" &&
          cluster_admin_type != "manual" &&
          cluster_admin_type != "ssh") {
        throw shcore::Exception::argument_error("Cluster Administration Type invalid. Valid types are: 'local', 'guided', 'manual', 'ssh'");
      }

      if (options->has_key("verbose"))
        verbose = options->get_bool("verbose");
    }
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

    MetadataStorage::Transaction tx(_metadata_storage);

    // Check if we have the instanceAdminUser password or we need to generate it
    if (mysql_innodb_cluster_admin_pwd.empty())
      mysql_innodb_cluster_admin_pwd = cluster_password;

    _default_cluster.reset(new Cluster(cluster_name, _metadata_storage));

    // Update the properties
    _default_cluster->set_master_key(cluster_password);
    _default_cluster->set_description("Default Cluster");

    _default_cluster->set_account_user(ACC_INSTANCE_ADMIN, instance_admin_user);
    _default_cluster->set_account_password(ACC_INSTANCE_ADMIN, generate_password(PASSWORD_LENGHT));
    _default_cluster->set_account_user(ACC_CLUSTER_READER, cluster_reader_user);
    _default_cluster->set_account_password(ACC_CLUSTER_READER, generate_password(PASSWORD_LENGHT));
    _default_cluster->set_account_user(ACC_REPLICATION_USER, replication_user);
    _default_cluster->set_account_password(ACC_REPLICATION_USER, generate_password(PASSWORD_LENGHT));
    _default_cluster->set_account_user(ACC_CLUSTER_ADMIN, cluster_admin_user);
    _default_cluster->set_account_password(ACC_CLUSTER_ADMIN, mysql_innodb_cluster_admin_pwd);

    _default_cluster->set_option(OPT_ADMIN_TYPE, shcore::Value(cluster_admin_type));

    _default_cluster->set_attribute(ATT_DEFAULT, shcore::Value::True());

    // For V1.0, let's see the Cluster's description to "default"

    // Insert Cluster on the Metadata Schema
    _metadata_storage->insert_cluster(_default_cluster);

    auto session = get_active_session();

    Value::Map_type_ref options(new shcore::Value::Map_type);
    shcore::Argument_list args;

    options = get_connection_data(session->uri(), false);
    (*options)["verbose"] = shcore::Value(verbose);
    args.push_back(shcore::Value(options));

    //args.push_back(shcore::Value(session->uri()));
    args.push_back(shcore::Value(session->get_password()));
    args.push_back(shcore::Value(multi_master ? ReplicaSet::kTopologyMultiMaster
                                              : ReplicaSet::kTopologyPrimaryMaster));
    _default_cluster->add_seed_instance(args);

    // If it reaches here, it means there are no exceptions
    ret_val = Value(std::static_pointer_cast<Object_bridge>(_default_cluster));

    tx.commit();
  } catch (...) {
    _default_cluster.reset();
    translate_crud_exception(get_function_name("createCluster"));
  }
  return ret_val;
}

/**
 * Drops the Metadata Schema.
 * \return nothing.
 */
#if DOXYGEN_JS
Undefined Dba::dropMetadataSchema() {}
#elif DOXYGEN_PY
None Dba::drop_metadata_schema() {}
#endif

shcore::Value Dba::drop_metadata_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("dropMetadataSchema").c_str());

  bool enforce = false;

  // Map with the options
  shcore::Value::Map_type_ref options = args.map_at(0);

  if (options->has_key("enforce"))
    enforce = options->get_bool("enforce");

  if (enforce) {
    try {
      _metadata_storage->drop_metadata_schema();
      _default_cluster.reset();
      _default_cluster_name = "";
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))
  }

  return Value();
}

//! Sets the session object to be used on the Dba operations.
#if DOXYGEN_CPP
//! \param args should contain an instance of ShellDevSession.
#else
//! \param session could be an instance of ClassicSession, XSession or NodeSession.
#endif
/**
* This function is available to configure a specific Session to be used on the Dba operations.
*
* If a specific Session is not set, the Dba operations will be executed through the established global Session.
*/
#if DOXYGEN_JS
Undefined Dba::resetSession(Session session) {}
#elif DOXYGEN_PY
None Dba::reset_session(Session session) {}
#endif
shcore::Value Dba::reset_session(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("resetSession").c_str());

  try {
    if (args.size()) {
      // TODO: Review the case when using a Global_session
      _custom_session = args[0].as_object<ShellDevelopmentSession>();

      if (!_custom_session)
        throw shcore::Exception::argument_error("Invalid session object.");
    } else
      _custom_session.reset();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("resetSession"))

  return Value();
}

shcore::Value Dba::validate_instance(const shcore::Argument_list &args) {
  validate_session(get_function_name("validateInstance"));

  args.ensure_count(1, 2, "validateInstance");

  shcore::Value ret_val;

  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string user;
  std::string password;
  std::string host;
  int port = 0;
  bool verbose = false;

  try {
    // Identify the type of connection data (String or Document)
    if (args[0].type == shcore::String) {
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

    // Verification of invalid attributes on the instance data
    auto invalids = shcore::get_additional_keys(options, _validate_instance_opts);
    if (invalids.size()) {
      std::string error = "The instance data contains the following invalid options: ";
      error += shcore::join_strings(invalids, ", ");
      throw shcore::Exception::argument_error(error);
    }

    // Verification of required attributes on the connection data
    auto missing = shcore::get_missing_keys(options, { "host", "password|dbPassword", "port" });
    if (missing.find("password") != missing.end() && args.size() == 2)
      missing.erase("password");

    if (missing.size()) {
      std::string error = "Missing instance options: ";
      error += shcore::join_strings(missing, ", ");
      throw shcore::Exception::argument_error(error);
    }

    port = options->get_int("port");

    // Sets a default user if not specified
    if (options->has_key("user"))
      user = options->get_string("user");
    else if (options->has_key("dbUser"))
      user = options->get_string("dbUser");
    else {
      user = "root";
      (*options)["dbUser"] = shcore::Value(user);
    }

    host = options->get_string("host");

    if (options->has_key("password"))
      password = options->get_string("password");
    else if (options->has_key("dbPassword"))
      password = options->get_string("dbPassword");
    else if (args.size() == 2 && args[1].type == shcore::String) {
      password = args.string_at(1);
      (*options)["dbPassword"] = shcore::Value(password);
    } else
      throw shcore::Exception::argument_error("Missing password for " + build_connection_string(options, false));

    if (options->has_key("verbose"))
      verbose = options->get_bool("verbose");

    std::string errors;

    // TODO: Add verbose option
    if (_provisioning_interface->check(user, host, port, password, errors, verbose) == 0) {
      std::string s_out = "The instance: " + host + ":" + std::to_string(port) + " is valid for Cluster usage";
      ret_val = shcore::Value(s_out);
    } else
      throw shcore::Exception::logic_error(errors);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION("validateInstance");

  return ret_val;
}

shcore::Value Dba::exec_instance_op(const std::string &function, const shcore::Argument_list &args) {
  shcore::Value ret_val;

  shcore::Value::Map_type_ref options; // Map with the connection data
  shcore::Value mycnf_options;

  int port = args.int_at(0);
  int portx = 0;
  bool verbose = false;
  std::string password;
  std::string sandbox_dir;

  if (args.size() == 2) {
    options = args.map_at(1);

    // Verification of invalid attributes on the instance deployment data
    auto invalids = shcore::get_additional_keys(options, _deploy_instance_opts);
    if (invalids.size()) {
      std::string error = "The instance data contains the following invalid attributes: ";
      error += shcore::join_strings(invalids, ", ");
      throw shcore::Exception::argument_error(error);
    }

    if (function == "deploy") {
      // Verification of required attributes on the instance deployment data
      auto missing = shcore::get_missing_keys(options, { "password|dbPassword" });

      if (missing.size())
        throw shcore::Exception::argument_error("Missing root password for the deployed instance");
      else {
        if (options->has_key("password"))
          password = options->get_string("password");
        else if (options->has_key("dbPassword"))
          password = options->get_string("dbPassword");
      }
    }

    if (options->has_key("portx"))
      portx = options->get_int("portx");

    if (options->has_key("sandboxDir"))
      sandbox_dir = options->get_string("sandboxDir");

    if (options->has_key("verbose"))
      verbose = options->get_bool("verbose");

    if (options->has_key("options"))
      mycnf_options = (*options)["options"];
  } else {
    if (function == "deploy")
      throw shcore::Exception::argument_error("Missing root password for the deployed instance");
  }

  std::string errors;

  if (port <= 0 || port > 65535)
    throw shcore::Exception::argument_error("Please use a valid TCP port number");

  if (function == "deploy") {
    if (_provisioning_interface->deploy_sandbox(port, portx, sandbox_dir, password, mycnf_options, errors, verbose) != 0)
      throw shcore::Exception::logic_error(errors);
  } else if (function == "delete") {
    if (_provisioning_interface->delete_sandbox(port, sandbox_dir, errors, verbose) != 0)
      throw shcore::Exception::logic_error(errors);
  } else if (function == "kill") {
    if (_provisioning_interface->kill_sandbox(port, sandbox_dir, errors, verbose) != 0)
      throw shcore::Exception::logic_error(errors);
  } else if (function == "stop") {
    if (_provisioning_interface->stop_sandbox(port, sandbox_dir, errors, verbose) != 0)
      throw shcore::Exception::logic_error(errors);
  }
  return ret_val;
}

shcore::Value Dba::deploy_local_instance(const shcore::Argument_list &args, const std::string& fname) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name(fname).c_str());

  try {
    ret_val = exec_instance_op("deploy", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(fname));

  return ret_val;
}

shcore::Value Dba::delete_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("deleteLocalInstance").c_str());

  try {
    ret_val = exec_instance_op("delete", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("deleteLocalInstance"));

  return ret_val;
}

shcore::Value Dba::kill_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("killLocalInstance").c_str());

  try {
    ret_val = exec_instance_op("kill", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("killLocalInstance"));

  return ret_val;
}

shcore::Value Dba::stop_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("stopLocalInstance").c_str());

  try {
    ret_val = exec_instance_op("stop", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("stopLocalInstance"));

  return ret_val;
}

shcore::Value Dba::restart_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("restartLocalInstance").c_str());

  try {
    // check if the instance is live 1st
    ret_val = exec_instance_op("stop", args);
    ret_val = exec_instance_op("deploy", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("restartLocalInstance"));

  return ret_val;
}

std::string Dba::get_help_text(const std::string& topic, bool full) {
  std::string ret_val;
  std::map<std::string, std::string> help_data;

  if (topic == "__brief__")
    ret_val = "Perform DBA operations using the MySQL AdminAPI.";
  else if (topic == "__detail__")
    ret_val = "The global variable 'dba' is used to access the MySQL AdminAPI functionality\n"\
              "and perform DBA operations. It is used for managing MySQL InnoDB clusters.";
  else if (topic == "__closing__")
    ret_val = "For more help on a specific function use dba.help('<functionName>')\n"\
              "e.g. dba.help('deployLocalInstance') or dba.help('deployLocalInstance()')";
  else if (topic == get_function_name("createCluster", false))
    ret_val = "Creates a MySQL InnoDB cluster.";
  else if (topic == get_function_name("getCluster", false)) {
    ret_val = "Retrieves a cluster from the Metadata Store.";
    if (full) {
      ret_val += "\n\n"\
    "SYNTAX:\n\n"\
    "   " + get_function_name("getCluster", false) + "([name])\n\n"\
    "WHERE:\n\n"\
    "   name: is an optional parameter to specify name of cluster to be returned.\n\n"\
    "   If name is not specified, the default cluster will be returned.\n\n"\
    "   If name is specified, and no cluster with the indicated name is found,\n"\
    "   an error will be raised.\n";
    }
  } else if (topic == get_function_name("dropMetadataSchema", false))
    ret_val = "Destroys the cluster configuration data.";
  else if (topic == get_function_name("validateInstance", false))
    ret_val = "Validates an instance.";
  else if (topic == get_function_name("deployLocalInstance", false))
    ret_val = "Creates a new MySQL Server instance on localhost.";
  else if (topic == get_function_name("help", false))
    ret_val = "Prints this help.";

  return ret_val;
}

void Dba::validate_session(const std::string &source) const {
  auto session = get_active_session();

  if (!session || session->class_name() != "ClassicSession")
    throw shcore::Exception::runtime_error(source + ": a Classic Session is required to perform this operation");
}
