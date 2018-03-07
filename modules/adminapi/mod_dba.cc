/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <mysqld_error.h>
#include <algorithm>
#include <memory>
#include <random>
#include <string>

#ifndef WIN32
#include <sys/un.h>
#endif
#include "modules/adminapi/mod_dba.h"

#include <iterator>
#include <utility>
#include <vector>

#include "modules/adminapi/dba/validations.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_shell.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "scripting/object_factory.h"
#include "shellcore/utils_help.h"
#include "utils/utils_general.h"
#include "utils/utils_sqlstring.h"

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/innodbcluster/cluster.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"

#include "modules/adminapi/dba/check_instance.h"
#include "modules/adminapi/dba/configure_instance.h"
#include "modules/adminapi/dba/configure_local_instance.h"

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

namespace mysqlsh {
namespace dba {

using namespace shcore;

#define PASSWORD_LENGHT 16
using mysqlshdk::db::uri::formats::only_transport;
std::set<std::string> Dba::_deploy_instance_opts = {
    "portx",         "sandboxDir",     "password",
    "allowRootFrom", "ignoreSslError", "mysqldOptions"};
std::set<std::string> Dba::_stop_instance_opts = {"sandboxDir", "password"};
std::set<std::string> Dba::_default_local_instance_opts = {"sandboxDir"};

std::set<std::string> Dba::_create_cluster_opts = {
    "multiMaster",   "adoptFromGR",  "force",
    "memberSslMode", "ipWhitelist",  "clearReadOnly",
    "groupName",     "localAddress", "groupSeeds"};
std::set<std::string> Dba::_reboot_cluster_opts = {
    "user",         "dbUser", "password", "removeInstances", "rejoinInstances",
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

Dba::Dba(shcore::IShell_core *owner,
         std::shared_ptr<mysqlsh::IConsole> console_handler, bool wizards_mode)
    : _shell_core(owner),
      m_console(console_handler),
      m_wizards_mode(wizards_mode) {
  init();
}

Dba::~Dba() {}

bool Dba::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Dba::init() {
  add_property("verbose");

  // Pure functions
  add_method("createCluster", std::bind(&Dba::create_cluster, this, _1),
             "clusterName", shcore::String);
  add_method("getCluster", std::bind(&Dba::get_cluster_, this, _1),
             "clusterName", shcore::String);
  add_method("dropMetadataSchema",
             std::bind(&Dba::drop_metadata_schema, this, _1), "data",
             shcore::Map);
  add_method("checkInstanceConfiguration",
             std::bind(&Dba::check_instance_configuration, this, _1), "data",
             shcore::Map);
  add_method("deploySandboxInstance",
             std::bind(&Dba::deploy_sandbox_instance, this, _1,
                       "deploySandboxInstance"),
             "data", shcore::Map);
  add_method("startSandboxInstance",
             std::bind(&Dba::start_sandbox_instance, this, _1), "data",
             shcore::Map);
  add_method("stopSandboxInstance",
             std::bind(&Dba::stop_sandbox_instance, this, _1), "data",
             shcore::Map);
  add_method("deleteSandboxInstance",
             std::bind(&Dba::delete_sandbox_instance, this, _1), "data",
             shcore::Map);
  add_method("killSandboxInstance",
             std::bind(&Dba::kill_sandbox_instance, this, _1), "data",
             shcore::Map);
  add_method("configureLocalInstance",
             std::bind(&Dba::configure_local_instance, this, _1), "data",
             shcore::Map);
  add_method("configureInstance", std::bind(&Dba::configure_instance, this, _1),
             "data", shcore::Map);
  add_varargs_method(
      "rebootClusterFromCompleteOutage",
      std::bind(&Dba::reboot_cluster_from_complete_outage, this, _1));

  std::string local_mp_path = mysqlsh::Base_shell::options().gadgets_path;

  if (local_mp_path.empty()) local_mp_path = shcore::get_mp_path();

  _provisioning_interface.reset(
      new ProvisioningInterface(_shell_core->get_delegate(), local_mp_path));
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
        if (owns_target_member_session) target_member_session->close();
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
              "MetadataError in the following scenarios:");
REGISTER_HELP(DBA_GETCLUSTER_THROWS1, "@li If the Metadata is inaccessible.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS2,
              "@li If the Metadata update operation failed.");

REGISTER_HELP(DBA_GETCLUSTER_THROWS3,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_GETCLUSTER_THROWS4, "@li If the Cluster name is empty.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS5, "@li If the Cluster name is invalid.");
REGISTER_HELP(DBA_GETCLUSTER_THROWS6, "@li If the Cluster does not exist.");

REGISTER_HELP(DBA_GETCLUSTER_THROWS7,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(DBA_GETCLUSTER_THROWS8, "@li If the current connection cannot be "
              "used for Group Replication.");

REGISTER_HELP(DBA_GETCLUSTER_RETURNS,
              "@returns The cluster object identified "
              "by the given name or the default "
              "cluster.");
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
 * $(DBA_GETCLUSTER_THROWS5)
 * $(DBA_GETCLUSTER_THROWS6)
 * $(DBA_GETCLUSTER_THROWS7)
 * $(DBA_GETCLUSTER_THROWS8)
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
    check_function_preconditions("Dba.getCluster", get_active_shell_session());

    if (args.size() > 1) {
      shcore::Argument_map options(*args[1].as_map());

      options.ensure_keys({}, {"connectToPrimary"}, "the options");

      if (options.has_key("connectToPrimary")) fallback_to_anything = false;

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

    return shcore::Value(
        get_cluster(default_cluster ? nullptr : cluster_name.c_str(), metadata,
                    group_session));
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
      new Cluster("", group_session, metadata, m_console));

  if (!name) {
    // Reloads the cluster (to avoid losing _default_cluster in case of error)
    metadata->load_default_cluster(cluster);
  } else {
    metadata->load_cluster(name, cluster);
  }
  // Verify if the current session instance group_replication_group_name
  // value differs from the one registered in the Metadata
  if (!validate_replicaset_group_name(
          group_session, cluster->get_default_replicaset()->get_group_name())) {
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
              "MetadataError in the following scenarios:");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS1,
              "@li If the Metadata is inaccessible.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS2,
              "@li If the Metadata update operation failed.");

REGISTER_HELP(DBA_CREATECLUSTER_THROWS3,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS4, "@li If the Cluster name is empty.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS5,
              "@li If the Cluster name is not valid.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS6,
              "@li If the options contain an invalid attribute.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS7,
              "@li If adoptFromGR "
              "is true and the memberSslMode option "
              "is used.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS8,
              "@li If the value "
              "for the memberSslMode option is not "
              "one of the allowed.");

REGISTER_HELP(DBA_CREATECLUSTER_THROWS9,
              "@li If adoptFromGR "
              "is true and the multiMaster option "
              "is used.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS10,
              "@li If the value for the ipWhitelist, "
              "groupName, localAddress, or groupSeeds options is empty.");

REGISTER_HELP(DBA_CREATECLUSTER_THROWS11,
              "RuntimeError in the following scenarios:");

REGISTER_HELP(DBA_CREATECLUSTER_THROWS12,
              "@li If the value for the groupName, "
              "localAddress, or groupSeeds options is not valid for Group "
              "Replication.");
REGISTER_HELP(DBA_CREATECLUSTER_THROWS13,
              "@li If the current connection cannot be used "
              "for Group Replication.");

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
              "@li groupName: string value with the Group Replication group "
              "name UUID to be used instead of the automatically generated "
              "one.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL9,
              "@li localAddress: string value with the Group Replication "
              "local address to be used instead of the automatically "
              "generated one.");
REGISTER_HELP(DBA_CREATECLUSTER_DETAIL10,
              "@li groupSeeds: string value with a comma-separated list of "
              "the Group Replication peer addresses to be used instead of the "
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
              "The groupName, localAddress, and groupSeeds are advanced "
              "options and their usage is discouraged since incorrect values "
              "can lead to Group Replication errors.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL22,
              "The value for groupName is used to set the Group Replication "
              "system variable 'group_replication_group_name'.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL23,
              "The value for localAddress is used to set the Group "
              "Replication system variable 'group_replication_local_address'. "
              "The localAddress option accepts values in the format: "
              "'host:port' or 'host:' or ':port'. If the specified "
              "value does not include a colon (:) and it is numeric, then it "
              "is assumed to be the port, otherwise it is considered to be "
              "the host. When the host is not specified, the default value is "
              "the host of the current active connection (session). When the "
              "port is not specified, the default value is the port of the "
              "current active connection (session) * 10 + 1. In case the "
              "automatically determined default port value is invalid "
              "(> 65535) then a random value in the range [1000, 65535] is "
              "used.");

REGISTER_HELP(DBA_CREATECLUSTER_DETAIL24,
              "The value for groupSeeds is used to set the Group Replication "
              "system variable 'group_replication_group_seeds'. The "
              "groupSeeds option accepts a comma-separated list of addresses "
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
 * $(DBA_CREATECLUSTER_THROWS10)
 * $(DBA_CREATECLUSTER_THROWS11)
 * $(DBA_CREATECLUSTER_THROWS12)
 * $(DBA_CREATECLUSTER_THROWS13)
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

  Cluster_check_info state;

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
  } catch (const mysqlshdk::db::Error &dberr) {
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

      // Validate local address option (group_replication_local_address)
      validate_local_address_option(options);

      // Validate group seeds option
      validate_group_seeds_option(options);

      if (opt_map.has_key("multiMaster"))
        multi_master = opt_map.bool_at("multiMaster");

      if (opt_map.has_key("force")) force = opt_map.bool_at("force");

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

    // TODO(alfredo) - this check might be redundant with the ones done
    // by ensure_instance_configuration_valid()
    // Validate if the connection host is resolved to a supported IP address.
    // NOTE: Not needed when adopting an existing cluster.
    if (!adopt_from_gr) {
      Connection_options seed_cnx_opts =
          group_session->get_connection_options();
      validate_host_ip(seed_cnx_opts.get_host());
    }

    if (state.source_type == GRInstanceType::GroupReplication && !adopt_from_gr)
      throw Exception::argument_error(
          "Creating a cluster on an unmanaged replication group requires "
          "adoptFromGR option to be true");

    if (adopt_from_gr && state.source_type != GRInstanceType::GroupReplication)
      throw Exception::argument_error(
          "The adoptFromGR option is set to true, but there is no replication "
          "group to adopt");

    mysqlshdk::mysql::Instance target_instance(group_session);
    target_instance.cache_global_sysvars();

    if (!adopt_from_gr) {
      // Check instance configuration and state, like dba.checkInstance
      // but skip if we're adopting, since in that case the target is obviously
      // already configured
      ensure_instance_configuration_valid(
          &target_instance, get_provisioning_interface(), m_console);
    }

    m_console->println(
        "Creating InnoDB cluster '" + cluster_name + "' on '" +
        group_session->uri(
            mysqlshdk::db::uri::formats::no_scheme_no_password()) +
        "'...");

    // Check if super_read_only is turned off and disable it if required
    // NOTE: this is left for last to avoid setting super_read_only to true
    // and right before some execution failure of the command leaving the
    // instance in an incorrect state
    validate_super_read_only(group_session, clear_read_only, m_console);

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
        new Cluster(cluster_name, group_session, metadata, m_console));
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
    if (!ssl_mode.empty()) (*options)["memberSslMode"] = Value(ssl_mode);

    // Set IP whitelist
    if (!ip_whitelist.empty()) (*options)["ipWhitelist"] = Value(ip_whitelist);

    // Set local address
    if (!local_address.empty())
      (*options)["localAddress"] = Value(local_address);

    // Set group seeds
    if (!group_seeds.empty()) (*options)["groupSeeds"] = Value(group_seeds);

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
          "https://dev.mysql.com/doc/refman/en/"
          "group-replication-limitations.html");
    }
    cluster->add_seed_instance(instance_def, new_args, multi_master,
                               adopt_from_gr, replication_user, replication_pwd,
                               group_name);

