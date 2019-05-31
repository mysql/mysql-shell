/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/mod_dba.h"

#include <mysqld_error.h>
#include <algorithm>
#include <memory>
#include <random>
#include <string>

#ifndef WIN32
#include <sys/un.h>
#endif

#include <iterator>
#include <utility>
#include <vector>

#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/dba/check_instance.h"
#include "modules/adminapi/dba/configure_instance.h"
#include "modules/adminapi/dba/configure_local_instance.h"
#include "modules/adminapi/dba/create_cluster.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/replicaset/add_instance.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_shell.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/innodbcluster/cluster.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "scripting/object_factory.h"
#include "shellcore/utils_help.h"

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

#define PASSWORD_LENGHT 16
using mysqlshdk::db::uri::formats::only_transport;
std::set<std::string> Dba::_deploy_instance_opts = {
    "portx",         "sandboxDir",     "password",
    "allowRootFrom", "ignoreSslError", "mysqldOptions"};
std::set<std::string> Dba::_stop_instance_opts = {"sandboxDir", "password"};
std::set<std::string> Dba::_default_local_instance_opts = {"sandboxDir"};

std::set<std::string> Dba::_reboot_cluster_opts = {
    "user",         "dbUser", "password", "removeInstances", "rejoinInstances",
    "clearReadOnly"};

/*
 * Global helper text for InnoDB Cluster configuration options used in:
 *  - dba.createCluster()
 *  - Cluster.addInstance()
 *  - Cluster.setOption()
 *  - Cluster.setInstanceOption()
 */
REGISTER_HELP(CLUSTER_OPT_EXIT_STATE_ACTION,
              "@li exitStateAction: string value indicating the group "
              "replication exit state action.");
REGISTER_HELP(CLUSTER_OPT_MEMBER_WEIGHT,
              "@li memberWeight: integer value with a percentage weight for "
              "automatic primary election on failover.");
REGISTER_HELP(CLUSTER_OPT_FAILOVER_CONSISTENCY,
              "@li failoverConsistency: string value indicating the "
              "consistency guarantees that the cluster provides.");
REGISTER_HELP(CLUSTER_OPT_CONSISTENCY,
              "@li consistency: string value indicating the "
              "consistency guarantees that the cluster provides.");
REGISTER_HELP(CLUSTER_OPT_EXPEL_TIMEOUT,
              "@li expelTimeout: integer value to define the time period in "
              "seconds that cluster members should wait for a non-responding "
              "member before evicting it from the cluster.");
REGISTER_HELP(
    CLUSTER_OPT_AUTO_REJOIN_TRIES,
    "@li autoRejoinTries: integer value to define the number of times an "
    "instance will attempt to rejoin the cluster after being expelled.");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL, R"*(
The exitStateAction option supports the following values:
@li ABORT_SERVER: if used, the instance shuts itself down if
it leaves the cluster unintentionally.
@li READ_ONLY: if used, the instance switches itself to
super-read-only mode if it leaves the cluster unintentionally.

If exitStateAction is not specified READ_ONLY will be used by default.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA, R"*(
The value for exitStateAction is used to configure how Group
Replication behaves when a server instance leaves the group
unintentionally, for example after encountering an applier
error. When set to ABORT_SERVER, the instance shuts itself
down, and when set to READ_ONLY the server switches itself to
super-read-only mode. The exitStateAction option accepts
case-insensitive string values, being the accepted values:
ABORT_SERVER (or 1) and READ_ONLY (or 0).

The default value is READ_ONLY.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA, R"*(
The value for memberWeight is used to set the Group Replication
system variable 'group_replication_member_weight'. The
memberWeight option accepts integer values. Group Replication
limits the value range from 0 to 100, automatically adjusting
it if a lower/bigger value is provided.

Group Replication uses a default value of 50 if no value is provided.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_CONSISTENCY_DETAIL, R"*(
The consistency option supports the following values:
@li BEFORE_ON_PRIMARY_FAILOVER: if used, new queries (read or
write) to the new primary will be put on hold until
after the backlog from the old primary is applied.
@li EVENTUAL: if used, read queries to the new primary are
allowed even if the backlog isn't applied.

If consistency is not specified, EVENTUAL will be used
by default.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_CONSISTENCY_EXTRA, R"*(
The value for consistency is used to set the Group
Replication system variable 'group_replication_consistency' and
configure the transaction consistency guarantee which a cluster provides.

When set to to BEFORE_ON_PRIMARY_FAILOVER, whenever a primary failover happens
in single-primary mode (default), new queries (read or write) to the newly
elected primary that is applying backlog from the old primary, will be hold
before execution until the backlog is applied. When set to EVENTUAL, read
queries to the new primary are allowed even if the backlog isn't applied but
writes will fail (if the backlog isn't applied) due to super-read-only mode
being enabled. The client may return old values.

When set to BEFORE, each transaction (RW or RO) waits until all preceding
transactions are complete before starting its execution. This ensures that each
transaction is executed on the most up-to-date snapshot of the data, regardless
of which member it is executed on. The latency of the transaction is affected
but the overhead of synchronization on RW transactions is reduced since
synchronization is used only on RO transactions.

When set to AFTER, each RW transaction waits until its changes have been
applied on all of the other members. This ensures that once this transaction
completes, all following transactions read a database state that includes its
changes, regardless of which member they are executed on. This mode shall only
be used on a group that is used for predominantly RO operations to  to ensure
that subsequent reads fetch the latest data which includes the latest writes.
The overhead of synchronization on every RO transaction is reduced since
synchronization is used only on RW transactions.

When set to BEFORE_AND_AFTER, each RW transaction waits for all preceding
transactions to complete before being applied and until its changes have been
applied on other members. A RO transaction waits for all preceding transactions
to complete before execution takes place. This ensures the guarantees given by
BEFORE and by AFTER. The overhead of synchronization is higher.

The consistency option accepts case-insensitive string values, being the
accepted values: EVENTUAL (or 0), BEFORE_ON_PRIMARY_FAILOVER (or 1), BEFORE
(or 2), AFTER (or 3), and BEFORE_AND_AFTER (or 4).

The default value is EVENTUAL.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_EXPEL_TIMEOUT_EXTRA, R"*(
The value for expelTimeout is used to set the Group Replication
system variable 'group_replication_member_expel_timeout' and
configure how long Group Replication will wait before expelling
from the group any members suspected of having failed. On slow
networks, or when there are expected machine slowdowns,
increase the value of this option. The expelTimeout option
accepts positive integer values in the range [0, 3600].

The default value is 0.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA, R"*(
The value for autoRejoinTries is used to set the Group Replication system
variable 'group_replication_autorejoin_tries' and configure how many
times an instance will try to rejoin a Group Replication group after
being expelled. In scenarios where network glitches happen but recover
quickly, setting this option prevents users from having to manually add
the expelled node back to the group. The autoRejoinTries option accepts
positive integer values in the range [0, 2016].

The default value is 0.
)*");

