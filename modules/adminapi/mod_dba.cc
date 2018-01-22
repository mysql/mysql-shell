/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <mysqld_error.h>

#ifndef WIN32
#include <sys/un.h>
#endif
#include "modules/adminapi/mod_dba.h"

#include <iterator>
#include <utility>
#include <vector>

#include "utils/utils_sqlstring.h"
#include "modules/mysqlxtest_utils.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_shell.h"
#include "scripting/object_factory.h"
#include "shellcore/utils_help.h"
#include "utils/utils_general.h"
#include "modules/mod_utils.h"

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/innodbcluster/cluster.h"

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"

/*
  Sessions used by AdminAPI
  =========================

  The following is a list of the types of MySQL sessions that exist in the
  AdminAPI. These are conceptual types of sessions and don't necessarily
  use different classes (they're all db::mysql::Session) or different session
  object instances.

  Shell session
  -------------

  The session opened by the shell to connect to the server the user gives in
  the command line, \connect or shell.connect().

  Use by DevAPI and the SQL mode, but not supposed to be used by AdminAPI,
  except for figuring out what is the cluster being managed.

  Target member session
  ---------------------

  A session to the MySQL server instance that is being operated on for cluster
  operations, like the member being added or removed from the group.

  Only instance local operations are meant to be performed with this session.
  Group-wide operations must go through the Group session.

  Metadata session
  ----------------

  The session to the MySQL server that stores the metadata. The general design
  is meant to allow the metadata to be stored in a location different from the
  cluster being managed, although the implementation does not allow that today..

  The MySQL session used for metadata operations is not shared with the
  shell session.

  When making changes to the cluster, the metadata session must be read-write
  and thus, it will connect to the primary by default when created. The
  connection to primary will not be required if the option sets the option for
  that.

  Only metadata operations may use this session (that is, reads and writes on
  the mysql_innodb_cluster_metadata schema).

  Group session
  -------------

  A session to a member of a GR group (usually the primary), that will be used
  to query the performance_schema to know the live status of the group/cluster.

  Also the member from which group-wide changes are applied, like creating the
  replication user account for new members. Thus, if changes to the cluster are
  going to be made, the group session must be connected to the primary.

  If possible, the group session will shared the same session as the metadata,
  but not always.

  Scenarios
  ---------

  The shell's target is the primary of the group being managed:
  Metadata  - connect to target
  Group     - share from metadata
  Member    - connect to shell target or param URI

  The shell's target is the secondary of the group being managed and
  connectToPrimary is true:
  Metadata  - find primary from metadata in target and connect
  Group     - share from metadata
  Member    - connect to shell target or param URI

  The shell's target is the secondary of the group being managed and
  connectToPrimary is false:
  Metadata  - connect to target
  Group     - share from metadata
  Member    - connect to shell target or param URI

  Note: in the future, if the managed cluster is not where the metadata resides,
  the metadata schema should contain information about where to find the
  real metadata.
 */

using std::placeholders::_1;
using namespace mysqlsh;
using namespace mysqlsh::dba;
using namespace shcore;

#define PASSWORD_LENGHT 16
using mysqlshdk::db::uri::formats::only_transport;
std::set<std::string> Dba::_deploy_instance_opts = {
    "portx",      "sandboxDir",    "password",
    "dbPassword", "allowRootFrom", "ignoreSslError",
    "mysqldOptions"};
std::set<std::string> Dba::_stop_instance_opts = {"sandboxDir", "password",
                                                  "dbPassword"};
std::set<std::string> Dba::_default_local_instance_opts = {"sandboxDir"};

std::set<std::string> Dba::_create_cluster_opts = {
    "multiMaster", "adoptFromGR", "force", "memberSslMode", "ipWhitelist",
    "clearReadOnly", "groupName", "localAddress", "groupSeeds"};
std::set<std::string> Dba::_reboot_cluster_opts = {
    "user",       "dbUser",          "password",
    "dbPassword", "removeInstances", "rejoinInstances",
    "clearReadOnly"};

// Documentation of the DBA Class
REGISTER_HELP(DBA_BRIEF,
              "Enables you to administer InnoDB clusters using the AdminAPI.");
REGISTER_HELP(
    DBA_DETAIL,
    "The global variable 'dba' is used to access the AdminAPI functionality "
    "and perform DBA operations. It is used for managing MySQL InnoDB "
    "clusters.");
REGISTER_HELP(
    DBA_CLOSING,
    "For more help on a specific function use: dba.help('<functionName>')");
REGISTER_HELP(DBA_CLOSING1, "e.g. dba.help('deploySandboxInstance')");

REGISTER_HELP(DBA_VERBOSE_BRIEF, "Enables verbose mode on the Dba operations.");
REGISTER_HELP(DBA_VERBOSE_DETAIL,
              "The assigned value can be either boolean or integer, the result "
              "depends on the assigned value:");
REGISTER_HELP(DBA_VERBOSE_DETAIL1, "@li 0: disables mysqlprovision verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL2, "@li 1: enables mysqlprovision verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL3,
              "@li >1: enables mysqlprovision debug verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL4,
              "@li Boolean: equivalent to assign either 0 or 1");


Dba::Dba(shcore::IShell_core *owner) : _shell_core(owner) {
  init();
}

Dba::~Dba() {
}

bool Dba::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Dba::init() {
  add_property("verbose");

  // Pure functions
  add_method("createCluster", std::bind(&Dba::create_cluster, this, _1),
             "clusterName", shcore::String, NULL);
  add_method("getCluster", std::bind(&Dba::get_cluster_, this, _1),
             "clusterName", shcore::String, NULL);
  add_method("dropMetadataSchema",
             std::bind(&Dba::drop_metadata_schema, this, _1), "data",
             shcore::Map, NULL);
  add_method("checkInstanceConfiguration",
             std::bind(&Dba::check_instance_configuration, this, _1), "data",
             shcore::Map, NULL);
  add_method("deploySandboxInstance",
             std::bind(&Dba::deploy_sandbox_instance, this, _1,
                       "deploySandboxInstance"),
             "data", shcore::Map, NULL);
  add_method("startSandboxInstance",
             std::bind(&Dba::start_sandbox_instance, this, _1), "data",
             shcore::Map, NULL);
  add_method("stopSandboxInstance",
             std::bind(&Dba::stop_sandbox_instance, this, _1), "data",
             shcore::Map, NULL);
  add_method("deleteSandboxInstance",
             std::bind(&Dba::delete_sandbox_instance, this, _1), "data",
             shcore::Map, NULL);
  add_method("killSandboxInstance",
             std::bind(&Dba::kill_sandbox_instance, this, _1), "data",
             shcore::Map, NULL);
  add_method("configureLocalInstance",
             std::bind(&Dba::configure_local_instance, this, _1), "data",
             shcore::Map, NULL);
  add_varargs_method(
      "rebootClusterFromCompleteOutage",
      std::bind(&Dba::reboot_cluster_from_complete_outage, this, _1));

  _provisioning_interface.reset(
      new ProvisioningInterface(_shell_core->get_delegate()));
}

