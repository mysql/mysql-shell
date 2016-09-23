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
#include "utils/utils_help.h"

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

std::set<std::string> Dba::_deploy_instance_opts = {"portx", "sandboxDir", "password", "dbPassword"};
std::set<std::string> Dba::_validate_instance_opts = {"host", "port", "user", "dbUser", "password", "dbPassword", "socket", "ssl_ca", "ssl_cert", "ssl_key", "ssl_key"};

// Documentation of the DBA Class
REGISTER_HELP(DBA_BRIEF, "Allows performing DBA operations using the MySQL Admin API.");
REGISTER_HELP(DBA_DETAIL, "The global variable 'dba' is used to access the MySQL AdminAPI functionality "\
"and perform DBA operations. It is used for managing MySQL InnoDB clusters.");
REGISTER_HELP(DBA_CLOSING, "For more help on a specific function use dba.help('<functionName>')");
REGISTER_HELP(DBA_CLOSING1, "e.g. dba.help('deployLocalInstance')");

REGISTER_HELP(DBA_VERBOSE_BRIEF, "Enables verbose mode on the Dba operations.");

Dba::Dba(IShell_core* owner) :
_shell_core(owner) {
  init();
}

bool Dba::operator == (const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Dba::init() {
  add_property("verbose");

  // Pure functions
  add_method("resetSession", std::bind(&Dba::reset_session, this, _1), "session", shcore::Object, NULL);
  add_method("createCluster", std::bind(&Dba::create_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("getCluster", std::bind(&Dba::get_cluster, this, _1), "clusterName", shcore::String, NULL);
  add_method("dropMetadataSchema", std::bind(&Dba::drop_metadata_schema, this, _1), "data", shcore::Map, NULL);
  add_method("validateInstance", std::bind(&Dba::validate_instance, this, _1), "data", shcore::Map, NULL);
  add_method("deployLocalInstance", std::bind(&Dba::deploy_local_instance, this, _1, "deployLocalInstance"), "data", shcore::Map, NULL);
  add_method("startLocalInstance", std::bind(&Dba::start_local_instance, this, _1), "data", shcore::Map, NULL);
  add_method("stopLocalInstance", std::bind(&Dba::stop_local_instance, this, _1), "data", shcore::Map, NULL);
  add_method("deleteLocalInstance", std::bind(&Dba::delete_local_instance, this, _1), "data", shcore::Map, NULL);
  add_method("killLocalInstance", std::bind(&Dba::kill_local_instance, this, _1), "data", shcore::Map, NULL);
  add_varargs_method("help", std::bind(&Dba::help, this, _1));

  _metadata_storage.reset(new MetadataStorage(this));
  _provisioning_interface.reset(new ProvisioningInterface(_shell_core->get_delegate()));
}

void Dba::set_member(const std::string &prop, Value value) {
  if (prop == "verbose") {
    if (value && value.type == shcore::Bool) {
      _provisioning_interface->set_verbose(value.as_bool());
    } else
      throw shcore::Exception::value_error("Invalid value for property 'verbose'");
  } else {
    Cpp_object_bridge::set_member(prop, value);
  }
}

Value Dba::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "verbose") {
    ret_val = shcore::Value(_provisioning_interface->get_verbose());
  } else {
    ret_val = Cpp_object_bridge::get_member(prop);
  }

  return ret_val;
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

// Documentation of the getCluster function
REGISTER_HELP(DBA_GETCLUSTER_BRIEF, "Retrieves a cluster from the Metadata Store.");
REGISTER_HELP(DBA_GETCLUSTER_PARAM, "@param name Optional parameter to specify the name of the cluster to be returned.");
REGISTER_HELP(DBA_GETCLUSTER_PARAM1, "@param options Optional parameter to specify the masterKey of the cluster being retrieved.");
REGISTER_HELP(DBA_GETCLUSTER_RETURN, "@return The cluster identified with the given name or the default cluster.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL, "If name is not specified, the default cluster will be returned.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL1, "If name is specified, and no cluster with the indicated name is found, an error will be raised.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL2, "The options dictionary must contain the key 'masterKey' with the value of the Cluster's MASTER key.");

/**
* $(DBA_GETCLUSTER_BRIEF)
*
* $(DBA_GETCLUSTER_PARAM)
* $(DBA_GETCLUSTER_PARAM1)
*
* $(DBA_GETCLUSTER_RETURN)
*
* $(DBA_GETCLUSTER_DETAIL)
*
* $(DBA_GETCLUSTER_DETAIL1)
* $(DBA_GETCLUSTER_DETAIL2)
*/
#if DOXYGEN_JS
Cluster Dba::getCluster(String name, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::get_cluster(str name, dict options) {}
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
      auto invalids = shcore::get_additional_keys(options, {"masterKey"});
      if (invalids.size()) {
        std::string error = "The instance options contain the following invalid attributes: ";
        error += shcore::join_strings(invalids, ", ");
        throw shcore::Exception::argument_error(error);
      }

      // Verification of required attributes on the instance deployment data
      auto missing = shcore::get_missing_keys(options, {"masterKey"});

      if (missing.size())
        throw shcore::Exception::argument_error("Missing the administrative MASTER key for the cluster");
      else
        master_key = options->get_string("masterKey");
    }

    if (get_default_cluster) {
      // Reloads the cluster (to avoid losing _default_cluster in case of error)
      cluster = _metadata_storage->get_default_cluster(master_key);
      // Set the provision interface pointer
      cluster->set_provisioning_interface(_provisioning_interface);
    } else {
      if (cluster_name.empty())
        throw Exception::argument_error("The Cluster name cannot be empty.");

      cluster = _metadata_storage->get_cluster(cluster_name, master_key);
      cluster->set_provisioning_interface(_provisioning_interface);
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

REGISTER_HELP(DBA_CREATECLUSTER_BRIEF, "Creates a MySQL InnoDB cluster.");
REGISTER_HELP(DBA_CREATECLUSTER_PARAM, "@param name The name of the cluster object to be created.");
REGISTER_HELP(DBA_CREATECLUSTER_PARAM1, "@param masterKey The cluster master key.");
REGISTER_HELP(DBA_CREATECLUSTER_PARAM2, "@param options Optional dictionary with options that modify the behavior of this function.");
REGISTER_HELP(DBA_CREATECLUSTER_RETURN, "@return The created cluster object.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL, "The options dictionary can contain the next values:");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL1, "@li clusterAdminType: determines the type of management to be done on the cluster instances. "\
"Valid values include: local, manual, guided or ssh. At the moment only local is supported and used as default value if not specified.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL2, "@li multiMaster: boolean value that indicates whether the group has a single master instance or multiple master instances. "\
"If not specified false is assigned.");

/**
 * $(DBA_CREATECLUSTER_BRIEF)
 *
 * $(DBA_CREATECLUSTER_PARAM)
 * $(DBA_CREATECLUSTER_PARAM1)
 * $(DBA_CREATECLUSTER_PARAM2)
 * $(DBA_CREATECLUSTER_RETURN)
 *
 * $(DBA_CREATECLUSTER_DETAIL)
 *
 * $(DBA_CREATECLUSTER_DETAIL1)
 * $(DBA_CREATECLUSTER_DETAIL2)
 */
#if DOXYGEN_JS
Cluster Dba::createCluster(String name, String masterKey, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::create_cluster(str name, str masterKey, dict options) {}
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
      auto invalids = shcore::get_additional_keys(options, {"clusterAdminType", "multiMaster"});
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
    }
    /*
     * For V1.0 we only support one single Cluster. That one shall be the default Cluster.
     * We must check if there's already a Default Cluster assigned, and if so thrown an exception.
     * And we must check if there's already one Cluster on the MD and if so assign it to Default
     */
    bool has_default_cluster = _metadata_storage->has_default_cluster();

    if (has_default_cluster)
      throw Exception::argument_error("Cluster is already initialized. Use getCluster() to access it.");

    // First we need to create the Metadata Schema, or update it if already exists
    _metadata_storage->create_metadata_schema();

    MetadataStorage::Transaction tx(_metadata_storage);

    // Check if we have the instanceAdminUser password or we need to generate it
    if (mysql_innodb_cluster_admin_pwd.empty())
      mysql_innodb_cluster_admin_pwd = cluster_password;

    std::shared_ptr<Cluster> cluster(new Cluster(cluster_name, _metadata_storage));
    cluster->set_provisioning_interface(_provisioning_interface);

    // Update the properties
    cluster->set_master_key(cluster_password);
    cluster->set_description("Default Cluster");

    cluster->set_account_user(ACC_INSTANCE_ADMIN, instance_admin_user);
    cluster->set_account_password(ACC_INSTANCE_ADMIN, generate_password(PASSWORD_LENGHT));
    cluster->set_account_user(ACC_CLUSTER_READER, cluster_reader_user);
    cluster->set_account_password(ACC_CLUSTER_READER, generate_password(PASSWORD_LENGHT));
    cluster->set_account_user(ACC_REPLICATION_USER, replication_user);
    cluster->set_account_password(ACC_REPLICATION_USER, generate_password(PASSWORD_LENGHT));
    cluster->set_account_user(ACC_CLUSTER_ADMIN, cluster_admin_user);
    cluster->set_account_password(ACC_CLUSTER_ADMIN, mysql_innodb_cluster_admin_pwd);

    cluster->set_option(OPT_ADMIN_TYPE, shcore::Value(cluster_admin_type));

    cluster->set_attribute(ATT_DEFAULT, shcore::Value::True());

    // For V1.0, let's see the Cluster's description to "default"

    // Insert Cluster on the Metadata Schema
    _metadata_storage->insert_cluster(cluster);

    auto session = get_active_session();

    Value::Map_type_ref options(new shcore::Value::Map_type);
    shcore::Argument_list args;
    options = get_connection_data(session->uri(), false);
    args.push_back(shcore::Value(options));

    //args.push_back(shcore::Value(session->uri()));
    args.push_back(shcore::Value(session->get_password()));
    args.push_back(shcore::Value(multi_master ? ReplicaSet::kTopologyMultiMaster
                                              : ReplicaSet::kTopologyPrimaryMaster));
    cluster->add_seed_instance(args);

    // If it reaches here, it means there are no exceptions
    ret_val = Value(std::static_pointer_cast<Object_bridge>(cluster));

    tx.commit();
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createCluster"));
  return ret_val;
}

REGISTER_HELP(DBA_DROPMETADATASCHEMA_BRIEF, "Drops the Metadata Schema.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_PARAM, "@param options Dictionary containing an option to confirm the drop operation.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_DETAIL, "The next is the only option supported:");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_DETAIL1, "@li enforce: boolean, confirms that the drop operation must be executed.");

/**
* $(DBA_DROPMETADATASCHEMA_BRIEF)
*
* $(DBA_DROPMETADATASCHEMA_PARAM)
*
* $(DBA_DROPMETADATASCHEMA_DETAIL)
* $(DBA_DROPMETADATASCHEMA_DETAIL1)
*/
#if DOXYGEN_JS
Undefined Dba::dropMetadataSchema(Dictionary options) {}
#elif DOXYGEN_PY
None Dba::drop_metadata_schema(dict options) {}
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
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))
  }

  return Value();
}