// Documentation of the DBA Class
REGISTER_HELP_TOPIC(AdminAPI, CATEGORY, adminapi, Contents, SCRIPTING);
REGISTER_HELP(ADMINAPI_BRIEF,
              "Introduces to the <b>dba</b> global object and the InnoDB "
              "cluster administration API.");
REGISTER_HELP(ADMINAPI_DETAIL,
              "MySQL InnoDB cluster provides a complete high "
              "availability solution for MySQL.");
REGISTER_HELP(ADMINAPI_OBJECTS_DESC, "IGNORE");
REGISTER_HELP(ADMINAPI_CLASSES_DESC, "IGNORE");
REGISTER_HELP(ADMINAPI_DETAIL1,
              "The <b>AdminAPI</b> is an interactive API that "
              "enables configuring and administering InnoDB clusters.");
REGISTER_HELP(ADMINAPI_DETAIL2, "Use the <b>dba</b> global object to:");
REGISTER_HELP(ADMINAPI_DETAIL3,
              "@li Verify if a MySQL server is suitable for InnoDB cluster.");
REGISTER_HELP(
    ADMINAPI_DETAIL4,
    "@li Configure a MySQL server to be used as an InnoDB cluster instance.");
REGISTER_HELP(ADMINAPI_DETAIL5, "@li Create an InnoDB cluster.");
REGISTER_HELP(
    ADMINAPI_DETAIL6,
    "@li Get a handle for performing operations on an InnoDB cluster.");
REGISTER_HELP(ADMINAPI_DETAIL7, "@li Other InnoDB cluster maintenance tasks.");
REGISTER_HELP(ADMINAPI_DETAIL8,
              "In the AdminAPI, an InnoDB cluster is represented as an "
              "instance of the <b>Cluster</b> class.");
REGISTER_HELP(ADMINAPI_DETAIL9,
              "For more information about the <b>dba</b> object use: \\? dba");
REGISTER_HELP(
    ADMINAPI_DETAIL10,
    "For more information about the <b>Cluster</b> class use: \\? Cluster");

REGISTER_HELP_GLOBAL_OBJECT(dba, adminapi);
REGISTER_HELP(DBA_BRIEF, "Global variable for InnoDB cluster management.");
REGISTER_HELP(DBA_GLOBAL_BRIEF, "Used for InnoDB cluster administration.");
REGISTER_HELP(
    DBA_DETAIL,
    "The global variable <b>dba</b> is used to access the AdminAPI "
    "functionality and perform DBA operations. It is used for managing MySQL "
    "InnoDB clusters.");
REGISTER_HELP(
    DBA_CLOSING,
    "For more help on a specific function use: dba.help('<functionName>')");
REGISTER_HELP(DBA_CLOSING1, "e.g. dba.help('<<<deploySandboxInstance>>>')");

REGISTER_HELP(DBA_VERBOSE_BRIEF,
              "Enables verbose mode on the <b>dba</b> operations.");
REGISTER_HELP(DBA_VERBOSE_DETAIL,
              "The assigned value can be either boolean or integer, the result "
              "depends on the assigned value:");
REGISTER_HELP(DBA_VERBOSE_DETAIL1, "@li 0: disables mysqlprovision verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL2, "@li 1: enables mysqlprovision verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL3,
              "@li >1: enables mysqlprovision debug verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL4,
              "@li Boolean: equivalent to assign either 0 or 1");

Dba::Dba(shcore::IShell_core *owner) : _shell_core(owner) { init(); }

Dba::~Dba() {}

bool Dba::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Dba::init() {
  add_property("verbose");

  // Pure functions
  expose("createCluster", &Dba::create_cluster, "clusterName", "?options");

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

  std::string local_mp_path =
      mysqlsh::current_shell_options()->get().gadgets_path;

  if (local_mp_path.empty()) local_mp_path = shcore::get_mp_path();

  _provisioning_interface.reset(new ProvisioningInterface(local_mp_path));
}

void Dba::set_member(const std::string &prop, shcore::Value value) {
  if (prop == "verbose") {
    try {
      int verbosity = value.as_int();
      _provisioning_interface->set_verbose(verbosity);
    } catch (const shcore::Exception &e) {
      throw shcore::Exception::value_error(
          "Invalid value for property 'verbose', use either boolean or integer "
          "value.");
    }
  } else {
    Cpp_object_bridge::set_member(prop, value);
  }
}

