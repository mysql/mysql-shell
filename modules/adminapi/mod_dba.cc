/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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
std::set<std::string> Dba::_stop_instance_opts = {"sandboxDir", "password", "dbPassword"};
std::set<std::string> Dba::_default_local_instance_opts = {"sandboxDir"};
std::set<std::string> Dba::_create_cluster_opts = {"clusterAdminType", "multiMaster", "adoptFromGR", "force", "memberSslMode", "ipWhitelist"};
std::set<std::string> Dba::_reboot_cluster_opts = {"user", "dbUser", "password", "dbPassword", "removeInstances", "rejoinInstances"};

// Documentation of the DBA Class
REGISTER_HELP(DBA_BRIEF, "Enables you to administer InnoDB clusters using the AdminAPI.");
REGISTER_HELP(DBA_DETAIL, "The global variable 'dba' is used to access the AdminAPI functionality "\
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
  add_method("checkInstanceConfiguration", std::bind(&Dba::check_instance_configuration, this, _1), "data", shcore::Map, NULL);
  add_method("deploySandboxInstance", std::bind(&Dba::deploy_sandbox_instance, this, _1, "deploySandboxInstance"), "data", shcore::Map, NULL);
  add_method("startSandboxInstance", std::bind(&Dba::start_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("stopSandboxInstance", std::bind(&Dba::stop_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("deleteSandboxInstance", std::bind(&Dba::delete_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("killSandboxInstance", std::bind(&Dba::kill_sandbox_instance, this, _1), "data", shcore::Map, NULL);
  add_method("configureLocalInstance", std::bind(&Dba::configure_local_instance, this, _1), "data", shcore::Map, NULL);
  add_varargs_method("rebootClusterFromCompleteOutage", std::bind(&Dba::reboot_cluster_from_complete_outage, this, _1));
  add_varargs_method("help", std::bind(&Dba::help, this, _1));

  _metadata_storage.reset(new MetadataStorage(_shell_core->get_dev_session()));
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
REGISTER_HELP(DBA_GETCLUSTER_PARAM, "@param name Optional parameter to specify "\
                                    "the name of the cluster to be returned.");

REGISTER_HELP(DBA_GETCLUSTER_THROWS, "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS1, "@throws MetadataError if the Metadata update operation failed.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS2, "@throws ArgumentError if the Cluster name is empty.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS3, "@throws ArgumentError if the Cluster name is invalid.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS4, "@throws ArgumentError if the Cluster does not exist.");

REGISTER_HELP(DBA_GETCLUSTER_RETURNS, "@returns The cluster object identified "\
                                      " by the given name or the default "\
                                      " cluster.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL, "If name is not specified, the default "\
                                      "cluster will be returned.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL1, "If name is specified, and no cluster "\
                                      "with the indicated name is found, an "\
                                      "error will be raised.");

/**
* $(DBA_GETCLUSTER_BRIEF)
*
* $(DBA_GETCLUSTER_PARAM)
*
* $(DBA_GETCLUSTER_THROWS)
* $(DBA_GETCLUSTER_THROWS1)
* $(DBA_GETCLUSTER_THROWS2)
* $(DBA_GETCLUSTER_THROWS3)
* $(DBA_GETCLUSTER_THROWS4)
*
* $(DBA_GETCLUSTER_RETURNS)
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

  // Point the metadata session to the dba session
  _metadata_storage->set_session(get_active_session());

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
REGISTER_HELP(DBA_CREATECLUSTER_PARAM1, "@param options Optional dictionary "\
                                        "with options that modify the behavior "\
                                        "of this function.");

REGISTER_HELP(DBA_CREATECLUSTER_THROWS, "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS1, "@throws MetadataError if the Metadata update operation failed.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS2, "@throws ArgumentError if the Cluster name is empty.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS3, "@throws ArgumentError if the Cluster name is not valid.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS4, "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS5, "@throws ArgumentError if adoptFromGR "\
                                         "is true and the memberSslMode option "\
                                         "is used.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS6, "@throws ArgumentError if the value "\
                                         "for the memberSslMode option is not "\
                                         "one of the allowed.");

REGISTER_HELP(DBA_CREATECLUSTER_RETURNS, "@returns The created cluster object.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL, "Creates a MySQL InnoDB cluster taking "\
                                        "as seed instance the active global "\
                                        "session.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL1, "The options dictionary can contain the next values:");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL2, "@li clusterAdminType: defines the "\
                                         "type of management to be done on the "\
                                         "cluster instances.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL3, "@li multiMaster: boolean value used "\
                                         "to define an InnoDB cluster with "\
                                         "multiple writable instances.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL4, "@li force: boolean, confirms that "\
                                         "the multiMaster option must be "\
                                         "applied.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL5, "@li adoptFromGR: boolean value used "\
                                         "to create the InnoDB cluster based "\
                                         "on existing replication group.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL6, "@li memberSslMode: SSL mode used to "\
                                         "configure the members of the "\
                                         "cluster.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL7, "@li ipWhitelist: The list of hosts "\
                                         "allowed to connect to the instance "\
                                         "for group replication.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL8, "The values for clusterAdminType options include: local, manual, "\
                                         "guided or ssh, however, at the moment only "\
                                         "local is supported and is used as default value if this "\
                                         "attribute is not specified.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL9,"A InnoDB cluster may be setup in two ways:");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL10, "@li Single Master: One member of the cluster allows write "\
                                          "operations while the rest are in read only mode.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL11, "@li Multi Master: All the members "\
                                          "in the cluster support both read "\
                                          "and write operations.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL12, "By default this function create a Single Master cluster, use "\
                                          "the multiMaster option set to true "\
                                          "if a Multi Master cluster is required.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL13, "The memberSslMode option supports these values:");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL14, "@li REQUIRED: if used, SSL (encryption) will be enabled for the "\
                                          "instances to communicate with other members of the cluster");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL15, "@li DISABLED: if used, SSL (encryption) will be disabled");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL16, "@li AUTO: if used, SSL (encryption) "\
                                          "will be enabled if supported by the "\
                                          "instance, otherwise disabled");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL17, "If memberSslMode is not specified AUTO will be used by default.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL18, "The ipWhitelist format is a comma separated list of IP "\
                                          "addresses or subnet CIDR "\
                                          "notation, for example: 192.168.1.0/24,10.0.0.1. By default the "\
                                          "value is set to AUTOMATIC, allowing addresses "\
                                          "from the instance private network to be automatically set for "\
                                          "the whitelist.");

/**
 * $(DBA_CREATECLUSTER_BRIEF)
 *
 * $(DBA_CREATECLUSTER_PARAM)
 * $(DBA_CREATECLUSTER_PARAM1)
 *
 * $(DBA_CREATECLUSTER_THROWS)
 * $(DBA_CREATECLUSTER_THROWS1)
 * $(DBA_CREATECLUSTER_THROWS2)
 * $(DBA_CREATECLUSTER_THROWS3)
 * $(DBA_CREATECLUSTER_THROWS4)
 * $(DBA_CREATECLUSTER_THROWS5)
 * $(DBA_CREATECLUSTER_THROWS6)
 *
 * $(DBA_CREATECLUSTER_RETURNS)
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
 *
 * $(DBA_CREATECLUSTER_DETAIL8)
 *
 * $(DBA_CREATECLUSTER_DETAIL9)
 * $(DBA_CREATECLUSTER_DETAIL10)
 * $(DBA_CREATECLUSTER_DETAIL11)
 * $(DBA_CREATECLUSTER_DETAIL12)
 *
 * $(DBA_CREATECLUSTER_DETAIL13)
 * $(DBA_CREATECLUSTER_DETAIL14)
 * $(DBA_CREATECLUSTER_DETAIL15)
 * $(DBA_CREATECLUSTER_DETAIL16)
 * $(DBA_CREATECLUSTER_DETAIL17)
 *
 * $(DBA_CREATECLUSTER_DETAIL18)
 */
#if DOXYGEN_JS
Cluster Dba::createCluster(String name, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::create_cluster(str name, dict options) {}
#endif
shcore::Value Dba::create_cluster(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("createCluster").c_str());

  ReplicationGroupState state;

  // Point the metadata session to the dba session
  _metadata_storage->set_session(get_active_session());

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

      // Get current active session
      auto session = get_active_session();
      std::string nice_error = get_function_name("createCluster") + ": Unable to create cluster. " \
                               "The instance '"+ session-> address() +"' already belongs to an InnoDB cluster. Use " \
                               "<Dba>." + get_function_name("getCluster", false) + "() to access it.";
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
  std::string ssl_mode;

  std::string replication_user;
  std::string replication_pwd;
  std::string ip_whitelist;

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

      //Validate ip whitelist option
      validate_ip_whitelist_option(options);

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

      if (opt_map.has_key("memberSslMode")) {
        ssl_mode = opt_map.string_at("memberSslMode");
      }
      if (opt_map.has_key("ipWhitelist")) {
        // if the ipWhitelist option was provided, we know it is a valid value
        // since we've already done the validation above.
        ip_whitelist = opt_map.string_at("ipWhitelist");
      }
    }

    if (state.source_type == GRInstanceType::GroupReplication && !adopt_from_gr)
      throw Exception::argument_error("Creating a cluster on an unmanaged replication group requires adoptFromGR option to be true");

    auto session = get_active_session();

    // Check replication filters before creating the Metadata.
    auto classic = std::static_pointer_cast<mysql::ClassicSession>(session);
    validate_replication_filters(classic.get());

    // First we need to create the Metadata Schema, or update it if already exists
    _metadata_storage->create_metadata_schema();

    // Creates the replication account to for the primary instance
    _metadata_storage->create_repl_account(replication_user, replication_pwd);
    log_debug("Created replication user '%s'", replication_user.c_str());

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



    if (adopt_from_gr) { // TODO: move this to a GR specific class
      // check whether single_primary_mode is on
      auto result = classic->execute_sql("select @@group_replication_single_primary_mode");
      auto row = result->fetch_one();
      if (row) {
        log_info("Adopted cluster: group_replication_single_primary_mode=%s",
                 row->get_value_as_string(0).c_str());
        multi_master = row->get_value(0).as_int() == 0;
      }
    }

    Value::Map_type_ref instance_def(new shcore::Value::Map_type);
    Value::Map_type_ref options(new shcore::Value::Map_type);
    shcore::Argument_list new_args;
    instance_def = get_connection_data(session->uri(), false);

    std::string cnx_ssl_ca = session->get_ssl_ca();
    std::string cnx_ssl_cert = session->get_ssl_cert();
    std::string cnx_ssl_key = session->get_ssl_key();
    if (!cnx_ssl_ca.empty())
      (*instance_def)["sslCa"] = Value(cnx_ssl_ca);
    if (!cnx_ssl_cert.empty())
      (*instance_def)["sslCert"] = Value(cnx_ssl_cert);
    if (!cnx_ssl_key.empty())
      (*instance_def)["sslKey"] = Value(cnx_ssl_key);

    new_args.push_back(shcore::Value(instance_def));

    // Only set SSL option if available from createCluster options (not empty).
    // Do not set default values to avoid validation issues for addInstance.
    if (!ssl_mode.empty())
      (*options)["memberSslMode"] = Value(ssl_mode);

    // Set IP whitelist
    if (!ip_whitelist.empty())
      (*options)["ipWhitelist"] = Value(ip_whitelist);

    (*options)["password"] = Value(session->get_password());
    new_args.push_back(shcore::Value(options));

    if (multi_master && !(force || adopt_from_gr)) {
      throw shcore::Exception::argument_error("Use of multiMaster mode is not recommended unless you understand the limitations. Please use the 'force' option if you understand and accept them.");
    }

    cluster->add_seed_instance(new_args, multi_master, adopt_from_gr, replication_user, replication_pwd);

    // If it reaches here, it means there are no exceptions
    ret_val = Value(std::static_pointer_cast<Object_bridge>(cluster));

    if (adopt_from_gr)
      cluster->get_default_replicaset()->adopt_from_gr();

    tx.commit();
    // We catch whatever to do final processing before bubbling up the exception
  } catch (...) {
    try {
      if (!replication_user.empty()) {
        log_debug("Removing replication user '%s'", replication_user.c_str());
        _metadata_storage->execute_sql("DROP USER IF EXISTS " + replication_user);
      }

      throw;
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createCluster"));
  }

  return ret_val;
}

REGISTER_HELP(DBA_DROPMETADATASCHEMA_BRIEF, "Drops the Metadata Schema.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_PARAM, "@param options Dictionary "\
                                            "containing an option to confirm "\
                                            "the drop operation.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_THROWS, "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_RETURNS, "@returns nothing.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_DETAIL, "The options dictionary may contain the following options:");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_DETAIL1, "@li force: boolean, confirms that the drop operation must be executed.");

/**
* $(DBA_DROPMETADATASCHEMA_BRIEF)
*
* $(DBA_DROPMETADATASCHEMA_PARAM)
*
* $(DBA_DROPMETADATASCHEMA_THROWS)
*
* $(DBA_DROPMETADATASCHEMA_RETURNS)
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

  // Point the metadata session to the dba session
  _metadata_storage->set_session(get_active_session());

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
"active session.");

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

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_BRIEF, "Validates an instance for cluster usage.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_PARAM, "@param instance An instance definition.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_PARAM1, "@param options Optional data for the operation.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS, "@throws ArgumentError if the instance parameter is empty.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS1, "@throws ArgumentError if the instance definition is invalid.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS2, "@throws ArgumentError if the instance definition is a "\
                                                      "connection dictionary but empty.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS3, "@throws RuntimeError if the instance accounts are invalid.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS4, "@throws RuntimeError if the instance is offline.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS5, "@throws RuntimeError if the instance is already part of a "\
                                                      "Replication Group.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS6, "@throws RuntimeError if the instance is already part of an "\
                                                      "InnoDB Cluster.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_RETURNS, "@returns A JSON object with the status.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL, "This function reviews the instance configuration to identify if "\
                                                     "it is valid "\
                                                     "for usage in group replication.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL1, "The instance definition can be any of:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL2, "@li URI string.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL3, "@li Connection data dictionary.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL4, "A basic URI string has the following format:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL5, "[mysql://][user[:password]@]host[:port][?sslCa=...&sslCert=...&sslKey=...]");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL6, "The connection data dictionary may contain the following attributes:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL7, "@li user/dbUser: username");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL8, "@li password/dbPassword: username password");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL9, "@li host: hostname or IP address");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL10, "@li port: port number");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL11, "@li sslCat: the path to the X509 certificate authority in PEM format.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL12, "@li sslCert: The path to the X509 certificate in PEM format.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL13, "@li sslKey: The path to the X509 key in PEM format.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL14, "The options dictionary may contain the following options:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL15, "@li mycnfPath: The path of the MySQL configuration file for the instance.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL16, "@li password: The password to get connected to the instance.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL17, "@li clusterAdmin: The name of the InnoDB cluster administrator user.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL18, "@li clusterAdminPassword: The password for the InnoDB cluster "\
                                                       "administrator account.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL19, "The connection password may be contained on the instance "\
                                                       "definition, however, it can be overwritten "\
                                                       "if it is specified on the options.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL20, "The returned JSON object contains the following attributes:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL21, "@li status: the final status of the command, either \"ok\" or \"error\"");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL22, "@li config_errors: a list of dictionaries containing the failed requirements");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL23, "@li errors: a list of errors of the operation");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL24, "@li restart_required: a boolean value indicating whether a "\
                                                       "restart is required");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL25, "Each dictionary of the list of config_errors includes the "\
                                                       "following attributes:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL26, "@li option: The configuration option for which the requirement "\
                                                       "wasn't met");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL27, "@li current: The current value of the configuration option");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL28, "@li required: The configuration option required value");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL29, "@li action: The action to be taken in order to meet the requirement");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL30, "The action can be one of the following:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL31, "@li server_update+config_update: Both the server and the "\
                                                       "configuration need to be updated");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL32, "@li config_update+restart: The configuration needs to be "\
                                                       "updated and the server restarted");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL33, "@li config_update: The configuration needs to be updated");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL34, "@li server_update: The server needs to be updated");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL35, "@li restart: The server needs to be restarted");

/**
* $(DBA_CHECKINSTANCECONFIGURATION_BRIEF)
*
* $(DBA_CHECKINSTANCECONFIGURATION_PARAM)
* $(DBA_CHECKINSTANCECONFIGURATION_PARAM1)
*
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS)
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS1)
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS2)
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS3)
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS4)
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS5)
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS6)
*
* $(DBA_CHECKINSTANCECONFIGURATION_RETURNS)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL1)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL2)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL3)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL4)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL5)

* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL6)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL7)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL8)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL9)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL10)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL11)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL12)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL13)

* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL14)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL15)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL16)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL17)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL18)

* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL19)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL20)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL21)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL22)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL23)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL24)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL25)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL26)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL27)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL28)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL29)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL30)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL31)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL32)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL33)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL34)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL35)
*/
#if DOXYGEN_JS
Undefined Dba::checkInstanceConfiguration(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::check_instance_configuration(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::check_instance_configuration(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("checkInstanceConfiguration").c_str());

  shcore::Value ret_val;

  try {
    ret_val = shcore::Value(_check_instance_configuration(args, false));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("checkInstanceConfiguration"));

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
  bool ignore_ssl_error = true;  // SSL errors are ignored by default.

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
    } else if (function == "stop") {
      opt_map.ensure_keys({}, _stop_instance_opts, "the instance data");
      if (opt_map.has_key("password"))
        password = opt_map.string_at("password");
      else if (opt_map.has_key("dbPassword"))
        password = opt_map.string_at("dbPassword");
      else
        throw shcore::Exception::argument_error("Missing root password for the instance");
    }else {
      opt_map.ensure_keys({}, _default_local_instance_opts, "the instance data");
    }

    if (opt_map.has_key("sandboxDir")) {
      sandbox_dir = opt_map.string_at("sandboxDir");
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
  if (function == "deploy")
    // First we need to create the instance
    rc = _provisioning_interface->create_sandbox(port, portx, sandbox_dir, password, mycnf_options,
                                                 true, ignore_ssl_error, errors);
  else if (function == "delete")
      rc = _provisioning_interface->delete_sandbox(port, sandbox_dir, errors);
  else if (function == "kill")
    rc = _provisioning_interface->kill_sandbox(port, sandbox_dir, errors);
  else if (function == "stop")
    rc = _provisioning_interface->stop_sandbox(port, sandbox_dir, password, errors);
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
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_PARAM1, "@param options Optional dictionary with options affecting the "\
                                                "new deployed instance.");

REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS, "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS1, "@throws ArgumentError if the root password is missing on the options.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS2, "@throws ArgumentError if the port value is < 1024 or > 65535.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS3, "@throws RuntimeError f SSL "\
                                                 "support can be provided and "\
                                                 "ignoreSslError: false.");

// REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_RETURNS, "@returns The deployed
// Instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL, "This function will deploy a new MySQL Server instance, the result may be "\
                                                "affected by the provided options: ");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL1, "@li portx: port where the "\
                                                 "new instance will listen for "\
                                                 "X Protocol connections.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL2, "@li sandboxDir: path where the new instance will be deployed.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL3, "@li password: password for the MySQL root user on the new instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL4, "@li allowRootFrom: create remote root account, restricted to "\
                                                 "the given address pattern (eg %).");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL5, "@li ignoreSslError: Ignore errors when adding SSL support for the new "\
                                                 "instance, by default: true.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL6, "If the portx option is not specified, it will be automatically calculated "\
                                                 "as 10 times the value of the provided MySQL port.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL7, "The password or dbPassword options specify the MySQL root "\
                                                 "password on the new instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL8, "The sandboxDir must be an existing folder where the new instance will be "\
                                                 "deployed. If not specified the new instance will be deployed at:");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL9, "  ~/mysql-sandboxes on Unix-like systems or "\
                                                 "%userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL10, "SSL support is added by "\
              "default if not already available for the new instance, but if "\
              "it fails to be "\
              "added then the error is ignored. Set the ignoreSslError option "\
              "to false to ensure the new instance is "\
              "deployed with SSL support.");

/**
* $(DBA_DEPLOYSANDBOXINSTANCE_BRIEF)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_PARAM)
* $(DBA_DEPLOYSANDBOXINSTANCE_PARAM1)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_THROWS)
* $(DBA_DEPLOYSANDBOXINSTANCE_THROWS1)
* $(DBA_DEPLOYSANDBOXINSTANCE_THROWS2)
* $(DBA_DEPLOYSANDBOXINSTANCE_THROWS3)
*
* $(DBA_DEPLOYSANDBOXINSTANCE_RETURNS)
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

REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_THROWS, "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_THROWS1, "@throws ArgumentError if the port value is < 1024 or > 65535.");

REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL, "This function will delete an existing MySQL Server instance on the local host. The following options affect the result:");
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
* $(DBA_DELETESANDBOXINSTANCE_THROWS)
* $(DBA_DELETESANDBOXINSTANCE_THROWS1)
*
* $(DBA_DELETESANDBOXINSTANCE_RETURNS)
*
* $(DBA_DELETESANDBOXINSTANCE_DETAIL)
*
* $(DBA_DELETESANDBOXINSTANCE_DETAIL1)
* $(DBA_DELETESANDBOXINSTANCE_DETAIL2)
* $(DBA_DELETESANDBOXINSTANCE_DETAIL3)
*
* $(DBA_DELETESANDBOXINSTANCE_DETAIL4)
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

REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_THROWS, "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_THROWS1, "@throws ArgumentError if the port value is < 1024 or > 65535.");

REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL, "This function will kill the process of a running MySQL Server instance "\
                                              "on the local host. The following options affect the result:");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL1, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL2, "The sandboxDir must be the one where the MySQL instance was "\
                                               "deployed. If not specified it will use:");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL3, "  ~/mysql-sandboxes on Unix-like systems or "\
                                               "%userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL4, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_KILLSANDBOXINSTANCE_BRIEF)
*
* $(DBA_KILLSANDBOXINSTANCE_PARAM)
* $(DBA_KILLSANDBOXINSTANCE_PARAM1)
*
* $(DBA_KILLSANDBOXINSTANCE_THROWS)
* $(DBA_KILLSANDBOXINSTANCE_THROWS1)
*
* $(DBA_KILLSANDBOXINSTANCE_RETURNS)
*
* $(DBA_KILLSANDBOXINSTANCE_DETAIL)
*
* $(DBA_KILLSANDBOXINSTANCE_DETAIL1)
* $(DBA_KILLSANDBOXINSTANCE_DETAIL2)
* $(DBA_KILLSANDBOXINSTANCE_DETAIL3)
*
* $(DBA_KILLSANDBOXINSTANCE_DETAIL4)
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

REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_THROWS, "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_THROWS1, "@throws ArgumentError if the root password is missing on the options.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_THROWS2, "@throws ArgumentError if the port value is < 1024 or > 65535.");

REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL, "This function will gracefully stop a running MySQL Server instance "\
                                              "on the local host. The following options affect the result:");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL1, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL2, "@li password: password for the MySQL root user on the instance.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL3, "The sandboxDir must be the one where the MySQL instance was "\
                                               "deployed. If not specified it will use:");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL4, "  ~/mysql-sandboxes on Unix-like systems or "\
                                               "%userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL5, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_STOPSANDBOXINSTANCE_BRIEF)
*
* $(DBA_STOPSANDBOXINSTANCE_PARAM)
* $(DBA_STOPSANDBOXINSTANCE_PARAM1)
*
* $(DBA_STOPSANDBOXINSTANCE_THROWS)
* $(DBA_STOPSANDBOXINSTANCE_THROWS1)
* $(DBA_STOPSANDBOXINSTANCE_THROWS2)
*
* $(DBA_STOPSANDBOXINSTANCE_RETURNS)
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

REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_THROWS, "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_THROWS1, "@throws ArgumentError if the port value is < 1024 or > 65535.");

REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL, "This function will start an existing MySQL Server instance on the local "\
                                               "host. The following options affect the result:");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL1, "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL2, "The sandboxDir must be the one where the MySQL instance was "\
                                                "deployed. If not specified it will use:");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL3, "  ~/mysql-sandboxes on Unix-like systems or "\
                                                "%userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL4, "If the instance is not located on the used path an error will occur.");

/**
* $(DBA_STARTSANDBOXINSTANCE_BRIEF)
*
* $(DBA_STARTSANDBOXINSTANCE_PARAM)
* $(DBA_STARTSANDBOXINSTANCE_PARAM1)
*
* $(DBA_STARTSANDBOXINSTANCE_THROWS)
* $(DBA_STARTSANDBOXINSTANCE_THROWS1)
*
* $(DBA_STARTSANDBOXINSTANCE_RETURNS)
*
* $(DBA_STARTSANDBOXINSTANCE_DETAIL)
*
* $(DBA_STARTSANDBOXINSTANCE_DETAIL1)
* $(DBA_STARTSANDBOXINSTANCE_DETAIL2)
* $(DBA_STARTSANDBOXINSTANCE_DETAIL3)
*
* $(DBA_STARTSANDBOXINSTANCE_DETAIL4)
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

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_BRIEF, "Validates and configures an instance for cluster usage.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_PARAM, "@param instance An instance definition.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_PARAM1, "@param options Optional Additional options for the operation.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS, "@throws ArgumentError if the instance parameter is empty.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS1, "@throws ArgumentError if the instance definition is invalid.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS2, "@throws ArgumentError if the instance definition is a "\
                                                  "connection dictionary but empty.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS3, "@throws RuntimeError if the instance accounts are invalid.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS4, "@throws RuntimeError if the instance is offline.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS5, "@throws RuntimeError if the "\
                                                  "instance is already part of "\
                                                  "a Replication Group.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS6, "@throws RuntimeError if the "\
                                                  "instance is already part of "\
                                                  "an InnoDB Cluster.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_RETURNS, "@returns resultset A JSON object with the status.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL, "This function reviews the instance configuration to identify if "\
                                                 "it is valid "\
                                                 "for usage in group replication and cluster. A JSON object is "\
                                                 "returned containing the result of the operation.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL1, "The instance definition can be any of:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL2, "@li URI string.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL3, "@li Connection data dictionary.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL4, "A basic URI string has the following format:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL5, "[mysql://][user[:password]@]host[:port][?sslCa=...&sslCert=...&sslKey=...]");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL6, "The connection data dictionary may contain the following attributes:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL7, "@li user/dbUser: username");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL8, "@li password/dbPassword: username password");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL9, "@li host: hostname or IP address");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL10, "@li port: port number");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL11, "@li sslCat: the path to the X509 certificate authority in PEM format.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL12, "@li sslCert: The path to the X509 certificate in PEM format.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL13, "@li sslKey: The path to the X509 key in PEM format.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL14, "The options dictionary may contain the following options:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL15, "@li mycnfPath: The path to the MySQL configuration file of the instance.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL16, "@li password: The password to be used on the connection.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL17, "@li clusterAdmin: The name of the InnoDB cluster administrator "\
                                                   "user to be created. The supported format is the standard MySQL account name format.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL18, "@li clusterAdminPassword: The password for the InnoDB cluster administrator account.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL19, "The connection password may be contained on the instance "\
                                                   "definition, however, it can be overwritten "\
                                                   "if it is specified on the options.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL20, "The returned JSON object contains the following attributes:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL21, "@li status: the final status of the command, either \"ok\" or \"error\"");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL22, "@li config_errors: a list "\
                                                   "of dictionaries containing "\
                                                   "the failed requirements");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL23, "@li errors: a list of errors of the operation");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL24, "@li restart_required: a boolean value indicating whether a restart is required");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL25, "Each dictionary of the list of config_errors includes the "\
                                                   "following attributes:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL26, "@li option: The configuration option for which the requirement "\
                                                   "wasn't met");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL27, "@li current: The current value of the configuration option");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL28, "@li required: The configuration option required value");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL29, "@li action: The action to be taken in order to meet the requirement");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL30, "The action can be one of the following:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL31, "@li server_update+config_update: Both the server and the "\
                                                   "configuration need to be updated");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL32, "@li config_update+restart: The configuration needs to be "\
                                                   "updated and the server restarted");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL33, "@li config_update: The configuration needs to be updated");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL34, "@li server_update: The server needs to be updated");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL35, "@li restart: The server needs to be restarted");

/**
* $(DBA_CONFIGURELOCALINSTANCE_BRIEF)
*
* $(DBA_CONFIGURELOCALINSTANCE_PARAM)
* $(DBA_CONFIGURELOCALINSTANCE_PARAM1)
*
* $(DBA_CONFIGURELOCALINSTANCE_THROWS)
* $(DBA_CONFIGURELOCALINSTANCE_THROWS1)
* $(DBA_CONFIGURELOCALINSTANCE_THROWS2)
* $(DBA_CONFIGURELOCALINSTANCE_THROWS3)
* $(DBA_CONFIGURELOCALINSTANCE_THROWS4)
* $(DBA_CONFIGURELOCALINSTANCE_THROWS5)
* $(DBA_CONFIGURELOCALINSTANCE_THROWS6)
*
* $(DBA_CONFIGURELOCALINSTANCE_RETURNS)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL1)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL2)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL3)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL4)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL5)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL6)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL7)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL8)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL10)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL11)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL12)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL13)

* $(DBA_CONFIGURELOCALINSTANCE_DETAIL14)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL15)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL16)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL17)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL18)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL19)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL20)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL21)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL22)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL23)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL24)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL25)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL26)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL27)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL28)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL29)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL30)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL31)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL32)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL33)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL34)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL35)
*/
#if DOXYGEN_JS
Instance Dba::configureLocalInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
Instance Dba::configure_local_instance(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::configure_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, 2, get_function_name("configureLocalInstance").c_str());

  try {
    auto instance_def = get_instance_options_map(args, mysqlsh::dba::PasswordFormat::OPTIONS);

    shcore::Argument_map opt_map(*instance_def);

    if (shcore::is_local_host(opt_map.string_at("host"), true)) {
      ret_val = shcore::Value(_check_instance_configuration(args, true));
    } else
      throw shcore::Exception::runtime_error("This function only works with local instances");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("configureLocalInstance"));

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

shcore::Value::Map_type_ref Dba::_check_instance_configuration(const shcore::Argument_list &args, bool allow_update) {
  shcore::Value::Map_type_ref ret_val(new shcore::Value::Map_type());
  shcore::Value::Array_type_ref errors(new shcore::Value::Array_type());

  // Validates the connection options
  shcore::Value::Map_type_ref instance_def = get_instance_options_map(args, mysqlsh::dba::PasswordFormat::OPTIONS);

  // Resolves user and validates password
  resolve_instance_credentials(instance_def);

  shcore::Argument_map opt_map(*instance_def);
  shcore::Argument_map validate_opt_map;

  opt_map.ensure_keys({"host", "port"}, shcore::connection_attributes,
                      "instance definition");

  shcore::Value::Map_type_ref validate_options;

  std::string cnfpath, cluster_admin, cluster_admin_password;

  if (args.size() == 2) {
    validate_options = args.map_at(1);
    shcore::Argument_map tmp_map(*validate_options);

    std::set<std::string> check_options = {"password", "dbPassword", "mycnfPath",
                                           "clusterAdmin", "clusterAdminPassword"};

    tmp_map.ensure_keys({}, check_options, "validation options");
    validate_opt_map = tmp_map;

    // Retrieves the .cnf path if exists or leaves it empty so the default is set afterwards
    if (validate_opt_map.has_key("mycnfPath"))
      cnfpath = validate_opt_map.string_at("mycnfPath");

    // Retrives the clusterAdmin if exists or leaves it empty so the default is set afterwards
    if (validate_opt_map.has_key("clusterAdmin"))
      cluster_admin = validate_opt_map.string_at("clusterAdmin");

    // Retrieves the clusterAdminPassword if exists or leaves it empty so the default is set afterwards
    if (validate_opt_map.has_key("clusterAdminPassword"))
      cluster_admin_password = validate_opt_map.string_at("clusterAdminPassword");
  }

  if (cnfpath.empty() && allow_update)
    throw shcore::Exception::runtime_error("The path to the MySQL Configuration is required to verify and fix the InnoDB Cluster settings");

  // Now validates the instance GR status itself
  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_def));
  auto session = Dba::get_session(new_args);

  std::string uri = session->uri();

  GRInstanceType type = get_gr_instance_type(session->connection());

  if (type == GRInstanceType::GroupReplication) {
    session->close(shcore::Argument_list());
    throw shcore::Exception::runtime_error("The instance '" + uri + "' is already part of a Replication Group");
  }
  // configureLocalInstance is allowed even if the instance is part of the InnoDB cluster
  // checkInstanceConfiguration is not
  else if (type == GRInstanceType::InnoDBCluster && !allow_update) {
    session->close(shcore::Argument_list());
    throw shcore::Exception::runtime_error("The instance '" + uri + "' is already part of an InnoDB Cluster");
  }
  else {
    if (!cluster_admin.empty() && allow_update) {
      try {
        create_cluster_admin_user(session,
                                  cluster_admin,
                                  cluster_admin_password);
      } catch (shcore::Exception &err) {
        // Catch ER_CANNOT_USER (1396) if the user already exists, and skip it
        // if the user has the needed privileges.
        if (err.code() == ER_CANNOT_USER) {
          std::string admin_user, admin_user_host;
          shcore::split_account(cluster_admin, &admin_user, &admin_user_host);
          // Host '%' is used by default if not provided in the user account.
          if (admin_user_host.empty())
            admin_user_host = "%";
          if (!validate_cluster_admin_user_privileges(session, admin_user,
                                                      admin_user_host)) {
            std::string error_msg =
                "User " + cluster_admin + " already exists but it does not "
                "have all the privileges for managing an InnoDB cluster. "
                "Please provide a non-existing user to be created or a "
                "different one with all the required privileges.";
            errors->push_back(shcore::Value(error_msg));
            log_error("%s", error_msg.c_str());
          } else {
            log_warning("User %s already exists.", cluster_admin.c_str());
          }
        } else {
          std::string error_msg = "Unexpected error creating " + cluster_admin +
                                  " user: " + err.what();
          errors->push_back(shcore::Value(error_msg));
          log_error("%s", error_msg.c_str());
        }
      }
    }

    std::string host = instance_def->get_string("host");
    int port = instance_def->get_int("port");
    std::string endpoint = host + ":" + std::to_string(port);

    if (type == GRInstanceType::InnoDBCluster) {
      auto seeds = get_peer_seeds(session->connection(), endpoint);
      auto peer_seeds = shcore::join_strings(seeds, ",");
      set_global_variable(session->connection(), "group_replication_group_seeds", peer_seeds);
    }

    std::string user;
    std::string password;
    user = instance_def->get_string(instance_def->has_key("user") ? "user" : "dbUser");
    password = instance_def->get_string(instance_def->has_key("password") ? "password" : "dbPassword");

    // Get SSL values to connect to instance
    Value::Map_type_ref instance_ssl_opts(new shcore::Value::Map_type);
    if (instance_def->has_key("sslCa"))
      (*instance_ssl_opts)["sslCa"] = Value(instance_def->get_string("sslCa"));
    if (instance_def->has_key("sslCert"))
      (*instance_ssl_opts)["sslCert"] = Value(instance_def->get_string("sslCert"));
    if (instance_def->has_key("sslKey"))
      (*instance_ssl_opts)["sslKey"] = Value(instance_def->get_string("sslKey"));

    // Verbose is mandatory for checkInstanceConfiguration
    shcore::Value::Array_type_ref mp_errors;

    if ((_provisioning_interface->check(user, host, port, password, instance_ssl_opts, cnfpath, allow_update, mp_errors) == 0)) {
      // Only return status "ok" if no previous errors were found.
      if (errors->size() == 0) {
        (*ret_val)["status"] = shcore::Value("ok");
      } else {
        (*ret_val)["status"] = shcore::Value("error");
        (*ret_val)["errors"] = shcore::Value(errors);
        (*ret_val)["restart_required"] = shcore::Value(false);
      }
    } else {
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
                  std::vector<std::string> dynamic_variables = {
                      "binlog_format", "binlog_checksum", "slave_parallel_type",
                      "slave_preserve_commit_order"};

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

REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_BRIEF, "Brings a cluster back ONLINE when all members are OFFLINE.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_PARAM, "@param clusterName Optional The name of the cluster to be rebooted.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_PARAM1, "@param options Optional dictionary with options that modify the "\
                                                          "behavior of this function.");

REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS, "@throws MetadataError  if the Metadata is inaccessible.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS1, "@throws ArgumentError if the Cluster name is empty.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS2, "@throws ArgumentError if the Cluster name is not valid.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS3, "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS4, "@throws RuntimeError if the Cluster does not exist on the Metadata.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS5, "@throws RuntimeError if some instance of the Cluster belongs to "\
                                                           "a Replication Group.");

REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_RETURNS, "@returns The rebooted cluster object.");

REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL, "The options dictionary can contain the next values:");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL1, "@li password: The password used for the instances sessions "\
                                                           "required operations.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL2, "@li removeInstances: The list of instances to be removed from "\
                                                           "the cluster.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL3, "@li rejoinInstances: The list of instances to be rejoined on "\
                                                           "the cluster.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL4, "This function reboots a cluster from complete outage. "\
                                                           "It picks the instance the MySQL Shell is connected to as new "\
                                                           "seed instance and recovers the cluster. "\
                                                           "Optionally it also updates the cluster configuration based on "\
                                                           "user provided options.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL5, "On success, the restored cluster object is returned by the function.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL6, "The current session must be connected to a former instance of the cluster.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL7, "If name is not specified, the default cluster will be returned.");

/**
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_BRIEF)
*
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_PARAM)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_PARAM1)
*
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS1)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS2)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS3)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS4)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS5)
*
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_RETURNS)
*
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL1)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL2)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL3)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL4)
*
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL5)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL6)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL7)
*/
#if DOXYGEN_JS
Undefined Dba::rebootClusterFromCompleteOutage(String clusterName, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::reboot_cluster_from_complete_outage(str clusterName, dict options) {}
#endif

shcore::Value Dba::reboot_cluster_from_complete_outage(const shcore::Argument_list &args) {
  args.ensure_count(0, 2, get_function_name("rebootClusterFromCompleteOutage").c_str());

  // Point the metadata session to the dba session
  _metadata_storage->set_session(get_active_session());

  shcore::Value ret_val;
  bool default_cluster = false;
  std::string cluster_name, password, user, group_replication_group_name,
              port, host, instance_session_address;
  shcore::Value::Map_type_ref options;
  std::shared_ptr<mysqlsh::dba::Cluster> cluster;
  std::shared_ptr<mysqlsh::dba::ReplicaSet> default_replicaset;
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> session;
  Value::Array_type_ref remove_instances_ref, rejoin_instances_ref;
  std::vector<std::string> remove_instances_list, rejoin_instances_list,
                           instances_lists_intersection;
  std::shared_ptr<shcore::Value::Array_type> online_instances;

  check_preconditions("rebootClusterFromCompleteOutage");

  try {
    if (args.size() == 0) {
      default_cluster = true;
    } else if (args.size() == 1) {
      cluster_name = args.string_at(0);
    } else {
      cluster_name = args.string_at(0);
      options = args.map_at(1);
    }

    // get the current session information
    auto instance_session(_metadata_storage->get_session());

    Value::Map_type_ref current_session_options = get_connection_data(instance_session->uri(), false);

    // Get the current session instance address
    port = std::to_string(current_session_options->get_int("port"));
    host = current_session_options->get_string("host");
    instance_session_address = host + ":" + port;

    if (options) {
      shcore::Argument_map opt_map(*options);

      if (opt_map.has_key("removeInstances"))
        remove_instances_ref = opt_map.array_at("removeInstances");

      if (opt_map.has_key("rejoinInstances"))
        rejoin_instances_ref = opt_map.array_at("rejoinInstances");

      // Check if the password is specified on the options and if not prompt it
      if (opt_map.has_key("password"))
        password = opt_map.string_at("password");
      else if (opt_map.has_key("dbPassword"))
        password = opt_map.string_at("dbPassword");
      else
        password = instance_session->get_password();

      // check if the user is specified on the options and it not prompt it
      if (opt_map.has_key("user"))
        user = opt_map.string_at("user");
      else if (opt_map.has_key("dbUser"))
        user = opt_map.string_at("dbUser");
      else
        user = instance_session->get_user();

    } else {
      user = instance_session->get_user();
      password = instance_session->get_password();
    }

    // Check if removeInstances and/or rejoinInstances are specified
    // And if so add them to simple vectors so the check for types is done
    // before moving on in the function logic
    if (remove_instances_ref) {
      for (auto value : *remove_instances_ref.get()) {
        // Check if seed instance is present on the list
        if (value.as_string() == instance_session_address)
          throw shcore::Exception::argument_error("The current session instance "
                "cannot be used on the 'removeInstances' list.");

        remove_instances_list.push_back(value.as_string());
      }
    }

    if (rejoin_instances_ref) {
      for (auto value : *rejoin_instances_ref.get()) {
        // Check if seed instance is present on the list
        if (value.as_string() == instance_session_address)
          throw shcore::Exception::argument_error("The current session instance "
                "cannot be used on the 'rejoinInstances' list.");
        rejoin_instances_list.push_back(value.as_string());
      }
    }

    // Check if there is an intersection of the two lists.
    // Sort the vectors because set_intersection works on sorted collections
    std::sort(remove_instances_list.begin(), remove_instances_list.end());
    std::sort(rejoin_instances_list.begin(), rejoin_instances_list.end());

    std::set_intersection(remove_instances_list.begin(), remove_instances_list.end(),
                          rejoin_instances_list.begin(), rejoin_instances_list.end(),
                          std::back_inserter(instances_lists_intersection));

    if (!instances_lists_intersection.empty()) {
      std::string list;

      list = shcore::join_strings(instances_lists_intersection, ", ");

      throw shcore::Exception::argument_error("The following instances: '" + list +
                "' belong to both 'rejoinInstances' and 'removeInstances' lists.");
    }

    // Getting the cluster from the metadata already complies with:
    // 1. Ensure that a Metadata Schema exists on the current session instance.
    // 3. Ensure that the provided cluster identifier exists on the Metadata Schema
    if (default_cluster) {
      cluster = _metadata_storage->get_default_cluster();
    } else {
      if (cluster_name.empty())
        throw Exception::argument_error("The cluster name cannot be empty.");

      if (!shcore::is_valid_identifier(cluster_name))
        throw Exception::argument_error("The cluster name must be a valid identifier.");

      cluster = _metadata_storage->get_cluster(cluster_name);
    }

    if (cluster) {
      // Set the provision interface pointer
      cluster->set_provisioning_interface(_provisioning_interface);

      // Set the cluster as return value
      ret_val = shcore::Value(std::dynamic_pointer_cast<Object_bridge>(cluster));

      // Get the default replicaset
      default_replicaset = cluster->get_default_replicaset();

      // 2. Ensure that the current session instance exists on the Metadata Schema
      if (!_metadata_storage->is_instance_on_replicaset(
            default_replicaset->get_id(), instance_session_address))
        throw Exception::runtime_error("The current session instance does not belong "
                                       "to the cluster: '" + cluster->get_name() + "'.");

      // Ensure that all of the instances specified on the 'removeInstances' list exist
      // on the Metadata Schema and are valid
      for (const auto &value : remove_instances_list) {
        shcore::Argument_list args;
        args.push_back(shcore::Value(value));

        try {
          auto instance_def = mysqlsh::dba::get_instance_options_map(args, mysqlsh::dba::PasswordFormat::NONE);
        }
        catch (std::exception &e) {
          std::string error(e.what());
          throw shcore::Exception::argument_error("Invalid value '" + value + "' for 'removeInstances': " +
                                                  error);
        }

        if (!_metadata_storage->is_instance_on_replicaset(default_replicaset->get_id(), value))
          throw Exception::runtime_error("The instance '" + value + "' does not belong "
                                         "to the cluster: '" + cluster->get_name() + "'.");
      }

      // Ensure that all of the instances specified on the 'rejoinInstances' list exist
      // on the Metadata Schema and are valid
      for (const auto &value : rejoin_instances_list) {
        shcore::Argument_list args;
        args.push_back(shcore::Value(value));

        try {
          auto instance_def = mysqlsh::dba::get_instance_options_map(args, mysqlsh::dba::PasswordFormat::NONE);
        }
        catch (std::exception &e) {
          std::string error(e.what());
          throw shcore::Exception::argument_error("Invalid value '" + value + "' for 'rejoinInstances': " +
                                                  error);
        }

        if (!_metadata_storage->is_instance_on_replicaset(default_replicaset->get_id(), value))
          throw Exception::runtime_error("The instance '" + value + "' does not belong "
                                         "to the cluster: '" + cluster->get_name() + "'.");

      }
      // Get the all the instances and their status
      std::vector<std::pair<std::string, std::string>> instances_status =
          get_replicaset_instances_status(&cluster_name, options);

      std::vector<std::string> non_reachable_rejoin_instances,
          non_reachable_instances;

      // get all non reachable instances
      for (auto &instance : instances_status) {
        if (!(instance.second.empty())) {
          non_reachable_instances.push_back(instance.first);
        }
      }
      // get all the list of non-reachable instances that were specified on the
      // rejoinInstances list.
      // Sort non_reachable_instances vector because set_intersection works on
      // sorted collections
      // The rejoin_instances_list vector was already sorted above.
      std::sort(non_reachable_instances.begin(), non_reachable_instances.end());

      std::set_intersection(
          non_reachable_instances.begin(), non_reachable_instances.end(),
          rejoin_instances_list.begin(), rejoin_instances_list.end(),
          std::back_inserter(non_reachable_rejoin_instances));

      if (!non_reachable_rejoin_instances.empty()) {
        std::string list;

        list = shcore::join_strings(non_reachable_rejoin_instances, ", ");

        throw std::runtime_error("The following instances: '" + list +
                                 "' were specified in the rejoinInstances list "
                                 "but are not reachable.");
      }
    } else {
      std::string message;
      if (default_cluster)
        message = "No default cluster is configured.";
      else
        message = "The cluster '" + cluster_name + "' is not configured.";

      throw shcore::Exception::logic_error(message);
    }

    // Check if the cluster is empty
    if (_metadata_storage->is_cluster_empty(cluster->get_id()))
      throw Exception::runtime_error("The cluster is empty.");

    // 4. Verify the status of all instances of the cluster:
    // 4.1 None of the instances can belong to a GR Group
    // 4.2 If any of the instances belongs to a GR group or is already managed by the
    // InnoDB Cluster, so include that information on the error message
    validate_instances_status_reboot_cluster(args);

    // 5. Verify which of the online instances has the GTID superset.
    // 5.1 Skip the verification on the list of instances to be removed: "removeInstances"
    // 5.2 If the current session instance doesn't have the GTID superset, error out
    // with that information and including on the message the instance with the GTID superset
    validate_instances_gtid_reboot_cluster(&cluster_name, options, instance_session);

    // Get the group_replication_group_name
    group_replication_group_name = _metadata_storage->get_replicaset_group_name();

    // 6. Set the current session instance as the seed instance of the Cluster
    {
      shcore::Argument_list new_args;
      std::string replication_user, replication_user_password;

      (*current_session_options)["user"] = Value(user);
      (*current_session_options)["password"] = Value(password);

      new_args.push_back(shcore::Value(current_session_options));

      // A new replication user and password must be created
      // so we pass an empty string to the MP call
      replication_user = "";
      replication_user_password = "";

      default_replicaset->add_instance(new_args, replication_user, replication_user_password, true);
    }

    // 7. Update the Metadata Schema information
    // 7.1 Remove the list of instances of "removeInstances" from the Metadata
    default_replicaset->remove_instances(remove_instances_list);

    // 8. Rejoin the list of instances of "rejoinInstances"
    default_replicaset->rejoin_instances(rejoin_instances_list, options);

    // check if @@group_replication_group_name changes after the reboot and
    // if so, update the metadata accordingly
    {
      std::string current_group_replication_group_name = _metadata_storage->get_replicaset_group_name();

      if (current_group_replication_group_name != group_replication_group_name)
        _metadata_storage->set_replicaset_group_name(default_replicaset, current_group_replication_group_name);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rebootClusterFromCompleteOutage"));

  return ret_val;
}

ReplicationGroupState Dba::check_preconditions(const std::string& function_name) const {
  // Point the metadata session to the dba session
  _metadata_storage->set_session(get_active_session());
  return check_function_preconditions(class_name(), function_name, get_function_name(function_name), _metadata_storage);
}

/*
 * get_replicaset_instances_status:
 *
 * Given a cluster id, this function verifies the connectivity status of all the instances
 * of the default replicaSet of the cluster. It returns a list of pairs <instance_id, status>,
 * on which 'status' is empty if the instance is reachable, or if not reachable contains the
 * connection failure error message
 */
std::vector<std::pair<std::string, std::string>> Dba::get_replicaset_instances_status(std::string *out_cluster_name,
          const shcore::Value::Map_type_ref &options) {
  std::vector<std::pair<std::string, std::string>> instances_status;
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> session;
  std::string user, password, host, port, active_session_address, instance_address, conn_status;

  // Point the metadata session to the dba session
  _metadata_storage->set_session(get_active_session());

  if (out_cluster_name->empty())
    *out_cluster_name = _metadata_storage->get_default_cluster()->get_name();

  std::shared_ptr<Cluster> cluster = _metadata_storage->get_cluster(*out_cluster_name);
  uint64_t rs_id = cluster->get_default_replicaset()->get_id();
  std::shared_ptr<shcore::Value::Array_type> instances = _metadata_storage->get_replicaset_instances(rs_id);

  // get the current session information
  auto instance_session(_metadata_storage->get_session());

  Value::Map_type_ref current_session_options = get_connection_data(instance_session->uri(), false);

  // Get the current session instance address
  port = std::to_string(current_session_options->get_int("port"));
  host = current_session_options->get_string("host");
  active_session_address = host + ":" + port;

  if (options) {
    shcore::Argument_map opt_map(*options);

    // Check if the password is specified on the options and if not prompt it
    if (opt_map.has_key("password"))
      password = opt_map.string_at("password");
    else if (opt_map.has_key("dbPassword"))
      password = opt_map.string_at("dbPassword");
    else
      password = instance_session->get_password();

    // check if the user is specified on the options and it not prompt it
    if (opt_map.has_key("user"))
      user = opt_map.string_at("user");
    else if (opt_map.has_key("dbUser"))
      user = opt_map.string_at("dbUser");
    else
      user = instance_session->get_user();

  } else {
    user = instance_session->get_user();
    password = instance_session->get_password();
  }

  // Iterate on all instances from the metadata
  for (auto it = instances->begin(); it != instances->end(); ++it) {
    auto row = it->as_object<mysqlsh::Row>();
    instance_address = row->get_member("host").as_string();
    conn_status.clear();
    session = NULL;

    // Skip the current session instance
    if (instance_address == active_session_address) {
      continue;
    }

    shcore::Argument_list session_args;
    Value::Map_type_ref instance_options(new shcore::Value::Map_type);

    shcore::Value::Map_type_ref connection_data = shcore::get_connection_data(instance_address);

    int instance_port = connection_data->get_int("port");
    std::string instance_host = connection_data->get_string("host");

    (*instance_options)["host"] = shcore::Value(instance_host);
    (*instance_options)["port"] = shcore::Value(instance_port);
    // We assume the root password is the same on all instances
    (*instance_options)["password"] = shcore::Value(password);
    (*instance_options)["user"] = shcore::Value(user);
    session_args.push_back(shcore::Value(instance_options));

    try {
      log_info("Opening a new session to the instance to determine its status: %s",
                instance_address.c_str());
      session = mysqlsh::connect_session(session_args, mysqlsh::SessionType::Classic);
      session->close(shcore::Argument_list());
    } catch (std::exception &e) {
      conn_status = e.what();
      log_warning("Could not open connection to %s: %s.", instance_address.c_str(), e.what());
    }

    // Add the <instance, connection_status> pair to the list
    instances_status.emplace_back(instance_address, conn_status);
  }

  return instances_status;
}

/*
 * validate_instances_status_reboot_cluster:
 *
 * This function is an auxiliary function to be used for the reboot_cluster operation.
 * It verifies the status of all the instances of the cluster referent to the arguments list.
 * Firstly, it verifies the status of the current session instance to determine if it belongs
 * to a GR group or is already managed by the InnoDB Cluster.cluster_name
 * If not, does the same validation for the remaining reachable instances of the cluster.
 */
void Dba::validate_instances_status_reboot_cluster(const shcore::Argument_list &args) {
  std::string cluster_name, user, password, port, host, active_session_address;
  shcore::Value::Map_type_ref options;
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> session;
  mysqlsh::mysql::ClassicSession *classic_current;

  if (args.size() == 1)
    cluster_name = args.string_at(0);
  if (args.size() > 1) {
    cluster_name = args.string_at(0);
    options = args.map_at(1);
  }

  // Point the metadata session to the dba session
  _metadata_storage->set_session(get_active_session());

  // get the current session information
  auto instance_session(_metadata_storage->get_session());
  classic_current = dynamic_cast<mysqlsh::mysql::ClassicSession*>(instance_session.get());

  Value::Map_type_ref current_session_options = get_connection_data(instance_session->uri(), false);

  // Get the current session instance address
  port = std::to_string(current_session_options->get_int("port"));
  host = current_session_options->get_string("host");
  active_session_address = host + ":" + port;

  if (options) {
    shcore::Argument_map opt_map(*options);

    opt_map.ensure_keys({}, mysqlsh::dba::Dba::_reboot_cluster_opts, "the options");

    // Check if the password is specified on the options and if not prompt it
    if (opt_map.has_key("password"))
      password = opt_map.string_at("password");
    else if (opt_map.has_key("dbPassword"))
      password = opt_map.string_at("dbPassword");
    else
      password = instance_session->get_password();

    // check if the user is specified on the options and it not prompt it
    if (opt_map.has_key("user"))
      user = opt_map.string_at("user");
    else if (opt_map.has_key("dbUser"))
      user = opt_map.string_at("dbUser");
    else
      user = instance_session->get_user();

  } else {
    user = instance_session->get_user();
    password = instance_session->get_password();
  }

  GRInstanceType type = get_gr_instance_type(classic_current->connection());

  switch (type) {
    case GRInstanceType::InnoDBCluster:
      throw Exception::runtime_error("The cluster's instance '" + active_session_address + "' belongs "
                                       "to an InnoDB Cluster and is reachable. Please use <Cluster>." +
                                       get_member_name("forceQuorumUsingPartitionOf", naming_style) +
                                       "() to restore the quorum loss.");

    case GRInstanceType::GroupReplication:
      throw Exception::runtime_error("The cluster's instance '" + active_session_address + "' belongs "
                                     "to an unmanaged GR group. ");

    case Standalone:
      // We only want to check whether the status if InnoDBCluster or GroupReplication to stop and thrown
      // an exception
      break;
  }

  // Verify all the remaining online instances for their status
  std::vector<std::pair<std::string, std::string>> instances_status =
          get_replicaset_instances_status(&cluster_name, options);

  for (auto &value : instances_status) {
    mysqlsh::mysql::ClassicSession *classic;

    std::string instance_address = value.first;
    std::string instance_status = value.second;

    // if the status is not empty it means the connection failed
    // so we skip this instance
    if (!instance_status.empty())
      continue;

    shcore::Argument_list session_args;
    Value::Map_type_ref instance_options(new shcore::Value::Map_type);

    shcore::Value::Map_type_ref connection_data = shcore::get_connection_data(instance_address);

    int instance_port = connection_data->get_int("port");
    std::string instance_host = connection_data->get_string("host");

    (*instance_options)["host"] = shcore::Value(instance_host);
    (*instance_options)["port"] = shcore::Value(instance_port);
    // We assume the root password is the same on all instances
    (*instance_options)["password"] = shcore::Value(password);
    (*instance_options)["user"] = shcore::Value(user);
    session_args.push_back(shcore::Value(instance_options));

    try {
      log_info("Opening a new session to the instance: %s",
                instance_address.c_str());
      session = mysqlsh::connect_session(session_args, mysqlsh::SessionType::Classic);
      classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
    } catch (std::exception &e) {
      throw Exception::runtime_error("Could not open connection to " + instance_address + "");
    }

    GRInstanceType type = get_gr_instance_type(classic->connection());

    // Close the session
    session->close(shcore::Argument_list());

    switch (type) {
      case GRInstanceType::InnoDBCluster:
        throw Exception::runtime_error("The cluster's instance '" + instance_address + "' belongs "
                                       "to an InnoDB Cluster and is reachable. Please use " +
                                       get_member_name("forceQuorumUsingPartitionOf", naming_style) +
                                       "() to restore the quorum loss.");

      case GRInstanceType::GroupReplication:
        throw Exception::runtime_error("The cluster's instance '" + instance_address + "' belongs "
                                       "to an unmanaged GR group. ");

      case Standalone:
        // We only want to check whether the status if InnoDBCluster or GroupReplication to stop and thrown
        // an exception
        break;
    }
  }
}

/*
 * validate_instances_gtid_reboot_cluster:
 *
 * This function is an auxiliary function to be used for the reboot_cluster operation.
 * It verifies which of the online instances of the cluster has the GTID superset.
 * If the current session instance doesn't have the GTID superset, it errors out with that information
 * and includes on the error message the instance with the GTID superset
 */
void Dba::validate_instances_gtid_reboot_cluster(std::string *out_cluster_name,
                                                 const shcore::Value::Map_type_ref &options,
                                                 const std::shared_ptr<ShellDevelopmentSession> &instance_session) {
  /* GTID verification is done by verifying which instance has the GTID superset.the
   * In order to do so, a union of the global gtid executed and the received transaction
   * set must be done using:
   *
   * CREATE FUNCTION GTID_UNION(g1 TEXT, g2 TEXT)
   *  RETURNS TEXT DETERMINISTIC
   *  RETURN CONCAT(g1,',',g2);
   *
   * Instance 1:
   *
   * A: select @@GLOBAL.GTID_EXECUTED
   * B: SELECT RECEIVED_TRANSACTION_SET FROM
   *    performance_schema.replication_connection_status where CHANNEL_NAME="group_replication_applier";
   *
   * Total = A + B (union)
   *
   * SELECT GTID_SUBSET("Total_instance1", "Total_instance2")
   */

  std::pair<std::string, std::string> most_updated_instance;
  mysqlsh::mysql::ClassicSession *classic_current;
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> session;
  std::string host, port, active_session_address, user, password;

  // get the current session information
  classic_current = dynamic_cast<mysqlsh::mysql::ClassicSession*>(instance_session.get());

  Value::Map_type_ref current_session_options = get_connection_data(instance_session->uri(), false);

  // Get the current session instance address
  port = std::to_string(current_session_options->get_int("port"));
  host = current_session_options->get_string("host");
  active_session_address = host + ":" + port;

  if (options) {
    shcore::Argument_map opt_map(*options);

    if (opt_map.has_key("password"))
      password = opt_map.string_at("password");
    else if (opt_map.has_key("dbPassword"))
      password = opt_map.string_at("dbPassword");
    else
      password = instance_session->get_password();

    if (opt_map.has_key("user"))
      user = opt_map.string_at("user");
    else if (opt_map.has_key("dbUser"))
      user = opt_map.string_at("dbUser");
    else
      user = instance_session->get_user();

  } else {
    user = instance_session->get_user();
    password = instance_session->get_password();
  }

  // Get the cluster instances and their status
  std::vector<std::pair<std::string, std::string>> instances_status =
        get_replicaset_instances_status(out_cluster_name, options);

  // Get @@GLOBAL.GTID_EXECUTED
  std::string gtid_executed_current;
  get_server_variable(classic_current->connection(), "GLOBAL.GTID_EXECUTED", gtid_executed_current);

  std::string msg = "The current session instance GLOBAL.GTID_EXECUTED is: " + gtid_executed_current;
  log_info("%s", msg.c_str());

  // Create a pair vector to store all the GTID_EXECUTED
  std::vector<std::pair<std::string, std::string>> gtids;

  // Insert the current session info
  gtids.emplace_back(active_session_address, gtid_executed_current);

  // Update most_updated_instance with the current session instance value
  most_updated_instance = std::make_pair(active_session_address, gtid_executed_current);

  for (auto &value : instances_status) {
    mysqlsh::mysql::ClassicSession *classic;
    std::string instance_address = value.first;
    std::string instance_status = value.second;

    // if the status is not empty it means the connection failed
    // so we skip this instance
    if (!instance_status.empty())
      continue;

    shcore::Argument_list session_args;
    Value::Map_type_ref instance_options(new shcore::Value::Map_type);

    shcore::Value::Map_type_ref connection_data = shcore::get_connection_data(instance_address);

    int instance_port = connection_data->get_int("port");
    std::string instance_host = connection_data->get_string("host");

    (*instance_options)["host"] = shcore::Value(instance_host);
    (*instance_options)["port"] = shcore::Value(instance_port);
    // We assume the root password is the same on all instances
    (*instance_options)["password"] = shcore::Value(password);
    (*instance_options)["user"] = shcore::Value(user);
    session_args.push_back(shcore::Value(instance_options));

    // Connect to the instance to obtain the GLOBAL.GTID_EXECUTED
    try {
      log_info("Opening a new session to the instance for gtid validations %s",
                instance_address.c_str());
      session = mysqlsh::connect_session(session_args, mysqlsh::SessionType::Classic);
      classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
    } catch (std::exception &e) {
      throw Exception::runtime_error("Could not open a connection to " +
                                      instance_address + ": " + e.what() +
                                      ".");
    }

    std::string gtid_executed;

    // Get @@GLOBAL.GTID_EXECUTED
    get_server_variable(classic->connection(), "GLOBAL.GTID_EXECUTED", gtid_executed);

    // Close the session
    session->close(shcore::Argument_list());

    std::string msg = "The instance: '" + instance_address + "' GLOBAL.GTID_EXECUTED is: " + gtid_executed;
    log_info("%s", msg.c_str());

    // Add to the pair vector of gtids
    gtids.emplace_back(instance_address, gtid_executed);
  }

  // Calculate the most up-to-date instance
  // TODO: calculate the Total GTID executed. See comment above
  for (auto &value : gtids) {
    // Compare the gtid's: SELECT GTID_SUBSET("Total_instance1", "Total_instance2")
    if (!is_gtid_subset(classic_current->connection(), value.second, most_updated_instance.second))
      most_updated_instance = value;
  }

  // Check if the most updated instance is not the current session instance
  if (!(most_updated_instance.first == active_session_address)) {
    throw Exception::runtime_error("The active session instance isn't the most updated "
                                   "in comparison with the ONLINE instances of the Cluster's "
                                   "metadata. Please use the most up to date instance: '" +
                                   most_updated_instance.first + "'.");
  }
}