    if (adopt_from_gr) cluster->get_default_replicaset()->adopt_from_gr();

    // If it reaches here, it means there are no exceptions
    ret_val = Value(std::static_pointer_cast<Object_bridge>(cluster));

    tx.commit();
    // We catch whatever to do final processing before bubbling up the exception
  } catch (...) {
    try {
      if (!replication_user.empty()) {
        log_debug("Removing replication user '%s'", replication_user.c_str());
        group_session->query("DROP USER IF EXISTS /*(*/" + replication_user +
                             "/*)*/");
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
              "MetadataError in the following scenarios:");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_THROWS1,
              "@li If the Metadata is inaccessible.");

REGISTER_HELP(DBA_DROPMETADATASCHEMA_THROWS2,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(DBA_DROPMETADATASCHEMA_THROWS3,
              "@li If the current connection cannot be used "
              "for Group Replication.");



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
 * $(DBA_DROPMETADATASCHEMA_THROWS1)
 * $(DBA_DROPMETADATASCHEMA_THROWS2)
 * $(DBA_DROPMETADATASCHEMA_THROWS3)
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

    if (opt_map.has_key("force")) force = opt_map.bool_at("force");

    if (opt_map.has_key("clearReadOnly"))
      clear_read_only = opt_map.bool_at("clearReadOnly");

    if (force) {
      // Check if super_read_only is turned off and disable it if required
      // NOTE: this is left for last to avoid setting super_read_only to true
      // and right before some execution failure of the command leaving the
      // instance in an incorrect state
      validate_super_read_only(group_session, clear_read_only, m_console);

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
              "Validates an instance for MySQL InnoDB Cluster usage.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_PARAM,
              "@param instance An instance definition.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_PARAM1,
              "@param options Optional data for the operation.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS1,
              "@li If the instance parameter is empty.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS2,
              "@li If the instance definition is invalid.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS3,
              "@li If the instance definition is a "
              "connection dictionary but empty.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS4,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS5,
              "@li If the instance accounts are invalid.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS6,
              "@li If the instance is offline.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS7,
              "@li If the instance is already part of a "
              "Replication Group.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS8,
              "@li If the instance is already part of an "
              "InnoDB Cluster.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_THROWS9,
              "@li If the given the instance cannot be used "
              "for Group Replication.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_RETURNS,
              "@returns A descriptive text of the operation result.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL,
              "This function reviews the instance configuration to identify if "
              "it is valid for usage with group replication. Use this to check "
              "for possible configuration issues on MySQL instances before "
              "creating a cluster with them or adding them to an existing "
              "cluster.");
REGISTER_HELP(
    DBA_CHECKINSTANCECONFIGURATION_DETAIL1,
    "The instance definition is the connection data for the instance.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL2,
              "TOPIC_CONNECTION_MORE_INFO_TCP_ONLY");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL3,
              "The options dictionary may contain the following options:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL4,
              "@li mycnfPath: Optional path to the MySQL configuration file "
              "for the instance. Alias for verifyMyCnf");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL5,
            "@li verifyMyCnf: Optional path to the MySQL configuration file "
            "for the instance. If this option is given, the configuration "
            "file will be verified for the expected option values, in "
            "addition to the global MySQL system variables.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL6,
              "@li password: The password to get connected to the instance.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL7,
              "@li interactive: boolean value used to disable the wizards in "
              "the command execution, i.e. prompts are not provided to the "
              "user and confirmation prompts are not shown.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL8,
              "The connection password may be contained on the instance "
              "definition, however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL9,
              "The returned descriptive text of the operation result indicates "
              "whether the instance is valid for InnoDB Cluster usage or not. "
              "If not, a table containing the following information is "
              "presented:");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL10,
              "@li Variable: the invalid configuration variable.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL11,
              "@li Current Value: the current value for the invalid "
              "configuration variable.");