void Dba::set_member(const std::string &prop, Value value) {
  if (prop == "verbose") {
    try {
      int verbosity = value.as_int();
      _provisioning_interface->set_verbose(verbosity);
    } catch (shcore::Exception &e) {
      throw shcore::Exception::value_error(
          "Invalid value for property 'verbose', use either boolean or integer "
          "value.");
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

/*
  Open a classic session to the instance the shell is connected to.
  If the shell's session is X protocol, it will query it for the classic
  port and connect to it.
 */
std::shared_ptr<mysqlshdk::db::ISession> Dba::connect_to_target_member() const {
  auto active_shell_session = get_active_shell_session();

  if (active_shell_session) {
    auto coptions = active_shell_session->get_connection_options();
    if (coptions.get_scheme() == "mysqlx") {
      // Find the classic port
      auto result = active_shell_session->query("select @@port");
      auto row = result->fetch_one();
      if (!row)
        throw std::logic_error(
            "Unable to determine classic MySQL port from active shell "
            "connection");
      coptions.clear_port();
      coptions.clear_scheme();
      coptions.set_port(row->get_int(0));
      coptions.set_scheme("mysql");
    }
    auto member_session = mysqlshdk::db::mysql::Session::create();
    member_session->connect(coptions);
    return member_session;
  }
  return {};
}

/*
   Create a metadata object and a MySQL session for managing the InnoDB cluster
   which the shell's active session target is part of.

   If a target_member_session is given, it will be reused for the metadata
   and group sessions if possible. Note that the target_member_session must
   not be the shell's global session.
 */
void Dba::connect_to_target_group(
    std::shared_ptr<mysqlshdk::db::ISession> target_member_session,
    std::shared_ptr<MetadataStorage> *out_metadata,
    std::shared_ptr<mysqlshdk::db::ISession> *out_group_session,
    bool connect_to_primary) const {

  bool owns_target_member_session = false;
  if (!target_member_session) {
    target_member_session = connect_to_target_member();
    owns_target_member_session = true;
  }
  if (!target_member_session)
    throw shcore::Exception::logic_error(
        "The shell must be connected to a member of the InnoDB cluster being "
        "managed");

  // Group session can be the same as the target member session
  std::shared_ptr<mysqlshdk::db::ISession> group_session;
  group_session = target_member_session;

  if (connect_to_primary) {
    mysqlshdk::mysql::Instance instance(target_member_session);

    if (!mysqlshdk::gr::is_primary(instance)) {
      log_info(
          "%s is not a primary, will try to find one and reconnect",
          target_member_session->get_connection_options().as_uri().c_str());
      auto metadata(mysqlshdk::innodbcluster::Metadata_mysql::create(
          target_member_session));

      // We need a primary, but the shell is not connected to one, so we need
      // to find out where it is and connect to it
      mysqlshdk::innodbcluster::Cluster_group_client cluster(
          metadata, target_member_session);

      try {
        std::string primary_uri = cluster.find_uri_to_any_primary(
            mysqlshdk::innodbcluster::Protocol_type::Classic);
        mysqlshdk::db::Connection_options coptions(primary_uri);

        coptions.set_login_options_from(
            target_member_session->get_connection_options());
        coptions.set_ssl_connection_options_from(
            target_member_session->get_connection_options().get_ssl_options());

        log_info("Opening session to primary of InnoDB cluster at %s...",
                coptions.as_uri().c_str());

        std::shared_ptr<mysqlshdk::db::mysql::Session> session;
        session = mysqlshdk::db::mysql::Session::create();
        session->connect(coptions);
        log_info(
            "Metadata and group sessions are now using the primary member");
        if (owns_target_member_session)
          target_member_session->close();
        target_member_session = session;
        group_session = session;
      } catch (std::exception &e) {
        throw shcore::Exception::runtime_error(
            std::string("Unable to find a primary member in the cluster: ") +
            e.what());
      }
    } else {
      // already the primary, but check if there's quorum
      mysqlshdk::innodbcluster::check_quorum(&instance);
    }
  }

  *out_group_session = group_session;

  // Metadata is always stored in the group, so for now the session can be
  // shared
  out_metadata->reset(new MetadataStorage(group_session));
}

std::shared_ptr<mysqlshdk::db::ISession> Dba::get_active_shell_session() const {
  if (_shell_core && _shell_core->get_dev_session())
    return _shell_core->get_dev_session()->get_core_session();
  return {};
}


// Documentation of the getCluster function
REGISTER_HELP(DBA_GETCLUSTER_BRIEF,
              "Retrieves a cluster from the Metadata Store.");
REGISTER_HELP(DBA_GETCLUSTER_PARAM,
              "@param name Optional parameter to specify "
              "the name of the cluster to be returned.");
REGISTER_HELP(DBA_GETCLUSTER_PARAM1,
              "@param options Optional dictionary with additional options.");

REGISTER_HELP(DBA_GETCLUSTER_THROWS,
              "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS1,
              "@throws MetadataError if the Metadata update operation failed.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS2,
              "@throws ArgumentError if the Cluster name is empty.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS3,
              "@throws ArgumentError if the Cluster name is invalid.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS4,
              "@throws ArgumentError if the Cluster does not exist.");

REGISTER_HELP(DBA_GETCLUSTER_RETURNS,
              "@returns The cluster object identified "
              " by the given name or the default "
              " cluster.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL,
              "If name is not specified or is null, the default "
              "cluster will be returned.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL1,
              "If name is specified, and no cluster "
              "with the indicated name is found, an "
              "error will be raised.");
REGISTER_HELP(DBA_GETCLUSTER_DETAIL2,
              "The options dictionary accepts the connectToPrimary option,"
              "which defaults to true and indicates the shell to automatically "
              "connect to the primary member of the cluster.");
/**
 * $(DBA_GETCLUSTER_BRIEF)
 *
 * $(DBA_GETCLUSTER_PARAM)
 * $(DBA_GETCLUSTER_PARAM1)
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
 * $(DBA_GETCLUSTER_DETAIL2)
 */
#if DOXYGEN_JS
Cluster Dba::getCluster(String name, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::get_cluster(str name, dict options) {}
#endif
shcore::Value Dba::get_cluster_(const shcore::Argument_list &args) const {
  args.ensure_count(0, 2, get_function_name("getCluster").c_str());
  // TODO(alfredo) - suggest running dba.diagnose() in case it's a dead
  // cluster that needs reboot
  bool connect_to_primary = true;
  bool fallback_to_anything = true;
  bool default_cluster = true;
  std::string cluster_name;

  try {
    if (args.size() > 1) {
      shcore::Argument_map options(*args[1].as_map());

      options.ensure_keys({}, {"connectToPrimary"}, "the options");

      if (options.has_key("connectToPrimary"))
        fallback_to_anything = false;

      if (options.has_key("connectToPrimary") &&
          !options.bool_at("connectToPrimary"))
        connect_to_primary = false;
    }
    // gets the cluster_name and/or options
    if (args.size() && args[0].type != shcore::Null) {
      try {
        cluster_name = args.string_at(0);
        default_cluster = false;
      } catch (std::exception &e) {
        throw shcore::Exception::argument_error(
            std::string("Invalid cluster name: ") + e.what());
      }
    }

    std::shared_ptr<MetadataStorage> metadata;
    std::shared_ptr<mysqlshdk::db::ISession> group_session;
    // This will throw if not a cluster member
    try {
      // Connect to the target cluster member and
      // also find the primary and connect to it, unless target is already
      // primary or connectToPrimary:false was given
      connect_to_target_group({}, &metadata, &group_session,
                              connect_to_primary);
    } catch (mysqlshdk::innodbcluster::cluster_error &e) {
      log_warning("Cluster error connecting to target: %s", e.format().c_str());

      if (e.code() == mysqlshdk::innodbcluster::Error::Group_has_no_quorum &&
          fallback_to_anything && connect_to_primary) {
        log_info("Retrying getCluster() without connectToPrimary");
        connect_to_target_group({}, &metadata, &group_session, false);
      } else {
        throw;
      }
    }

    // TODO(.) This may be redundant
    check_function_preconditions(class_name(), "getCluster",
                                 get_function_name("getCluster"),
                                 group_session);

    return shcore::Value(
        get_cluster(default_cluster ? nullptr : cluster_name.c_str(),
                    metadata, group_session));
  } catch (mysqlshdk::innodbcluster::cluster_error &e) {
    if (e.code() == mysqlshdk::innodbcluster::Error::Group_has_no_quorum)
      throw shcore::Exception::runtime_error(
          get_function_name("getCluster") +
          ": Unable to find a cluster PRIMARY member from the active shell "
          "session because the cluster has too many UNREACHABLE members and "
          "no quorum is possible.\n"
          "Use " +
          get_function_name("getCluster") +
          "(null, {connectToPrimary:false}) to get a read-only cluster "
          "handle.");
    throw;
  }
  CATCH_AND_TRANSLATE_CLUSTER_EXCEPTION(get_function_name("getCluster"));
}


std::shared_ptr<Cluster> Dba::get_cluster(
    const char *name, std::shared_ptr<MetadataStorage> metadata,
    std::shared_ptr<mysqlshdk::db::ISession> group_session) const {

  std::shared_ptr<mysqlsh::dba::Cluster> cluster(
      new Cluster("", group_session, metadata));

  if (!name) {
    // Reloads the cluster (to avoid losing _default_cluster in case of error)
    metadata->load_default_cluster(cluster);
  } else {
    metadata->load_cluster(name, cluster);
  }
  // Verify if the current session instance group_replication_group_name
  // value differs from the one registered in the Metadata
  if (!validate_replicaset_group_name(
          group_session,
          cluster->get_default_replicaset()->get_group_name())) {
    std::string nice_error =
        "Unable to get a InnoDB cluster handle. "
        "The instance '" +
        group_session->uri(mysqlshdk::db::uri::formats::only_transport()) +
        ""
        "' may belong to "
        "a different ReplicaSet as the one registered in the Metadata "
        "since the value of 'group_replication_group_name' does "
        "not match the one registered in the ReplicaSet's "
        "Metadata: possible split-brain scenario. "
        "Please connect to another member of the ReplicaSet to get the "
        "Cluster.";

    throw shcore::Exception::runtime_error(nice_error);
  }

  // Set the provision interface pointer
  cluster->set_provisioning_interface(_provisioning_interface);

  return cluster;
}

REGISTER_HELP(DBA_CREATECLUSTER_BRIEF, "Creates a MySQL InnoDB cluster.");
REGISTER_HELP(DBA_CREATECLUSTER_PARAM,
              "@param name The name of the cluster object to be created.");
REGISTER_HELP(DBA_CREATECLUSTER_PARAM1,
              "@param options Optional dictionary "
              "with options that modify the behavior "
              "of this function.");

REGISTER_HELP(DBA_CREATECLUSTER_THROWS,
              "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS1,
              "@throws MetadataError if the Metadata update operation failed.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS2,
              "@throws ArgumentError if the Cluster name is empty.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS3,
              "@throws ArgumentError if the Cluster name is not valid.");
REGISTER_HELP(
    DBA_CREATECLUSTER_THROWS4,
    "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS5,
              "@throws ArgumentError if adoptFromGR "
              "is true and the memberSslMode option "
              "is used.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS6,
              "@throws ArgumentError if the value "
              "for the memberSslMode option is not "
              "one of the allowed.");

REGISTER_HELP(DBA_CREATECLUSTER_THROWS7,
              "@throws ArgumentError if adoptFromGR "
               "is true and the multiMaster option "
               "is used.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS8,
              "@throws ArgumentError if the value for the ipWhitelist, "\
              "groupName, localAddress, or groupSeeds options is empty.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS9,
              "@throws RuntimeError if the value for the groupName, "\
              "localAddress, or groupSeeds options is not valid for Group "\
              "Replication.");

REGISTER_HELP(DBA_CREATECLUSTER_RETURNS,
              "@returns The created cluster object.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL,
              "Creates a MySQL InnoDB cluster taking "
              "as seed instance the active global "
              "session.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL1,
              "The options dictionary can contain the next values:");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL2,
              "@li multiMaster: boolean value used "
              "to define an InnoDB cluster with "
              "multiple writable instances.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL3,
              "@li force: boolean, confirms that "
              "the multiMaster option must be "
              "applied.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL4,
              "@li adoptFromGR: boolean value used "
              "to create the InnoDB cluster based "
              "on existing replication group.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL5,
              "@li memberSslMode: SSL mode used to "
              "configure the members of the "
              "cluster.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL6,
              "@li ipWhitelist: The list of hosts "
              "allowed to connect to the instance "
              "for group replication.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL7,
              "@li clearReadOnly: boolean value "
              "used to confirm that super_read_only "
              "must be disabled.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL8,
"@li groupName: string value with the Group Replication group "\
              "name UUID to be used instead of the automatically generated "
"one.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL9,
"@li localAddress: string value with the Group Replication "\
              "local address to be used instead of the automatically "\
              "generated one.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL10,
"@li groupSeeds: string value with a comma-separated list of "\
              "the Group Replication peer addresses to be used instead of the "\
              "automatically generated one.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL11,
              "A InnoDB cluster may be setup in two ways:");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL12,
              "@li Single Master: One member of the cluster allows write "
              "operations while the rest are in read only mode.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL13,
              "@li Multi Master: All the members "
              "in the cluster support both read "
              "and write operations.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL14,
              "By default this function create a Single Master cluster, use "
              "the multiMaster option set to true "
              "if a Multi Master cluster is required.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL15,
              "The memberSslMode option supports these values:");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL16,
              "@li REQUIRED: if used, SSL (encryption) will be enabled for the "
              "instances to communicate with other members of the cluster");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL17,
              "@li DISABLED: if used, SSL (encryption) will be disabled");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL18,
              "@li AUTO: if used, SSL (encryption) "
              "will be enabled if supported by the "
              "instance, otherwise disabled");
REGISTER_HELP(
    DBA_CREATECLUSTER_DETAIL19,
    "If memberSslMode is not specified AUTO will be used by default.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL20,
              "The ipWhitelist format is a comma separated list of IP "
              "addresses or subnet CIDR "
              "notation, for example: 192.168.1.0/24,10.0.0.1. By default the "
              "value is set to AUTOMATIC, allowing addresses "
              "from the instance private network to be automatically set for "
              "the whitelist.");