//                                    0         1         2         3         4         5         6         7         8
//                                    112345678901234567890123456789012345678901234567890123456789012345679801234567980
REGISTER_HELP(DBA_RESETSESSION_BRIEF, "Sets the session object to be used on the Dba operations.");
REGISTER_HELP(DBA_RESETSESSION_PARAM, "@param session Session object to be used on the Dba operations.");
REGISTER_HELP(DBA_RESETSESSION_DETAIL, "Many of the Dba operations require an active session to the Metadata Store, "\
"use this function to define the session to be used.");
REGISTER_HELP(DBA_RESETSESSION_DETAIL1, "At the moment only a Classic session type is supported.");
REGISTER_HELP(DBA_RESETSESSION_DETAIL2, "If the session type is not defined, the global dba object will use the "\
"established global session.");

/**
* $(DBA_RESETSESSION_BRIEF)
*
* $(DBA_RESETSESSION_PARAM)
*
* $(DBA_RESETSESSION_DETAIL)
*
* $(DBA_RESETSESSION_DETAIL1)
*
* $(DBA_RESETSESSION_DETAIL2)
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

//                                    0         1         2         3         4         5         6         7         8
//                                    112345678901234567890123456789012345678901234567890123456789012345679801234567980
REGISTER_HELP(DBA_VALIDATEINSTANCE_BRIEF, "Validates an instance for usage in Group Replication.");
REGISTER_HELP(DBA_VALIDATEINSTANCE_PARAM, "@param connectionData The instance connection data.");
REGISTER_HELP(DBA_VALIDATEINSTANCE_PARAM1, "@param password Optional string with the password for the connection.");
REGISTER_HELP(DBA_VALIDATEINSTANCE_DETAIL, "This function reviews the instance configuration to identify if it is valid "\
"for usage in group replication.");
REGISTER_HELP(DBA_VALIDATEINSTANCE_DETAIL3, "The connectionData parameter can be any of:");
REGISTER_HELP(DBA_VALIDATEINSTANCE_DETAIL4, "@li URI string ");
REGISTER_HELP(DBA_VALIDATEINSTANCE_DETAIL5, "@li Connection data dictionary ");
REGISTER_HELP(DBA_VALIDATEINSTANCE_DETAIL7, "The password may be contained on the connectionData parameter or can be "\
"specified on the password parameter. When both are specified the parameter "\
"is used instead of the one in the connectionData");

/**
* $(DBA_VALIDATEINSTANCE_BRIEF)
*
* $(DBA_VALIDATEINSTANCE_PARAM)
* $(DBA_VALIDATEINSTANCE_PARAM1)
*
* $(DBA_VALIDATEINSTANCE_DETAIL)
* $(DBA_VALIDATEINSTANCE_DETAIL1)
*
* $(DBA_VALIDATEINSTANCE_DETAIL3)
* $(DBA_VALIDATEINSTANCE_DETAIL4)
* $(DBA_VALIDATEINSTANCE_DETAIL5)
*
* $(DBA_VALIDATEINSTANCE_DETAIL7)
* $(DBA_VALIDATEINSTANCE_DETAIL8)
* $(DBA_VALIDATEINSTANCE_DETAIL9)
*/
#if DOXYGEN_JS
Undefined Dba::validateInstance(Variant connectionData, String password) {}
#elif DOXYGEN_PY
None Dba::validate_instance(variant connectionData, str password) {}
#endif
shcore::Value Dba::validate_instance(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, "validateInstance");

  shcore::Value ret_val;

  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string user;
  std::string password;
  std::string host;
  int port = 0;

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
    auto missing = shcore::get_missing_keys(options, {"host", "password|dbPassword", "port", "user|dbUser"});
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

    std::string errors;

    // Verbose is mandatory for validateInstance
    if (_provisioning_interface->check(user, host, port, password, errors) == 0) {
      std::string s_out = "The instance: " + host + ":" + std::to_string(port) + " is valid for Cluster usage\n";
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
      auto missing = shcore::get_missing_keys(options, {"password|dbPassword"});

      if (missing.size())
        throw shcore::Exception::argument_error("Missing root password for the deployed instance");
      else {
        if (options->has_key("password"))
          password = options->get_string("password");
        else if (options->has_key("dbPassword"))
          password = options->get_string("dbPassword");
      }
    }

    if (options->has_key("portx")) {
      portx = options->get_int("portx");

      if (portx < 1024 || portx > 65535)
        throw shcore::Exception::argument_error("Invalid value for 'portx': Please use a valid TCP port number >= 1024 and <= 65535");
    }

    if (options->has_key("sandboxDir"))
      sandbox_dir = options->get_string("sandboxDir");

    if (options->has_key("options"))
      mycnf_options = (*options)["options"];
  } else {
    if (function == "deploy")
      throw shcore::Exception::argument_error("Missing root password for the deployed instance");
  }

  std::string errors;

  if (port < 1024 || port > 65535)
    throw shcore::Exception::argument_error("Invalid value for 'port': Please use a valid TCP port number >= 1024 and <= 65535");

  if (function == "deploy") {
    // First we need to create the instance
    if (_provisioning_interface->create_sandbox(port, portx, sandbox_dir, password, mycnf_options, errors) != 0)
      throw shcore::Exception::logic_error(errors);
    // And now we start it
    if (_provisioning_interface->start_sandbox(port, sandbox_dir, errors) != 0)
      throw shcore::Exception::logic_error(errors);
  } else if (function == "delete") {
    if (_provisioning_interface->delete_sandbox(port, sandbox_dir, errors) != 0)
      throw shcore::Exception::logic_error(errors);
  } else if (function == "kill") {
    if (_provisioning_interface->kill_sandbox(port, sandbox_dir, errors) != 0)
      throw shcore::Exception::logic_error(errors);
  } else if (function == "stop") {
    if (_provisioning_interface->stop_sandbox(port, sandbox_dir, errors) != 0)
      throw shcore::Exception::logic_error(errors);
  } else if (function == "start") {
    if (_provisioning_interface->start_sandbox(port, sandbox_dir, errors) != 0)
      throw shcore::Exception::logic_error(errors);
  }
  return ret_val;
}