REGISTER_HELP(
    DBA_CHECKINSTANCECONFIGURATION_DETAIL12,
    "@li Required Value: the required value for the configuration variable.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL13,
              "@li Note: the action to be taken.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL14,
              "The note can be one of the following:");

REGISTER_HELP(
    DBA_CHECKINSTANCECONFIGURATION_DETAIL15,
    "@li Update the config file and update or restart the server variable.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL16,
              "@li Update the config file and restart the server.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL17,
              "@li Update the config file.");
REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL18,
              "@li Update the server variable.");

REGISTER_HELP(DBA_CHECKINSTANCECONFIGURATION_DETAIL19,
              "@li Restart the server.");

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
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS7)
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS8)
* $(DBA_CHECKINSTANCECONFIGURATION_THROWS9)
*
* $(DBA_CHECKINSTANCECONFIGURATION_RETURNS)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL1)
*
* Detailed description of the connection data format is available at \ref
* connection_data.
*
* Only TCP/IP connections are allowed for this function.
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL3)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL4)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL5)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL6)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL7)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL8)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL9)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL10)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL11)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL12)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL13)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL14)
*
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL15)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL16)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL17)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL18)
* $(DBA_CHECKINSTANCECONFIGURATION_DETAIL19)
*/
#if DOXYGEN_JS
JSON Dba::checkInstanceConfiguration(InstanceDef instance,
                                          Dictionary options) {}