REGISTER_HELP(DBA_CREATECLUSTER_DETAIL21,
              "The groupName, localAddress, and groupSeeds are advanced "\
              "options and their usage is discouraged since incorrect values "\
              "can lead to Group Replication errors.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL22,
              "The value for groupName is used to set the Group Replication "\
              "system variable 'group_replication_group_name'.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL23,
              "The value for localAddress is used to set the Group "\
              "Replication system variable 'group_replication_local_address'. "\
              "The localAddress option accepts values in the format: "\
              "'host:port' or 'host:' or ':port'. If the specified "\
              "value does not include a colon (:) and it is numeric, then it "\
              "is assumed to be the port, otherwise it is considered to be "\
              "the host. When the host is not specified, the default value is "\
              "the host of the current active connection (session). When the "\
              "port is not specified, the default value is the port of the "\
              "current active connection (session) * 10 + 1. In case the "\
              "automatically determined default port value is invalid "\
              "(> 65535) then a random value in the range [1000, 65535] is "\
              "used.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL24,
              "The value for groupSeeds is used to set the Group Replication "\
              "system variable 'group_replication_group_seeds'. The "\
              "groupSeeds option accepts a comma-separated list of addresses "\
              "in the format: 'host1:port1,...,hostN:portN'.");

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
 * $(DBA_CREATECLUSTER_THROWS7)
 * $(DBA_CREATECLUSTER_THROWS8)
 * $(DBA_CREATECLUSTER_THROWS9)
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
 * $(DBA_CREATECLUSTER_DETAIL8)
 * $(DBA_CREATECLUSTER_DETAIL9)
 * $(DBA_CREATECLUSTER_DETAIL10)
 *
 * $(DBA_CREATECLUSTER_DETAIL11)
 *
 * $(DBA_CREATECLUSTER_DETAIL12)
 * $(DBA_CREATECLUSTER_DETAIL13)
 * $(DBA_CREATECLUSTER_DETAIL14)
 *
 * $(DBA_CREATECLUSTER_DETAIL15)
 * $(DBA_CREATECLUSTER_DETAIL16)
 * $(DBA_CREATECLUSTER_DETAIL17)
 * $(DBA_CREATECLUSTER_DETAIL18)
 * $(DBA_CREATECLUSTER_DETAIL19)
 *
 * $(DBA_CREATECLUSTER_DETAIL20)
 *
 * $(DBA_CREATECLUSTER_DETAIL21)
 *
 * $(DBA_CREATECLUSTER_DETAIL22)
 *
 * $(DBA_CREATECLUSTER_DETAIL23)
 *
 * $(DBA_CREATECLUSTER_DETAIL24)
 */
#if DOXYGEN_JS
Cluster Dba::createCluster(String name, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::create_cluster(str name, dict options) {}
#endif
shcore::Value Dba::create_cluster(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("createCluster").c_str());

  ReplicationGroupState state;

  std::shared_ptr<MetadataStorage> metadata;
  std::shared_ptr<mysqlshdk::db::ISession> group_session;

  // We're in createCluster(), so there's no metadata yet, but the metadata
  // object can already exist now to hold a session
  // connect_to_primary must be false, because we don't have a cluster yet
  // and this will be the seed instance anyway. If we start storing metadata
  // outside the managed cluster, then this will have to be updated.
  connect_to_target_group({}, &metadata, &group_session, false);

  try {
    state = check_preconditions(group_session, "createCluster");
  } catch (shcore::Exception &e) {
    std::string error(e.what());
    if (error.find("already in an InnoDB cluster") != std::string::npos) {
      /*
       * For V1.0 we only support one single Cluster. That one shall be the
       * default Cluster. We must check if there's already a Default Cluster
       * assigned, and if so thrown an exception. And we must check if there's
       * already one Cluster on the MD and if so assign it to Default
       */

      std::string nice_error = get_function_name("createCluster") +
                               ": Unable to create cluster. The instance '" +
                               group_session->uri(only_transport()) +
                               "' already belongs to an InnoDB cluster. Use "
                               "<Dba>." +
                               get_function_name("getCluster", false) +
                               "() to access it.";
      throw shcore::Exception::runtime_error(nice_error);
    } else {
      throw;
    }
  } catch (const mysqlshdk::db::Error& dberr) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        dberr.what(), dberr.code(), dberr.sqlstate());
  }

  // Available options
  Value ret_val;
  // SSL values are only set if available from args.
  std::string ssl_mode, group_name, local_address, group_seeds;

  std::string replication_user;
  std::string replication_pwd;
  std::string ip_whitelist;

  try {
    bool multi_master = false;  // Default single/primary master
    bool adopt_from_gr = false;
    bool force = false;
    bool clear_read_only = false;

    std::string cluster_name = args.string_at(0);

    // Validate the cluster_name
    mysqlsh::dba::validate_cluster_name(cluster_name);

    if (args.size() > 1) {
      // Map with the options
      shcore::Value::Map_type_ref options = args.map_at(1);

      // Verification of invalid attributes on the instance creation options
      shcore::Argument_map opt_map(*options);

      opt_map.ensure_keys({}, _create_cluster_opts, "the options");

      // Validate SSL options for the cluster instance
      validate_ssl_instance_options(options);

      // Validate ip whitelist option
      validate_ip_whitelist_option(options);

      // Validate group name option
      validate_group_name_option(options);

      // Validate local address option
      validate_local_address_option(options);

      // Validate group seeds option
      validate_group_seeds_option(options);

      if (opt_map.has_key("multiMaster"))
        multi_master = opt_map.bool_at("multiMaster");

      if (opt_map.has_key("force"))
        force = opt_map.bool_at("force");

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
      if (adopt_from_gr && opt_map.has_key("multiMaster")) {
        throw shcore::Exception::argument_error(
            "Cannot use multiMaster option if adoptFromGR is set to true."
            " Using adoptFromGR mode will adopt the primary mode in use by the "
            "Cluster.");
      }
      if (opt_map.has_key("clearReadOnly"))
        clear_read_only = opt_map.bool_at("clearReadOnly");

      if (opt_map.has_key("groupName"))
        group_name = opt_map.string_at("groupName");

      if (opt_map.has_key("localAddress"))
        local_address = opt_map.string_at("localAddress");

      if (opt_map.has_key("groupSeeds"))
        group_seeds = opt_map.string_at("groupSeeds");
    }

    if (state.source_type == GRInstanceType::GroupReplication && !adopt_from_gr)
      throw Exception::argument_error(
          "Creating a cluster on an unmanaged replication group requires "
          "adoptFromGR option to be true");

    // Check if super_read_only is turned off and disable it if required
    // NOTE: this is left for last to avoid setting super_read_only to true
    // and right before some execution failure of the command leaving the
    // instance in an incorrect state
    validate_super_read_only(group_session, clear_read_only);

    // Check replication filters before creating the Metadata.
    validate_replication_filters(group_session);

    // First we need to create the Metadata Schema, or update it if already
    // exists
    metadata->create_metadata_schema();

    // Creates the replication account to for the primary instance
    metadata->create_repl_account(replication_user, replication_pwd);
    log_debug("Created replication user '%s'", replication_user.c_str());

    MetadataStorage::Transaction tx(metadata);

    std::shared_ptr<Cluster> cluster(
        new Cluster(cluster_name, group_session, metadata));
    cluster->set_provisioning_interface(_provisioning_interface);

    // Update the properties
    // For V1.0, let's see the Cluster's description to "default"
    cluster->set_description("Default Cluster");

    cluster->set_attribute(ATT_DEFAULT, shcore::Value::True());

    // Insert Cluster on the Metadata Schema
    metadata->insert_cluster(cluster);

    if (adopt_from_gr) {  // TODO(paulo): move this to a GR specific class
      // check whether single_primary_mode is on
      auto result = group_session->query(
          "select @@group_replication_single_primary_mode");
      auto row = result->fetch_one();
      if (row) {
        log_info("Adopted cluster: group_replication_single_primary_mode=%s",
                 row->get_as_string(0).c_str());
        multi_master = row->get_int(0) == 0;
      }
    }

    Value::Map_type_ref options(new shcore::Value::Map_type);
    shcore::Argument_list new_args;

    auto instance_def = group_session->get_connection_options();

    // Only set SSL option if available from createCluster options (not empty).
    // Do not set default values to avoid validation issues for addInstance.
    if (!ssl_mode.empty())
      (*options)["memberSslMode"] = Value(ssl_mode);

    // Set IP whitelist
    if (!ip_whitelist.empty())
      (*options)["ipWhitelist"] = Value(ip_whitelist);

    // Set local address
    if (!local_address.empty())
      (*options)["localAddress"] = Value(local_address);

    // Set group seeds
    if (!group_seeds.empty())
      (*options)["groupSeeds"] = Value(group_seeds);

    new_args.push_back(shcore::Value(options));

    if (multi_master && !force) {
      throw shcore::Exception::argument_error(
          "Use of multiMaster mode is not recommended unless you understand "
          "the limitations. Please use the 'force' option if you understand "
          "and accept them.");
    }
    if (multi_master) {
      log_info(
          "The MySQL InnoDB cluster is going to be setup in advanced "
          "Multi-Master Mode. Consult its requirements and limitations in "
          "https://dev.mysql.com/doc/refman/en/group-replication-limitations.html");
    }
    cluster->add_seed_instance(instance_def, new_args, multi_master,
                               adopt_from_gr, replication_user,
                               replication_pwd, group_name);

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
        group_session->query("DROP USER IF EXISTS /*(*/" +
                             replication_user + "/*)*/");
      }

      throw;
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createCluster"));
    throw;
  }

  return ret_val;
}

REGISTER_HELP(DBA_DROPMETADATASCHEMA_BRIEF, "Drops the Metadata Schema.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_PARAM,
              "@param options Dictionary "
              "containing an option to confirm "
              "the drop operation.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_THROWS,
              "@throws MetadataError if the Metadata is inaccessible.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_RETURNS, "@returns nothing.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_DETAIL,
              "The options dictionary may contain the following options:");
REGISTER_HELP(
    DBA_DROPMETADATASCHEMA_DETAIL1,
    "@li force: boolean, confirms that the drop operation must be executed.");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_DETAIL2,
              "@li clearReadOnly: boolean "
              "value used to confirm that "
              "super_read_only must be "
              "disabled");

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
* $(DBA_DROPMETADATASCHEMA_DETAIL2)
*/
#if DOXYGEN_JS
Undefined Dba::dropMetadataSchema(Dictionary options) {}
#elif DOXYGEN_PY
None Dba::drop_metadata_schema(dict options) {}
#endif