REGISTER_HELP_PROPERTY(verbose, dba);
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
shcore::Value Dba::get_member(const std::string &prop) const {
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
      } catch (const std::exception &e) {
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
REGISTER_HELP_FUNCTION(getCluster, dba);

REGISTER_HELP_FUNCTION_TEXT(DBA_GETCLUSTER, R"*(
Retrieves a cluster from the Metadata Store.

@param name Optional parameter to specify the name of the cluster to be returned.
@param options Optional dictionary with additional options.

@returns The cluster object identified by the given name or the default cluster.

If name is not specified or is null, the default cluster will be returned.

If name is specified, and no cluster with the indicated name is found, an
error will be raised.

The options dictionary accepts the connectToPrimary option,
which defaults to true and indicates the shell to automatically
connect to the primary member of the cluster.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@li If the Metadata update operation failed.

@throw ArgumentError in the following scenarios:
@li If the Cluster name is empty.
@li If the Cluster name is invalid.
@li If the Cluster does not exist.

@throw RuntimeError in the following scenarios:
@li If the current connection cannot be used for Group Replication.
)*");
/**
 * $(DBA_GETCLUSTER_BRIEF)
 *
 * $(DBA_GETCLUSTER)
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
  std::string cluster_name;

  try {
    bool connect_to_primary = true;
    bool fallback_to_anything = true;
    bool default_cluster = true;

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
      } catch (const std::exception &e) {
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
    } catch (const mysqlshdk::innodbcluster::cluster_error &e) {
      auto console = mysqlsh::current_console();

      // Print warning in case a cluster error is found (e.g., no quorum).
      if (e.code() == mysqlshdk::innodbcluster::Error::Group_has_no_quorum) {
        console->print_warning(
            "Cluster has no quorum and cannot process write transactions: " +
            std::string(e.what()));
      } else {
        console->print_warning("Cluster error connecting to target: " +
                               e.format());
      }

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
  } catch (const mysqlshdk::innodbcluster::cluster_error &e) {
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
    metadata->load_default_cluster(cluster->impl());
  } else {
    metadata->load_cluster(name, cluster->impl());
  }
  // Verify if the current session instance group_replication_group_name
  // value differs from the one registered in the Metadata
  if (!validate_replicaset_group_name(
          group_session,
          cluster->impl()->get_default_replicaset()->get_group_name())) {
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

  return cluster;
}

REGISTER_HELP_FUNCTION(createCluster, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CREATECLUSTER, R"*(
Creates a MySQL InnoDB cluster.

@param name The name of the cluster object to be created.
@param options Optional dictionary with options that modify the behavior of
this function.

@returns The created cluster object.

Creates a MySQL InnoDB cluster taking as seed instance the active global
session.

The options dictionary can contain the following values:

@li interactive: boolean value used to disable the wizards in the command
execution, i.e. prompts are not provided to the user and confirmation prompts
are not shown.
@li disableClone: boolean value used to disable the clone usage on the cluster.
@li gtidSetIsComplete: boolean value which indicates whether the GTID set
of the seed instance corresponds to all transactions executed. Default is false.
@li multiPrimary: boolean value used to define an InnoDB cluster with multiple
writable instances.
@li force: boolean, confirms that the multiPrimary option must be applied.
@li adoptFromGR: boolean value used to create the InnoDB cluster based on
existing replication group.
@li memberSslMode: SSL mode used to configure the members of the cluster.
@li ipWhitelist: The list of hosts allowed to connect to the instance for group
replication.
@li groupName: string value with the Group Replication group name UUID to be
used instead of the automatically generated one.
@li localAddress: string value with the Group Replication local address to be
used instead of the automatically generated one.
@li groupSeeds: string value with a comma-separated list of the Group
Replication peer addresses to be used instead of the automatically generated
one.
${CLUSTER_OPT_EXIT_STATE_ACTION}
${CLUSTER_OPT_MEMBER_WEIGHT}
${CLUSTER_OPT_FAILOVER_CONSISTENCY}
${CLUSTER_OPT_EXPEL_TIMEOUT}
${CLUSTER_OPT_AUTO_REJOIN_TRIES}
@li clearReadOnly: boolean value used to confirm that super_read_only must be
disabled. Deprecated.
@li multiMaster: boolean value used to define an InnoDB cluster with multiple
writable instances. Deprecated.

An InnoDB cluster may be setup in two ways:

@li Single Primary: One member of the cluster allows write operations while the
rest are read-only secondaries.
@li Multi Primary: All the members in the cluster allow both read and write
operations.

Note that Multi-Primary mode has limitations about what can be safely executed.
Make sure to read the MySQL documentation for Group Replication and be aware of
what is and is not safely executable in such setups.

By default this function creates a Single Primary cluster. Use the multiPrimary
option set to true if a Multi Primary cluster is required.

<b>Options</b>

interactive controls whether prompts are shown for MySQL passwords,
confirmations and handling of cases where user feedback may be required.
Defaults to true, unless the Shell is started with the --no-wizards option.

disableClone should be set to true if built-in clone support should be
completely disabled, even in instances where that is supported. Built-in clone
support is available starting with MySQL 8.0.17 and allows automatically
provisioning new cluster members by copying state from an existing cluster
member. Note that clone will completely delete all data in the instance being
added to the cluster.

gtidSetIsComplete is used to indicate that GTIDs have been always enabled
at the cluster seed instance and that GTID_EXECUTED contains all transactions
ever executed. It must be left as false if data was inserted or modified while
GTIDs were disabled or if RESET MASTER was executed. This flag affects how
cluster.<<<addInstance>>>() decides which recovery methods are safe to use.
Distributed recovery based on replaying the transaction history is only assumed
to be safe if the transaction history is known to be complete, otherwise
cluster members could end up with incomplete data sets.

adoptFromGR allows creating an InnoDB cluster from an existing unmanaged
Group Replication setup, enabling use of MySQL Router and the shell AdminAPI
for managing it.

The memberSslMode option supports the following values:

@li REQUIRED: if used, SSL (encryption) will be enabled for the instances to
communicate with other members of the cluster
@li DISABLED: if used, SSL (encryption) will be disabled
@li AUTO: if used, SSL (encryption) will be enabled if supported by the
instance, otherwise disabled

If memberSslMode is not specified AUTO will be used by default.

The ipWhitelist format is a comma separated list of IP addresses or subnet CIDR
notation, for example: 192.168.1.0/24,10.0.0.1. By default the value is set to
AUTOMATIC, allowing addresses from the instance private network to be
automatically set for the whitelist.

The groupName, localAddress, and groupSeeds are advanced options and their
usage is discouraged since incorrect values can lead to Group Replication
errors.

The value for groupName is used to set the Group Replication system variable
'group_replication_group_name'.

The value for localAddress is used to set the Group Replication system variable
'group_replication_local_address'. The localAddress option accepts values in
the format: 'host:port' or 'host:' or ':port'. If the specified value does not
include a colon (:) and it is numeric, then it is assumed to be the port,
otherwise it is considered to be the host. When the host is not specified, the
default value is the value of the system variable 'report_host' if defined
(i.e., not 'NULL'), otherwise it is the hostname value. When the port is not
specified, the default value is the port of the current active connection
(session) * 10 + 1. In case the automatically determined default port value is
invalid (> 65535) then an error is thrown.

The value for groupSeeds is used to set the Group Replication system variable
'group_replication_group_seeds'. The groupSeeds option accepts a
comma-separated list of addresses in the format: 'host1:port1,...,hostN:portN'.

${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}

${CLUSTER_OPT_CONSISTENCY_DETAIL}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_CONSISTENCY_EXTRA}

${CLUSTER_OPT_EXPEL_TIMEOUT_EXTRA}

${CLUSTER_OPT_AUTO_REJOIN_TRIES_EXTRA}

@attention The clearReadOnly option will be removed in a future release.

@attention The multiMaster option will be removed in a future release. Please
use the multiPrimary option instead.

@attention The failoverConsistency option will be removed in a future release.
Please use the consistency option instead.


@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.
@li If the Metadata update operation failed.

@throw ArgumentError in the following scenarios:
@li If the Cluster name is empty.
@li If the Cluster name is not valid.
@li If the options contain an invalid attribute.
@li If adoptFromGR is true and the memberSslMode option is used.
@li If the value for the memberSslMode option is not one of the allowed.
@li If adoptFromGR is true and the multiPrimary option is used.
@li If the value for the ipWhitelist, groupName, localAddress, groupSeeds,
exitStateAction or consistency options is empty.
@li If the value for the expelTimeout is not in the range: [0, 3600]

@throw RuntimeError in the following scenarios:
@li If the value for the groupName, localAddress, groupSeeds, exitStateAction,
memberWeight, consistency, expelTimeout or autoRejoinTries options is
not valid for Group Replication.
@li If the current connection cannot be used for Group Replication.
@li If disableClone is not supported on the target instance.
)*");

/**
 * $(DBA_CREATECLUSTER_BRIEF)
 *
 * $(DBA_CREATECLUSTER)
 */

#if DOXYGEN_JS
Cluster Dba::createCluster(String name, Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::create_cluster(str name, dict options) {}
#endif
shcore::Value Dba::create_cluster(const std::string &cluster_name,
                                  const shcore::Dictionary_t &options) {
  Group_replication_options gr_options(Group_replication_options::CREATE);
  Clone_options clone_options(Clone_options::CREATE);
  bool adopt_from_gr = false;
  mysqlshdk::utils::nullable<bool> multi_primary;
  bool force = false;
  mysqlshdk::utils::nullable<bool> clear_read_only;
  bool interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (options) {
    // Retrieves optional options if exists
    Unpack_options(options)
        .unpack(&gr_options)
        .unpack(&clone_options)
        .optional("multiPrimary", &multi_primary)
        .optional("multiMaster", &multi_primary)
        .optional("force", &force)
        .optional("adoptFromGR", &adopt_from_gr)
        .optional("clearReadOnly", &clear_read_only)
        .optional("interactive", &interactive)
        .end();

    // Verify the deprecation of failoverConsistency
    if (options->has_key(kFailoverConsistency)) {
      if (options->has_key("consistency")) {
        throw shcore::Exception::argument_error(
            "Cannot use the failoverConsistency and consistency options "
            "simultaneously. The failoverConsistency option is deprecated, "
            "please use the consistency option instead.");
      } else {
        auto console = mysqlsh::current_console();
        console->print_warning(
            "The failoverConsistency option is deprecated. "
            "Please use the consistency option instead.");
        console->println();
      }
    }

    // Verify deprecation of multiMaster
    if (options->has_key("multiMaster")) {
      if (options->has_key("multiPrimary")) {
        throw shcore::Exception::argument_error(
            "Cannot use the multiMaster and multiPrimary options "
            "simultaneously. The multiMaster option is deprecated, please use "
            "the multiPrimary option instead.");
      } else {
        std::string warn_msg =
            "The multiMaster option is deprecated. "
            "Please use the multiPrimary option instead.";
        auto console = mysqlsh::current_console();
        console->print_warning(warn_msg);
        console->println();
      }
    }

    // Verify deprecation of clearReadOnly
    if (options->has_key("clearReadOnly")) {
      std::string warn_msg =
          "The clearReadOnly option is deprecated. The super_read_only mode is "
          "now automatically cleared.";
      auto console = mysqlsh::current_console();
      console->print_warning(warn_msg);
      console->println();
    }
  }

  std::shared_ptr<MetadataStorage> metadata;
  std::shared_ptr<mysqlshdk::db::ISession> group_session;

  // We're in createCluster(), so there's no metadata yet, but the metadata
  // object can already exist now to hold a session
  // connect_to_primary must be false, because we don't have a cluster yet
  // and this will be the seed instance anyway. If we start storing metadata
  // outside the managed cluster, then this will have to be updated.
  connect_to_target_group({}, &metadata, &group_session, false);

  mysqlshdk::mysql::Instance target_instance(group_session);

  // Check preconditions.
  Cluster_check_info state;
  try {
    state = check_preconditions(group_session, "createCluster");
  } catch (const shcore::Exception &e) {
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
    } else if (error.find("instance belongs to that metadata") !=
               std::string::npos) {
      // BUG#29271400:
      // createCluster() should not be allowed in instance with metadata
      std::string nice_error =
          "dba.<<<createCluster>>>: Unable to create cluster. The instance '" +
          group_session->uri(only_transport()) +
          "' has a populated Metadata schema and belongs to that Metadata. Use "
          "either dba.<<<dropMetadataSchema>>>() to drop the schema, or "
          "dba.<<<rebootClusterFromCompleteOutage>>>() to reboot the cluster "
          "from complete outage.";
      throw shcore::Exception::runtime_error(nice_error);
    } else {
      throw;
    }
  } catch (const mysqlshdk::db::Error &dberr) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        dberr.what(), dberr.code(), dberr.sqlstate());
  }

  // Create the cluster
  {
    // Create the add_instance command and execute it.
    Create_cluster op_create_cluster(&target_instance, cluster_name, gr_options,
                                     clone_options, multi_primary,
                                     adopt_from_gr, force, interactive);

    // Always execute finish when leaving "try catch".
    auto finally = shcore::on_leave_scope(
        [&op_create_cluster]() { op_create_cluster.finish(); });

    // Prepare the add_instance command execution (validations).
    op_create_cluster.prepare();

    // Execute add_instance operations.
    return op_create_cluster.execute();
  }
}

