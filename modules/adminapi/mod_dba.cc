/*
 * Copyright (c) 2016, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/cluster/replicaset/add_instance.h"
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
#include "modules/adminapi/dba/upgrade_metadata.h"
#include "modules/adminapi/dba_utils.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_replica_set.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_shell.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/object_factory.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_console.h"

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

namespace sandbox {
struct Op_data {
  std::string name;
  std::string progressive;
  std::string past;
};

static std::map<std::string, Op_data> Operations_text{
    {"delete", {"deleteSandboxInstance", "Deleting", "deleted"}},
    {"kill", {"killSandboxInstance", "Killing", "killed"}},
    {"stop", {"stopSandboxInstance", "Stopping", "stopped"}},
    {"deploy",
     {"deploySandboxInstance", "Deploying new", "deployed and started"}},
    {"start", {"startSandboxInstance", "Starting", "started"}}};

}  // namespace sandbox

namespace {
void validate_port(int port, const std::string &name) {
  if (port < 1024 || port > 65535)
    throw shcore::Exception::argument_error(
        "Invalid value for '" + name +
        "': Please use a valid TCP port number >= 1024 and <= 65535");
}

std::string get_sandbox_dir(shcore::Argument_map *opt_map = nullptr) {
  std::string sandbox_dir =
      mysqlsh::current_shell_options()->get().sandbox_directory;

  if (opt_map) {
    if (opt_map->has_key("sandboxDir")) {
      sandbox_dir = opt_map->string_at("sandboxDir");

      // NOTE this validation is not done if the sandbox dir is the one
      // At the shell options, is that intentional?
      if (!shcore::is_folder(sandbox_dir))
        throw shcore::Exception::argument_error(
            "The sandbox dir path '" + sandbox_dir + "' is not valid: it " +
            (shcore::path_exists(sandbox_dir) ? "is not a directory"
                                              : "does not exist") +
            ".");
    }
  }

  return sandbox_dir;
}

}  // namespace

using mysqlshdk::db::uri::formats::only_transport;
std::set<std::string> Dba::_deploy_instance_opts = {
    "portx",         "sandboxDir",     "password",
    "allowRootFrom", "ignoreSslError", "mysqldOptions"};
std::set<std::string> Dba::_stop_instance_opts = {"sandboxDir", "password"};
std::set<std::string> Dba::_default_local_instance_opts = {"sandboxDir"};

/*
 * Global helper text for setOption and setInstance Option namespaces
 */
REGISTER_HELP(NAMESPACE_TAG,
              "@li tag:@<option@>: built-in and user-defined tags to be "
              "associated to the Cluster.");

REGISTER_HELP_DETAIL_TEXT(NAMESPACE_TAG_DETAIL_CLUSTER, R"*(
<b>Tags</b>

Tags make it possible to associate custom key/value pairs to a Cluster,
storing them in its metadata. Custom tag names can be any string
starting with letters and followed by letters, numbers and _.
Tag values may be any JSON value. If the value is null, the tag is deleted.
)*");

REGISTER_HELP_DETAIL_TEXT(NAMESPACE_TAG_DETAIL_REPLICASET, R"*(
<b>Tags</b>

Tags make it possible to associate custom key/value pairs to a ReplicaSet,
storing them in its metadata. Custom tag names can be any string
starting with letters and followed by letters, numbers and _.
Tag values may be any JSON value. If the value is null, the tag is deleted.
)*");

REGISTER_HELP_DETAIL_TEXT(NAMESPACE_TAG_INSTANCE_DETAILS_EXTRA, R"*(
The following pre-defined tags are available:
@li _hidden: bool - instructs the router to exclude the instance from the
list of possible destinations for client applications.
@li _disconnect_existing_sessions_when_hidden: bool - instructs the router
to disconnect existing connections from instances that are marked to be hidden.
@note '_hidden' and '_disconnect_existing_sessions_when_hidden' can be useful to shut
down the instance and perform maintenance on it without disrupting incoming
application traffic.
)*");

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

REGISTER_HELP(
    OPT_INTERACTIVE,
    "@li interactive: boolean value used to disable/enable the wizards in the "
    "command execution, i.e. prompts and confirmations will be provided or not "
    "according to the value set. The default value is equal to MySQL Shell "
    "wizard mode.");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL, R"*(
The exitStateAction option supports the following values:
@li ABORT_SERVER: if used, the instance shuts itself down if
it leaves the cluster unintentionally or exhausts its auto-rejoin attempts.
@li READ_ONLY: if used, the instance switches itself to
super-read-only mode if it leaves the cluster unintentionally or exhausts
its auto-rejoin attempts.
@li OFFLINE_MODE: if used, the instance switches itself to offline mode if
it leaves the cluster unintentionally or exhausts its auto-rejoin attempts.
Requires MySQL 8.0.18 or newer.

If exitStateAction is not specified READ_ONLY will be used by default.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA, R"*(
The value for exitStateAction is used to configure how Group
Replication behaves when a server instance leaves the group
unintentionally (for example after encountering an applier
error) or exhausts its auto-rejoin attempts.
When set to ABORT_SERVER, the instance shuts itself
down.
When set to READ_ONLY the server switches itself to
super-read-only mode.
When set to OFFLINE_MODE it switches itself to offline mode.
In this mode, connected client users are disconnected on their
next request and connections are no longer accepted, with the
exception of client users that have the CONNECTION_ADMIN or
SUPER privilege.
The exitStateAction option accepts case-insensitive string
values, being the accepted values: OFFLINE_MODE (or 2),
ABORT_SERVER (or 1) and READ_ONLY (or 0).

The default value is READ_ONLY.
)*");