shcore::Value Dba::drop_metadata_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("dropMetadataSchema").c_str());

  std::shared_ptr<MetadataStorage> metadata;
  std::shared_ptr<mysqlshdk::db::ISession> group_session(
      connect_to_target_member());
  check_preconditions(group_session, "dropMetadataSchema");

  connect_to_target_group(group_session, &metadata, &group_session, false);

  check_preconditions(group_session, "dropMetadataSchema");

  try {
    bool force = false;
    bool clear_read_only = false;

    // Map with the options
    shcore::Value::Map_type_ref options = args.map_at(0);

    shcore::Argument_map opt_map(*options);

    opt_map.ensure_keys({}, {"force", "clearReadOnly"}, "the options");

    if (opt_map.has_key("force"))
      force = opt_map.bool_at("force");

    if (opt_map.has_key("clearReadOnly"))
        clear_read_only = opt_map.bool_at("clearReadOnly");

    if (force) {
      // Check if super_read_only is turned off and disable it if required
      // NOTE: this is left for last to avoid setting super_read_only to true
      // and right before some execution failure of the command leaving the
      // instance in an incorrect state
      validate_super_read_only(group_session, clear_read_only);

      metadata->drop_metadata_schema();
    } else {
      throw shcore::Exception::runtime_error(
          "No operation executed, use the 'force' option");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("dropMetadataSchema"))

  return Value();
}

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_BRIEF,
              "Validates an instance for cluster usage.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_PARAM,
              "@param instance An instance definition.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_PARAM1,
              "@param options Optional data for the operation.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS,
              "@throws ArgumentError if the instance parameter is empty.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS1,
              "@throws ArgumentError if the instance definition is invalid.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS2,
              "@throws ArgumentError if the instance definition is a "
              "connection dictionary but empty.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS3,
              "@throws RuntimeError if the instance accounts are invalid.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS4,
              "@throws RuntimeError if the instance is offline.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS5,
              "@throws RuntimeError if the instance is already part of a "
              "Replication Group.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS6,
              "@throws RuntimeError if the instance is already part of an "
              "InnoDB Cluster.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_RETURNS,
              "@returns A JSON object with the status.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL,
              "This function reviews the instance configuration to identify if "
              "it is valid "
              "for usage in group replication.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL1,
              "The instance definition is the connection data for the instance.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL2,
              "TOPIC_CONNECTION_MORE_INFO_TCP_ONLY");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL3,
              "The options dictionary may contain the following options:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL4,
              "@li mycnfPath: The path of the MySQL configuration file for the "
              "instance.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL5,
              "@li password: The password to get connected to the instance.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL6,
              "The connection password may be contained on the instance "
              "definition, however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL7,
              "The returned JSON object contains the following attributes:");
REGISTER_HELP(
    DBA_CHECKINSTANCECONFIGURATION_DETAIL8,
    "@li status: the final status of the command, either \"ok\" or \"error\".");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL9,
              "@li config_errors: a list of dictionaries containing the failed "
              "requirements");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL10,
              "@li errors: a list of errors of the operation");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL11,
              "@li restart_required: a boolean value indicating whether a "
              "restart is required");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL12,
              "Each dictionary of the list of config_errors includes the "
              "following attributes:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL13,
              "@li option: The configuration option for which the requirement "
              "wasn't met");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL14,
              "@li current: The current value of the configuration option");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL15,
              "@li required: The configuration option required value");
REGISTER_HELP(
    DBA_CHECKINSTANCECONFIGURATION_DETAIL16,
    "@li action: The action to be taken in order to meet the requirement");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL17,
              "The action can be one of the following:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL18,
              "@li server_update+config_update: Both the server and the "
              "configuration need to be updated");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL19,
              "@li config_update+restart: The configuration needs to be "
              "updated and the server restarted");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL20,
              "@li config_update: The configuration needs to be updated");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL21,
              "@li server_update: The server needs to be updated");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL22,
              "@li restart: The server needs to be restarted");

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
*
* Detailed description of the connection data format is available at \ref connection_data.
*
* Only TCP/IP connections are allowed for this function.
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL3)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL4)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL5)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL6)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL7)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL8)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL9)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL10)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL11)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL12)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL13)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL14)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL15)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL16)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL17)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL18)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL19)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL20)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL21)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL22)
*/
#if DOXYGEN_JS
Undefined Dba::checkInstanceConfiguration(InstanceDef instance,
                                          Dictionary options) {}
#elif DOXYGEN_PY
None Dba::check_instance_configuration(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::check_instance_configuration(
    const shcore::Argument_list &args) {
  args.ensure_count(1, 2,
                    get_function_name("checkInstanceConfiguration").c_str());

  shcore::Value ret_val;

  try {
    auto instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    // Resolves user and validates password
    mysqlsh::resolve_connection_credentials(&instance_def);

    shcore::Value::Map_type_ref options;
    if (args.size() == 2)
      options = args.map_at(1);

    ret_val = shcore::Value(
        _check_instance_configuration(instance_def, options, false));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("checkInstanceConfiguration"));

  return ret_val;
}

shcore::Value Dba::exec_instance_op(const std::string &function,
                                    const shcore::Argument_list &args) {
  shcore::Value ret_val;

  shcore::Value::Map_type_ref options;  // Map with the connection data
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
          throw shcore::Exception::argument_error(
              "Invalid value for 'portx': Please use a valid TCP port number "
              ">= 1024 and <= 65535");
      }

      if (opt_map.has_key("password"))
        password = opt_map.string_at("password");
      else if (opt_map.has_key("dbPassword"))
        password = opt_map.string_at("dbPassword");
      else
        throw shcore::Exception::argument_error(
            "Missing root password for the deployed instance");
    } else if (function == "stop") {
      opt_map.ensure_keys({}, _stop_instance_opts, "the instance data");
      if (opt_map.has_key("password"))
        password = opt_map.string_at("password");
      else if (opt_map.has_key("dbPassword"))
        password = opt_map.string_at("dbPassword");
      else
        throw shcore::Exception::argument_error(
            "Missing root password for the instance");
    } else {
      opt_map.ensure_keys({}, _default_local_instance_opts,
                          "the instance data");
    }

    if (opt_map.has_key("sandboxDir")) {
      sandbox_dir = opt_map.string_at("sandboxDir");
    }

    if (opt_map.has_key("ignoreSslError"))
      ignore_ssl_error = opt_map.bool_at("ignoreSslError");

    if (options->has_key("mysqldOptions"))
      mycnf_options = (*options)["mysqldOptions"];
  } else {
    if (function == "deploy")
      throw shcore::Exception::argument_error(
          "Missing root password for the deployed instance");
  }

  shcore::Value::Array_type_ref errors;

  if (port < 1024 || port > 65535)
    throw shcore::Exception::argument_error(
        "Invalid value for 'port': Please use a valid TCP port number >= 1024 "
        "and <= 65535");

  int rc = 0;
  if (function == "deploy")
    // First we need to create the instance
    rc = _provisioning_interface->create_sandbox(port, portx, sandbox_dir,
                                                 password, mycnf_options, true,
                                                 ignore_ssl_error, &errors);
  else if (function == "delete")
    rc = _provisioning_interface->delete_sandbox(port, sandbox_dir, &errors);
  else if (function == "kill")
    rc = _provisioning_interface->kill_sandbox(port, sandbox_dir, &errors);
  else if (function == "stop")
    rc = _provisioning_interface->stop_sandbox(port, sandbox_dir, password,
                                               &errors);
  else if (function == "start")
    rc = _provisioning_interface->start_sandbox(port, sandbox_dir, &errors);

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

    throw shcore::Exception::runtime_error(shcore::str_join(str_errors, "\n"));
  }

  return ret_val;
}

REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_BRIEF,
              "Creates a new MySQL Server instance on localhost.");
REGISTER_HELP(
    DBA_DEPLOYSANDBOXINSTANCE_PARAM,
    "@param port The port where the new instance will listen for connections.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_PARAM1,
              "@param options Optional dictionary with options affecting the "
              "new deployed instance.");

REGISTER_HELP(
    DBA_DEPLOYSANDBOXINSTANCE_THROWS,
    "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(
    DBA_DEPLOYSANDBOXINSTANCE_THROWS1,
    "@throws ArgumentError if the root password is missing on the options.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS2,
              "@throws ArgumentError if the port value is < 1024 or > 65535.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS3,
              "@throws RuntimeError if SSL "
              "support can be provided and "
              "ignoreSslError: false.");

// REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_RETURNS, "@returns The deployed
// Instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(
    DBA_DEPLOYSANDBOXINSTANCE_DETAIL,
    "This function will deploy a new MySQL Server instance, the result may be "
    "affected by the provided options: ");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL1,
              "@li portx: port where the "
              "new instance will listen for "
              "X Protocol connections.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL2,
              "@li sandboxDir: path where the new instance will be deployed.");
REGISTER_HELP(
    DBA_DEPLOYSANDBOXINSTANCE_DETAIL3,
    "@li password: password for the MySQL root user on the new instance.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL4,
              "@li allowRootFrom: create remote root account, restricted to "
              "the given address pattern (eg %).");
REGISTER_HELP(
    DBA_DEPLOYSANDBOXINSTANCE_DETAIL5,
    "@li ignoreSslError: Ignore errors when adding SSL support for the new "
    "instance, by default: true.");
REGISTER_HELP(
    DBA_DEPLOYSANDBOXINSTANCE_DETAIL6,
    "If the portx option is not specified, it will be automatically calculated "
    "as 10 times the value of the provided MySQL port.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL7,
              "The password or dbPassword options specify the MySQL root "
              "password on the new instance.");
REGISTER_HELP(
    DBA_DEPLOYSANDBOXINSTANCE_DETAIL8,
    "The sandboxDir must be an existing folder where the new instance will be "
    "deployed. If not specified the new instance will be deployed at:");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL9,
              "  ~/mysql-sandboxes on Unix-like systems or "
              "%userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_DETAIL10,
              "SSL support is added by "
              "default if not already available for the new instance, but if "
              "it fails to be "
              "added then the error is ignored. Set the ignoreSslError option "
              "to false to ensure the new instance is "
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
shcore::Value Dba::deploy_sandbox_instance(const shcore::Argument_list &args,
                                           const std::string &fname) {
  shcore::Value ret_val;

  args.ensure_count(1, 2, get_function_name(fname).c_str());

  try {
    ret_val = exec_instance_op("deploy", args);

    if (args.size() == 2) {
      auto map = args.map_at(1);
      shcore::Argument_map opt_map(*map);
      // create root@<addr> if needed
      // Valid values:
      // allowRootFrom: address
      // allowRootFrom: %
      // allowRootFrom: null (that is, disable the option)
      if (opt_map.has_key("allowRootFrom") &&
          opt_map.at("allowRootFrom").type != shcore::Null) {
        std::string remote_root = opt_map.string_at("allowRootFrom");
        if (!remote_root.empty()) {
          int port = args.int_at(0);
          std::string uri = "root@localhost:" + std::to_string(port);
          mysqlshdk::db::Connection_options instance_def(uri);
          mysqlsh::set_password_from_map(&instance_def, map);

          auto session = get_session(instance_def);
          assert(session);

          log_info("Creating root@%s account for sandbox %i",
                   remote_root.c_str(), port);
          session->execute("SET sql_log_bin = 0");
          {
            std::string pwd;
            if (instance_def.has_password())
              pwd = instance_def.get_password();

            sqlstring create_user("CREATE USER root@? IDENTIFIED BY ?", 0);
            create_user << remote_root << pwd;
            create_user.done();
            session->execute(create_user);
          }
          {
            sqlstring grant("GRANT ALL ON *.* TO root@? WITH GRANT OPTION", 0);
            grant << remote_root;
            grant.done();
            session->execute(grant);
          }
          session->execute("SET sql_log_bin = 1");

          session->close();
        }
      }
      log_warning(
          "Sandbox instances are only suitable for deploying and running on "
          "your local machine for testing purposes and are not accessible from "
          "external networks.");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(fname));

  return ret_val;
}

REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_BRIEF,
              "Deletes an existing MySQL Server instance on localhost.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_PARAM,
              "@param port The port of the instance to be deleted.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_PARAM1,
              "@param options Optional dictionary with options that modify the "
              "way this function is executed.");