REGISTER_HELP_FUNCTION(dropMetadataSchema, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_DROPMETADATASCHEMA, R"*(
Drops the Metadata Schema.

@param options Dictionary containing an option to confirm the drop operation.

@returns Nothing.

The options dictionary may contain the following options:

@li force: boolean, confirms that the drop operation must be executed.
@li clearReadOnly: boolean value used to confirm that super_read_only must be
disabled

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.

@throw RuntimeError in the following scenarios:
@li If the current connection cannot be used for Group Replication.
)*");

/**
 * $(DBA_DROPMETADATASCHEMA_BRIEF)
 *
 * $(DBA_DROPMETADATASCHEMA)
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
    mysqlshdk::utils::nullable<bool> clear_read_only;
    bool interactive = current_shell_options()->get().wizards;

    // Map with the options
    Unpack_options(args.map_at(0))
        .optional("force", &force)
        .optional("clearReadOnly", &clear_read_only)
        .end();

    if (force) {
      mysqlshdk::mysql::Instance instance(group_session);

      // Check if super_read_only is turned off and disable it if required
      // NOTE: this is left for last to avoid setting super_read_only to true
      // and right before some execution failure of the command leaving the
      // instance in an incorrect state
      validate_super_read_only(instance, clear_read_only, interactive);

      metadata::uninstall(metadata->get_session());
    } else {
      throw shcore::Exception::runtime_error(
          "No operation executed, use the 'force' option");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("dropMetadataSchema"))

  return shcore::Value();
}

REGISTER_HELP_FUNCTION(checkInstanceConfiguration, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CHECKINSTANCECONFIGURATION, R"*(
Validates an instance for MySQL InnoDB Cluster usage.

@param instance An instance definition.
@param options Optional data for the operation.

@returns A descriptive text of the operation result.

This function reviews the instance configuration to identify if it is valid for
usage with group replication. Use this to check for possible configuration
issues on MySQL instances before creating a cluster with them or adding them to
an existing cluster.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

The options dictionary may contain the following options:

@li mycnfPath: Optional path to the MySQL configuration file for the instance.
Alias for verifyMyCnf
@li verifyMyCnf: Optional path to the MySQL configuration file for the
instance. If this option is given, the configuration file will be verified for
the expected option values, in addition to the global MySQL system variables.
@li password: The password to get connected to the instance.
@li interactive: boolean value used to disable the wizards in the command
execution, i.e. prompts are not provided to the user and confirmation prompts
are not shown.

The connection password may be contained on the instance definition, however,
it can be overwritten if it is specified on the options.

The returned descriptive text of the operation result indicates whether the
instance is valid for InnoDB Cluster usage or not. If not, a table containing
the following information is presented:

@li Variable: the invalid configuration variable.
@li Current Value: the current value for the invalid configuration variable.
@li Required Value: the required value for the configuration variable.
@li Note: the action to be taken.

The note can be one of the following:

@li Update the config file and update or restart the server variable.
@li Update the config file and restart the server.
@li Update the config file.
@li Update the server variable.
@li Restart the server.


@throw ArgumentError in the following scenarios:
@li If the instance parameter is empty.
@li If the instance definition is invalid.
@li If the instance definition is a connection dictionary but empty.

@throw RuntimeError in the following scenarios:
@li If the instance accounts are invalid.
@li If the instance is offline.
@li If the instance is already part of a Replication Group.
@li If the instance is already part of an InnoDB Cluster.
@li If the given the instance cannot be used for Group Replication.
)*");

/**
 * $(DBA_CHECKINSTANCECONFIGURATION_BRIEF)
 *
 * $(DBA_CHECKINSTANCECONFIGURATION)
 */