//                                    0         1         2         3         4         5         6         7         8
//                                    112345678901234567890123456789012345678901234567890123456789012345679801234567980
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_BRIEF, "Creates a new MySQL Server instance on localhost.");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_PARAM, "@param port The port where the new instance will listen for connections.");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the new deployed instance.");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_DETAIL, "This function will deploy a new MySQL Server instance, the result may be "\
"affected by the provided options: ");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_DETAIL1, "@li portx: port where the new instance will listen for X Protocol connections.");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_DETAIL2, "@li sandboxDir: path where the new instance will be deployed. ");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the new instance. ");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_DETAIL4, "If the portx option is not specified, it will be automatically calculated "\
"as 10 times the value of the provided MySQL port.");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_DETAIL5, "The password or dbPassword options are mandatory to specify the MySQL root "\
"password on the new instance.");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_DETAIL6, "The sandboxDir must be an existing folder where the new instance will be "\
"deployed. If not specified the new instance will be deployed at:");
REGISTER_HELP(DBA_DEPLOYLOCALINSTANCE_DETAIL7, "  ~HOME/mysql-sandboxes");

/**
* $(DBA_DEPLOYLOCALINSTANCE_BRIEF)
*
* $(DBA_DEPLOYLOCALINSTANCE_PARAM)
* $(DBA_DEPLOYLOCALINSTANCE_PARAM1)
*
* $(DBA_DEPLOYLOCALINSTANCE_DETAIL)
*
* $(DBA_DEPLOYLOCALINSTANCE_DETAIL1)
* $(DBA_DEPLOYLOCALINSTANCE_DETAIL2)
* $(DBA_DEPLOYLOCALINSTANCE_DETAIL3)
*
* $(DBA_DEPLOYLOCALINSTANCE_DETAIL4)
*
* $(DBA_DEPLOYLOCALINSTANCE_DETAIL5)
*
* $(DBA_DEPLOYLOCALINSTANCE_DETAIL6)
*
* $(DBA_DEPLOYLOCALINSTANCE_DETAIL7)
*/
#if DOXYGEN_JS
Undefined Dba::deployLocalInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::deploy_local_instance(int port, dict options) {}
#endif
shcore::Value Dba::deploy_local_instance(const shcore::Argument_list &args, const std::string& fname) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name(fname).c_str());

  try {
    ret_val = exec_instance_op("deploy", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(fname));

  return ret_val;
}