REGISTER_HELP(
    DBA_DELETESANDBOXINSTANCE_THROWS,
    "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_THROWS1,
              "@throws ArgumentError if the port value is < 1024 or > 65535.");

REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL,
              "This function will delete an existing MySQL Server instance on "
              "the local host. The following options affect the result:");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL1,
              "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL2,
              "The sandboxDir must be the one where the MySQL instance was "
              "deployed. If not specified it will use:");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_DETAIL3,
              "  ~/mysql-sandboxes on Unix-like systems or "
              "%userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(
    DBA_DELETESANDBOXINSTANCE_DETAIL4,
    "If the instance is not located on the used path an error will occur.");

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
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("deleteSandboxInstance"));

  return ret_val;
}

REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_BRIEF,
              "Kills a running MySQL Server instance on localhost.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_PARAM,
              "@param port The port of the instance to be killed.");
REGISTER_HELP(
    DBA_KILLSANDBOXINSTANCE_PARAM1,
    "@param options Optional dictionary with options affecting the result.");

REGISTER_HELP(
    DBA_KILLSANDBOXINSTANCE_THROWS,
    "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_THROWS1,
              "@throws ArgumentError if the port value is < 1024 or > 65535.");

REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(
    DBA_KILLSANDBOXINSTANCE_DETAIL,
    "This function will kill the process of a running MySQL Server instance "
    "on the local host. The following options affect the result:");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL1,
              "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL2,
              "The sandboxDir must be the one where the MySQL instance was "
              "deployed. If not specified it will use:");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_DETAIL3,
              "  ~/mysql-sandboxes on Unix-like systems or "
              "%userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(
    DBA_KILLSANDBOXINSTANCE_DETAIL4,
    "If the instance is not located on the used path an error will occur.");

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
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("killSandboxInstance"));

  return ret_val;
}

REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_BRIEF,
              "Stops a running MySQL Server instance on localhost.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_PARAM,
              "@param port The port of the instance to be stopped.");
REGISTER_HELP(
    DBA_STOPSANDBOXINSTANCE_PARAM1,
    "@param options Optional dictionary with options affecting the result.");

REGISTER_HELP(
    DBA_STOPSANDBOXINSTANCE_THROWS,
    "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(
    DBA_STOPSANDBOXINSTANCE_THROWS1,
    "@throws ArgumentError if the root password is missing on the options.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_THROWS2,
              "@throws ArgumentError if the port value is < 1024 or > 65535.");

REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(
    DBA_STOPSANDBOXINSTANCE_DETAIL,
    "This function will gracefully stop a running MySQL Server instance "
    "on the local host. The following options affect the result:");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL1,
              "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(
    DBA_STOPSANDBOXINSTANCE_DETAIL2,
    "@li password: password for the MySQL root user on the instance.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL3,
              "The sandboxDir must be the one where the MySQL instance was "
              "deployed. If not specified it will use:");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_DETAIL4,
              "  ~/mysql-sandboxes on Unix-like systems or "
              "%userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(
    DBA_STOPSANDBOXINSTANCE_DETAIL5,
    "If the instance is not located on the used path an error will occur.");

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
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("stopSandboxInstance"));

  return ret_val;
}

REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_BRIEF,
              "Starts an existing MySQL Server instance on localhost.");
REGISTER_HELP(
    DBA_STARTSANDBOXINSTANCE_PARAM,
    "@param port The port where the instance listens for MySQL connections.");
REGISTER_HELP(
    DBA_STARTSANDBOXINSTANCE_PARAM1,
    "@param options Optional dictionary with options affecting the result.");

REGISTER_HELP(
    DBA_STARTSANDBOXINSTANCE_THROWS,
    "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_THROWS1,
              "@throws ArgumentError if the port value is < 1024 or > 65535.");

REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_RETURNS, "@returns nothing.");

REGISTER_HELP(
    DBA_STARTSANDBOXINSTANCE_DETAIL,
    "This function will start an existing MySQL Server instance on the local "
    "host. The following options affect the result:");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL1,
              "@li sandboxDir: path where the instance is located.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL2,
              "The sandboxDir must be the one where the MySQL instance was "
              "deployed. If not specified it will use:");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_DETAIL3,
              "  ~/mysql-sandboxes on Unix-like systems or "
              "%userprofile%\\MySQL\\mysql-sandboxes on Windows systems.");
REGISTER_HELP(
    DBA_STARTSANDBOXINSTANCE_DETAIL4,
    "If the instance is not located on the used path an error will occur.");

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
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("startSandboxInstance"));
  return ret_val;
}

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_BRIEF,
              "Validates and configures an instance for cluster usage.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_PARAM,
              "@param instance An instance definition.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_PARAM1,
              "@param options Optional Additional options for the operation.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS,
              "@throws ArgumentError if the instance parameter is empty.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS1,
              "@throws ArgumentError if the instance definition is invalid.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS2,
              "@throws ArgumentError if the instance definition is a "
              "connection dictionary but empty.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS3,
              "@throws RuntimeError if the instance accounts are invalid.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS4,
              "@throws RuntimeError if the instance is offline.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS5,
              "@throws RuntimeError if the "
              "instance is already part of "
              "a Replication Group.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS6,
              "@throws RuntimeError if the "
              "instance is already part of "
              "an InnoDB Cluster.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_RETURNS,
              "@returns resultset A JSON object with the status.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL,
              "This function reviews the instance configuration to identify if "
              "it is valid "
              "for usage in group replication and cluster. A JSON object is "
              "returned containing the result of the operation.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL1,
              "The instance definition is the connection data for the instance.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL2,
              "TOPIC_CONNECTION_MORE_INFO_TCP_ONLY");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL3,
              "The options dictionary may contain the following options:");
REGISTER_HELP(
    DBA_CONFIGURELOCALINSTANCE_DETAIL4,
    "@li mycnfPath: The path to the MySQL configuration file of the instance.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL5,
              "@li password: The password to be used on the connection.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL6,
              "@li clusterAdmin: The name of the InnoDB cluster administrator "
              "user to be created. The supported format is the standard MySQL "
              "account name format.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL7,
              "@li clusterAdminPassword: The password for the InnoDB cluster "
              "administrator account.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL8,
              "@li clearReadOnly: boolean value used to confirm that "
              "super_read_only must be disabled.");


REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL9,
              "The connection password may be contained on the instance "
              "definition, however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL10,
              "The returned JSON object contains the following attributes:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL11,
    "@li status: the final status of the command, either \"ok\" or \"error\".");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL12,
              "@li config_errors: a list "
              "of dictionaries containing "
              "the failed requirements");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL13,
              "@li errors: a list of errors of the operation");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL14,
              "@li restart_required: a boolean value indicating whether a "
              "restart is required");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL15,
              "Each dictionary of the list of config_errors includes the "
              "following attributes:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL16,
              "@li option: The configuration option for which the requirement "
              "wasn't met");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL17,
              "@li current: The current value of the configuration option");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL18,
              "@li required: The configuration option required value");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL19,
    "@li action: The action to be taken in order to meet the requirement");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL20,
              "The action can be one of the following:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL21,
              "@li server_update+config_update: Both the server and the "
              "configuration need to be updated");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL22,
              "@li config_update+restart: The configuration needs to be "
              "updated and the server restarted");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL23,
              "@li config_update: The configuration needs to be updated");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL24,
              "@li server_update: The server needs to be updated");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL25,
              "@li restart: The server needs to be restarted");
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
*
* Detailed description of the connection data format is available at \ref connection_data.
*
* Only TCP/IP connections are allowed for this function.
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL3)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL4)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL5)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL6)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL7)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL8)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL9)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL10)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL11)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL12)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL13)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL14)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL15)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL16)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL17)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL18)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL19)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL20)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL21)
*
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL22)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL23)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL24)
* $(DBA_CONFIGURELOCALINSTANCE_DETAIL25)
*/
#if DOXYGEN_JS
Instance Dba::configureLocalInstance(InstanceDef instance, Dictionary options) {
}
#elif DOXYGEN_PY
Instance Dba::configure_local_instance(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::configure_local_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, 2, get_function_name("configureLocalInstance").c_str());

  try {
    auto instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    // Resolves user and validates password
    mysqlsh::resolve_connection_credentials(&instance_def);

    shcore::Value::Map_type_ref options;
    if (args.size() == 2)
      options = args.map_at(1);

    if (instance_def.has_host()) {
      if (shcore::is_local_host(instance_def.get_host(), true)) {
        ret_val = shcore::Value(
            _check_instance_configuration(instance_def, options, true));
      } else {
        throw shcore::Exception::runtime_error(
            "This function only works with local instances");
      }
    } else {
      throw shcore::Exception::runtime_error(
          "This function requires a TCP connection, host is missing.");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("configureLocalInstance"));

  return ret_val;
}

std::shared_ptr<mysqlshdk::db::ISession> Dba::get_session(
    const mysqlshdk::db::Connection_options &args) {
  std::shared_ptr<mysqlshdk::db::ISession> ret_val;

  ret_val = mysqlshdk::db::mysql::Session::create();

  ret_val->connect(args);

  return ret_val;
}

