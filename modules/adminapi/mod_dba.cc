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
//#include "modules/adminapi/mod_dba_instance.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/mod_mysql_resultset.h"
#include "utils/utils_general.h"
#include "shellcore/object_factory.h"
#include "shellcore/shell_core_options.h"
#include "../mysqlxtest_utils.h"
#include "utils/utils_help.h"
#include "modules/adminapi/mod_dba_sql.h"

#include "logger/logger.h"

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::dba;
using namespace shcore;

#define PASSWORD_LENGHT 16

std::set<std::string> Dba::_deploy_instance_opts = {"portx", "sandboxDir", "password", "dbPassword", "allowRootFrom", "ignoreSslError"};
std::set<std::string> Dba::_default_local_instance_opts = {"sandboxDir"};
std::set<std::string> Dba::_create_cluster_opts = {"clusterAdminType", "multiMaster", "adoptFromGR", "force", "memberSsl", "memberSslCa", "memberSslCert", "memberSslKey"};

// Documentation of the DBA Class
REGISTER_HELP(DBA_BRIEF, "Allows performing DBA operations using the MySQL X AdminAPI.");
REGISTER_HELP(DBA_DETAIL, "The global variable 'dba' is used to access the MySQL AdminAPI functionality "\
"and perform DBA operations. It is used for managing MySQL InnoDB clusters.");
REGISTER_HELP(DBA_CLOSING, "For more help on a specific function use: dba.help('<functionName>')");
REGISTER_HELP(DBA_CLOSING1, "e.g. dba.help('deploySandboxInstance')");