REGISTER_HELP_DETAIL_TEXT(CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA, R"*(
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

// TODO create a dedicated topic for InnoDB clusters and replicasets,
// with a quick tutorial for both, in addition to more deep technical info

// Documentation of the DBA Class
REGISTER_HELP_TOPIC(AdminAPI, CATEGORY, adminapi, Contents, SCRIPTING);
REGISTER_HELP_TOPIC_WITH_BRIEF_TEXT(ADMINAPI, R"*(
The <b>AdminAPI</b> is an API that enables configuring and managing InnoDB
clusters and replicasets, among other things.

The AdminAPI can be used interactively from the MySQL Shell prompt and
non-interactively from JavaScript and Python scripts and directly from the
command line.

For more information about the <b>dba</b> object use: \\? dba

In the AdminAPI, an InnoDB cluster is represented as an instance of the
<b>Cluster</b> class, while replicasets are represented as an instance
of the <b>ReplicaSet</b> class.

For more information about the <b>Cluster</b> class use: \\? Cluster

For more information about the <b>ReplicaSet</b> class use: \\? ReplicaSet

<b>Scripting</b>

Through the JavaScript and Python bindings of the MySQL Shell, the AdminAPI can
be used from scripts, which can in turn be used interactively or
non-interactively. To execute a script, use the -f command line option, followed
by the script file path. Options that follow the path are passed directly
to the script being executed, which can access them from sys.argv

@code
    mysqlsh root@localhost -f myscript.py arg1 arg2
@endcode

If the script finishes successfully, the Shell will exit with code 0, while
uncaught exceptions/errors cause it to exist with a non-0 code.

By default, the AdminAPI enables interactivity, which will cause functions to
prompt for missing passwords, confirmations and bits of information that cannot
be obtained automatically.

Prompts can be completely disabled with the --no-wizard command line option
or using the "interactive" boolean option available in some of functions.
If interactivity is disabled and some information is missing (e.g. a password),
an error will be raised instead of being prompted.

<b>Secure Password Handling</b>

Passwords can be safely stored locally, using the system's native secrets
storage functionality (or login-paths in Linux). Whenever the Shell needs
a password, it will first query for the password in the system, before
prompting for it.

Passwords can be stored during interactive use, by confirming in the Store
Password prompt. They can also be stored programmatically, using the
shell.<<<storeCredential>>>() function.

You can also use environment variables to pass information to your scripts.
In JavaScript, the os.getenv() function can be used to access them.

<b>Command Line Interface</b>

In addition to the scripting interface, the MySQL Shell supports generic
command line integration, which allows calling AdminAPI functions directly
from the system shell (e.g. bash). Examples:

@code
    $ mysqlsh -- dba configure-instance root@localhost

    is equivalent to:

    > dba.configureInstance("root@localhost")

    $ mysqlsh root@localhost -- cluster status --extended

    is equivalent to:

    > dba.getCluster().status({extended:true})
@endcode

The mapping from AdminAPI function signatures works as follows:

@li The first argument after a -- can be a shell global object, such as dba.
As a special case, cluster and rs are also accepted.
@li The second argument is the name of the function of the object to be called.
The naming convention is automatically converted from camelCase/snake_case
to lower case separated by dashes.
@li The rest of the arguments are used in the same order as their JS/Python
counterparts. Instances can be given as URIs. Option dictionaries can be passed
as --options, where the option name is the same as in JS/Python.
)*");

REGISTER_HELP_GLOBAL_OBJECT(dba, adminapi);
REGISTER_HELP(DBA_BRIEF, "InnoDB cluster and replicaset management functions.");
REGISTER_HELP(DBA_GLOBAL_BRIEF, "Used for InnoDB cluster administration.");

REGISTER_HELP_TOPIC_TEXT(DBA, R"*(
Entry point for AdminAPI functions, including InnoDB clusters and replica
sets.

<b>InnoDB clusters</b>

The dba.<<<configureInstance>>>() function can be used to configure a MySQL
instance with the settings required to use it in an InnoDB cluster.

InnoDB clusters can be created with the dba.<<<createCluster>>>() function.

Once created, InnoDB cluster management objects can be obtained with the
dba.<<<getCluster>>>() function.

<b>InnoDB ReplicaSets</b>

The dba.<<<configureReplicaSetInstance>>>() function can be used to configure a
MySQL instance with the settings required to use it in a replicaset.

ReplicaSets can be created with the dba.<<<createReplicaSet>>>()
function.

Once created, replicaset management objects can be obtained with the
dba.<<<getReplicaSet>>>() function.

<b>Sandboxes</b>

Utility functions are provided to create sandbox MySQL instances, which can
be used to create test clusters and replicasets.
)*");

REGISTER_HELP(DBA_CLOSING, "<b>SEE ALSO</b>");
REGISTER_HELP(
    DBA_CLOSING1,
    "@li For general information about the AdminAPI use: \\? AdminAPI");
REGISTER_HELP(
    DBA_CLOSING2,
    "@li For help on a specific function use: \\? dba.<functionName>");
REGISTER_HELP(DBA_CLOSING3, "e.g. \\? dba.<<<deploySandboxInstance>>>");

Dba::Dba(shcore::IShell_core *owner) : _shell_core(owner) { init(); }

Dba::~Dba() {}

bool Dba::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Dba::init() {
  add_property("verbose");

  // Pure functions
  expose("createCluster", &Dba::create_cluster, "clusterName", "?options");

  expose("getCluster", &Dba::get_cluster, "?clusterName", "?options");
  expose("dropMetadataSchema", &Dba::drop_metadata_schema, "?options");
  expose("checkInstanceConfiguration", &Dba::check_instance_configuration,
         "?instanceDef", "?options");
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
  expose("rebootClusterFromCompleteOutage",
         &Dba::reboot_cluster_from_complete_outage, "?clusterName", "?options");
  expose("upgradeMetadata", &Dba::upgrade_metadata, "?options");
  expose("configureLocalInstance", &Dba::configure_local_instance,
         "?instanceDef", "?options");
  expose("configureInstance", &Dba::configure_instance, "?instanceDef",
         "?options");
  expose("configureReplicaSetInstance", &Dba::configure_replica_set_instance,
         "?instanceDef", "?options");
  expose("createReplicaSet", &Dba::create_replica_set, "replicaSetName",
         "?options");
  expose("getReplicaSet", &Dba::get_replica_set);

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
REGISTER_HELP(DBA_VERBOSE_BRIEF,
              "Controls debug message verbosity for sandbox related <b>dba</b> "
              "operations.");
REGISTER_HELP(DBA_VERBOSE_DETAIL,
              "The assigned value can be either boolean or integer, the result "
              "depends on the assigned value:");
REGISTER_HELP(DBA_VERBOSE_DETAIL1, "@li 0: disables mysqlprovision verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL2, "@li 1: enables mysqlprovision verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL3,
              "@li >1: enables mysqlprovision debug verbosity");
REGISTER_HELP(DBA_VERBOSE_DETAIL4,
              "@li Boolean: equivalent to assign either 0 or 1");

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
std::shared_ptr<Instance> Dba::connect_to_target_member() const {
  DBUG_TRACE;

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

    return Instance::connect(coptions, false);
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
    std::shared_ptr<Instance> target_member,
    std::shared_ptr<MetadataStorage> *out_metadata,
    std::shared_ptr<Instance> *out_group_server,
    bool connect_to_primary) const {
  bool owns_target_member_session = false;
  if (!target_member) {
    target_member = connect_to_target_member();
    owns_target_member_session = true;
  }
  if (!target_member)
    throw shcore::Exception::logic_error(
        "The shell must be connected to a member of the InnoDB cluster being "
        "managed");

  if (connect_to_primary) {
    std::string primary_uri = find_primary_member_uri(target_member, false);

    if (primary_uri.empty()) {
      throw shcore::Exception::runtime_error(
          "Unable to find a primary member in the cluster");
    } else if (primary_uri !=
               target_member->get_connection_options().uri_endpoint()) {
      log_info("%s is not a primary, will try to find one and reconnect",
               target_member->get_connection_options().as_uri().c_str());

      try {
        mysqlshdk::db::Connection_options coptions(primary_uri);

        coptions.set_login_options_from(
            target_member->get_connection_options());
        coptions.set_ssl_connection_options_from(
            target_member->get_connection_options().get_ssl_options());

        log_info("Opening session to primary of InnoDB cluster at %s...",
                 coptions.as_uri().c_str());

        auto instance = Instance::connect(coptions);
        mysqlshdk::gr::Member_state state =
            mysqlshdk::gr::get_member_state(*instance);

        // Check whether the member we connected to is really ONLINE. If e.g.
        // the PRIMARY restarted, it will not be expelled from the group until
        // some time has passed.
        if (state != mysqlshdk::gr::Member_state::ONLINE) {
          log_warning("PRIMARY member %s is currently in state %s",
                      instance->descr().c_str(),
                      mysqlshdk::gr::to_string(state).c_str());
          throw shcore::Exception("Group PRIMARY is not ONLINE",
                                  SHERR_DBA_GROUP_MEMBER_NOT_ONLINE);
        }

        log_info(
            "Metadata and group sessions are now using the primary member");
        if (owns_target_member_session) target_member->close_session();
        target_member = instance;
      } catch (const shcore::Exception &) {
        throw;
      } catch (const std::exception &e) {
        throw shcore::Exception::runtime_error(
            std::string("Unable to find a primary member in the cluster: ") +
            e.what());
      }
    }
  }

  if (out_group_server) *out_group_server = target_member;

  // Metadata is always stored in the group, so for now the session can be
  // shared
  if (out_metadata) out_metadata->reset(new MetadataStorage(target_member));
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
std::shared_ptr<Cluster> Dba::get_cluster(
    const mysqlshdk::utils::nullable<std::string> &cluster_name,
    const shcore::Dictionary_t &options) const {
  // TODO(alfredo) - suggest running dba.diagnose() in case it's a dead
  // cluster that needs reboot
  try {
    bool connect_to_primary = true;
    bool fallback_to_anything = true;

    check_function_preconditions(
        "Dba.getCluster",
        std::make_shared<Instance>(get_active_shell_session()));

    if (options) {
      Unpack_options(options)
          .optional("connectToPrimary", &connect_to_primary)
          .end();

      if (options->has_key("connectToPrimary")) fallback_to_anything = false;
    }

    std::shared_ptr<MetadataStorage> metadata;
    std::shared_ptr<Instance> group_server;
    auto console = mysqlsh::current_console();

    // This will throw if not a cluster member
    try {
      // Connect to the target cluster member and
      // also find the primary and connect to it, unless target is already
      // primary or connectToPrimary:false was given
      connect_to_target_group({}, &metadata, &group_server, connect_to_primary);
    } catch (const shcore::Exception &e) {
      // Print warning in case a cluster error is found (e.g., no quorum).
      if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
        console->print_warning(
            "Cluster has no quorum and cannot process write transactions: " +
            std::string(e.what()));
      } else {
        console->print_warning("Cluster error connecting to target: " +
                               e.format());
      }

      if ((e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM ||
           e.code() == SHERR_DBA_GROUP_MEMBER_NOT_ONLINE) &&
          fallback_to_anything && connect_to_primary) {
        log_info("Retrying getCluster() without connectToPrimary");
        connect_to_target_group({}, &metadata, &group_server, false);
      } else {
        throw;
      }
    }

    if (current_shell_options()->get().wizards) {
      auto state = get_cluster_check_info(*metadata);
      if (state.source_state == mysqlsh::dba::ManagedInstance::OnlineRO) {
        console->println(
            "WARNING: You are connected to an instance in state '" +
            mysqlsh::dba::ManagedInstance::describe(
                static_cast<mysqlsh::dba::ManagedInstance::State>(
                    state.source_state)) +
            "'\n"
            "Write operations on the InnoDB cluster will not be allowed.\n");
      } else if (state.source_state !=
                 mysqlsh::dba::ManagedInstance::OnlineRW) {
        console->println(
            "WARNING: You are connected to an instance in state '" +
            mysqlsh::dba::ManagedInstance::describe(
                static_cast<mysqlsh::dba::ManagedInstance::State>(
                    state.source_state)) +
            "'\n"
            "Write operations on the InnoDB cluster will not be allowed.\n"
            "Output from describe() and status() may be outdated.\n");
      }
    }

    return get_cluster(!cluster_name ? nullptr : cluster_name->c_str(),
                       metadata, group_server);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM)
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
}

std::shared_ptr<Cluster> Dba::get_cluster(
    const char *name, std::shared_ptr<MetadataStorage> metadata,
    std::shared_ptr<Instance> group_server) const {
  Cluster_metadata target_cm;
  Cluster_metadata group_server_cm;

  if (!metadata->get_cluster_for_server_uuid(group_server->get_uuid(),
                                             &group_server_cm)) {
    throw shcore::Exception("No cluster metadata found for " +
                                group_server->get_uuid() + " of instance " +
                                group_server->descr(),
                            SHERR_DBA_METADATA_MISSING);
  }

  if (!name) {
    target_cm = group_server_cm;
  } else {
    if (!metadata->get_cluster_for_cluster_name(name, &target_cm)) {
      throw shcore::Exception(
          shcore::str_format("The cluster with the name '%s' does not exist.",
                             name),
          SHERR_DBA_METADATA_MISSING);
    }
  }

  auto cluster =
      std::make_shared<Cluster_impl>(target_cm, group_server, metadata);

  // Verify if the current session instance group_replication_group_name
  // value differs from the one registered in the Metadata
  if (!validate_replicaset_group_name(*group_server,
                                      cluster->get_group_name())) {
    std::string nice_error =
        "Unable to get an InnoDB cluster handle. "
        "The instance '" +
        group_server->descr() +
        "' may belong to a different cluster from the one registered in the "
        "Metadata since the value of 'group_replication_group_name' does "
        "not match the one registered in the Metadata: possible split-brain "
        "scenario. Please retry while connected to another member of the "
        "cluster.";

    throw shcore::Exception::runtime_error(nice_error);
  }

  return std::make_shared<mysqlsh::dba::Cluster>(cluster);
}

REGISTER_HELP_FUNCTION(createCluster, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CREATECLUSTER, R"*(
Creates a MySQL InnoDB cluster.

@param name An identifier for the cluster to be created.
@param options Optional dictionary with additional parameters described below.

@returns The created cluster object.

Creates a MySQL InnoDB cluster taking as seed instance the server the shell
is currently connected to.

The options dictionary can contain the following values:

@li disableClone: boolean value used to disable the clone usage on the cluster.
@li gtidSetIsComplete: boolean value which indicates whether the GTID set
of the seed instance corresponds to all transactions executed. Default is false.
@li multiPrimary: boolean value used to define an InnoDB cluster with multiple
writable instances.
@li force: boolean, confirms that the multiPrimary option must be applied.
${OPT_INTERACTIVE}
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

${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}

${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}

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
  Clone_options clone_options(Clone_options::CREATE_CLUSTER);
  bool adopt_from_gr = false;
  mysqlshdk::utils::nullable<bool> multi_primary;
  bool force = false;
  mysqlshdk::utils::nullable<bool> clear_read_only;
  bool interactive = current_shell_options()->get().wizards;
  std::string instance_label;

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
  std::shared_ptr<Instance> group_server;

  // We're in createCluster(), so there's no metadata yet, but the metadata
  // object can already exist now to hold a session
  // connect_to_primary must be false, because we don't have a cluster yet
  // and this will be the seed instance anyway. If we start storing metadata
  // outside the managed cluster, then this will have to be updated.
  connect_to_target_group({}, &metadata, &group_server, false);

  // Check preconditions.
  Cluster_check_info state;
  try {
    state = check_function_preconditions("Dba.createCluster", group_server);
  } catch (const shcore::Exception &e) {
    std::string error(e.what());
    if (error.find("already in an InnoDB cluster") != std::string::npos) {
      /*
       * For V1.0 we only support one single Cluster. That one shall be the
       * default Cluster. We must check if there's already a Default Cluster
       * assigned, and if so thrown an exception. And we must check if there's
       * already one Cluster on the MD and if so assign it to Default
       */

      std::string nice_error =
          get_function_name("createCluster") +
          ": Unable to create cluster. The instance '" + group_server->descr() +
          "' already belongs to an InnoDB cluster. Use "
          "dba." +
          get_function_name("getCluster", false) + "() to access it.";
      throw shcore::Exception::runtime_error(nice_error);
    } else if (error.find("instance belongs to that metadata") !=
               std::string::npos) {
      // BUG#29271400:
      // createCluster() should not be allowed in instance with metadata
      std::string nice_error =
          "dba.<<<createCluster>>>: Unable to create cluster. The instance '" +
          group_server->descr() +
          "' has a populated Metadata schema and belongs to that Metadata. Use "
          "either dba.<<<dropMetadataSchema>>>() to drop the schema, or "
          "dba.<<<rebootClusterFromCompleteOutage>>>() to reboot the cluster "
          "from complete outage.";
      throw shcore::Exception::runtime_error(shcore::str_subvars(nice_error));
    } else {
      throw;
    }
  } catch (const shcore::Error &dberr) {
    throw shcore::Exception::mysql_error_with_code(dberr.what(), dberr.code());
  }

  // Create the cluster
  {
    // Create the add_instance command and execute it.
    Create_cluster op_create_cluster(group_server, cluster_name, gr_options,
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

void Dba::drop_metadata_schema(const shcore::Dictionary_t &options) {
  mysqlshdk::utils::nullable<bool> force;
  mysqlshdk::utils::nullable<bool> clear_read_only;

  // Map with the options
  if (options) {
    Unpack_options(options)
        .optional("force", &force)
        .optional("clearReadOnly", &clear_read_only)
        .end();
  }

  auto instance = std::make_shared<Instance>(get_active_shell_session());
  auto state = check_function_preconditions("Dba.dropMetadataSchema", instance);
  auto metadata = std::make_shared<MetadataStorage>(instance);

  bool interactive = current_shell_options()->get().wizards;

  // The drop operation is only allowed on members of an InnoDB Cluster or a
  // Replica Set, and only for online members that are either RW or RO, however
  // if it is RO we need to get to a primary instance to perform the operation
  if (state.source_state == ManagedInstance::OnlineRO) {
    if (state.source_type == GRInstanceType::InnoDBCluster) {
      connect_to_target_group({}, &metadata, nullptr, true);
    } else {
      Instance_pool::Auth_options auth_opts;
      auth_opts.get(instance->get_connection_options());
      Scoped_instance_pool ipool(interactive, auth_opts);
      ipool->set_metadata(metadata);

      auto rs = get_replica_set(metadata, instance);

      rs->acquire_primary();
      auto finally = shcore::on_leave_scope([&rs]() { rs->release_primary(); });

      metadata = ipool->get_metadata();
    }
  }

  instance = metadata->get_md_server();

  auto console = current_console();

  if (force.is_null() && interactive &&
      console->confirm("Are you sure you want to remove the Metadata?",
                       mysqlsh::Prompt_answer::NO) ==
          mysqlsh::Prompt_answer::YES) {
    force = true;
  }

  if (force.get_safe(false)) {
    // Check if super_read_only is turned off and disable it if required
    // NOTE: this is left for last to avoid setting super_read_only to true
    // and right before some execution failure of the command leaving the
    // instance in an incorrect state
    validate_super_read_only(*instance, clear_read_only, interactive);

    metadata::uninstall(instance);
    if (interactive) {
      console->println("Metadata Schema successfully removed.");
    }
  } else {
    if (interactive) {
      console->println("No changes made to the Metadata Schema.");
    } else {
      throw shcore::Exception::runtime_error(
          "No operation executed, use the 'force' option");
    }
  }
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

${TOPIC_CONNECTION_MORE_INFO}

The options dictionary may contain the following options:

@li mycnfPath: Optional path to the MySQL configuration file for the instance.
Alias for verifyMyCnf
@li verifyMyCnf: Optional path to the MySQL configuration file for the
instance. If this option is given, the configuration file will be verified for
the expected option values, in addition to the global MySQL system variables.
@li password: The password to get connected to the instance.
${OPT_INTERACTIVE}

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
    const mysqlshdk::utils::nullable<Connection_options> &instance_def_,
    const shcore::Dictionary_t &options) {
  auto instance_def = instance_def_;
  const auto has_co = instance_def && instance_def->has_data();

  if (has_co) {
    set_password_from_map(instance_def.operator->(), options);
  }

  shcore::Value ret_val;
  std::shared_ptr<Instance> instance;
  bool interactive = current_shell_options()->get().wizards;
  std::string mycnf_path;

  if (options) {
    // instance_def is optional, password is used when unpacking options to
    // avoid errors when instance_def is not given
    std::string password;

    // Retrieves optional options if exists or leaves empty so the default is
    // set afterwards
    Unpack_options(options)
        .optional("mycnfPath", &mycnf_path)
        .optional("verifyMyCnf", &mycnf_path)
        .optional("interactive", &interactive)
        .optional_ci("password", &password)
        .end();
  }

  // Establish the session to the target instance
  if (has_co) {
    validate_connection_options(*instance_def);

    instance = Instance::connect(*instance_def, interactive);
  } else {
    instance = connect_to_target_member();
  }

  check_preconditions(instance, "checkInstanceConfiguration");

  // Get the Connection_options
  Connection_options coptions = instance->get_connection_options();

  // Close the session
  instance->close_session();

  // Call the API
  Check_instance op_check_instance{coptions, mycnf_path};

  op_check_instance.prepare();
  ret_val = op_check_instance.execute();
  op_check_instance.finish();

  return ret_val;
}

// -----------------------------------------------------------------------------

REGISTER_HELP_FUNCTION(getReplicaSet, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_GETREPLICASET, R"*(
Returns an object representing a ReplicaSet.

The returned object is identical to the one returned by
<<<createReplicaSet>>>() and can be used to manage the replicaset.

The function will work regardless of whether the target instance is a PRIMARY
or a SECONDARY, but its copy of the metadata is expected to be up-to-date.
This function will also work if the PRIMARY is unreachable or unavailable,
although replicaset change operations will not be possible, except for
<<<forcePrimaryInstance>>>().
)*");
/**
 * $(DBA_GETREPLICASET_BRIEF)
 *
 * $(DBA_GETREPLICASET)
 */
#if DOXYGEN_JS
ReplicaSet Dba::getReplicaSet() {}
#elif DOXYGEN_PY
ReplicaSet Dba::get_replica_set() {}
#endif
std::shared_ptr<ReplicaSet> Dba::get_replica_set() {
  const auto console = mysqlsh::current_console();
  const auto target_server = connect_to_target_member();

  check_function_preconditions("Dba.getReplicaSet", target_server);

  const auto rs = get_replica_set(
      std::make_shared<MetadataStorage>(target_server), target_server);

  console->print_info("You are connected to a member of replicaset '" +
                      rs->get_name() + "'.");

  return std::make_shared<ReplicaSet>(rs);
}

std::shared_ptr<Replica_set_impl> Dba::get_replica_set(
    const std::shared_ptr<MetadataStorage> &metadata,
    const std::shared_ptr<Instance> &target_server) {
  auto console = mysqlsh::current_console();

  Cluster_metadata target_server_cm;

  if (!metadata->get_cluster_for_server_uuid(target_server->get_uuid(),
                                             &target_server_cm)) {
    throw shcore::Exception("No metadata found for " + target_server->descr() +
                                " (" + target_server->get_uuid() + ")",
                            SHERR_DBA_METADATA_MISSING);
  }

  return std::make_shared<Replica_set_impl>(target_server_cm, target_server,
                                            metadata);
}

REGISTER_HELP_FUNCTION(createReplicaSet, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CREATEREPLICASET, R"*(
Creates a MySQL InnoDB ReplicaSet.

@param name An identifier for the replicaset to be created.
@param options Optional dictionary with additional parameters described below.

@returns The created replicaset object.

This function will create a managed replicaset using MySQL master/slave
replication, as opposed to Group Replication. The MySQL instance the shell is
connected to will be the initial PRIMARY of the replica set.

The function will perform several checks to ensure the instance state and
configuration are compatible with a managed replicaset and if so, a metadata
schema will be initialized there.

New replica instances can be added through the <<<addInstance>>>() function of
the returned replicaset object. Status of the instances and their replication
channels can be inspected with <<<status>>>().

<b>InnoDB ReplicaSets</b>

A replicaset allows managing a GTID-based MySQL replication setup, in a single
PRIMARY/multiple SECONDARY topology.

A replicaset has several limitations compared to a InnoDB cluster
and thus, it is recommended that InnoDB clusters be preferred unless not
possible. Generally, a ReplicaSet on its own does not provide High Availability.
Among its limitations are:

@li No automatic failover
@li No protection against inconsistencies or partial data loss in a crash

<b>Pre-Requisites</b>

The following is a non-exhaustive list of requirements for managed replicasets.
The dba.<<<configureInstance>>>() command can be used to make necessary
configuration changes automatically.

@li MySQL 8.0 or newer required
@li Statement Based Replication (SBR) is unsupported, only Row Based Replication
@li GTIDs required
@li Replication filters are not allowed
@li All instances in the replicaset must be managed
@li Unmanaged replication channels are not allowed in any instance

<b>Adopting an Existing Topology</b>

Existing asynchronous setups can be managed by calling this function with the
adoptFromAR option. The topology will be automatically scanned and validated,
starting from the instance the shell is connected to, and all instances that are
part of the topology will be automatically added to the replicaset.

The only changes made by this function to an adopted replicaset are the
creation of the metadata schema. Existing replication channels will not be
changed during adoption, although they will be changed during PRIMARY switch
operations.

However, it is only possible to manage setups that use supported configurations
and topology. Configuration of all instances will be checked during adoption,
to ensure they are compatible. All replication channels must be active and
their transaction sets as verified through GTID sets must be consistent.
The data set of all instances are expected to be identical, but is not verified.

<b>Options</b>

The options dictionary can contain the following values:

@li adoptFromAR: boolean value used to create the replicaset based on an
existing asynchronous replication setup.
@li instanceLabel: string a name to identify the target instance.
Defaults to hostname:port
@li dryRun: boolean if true, all validations and steps for creating a replica
set are executed, but no changes are actually made. An exception will be thrown
when finished.
@li gtidSetIsComplete: boolean value which indicates whether the GTID set
of the seed instance corresponds to all transactions executed. Default is false.
)*");
/**
 * $(DBA_CREATEREPLICASET_BRIEF)
 *
 * $(DBA_CREATEREPLICASET)
 */
#if DOXYGEN_JS
ReplicaSet Dba::createReplicaSet(String name, Dictionary options) {}
#elif DOXYGEN_PY
ReplicaSet Dba::create_replica_set(str name, dict options) {}
#endif
shcore::Value Dba::create_replica_set(const std::string &full_rs_name,
                                      const shcore::Dictionary_t &options) {
  bool adopt = false;
  bool dry_run = false;
  auto console = mysqlsh::current_console();
  bool interactive = current_shell_options()->get().wizards;
  bool gtid_set_is_complete = false;
  std::string instance_label;
  Async_replication_options ar_options;
  Global_topology_type topology_type =
      Global_topology_type::SINGLE_PRIMARY_TREE;

  Unpack_options(options)
      .unpack(&ar_options)
      // .optional("interactive", &interactive)
      .optional("adoptFromAR", &adopt)
      .optional("dryRun", &dry_run)
      .optional("instanceLabel", &instance_label)
      .optional(kGtidSetIsComplete, &gtid_set_is_complete)
      .end();

  if (adopt && !instance_label.empty()) {
    throw shcore::Exception::argument_error(
        "instanceLabel option not allowed when adoptFromAR:true");
  }

  std::shared_ptr<Instance> target_server = connect_to_target_member();
  try {
    check_function_preconditions("Dba.createReplicaSet", target_server);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER) {
      throw shcore::Exception("Unable to create replicaset. The instance '" +
                                  target_server->descr() +
                                  "' already belongs to an InnoDB cluster. Use "
                                  "dba.<<<getCluster>>>() to access it.",
                              e.code());
    } else if (e.code() == SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_REPLICASET) {
      throw shcore::Exception("Unable to create replicaset. The instance '" +
                                  target_server->descr() +
                                  "' already belongs to a replicaset. Use "
                                  "dba.<<<getReplicaSet>>>() to access it.",
                              e.code());
    } else {
      throw;
    }
  } catch (const shcore::Error &dberr) {
    throw shcore::Exception::mysql_error_with_code(dberr.what(), dberr.code());
  }

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(target_server->get_connection_options());
  Scoped_instance_pool ipool(interactive, auth_opts);

  auto cluster = Replica_set_impl::create(
      full_rs_name, topology_type, target_server, instance_label, ar_options,
      adopt, dry_run, gtid_set_is_complete);

  console->print_info(
      "ReplicaSet object successfully created for " + target_server->descr() +
      ".\nUse rs.<<<addInstance>>>()"
      " to add more asynchronously replicated instances to this replicaset and"
      " rs.<<<status>>>() to check its status.");
  console->print_info();

  return shcore::Value(std::make_shared<ReplicaSet>(cluster));
}

shcore::Value Dba::exec_instance_op(const std::string &function,
                                    const shcore::Argument_list &args,
                                    const std::string &password) {
  shcore::Value ret_val;

  shcore::Value::Map_type_ref options;  // Map with the connection data
  shcore::Value mycnf_options;

  std::string sandbox_dir;
  int port = args.int_at(0);
  validate_port(port, "port");
  int portx = 0;

  bool ignore_ssl_error = true;  // SSL errors are ignored by default.

  if (args.size() == 2) {
    options = args.map_at(1);

    // Verification of invalid attributes on the instance commands
    shcore::Argument_map opt_map(*options);

    sandbox_dir = get_sandbox_dir(&opt_map);

    if (function == "deploy") {
      if (opt_map.has_key("ignoreSslError"))
        ignore_ssl_error = opt_map.bool_at("ignoreSslError");

      if (options->has_key("mysqldOptions"))
        mycnf_options = (*options)["mysqldOptions"];

      if (options->has_key("portx")) portx = opt_map.int_at("portx");

      mycnf_options = (*options)["mysqldOptions"];

    } else if (function != "stop") {
      opt_map.ensure_keys({}, _default_local_instance_opts,
                          "the instance data");
    }
  } else {
    sandbox_dir = get_sandbox_dir();
  }

  bool interactive = current_shell_options()->get().wizards;

  // Prints the operation introductory message
  auto console = mysqlsh::current_console();

  shcore::Value::Array_type_ref errors;

  if (interactive) {
    console->println();
    console->println(sandbox::Operations_text[function].progressive +
                     " MySQL instance...");
  }

  int rc = 0;
  if (function == "deploy") {
    // First we need to create the instance
    rc = _provisioning_interface->create_sandbox(
        port, portx, sandbox_dir, password, mycnf_options, true,
        ignore_ssl_error, 0, "", &errors);
  } else if (function == "delete") {
    rc = _provisioning_interface->delete_sandbox(port, sandbox_dir, false,
                                                 &errors);
  } else if (function == "kill") {
    rc = _provisioning_interface->kill_sandbox(port, sandbox_dir, &errors);
  } else if (function == "stop") {
    rc = _provisioning_interface->stop_sandbox(port, sandbox_dir, password,
                                               &errors);
  } else if (function == "start") {
    rc = _provisioning_interface->start_sandbox(port, sandbox_dir, &errors);
  }

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
  } else if (interactive && function != "deploy") {
    console->println();
    console->println("Instance localhost:" + std::to_string(port) +
                     " successfully " +
                     sandbox::Operations_text[function].past + ".");
    console->println();
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
@li mysqldOptions: List of MySQL configuration options to write to the my.cnf
file, as option=value strings.

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

  int port = 0;
  try {
    port = args.int_at(0);
    validate_port(port, "port");

    std::string remote_root;
    std::string sandbox_dir;
    mysqlshdk::utils::nullable<std::string> password;

    if (args.size() == 2) {
      auto map = args.map_at(1);
      shcore::Argument_map opt_map(*map);

      opt_map.ensure_keys({}, _deploy_instance_opts, "the instance data");

      if (opt_map.has_key("portx")) {
        validate_port(opt_map.int_at("portx"), "portx");
      }

      sandbox_dir = get_sandbox_dir(&opt_map);

      if (opt_map.has_key("password")) password = opt_map.string_at("password");

      // create root@<addr> if needed
      // Valid values:
      // allowRootFrom: address
      // allowRootFrom: %
      // allowRootFrom: null (that is, disable the option)
      if (opt_map.has_key("allowRootFrom") &&
          opt_map.at("allowRootFrom").type != shcore::Null) {
        remote_root = opt_map.string_at("allowRootFrom");
      }
    } else {
      sandbox_dir = get_sandbox_dir();
    }

    bool interactive = current_shell_options()->get().wizards;
    auto console = mysqlsh::current_console();
    std::string path =
        shcore::path::join_path(sandbox_dir, std::to_string(port));

    if (interactive) {
      // TODO(anyone): This default value was being set on the interactive
      // layer, for that reason is kept here only if interactive, to avoid
      // changes in behavior. This should be fixed at BUG#27369121."
      //
      // When this is fixed search for the following test chunk:
      // "//@ Deploy instances (with specific innodb_page_size)."
      // The allowRootFrom:"%" option was added there to make this test pass
      // after the removal of the Interactive wrappers.
      //
      // The only reason the test was passing before was because of a BUG on the
      // test framework that was meant to execute the script in NON interative
      // mode.
      //
      // Debugging the test it effectively had options.wizards = false,
      // however the Interactive Wrappers were in place which means this
      // function was getting allowRootFrom="%" even the test was not in
      // interactive mode.
      //
      // I tried reproducing the chunk using the shell and it effectively fails
      // as expected, rather than succeeding as the test suite claimed.
      //
      // Funny thing is that the BUG in the test suite gets fixed with the
      // removal of the Interactive wrappers.
      //
      // The test mentioned above must be also revisited when BUG#27369121 is
      // addressed.
      if (remote_root.empty()) remote_root = "%";

      console->println(
          "A new MySQL sandbox instance will be created on this host in \n" +
          path +
          "\n\nWarning: Sandbox instances are only suitable for deploying and "
          "\nrunning on your local machine for testing purposes and are not "
          "\naccessible from external networks.\n");

      std::string answer;
      if (password.is_null()) {
        if (console->prompt_password(
                "Please enter a MySQL root password for the new instance: ",
                &answer) == shcore::Prompt_result::Ok) {
          password = answer;
        } else {
          return shcore::Value();
        }
      }
    }

    if (password.is_null()) {
      throw shcore::Exception::argument_error(
          "Missing root password for the deployed instance");
    }

    ret_val = exec_instance_op("deploy", args, *password);

    if (!remote_root.empty()) {
      std::string uri = "root@localhost:" + std::to_string(port);
      mysqlshdk::db::Connection_options instance_def(uri);
      instance_def.set_password(*password);

      std::shared_ptr<Instance> instance = Instance::connect(instance_def);

      log_info("Creating root@%s account for sandbox %i", remote_root.c_str(),
               port);
      instance->execute("SET sql_log_bin = 0");
      {
        std::string pwd;
        if (instance_def.has_password()) pwd = instance_def.get_password();

        shcore::sqlstring create_user(
            "CREATE USER root@? IDENTIFIED BY /*((*/ ? /*))*/", 0);
        create_user << remote_root << pwd;
        create_user.done();
        instance->execute(create_user);
      }
      {
        shcore::sqlstring grant("GRANT ALL ON *.* TO root@? WITH GRANT OPTION",
                                0);
        grant << remote_root;
        grant.done();
        instance->execute(grant);
      }
      instance->execute("SET sql_log_bin = 1");
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(fname));
  log_warning(
      "Sandbox instances are only suitable for deploying and running on "
      "your local machine for testing purposes and are not accessible "
      "from external networks.");

  if (current_shell_options()->get().wizards) {
    auto console = current_console();

    console->print_info();
    console->print_info("Instance localhost:" + std::to_string(port) +
                        " successfully deployed and started.");

    console->print_info(
        "Use shell.connect('root@localhost:" + std::to_string(port) +
        "') to connect to the instance.");
    console->print_info();
  }

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
    std::string sandbox_dir;
    int port = args.int_at(0);
    validate_port(port, "port");

    mysqlshdk::utils::nullable<std::string> password;

    if (args.size() == 2) {
      auto map = args.map_at(1);
      shcore::Argument_map opt_map(*map);

      opt_map.ensure_keys({}, _stop_instance_opts, "the instance data");

      if (opt_map.has_key("password")) password = opt_map.string_at("password");

      sandbox_dir = get_sandbox_dir(&opt_map);
    } else {
      sandbox_dir = get_sandbox_dir();
    }

    bool interactive = current_shell_options()->get().wizards;
    auto console = mysqlsh::current_console();
    std::string path =
        shcore::path::join_path(sandbox_dir, std::to_string(port));
    if (interactive) {
      console->println("The MySQL sandbox instance on this host in \n" + path +
                       " will be " + sandbox::Operations_text["stop"].past +
                       "\n");

      if (password.is_null()) {
        std::string answer;
        if (console->prompt_password(
                "Please enter the MySQL root password for the instance "
                "'localhost:" +
                    std::to_string(port) + "': ",
                &answer) == shcore::Prompt_result::Ok) {
          password = answer;
        } else {
          return shcore::Value();
        }
      }
    }

    if (password.is_null()) {
      throw shcore::Exception::argument_error(
          "Missing root password for the instance");
    }

    ret_val = exec_instance_op("stop", args, *password);
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

void Dba::do_configure_instance(
    const mysqlshdk::db::Connection_options &instance_def_,
    const shcore::Dictionary_t &options, bool local,
    Cluster_type cluster_type) {
  shcore::Value ret_val;
  mysqlshdk::db::Connection_options instance_def(instance_def_);
  std::shared_ptr<Instance> instance;

  std::string mycnf_path, output_mycnf_path, cluster_admin;
  mysqlshdk::utils::nullable<std::string> cluster_admin_password;
  mysqlshdk::utils::nullable<bool> clear_read_only;
  bool interactive = current_shell_options()->get().wizards;
  mysqlshdk::utils::nullable<bool> restart;

  {
    mysqlshdk::utils::nullable<std::string> password;
    // Retrieves optional options if exists or leaves empty so the default
    // is set afterwards
    Unpack_options unpacker(options);

    unpacker.optional("clusterAdmin", &cluster_admin)
        .optional("clusterAdminPassword", &cluster_admin_password)
        .optional("restart", &restart)
        .optional("interactive", &interactive);

    if (cluster_type == Cluster_type::GROUP_REPLICATION) {
      unpacker.optional("mycnfPath", &mycnf_path)
          .optional("outputMycnfPath", &output_mycnf_path)
          .optional("clearReadOnly", &clear_read_only)
          .optional_ci("password", &password);
    } else {
      clear_read_only = true;
    }
    unpacker.end();

    if (!password.is_null()) {
      instance_def.clear_password();
      instance_def.set_password(*password);
    }
  }

  // Establish the session to the target instance
  if (instance_def.has_data()) {
    validate_connection_options(instance_def);

    instance = Instance::connect(instance_def, interactive);
  } else {
    instance = connect_to_target_member();
  }

  // Check the function preconditions
  // (FR7) Validate if the instance is already part of a GR group
  // or InnoDB cluster
  if (cluster_type == Cluster_type::ASYNC_REPLICATION) {
    check_function_preconditions("Dba.configureReplicaSetInstance", instance);
  } else {
    check_function_preconditions(
        local ? "Dba.configureLocalInstance" : "Dba.configureInstance",
        instance);
  }

  {
    // Get the Connection_options
    Connection_options coptions = instance->get_connection_options();

    // Close the session
    // TODO(alfredo) - instead of opening a session just for the preconditions
    // and then closing it right away, the instance obj should be given to the
    // configure objects
    instance->close_session();

    // Call the API
    std::unique_ptr<Configure_instance> op_configure_instance;
    if (local)
      op_configure_instance.reset(new Configure_local_instance(
          coptions, mycnf_path, output_mycnf_path, cluster_admin,
          cluster_admin_password, clear_read_only, interactive, restart));
    else
      op_configure_instance.reset(new Configure_instance(
          coptions, mycnf_path, output_mycnf_path, cluster_admin,
          cluster_admin_password, clear_read_only, interactive, restart,
          cluster_type));

    try {
      // Always execute finish when leaving "try catch".
      auto finally = shcore::on_leave_scope(
          [&op_configure_instance]() { op_configure_instance->finish(); });

      // Prepare and execute the operation.
      op_configure_instance->prepare();
      op_configure_instance->execute();
    } catch (...) {
      op_configure_instance->rollback();
      throw;
    }
  }
}

REGISTER_HELP_FUNCTION(configureLocalInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CONFIGURELOCALINSTANCE, R"*(
Validates and configures a local instance for MySQL InnoDB Cluster usage.

@param instance An instance definition.
@param options Optional Additional options for the operation.

@returns Nothing

This function reviews the instance configuration to identify if it is valid for
usage in an InnoDB cluster, making configuration changes if necessary.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

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
void Dba::configure_local_instance(
    const mysqlshdk::utils::nullable<Connection_options> &instance_def,
    const shcore::Dictionary_t &options) {
  return do_configure_instance(
      instance_def ? *instance_def : Connection_options{}, options, true,
      Cluster_type::GROUP_REPLICATION);
}

REGISTER_HELP_FUNCTION(configureInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CONFIGUREINSTANCE, R"*(
Validates and configures an instance for MySQL InnoDB Cluster usage.

@param instance Optional An instance definition.
@param options Optional Additional options for the operation.

@returns A descriptive text of the operation result.

This function auto-configures the instance for InnoDB Cluster usage. If the
target instance already belongs to an InnoDB Cluster it errors out.

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

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

REGISTER_HELP_TOPIC_TEXT(CONFIGURE_INSTANCE_COMMON_OPTIONS, R"*(
The options dictionary may contain the following options:

@li mycnfPath: The path to the MySQL configuration file of the instance.
@li outputMycnfPath: Alternative output path to write the MySQL configuration
file of the instance.
@li password: The password to be used on the connection.
@li clusterAdmin: The name of the "cluster administrator" account.
@li clusterAdminPassword: The password for the "cluster administrator" account.
@li clearReadOnly: boolean value used to confirm that super_read_only must be
disabled.
${OPT_INTERACTIVE}
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
void Dba::configure_instance(
    const mysqlshdk::utils::nullable<Connection_options> &instance_def,
    const shcore::Dictionary_t &options) {
  do_configure_instance(instance_def ? *instance_def : Connection_options{},
                        options, false, Cluster_type::GROUP_REPLICATION);
}

REGISTER_HELP_FUNCTION(configureReplicaSetInstance, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_CONFIGUREREPLICASETINSTANCE, R"*(
Validates and configures an instance for use in an InnoDB ReplicaSet.

@param instance Optional An instance definition. By default, the active shell
session is used.
@param options Optional Additional options for the operation.

@returns Nothing

This function will verify and automatically configure the target instance for
use in an InnoDB ReplicaSet.

The function can optionally create a "cluster administrator" account, if the
"clusterAdmin" and "clusterAdminPassword" options are given. The account is
created with the minimal set of privileges required to manage InnoDB clusters
or ReplicaSets. The "cluster administrator" account must have matching username
and password across all instances of the same cluster or replicaset.

<b>Options</b>

The instance definition is the connection data for the instance.

${TOPIC_CONNECTION_MORE_INFO}

The options dictionary may contain the following options:

@li clusterAdmin: The name of a "cluster administrator" user to be
created. The supported format is the standard MySQL account name format.
@li clusterAdminPassword: The password for the "cluster administrator" account.
${OPT_INTERACTIVE}
@li restart: boolean value used to indicate that a remote restart of the target
instance should be performed to finalize the operation.

The connection password may be contained on the instance definition, however,
it can be overwritten if it is specified on the options.

This function reviews the instance configuration to identify if it is valid for
usage in replicasets. An exception is thrown if not.

If the instance was not valid for InnoDB ReplicaSet and interaction is enabled,
before configuring the instance a prompt to confirm the changes is presented
and a table with the following information:

@li Variable: the invalid configuration variable.
@li Current Value: the current value for the invalid configuration variable.
@li Required Value: the required value for the configuration variable.
)*");

/**
 * $(DBA_CONFIGUREREPLICASETINSTANCE_BRIEF)
 *
 * $(DBA_CONFIGUREREPLICASETINSTANCE)
 */
#if DOXYGEN_JS
Undefined Dba::configureReplicaSetInstance(InstanceDef instance,
                                           Dictionary options) {}
#elif DOXYGEN_PY
None Dba::configure_replica_set_instance(InstanceDef instance, dict options) {}
#endif
void Dba::configure_replica_set_instance(
    const mysqlshdk::utils::nullable<Connection_options> &instance_def,
    const shcore::Dictionary_t &options) {
  return do_configure_instance(
      instance_def ? *instance_def : Connection_options{}, options, false,
      Cluster_type::ASYNC_REPLICATION);
}

namespace {
void ensure_not_auto_rejoining(Instance *instance) {
  if (mysqlshdk::gr::is_group_replication_delayed_starting(*instance) ||
      mysqlshdk::gr::is_running_gr_auto_rejoin(*instance)) {
    auto console = current_console();
    console->print_note("Cancelling active GR auto-initialization at " +
                        instance->descr());
    mysqlshdk::gr::stop_group_replication(*instance);
  }
}
}  // namespace

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

@note When used with a metadata version lower than the one supported by this
version of the Shell, the removeInstances option is NOT allowed, as in such
scenario, no changes to the metadata are allowed.

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
Cluster Dba::rebootClusterFromCompleteOutage(String clusterName,
                                             Dictionary options) {}
#elif DOXYGEN_PY
Cluster Dba::reboot_cluster_from_complete_outage(str clusterName,
                                                 dict options) {}
#endif

std::shared_ptr<Cluster> Dba::reboot_cluster_from_complete_outage(
    const mysqlshdk::utils::nullable<std::string> &cluster_name,
    const shcore::Dictionary_t &options) {
  std::shared_ptr<MetadataStorage> metadata;
  std::shared_ptr<Instance> target_instance;
  // The cluster is completely dead, so we can't find the primary anyway...
  // thus this instance will be used as the primary
  connect_to_target_group({}, &metadata, &target_instance, false);

  check_preconditions(metadata, "rebootClusterFromCompleteOutage");

  std::string instance_session_address;
  std::shared_ptr<mysqlsh::dba::Cluster> cluster;
  std::shared_ptr<mysqlsh::dba::GRReplicaSet> default_replicaset;
  shcore::Array_t remove_instances_ref, rejoin_instances_ref;
  std::vector<std::string> remove_instances_list, rejoin_instances_list,
      instances_lists_intersection;

  bool interactive = current_shell_options()->get().wizards;
  const auto console = current_console();

  // These session options are taken as base options for further operations
  auto current_session_options = target_instance->get_connection_options();

  // Get the current session instance address
  instance_session_address = current_session_options.as_uri(only_transport());

  if (options) {
    mysqlsh::set_user_from_map(&current_session_options, options);
    mysqlsh::set_password_from_map(&current_session_options, options);

    mysqlshdk::utils::nullable<bool> clear_read_only;

    Unpack_options(options)
        .optional("removeInstances", &remove_instances_ref)
        .optional("rejoinInstances", &rejoin_instances_ref)
        .optional("clearReadOnly", &clear_read_only)
        .end();

    if (clear_read_only) {
      std::string warn_msg =
          "The clearReadOnly option is deprecated. The super_read_only mode is "
          "now automatically cleared.";
      console->print_warning(warn_msg);
      console->println();
    }
  }

  auto state = metadata->get_state();
  mysqlshdk::utils::Version md_version;
  metadata->check_version(&md_version);
  bool rebooting_old_version = state == metadata::State::MAJOR_LOWER;

  // Check if removeInstances and/or rejoinInstances are specified
  // And if so add them to simple vectors so the check for types is done
  // before moving on in the function logic
  if (remove_instances_ref) {
    if (!remove_instances_ref->empty() && rebooting_old_version) {
      throw shcore::Exception::argument_error(
          "removeInstances option can not be used if the metadata version is "
          "lower than " +
          metadata::current_version().get_base() + ". Metadata version is " +
          md_version.get_base());
    }
    for (auto value : *remove_instances_ref.get()) {
      // Check if seed instance is present on the list
      if (value.get_string() == instance_session_address)
        throw shcore::Exception::argument_error(
            "The current session instance cannot be used on the "
            "'removeInstances' list.");

      remove_instances_list.push_back(value.get_string());
    }
  }

  if (rejoin_instances_ref) {
    for (auto value : *rejoin_instances_ref.get()) {
      // Check if seed instance is present on the list
      if (value.get_string() == instance_session_address)
        throw shcore::Exception::argument_error(
            "The current session instance cannot be used on the "
            "'rejoinInstances' list.");
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
  // On this case a warning has been printed before, so a blank line is printed
  // for readability
  if (rebooting_old_version) console->print_info();

  // Getting the cluster from the metadata already complies with:
  // 1. Ensure that a Metadata Schema exists on the current session
  // instance.
  // 3. Ensure that the provided cluster identifier exists on the Metadata
  // Schema
  if (!cluster_name) {
    console->print_info(
        "Restoring the default cluster from complete outage...");
  } else {
    console->print_info(
        shcore::str_format("Restoring the cluster '%s' from complete outage...",
                           cluster_name->c_str()));
  }
  console->print_info();

  try {
    cluster = get_cluster(!cluster_name ? nullptr : cluster_name->c_str(),
                          metadata, target_instance);
  } catch (const shcore::Error &e) {
    // If the GR plugin is not installed, we can get this error.
    // In that case, we install the GR plugin and retry.
    if (e.code() == ER_UNKNOWN_SYSTEM_VARIABLE) {
      log_info("%s: installing GR plugin (%s)",
               target_instance->descr().c_str(), e.format().c_str());

      mysqlshdk::gr::install_group_replication_plugin(*target_instance,
                                                      nullptr);

      cluster = get_cluster(!cluster_name ? nullptr : cluster_name->c_str(),
                            metadata, target_instance);
    } else {
      throw;
    }
  }

  // Verify the status of the instances
  validate_instances_status_reboot_cluster(cluster, *target_instance, options);

  // Get the all the instances and their status
  // TODO(rennox): This is called inside of
  // validate_instances_status_reboot_cluster, review if both calls are needed
  std::vector<std::pair<std::string, std::string>> instances_status =
      get_replicaset_instances_status(cluster, options);

  if (cluster) {
    // Get the default replicaset
    default_replicaset = cluster->impl()->get_default_replicaset();

    // Get get instance address in metadata.
    std::string group_md_address = target_instance->get_canonical_address();

    // 2. Ensure that the current session instance exists on the Metadata
    // Schema
    if (!cluster->impl()->contains_instance_with_address(group_md_address))
      throw shcore::Exception::runtime_error(
          "The current session instance does not belong to the cluster: '" +
          cluster->impl()->get_name() + "'.");

    std::vector<std::string> non_reachable_rejoin_instances,
        non_reachable_instances;

    // get all non reachable instances
    for (auto &instance : instances_status) {
      if (!(instance.second.empty())) {
        non_reachable_instances.push_back(instance.first);
      }
    }
    // get all the list of non-reachable instances that were specified on
    // the rejoinInstances list. Sort non_reachable_instances vector because
    // set_intersection works on sorted collections The
    // rejoin_instances_list vector was already sorted above.
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

    // Ensure that all of the instances specified on the 'rejoinInstances'
    // list exist on the Metadata Schema and are valid
    if (rejoin_instances_ref) {
      for (const auto &value : rejoin_instances_list) {
        std::string md_address = value;

        try {
          auto instance_def = shcore::get_connection_options(value, false);

          // Get the instance metadata address (reported host).
          md_address = mysqlsh::dba::get_report_host_address(
              instance_def, current_session_options);
        } catch (const std::exception &e) {
          std::string error(e.what());
          throw shcore::Exception::argument_error(
              "Invalid value '" + value + "' for 'rejoinInstances': " + error);
        }

        if (!cluster->impl()->contains_instance_with_address(md_address))
          throw shcore::Exception::runtime_error(
              "The instance '" + value + "' does not belong to the cluster: '" +
              cluster->impl()->get_name() + "'.");
      }
    } else if (interactive || rebooting_old_version) {
      for (const auto &value : instances_status) {
        std::string instance_address = value.first;
        std::string instance_status = value.second;

        // if the status is not empty it means the connection failed
        // so we skip this instance
        if (!instance_status.empty()) {
          std::string msg = "The instance '" + instance_address +
                            "' is not reachable: '" + instance_status +
                            "'. Skipping rejoin to the Cluster.";
          log_warning("%s", msg.c_str());
          continue;
        }
        // If the instance is part of the remove_instances list we skip this
        // instance
        if (remove_instances_ref) {
          auto it = std::find_if(remove_instances_list.begin(),
                                 remove_instances_list.end(),
                                 [&instance_address](std::string val) {
                                   return val == instance_address;
                                 });
          if (it != remove_instances_list.end()) continue;
        }

        // When rebooting old version there's no option to modify, instances are
        // included in the cluster right away
        if (rebooting_old_version) {
          rejoin_instances_list.push_back(instance_address);
        } else {
          console->println("The instance '" + instance_address +
                           "' was part of the cluster configuration.");

          if (console->confirm("Would you like to rejoin it to the cluster?",
                               mysqlsh::Prompt_answer::NO) ==
              mysqlsh::Prompt_answer::YES) {
            rejoin_instances_list.push_back(instance_address);
          }
          console->println();
        }
      }
    }

    // Ensure that all of the instances specified on the 'removeInstances'
    // list exist on the Metadata Schema and are valid
    if (remove_instances_ref) {
      for (const auto &value : remove_instances_list) {
        std::string md_address = value;

        try {
          auto instance_def = shcore::get_connection_options(value, false);

          // Get the instance metadata address (reported host).
          md_address = mysqlsh::dba::get_report_host_address(
              instance_def, current_session_options);
        } catch (const std::exception &e) {
          std::string error(e.what());
          throw shcore::Exception::argument_error(
              "Invalid value '" + value + "' for 'removeInstances': " + error);
        }

        if (!cluster->impl()->contains_instance_with_address(md_address))
          throw shcore::Exception::runtime_error(
              "The instance '" + value + "' does not belong to the cluster: '" +
              cluster->impl()->get_name() + "'.");
      }
      // When rebooting old version there's no option to remove instances
    } else if (interactive && !rebooting_old_version) {
      for (const auto &value : instances_status) {
        std::string instance_address = value.first;
        std::string instance_status = value.second;

        // if the status is empty it means the connection succeeded
        // so we skip this instance
        if (instance_status.empty()) continue;

        // If the instance is part of the rejoin_instances list we skip this
        // instance
        if (rejoin_instances_ref) {
          auto it = std::find_if(rejoin_instances_list.begin(),
                                 rejoin_instances_list.end(),
                                 [&instance_address](std::string val) {
                                   return val == instance_address;
                                 });
          if (it != rejoin_instances_list.end()) continue;
        }
        console->println("Could not open a connection to '" + instance_address +
                         "': '" + instance_status + "'");

        if (console->confirm(
                "Would you like to remove it from the cluster's metadata?",
                mysqlsh::Prompt_answer::NO) == mysqlsh::Prompt_answer::YES)
          remove_instances_list.push_back(instance_address);
        console->println();
      }
    }
  } else {
    if (!cluster_name)
      throw shcore::Exception::logic_error("No default cluster is configured.");
    else
      throw shcore::Exception::logic_error(shcore::str_format(
          "The cluster '%s' is not configured.", cluster_name->c_str()));
  }

  // 4. Verify the status of all instances of the cluster:
  // 4.1 None of the instances can belong to a GR Group
  // 4.2 If any of the instances belongs to a GR group or is already managed
  // by the InnoDB Cluster, so include that information on the error message
  validate_instances_status_reboot_cluster(cluster, *target_instance, options);

  // 5. Verify which of the online instances has the GTID superset.
  // 5.1 Skip the verification on the list of instances to be removed:
  // "removeInstances" 5.2 If the current session instance doesn't have the
  // GTID superset, error out with that information and including on the
  // message the instance with the GTID superset
  validate_instances_gtid_reboot_cluster(cluster, options, *target_instance);

  // Check if trying to auto-rejoin and if so, stop GR to abort the
  // auto-rejoin
  ensure_not_auto_rejoining(target_instance.get());

  // 6. Set the current session instance as the seed instance of the Cluster
  {
    // Disable super_read_only mode if it is enabled.
    bool super_read_only =
        target_instance->get_sysvar_bool("super_read_only").get_safe();
    if (super_read_only) {
      console->print_info("Disabling super_read_only mode on instance '" +
                          target_instance->descr() + "'.");
      log_debug(
          "Disabling super_read_only mode on instance '%s' to run "
          "rebootClusterFromCompleteOutage.",
          target_instance->descr().c_str());
      target_instance->set_sysvar("super_read_only", false);
    }
    Group_replication_options gr_options(Group_replication_options::NONE);

    Clone_options clone_options(Clone_options::NONE);

    // The current 'group_replication_group_name' must be kept otherwise
    // if instances are rejoined later the operation may fail because
    // a new group_name started being used.
    // This must be done before rejoining the instances due to the fixes for
    // BUG #26159339: SHELL: ADMINAPI DOES NOT TAKE GROUP_NAME INTO ACCOUNT
    gr_options.group_name = cluster->impl()->get_group_name();

    // BUG#29265869: reboot cluster overrides some GR settings.
    // Read actual GR configurations to preserve them in the seed instance.
    gr_options.read_option_values(*target_instance);

    // BUG#27344040: REBOOT CLUSTER SHOULD NOT CREATE NEW USER
    // No new replication user should be created when rebooting a cluster
    // and existing recovery users should be reused. Therefore the boolean
    // to skip replication users is set to true and the respective
    // replication user credentials left empty.
    try {
      // Create the add_instance command and execute it.
      Add_instance op_add_instance(target_instance, *default_replicaset,
                                   gr_options, clone_options, {}, interactive,
                                   0, "", "", true);

      // Always execute finish when leaving scope.
      auto finally = shcore::on_leave_scope(
          [&op_add_instance]() { op_add_instance.finish(); });

      // Prepare the add_instance command execution (validations).
      op_add_instance.prepare();

      // Execute add_instance operations.
      op_add_instance.execute();

      console->print_info(target_instance->descr() + " was restored.");
    } catch (...) {
      // catch any exception that is thrown, restore super read-only-mode if
      // it was enabled and re-throw the caught exception.
      if (super_read_only) {
        log_debug(
            "Restoring super_read_only mode on instance '%s' to 'ON' since it "
            "was enabled before rebootClusterFromCompleteOutage operation "
            "started.",
            target_instance->descr().c_str());
        console->print_info(
            "Restoring super_read_only mode on instance '" +
            target_instance->descr() +
            "' since dba.<<<rebootClusterFromCompleteOutage>>>() operation "
            "failed.");
        target_instance->set_sysvar("super_read_only", true);
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
    mysqlshdk::utils::Version gr_protocol_version_to_upgrade;

    // After removing instance, the remove command must set
    // the GR protocol of the group to the lowest MySQL version on the
    // group.
    try {
      if (mysqlshdk::gr::is_protocol_upgrade_required(
              *target_instance, mysqlshdk::utils::nullable<std::string>(),
              &gr_protocol_version_to_upgrade)) {
        mysqlshdk::gr::set_group_protocol_version(
            *target_instance, gr_protocol_version_to_upgrade);
      }
    } catch (const shcore::Exception &error) {
      // The UDF may fail with MySQL Error 1123 if any of the members is
      // RECOVERING In such scenario, we must abort the upgrade protocol
      // version process and warn the user
      if (error.code() == ER_CANT_INITIALIZE_UDF) {
        console->print_note(
            "Unable to determine the Group Replication protocol version, while "
            "verifying if a protocol upgrade would be possible: " +
            std::string(error.what()) + ".");
      } else {
        throw;
      }
    }
  }

  if (interactive) {
    console->println("The cluster was successfully rebooted.");
    console->println();
  }

  return cluster;
}

Cluster_check_info Dba::check_preconditions(
    std::shared_ptr<Instance> target_instance,
    const std::string &function_name) const {
  try {
    return check_function_preconditions("Dba." + function_name,
                                        target_instance);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(function_name));
  return Cluster_check_info{};
}

Cluster_check_info Dba::check_preconditions(
    std::shared_ptr<MetadataStorage> metadata,
    const std::string &function_name) const {
  try {
    return check_function_preconditions("Dba." + function_name, metadata);
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
 * reachable, or if not reachable contains the connection failure error
 * message
 */
std::vector<std::pair<std::string, std::string>>
Dba::get_replicaset_instances_status(
    std::shared_ptr<Cluster> cluster,
    const shcore::Value::Map_type_ref &options) {
  std::vector<std::pair<std::string, std::string>> instances_status;
  std::string instance_address, conn_status;

  log_info("Checking instance status for cluster '%s'",
           cluster->impl()->get_name().c_str());

  std::vector<Instance_metadata> instances =
      cluster->impl()->get_default_replicaset()->get_instances();

  auto current_session_options =
      cluster->impl()->get_target_server()->get_connection_options();

  // Get the current session instance reported host address
  std::string active_session_md_address =
      cluster->impl()->get_target_server()->get_canonical_address();

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

    connection_options.set_login_options_from(current_session_options);

    try {
      log_info(
          "Opening a new session to the instance to determine its status: %s",
          instance_address.c_str());
      Instance::connect_raw(connection_options);
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
    const mysqlshdk::mysql::IInstance &instance,
    const std::string & /* gr_group_name */,
    const std::string &restore_function) {
  // TODO(alfredo) gr_group_name should receive the group_name as stored
  // in the metadata to validate if it matches the expected value
  // if the name does not match, an error should be thrown asking for an
  // option like update_mismatched_group_name to be set to ignore the
  // validation error and adopt the new group name

  GRInstanceType::Type type = get_gr_instance_type(instance);

  std::string member_session_address = instance.descr();

  switch (type) {
    case GRInstanceType::InnoDBCluster: {
      std::string err_msg = "The MySQL instance '" + member_session_address +
                            "' belongs to an InnoDB Cluster and is reachable.";
      // Check if quorum is lost to add additional instructions to users.
      if (!mysqlshdk::gr::has_quorum(instance, nullptr, nullptr)) {
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

    case GRInstanceType::AsyncReplicaSet:
      throw shcore::Exception::runtime_error(
          "The MySQL instance '" + member_session_address +
          "' belongs to an InnoDB ReplicaSet. ");

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
    const mysqlshdk::mysql::IInstance &target_instance,
    shcore::Value::Map_type_ref options) {
  // Validate the member we're connected to
  validate_instance_belongs_to_cluster(
      target_instance, "",
      get_member_name("forceQuorumUsingPartitionOf",
                      shcore::current_naming_style()));

  mysqlshdk::db::Connection_options member_connection_options =
      target_instance.get_connection_options();

  // Get the current session instance address
  if (options) {
    mysqlsh::set_user_from_map(&member_connection_options, options);
    mysqlsh::set_password_from_map(&member_connection_options, options);
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
    connection_options.set_login_options_from(member_connection_options);

    std::shared_ptr<Instance> instance;
    try {
      log_info("Opening a new session to the instance: %s",
               instance_address.c_str());
      instance = Instance::connect(connection_options);
    } catch (const std::exception &e) {
      throw shcore::Exception::runtime_error("Could not open connection to " +
                                             instance_address + "");
    }

    log_info("Checking state of instance '%s'", instance_address.c_str());
    validate_instance_belongs_to_cluster(
        *instance, "",
        get_member_name("forceQuorumUsingPartitionOf",
                        shcore::current_naming_style()));
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
        mysqlshdk::mysql::get_executed_gtid_set(target_instance);
    instance_gtids.push_back(info);
  }

  // Get the cluster instances
  std::vector<Instance_metadata> instances =
      cluster->impl()->get_default_replicaset()->get_instances();

  for (const auto &inst : instances) {
    if (inst.endpoint == instance_gtids[0].server) continue;

    auto connection_options =
        shcore::get_connection_options(inst.endpoint, false);
    connection_options.set_login_options_from(current_session_options);

    std::shared_ptr<Instance> instance;

    // Connect to the instance to obtain the GLOBAL.GTID_EXECUTED
    try {
      log_info("Opening a new session to the instance for gtid validations %s",
               inst.endpoint.c_str());
      instance = Instance::connect(connection_options);
    } catch (const std::exception &e) {
      log_warning("Could not open a connection to %s: %s",
                  inst.endpoint.c_str(), e.what());
      continue;
    }

    Instance_gtid_info info;
    info.server = inst.endpoint;
    info.gtid_executed = mysqlshdk::mysql::get_executed_gtid_set(*instance);
    instance_gtids.push_back(info);
  }

  std::vector<Instance_gtid_info> primary_candidates;

  try {
    primary_candidates =
        filter_primary_candidates(target_instance, instance_gtids);

    // Returned list should have at least 1 element
    assert(!primary_candidates.empty());
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_DATA_ERRANT_TRANSACTIONS) {
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

REGISTER_HELP_FUNCTION(upgradeMetadata, dba);
REGISTER_HELP_FUNCTION_TEXT(DBA_UPGRADEMETADATA, R"*(
Upgrades (or restores) the metadata to the version supported by the Shell.

@param options Optional dictionary with option for the operation.

This function will compare the version of the installed metadata schema
with the version of the metadata schema supported by this Shell. If the
installed metadata version is lower, an upgrade process will be started.

The options dictionary accepts the following attributes:

@li dryRun: boolean value used to enable a dry run of the upgrade process.
${OPT_INTERACTIVE}

If dryRun is used, the function will determine whether a metadata upgrade
or restore is required and inform the user without actually executing the
operation.

The interactive option can be used to explicitly enable or disable the
interactive prompts that help the user through te upgrade process. The default
value is equal to MySQL Shell wizard mode.

<b>The Upgrade Process</b>

When upgrading the metadata schema of clusters deployed by Shell versions before
8.0.19, a rolling upgrade of existing MySQL Router instances is required.
This process allows minimizing disruption to applications during the upgrade.

The rolling upgrade process must be performed in the following order:

-# Execute dba.<<<upgradeMetadata>>>() using the latest Shell version. The
upgrade function will stop if outdated Router instances are detected, at
which point you can stop the upgrade to resume later.
-# Upgrade MySQL Router to the latest version (same version number as the Shell)
-# Continue or re-execute dba.<<<upgradeMetadata>>>()


<b>Failed Upgrades</b>

If the installed metadata is not available because a previous call to this
function ended unexpectedly, this function will restore the metadata to the
state it was before the failed upgrade operation.

@throw RuntimeError in the following scenarios:
@li A global session is not available.
@li A global session is available but the target instance does not have the
metadata installed.
@li The installed metadata is a newer version than the one supported by the
Shell.
)*");
/**
 * $(DBA_UPGRADEMETADATA_BRIEF)
 *
 * $(DBA_UPGRADEMETADATA)
 */
#if DOXYGEN_JS
Undefined Dba::upgradeMetadata(Dictionary options) {}
#elif DOXYGEN_PY
None Dba::upgrade_metadata(dict options) {}
#endif
void Dba::upgrade_metadata(const shcore::Dictionary_t &options) {
  bool dry_run = false;
  bool interactive = current_shell_options()->get().wizards;

  if (options) {
    // Retrieves optional options if exists
    Unpack_options(options)
        .optional("dryRun", &dry_run)
        .optional("interactive", &interactive)
        .end();
  }

  auto instance = std::make_shared<Instance>(get_active_shell_session());
  auto state = check_function_preconditions("Dba.upgradeMetadata", instance);
  auto metadata = std::make_shared<MetadataStorage>(instance);

  // The pool is initialized with the metadata using the current session
  Instance_pool::Auth_options auth_opts;
  auth_opts.get(instance->get_connection_options());
  Scoped_instance_pool ipool(interactive, auth_opts);
  ipool->set_metadata(metadata);

  // If it happens we are on a RO instance, we update the metadata to make it
  // use a session to the PRIMARY of the IDC/RSET
  if (state.source_state == ManagedInstance::OnlineRO) {
    if (state.source_type == GRInstanceType::InnoDBCluster) {
      connect_to_target_group({}, &metadata, nullptr, true);
    } else {
      auto rs = get_replica_set(metadata, instance);

      rs->acquire_primary();
      auto finally = shcore::on_leave_scope([&rs]() { rs->release_primary(); });

      metadata = ipool->get_metadata();
    }
  }

  Upgrade_metadata op_upgrade(metadata, interactive, dry_run);

  auto finally =
      shcore::on_leave_scope([&op_upgrade]() { op_upgrade.finish(); });

  op_upgrade.prepare();

  op_upgrade.execute();
}

}  // namespace dba
}  // namespace mysqlsh