shcore::Value::Map_type_ref Dba::_check_instance_configuration(
    const mysqlshdk::db::Connection_options &instance_def,
    const shcore::Value::Map_type_ref &options, bool allow_update) {
  shcore::Value::Map_type_ref ret_val(new shcore::Value::Map_type());
  shcore::Value::Array_type_ref errors(new shcore::Value::Array_type());

  shcore::Argument_map validate_opt_map;

  shcore::Value::Map_type_ref validate_options;

  std::string cnfpath, cluster_admin, cluster_admin_password;
  std::string admin_user, admin_user_host, validate_user, validate_host;
  std::string current_user, current_host, account_type, error_msg_extra;
  std::string uri_user = instance_def.get_user();
  std::string uri_host = instance_def.get_host();
  bool cluster_admin_user_exists = false;
  bool clear_read_only = false;

  if (options) {
    shcore::Argument_map tmp_map(*options);

    std::set<std::string> check_options {"password", "dbPassword", "mycnfPath"};
    // The clearReadOnly option is only available in configureLocalInstance
    // i.e. with allow_update set as true
    if (allow_update) {
      check_options.insert("clusterAdmin");
      check_options.insert("clusterAdminPassword");
      check_options.insert("clearReadOnly");
    }
    tmp_map.ensure_keys({}, check_options, "validation options");
    validate_opt_map = tmp_map;

    // Retrieves the .cnf path if exists or leaves it empty so the default is
    // set afterwards
    if (validate_opt_map.has_key("mycnfPath"))
      cnfpath = validate_opt_map.string_at("mycnfPath");

    // Retrives the clusterAdmin if exists or leaves it empty so the default is
    // set afterwards
    if (validate_opt_map.has_key("clusterAdmin"))
      cluster_admin = validate_opt_map.string_at("clusterAdmin");

    // Retrieves the clusterAdminPassword if exists or leaves it empty so the
    // default is set afterwards
    if (validate_opt_map.has_key("clusterAdminPassword"))
      cluster_admin_password =
          validate_opt_map.string_at("clusterAdminPassword");

    if (allow_update) {
      if (validate_opt_map.has_key("clearReadOnly"))
          clear_read_only = validate_opt_map.bool_at("clearReadOnly");
    }
  }

  if (cnfpath.empty() && allow_update)
    throw shcore::Exception::runtime_error(
        "The path to the MySQL Configuration is required to verify and fix the "
        "InnoDB Cluster settings");

  // Now validates the instance GR status itself
  auto session = Dba::get_session(instance_def);

  // get the current user
  auto result = session->query("SELECT CURRENT_USER()");
  auto row = result->fetch_one();
  std::string current_account = row->get_string(0);
  split_account(current_account, &current_user, &current_host, true);

  // if this is a configureLocalInstance operation and the clusterAdmin
  // option was used and that user exists, validate its privileges, otherwise
  // validate the privileges of the current user
  if (allow_update && !cluster_admin.empty()) {
    shcore::split_account(cluster_admin, &admin_user, &admin_user_host, false);
    // Host '%' is used by default if not provided in the user account.
    if (admin_user_host.empty())
      admin_user_host = "%";
    // Check if the cluster_admin user exists
    shcore::sqlstring query = shcore::sqlstring(
        "SELECT COUNT(*) from mysql.user WHERE "
        "User = ? AND Host = ?", 0);
    query << admin_user << admin_user_host;
    query.done();
    try {
      result = session->query(query);
      cluster_admin_user_exists =
          result->fetch_one()->get_int(0) != 0;
    } catch (mysqlshdk::db::Error &err) {
      if (err.code() == ER_TABLEACCESS_DENIED_ERROR) {
        // the current_account doesn't have enough privileges to execute
        // the query
        std::string error_msg;
        if (uri_host != current_host || uri_user != current_user)
          error_msg =
              "Session account '" + current_user + "'@'" + current_host +
              "'(used to authenticate '" + uri_user + "'@'" + uri_host +
              "') does not have all the required privileges to execute this "
              "operation. For more information, see the online documentation.";
        else
          error_msg =
              "Session account '" + current_user + "'@'" + current_host +
              "' does not have all the required privileges to execute this "
              "operation. For more information, see the online documentation.";
        log_error("%s", error_msg.c_str());
        throw std::runtime_error(error_msg);
      } else {
        throw;
      }
    }
  }
  if (cluster_admin_user_exists) {
    // cluster admin account exists, so we will validate its privileges
    validate_user = admin_user;
    validate_host = admin_user_host;
    account_type = "Cluster Admin";
  } else {
    // cluster admin doesn't exist, so we validate the privileges of the
    // current user
    validate_user = current_user;
    validate_host = current_host;
    account_type = "Session";
    if (uri_host != current_host || uri_user != current_user)
      error_msg_extra =
          " (used to authenticate '" + uri_user + "'@'" + uri_host + "')";
  }
  // Validate the permissions of the user running the operation.
  if (!validate_cluster_admin_user_privileges(session, validate_user,
                                              validate_host)) {
    std::string error_msg =
        account_type + " account '" + validate_user + "'@'" + validate_host +
        "'" + error_msg_extra +
        " does not have all the required privileges to "
        "execute this operation. For more information, see the online "
        "documentation.";
    log_error("%s", error_msg.c_str());
    throw std::runtime_error(error_msg);
  }
  // Now validates the instance GR status itself
  std::string uri = session->uri(mysqlshdk::db::uri::formats::only_transport());

  GRInstanceType type = get_gr_instance_type(session);

  if (type == GRInstanceType::GroupReplication) {
    session->close();
    throw shcore::Exception::runtime_error(
        "The instance '" + uri + "' is already part of a Replication Group");

  // configureLocalInstance is allowed even if the instance is part of the
  // InnoDB cluster checkInstanceConfiguration is not
  } else if (type == GRInstanceType::InnoDBCluster && !allow_update) {
    session->close();
    throw shcore::Exception::runtime_error(
        "The instance '" + uri + "' is already part of an InnoDB Cluster");
  } else {
    if (!cluster_admin.empty() && allow_update) {
      // Check if super_read_only is turned off and disable it if required
      // NOTE: this is left for last to avoid setting super_read_only to true
      // and right before some execution failure of the command leaving the
      // instance in an incorrect state
      bool super_read_only = validate_super_read_only(session, clear_read_only);
      if (!cluster_admin_user_exists) {
        try {
          create_cluster_admin_user(session, cluster_admin,
                                    cluster_admin_password);
        } catch (shcore::Exception &err) {
          std::string error_msg = "Unexpected error creating " + cluster_admin +
                                  " user: " + err.what();
          errors->push_back(shcore::Value(error_msg));
          log_error("%s", error_msg.c_str());
        }
        // If we disabled super_read_only we must enable it back
        // also confirm that the initial status was 1/ON
        if (clear_read_only && super_read_only) {
          log_info("Enabling super_read_only on the instance '%s'",
                  uri.c_str());
          set_global_variable(session, "super_read_only", "ON");
        }
      } else {
        log_warning("User %s already exists.", cluster_admin.c_str());
      }
    }

    if (type == GRInstanceType::InnoDBCluster) {
      auto seeds =
          get_peer_seeds(session,
                         instance_def.as_uri(only_transport()));

      auto peer_seeds = shcore::str_join(seeds, ",");
      set_global_variable(session,
                          "group_replication_group_seeds", peer_seeds);
    }

    std::string user;
    std::string password;
    // If a clusterAdmin account was specified, use it, otherwise user the
    // user of the instance.
    if (!cluster_admin.empty()) {
      user = admin_user;
      password = cluster_admin_password;
    } else {
      user = uri_user;
      password = instance_def.get_value(
          instance_def.has("password") ? "password" : "dbPassword");
    }

    // Verbose is mandatory for checkInstanceConfiguration
    shcore::Value::Array_type_ref mp_errors;
    auto check_options = instance_def;
    check_options.clear_user();
    check_options.clear_password();
    check_options.set_user(user);
    check_options.set_password(password);
    if ((_provisioning_interface->check(check_options, cnfpath,
                                        allow_update, &mp_errors) == 0)) {
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

            if (error_str.find("The operation could not continue due to the "
                               "following requirements not being met") !=
                std::string::npos) {
              auto lines = shcore::split_string(error_str, "\n");

              bool loading_options = false;

              shcore::Value::Map_type_ref server_options(
                  new shcore::Value::Map_type());
              std::string option_type;

              for (size_t index = 1; index < lines.size(); index++) {
                if (loading_options) {
                  auto option_tokens =
                      shcore::split_string(lines[index], " ", true);

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
                      (*server_options)[option_tokens[0]] =
                          shcore::Value(option);
                    } else {
                      option = (*server_options)[option_tokens[0]].as_map();
                    }

                    (*option)["required"] =
                        shcore::Value(option_tokens[1]);  // The required value
                    (*option)[option_type] =
                        shcore::Value(option_tokens[2]);  // The current value
                  }
                } else {
                  if (lines[index].find("Some active options on server") !=
                      std::string::npos) {
                    option_type = "server";
                    loading_options = true;
                    index += 3;  // Skips to the actual option table
                  } else if (lines[index].find("Some of the configuration "
                                               "values on your options file") !=
                             std::string::npos) {
                    option_type = "config";
                    loading_options = true;
                    index += 3;  // Skips to the actual option table
                  }
                }
              }

              if (server_options->size()) {
                shcore::Value::Array_type_ref config_errors(
                    new shcore::Value::Array_type());
                for (auto option : *server_options) {
                  auto state = option.second.as_map();

                  std::string required_value = state->get_string("required");
                  std::string server_value = state->get_string("server", "");
                  std::string config_value = state->get_string("config", "");

                  // Taken from MP, reading docs made me think all variables
                  // should require restart Even several of them are dynamic, it
                  // seems changing values may lead to problems An
                  // extransaction_write_set_extraction which apparently is
                  // reserved for future use So I just took what I saw on the MP
                  // code Source:
                  // http://dev.mysql.com/doc/refman/5.7/en/dynamic-system-variables.html
                  std::vector<std::string> dynamic_variables = {
                      "binlog_format", "binlog_checksum", "slave_parallel_type",
                      "slave_preserve_commit_order"};

                  bool dynamic =
                      std::find(dynamic_variables.begin(),
                                dynamic_variables.end(),
                                option.first) != dynamic_variables.end();

                  std::string action;
                  std::string current;
                  if (!server_value.empty() &&
                      !config_value.empty()) {  // Both server and configuration
                                                // are wrong
                    if (dynamic)
                      action = "server_update+config_update";
                    else {
                      action = "config_update+restart";
                      restart_required = true;
                    }

                    current = server_value;
                  } else if (!config_value.empty()) {  // Configuration is
                                                       // wrong, server is OK
                    action = "config_update";
                    current = config_value;
                  } else if (!server_value.empty()) {  // Server is wronf,
                                                       // configuration is OK
                    if (dynamic)
                      action = "server_update";
                    else {
                      action = "restart";
                      restart_required = true;
                    }
                    current = server_value;
                  }

                  shcore::Value::Map_type_ref error(
                      new shcore::Value::Map_type());

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

REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_BRIEF,
              "Brings a cluster back ONLINE when all members are OFFLINE.");
REGISTER_HELP(
    DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_PARAM,
    "@param clusterName Optional The name of the cluster to be rebooted.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_PARAM1,
              "@param options Optional dictionary with options that modify the "
              "behavior of this function.");

REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS,
              "@throws MetadataError  if the Metadata is inaccessible.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS1,
              "@throws ArgumentError if the Cluster name is empty.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS2,
              "@throws ArgumentError if the Cluster name is not valid.");
REGISTER_HELP(
    DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS3,
    "@throws ArgumentError if the options contain an invalid attribute.");
REGISTER_HELP(
    DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS4,
    "@throws RuntimeError if the Cluster does not exist on the Metadata.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS5,
              "@throws RuntimeError if some instance of the Cluster belongs to "
              "a Replication Group.");

REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_RETURNS,
              "@returns The rebooted cluster object.");

REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL,
              "The options dictionary can contain the next values:");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL1,
              "@li password: The password used for the instances sessions "
              "required operations.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL2,
              "@li removeInstances: The list of instances to be removed from "
              "the cluster.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL3,
              "@li rejoinInstances: The list of instances to be rejoined on "
              "the cluster.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL4,
              "@li clearReadOnly: boolean value used to confirm that "\
              "super_read_only must be disabled");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL5,
              "This function reboots a cluster from complete outage. "
              "It picks the instance the MySQL Shell is connected to as new "
              "seed instance and recovers the cluster. "
              "Optionally it also updates the cluster configuration based on "
              "user provided options.");