#if DOXYGEN_JS
JSON Dba::checkInstanceConfiguration(InstanceDef instance, Dictionary options) {
}
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
  bool interactive = current_shell_options()->get().wizards;
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

    // Establish the session to the target instance
    if (instance_def.has_data()) {
      validate_connection_options(instance_def);

      instance_session = establish_mysql_session(instance_def, interactive);
    } else {
      instance_session = connect_to_target_member();
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("checkInstanceConfiguration"));

  check_preconditions(instance_session, "checkInstanceConfiguration");

  try {
    // Get the Connection_options
    Connection_options coptions = instance_session->get_connection_options();

    // Close the session
    instance_session->close();

    // Call the API
    std::unique_ptr<Check_instance> op_check_instance(
        new Check_instance(coptions, mycnf_path));

    op_check_instance->prepare();
    ret_val = shcore::Value(op_check_instance->execute());
    op_check_instance->finish();
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
                                                 ignore_ssl_error, 0, &errors);
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

REGISTER_HELP_FUNCTION(deploySandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_DEPLOYSANDBOXINSTANCE, R"*(
Creates a new MySQL Server instance on localhost.

@param port The port where the new instance will listen for connections.
@param options Optional dictionary with options affecting the new deployed
instance.

@returns Nothing.

This function will deploy a new MySQL Server instance, the result may be
affected by the provided options:

@li portx: port where the new instance will listen for X Protocol connections.
@li sandboxDir: path where the new instance will be deployed.
@li password: password for the MySQL root user on the new instance.
@li allowRootFrom: create remote root account, restricted to the given address
pattern (eg %).
@li ignoreSslError: Ignore errors when adding SSL support for the new instance,
by default: true.

If the portx option is not specified, it will be automatically calculated as 10
times the value of the provided MySQL port.

The password option specifies the MySQL root password on the new instance.

The sandboxDir must be an existing folder where the new instance will be
deployed. If not specified the new instance will be deployed at:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

SSL support is added by default if not already available for the new instance,
but if it fails to be added then the error is ignored. Set the ignoreSslError
option to false to ensure the new instance is deployed with SSL support.

@throw ArgumentError in the following scenarios:
@li If the options contain an invalid attribute.
@li If the root password is missing on the options.
@li If the port value is < 1024 or > 65535.
@throw RuntimeError in the following scenarios:
@li If SSL support can be provided and ignoreSslError: false.
)*");