REGISTER_HELP(DBA_VERBOSE_BRIEF, "Enables verbose mode on the Dba operations.");
REGISTER_HELP(DBA_VERBOSE_DETAIL, "The assigned value can be either boolean or integer, the result depends on the assigned value:");
REGISTER_HELP(DBA_VERBOSE_DETAIL1, "@li 0: disables mysqlprovision verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL2, "@li 1: enables mysqlprovision verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL3, "@li >1: enables mysqlprovision debug verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL4, "@li Boolean: equivalent to assign either 0 or 1");

std::map <std::string, std::shared_ptr<mysqlsh::mysql::ClassicSession> > Dba::_session_cache;

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
  add_method("checkInstanceConfig", std::bind(&Dba::check_instance_config, this, _1), "data", shcore::Map, NULL);
  add_method("deploySandboxInstance", std::bind(&Dba::deploy_sandbox_instance, this, _1, "deploySandboxInstance"), "data", shcore::Map, NULL);
  add_method("startSandboxInstance", std::bind(&Dba::start_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("stopSandboxInstance", std::bind(&Dba::stop_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("deleteSandboxInstance", std::bind(&Dba::delete_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("killSandboxInstance", std::bind(&Dba::kill_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("configLocalInstance", std::bind(&Dba::config_local_instance, this, _1), "data", shcore::Map, NULL);
  add_varargs_method("rebootClusterFromCompleteOutage", std::bind(&Dba::reboot_cluster_from_complete_outage, this, _1));
  add_varargs_method("help", std::bind(&Dba::help, this, _1));

  _metadata_storage.reset(new MetadataStorage(this));
  _provisioning_interface.reset(new ProvisioningInterface(_shell_core->get_delegate()));
}

void Dba::set_member(const std::string &prop, Value value) {
  if (prop == "verbose") {
    int verbosity;
    try {
      verbosity = value.as_int();
      _provisioning_interface->set_verbose(verbosity);
    } catch (shcore::Exception& e) {
      throw shcore::Exception::value_error("Invalid value for property 'verbose', use either boolean or integer value.");
    }
  } else {
    Cpp_object_bridge::set_member(prop, value);
  }
}
/**
 * $(DBA_VERBOSE_BRIEF)
 *
 * $(DBA_VERBOSE_DETAIL)
 * $(DBA_VERBOSE_DETAIL1)
 * $(DBA_VERBOSE_DETAIL2)
 * $(DBA_VERBOSE_DETAIL3)
 * $(DBA_VERBOSE_DETAIL4)
 */
#if DOXYGEN_JS || DOXYGEN_PY
Cluster Dba::verbose;
#endif
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
  args.ensure_count(0, 1, get_function_name("getCluster").c_str());

  check_preconditions("getCluster");

  std::shared_ptr<mysqlsh::dba::Cluster> cluster;
  bool get_default_cluster = false;
  std::string cluster_name;
  Value ret_val;

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
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL3, "@li force: boolean, confirms that the multiMaster option must be applied.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL4, "@li adoptFromGR: boolean value that indicates that the cluster shall be created empty and adopt the topology from an existing Group Replication group.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL5, "@li memberSsl: boolean, indicates if SSL "\
    "is used for the instance to start the cluster, by default: true. Set this "\
    "option to false to not use SSL.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL6, "@li memberSslCa: Path of file that "\
    "contains list of trusted SSL CAs to set for the instance.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL7, "@li memberSslCert: Path of file that "\
    "contains X509 certificate in PEM format to set for the instance.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL8, "@li memberSslKey: Path of file that "\
    "contains X509 key in PEM format to set for the instance.");

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
 * $(DBA_CREATECLUSTER_DETAIL3)
 * $(DBA_CREATECLUSTER_DETAIL4)
 * $(DBA_CREATECLUSTER_DETAIL5)
 * $(DBA_CREATECLUSTER_DETAIL6)
 * $(DBA_CREATECLUSTER_DETAIL7)
 * $(DBA_CREATECLUSTER_DETAIL8)
 */
#if DOXYGEN_JS
Cluster Dba::createCluster(String name, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::create_cluster(str name, dict options) {}
#endif
shcore::Value Dba::create_cluster(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("createCluster").c_str());

  ReplicationGroupState state;

  try {
    state = check_preconditions("createCluster");
  } catch (shcore::Exception &e) {
    std::string error(e.what());
    if (error.find("already in an InnoDB cluster") != std::string::npos) {
      /*
      * For V1.0 we only support one single Cluster. That one shall be the default Cluster.
      * We must check if there's already a Default Cluster assigned, and if so thrown an exception.
      * And we must check if there's already one Cluster on the MD and if so assign it to Default
      */
      std::string nice_error = get_function_name("createCluster") + ": Cluster is already initialized. "\
                               "Use " + get_function_name("getCluster") + "() to access it";
      throw shcore::Exception::runtime_error(nice_error);
    } else throw;
  }

  // Available options
  std::string cluster_admin_type = "local"; // Default is local
  Value ret_val;
  bool multi_master = false; // Default single/primary master
  bool adopt_from_gr = false;
  bool force = false;
  // SSL values are only set if available from args.
  bool has_ssl = false;
  bool has_ssl_ca = false, has_ssl_cert = false, has_ssl_key = false;
  bool ssl;
  std::string ssl_ca, ssl_cert, ssl_key;

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

      opt_map.ensure_keys({}, _create_cluster_opts, "the options");

      // Validate SSL options for the cluster instance
      validate_ssl_instance_options(options);

      if (opt_map.has_key("clusterAdminType"))
        cluster_admin_type = opt_map.string_at("clusterAdminType");

      if (opt_map.has_key("multiMaster"))
        multi_master = opt_map.bool_at("multiMaster");

      if (opt_map.has_key("force"))
        force = opt_map.bool_at("force");

      if (cluster_admin_type != "local" &&
          cluster_admin_type != "guided" &&
          cluster_admin_type != "manual" &&
          cluster_admin_type != "ssh") {
        throw shcore::Exception::argument_error("Cluster Administration Type invalid. Valid types are: 'local', 'guided', 'manual', 'ssh'");
      }

      if (opt_map.has_key("adoptFromGR"))
        adopt_from_gr = opt_map.bool_at("adoptFromGR");

      if (opt_map.has_key("memberSsl")) {
        has_ssl = true;
        ssl = opt_map.bool_at("memberSsl");
      }
      if (opt_map.has_key("memberSslCa")) {
        has_ssl_ca = true;
        ssl_ca = opt_map.string_at("memberSslCa");
      }
      if (opt_map.has_key("memberSslCert")) {
        has_ssl_cert = true;
        ssl_cert = opt_map.string_at("memberSslCert");
      }
      if (opt_map.has_key("memberSslKey")) {
        has_ssl_key = true;
        ssl_key = opt_map.string_at("memberSslKey");
      }
    }

    if (state.source_type == GRInstanceType::GroupReplication && !adopt_from_gr)
      throw Exception::argument_error("Creating a cluster on an unmanaged replication group requires adoptFromGR option to be true");

    // First we need to create the Metadata Schema, or update it if already exists
    _metadata_storage->create_metadata_schema();

    MetadataStorage::Transaction tx(_metadata_storage);

    std::shared_ptr<Cluster> cluster(new Cluster(cluster_name, _metadata_storage));
    cluster->set_provisioning_interface(_provisioning_interface);

    // Update the properties
    // For V1.0, let's see the Cluster's description to "default"
    cluster->set_description("Default Cluster");

    cluster->set_option(OPT_ADMIN_TYPE, shcore::Value(cluster_admin_type));

    cluster->set_attribute(ATT_DEFAULT, shcore::Value::True());

    // Insert Cluster on the Metadata Schema
    _metadata_storage->insert_cluster(cluster);

    auto session = get_active_session();

    if (adopt_from_gr) { // TODO: move this to a GR specific class
      // check whether single_primary_mode is on
      auto classic = std::static_pointer_cast<mysql::ClassicSession>(session);
      auto result = classic->execute_sql("select @@group_replication_single_primary_mode");
      auto row = result->fetch_one();
      if (row) {
        log_info("Adopted cluster: group_replication_single_primary_mode=%s",
                 row->get_value_as_string(0).c_str());
        multi_master = row->get_value(0).as_int() == 0;
      }
    }

    Value::Map_type_ref options(new shcore::Value::Map_type);
    shcore::Argument_list args;
    options = get_connection_data(session->uri(), false);
    // Only set SSL option if available from createCluster options. Do not set
    // default values to avoid validation issues for addInstance.
    if (has_ssl)
      (*options)["memberSsl"] = Value(ssl);
    if (has_ssl_ca)
      (*options)["memberSslCa"] = Value(ssl_ca);
    if (has_ssl_cert)
      (*options)["memberSslCert"] = Value(ssl_cert);
    if (has_ssl_key)
      (*options)["memberSslKey"] = Value(ssl_key);
    args.push_back(shcore::Value(options));

    // args.push_back(shcore::Value(session->uri()));
    args.push_back(shcore::Value(session->get_password()));

    if (multi_master && !(force || adopt_from_gr)) {
      throw shcore::Exception::argument_error("Use of multiMaster mode is not recommended unless you understand the limitations. Please use the 'force' option if you understand and accept them.");
    }
    cluster->add_seed_instance(args, multi_master, adopt_from_gr);

    // If it reaches here, it means there are no exceptions
    ret_val = Value(std::static_pointer_cast<Object_bridge>(cluster));

    if (adopt_from_gr)
      cluster->get_default_replicaset()->adopt_from_gr();

    tx.commit();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createCluster"));
  return ret_val;
}

REGISTER_HELP(DBA_DROPMETADATASCHEMA_BRIEF, "Drops the Metadata Schema.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_PARAM, "@param options Dictionary containing an option to confirm the drop operation.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_DETAIL, "The next is the only option supported:");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_DETAIL1, "@li force: boolean, confirms that the drop operation must be executed.");

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

  check_preconditions("dropMetadataSchema");

  try {
    bool force = false;

    // Map with the options
    shcore::Value::Map_type_ref options = args.map_at(0);

    shcore::Argument_map opt_map(*options);

    opt_map.ensure_keys({}, {"force"}, "the options");

    if (opt_map.has_key("force"))
      force = opt_map.bool_at("force");

    if (force)
      _metadata_storage->drop_metadata_schema();
    else
      throw shcore::Exception::runtime_error("No operation executed, use the 'force' option");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropMetadataSchema"))

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

REGISTER_HELP(DBA_CHECKINSTANCECONFIG_BRIEF, "Validates an instance for usage in Group Replication.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_PARAM, "@param instance An instance definition.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_PARAM1, "@param options Optional data for the operation.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_DETAIL, "This function reviews the instance configuration to identify if it is valid "\
"for usage in group replication.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_DETAIL1, "The instance definition can be any of:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_DETAIL2, "@li URI string.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_DETAIL3, "@li Connection data dictionary.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_DETAIL4, "The options parameter can be any of:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_DETAIL5, "@li mycnfPath: The path of the MySQL configuration file for the instance.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_DETAIL6, "@li password: The password to get connected to the instance.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIG_DETAIL7, "The password may be contained on the connectionData parameter or can be "\
"specified on options parameter. When both are specified the one in the options parameter "\
"overrides the one provided in connectionData");

/**
* $(DBA_CHECKINSTANCECONFIG_BRIEF)
*
* $(DBA_CHECKINSTANCECONFIG_PARAM)
* $(DBA_CHECKINSTANCECONFIG_PARAM1)
*
* $(DBA_CHECKINSTANCECONFIG_DETAIL)
*
* $(DBA_CHECKINSTANCECONFIG_DETAIL1)
* $(DBA_CHECKINSTANCECONFIG_DETAIL2)
* $(DBA_CHECKINSTANCECONFIG_DETAIL3)
*
* $(DBA_CHECKINSTANCECONFIG_DETAIL4)
* $(DBA_CHECKINSTANCECONFIG_DETAIL5)
* $(DBA_CHECKINSTANCECONFIG_DETAIL6)
*
* $(DBA_CHECKINSTANCECONFIG_DETAIL7)
*/
#if DOXYGEN_JS
Undefined Dba::checkInstanceConfig(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::check_instance_config(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::check_instance_config(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("checkInstanceConfig").c_str());

  shcore::Value ret_val;

  try {
    ret_val = shcore::Value(_check_instance_config(args, false));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("checkInstanceConfig"));

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
  bool ignore_ssl_error = false;  // SSL is used by default and errors reported.

  if (args.size() == 2) {
    options = args.map_at(1);

    // Verification of invalid attributes on the instance commands
    shcore::Argument_map opt_map(*options);

    if (function == "deploy") {
      opt_map.ensure_keys({}, _deploy_instance_opts, "the instance data");

      if (opt_map.has_key("portx")) {
        portx = opt_map.int_at("portx");

        if (portx < 1024 || portx > 65535)
          throw shcore::Exception::argument_error("Invalid value for 'portx': Please use a valid TCP port number >= 1024 and <= 65535");
      }

      if (opt_map.has_key("password"))
        password = opt_map.string_at("password");
      else if (opt_map.has_key("dbPassword"))
        password = opt_map.string_at("dbPassword");
      else
        throw shcore::Exception::argument_error("Missing root password for the deployed instance");
    } else {
      opt_map.ensure_keys({}, _default_local_instance_opts, "the instance data");
    }

    if (opt_map.has_key("sandboxDir")) {
      sandbox_dir = opt_map.string_at("sandboxDir");

#ifndef WIN32
      // The UNIX domain socket address path has a length limitation so we must check the sandboxDir length
      // sizeof(sockaddr_un::sun_path) - strlen("mysqlx.sock") - strlen("64000") - 2 - 1
      size_t max_socket_path_length = sizeof(sockaddr_un::sun_path) - 19;
      if (sandbox_dir.length() > max_socket_path_length)
        throw shcore::Exception::argument_error("Invalid value for 'sandboxDir': sandboxDir path too long. "\
        "Please keep it shorter than " + std::to_string(max_socket_path_length) + " chars.");
#endif
    }

    if (opt_map.has_key("ignoreSslError"))
      ignore_ssl_error = opt_map.bool_at("ignoreSslError");

    if (options->has_key("options"))
      mycnf_options = (*options)["options"];
  } else {
    if (function == "deploy")
      throw shcore::Exception::argument_error("Missing root password for the deployed instance");
  }

  shcore::Value::Array_type_ref errors;

  if (port < 1024 || port > 65535)
    throw shcore::Exception::argument_error("Invalid value for 'port': Please use a valid TCP port number >= 1024 and <= 65535");

  int rc = 0;
  if (function == "deploy") {
    // First we need to create the instance
    rc = _provisioning_interface->create_sandbox(port, portx, sandbox_dir, password, mycnf_options, ignore_ssl_error, errors);
    if (rc == 0) {
      rc = _provisioning_interface->start_sandbox(port, sandbox_dir, errors);
      //std::string uri = "localhost:" + std::to_string(port);
      //ret_val = shcore::Value::wrap<Instance>(new Instance(uri, uri, options));
    }
  } else if (function == "delete")
      rc = _provisioning_interface->delete_sandbox(port, sandbox_dir, errors);
  else if (function == "kill")
    rc = _provisioning_interface->kill_sandbox(port, sandbox_dir, errors);
  else if (function == "stop")
    rc = _provisioning_interface->stop_sandbox(port, sandbox_dir, errors);
  else if (function == "start")
    rc = _provisioning_interface->start_sandbox(port, sandbox_dir, errors);

  if (rc != 0) {
    std::vector<std::string> str_errors;
    if (errors) {
      for (auto error : *errors) {
        auto data = error.as_map();
        auto error_type = data->get_string("type");
        auto error_text = data->get_string("msg");
        str_errors.push_back(error_type + ": " + error_text);
      }
    }

    throw shcore::Exception::runtime_error(shcore::join_strings(str_errors, "\n"));
  }

  return ret_val;
}

REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_BRIEF, "Creates a new MySQL Server instance on localhost.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_PARAM, "@param port The port where the new instance will listen for connections.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the new deployed instance.");
//REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_RETURN, "@returns The deployed Instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL, "This function will deploy a new MySQL Server instance, the result may be "\
"affected by the provided options: ");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL1, "@li portx: port where the new instance will listen for X Protocol connections.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL2, "@li sandboxDir: path where the new instance will be deployed.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the new instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL4, "@li allowRootFrom: create remote root account, restricted to the given address pattern (eg %).");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL5, "@li ignoreSslError: Ignore errors when adding SSL support for the new "\
    "instance, by default: false.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL6, "If the portx option is not specified, it will be automatically calculated "\
"as 10 times the value of the provided MySQL port.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL7, "The password or dbPassword options are mandatory to specify the MySQL root "\
"password on the new instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL8, "The sandboxDir must be an existing folder where the new instance will be "\
"deployed. If not specified the new instance will be deployed at:");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL9, "  ~/mysql-sandboxes on Unix-like systems or %userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL10, "SSL support is added by "\
    "default if not already available for the new instance. If it fails to be "\
    "added set the ignoreSslError option to true to allow the new instance to "\
    "be deployed without SSL support.");

/**
* $(DBA_DEPLOYSANDBOXINSTANCE_BRIEF)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_PARAM)
* $(DBA_DEPLOYSANDBOXINSTANCE_PARAM1)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL1)
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL2)
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL3)
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL4)
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL5)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL6)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL7)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL8)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL9)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_DETAIL10)
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

    if (args.size() == 2) {
      shcore::Argument_map opt_map(*args.map_at(1));
      // create root@<addr> if needed
      // Valid values:
      // allowRootFrom: address
      // allowRootFrom: %
      // allowRootFrom: null (that is, disable the option)
      if (opt_map.has_key("allowRootFrom") && opt_map.at("allowRootFrom").type != shcore::Null) {
        std::string remote_root = opt_map.string_at("allowRootFrom");
        if (!remote_root.empty()) {
          int port = args.int_at(0);
          std::string password;
          std::string uri = "root@localhost:" + std::to_string(port);
          if (opt_map.has_key("password"))
            password = opt_map.string_at("password");
          else if (opt_map.has_key("dbPassword"))
            password = opt_map.string_at("dbPassword");

          auto session = std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(
                mysqlsh::connect_session(uri, password, mysqlsh::SessionType::Classic));
          assert(session);

          log_info("Creating root@%s account for sandbox %i", remote_root.c_str(), port);
          session->execute_sql("SET sql_log_bin = 0");
          {
            sqlstring create_user("CREATE USER root@? IDENTIFIED BY ?", 0);
            create_user << remote_root << password;
            create_user.done();
            session->execute_sql(create_user);
          }
          {
            sqlstring grant("GRANT ALL ON *.* TO root@? WITH GRANT OPTION", 0);
            grant << remote_root;
            grant.done();
            session->execute_sql(grant);
          }
          session->execute_sql("SET sql_log_bin = 1");

          session->close(shcore::Argument_list());
        }
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(fname));

  return ret_val;
}

REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_BRIEF, "Deletes an existing MySQL Server instance on localhost.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_PARAM, "@param port The port of the instance to be deleted.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_PARAM1, "@param options Optional dictionary with options that modify the way this function is executed.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL, "This function will delete an existing MySQL Server instance on the local host. The next options affect the result:");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL1, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL2, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL3, "  ~/mysql-sandboxes on Unix-like systems or %userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL4, "If the instance is not located on the used path an error will occur.");

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
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL1, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL2, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL3, "  ~/mysql-sandboxes on Unix-like systems or %userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL4, "If the instance is not located on the used path an error will occur.");

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
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL1, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL2, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL3, "  ~/mysql-sandboxes on Unix-like systems or %userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL4, "If the instance is not located on the used path an error will occur.");

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
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL1, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL2, "The sandboxDir must be the one where the MySQL instance was deployed. If not specified it will use:");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL3, "  ~/mysql-sandboxes on Unix-like systems or %userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL4, "If the instance is not located on the used path an error will occur.");

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

REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_BRIEF, "Validates and configures an instance for cluster usage.");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_PARAM, "@param instance An instance definition.");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_PARAM1, "@param options Additional options for the operation.");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_RETURN, "@returns A JSON object with the status.");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_DETAIL, "This function reviews the instance configuration to identify if it is valid "\
"for usage in group replication and cluster. A JSON object is returned containing the result of the operation.");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_DETAIL1, "The instance definition can be any of:");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_DETAIL2, "@li URI string.");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_DETAIL3, "@li Connection data dictionary.");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_DETAIL4, "The options parameter may include:");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_DETAIL5, "@li mycnfPath: The path to the MySQL configuration file of the instance.");
REGISTER_HELP(DBA_CONFIGLOCALINSTANCE_DETAIL6, "@li password: The password to be used on the connection.");

/**
* $(DBA_CONFIGLOCALINSTANCE_BRIEF)
*
* $(DBA_CONFIGLOCALINSTANCE_PARAM)
* $(DBA_CONFIGLOCALINSTANCE_PARAM1)
*
* $(DBA_CONFIGLOCALINSTANCE_RETURN)
*
* $(DBA_CONFIGLOCALINSTANCE_DETAIL)
*
* $(DBA_CONFIGLOCALINSTANCE_DETAIL1)
* $(DBA_CONFIGLOCALINSTANCE_DETAIL2)
* $(DBA_CONFIGLOCALINSTANCE_DETAIL3)
*
* $(DBA_CONFIGLOCALINSTANCE_DETAIL4)
* $(DBA_CONFIGLOCALINSTANCE_DETAIL5)
* $(DBA_CONFIGLOCALINSTANCE_DETAIL6)
*
*/
#if DOXYGEN_JS
Instance Dba::configLocalInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
Instance Dba::config_local_instance(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::config_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, 2, get_function_name("configLocalInstance").c_str());

  try {
    auto options = get_instance_options_map(args, true);

    shcore::Argument_map opt_map(*options);

    if (shcore::is_local_host(opt_map.string_at("host"), true)) {
      ret_val = shcore::Value(_check_instance_config(args, true));
    } else
      throw shcore::Exception::runtime_error("This function only works with local instances");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("configLocalInstance"));

  return ret_val;
}

Dba::~Dba() {
  Dba::_session_cache.clear();
}

std::shared_ptr<mysqlsh::mysql::ClassicSession> Dba::get_session(const shcore::Argument_list& args) {
  std::shared_ptr<mysqlsh::mysql::ClassicSession> ret_val;

  auto options = args.map_at(0);

  std::string session_id = shcore::build_connection_string(options, false);

  if (_session_cache.find(session_id) == _session_cache.end()) {
    auto session = mysqlsh::connect_session(args, mysqlsh::SessionType::Classic);

    ret_val = std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(session);

    // Disabling the session caching for now
    //_session_cache[session_id] = ret_val;
  }

  return ret_val;
}

shcore::Value::Map_type_ref Dba::_check_instance_config(const shcore::Argument_list &args, bool allow_update) {
  shcore::Value::Map_type_ref ret_val(new shcore::Value::Map_type());

  // Validates the connection options
  shcore::Value::Map_type_ref options = get_instance_options_map(args, true);

  // Resolves user and validates password
  resolve_instance_credentials(options);

  shcore::Argument_map opt_map(*options);
  shcore::Argument_map validate_opt_map;

  std::set<std::string> check_instance_config_opts = {"host", "port", "user", "dbUser", "password", "dbPassword", "socket", "sslCa", "sslCert", "sslKey"};

  opt_map.ensure_keys({"host", "port"}, check_instance_config_opts, "instance definition");

  shcore::Value::Map_type_ref validate_options;

  std::string cnfpath;
  if (args.size() == 2) {
    validate_options = args.map_at(1);
    shcore::Argument_map tmp_map(*validate_options);

    std::set<std::string> check_options = {"password", "dbPassword", "mycnfPath"};

    tmp_map.ensure_keys({}, check_options, "validation options");
    validate_opt_map = tmp_map;

    // Retrieves the .cnf path if exists
    if (validate_opt_map.has_key("mycnfPath"))
      cnfpath = validate_opt_map.string_at("mycnfPath");
  }

  if (cnfpath.empty() && allow_update)
    throw shcore::Exception::runtime_error("The path to the MySQL Configuration is required to verify and fix the InnoDB Cluster settings");

  // Now validates the instance GR status itself
  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(options));
  auto session = Dba::get_session(new_args);

  auto uri = session->uri();

  GRInstanceType type = get_gr_instance_type(session->connection());

  session->close(shcore::Argument_list());

  if (type == GRInstanceType::GroupReplication)
    throw shcore::Exception::runtime_error("The instance '" + uri + "' is already part of a Replication Group");
  else if (type == GRInstanceType::InnoDBCluster)
    throw shcore::Exception::runtime_error("The instance '" + uri + "' is already part of an InnoDB Cluster");
  else if (type == GRInstanceType::Standalone) {
    std::string user;
    std::string password;
    std::string host = options->get_string("host");
    int port = options->get_int("port");

    user = options->get_string(options->has_key("user") ? "user" : "dbUser");
    password = options->get_string(options->has_key("password") ? "password" : "dbPassword");

    // Verbose is mandatory for checkInstanceConfig
    shcore::Value::Array_type_ref mp_errors;
    if (_provisioning_interface->check(user, host, port, password, cnfpath, allow_update, mp_errors) == 0)
      (*ret_val)["status"] = shcore::Value("ok");
    else {
      shcore::Value::Array_type_ref errors(new shcore::Value::Array_type());
      bool restart_required = false;

      (*ret_val)["status"] = shcore::Value("error");

      if (mp_errors) {
        for (auto error_object : *mp_errors) {
          auto map = error_object.as_map();

          std::string error_str;
          if (map->get_string("type") == "ERROR") {
            error_str = map->get_string("msg");

            if (error_str.find("The operation could not continue due to the following requirements not being met") != std::string::npos) {
              auto lines = shcore::split_string(error_str, "\n");

              bool loading_options = false;

              shcore::Value::Map_type_ref server_options(new shcore::Value::Map_type());
              std::string option_type;

              for (size_t index = 1; index < lines.size(); index++) {
                if (loading_options) {
                  auto option_tokens = shcore::split_string(lines[index], " ", true);

                  if (option_tokens[1] == "<no") {
                    option_tokens[1] = "<no value>";
                    option_tokens.erase(option_tokens.begin() + 2);
                  }

                  if (option_tokens[2] == "<not") {
                    option_tokens[2] = "<not set>";
                    option_tokens.erase(option_tokens.begin() + 3);
                  }

                  // The tokens describing each option have length of 5
                  if (option_tokens.size() > 5) {
                    index--;
                    loading_options = false;
                  } else {
                    shcore::Value::Map_type_ref option;
                    if (!server_options->has_key(option_tokens[0])) {
                      option.reset(new shcore::Value::Map_type());
                      (*server_options)[option_tokens[0]] = shcore::Value(option);
                    } else
                      option = (*server_options)[option_tokens[0]].as_map();

                    (*option)["required"] = shcore::Value(option_tokens[1]); // The required value
                    (*option)[option_type] = shcore::Value(option_tokens[2]);// The current value
                  }
                } else {
                  if (lines[index].find("Some active options on server") != std::string::npos) {
                    option_type = "server";
                    loading_options = true;
                    index += 3; // Skips to the actual option table
                  } else if (lines[index].find("Some of the configuration values on your options file") != std::string::npos) {
                    option_type = "config";
                    loading_options = true;
                    index += 3; // Skips to the actual option table
                  }
                }
              }

              if (server_options->size()) {
                shcore::Value::Array_type_ref config_errors(new shcore::Value::Array_type());
                for (auto option : *server_options) {
                  auto state = option.second.as_map();

                  std::string required_value = state->get_string("required");
                  std::string server_value = state->get_string("server", "");
                  std::string config_value = state->get_string("config", "");

                  // Taken from MP, reading docs made me think all variables should require restart
                  // Even several of them are dynamic, it seems changing values may lead to problems
                  // An extransaction_write_set_extraction which apparently is reserved for future use
                  // So I just took what I saw on the MP code
                  // Source: http://dev.mysql.com/doc/refman/5.7/en/dynamic-system-variables.html
                  std::vector<std::string> dynamic_variables = {"binlog_format", "binlog_checksum"};

                  bool dynamic = std::find(dynamic_variables.begin(), dynamic_variables.end(), option.first) != dynamic_variables.end();

                  std::string action;
                  std::string current;
                  if (!server_value.empty() && !config_value.empty()) { // Both server and configuration are wrong
                    if (dynamic)
                      action = "server_update+config_update";
                    else {
                      action = "config_update+restart";
                      restart_required = true;
                    }

                    current = server_value;
                  } else if (!config_value.empty()) { // Configuration is wrong, server is OK
                    action = "config_update";
                    current = config_value;
                  } else if (!server_value.empty()) { // Server is wronf, configuration is OK
                    if (dynamic)
                      action = "server_update";
                    else {
                      action = "restart";
                      restart_required = true;
                    }
                    current = server_value;
                  }

                  shcore::Value::Map_type_ref error(new shcore::Value::Map_type());

                  (*error)["option"] = shcore::Value(option.first);
                  (*error)["current"] = shcore::Value(current);
                  (*error)["required"] = shcore::Value(required_value);
                  (*error)["action"] = shcore::Value(action);

                  config_errors->push_back(shcore::Value(error));
                }

                (*ret_val)["config_errors"] = shcore::Value(config_errors);
              }
            } else
              errors->push_back(shcore::Value(error_str));
          }
        }
      }

      (*ret_val)["errors"] = shcore::Value(errors);
      (*ret_val)["restart_required"] = shcore::Value(restart_required);
    }
  }

  return ret_val;
}

REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_BRIEF, "Reboots a cluster from complete outage.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_PARAM, "@param name Optional The name of the cluster to be rebooted.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_RETURN, "@return The rebooted cluster object.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL, "This function reboots a cluster from complete outage, "\
  "It picks the instance the MySQL Shell is connected to as new seed instance and recovers the cluster "\
  "based on the existent Metadata of that instance.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL1, "On success, the restored cluster object is returned by the function.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL2, "The current session must be connected to a former instance of the cluster.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL3, "If name is not specified, the default cluster will be returned.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL4, "EXAMPLE:");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL5, "shell.connect('root@localhost:3310');");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL6, "var cluster = dba.rebootClusterFromCompleteOutage():");

/**
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_BRIEF)
*
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_PARAM)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_RETURN)
*
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL1)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL2)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL3)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL4)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL5)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL6)
*/
#if DOXYGEN_JS
Undefined Dba::rebootClusterFromCompleteOutage(Dictionary options) {}
#elif DOXYGEN_PY
None Dba::reboot_cluster_from_complete_outage(dict options) {}
#endif

shcore::Value Dba::reboot_cluster_from_complete_outage(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("rebootClusterFromCompleteOutage").c_str());

  shcore::Value ret_val;

  throw Exception::logic_error("Function not available yet.");

  if (args.size() == 0) {
    // Use the default cluster
  } else {
    std::string cluster_name = args.string_at(0);

    if (cluster_name.empty())
      throw Exception::argument_error("The Cluster name cannot be empty.");

    if (!shcore::is_valid_identifier(cluster_name))
      throw Exception::argument_error("The Cluster name must be a valid identifier.");
  }

  // Do not forget to check if @@group_replication_group_name changes after the reboot and
  // if so, update the metadata accordingly

  return ret_val;
}

ReplicationGroupState Dba::check_preconditions(const std::string& function_name) const {
  return check_function_preconditions(class_name(), function_name, get_function_name(function_name), _metadata_storage);
}