#elif DOXYGEN_PY
JSON Dba::check_instance_configuration(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::check_instance_configuration(
    const shcore::Argument_list &args) {
  args.ensure_count(0, 2,
                    get_function_name("checkInstanceConfiguration").c_str());
  shcore::Value ret_val;
  mysqlshdk::db::Connection_options instance_def;
  std::shared_ptr<mysqlshdk::db::ISession> instance_session;
  bool interactive = m_wizards_mode;
  std::string mycnf_path, password;

  try {
    if (args.size() == 2) {
      // Retrieves optional options if exists or leaves empty so the default is
      // set afterwards
      Unpack_options(args.map_at(1))
          .optional("mycnfPath", &mycnf_path)
          .optional("verifyMyCnf", &mycnf_path)
          .optional_ci("password", &password)
          .optional("interactive", &interactive)
          .end();
    }

    // If there are no args, the instanceDef is empty (no data)
    if (args.size() != 0 && args[0].type != shcore::Null) {
      instance_def = mysqlsh::get_connection_options(
          args, mysqlsh::PasswordFormat::OPTIONS);
    }

    // If interactive is enabled, resolves user and validates password
    if (interactive) {
      if (instance_def.has_data())
        mysqlsh::resolve_connection_credentials(&instance_def, m_console);
    }

    // Establish the session to the target instance
    if (instance_def.has_data()) {
      validate_connection_options(instance_def);

      instance_session = mysqlshdk::db::mysql::Session::create();
      instance_session->connect(instance_def);
    } else {
      instance_session = connect_to_target_member();
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("checkInstanceConfiguration"));

  check_preconditions(instance_session, "checkInstanceConfiguration");

  try {
    // Create the Instance object with the established session
    mysqlshdk::mysql::Instance target_instance(instance_session);

    target_instance.cache_global_sysvars();

    // Call the API
    std::unique_ptr<Check_instance> op_check_instance(new Check_instance(
        &target_instance, mycnf_path, _provisioning_interface, m_console));

    op_check_instance->prepare();
    ret_val = shcore::Value(op_check_instance->execute());
    op_check_instance->finish();

    // Close the session
    target_instance.close_session();
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
      else
        throw shcore::Exception::argument_error(
            "Missing root password for the deployed instance");
    } else if (function == "stop") {
      opt_map.ensure_keys({}, _stop_instance_opts, "the instance data");
      if (opt_map.has_key("password"))
        password = opt_map.string_at("password");
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

REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS1,
              "@li If the options contain an invalid attribute.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS2,
              "@li If the root password is missing on the options.");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS3,
              "@li AIf the port value is < 1024 or > 65535.");

REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS4,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(DBA_DEPLOYSANDBOXINSTANCE_THROWS5,
              "@li If SSL "
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
              "The password option specifies the MySQL root "
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
 * $(DBA_DEPLOYSANDBOXINSTANCE_THROWS4)
 * $(DBA_DEPLOYSANDBOXINSTANCE_THROWS5)
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
            if (instance_def.has_password()) pwd = instance_def.get_password();

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

REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_THROWS1,
              "@li If the options contain an invalid attribute.");
REGISTER_HELP(DBA_DELETESANDBOXINSTANCE_THROWS2,
              "@li If the port value is < 1024 or > 65535.");

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
 * $(DBA_DELETESANDBOXINSTANCE_THROWS2)
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

REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_THROWS1,
              "@li If the options contain an invalid attribute.");
REGISTER_HELP(DBA_KILLSANDBOXINSTANCE_THROWS2,
              "@li If the port value is < 1024 or > 65535.");

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
 * $(DBA_KILLSANDBOXINSTANCE_THROWS2)
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

REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_THROWS1,
              "@li If the options contain an invalid attribute.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_THROWS2,
              "@li If the root password is missing on the options.");