//                                    0         1         2         3         4         5         6         7         8
//                                    112345678901234567890123456789012345678901234567890123456789012345679801234567980
REGISTER_HELP(DBA_DELETELOCALINSTANCE_BRIEF, "Deletes an existing MySQL Server instance on localhost.");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_PARAM, "@param port The port of the instance to be deleted.");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_PARAM1, "@param options Optional dictionary with options that modify the way this function is executed.");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_DETAIL, "This function will delete an existing MySQL Server instance on the local host. The next options affect the result:");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_DETAIL1, "@li portx: port where new instance listens for X Protocol connections.");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_DETAIL2, "@li sandboxDir: path where the instance is located. ");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the instance. ");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_DETAIL4, "The password or dbPassword options are mandatory.");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_DETAIL5, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_DETAIL6, "  ~HOME/mysql-sandboxes");
REGISTER_HELP(DBA_DELETELOCALINSTANCE_DETAIL7, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_DELETELOCALINSTANCE_BRIEF)
*
* $(DBA_DELETELOCALINSTANCE_PARAM)
* $(DBA_DELETELOCALINSTANCE_PARAM1)
*
* $(DBA_DELETELOCALINSTANCE_DETAIL)
*
* $(DBA_DELETELOCALINSTANCE_DETAIL1)
* $(DBA_DELETELOCALINSTANCE_DETAIL2)
* $(DBA_DELETELOCALINSTANCE_DETAIL3)
*
* $(DBA_DELETELOCALINSTANCE_DETAIL4)
*
* $(DBA_DELETELOCALINSTANCE_DETAIL5)
*
* $(DBA_DELETELOCALINSTANCE_DETAIL6)
*
* $(DBA_DELETELOCALINSTANCE_DETAIL7)
*
*/
#if DOXYGEN_JS
Undefined Dba::deleteLocalInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::delete_local_instance(int port, dict options) {}
#endif
shcore::Value Dba::delete_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("deleteLocalInstance").c_str());

  try {
    ret_val = exec_instance_op("delete", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("deleteLocalInstance"));

  return ret_val;
}