REGISTER_HELP(
    DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL6,
    "On success, the restored cluster object is returned by the function.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL7,
              "The current session must be connected to a former instance of "
              "the cluster.");
REGISTER_HELP(
    DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL8,
    "If name is not specified, the default cluster will be returned.");

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
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL5)
*
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL6)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL7)
* $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_DETAIL8)
*/
#if DOXYGEN_JS
Undefined Dba::rebootClusterFromCompleteOutage(String clusterName,
                                               Dictionary options) {}
#elif DOXYGEN_PY
None Dba::reboot_cluster_from_complete_outage(str clusterName, dict options) {}
#endif

shcore::Value Dba::reboot_cluster_from_complete_outage(
    const shcore::Argument_list &args) {
  args.ensure_count(
      0, 2, get_function_name("rebootClusterFromCompleteOutage").c_str());

  std::shared_ptr<MetadataStorage> metadata;
  std::shared_ptr<mysqlshdk::db::ISession> group_session;
  // The cluster is completely dead, so we can't find the primary anyway...
  // thus this instance will be used as the primary
  try {
    connect_to_target_group({}, &metadata, &group_session, false);
  }
  CATCH_AND_TRANSLATE_CLUSTER_EXCEPTION(
      get_function_name("rebootClusterFromCompleteOutage"));

  shcore::Value ret_val;
  std::string instance_session_address;
  shcore::Value::Map_type_ref options;
  std::shared_ptr<mysqlsh::dba::Cluster> cluster;
  std::shared_ptr<mysqlsh::dba::ReplicaSet> default_replicaset;
  Value::Array_type_ref remove_instances_ref, rejoin_instances_ref;
  std::vector<std::string> remove_instances_list, rejoin_instances_list,
      instances_lists_intersection;

  check_preconditions(group_session, "rebootClusterFromCompleteOutage");

  std::string cluster_name;
  try {
    bool default_cluster = false;
    bool clear_read_only = false;

    if (args.size() == 0) {
      default_cluster = true;
    } else if (args.size() == 1) {
      cluster_name = args.string_at(0);
    } else {
      cluster_name = args.string_at(0);
      options = args.map_at(1);
    }

    // These session options are taken as base options for further operations
    auto current_session_options = group_session->get_connection_options();

    // Get the current session instance address
    instance_session_address =
        current_session_options.as_uri(only_transport());

    if (options) {
      mysqlsh::set_user_from_map(&current_session_options, options);
      mysqlsh::set_password_from_map(&current_session_options, options);

      shcore::Argument_map opt_map(*options);

      // Case sensitive validation of the rest of the options, at this point the
      // user and password should have been already removed
      opt_map.ensure_keys({}, mysqlsh::dba::Dba::_reboot_cluster_opts,
                          "the options");

      if (opt_map.has_key("removeInstances"))
        remove_instances_ref = opt_map.array_at("removeInstances");

      if (opt_map.has_key("rejoinInstances"))
        rejoin_instances_ref = opt_map.array_at("rejoinInstances");

      if (opt_map.has_key("clearReadOnly"))
        clear_read_only = opt_map.bool_at("clearReadOnly");
    }

    // Check if removeInstances and/or rejoinInstances are specified
    // And if so add them to simple vectors so the check for types is done
    // before moving on in the function logic
    if (remove_instances_ref) {
      for (auto value : *remove_instances_ref.get()) {
        // Check if seed instance is present on the list
        if (value.as_string() == instance_session_address)
          throw shcore::Exception::argument_error(
              "The current session instance "
              "cannot be used on the 'removeInstances' list.");

        remove_instances_list.push_back(value.as_string());
      }
    }

    if (rejoin_instances_ref) {
      for (auto value : *rejoin_instances_ref.get()) {
        // Check if seed instance is present on the list
        if (value.as_string() == instance_session_address)
          throw shcore::Exception::argument_error(
              "The current session instance "
              "cannot be used on the 'rejoinInstances' list.");
        rejoin_instances_list.push_back(value.as_string());
      }
    }

    // Check if there is an intersection of the two lists.
    // Sort the vectors because set_intersection works on sorted collections
    std::sort(remove_instances_list.begin(), remove_instances_list.end());
    std::sort(rejoin_instances_list.begin(), rejoin_instances_list.end());

    std::set_intersection(
        remove_instances_list.begin(), remove_instances_list.end(),
        rejoin_instances_list.begin(), rejoin_instances_list.end(),
        std::back_inserter(instances_lists_intersection));

    if (!instances_lists_intersection.empty()) {
      std::string list;

      list = shcore::str_join(instances_lists_intersection, ", ");

      throw shcore::Exception::argument_error(
          "The following instances: '" + list +
          "' belong to both 'rejoinInstances' and 'removeInstances' lists.");
    }

    cluster.reset(new Cluster(cluster_name, group_session, metadata));

    // Getting the cluster from the metadata already complies with:
    // 1. Ensure that a Metadata Schema exists on the current session instance.
    // 3. Ensure that the provided cluster identifier exists on the Metadata
    // Schema
    if (default_cluster) {
      metadata->load_default_cluster(cluster);
    } else {
      metadata->load_cluster(cluster_name, cluster);
    }

    if (cluster) {
      // Set the provision interface pointer
      cluster->set_provisioning_interface(_provisioning_interface);

      // Set the cluster as return value
      ret_val =
          shcore::Value(std::dynamic_pointer_cast<Object_bridge>(cluster));

      // Get the default replicaset
      default_replicaset = cluster->get_default_replicaset();

      // 2. Ensure that the current session instance exists on the Metadata
      // Schema
      if (!metadata->is_instance_on_replicaset(
              default_replicaset->get_id(), instance_session_address))
        throw Exception::runtime_error(
            "The current session instance does not belong "
            "to the cluster: '" +
            cluster->get_name() + "'.");

      // Ensure that all of the instances specified on the 'removeInstances'
      // list exist on the Metadata Schema and are valid
      for (const auto &value : remove_instances_list) {
        shcore::Argument_list args;
        args.push_back(shcore::Value(value));

        try {
          auto instance_def = mysqlsh::get_connection_options(
              args, mysqlsh::PasswordFormat::NONE);
        } catch (std::exception &e) {
          std::string error(e.what());
          throw shcore::Exception::argument_error(
              "Invalid value '" + value + "' for 'removeInstances': " + error);
        }

        if (!metadata->is_instance_on_replicaset(default_replicaset->get_id(),
                                                 value))
          throw Exception::runtime_error("The instance '" + value +
                                         "' does not belong "
                                         "to the cluster: '" +
                                         cluster->get_name() + "'.");
      }

      // Ensure that all of the instances specified on the 'rejoinInstances'
      // list exist on the Metadata Schema and are valid
      for (const auto &value : rejoin_instances_list) {
        shcore::Argument_list args;
        args.push_back(shcore::Value(value));

        try {
          auto instance_def = mysqlsh::get_connection_options(
              args, mysqlsh::PasswordFormat::NONE);
        } catch (std::exception &e) {
          std::string error(e.what());
          throw shcore::Exception::argument_error(
              "Invalid value '" + value + "' for 'rejoinInstances': " + error);
        }

        if (!metadata->is_instance_on_replicaset(default_replicaset->get_id(),
                                                 value))
          throw Exception::runtime_error("The instance '" + value +
                                         "' does not belong "
                                         "to the cluster: '" +
                                         cluster->get_name() + "'.");
      }
      // Get the all the instances and their status
      std::vector<std::pair<std::string, std::string>> instances_status =
          get_replicaset_instances_status(cluster, options);

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

        list = shcore::str_join(non_reachable_rejoin_instances, ", ");

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
    if (metadata->is_cluster_empty(cluster->get_id()))
      throw Exception::runtime_error("The cluster has no instances in it.");

    // 4. Verify the status of all instances of the cluster:
    // 4.1 None of the instances can belong to a GR Group
    // 4.2 If any of the instances belongs to a GR group or is already managed
    // by the InnoDB Cluster, so include that information on the error message
    validate_instances_status_reboot_cluster(cluster, group_session, options);

    // 5. Verify which of the online instances has the GTID superset.
    // 5.1 Skip the verification on the list of instances to be removed:
    // "removeInstances" 5.2 If the current session instance doesn't have the
    // GTID superset, error out with that information and including on the
    // message the instance with the GTID superset
    validate_instances_gtid_reboot_cluster(cluster, options, group_session);

    // 6. Set the current session instance as the seed instance of the Cluster
    {
      // Check if super_read_only is turned off and disable it if required
      // NOTE: this is left for last to avoid setting super_read_only to true
      // and right before some execution failure of the command leaving the
      // instance in an incorrect state
      validate_super_read_only(group_session, clear_read_only);

      shcore::Argument_list new_args;
      std::string replication_user, replication_user_password;
      // A new replication user and password must be created
      // so we pass an empty string to the MP call
      replication_user = "";
      replication_user_password = "";

      // The current 'group_replication_group_name' must be kept otherwise
      // if instances are rejoined later the operation may fail because
      // a new group_name started being used.
      // This must be done before rejoining the instances due to the fixes for
      // BUG #26159339: SHELL: ADMINAPI DOES NOT TAKE GROUP_NAME INTO ACCOUNT
      std::string current_group_replication_group_name =
          default_replicaset->get_group_name();

      default_replicaset->add_instance(current_session_options, new_args,
                                       replication_user,
                                       replication_user_password, true,
                                       current_group_replication_group_name);
    }

    // 7. Update the Metadata Schema information
    // 7.1 Remove the list of instances of "removeInstances" from the Metadata
    default_replicaset->remove_instances(remove_instances_list);

    // 8. Rejoin the list of instances of "rejoinInstances"
    default_replicaset->rejoin_instances(rejoin_instances_list, options);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("rebootClusterFromCompleteOutage"));

  return ret_val;
}

ReplicationGroupState Dba::check_preconditions(
    std::shared_ptr<mysqlshdk::db::ISession> group_session,
    const std::string &function_name) const {
  try {
    return check_function_preconditions(class_name(), function_name,
                                        get_function_name(function_name),
                                        group_session);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(function_name));
  return ReplicationGroupState{};
}

/*
 * get_replicaset_instances_status:
 *
 * Given a cluster, this function verifies the connectivity status of all the
 * instances of the default replicaSet of the cluster. It returns a list of
 * pairs <instance_id, status>, on which 'status' is empty if the instance is
 * reachable, or if not reachable contains the connection failure error message
 */
std::vector<std::pair<std::string, std::string>>
Dba::get_replicaset_instances_status(std::shared_ptr<Cluster> cluster,
    const shcore::Value::Map_type_ref &options) {
  std::vector<std::pair<std::string, std::string>> instances_status;
  std::string active_session_address, instance_address, conn_status;

  // TODO(alfredo) This should be in the Cluster object

  log_info("Checking instance status for cluster '%s'",
           cluster->get_name().c_str());

  std::vector<Instance_definition> instances =
      cluster->get_default_replicaset()->get_instances_from_metadata();

  auto current_session_options =
      cluster->get_group_session()->get_connection_options();

  // Get the current session instance address
  active_session_address =
      current_session_options.as_uri(only_transport());

  if (options) {
    // Check if the password is specified on the options and if not prompt it
    mysqlsh::set_user_from_map(&current_session_options, options);
    mysqlsh::set_password_from_map(&current_session_options, options);
  }

  // Iterate on all instances from the metadata
  for (const auto &it : instances) {
    instance_address = it.endpoint;
    conn_status.clear();

    // Skip the current session instance
    if (instance_address == active_session_address) {
      continue;
    }

    auto connection_options =
        shcore::get_connection_options(instance_address, false);

    connection_options.set_user(current_session_options.get_user());
    connection_options.set_password(current_session_options.get_password());

    try {
      log_info(
          "Opening a new session to the instance to determine its status: %s",
          instance_address.c_str());
      auto session = get_session(connection_options);
      session->close();
    } catch (std::exception &e) {
      conn_status = e.what();
      log_warning("Could not open connection to %s: %s.",
                  instance_address.c_str(), e.what());
    }

    // Add the <instance, connection_status> pair to the list
    instances_status.emplace_back(instance_address, conn_status);
  }

  return instances_status;
}


static void validate_instance_belongs_to_cluster(
    std::shared_ptr<mysqlshdk::db::ISession> member_session,
    const std::string &gr_group_name,
    const std::string &restore_function) {

  // TODO(alfredo) gr_group_name should receive the group_name as stored
  // in the metadata to validate if it matches the expected value
  // if the name does not match, an error should be thrown asking for an option
  // like update_mismatched_group_name to be set to ignore the validation error
  // and adopt the new group name

  GRInstanceType type = get_gr_instance_type(member_session);

  std::string member_session_address =
      member_session->get_connection_options().as_uri(only_transport());

  switch (type) {
    case GRInstanceType::InnoDBCluster:
      throw Exception::runtime_error(
          "The MySQL instance '" + member_session_address +
          "' belongs "
          "to an InnoDB Cluster and is reachable. Please use <Cluster>." +
          restore_function + "() to restore from the quorum loss.");

    case GRInstanceType::GroupReplication:
      throw Exception::runtime_error("The MySQL instance '" +
                                     member_session_address +
                                     "' belongs "
                                     "GR group that is not managed as an "
                                     "InnoDB cluster. ");

    case Standalone:
      // We only want to check whether the status if InnoDBCluster or
      // GroupReplication to stop and thrown an exception
      break;

    case Any:
      // FIXME(anyone) to silence warning... should not use a enum as a bitmask
      assert(0);
      break;
  }
}

/*
 * validate_instances_status_reboot_cluster:
 *
 * This function is an auxiliary function to be used for the reboot_cluster
 * operation. It verifies the status of all the instances of the cluster
 * referent to the arguments list. Firstly, it verifies the status of the
 * current session instance to determine if it belongs to a GR group or is
 * already managed by the InnoDB Cluster.cluster_name If not, does the same
 * validation for the remaining reachable instances of the cluster.
 */
void Dba::validate_instances_status_reboot_cluster(
    std::shared_ptr<Cluster> cluster,
    std::shared_ptr<mysqlshdk::db::ISession> member_session,
    shcore::Value::Map_type_ref options) {
  // Validate the member we're connected to
  validate_instance_belongs_to_cluster(
      member_session, "",
      get_member_name("forceQuorumUsingPartitionOf", naming_style));

  mysqlshdk::db::Connection_options member_connection_options =
      member_session->get_connection_options();

  // Get the current session instance address
  if (options) {
    mysqlsh::set_user_from_map(&member_connection_options, options);
    mysqlsh::set_password_from_map(&member_connection_options, options);

    shcore::Argument_map opt_map(*options);

    // Case sensitive validation of the rest of the options, at this point the
    // user and password should have been already removed
    opt_map.ensure_keys({}, mysqlsh::dba::Dba::_reboot_cluster_opts,
                        "the options");
  }

  // Verify all the remaining online instances for their status
  std::vector<std::pair<std::string, std::string>> instances_status =
      get_replicaset_instances_status(cluster, options);

  for (const auto &value : instances_status) {
    std::string instance_address = value.first;
    std::string instance_status = value.second;

    // if the status is not empty it means the connection failed
    // so we skip this instance
    if (!instance_status.empty()) {
      continue;
    }

    mysqlshdk::db::Connection_options connection_options =
        shcore::get_connection_options(instance_address, false);
    connection_options.set_user(member_connection_options.get_user());
    connection_options.set_password(member_connection_options.get_password());

    std::shared_ptr<mysqlshdk::db::ISession> session;
    try {
      log_info("Opening a new session to the instance: %s",
               instance_address.c_str());
      session = get_session(connection_options);
    } catch (std::exception &e) {
      throw Exception::runtime_error("Could not open connection to " +
                                     instance_address + "");
    }

    log_info("Checking state of instance '%s'", instance_address.c_str());
    validate_instance_belongs_to_cluster(
        session, "",
        get_member_name("forceQuorumUsingPartitionOf", naming_style));
    session->close();
  }
}

/*
 * validate_instances_gtid_reboot_cluster:
 *
 * This function is an auxiliary function to be used for the reboot_cluster
 * operation. It verifies which of the online instances of the cluster has the
 * GTID superset. If the current session instance doesn't have the GTID
 * superset, it errors out with that information and includes on the error
 * message the instance with the GTID superset
 */
void Dba::validate_instances_gtid_reboot_cluster(
    std::shared_ptr<Cluster> cluster,
    const shcore::Value::Map_type_ref &options,
    const std::shared_ptr<mysqlshdk::db::ISession> &instance_session) {
  /* GTID verification is done by verifying which instance has the GTID
   * superset.the In order to do so, a union of the global gtid executed and the
   * received transaction set must be done using:
   *
   * CREATE FUNCTION GTID_UNION(g1 TEXT, g2 TEXT)
   *  RETURNS TEXT DETERMINISTIC
   *  RETURN CONCAT(g1,',',g2);
   *
   * Instance 1:
   *
   * A: select @@GLOBAL.GTID_EXECUTED
   * B: SELECT RECEIVED_TRANSACTION_SET FROM
   *    performance_schema.replication_connection_status where
   * CHANNEL_NAME="group_replication_applier";
   *
   * Total = A + B (union)
   *
   * SELECT GTID_SUBSET("Total_instance1", "Total_instance2")
   */

  std::pair<std::string, std::string> most_updated_instance;
  std::shared_ptr<mysqlshdk::db::ISession> session;
  std::string active_session_address;

  // get the current session information
  auto current_session_options = instance_session->get_connection_options();

  // Get the current session instance address
  active_session_address = current_session_options.as_uri(only_transport());

  if (options) {
    mysqlsh::set_user_from_map(&current_session_options, options);
    mysqlsh::set_password_from_map(&current_session_options, options);
  }

  // Get the cluster instances and their status
  std::vector<std::pair<std::string, std::string>> instances_status =
      get_replicaset_instances_status(cluster, options);

  // Get @@GLOBAL.GTID_EXECUTED
  std::string gtid_executed_current;
  get_server_variable(instance_session, "GLOBAL.GTID_EXECUTED",
                      gtid_executed_current);

  std::string msg = "The current session instance GLOBAL.GTID_EXECUTED is: " +
                    gtid_executed_current;
  log_info("%s", msg.c_str());

  // Create a pair vector to store all the GTID_EXECUTED
  std::vector<std::pair<std::string, std::string>> gtids;

  // Insert the current session info
  gtids.emplace_back(active_session_address, gtid_executed_current);

  // Update most_updated_instance with the current session instance value
  most_updated_instance =
      std::make_pair(active_session_address, gtid_executed_current);

  for (const auto &value : instances_status) {
    std::string instance_address = value.first;
    std::string instance_status = value.second;

    // if the status is not empty it means the connection failed
    // so we skip this instance
    if (!instance_status.empty())
      continue;

    auto connection_options =
        shcore::get_connection_options(instance_address, false);
    connection_options.set_user(current_session_options.get_user());
    connection_options.set_password(current_session_options.get_password());

    // Connect to the instance to obtain the GLOBAL.GTID_EXECUTED
    try {
      log_info("Opening a new session to the instance for gtid validations %s",
               instance_address.c_str());
      session = get_session(connection_options);
    } catch (std::exception &e) {
      throw Exception::runtime_error("Could not open a connection to " +
                                     instance_address + ": " + e.what() + ".");
    }

    std::string gtid_executed;

    // Get @@GLOBAL.GTID_EXECUTED
    get_server_variable(session, "GLOBAL.GTID_EXECUTED",
                        gtid_executed);

    // Close the session
    session->close();

    std::string msg = "The instance: '" + instance_address +
                      "' GLOBAL.GTID_EXECUTED is: " + gtid_executed;
    log_info("%s", msg.c_str());

    // Add to the pair vector of gtids
    gtids.emplace_back(instance_address, gtid_executed);
  }

  // Calculate the most up-to-date instance
  // TODO(miguel): calculate the Total GTID executed. See comment above
  for (auto &value : gtids) {
    // Compare the gtid's: SELECT GTID_SUBSET("Total_instance1",
    // "Total_instance2")
    if (!is_gtid_subset(instance_session, value.second,
                        most_updated_instance.second))
      most_updated_instance = value;
  }

  // Check if the most updated instance is not the current session instance
  if (!(most_updated_instance.first == active_session_address)) {
    throw Exception::runtime_error(
        "The active session instance isn't the most updated "
        "in comparison with the ONLINE instances of the Cluster's "
        "metadata. Please use the most up to date instance: '" +
        most_updated_instance.first + "'.");
  }
}