REGISTER_HELP(DBA_STOPSANDBOXINSTANCE_THROWS3,
              "@li If the port value is < 1024 or > 65535.");

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
 * $(DBA_STOPSANDBOXINSTANCE_THROWS3)
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

REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_THROWS1,
              "@li If the options contain an invalid attribute.");
REGISTER_HELP(DBA_STARTSANDBOXINSTANCE_THROWS2,
              "@li If the port value is < 1024 or > 65535.");

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
 * $(DBA_STARTSANDBOXINSTANCE_THROWS2)
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

shcore::Value Dba::do_configure_instance(const shcore::Argument_list &args,
                                         bool local) {
  shcore::Value ret_val;
  mysqlshdk::db::Connection_options instance_def;
  std::shared_ptr<mysqlshdk::db::ISession> instance_session;

  std::string mycnf_path, output_mycnf_path, cluster_admin, password;
  mysqlshdk::utils::nullable<std::string> cluster_admin_password;
  mysqlshdk::utils::nullable<bool> clear_read_only;
  bool interactive = m_wizards_mode;
  mysqlshdk::utils::nullable<bool> restart;

  {
    if (args.size() == 2) {
      // Retrieves optional options if exists or leaves empty so the default is
      // set afterwards
      Unpack_options(args.map_at(1))
          .optional("mycnfPath", &mycnf_path)
          .optional("outputMycnfPath", &output_mycnf_path)
          .optional("clusterAdmin", &cluster_admin)
          .optional("clusterAdminPassword", &cluster_admin_password)
          .optional("clearReadOnly", &clear_read_only)
          .optional("interactive", &interactive)
          .optional("restart", &restart)
          .optional_ci("password", &password)
          .end();
    }

    // If there are no args, the instanceDef is empty (no data)
    if (args.size() != 0 && args[0].type != shcore::Null) {
      instance_def = mysqlsh::get_connection_options(
          args, mysqlsh::PasswordFormat::OPTIONS);
    }

    // If interactive is enabled, resolves user and validates password
    if (interactive) {
      if (instance_def.has_data())
        mysqlsh::resolve_connection_credentials(&instance_def, m_console);
    }

    // Establish the session to the target instance
    if (instance_def.has_data()) {
      validate_connection_options(instance_def);

      instance_session = mysqlshdk::db::mysql::Session::create();
      instance_session->connect(instance_def);
    } else {
      instance_session = connect_to_target_member();
    }
  }

  // Check the function preconditions
  // (FR7) Validate if the instance is already part of a GR group
  // or InnoDB cluster
  check_preconditions(instance_session,
                      local ? "configureLocalInstance" : "configureInstance");

  {
    // Create the Instance object with the established session
    mysqlshdk::mysql::Instance target_instance(instance_session);

    target_instance.cache_global_sysvars();

    // Call the API
    std::unique_ptr<Configure_instance> op_configure_instance;
    if (local)
      op_configure_instance.reset(new Configure_local_instance(
          &target_instance, mycnf_path, output_mycnf_path, cluster_admin,
          cluster_admin_password, clear_read_only, interactive, restart,
          _provisioning_interface, m_console));
    else
      op_configure_instance.reset(new Configure_instance(
          &target_instance, mycnf_path, output_mycnf_path, cluster_admin,
          cluster_admin_password, clear_read_only, interactive, restart,
          _provisioning_interface, m_console));

    op_configure_instance->prepare();
    ret_val = shcore::Value(op_configure_instance->execute());
    op_configure_instance->finish();

    // Close the session
    target_instance.close_session();
  }

  return ret_val;
}

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_BRIEF,
              "Validates and configures a local instance for MySQL InnoDB "
              "Cluster usage.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_PARAM,
              "@param instance An instance definition.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_PARAM1,
              "@param options Optional Additional options for the operation.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS1,
              "@li If the instance parameter is empty.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS2,
              "@li If the instance definition is invalid.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS3,
              "@li If the instance definition is a "
              "connection dictionary but empty.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS4,
              "@li If the instance definition is a "
              "connection dictionary but any option is invalid.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS5,
              "@li If the instance definition is missing "
              "the password.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS6,
              "@li If the provided password is empty.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS7,
              "@li If the configuration file path is "
              "required but not provided or wrong.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS8,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS9,
              "@li If the instance accounts are invalid.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS10,
              "@li If the instance is offline.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS11,
              "@li If the instance is already part of a "
              "Replication Group.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_THROWS12,
              "@li If the given instance cannot be used "
              "for Group Replication.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL,
              "This function reviews the instance configuration to identify if "
              "it is valid for usage in group replication and cluster. "
              "An exception is thrown if not.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL1,
              "The instance definition is the connection data for the "
              "instance.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL2,
              "TOPIC_CONNECTION_MORE_INFO_TCP_ONLY");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL3,
              "The options dictionary may contain the following options:");