//                                    0         1         2         3         4         5         6         7         8
//                                    112345678901234567890123456789012345678901234567890123456789012345679801234567980
REGISTER_HELP(DBA_KILLLOCALINSTANCE_BRIEF, "Kills a running MySQL Server instance on localhost.");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_PARAM, "@param port The port of the instance to be killed.");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the result.");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_DETAIL, "This function will kill the process of a running MySQL Server instance "\
"on the local host. The next options affect the result:");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_DETAIL1, "@li portx: port where the instance listens for X Protocol connections.");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_DETAIL2, "@li sandboxDir: path where the instance is located. ");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the instance. ");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_DETAIL4, "The password or dbPassword options are mandatory.");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_DETAIL5, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_DETAIL6, "  ~HOME/mysql-sandboxes");
REGISTER_HELP(DBA_KILLLOCALINSTANCE_DETAIL7, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_KILLLOCALINSTANCE_BRIEF)
*
* $(DBA_KILLLOCALINSTANCE_PARAM)
* $(DBA_KILLLOCALINSTANCE_PARAM1)
*
* $(DBA_KILLLOCALINSTANCE_DETAIL)
*
* $(DBA_KILLLOCALINSTANCE_DETAIL1)
* $(DBA_KILLLOCALINSTANCE_DETAIL2)
* $(DBA_KILLLOCALINSTANCE_DETAIL3)
*
* $(DBA_KILLLOCALINSTANCE_DETAIL4)
*
* $(DBA_KILLLOCALINSTANCE_DETAIL5)
*
* $(DBA_KILLLOCALINSTANCE_DETAIL6)
*
* $(DBA_KILLLOCALINSTANCE_DETAIL7)
*/
#if DOXYGEN_JS
Undefined Dba::killLocalInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::kill_local_instance(int port, dict options) {}
#endif
shcore::Value Dba::kill_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("killLocalInstance").c_str());

  try {
    ret_val = exec_instance_op("kill", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("killLocalInstance"));

  return ret_val;
}