/**
 * $(DBA_DEPLOYSANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_DEPLOYSANDBOXINSTANCE)
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

            shcore::sqlstring create_user("CREATE USER root@? IDENTIFIED BY ?",
                                          0);
            create_user << remote_root << pwd;
            create_user.done();
            session->execute(create_user);
          }
          {
            shcore::sqlstring grant(
                "GRANT ALL ON *.* TO root@? WITH GRANT OPTION", 0);
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

REGISTER_HELP_FUNCTION(deleteSandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_DELETESANDBOXINSTANCE, R"*(
Deletes an existing MySQL Server instance on localhost.

@param port The port of the instance to be deleted.
@param options Optional dictionary with options that modify the way this
function is executed.

@returns Nothing.

This function will delete an existing MySQL Server instance on the local host.
The following options affect the result:

@li sandboxDir: path where the instance is located.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

If the instance is not located on the used path an error will occur.

@throw ArgumentError in the following scenarios:
@li If the options contain an invalid attribute.
@li If the port value is < 1024 or > 65535.
)*");

/**
 * $(DBA_DELETESANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_DELETESANDBOXINSTANCE)
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

REGISTER_HELP_FUNCTION(killSandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_KILLSANDBOXINSTANCE, R"*(
Kills a running MySQL Server instance on localhost.

@param port The port of the instance to be killed.
@param options Optional dictionary with options affecting the result.

@returns Nothing.

This function will kill the process of a running MySQL Server instance on the
local host. The following options affect the result:

@li sandboxDir: path where the instance is located.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

If the instance is not located on the used path an error will occur.

@throw ArgumentError in the following scenarios:
@li If the options contain an invalid attribute.
@li If the port value is < 1024 or > 65535.
)*");

/**
 * $(DBA_KILLSANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_KILLSANDBOXINSTANCE)
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

REGISTER_HELP_FUNCTION(stopSandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_STOPSANDBOXINSTANCE, R"*(
Stops a running MySQL Server instance on localhost.

@param port The port of the instance to be stopped.
@param options Optional dictionary with options affecting the result.

@returns Nothing.

This function will gracefully stop a running MySQL Server instance on the local
host. The following options affect the result:

@li sandboxDir: path where the instance is located.
@li password: password for the MySQL root user on the instance.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

If the instance is not located on the used path an error will occur.

@throw ArgumentError in the following scenarios:
@li If the options contain an invalid attribute.
@li If the root password is missing on the options.
@li If the port value is < 1024 or > 65535.
)*");

/**
 * $(DBA_STOPSANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_STOPSANDBOXINSTANCE)
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

REGISTER_HELP_FUNCTION(startSandboxInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_STARTSANDBOXINSTANCE, R"*(
Starts an existing MySQL Server instance on localhost.

@param port The port where the instance listens for MySQL connections.
@param options Optional dictionary with options affecting the result.

@returns Nothing.

This function will start an existing MySQL Server instance on the local host.
The following options affect the result:

@li sandboxDir: path where the instance is located.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems
or @%userprofile@%\\MySQL\\mysql-sandboxes on Windows systems.

If the instance is not located on the used path an error will occur.

@throw ArgumentError in the following scenarios:
@li If the options contain an invalid attribute.
@li If the port value is < 1024 or > 65535.
)*");

/**
 * $(DBA_STARTSANDBOXINSTANCE_BRIEF)
 *
 * $(DBA_STARTSANDBOXINSTANCE)
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
  bool interactive = current_shell_options()->get().wizards;
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

    // Establish the session to the target instance
    if (instance_def.has_data()) {
      validate_connection_options(instance_def);

      instance_session = establish_mysql_session(instance_def, interactive);
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
    // Get the Connection_options
    Connection_options coptions = instance_session->get_connection_options();

    // Close the session
    instance_session->close();

    // Call the API
    std::unique_ptr<Configure_instance> op_configure_instance;
    if (local)
      op_configure_instance.reset(new Configure_local_instance(
          coptions, mycnf_path, output_mycnf_path, cluster_admin,
          cluster_admin_password, clear_read_only, interactive, restart));
    else
      op_configure_instance.reset(new Configure_instance(
          coptions, mycnf_path, output_mycnf_path, cluster_admin,
          cluster_admin_password, clear_read_only, interactive, restart));

    op_configure_instance->prepare();
    ret_val = shcore::Value(op_configure_instance->execute());
    op_configure_instance->finish();
  }

  return ret_val;
}

REGISTER_HELP_FUNCTION(configureLocalInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CONFIGURELOCALINSTANCE, R"*(
Validates and configures a local instance for MySQL InnoDB Cluster usage.

@param instance An instance definition.
@param options Optional Additional options for the operation.

@returns Nothing

This function reviews the instance configuration to identify if it is valid for
usage in group replication and cluster. An exception is thrown if not.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

${CONFIGURE_INSTANCE_COMMON_OPTIONS}

${CONFIGURE_INSTANCE_COMMON_DETAILS_1}

The returned descriptive text of the operation result indicates whether the
instance was successfully configured for InnoDB Cluster usage or if it was
already valid for InnoDB Cluster usage.

${CONFIGURE_INSTANCE_COMMON_DETAILS_2}

@throw ArgumentError in the following scenarios:
@li If the instance parameter is empty.
@li If the instance definition is invalid.
@li If the instance definition is a connection dictionary but empty.
@li If the instance definition is a connection dictionary but any option is
invalid.
@li If the instance definition is missing the password.
@li If the provided password is empty.
@li If the clusterAdminPassword is provided and clusterAdmin is not provided.
@li If the clusterAdminPassword is provided but the clusterAdmin account already
exists.
@li If the configuration file path is required but not provided or wrong.

@throw RuntimeError in the following scenarios:
@li If the instance accounts are invalid.
@li If the instance is offline.
@li If the instance is already part of a Replication Group.
@li If the given instance cannot be used for Group Replication.
)*");

/**
 * $(DBA_CONFIGURELOCALINSTANCE_BRIEF)
 *
 * $(DBA_CONFIGURELOCALINSTANCE)
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
REGISTER_HELP_TOPIC_TEXT(CONFIGURE_INSTANCE_COMMON_OPTIONS, R"*(
The options dictionary may contain the following options:

@li mycnfPath: The path to the MySQL configuration file of the instance.
@li outputMycnfPath: Alternative output path to write the MySQL configuration
file of the instance.
@li password: The password to be used on the connection.
@li clusterAdmin: The name of the InnoDB cluster administrator account.
@li clusterAdminPassword: The password for the InnoDB cluster administrator
account.
@li clearReadOnly: boolean value used to confirm that super_read_only must be
disabled.
@li interactive: boolean value used to disable the wizards in the command
execution, i.e. prompts are not provided to the user and confirmation prompts
are not shown.
)*");

REGISTER_HELP_TOPIC_TEXT(CONFIGURE_INSTANCE_COMMON_DETAILS_1, R"*(
If the outputMycnfPath option is used, only that file is updated and mycnfPath
is treated as read-only.

The connection password may be contained on the instance definition, however,
it can be overwritten if it is specified on the options.

The clusterAdmin must be a standard MySQL account name. It could be either an
existing account or an account to be created.

The clusterAdminPassword must be specified only if the clusterAdmin account will
be created.
)*");

REGISTER_HELP_TOPIC_TEXT(CONFIGURE_INSTANCE_COMMON_DETAILS_2, R"*(
If the instance was not valid for InnoDB Cluster and interaction is enabled,
before configuring the instance a prompt to confirm the changes is presented
and a table with the following information:

@li Variable: the invalid configuration variable.
@li Current Value: the current value for the invalid configuration variable.
@li Required Value: the required value for the configuration variable.
)*");

REGISTER_HELP_FUNCTION(configureInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CONFIGUREINSTANCE, R"*(
Validates and configures an instance for MySQL InnoDB Cluster usage.

@param instance Optional An instance definition.
@param options Optional Additional options for the operation.

@returns A descriptive text of the operation result.

This function auto-configures the instance for InnoDB Cluster usage.If the
target instance already belongs to an InnoDB Cluster it errors out.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}

${CONFIGURE_INSTANCE_COMMON_OPTIONS}
@li restart: boolean value used to indicate that a remote restart of the target
instance should be performed to finalize the operation.

${CONFIGURE_INSTANCE_COMMON_DETAILS_1}

This function reviews the instance configuration to identify if it is valid for
usage in group replication and cluster. An exception is thrown if not.

${CONFIGURE_INSTANCE_COMMON_DETAILS_2}

@throw ArgumentError in the following scenarios:
@li If 'interactive' is disabled and the instance parameter is empty.
@li If the instance definition is invalid.
@li If the instance definition is a connection dictionary but empty.
@li If the instance definition is a connection dictionary but any option is
invalid.
@li If 'interactive' mode is disabled and the instance definition is missing
the password.
@li If 'interactive' mode is enabled and the provided password is empty.
@li If the clusterAdminPassword is provided and clusterAdmin is not provided.
@li If the clusterAdminPassword is provided but the clusterAdmin account already
exists.

@throw RuntimeError in the following scenarios:
@li If the configuration file path is required but not provided or wrong.
@li If the instance accounts are invalid.
@li If the instance is offline.
@li If the instance is already part of a Replication Group.
@li If the instance is already part of an InnoDB Cluster.
@li If the given instance cannot be used for Group Replication.
)*");

/**
 * $(DBA_CONFIGUREINSTANCE_BRIEF)
 *
 * $(DBA_CONFIGUREINSTANCE)
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

REGISTER_HELP_FUNCTION(rebootClusterFromCompleteOutage, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE, R"*(
Brings a cluster back ONLINE when all members are OFFLINE.

@param clusterName Optional The name of the cluster to be rebooted.
@param options Optional dictionary with options that modify the behavior of
this function.

@returns The rebooted cluster object.

The options dictionary can contain the next values:

@li password: The password used for the instances sessions required operations.
@li removeInstances: The list of instances to be removed from the cluster.
@li rejoinInstances: The list of instances to be rejoined on the cluster.
@li clearReadOnly: boolean value used to confirm that super_read_only must be
disabled

@attention The clearReadOnly option will be removed in a future release.

This function reboots a cluster from complete outage. It picks the instance the
MySQL Shell is connected to as new seed instance and recovers the cluster.
Optionally it also updates the cluster configuration based on user provided
options.

On success, the restored cluster object is returned by the function.

The current session must be connected to a former instance of the cluster.

If name is not specified, the default cluster will be returned.

@throw MetadataError in the following scenarios:
@li If the Metadata is inaccessible.

@throw ArgumentError in the following scenarios:
@li If the Cluster name is empty.
@li If the Cluster name is not valid.
@li If the options contain an invalid attribute.

@throw RuntimeError in the following scenarios:
@li If the Cluster does not exist on the Metadata.
@li If some instance of the Cluster belongs to a Replication Group.
)*");

/**
 * $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE_BRIEF)
 *
 * $(DBA_REBOOTCLUSTERFROMCOMPLETEOUTAGE)
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

  mysqlshdk::mysql::Instance target_instance(group_session);

  shcore::Value ret_val;
  std::string instance_session_address;
  shcore::Value::Map_type_ref options;
  std::shared_ptr<mysqlsh::dba::Cluster> cluster;
  std::shared_ptr<mysqlsh::dba::ReplicaSet> default_replicaset;
  shcore::Value::Array_type_ref remove_instances_ref, rejoin_instances_ref;
  std::vector<std::string> remove_instances_list, rejoin_instances_list,
      instances_lists_intersection;

  std::string cluster_name;
  try {
    bool default_cluster = false;
    auto console = mysqlsh::current_console();

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

      if (opt_map.has_key("clearReadOnly")) {
        opt_map.bool_at("clearReadOnly");
        std::string warn_msg =
            "The clearReadOnly option is deprecated. The super_read_only mode "
            "is now automatically cleared.";
        console->print_warning(warn_msg);
        console->println();
      }
    }

    // Check if removeInstances and/or rejoinInstances are specified
    // And if so add them to simple vectors so the check for types is done
    // before moving on in the function logic
    if (remove_instances_ref) {
      for (auto value : *remove_instances_ref.get()) {
        // Check if seed instance is present on the list
        if (value.get_string() == instance_session_address)
          throw shcore::Exception::argument_error(
              "The current session instance "
              "cannot be used on the 'removeInstances' list.");

        remove_instances_list.push_back(value.get_string());
      }
    }

    if (rejoin_instances_ref) {
      for (auto value : *rejoin_instances_ref.get()) {
        // Check if seed instance is present on the list
        if (value.get_string() == instance_session_address)
          throw shcore::Exception::argument_error(
              "The current session instance "
              "cannot be used on the 'rejoinInstances' list.");
        rejoin_instances_list.push_back(value.get_string());
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
      metadata->load_default_cluster(cluster->impl());
    } else {
      metadata->load_cluster(cluster_name, cluster->impl());
    }

    mysqlshdk::mysql::Instance group_instance(group_session);

    if (cluster) {
      // Set the cluster as return value
      ret_val =
          shcore::Value(std::dynamic_pointer_cast<Object_bridge>(cluster));

      // Get the default replicaset
      default_replicaset = cluster->impl()->get_default_replicaset();

      // Get get instance address in metadata.
      std::string group_md_address = group_instance.get_canonical_address();

      // 2. Ensure that the current session instance exists on the Metadata
      // Schema
      if (!metadata->is_instance_on_replicaset(default_replicaset->get_id(),
                                               group_md_address))
        throw shcore::Exception::runtime_error(
            "The current session instance does not belong "
            "to the cluster: '" +
            cluster->impl()->get_name() + "'.");

      // Ensure that all of the instances specified on the 'removeInstances'
      // list exist on the Metadata Schema and are valid
      for (const auto &value : remove_instances_list) {
        shcore::Argument_list args;
        args.push_back(shcore::Value(value));

        std::string md_address = value;
        try {
          auto instance_def = mysqlsh::get_connection_options(
              args, mysqlsh::PasswordFormat::NONE);

          // Get the instance metadata address (reported host).
          md_address = mysqlsh::dba::get_report_host_address(
              instance_def, current_session_options);
        } catch (const std::exception &e) {
          std::string error(e.what());
          throw shcore::Exception::argument_error(
              "Invalid value '" + value + "' for 'removeInstances': " + error);
        }

        if (!metadata->is_instance_on_replicaset(default_replicaset->get_id(),
                                                 md_address))
          throw shcore::Exception::runtime_error("The instance '" + value +
                                                 "' does not belong "
                                                 "to the cluster: '" +
                                                 cluster->impl()->get_name() +
                                                 "'.");
      }

      // Ensure that all of the instances specified on the 'rejoinInstances'
      // list exist on the Metadata Schema and are valid
      for (const auto &value : rejoin_instances_list) {
        shcore::Argument_list args;
        args.push_back(shcore::Value(value));

        std::string md_address = value;
        try {
          auto instance_def = mysqlsh::get_connection_options(
              args, mysqlsh::PasswordFormat::NONE);

          // Get the instance metadata address (reported host).
          md_address = mysqlsh::dba::get_report_host_address(
              instance_def, current_session_options);
        } catch (const std::exception &e) {
          std::string error(e.what());
          throw shcore::Exception::argument_error(
              "Invalid value '" + value + "' for 'rejoinInstances': " + error);
        }

        if (!metadata->is_instance_on_replicaset(default_replicaset->get_id(),
                                                 md_address))
          throw shcore::Exception::runtime_error("The instance '" + value +
                                                 "' does not belong "
                                                 "to the cluster: '" +
                                                 cluster->impl()->get_name() +
                                                 "'.");
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
    if (metadata->is_cluster_empty(cluster->impl()->get_id()))
      throw shcore::Exception::runtime_error(
          "The cluster has no instances in it.");

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
    validate_instances_gtid_reboot_cluster(cluster, options, group_instance);

    // 6. Set the current session instance as the seed instance of the Cluster
    {
      // Disable super_read_only mode if it is enabled.
      bool super_read_only =
          group_instance.get_sysvar_bool("super_read_only").get_safe();
      if (super_read_only) {
        console->print_info("Disabling super_read_only mode on instance '" +
                            group_instance.descr() + "'.");
        log_debug(
            "Disabling super_read_only mode on instance '%s' to run "
            "rebootClusterFromCompleteOutage.",
            group_instance.descr().c_str());
        group_instance.set_sysvar("super_read_only", false);
      }
      Group_replication_options gr_options(Group_replication_options::NONE);

      Clone_options clone_options(Clone_options::NONE);

      // The current 'group_replication_group_name' must be kept otherwise
      // if instances are rejoined later the operation may fail because
      // a new group_name started being used.
      // This must be done before rejoining the instances due to the fixes for
      // BUG #26159339: SHELL: ADMINAPI DOES NOT TAKE GROUP_NAME INTO ACCOUNT
      gr_options.group_name = default_replicaset->get_group_name();

      // BUG#29265869: reboot cluster overrides some GR settings.
      // Read actual GR configurations to preserve them in the seed instance.
      gr_options.read_option_values(target_instance);

      // BUG#27344040: REBOOT CLUSTER SHOULD NOT CREATE NEW USER
      // No new replication user should be created when rebooting a cluster and
      // existing recovery users should be reused. Therefore the boolean to
      // skip replication users is set to true and the respective replication
      // user credentials left empty.
      try {
        bool interactive = current_shell_options()->get().wizards;

        // Create the add_instance command and execute it.
        Add_instance op_add_instance(&target_instance, *default_replicaset,
                                     gr_options, clone_options, {}, interactive,
                                     0, "", "", true, true, true);

        // Always execute finish when leaving scope.
        auto finally = shcore::on_leave_scope(
            [&op_add_instance]() { op_add_instance.finish(); });

        // Prepare the add_instance command execution (validations).
        op_add_instance.prepare();

        // Execute add_instance operations.
        op_add_instance.execute();
      } catch (...) {
        // catch any exception that is thrown, restore super read-only-mode if
        // it was enabled and re-throw the caught exception.
        if (super_read_only) {
          log_debug(
              "Restoring super_read_only mode on instance '%s' to 'ON' since "
              "it was enabled before rebootClusterFromCompleteOutage operation "
              "started.",
              group_instance.descr().c_str());
          console->print_info(
              "Restoring super_read_only mode on instance '" +
              group_instance.descr() +
              "' since dba.<<<rebootClusterFromCompleteOutage>>>() "
              "operation failed.");
          group_instance.set_sysvar("super_read_only", true);
        }
        throw;
      }
    }

    // 7. Update the Metadata Schema information
    // 7.1 Remove the list of instances of "removeInstances" from the Metadata
    default_replicaset->remove_instances(remove_instances_list);

    // 8. Rejoin the list of instances of "rejoinInstances"
    default_replicaset->rejoin_instances(rejoin_instances_list, options);

    // Handling of GR protocol version:
    // Verify if an upgrade of the protocol is required
    if (!remove_instances_list.empty()) {
      mysqlshdk::mysql::Instance cluster_session_instance(group_session);

      mysqlshdk::utils::Version gr_protocol_version_to_upgrade;

      // After removing instance, the remove command must set
      // the GR protocol of the group to the lowest MySQL version on the group.
      try {
        if (mysqlshdk::gr::is_protocol_upgrade_required(
                cluster_session_instance,
                mysqlshdk::utils::nullable<std::string>(),
                &gr_protocol_version_to_upgrade)) {
          mysqlshdk::gr::set_group_protocol_version(
              cluster_session_instance, gr_protocol_version_to_upgrade);
        }
      } catch (const shcore::Exception &error) {
        // The UDF may fail with MySQL Error 1123 if any of the members is
        // RECOVERING In such scenario, we must abort the upgrade protocol
        // version process and warn the user
        if (error.code() == ER_CANT_INITIALIZE_UDF) {
          auto console = mysqlsh::current_console();
          console->print_note(
              "Unable to determine the Group Replication protocol version, "
              "while verifying if a protocol upgrade would be possible: " +
              std::string(error.what()) + ".");
        } else {
          throw;
        }
      }
    }
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
  std::string instance_address, conn_status;

  log_info("Checking instance status for cluster '%s'",
           cluster->impl()->get_name().c_str());

  std::vector<Instance_definition> instances =
      cluster->impl()->get_default_replicaset()->get_instances_from_metadata();

  auto current_session_options =
      cluster->impl()->get_group_session()->get_connection_options();

  // Get the current session instance reported host address
  mysqlshdk::mysql::Instance group_instance(
      cluster->impl()->get_group_session());
  std::string active_session_md_address =
      group_instance.get_canonical_address();

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
    if (instance_address == active_session_md_address) {
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
    } catch (const std::exception &e) {
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
    case GRInstanceType::InnoDBCluster: {
      std::string err_msg = "The MySQL instance '" + member_session_address +
                            "' belongs to an InnoDB Cluster and is reachable.";
      // Check if quorum is lost to add additional instructions to users.
      mysqlshdk::mysql::Instance target_instance(member_session);
      if (!mysqlshdk::gr::has_quorum(target_instance, nullptr, nullptr)) {
        err_msg += " Please use <Cluster>." + restore_function +
                   "() to restore from the quorum loss.";
      }
      throw shcore::Exception::runtime_error(err_msg);
    }

    case GRInstanceType::GroupReplication:
      throw shcore::Exception::runtime_error(
          "The MySQL instance '" + member_session_address +
          "' belongs to a GR group that is not managed as an "
          "InnoDB cluster. ");

    case GRInstanceType::Standalone:
    case GRInstanceType::StandaloneWithMetadata:
    case GRInstanceType::StandaloneInMetadata:
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
      get_member_name("forceQuorumUsingPartitionOf",
                      shcore::current_naming_style()));

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
  // TODO- this should just get a list of instances, the status check
  // is redundant
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
    } catch (const std::exception &e) {
      throw shcore::Exception::runtime_error("Could not open connection to " +
                                             instance_address + "");
    }

    log_info("Checking state of instance '%s'", instance_address.c_str());
    validate_instance_belongs_to_cluster(
        session, "",
        get_member_name("forceQuorumUsingPartitionOf",
                        shcore::current_naming_style()));
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
    const mysqlshdk::mysql::IInstance &target_instance) {
  std::shared_ptr<mysqlshdk::db::ISession> session;
  auto console = current_console();

  // get the current session information
  auto current_session_options = target_instance.get_connection_options();

  if (options) {
    mysqlsh::set_user_from_map(&current_session_options, options);
    mysqlsh::set_password_from_map(&current_session_options, options);
  }

  std::vector<Instance_gtid_info> instance_gtids;
  {
    Instance_gtid_info info;
    info.server = target_instance.get_canonical_address();
    info.gtid_executed =
        mysqlshdk::mysql::get_executed_gtid_set(target_instance, true);
    instance_gtids.push_back(info);
  }

  // Get the cluster instances
  std::vector<Instance_definition> instances =
      cluster->impl()->get_default_replicaset()->get_instances_from_metadata();

  for (const auto &inst : instances) {
    if (inst.endpoint == instance_gtids[0].server) continue;

    auto connection_options =
        shcore::get_connection_options(inst.endpoint, false);
    connection_options.set_user(current_session_options.get_user());
    connection_options.set_password(current_session_options.get_password());

    // Connect to the instance to obtain the GLOBAL.GTID_EXECUTED
    try {
      log_info("Opening a new session to the instance for gtid validations %s",
               inst.endpoint.c_str());
      session = get_session(connection_options);
    } catch (const std::exception &e) {
      log_warning("Could not open a connection to %s: %s",
                  inst.endpoint.c_str(), e.what());
      continue;
    }

    Instance_gtid_info info;
    info.server = inst.endpoint;
    info.gtid_executed = mysqlshdk::mysql::get_executed_gtid_set(
        mysqlshdk::mysql::Instance(session), true);
    instance_gtids.push_back(info);
  }

  std::vector<Instance_gtid_info> primary_candidates;

  try {
    primary_candidates =
        filter_primary_candidates(target_instance, instance_gtids);

    // Returned list should have at least 1 element
    assert(!primary_candidates.empty());
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_DATA_INCONSISTENT_ERRANT_TRANSACTIONS) {
      console->print_error(e.what());
      // TODO(alfredo) - suggest a way to recover from this scenario
    }
    throw;
  }

  // Check if the most updated instance is not the current session instance
  if (std::find_if(primary_candidates.begin(), primary_candidates.end(),
                   [&instance_gtids](const Instance_gtid_info &c) {
                     return c.server == instance_gtids[0].server;
                   }) == primary_candidates.end()) {
    throw shcore::Exception::runtime_error(
        "The active session instance isn't the most updated "
        "in comparison with the ONLINE instances of the Cluster's "
        "metadata. Please use the most up to date instance: '" +
        primary_candidates.front().server + "'.");
  }
}

}  // namespace dba
}  // namespace mysqlsh