REGISTER_HELP(
    DBA_CONFIGURELOCALINSTANCE_DETAIL4,
    "@li mycnfPath: The path to the MySQL configuration file of the instance.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL5,
              "@li outputMycnfPath: Alternative output path to write the MySQL "
              "configuration file of the instance.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL6,
              "@li password: The password to be used on the connection.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL7,
              "@li clusterAdmin: The name of the InnoDB cluster administrator "
              "user to be created. The supported format is the standard MySQL "
              "account name format.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL8,
              "@li clusterAdminPassword: The password for the InnoDB cluster "
              "administrator account.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL9,
              "@li clearReadOnly: boolean value used to confirm that "
              "super_read_only must be disabled.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL10,
              "@li interactive: boolean value used to disable the wizards in "
              "the command execution, i.e. prompts are not provided to the "
              "user and confirmation prompts are not shown.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL11,
              "The connection password may be contained on the instance "
              "definition, however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL12,
              "The returned descriptive text of the operation result indicates "
              "whether the instance was successfully configured for InnoDB "
              "Cluster usage or if it was already valid for InnoDB Cluster "
              "usage.");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL13,
              "If the instance was not valid for InnoDB Cluster and "
              "interaction is enabled, before configuring the instance a "
              "prompt to confirm the changes is presented and a table with the "
              "following information:");

REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL14,
              "@li Variable: the invalid configuration variable.");
REGISTER_HELP(DBA_CONFIGURELOCALINSTANCE_DETAIL15,
              "@li Current Value: the current value for the invalid "
              "configuration variable.");
REGISTER_HELP(
    DBA_CONFIGURELOCALINSTANCE_DETAIL16,
    "@li Required Value: the required value for the configuration variable.");
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
 * $(DBA_CONFIGURELOCALINSTANCE_THROWS7)
 * $(DBA_CONFIGURELOCALINSTANCE_THROWS8)
 * $(DBA_CONFIGURELOCALINSTANCE_THROWS9)
 * $(DBA_CONFIGURELOCALINSTANCE_THROWS10)
 * $(DBA_CONFIGURELOCALINSTANCE_THROWS11)
 * $(DBA_CONFIGURELOCALINSTANCE_THROWS12)
 *
 * $(DBA_CONFIGURELOCALINSTANCE_RETURNS)
 *
 * $(DBA_CONFIGURELOCALINSTANCE_DETAIL)
 *
 * $(DBA_CONFIGURELOCALINSTANCE_DETAIL1)
 *
 * Detailed description of the connection data format is available at \ref
 * connection_data.
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
 * $(DBA_CONFIGURELOCALINSTANCE_DETAIL15)
 * $(DBA_CONFIGURELOCALINSTANCE_DETAIL16)
 */