//                                    0         1         2         3         4         5         6         7         8
//                                    112345678901234567890123456789012345678901234567890123456789012345679801234567980
REGISTER_HELP(DBA_STOPLOCALINSTANCE_BRIEF, "Stops a running MySQL Server instance on localhost.");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_PARAM, "@param port The port of the instance to be stopped.");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the result.");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_DETAIL, "This function will gracefully stop a running MySQL Server instance "\
"on the local host. The next options affect the result:");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_DETAIL1, "@li portx: port where the instance listens for X Protocol connections.");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_DETAIL2, "@li sandboxDir: path where the instance is located. ");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the instance. ");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_DETAIL4, "The password or dbPassword options are mandatory.");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_DETAIL5, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_DETAIL6, "  ~HOME/mysql-sandboxes");
REGISTER_HELP(DBA_STOPLOCALINSTANCE_DETAIL7, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_STOPLOCALINSTANCE_BRIEF)
*
* $(DBA_STOPLOCALINSTANCE_PARAM)
* $(DBA_STOPLOCALINSTANCE_PARAM1)
*
* $(DBA_STOPLOCALINSTANCE_DETAIL)
*
* $(DBA_STOPLOCALINSTANCE_DETAIL1)
* $(DBA_STOPLOCALINSTANCE_DETAIL2)
* $(DBA_STOPLOCALINSTANCE_DETAIL3)
*
* $(DBA_STOPLOCALINSTANCE_DETAIL4)
*
* $(DBA_STOPLOCALINSTANCE_DETAIL5)
*
* $(DBA_STOPLOCALINSTANCE_DETAIL6)
*
* $(DBA_STOPLOCALINSTANCE_DETAIL7)
*/
#if DOXYGEN_JS
Undefined Dba::stopLocalInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::stop_local_instance(int port, dict options) {}
#endif
shcore::Value Dba::stop_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("stopLocalInstance").c_str());

  try {
    ret_val = exec_instance_op("stop", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("stopLocalInstance"));

  return ret_val;
}

