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

#include <string>
#include <random>
#ifndef WIN32
#include <sys/un.h>
#endif
#include "utils/utils_sqlstring.h"
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_instance.h"
#include "modules/adminapi/mod_dba_common.h"
#include "utils/utils_general.h"
#include "shellcore/object_factory.h"
#include "shellcore/shell_core_options.h"
#include "../mysqlxtest_utils.h"
#include "utils/utils_help.h"

#include "logger/logger.h"

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include <boost/lexical_cast.hpp>

#include "common/process_launcher/process_launcher.h"

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::dba;
using namespace shcore;

#define PASSWORD_LENGHT 16

std::set<std::string> Dba::_deploy_instance_opts = {"portx", "sandboxDir", "password", "dbPassword"};
std::set<std::string> Dba::_validate_instance_opts = {"host", "port", "user", "dbUser", "password", "dbPassword", "socket", "ssl_ca", "ssl_cert", "ssl_key", "ssl_key"};
std::set<std::string> Dba::_prepare_instance_opts = {"host", "port", "user", "dbUser", "password", "dbPassword", "socket"};

// Documentation of the DBA Class
REGISTER_HELP(DBA_BRIEF, "Allows performing DBA operations using the MySQL Admin API.");
REGISTER_HELP(DBA_DETAIL, "The global variable 'dba' is used to access the MySQL AdminAPI functionality "\
"and perform DBA operations. It is used for managing MySQL InnoDB clusters.");
REGISTER_HELP(DBA_CLOSING, "For more help on a specific function use: dba.help('<functionName>')");
REGISTER_HELP(DBA_CLOSING1, "e.g. dba.help('deploySandboxInstance')");

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
  add_method("deploySandboxInstance", std::bind(&Dba::deploy_sandbox_instance, this, _1, "deploySandboxInstance"), "data", shcore::Map, NULL);
  add_method("startSandboxInstance", std::bind(&Dba::start_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("stopSandboxInstance", std::bind(&Dba::stop_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("deleteSandboxInstance", std::bind(&Dba::delete_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("killSandboxInstance", std::bind(&Dba::kill_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("prepareInstance", std::bind(&Dba::prepare_instance, this, _1), "data", shcore::Map, NULL);
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
REGISTER_HELP(DBA_GETCLUSTER_RETURN, "@return The cluster identified with the given name or the default cluster.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL, "If name is not specified, the default cluster will be returned.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL1, "If name is specified, and no cluster with the indicated name is found, an error will be raised.");

/**
* $(DBA_GETCLUSTER_BRIEF)
*
* $(DBA_GETCLUSTER_PARAM)
*
* $(DBA_GETCLUSTER_RETURN)
*
* $(DBA_GETCLUSTER_DETAIL)
*
* $(DBA_GETCLUSTER_DETAIL1)
*/
#if DOXYGEN_JS
Cluster Dba::getCluster(String name) {}
#elif DOXYGEN_PY
Cluster Dba::get_cluster(str name) {}
#endif
shcore::Value Dba::get_cluster(const shcore::Argument_list &args) const {
  Value ret_val;

  validate_session(get_function_name("getCluster"));

  args.ensure_count(0, 1, get_function_name("getCluster").c_str());

  std::shared_ptr<mysh::dba::Cluster> cluster;
  bool get_default_cluster = false;
  std::string cluster_name;

  try {
    // gets the cluster_name and/or options
    if (args.size()) {
      try {
        cluster_name = args.string_at(0);
      } catch (std::exception &e) {
        throw shcore::Exception::argument_error(std::string("Invalid cluster name: ") + e.what());
      }
    } else
      get_default_cluster = true;

    if (get_default_cluster) {
      // Reloads the cluster (to avoid losing _default_cluster in case of error)
      cluster = _metadata_storage->get_default_cluster();
    } else {
      if (cluster_name.empty())
        throw Exception::argument_error("The Cluster name cannot be empty.");

      cluster = _metadata_storage->get_cluster(cluster_name);
    }
    if (cluster) {
      // Set the provision interface pointer
      cluster->set_provisioning_interface(_provisioning_interface);
      ret_val = shcore::Value(std::dynamic_pointer_cast<Object_bridge>(cluster));
    } else {
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
REGISTER_HELP(DBA_CREATECLUSTER_PARAM1, "@param options Optional dictionary with options that modify the behavior of this function.");
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
 * $(DBA_CREATECLUSTER_RETURN)
 *
 * $(DBA_CREATECLUSTER_DETAIL)
 *
 * $(DBA_CREATECLUSTER_DETAIL1)
 * $(DBA_CREATECLUSTER_DETAIL2)
 */
#if DOXYGEN_JS
Cluster Dba::createCluster(String name, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::create_cluster(str name, dict options) {}
#endif
shcore::Value Dba::create_cluster(const shcore::Argument_list &args) {
  Value ret_val;

  validate_session(get_function_name("createCluster"));

  args.ensure_count(1, 2, get_function_name("createCluster").c_str());

  // Available options
  std::string cluster_admin_type = "local"; // Default is local

  bool multi_master = false; // Default single/primary master

  try {
    std::string cluster_name = args.string_at(0);

    if (cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

    if (!shcore::is_valid_identifier(cluster_name))
      throw Exception::argument_error("The Cluster name must be a valid identifier.");

    if (args.size() > 1) {
      // Map with the options
      shcore::Value::Map_type_ref options = args.map_at(1);

      // Verification of invalid attributes on the instance creation options
      shcore::Argument_map opt_map(*options);
      
      opt_map.ensure_keys({}, {"clusterAdminType", "multiMaster"}, "the options");
      
      if (opt_map.has_key("clusterAdminType"))
        cluster_admin_type = opt_map.string_at("clusterAdminType");

      if (opt_map.has_key("multiMaster"))
        multi_master = opt_map.bool_at("multiMaster");

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
      throw Exception::argument_error("Cluster is already initialized. Use " + get_function_name("getCluster") + "() to access it.");

    // First we need to create the Metadata Schema, or update it if already exists
    _metadata_storage->create_metadata_schema();

    MetadataStorage::Transaction tx(_metadata_storage);

    std::shared_ptr<Cluster> cluster(new Cluster(cluster_name, _metadata_storage));
    cluster->set_provisioning_interface(_provisioning_interface);

    // Update the properties
    cluster->set_description("Default Cluster");

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
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createCluster"));
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

REGISTER_HELP(DBA_VALIDATEINSTANCE_BRIEF, "Validates an instance for usage in Group Replication.");
REGISTER_HELP(DBA_VALIDATEINSTANCE_PARAM, "@param connectionData The instance connection data.");
REGISTER_HELP(DBA_VALIDATEINSTANCE_PARAM1, "@param password Optional string with the password for the connection.");
REGISTER_HELP(DBA_VALIDATEINSTANCE_DETAIL, "This function reviews the instance configuration to identify if it is valid "\
"for usage in group replication.");
REGISTER_HELP(DBA_VALIDATEINSTANCE_DETAIL3, "The connectionData parameter can be any of:");
REGISTER_HELP(DBA_VALIDATEINSTANCE_DETAIL4, "@li URI string.");
REGISTER_HELP(DBA_VALIDATEINSTANCE_DETAIL5, "@li Connection data dictionary.");
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
    
    auto options = get_instance_options_map(args);
    
    shcore::Argument_map opt_map(*options);
    opt_map.ensure_keys({"host", "port", "user|dbUser"}, _validate_instance_opts, "instance definition");
    
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
    else
      throw shcore::Exception::argument_error("Missing password for " + build_connection_string(options, false));

    std::string errors;

    // Verbose is mandatory for validateInstance
    if (_provisioning_interface->check(user, host, port, password, errors, true) == 0) {
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
    shcore::Argument_map opt_map (*options);
    
    opt_map.ensure_keys({}, _deploy_instance_opts, "the instance data");

    if (function == "deploy") {
      if (opt_map.has_key("password"))
        password = opt_map.string_at("password");
      else if (opt_map.has_key("dbPassword"))
        password = opt_map.string_at("dbPassword");
      else
        throw shcore::Exception::argument_error("Missing root password for the deployed instance");
    }

    if (opt_map.has_key("portx")) {
      portx = opt_map.int_at("portx");

      if (portx < 1024 || portx > 65535)
        throw shcore::Exception::argument_error("Invalid value for 'portx': Please use a valid TCP port number >= 1024 and <= 65535");
    }

#ifndef WIN32
    if (opt_map.has_key("sandboxDir")) {
      sandbox_dir = opt_map.string_at("sandboxDir");

      // The UNIX domain socket address path has a length limitation so we must check the sandboxDir length
      // sizeof(sockaddr_un::sun_path) - strlen("mysqlx.sock") - strlen("64000") - 2 - 1
      size_t max_socket_path_length = sizeof(sockaddr_un::sun_path) - 19;
      if (sandbox_dir.length() > max_socket_path_length)
        throw shcore::Exception::argument_error("Invalid value for 'sandboxDir': sandboxDir path too long. "\
        "Please keep it shorter than " + std::to_string(max_socket_path_length) + " chars.");
    }
#endif

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

    std::string uri = "localhost:" + std::to_string(port);
    ret_val = shcore::Value::wrap<Instance>(new Instance(uri, uri, options));
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

REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_BRIEF, "Creates a new MySQL Server instance on localhost.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_PARAM, "@param port The port where the new instance will listen for connections.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the new deployed instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_RETURN, "@returns The deployed Instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL, "This function will deploy a new MySQL Server instance, the result may be "\
"affected by the provided options: ");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL1, "@li portx: port where the new instance will listen for X Protocol connections.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL2, "@li sandboxDir: path where the new instance will be deployed.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the new instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL4, "If the portx option is not specified, it will be automatically calculated "\
"as 10 times the value of the provided MySQL port.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL5, "The password or dbPassword options are mandatory to specify the MySQL root "\
"password on the new instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL6, "The sandboxDir must be an existing folder where the new instance will be "\
"deployed. If not specified the new instance will be deployed at:");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL7, "  ~HOME/mysql-sandboxes");

/**
* $(DBA_DEPLOYSANDBOXINSTANCE_BRIEF)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_PARAM)
* $(DBA_DEPLOYSANDBOXINSTANCE_PARAM1)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_RETURN)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL1)
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL2)
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL3)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL4)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL5)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL6)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL7)
*/
#if DOXYGEN_JS
Instance Dba::deploySandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
Instance Dba::deploy_sandbox_instance(int port, dict options) {}
#endif
shcore::Value Dba::deploy_sandbox_instance(const shcore::Argument_list &args, const std::string& fname) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name(fname).c_str());

  try {
    ret_val = exec_instance_op("deploy", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(fname));

  return ret_val;
}

REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_BRIEF, "Deletes an existing MySQL Server instance on localhost.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_PARAM, "@param port The port of the instance to be deleted.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_PARAM1, "@param options Optional dictionary with options that modify the way this function is executed.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL, "This function will delete an existing MySQL Server instance on the local host. The next options affect the result:");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL1, "@li portx: port where new instance listens for X Protocol connections.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL2, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the instance.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL4, "The password or dbPassword options are mandatory.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL5, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL6, "  ~HOME/mysql-sandboxes");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL7, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_DELETESANDBOXINSTANCE_BRIEF)
*
* $(DBA_DELETESANDBOXINSTANCE_PARAM)
* $(DBA_DELETESANDBOXINSTANCE_PARAM1)
*
* $(DBA_DELETESANDBOXINSTANCE_DETAIL)
*
* $(DBA_DELETESANDBOXINSTANCE_DETAIL1)
* $(DBA_DELETESANDBOXINSTANCE_DETAIL2)
* $(DBA_DELETESANDBOXINSTANCE_DETAIL3)
*
* $(DBA_DELETESANDBOXINSTANCE_DETAIL4)
*
* $(DBA_DELETESANDBOXINSTANCE_DETAIL5)
*
* $(DBA_DELETESANDBOXINSTANCE_DETAIL6)
*
* $(DBA_DELETESANDBOXINSTANCE_DETAIL7)
*
*/
#if DOXYGEN_JS
Undefined Dba::deleteSandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::delete_sandbox_instance(int port, dict options) {}
#endif
shcore::Value Dba::delete_sandbox_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("deleteSandboxInstance").c_str());

  try {
    ret_val = exec_instance_op("delete", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("deleteSandboxInstance"));

  return ret_val;
}

REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_BRIEF, "Kills a running MySQL Server instance on localhost.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_PARAM, "@param port The port of the instance to be killed.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the result.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL, "This function will kill the process of a running MySQL Server instance "\
"on the local host. The next options affect the result:");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL1, "@li portx: port where the instance listens for X Protocol connections.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL2, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the instance.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL4, "The password or dbPassword options are mandatory.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL5, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL6, "  ~HOME/mysql-sandboxes");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL7, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_KILLSANDBOXINSTANCE_BRIEF)
*
* $(DBA_KILLSANDBOXINSTANCE_PARAM)
* $(DBA_KILLSANDBOXINSTANCE_PARAM1)
*
* $(DBA_KILLSANDBOXINSTANCE_DETAIL)
*
* $(DBA_KILLSANDBOXINSTANCE_DETAIL1)
* $(DBA_KILLSANDBOXINSTANCE_DETAIL2)
* $(DBA_KILLSANDBOXINSTANCE_DETAIL3)
*
* $(DBA_KILLSANDBOXINSTANCE_DETAIL4)
*
* $(DBA_KILLSANDBOXINSTANCE_DETAIL5)
*
* $(DBA_KILLSANDBOXINSTANCE_DETAIL6)
*
* $(DBA_KILLSANDBOXINSTANCE_DETAIL7)
*/
#if DOXYGEN_JS
Undefined Dba::killSandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::kill_sandbox_instance(int port, dict options) {}
#endif
shcore::Value Dba::kill_sandbox_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("killSandboxInstance").c_str());

  try {
    ret_val = exec_instance_op("kill", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("killSandboxInstance"));

  return ret_val;
}

REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_BRIEF, "Stops a running MySQL Server instance on localhost.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_PARAM, "@param port The port of the instance to be stopped.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the result.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL, "This function will gracefully stop a running MySQL Server instance "\
"on the local host. The next options affect the result:");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL1, "@li portx: port where the instance listens for X Protocol connections.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL2, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the instance.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL4, "The password or dbPassword options are mandatory.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL5, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL6, "  ~HOME/mysql-sandboxes");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL7, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_STOPSANDBOXINSTANCE_BRIEF)
*
* $(DBA_STOPSANDBOXINSTANCE_PARAM)
* $(DBA_STOPSANDBOXINSTANCE_PARAM1)
*
* $(DBA_STOPSANDBOXINSTANCE_DETAIL)
*
* $(DBA_STOPSANDBOXINSTANCE_DETAIL1)
* $(DBA_STOPSANDBOXINSTANCE_DETAIL2)
* $(DBA_STOPSANDBOXINSTANCE_DETAIL3)
*
* $(DBA_STOPSANDBOXINSTANCE_DETAIL4)
*
* $(DBA_STOPSANDBOXINSTANCE_DETAIL5)
*
* $(DBA_STOPSANDBOXINSTANCE_DETAIL6)
*
* $(DBA_STOPSANDBOXINSTANCE_DETAIL7)
*/
#if DOXYGEN_JS
Undefined Dba::stopSandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::stop_sandbox_instance(int port, dict options) {}
#endif
shcore::Value Dba::stop_sandbox_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("stopSandboxInstance").c_str());

  try {
    ret_val = exec_instance_op("stop", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("stopSandboxInstance"));

  return ret_val;
}

REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_BRIEF, "Starts an existing MySQL Server instance on localhost.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_PARAM, "@param port The port where the instance listens for MySQL connections.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the result.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL, "This function will start an existing MySQL Server instance on the local "\
              "host. The next options affect the result:");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL1, "@li portx: port where the instance listens for X Protocol connections.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL2, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the instance.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL4, "The password or dbPassword options are mandatory.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL5, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL6, "  ~HOME/mysql-sandboxes");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL7, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_STARTSANDBOXINSTANCE_BRIEF)
*
* $(DBA_STARTSANDBOXINSTANCE_PARAM)
* $(DBA_STARTSANDBOXINSTANCE_PARAM1)
*
* $(DBA_STARTSANDBOXINSTANCE_DETAIL)
*
* $(DBA_STARTSANDBOXINSTANCE_DETAIL1)
* $(DBA_STARTSANDBOXINSTANCE_DETAIL2)
* $(DBA_STARTSANDBOXINSTANCE_DETAIL3)
*
* $(DBA_STARTSANDBOXINSTANCE_DETAIL4)
*
* $(DBA_STARTSANDBOXINSTANCE_DETAIL5)
*
* $(DBA_STARTSANDBOXINSTANCE_DETAIL6)
*
* $(DBA_STARTSANDBOXINSTANCE_DETAIL7)
*/
#if DOXYGEN_JS
Undefined Dba::startSandboxInstance(Integer port, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::start_sandbox_instance(int port, dict options) {}
#endif
shcore::Value Dba::start_sandbox_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name("startSandboxInstance").c_str());

  try {
    ret_val = exec_instance_op("start", args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("startSandboxInstance"));

  return ret_val;
}

void Dba::validate_session(const std::string &source) const {
  auto session = get_active_session();

  if (!session || session->class_name() != "ClassicSession")
    throw shcore::Exception::runtime_error(source + ": a Classic Session is required to perform this operation");
}

REGISTER_HELP(DBA_PREPAREINSTANCE_BRIEF, "Validates and prepares an instance for cluster usage.");
REGISTER_HELP(DBA_PREPAREINSTANCE_PARAM, "@param connectionData The instance connection data.");
REGISTER_HELP(DBA_PREPAREINSTANCE_RETURN, "@returns The prepared Instance.");
REGISTER_HELP(DBA_PREPAREINSTANCE_DETAIL, "This function reviews the instance configuration to identify if it is valid "\
"for usage in group replication and cluster and returns an Instance object if so.");
REGISTER_HELP(DBA_PREPAREINSTANCE_DETAIL1, "The connectionData parameter can be any of:");
REGISTER_HELP(DBA_PREPAREINSTANCE_DETAIL2, "@li URI string.");
REGISTER_HELP(DBA_PREPAREINSTANCE_DETAIL3, "@li Connection data dictionary.");

/**
* $(DBA_PREPAREINSTANCE_BRIEF)
*
* $(DBA_PREPAREINSTANCE_PARAM)
*
* $(DBA_PREPAREINSTANCE_RETURN)
*
* $(DBA_PREPAREINSTANCE_DETAIL)
* $(DBA_PREPAREINSTANCE_DETAIL1)
* $(DBA_PREPAREINSTANCE_DETAIL2)
* $(DBA_PREPAREINSTANCE_DETAIL3)
*/
#if DOXYGEN_JS
Instance Dba::prepareInstance(Variant connectionData) {}
#elif DOXYGEN_PY
Instance Dba::prepare_instance(variant connectionData) {}
#endif
shcore::Value Dba::prepare_instance(const shcore::Argument_list &args) {
  args.ensure_count(1, "prepareInstance");

  // For FB3, we will allow an options dictionary as argument

  shcore::Value ret_val;

  std::string uri;
  shcore::Value::Map_type_ref options;  // Map with the connection data

  std::string user;
  std::string password;
  std::string host;
  int port = 0;

  try {
    
    auto options = get_instance_options_map(args);
    
    shcore::Argument_map opt_map(*options);
    opt_map.ensure_keys({"host", "port", "user|dbUser"}, _prepare_instance_opts, "instance definition");
    
    port = options->get_int("port");

    // Sets a default user if not specified
    if (options->has_key("user")) {
      user = options->get_string("user");
    } else if (options->has_key("dbUser")) {
      user = options->get_string("dbUser");
    } else {
      user = "root";
      (*options)["dbUser"] = shcore::Value(user);
    }

    host = options->get_string("host");

    if (options->has_key("password"))
      password = options->get_string("password");
    else if (options->has_key("dbPassword"))
      password = options->get_string("dbPassword");
    else
      throw shcore::Exception::argument_error("Missing password for " + build_connection_string(options, false));

    std::string errors;

    if (_provisioning_interface->check(user, host, port, password, errors, false) == 0) {
      std::string uri = host + ":" + std::to_string(port);
      ret_val = shcore::Value::wrap<Instance>(new Instance(uri, uri, options));
    } else {
      throw shcore::Exception::logic_error(errors);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION("prepareInstance");

  return ret_val;
}