#if DOXYGEN_JS
Undefined Dba::configureLocalInstance(InstanceDef instance,
                                      Dictionary options) {}
#elif DOXYGEN_PY
None Dba::configure_local_instance(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::configure_local_instance(const shcore::Argument_list &args) {
  args.ensure_count(0, 2, get_function_name("configureLocalInstance").c_str());

  try {
    return do_configure_instance(args, true);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("configureLocalInstance"));
}

REGISTER_HELP(DBA_CONFIGUREINSTANCE_BRIEF,
              "Validates and configures an instance for MySQL InnoDB Cluster "
              "usage.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_PARAM,
              "@param instance Optional An instance definition.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_PARAM1,
              "@param options Optional Additional options for the operation.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_RETURNS,
              "@returns A descriptive text of the operation result.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL,
              "This function auto-configures the instance for InnoDB Cluster "
              "usage."
              "If the target instance already belongs to an InnoDB Cluster it "
              "errors out.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL1,
              "The instance definition is the connection data for the "
              "instance.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL2,
              "TOPIC_CONNECTION_MORE_INFO_TCP_ONLY");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL3,
              "The options dictionary may contain the following options:");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL4,
              "@li mycnfPath: The path to the MySQL configuration file of "
              "the instance.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL5,
              "@li outputMycnfPath: Alternative output path to write the MySQL "
              "configuration file of the instance.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL6,
              "@li password: The password to be used on the connection.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL7,
              "@li clusterAdmin: The name of the InnoDB cluster administrator "
              "user to be created. The supported format is the standard MySQL "
              "account name format.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL8,
              "@li clusterAdminPassword: The password for the InnoDB cluster "
              "administrator account.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL9,
              "@li clearReadOnly: boolean value used to confirm that "
              "super_read_only must be disabled.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL10,
              "@li interactive: boolean value used to disable the wizards in "
              "the command execution, i.e. prompts are not provided to the "
              "user and confirmation prompts are not shown.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL11,
              "@li restart: boolean value used to indicate that a remote "
              "restart of the target instance should be performed to finalize "
              "the operation.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL12,
              "The connection password may be contained on the instance "
              "definition, however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL13,
              "This function reviews the instance configuration to identify if "
              "it is valid for usage in group replication and cluster. "
              "An exception is thrown if not.");

REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL14,
              "If the instance was not valid for InnoDB Cluster and "
              "interaction is enabled, before configuring the instance a "
              "prompt to confirm the changes is presented and a table with the "
              "following information:");

REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL15,
              "@li Variable: the invalid configuration variable.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_DETAIL16,
              "@li Current Value: the current value for the invalid "
              "configuration variable.");
REGISTER_HELP(
    DBA_CONFIGUREINSTANCE_DETAIL17,
    "@li Required Value: the required value for the configuration variable.");

REGISTER_HELP(
    DBA_CONFIGUREINSTANCE_DETAIL18,
    "@li Required Value: the required value for the configuration variable.");

REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS1,
              "@li If 'interactive' is disabled and the "
              "instance parameter is empty.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS2,
              "@li If the instance definition is invalid.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS3,
              "@li If the instance definition is a "
              "connection dictionary but empty.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS4,
              "@li If the instance definition is a "
              "connection dictionary but any option is invalid.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS5,
              "@li If 'interactive' mode is disabled and "
              "the instance definition is missing the password.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS6,
              "@li If 'interactive' mode is enabled and the "
              "provided password is empty.");

REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS7,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS8,
              "@li If the configuration file path is "
              "required but not provided or wrong.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS9,
              "@li If the instance accounts are invalid.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS10,
              "@li If the instance is offline.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS11,
              "@li If the instance is already part of a "
              "Replication Group.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS12,
              "@li If the instance is already part of an "
              "InnoDB Cluster.");
REGISTER_HELP(DBA_CONFIGUREINSTANCE_THROWS13,
              "@li If the given instance cannot be used "
              "for Group Replication.");