//                                    0         1         2         3         4         5         6         7         8
//                                    112345678901234567890123456789012345678901234567890123456789012345679801234567980
REGISTER_HELP(DBA_STARTLOCALINSTANCE_BRIEF, "Starts an existing MySQL Server instance on localhost.");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_PARAM, "@param port The port where the instance listens for MySQL connections.");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the result.");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_DETAIL, "This function will start an existing MySQL Server instance on the local"\
"host. The next options affect the result:");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_DETAIL1, "@li portx: port where the instance listens for X Protocol connections.");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_DETAIL2, "@li sandboxDir: path where the instance is located. ");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the instance. ");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_DETAIL4, "The password or dbPassword options are mandatory.");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_DETAIL5, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_DETAIL6, "  ~HOME/mysql-sandboxes");
REGISTER_HELP(DBA_STARTLOCALINSTANCE_DETAIL7, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_STARTLOCALINSTANCE_BRIEF)
*
* $(DBA_STARTLOCALINSTANCE_PARAM)
* $(DBA_STARTLOCALINSTANCE_PARAM1)
*
* $(DBA_STARTLOCALINSTANCE_DETAIL)
*
* $(DBA_STARTLOCALINSTANCE_DETAIL1)
* $(DBA_STARTLOCALINSTANCE_DETAIL2)
* $(DBA_STARTLOCALINSTANCE_DETAIL3)
*
* $(DBA_STARTLOCALINSTANCE_DETAIL4)
*
* $(DBA_STARTLOCALINSTANCE_DETAIL5)
*
* $(DBA_STARTLOCALINSTANCE_DETAIL6)
*
* $(DBA_STARTLOCALINSTANCE_DETAIL7)
*/
#if DOXYGEN_JS
Undefined Dba::startLocalInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::start_local_instance(int port, dict options) {}
#endif
shcore::Value Dba::start_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("startLocalInstance").c_str());

  try {
    ret_val = exec_instance_op("start", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("startLocalInstance"));

  return ret_val;
}

void Dba::validate_session(const std::string &source) const {
  auto session = get_active_session();

  if (!session || session->class_name() != "ClassicSession")
    throw shcore::Exception::runtime_error(source + ": a Classic Session is required to perform this operation");
}