/**
 * $(DBA_CONFIGUREINSTANCE_BRIEF)
 *
 * $(DBA_CONFIGUREINSTANCE_PARAM)
 * $(DBA_CONFIGUREINSTANCE_PARAM1)
 *
 * $(DBA_CONFIGUREINSTANCE_RETURNS)
 *
 * $(DBA_CONFIGUREINSTANCE_DETAIL)
 *
 * $(DBA_CONFIGUREINSTANCE_DETAIL1)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref
 * connection_data
 *
 * $(DBA_CONFIGUREINSTANCE_DETAIL3)
 * $(DBA_CONFIGUREINSTANCE_DETAIL4)
 * $(DBA_CONFIGUREINSTANCE_DETAIL5)
 * $(DBA_CONFIGUREINSTANCE_DETAIL6)
 * $(DBA_CONFIGUREINSTANCE_DETAIL7)
 * $(DBA_CONFIGUREINSTANCE_DETAIL8)
 * $(DBA_CONFIGUREINSTANCE_DETAIL9)
 *
 * $(DBA_CONFIGUREINSTANCE_DETAIL10)
 *
 * $(DBA_CONFIGUREINSTANCE_DETAIL11)
 * $(DBA_CONFIGUREINSTANCE_DETAIL12)
 * $(DBA_CONFIGUREINSTANCE_DETAIL13)
 * $(DBA_CONFIGUREINSTANCE_DETAIL14)
 * $(DBA_CONFIGUREINSTANCE_DETAIL15)
 * $(DBA_CONFIGUREINSTANCE_DETAIL16)
 * $(DBA_CONFIGUREINSTANCE_DETAIL17)
 * $(DBA_CONFIGUREINSTANCE_DETAIL18)
 *
 * $(DBA_CONFIGUREINSTANCE_THROWS)
 * $(DBA_CONFIGUREINSTANCE_THROWS1)
 * $(DBA_CONFIGUREINSTANCE_THROWS2)
 * $(DBA_CONFIGUREINSTANCE_THROWS3)
 * $(DBA_CONFIGUREINSTANCE_THROWS4)
 * $(DBA_CONFIGUREINSTANCE_THROWS5)
 * $(DBA_CONFIGUREINSTANCE_THROWS6)
 * $(DBA_CONFIGUREINSTANCE_THROWS7)
 * $(DBA_CONFIGUREINSTANCE_THROWS8)
 * $(DBA_CONFIGUREINSTANCE_THROWS9)
 * $(DBA_CONFIGUREINSTANCE_THROWS10)
 * $(DBA_CONFIGUREINSTANCE_THROWS11)
 * $(DBA_CONFIGUREINSTANCE_THROWS12)
 * $(DBA_CONFIGUREINSTANCE_THROWS13)
 */
#if DOXYGEN_JS
Undefined Dba::configureInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Dba::configure_instance(InstanceDef instance, dict options) {}
#endif
shcore::Value Dba::configure_instance(const shcore::Argument_list &args) {
  args.ensure_count(0, 2, get_function_name("configureLocalInstance").c_str());

  try {
    return do_configure_instance(args, false);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("configureInstance"));
}

std::shared_ptr<mysqlshdk::db::ISession> Dba::get_session(
    const mysqlshdk::db::Connection_options &args) {
  std::shared_ptr<mysqlshdk::db::ISession> ret_val;

  ret_val = mysqlshdk::db::mysql::Session::create();

  ret_val->connect(args);

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
              "MetadataError in the following scenarios:");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS1,
              "@li If the Metadata is inaccessible.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS2,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS3,
              "@li If the Cluster name is empty.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS4,
              "@li If the Cluster name is not valid.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS5,
              "@li If the options contain an invalid attribute.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS6,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS7,
              "@li If the Cluster does not exist on the Metadata.");
REGISTER_HELP(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS8,
              "@li If some instance of the Cluster belongs to "
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
              "@li clearReadOnly: boolean value used to confirm that "
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
 * $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS6)
 * $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS7)
 * $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_THROWS8)
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

  check_preconditions(get_active_shell_session(),
                      "rebootClusterFromCompleteOutage");

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
    instance_session_address = current_session_options.as_uri(only_transport());

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

    cluster.reset(
        new Cluster(cluster_name, group_session, metadata, m_console));

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
      if (!metadata->is_instance_on_replicaset(default_replicaset->get_id(),
                                               instance_session_address))
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
      validate_super_read_only(group_session, clear_read_only, m_console);

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
                                       current_group_replication_group_name,
                                       true);
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

Cluster_check_info Dba::check_preconditions(
    std::shared_ptr<mysqlshdk::db::ISession> group_session,
    const std::string &function_name) const {
  try {
    return check_function_preconditions("Dba." + function_name, group_session);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(function_name));
  return Cluster_check_info{};
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
Dba::get_replicaset_instances_status(
    std::shared_ptr<Cluster> cluster,
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
  active_session_address = current_session_options.as_uri(only_transport());

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
    const std::string &gr_group_name, const std::string &restore_function) {
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

    case GRInstanceType::Standalone:
    case GRInstanceType::StandaloneWithMetadata:
    case GRInstanceType::Unknown:
      // We only want to check whether the status if InnoDBCluster or
      // GroupReplication to stop and thrown an exception
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
    if (!instance_status.empty()) continue;

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
    get_server_variable(session, "GLOBAL.GTID_EXECUTED", gtid_executed);

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

}  // namespace dba
}  // namespace mysqlsh
