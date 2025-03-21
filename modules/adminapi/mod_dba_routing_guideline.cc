/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/adminapi/mod_dba_routing_guideline.h"

#include "modules/adminapi/common/preconditions.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/debug.h"

DEBUG_OBJ_ENABLE(RoutingGuideline);

namespace mysqlsh::dba {

// Documentation of the Routing Guideline Class
REGISTER_HELP_CLASS_KW(
    RoutingGuideline, adminapi,
    (std::map<std::string, std::string>({{"FullType", "Routing Guideline"},
                                         {"Type", "RoutingGuideline"},
                                         {"type", "routingguideline"}})));

REGISTER_HELP_CLASS_TEXT(ROUTINGGUIDELINE, R"*(
Routing Guidelines operations

A RoutingGuideline can be used to control routing behavior of MySQL Router
through rules that define possible destination MySQL servers for incoming client
sessions.

Routing Guidelines consist of two main components: destination classes, and
routes.

<b>Destination classes</b>

Destination classes group MySQL server instances in the topology using
expressions. These expressions define the conditions under which servers are
added to a destination class. Each class can then be used to form a pool of
candidate instances for routing. A server can belong to multiple destination
classes simultaneously. However, only ONLINE instances in the topology are
considered when forming the candidate pool.

<b>Routes</b>

Routes, on the other hand, specify an expression that can match incoming
client sessions, and a list of destination classes to form a pool of candidate
MySQL servers to route them to.

Routes specify how incoming client sessions are classified and directed. When a
client connects to a MySQL Router port:

1. Routes are evaluated in order of appearance in the routing guideline (
top-bottom)

2. The first matching route determines the routing behavior for that
session. A list of candidate destinations is built.

3. Candidate destinations are organized in a tiered list:

@li Each tier contains one or more destination classes.
@li MySQL Router evaluates the availability of instances tier by tier.
@li If no instances in the current tier are available, the next tier is
evaluated.
@li If no instances in any tier are available, the client connection is
rejected.

<b>Topology monitoring</b>

MySQL Router continuously monitors the topology of its topology. If the
topology changes:

@li Servers are reclassified dynamically.
@li Existing client sessions are re-evaluated.
@li Sessions connected to servers no longer in their candidate pool are
disconnected.

<b>Example Use Cases for Client Classification</b>

Routing Guidelines can classify clients based on a variety of attributes, such
as:

@li Source IP Address: Direct clients from a specific subnet to a particular
group of servers.
@li MySQL Username: Route connections authenticated with a specific MySQL user
to designated servers.
@li Default Schema: Direct traffic based on the schema selected during
connection initialization.
@li Session Connection Attributes: Use attributes like client name, version, or
operating system (from `performance_schema.session_connect_attrs`).
@li Router Port: Route based on the MySQL Router port used by the client.

For the full list of session-related variables to define the matching rules for
routes , see \\? RoutingGuideline.<<<addRoute>>>()

<b>Example Use Cases for Destinations</b>

Destinations can be defined with broad or specific criteria, such as:

@li A specific IP address or hostname.
@li Server Version (e.g., MySQL 8.0.29 or higher).
@li Role (e.g., Primary, Secondary, or Read-Replica).
@li Custom tags (e.g., 'performance = "high"' or 'type = "compliance"').

For the full list of server-related variables to define the destination classes
matching rules, see \\? RoutingGuideline.<<<addDestination>>>()

<b>Practical Example</b>

1. Client classification:

@li '$.router.port.ro': Clients connecting to Router's RO port are directed
to read-only servers.
@li '$.session.user': Clients authenticated as "admin_user" are routed to
servers optimized for administrative operations.

2. Destination Classes:

@li Write traffic: A class for (Primary) servers that handle write traffic.
@li Read-only traffic: A class for (Secondary) servers that handle read-only
operations.
@li Compliance: A class for servers in "compliance mode", ensuring adherence to
regulatory requirements.
@li Target port: A route that matches a specific "$.session.targetPort" and
directs traffic to target destinations, prioritized in tiers.

<b>Adding Destinations and Routes</b>

Destination classes can be added with the <<<addDestination>>>() method.

For example: rg.addDestination("ReadOnlyServers", "$.server.memberRole =
SECONDARY")

Routes can be added with the <<<addRoute>>>() method.

For example: rg.addRoute('priority_reads', '$.session.user = "myapp"',
['round-robin(Primary)'])
)*");

shcore::Value RoutingGuideline::get_member(const std::string &prop) const {
  assert_valid(prop);

  if (prop == "name") return shcore::Value(impl()->get_name());

  return shcore::Cpp_object_bridge::get_member(prop);
}

std::string &RoutingGuideline::append_descr(
    std::string &s_out, [[maybe_unused]] int indent,
    [[maybe_unused]] int quote_strings) const {
  s_out.append("<")
      .append(class_name())
      .append(+":")
      .append(impl()->get_name())
      .append(">");
  return s_out;
}

void RoutingGuideline::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();
  dumper.append_string("class", class_name());
  dumper.append_string("name", impl()->get_name());
  dumper.end_object();
}

bool RoutingGuideline::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

RoutingGuideline::RoutingGuideline(
    const std::shared_ptr<Routing_guideline_impl> &rg)
    : m_impl(rg) {
  DEBUG_OBJ_ALLOC2(RoutingGuideline, [](void *ptr) {
    return "refs:" + std::to_string(reinterpret_cast<RoutingGuideline *>(ptr)
                                        ->shared_from_this()
                                        .use_count());
  });

  init();
}

RoutingGuideline::~RoutingGuideline() { DEBUG_OBJ_DEALLOC(RoutingGuideline); }

void RoutingGuideline::init() {
  add_property("name", "getName");

  expose("asJson", &RoutingGuideline::as_json);
  expose("destinations", &RoutingGuideline::destinations);
  expose("routes", &RoutingGuideline::routes);
  expose("addDestination", &RoutingGuideline::add_destination, "name", "match",
         "?options");
  expose("addRoute", &RoutingGuideline::add_route, "name", "match",
         "destinations", "?options");
  expose("removeDestination", &RoutingGuideline::remove_destination, "name");
  expose("removeRoute", &RoutingGuideline::remove_route, "name");
  expose("show", &RoutingGuideline::show, "?options");
  expose("rename", &RoutingGuideline::rename, "guideline", "name");
  expose("setDestinationOption", &RoutingGuideline::set_destination_option,
         "destinationName", "option", "value");
  expose("setRouteOption", &RoutingGuideline::set_route_option, "routeName",
         "option", "value");
  expose("copy", &RoutingGuideline::copy, "name");
  expose("export", &RoutingGuideline::export_to_file, "filePath");
}

void RoutingGuideline::assert_valid(const std::string &) const {
  if (!impl()->owner()->check_valid()) {
    throw shcore::Exception::runtime_error(
        "The owner " +
        to_display_string(impl()->owner()->get_type(),
                          Display_form::API_CLASS) +
        " object is disconnected.");
  }

  impl()->validate_or_throw_if_invalid();
}

// Documentation of the getName function
REGISTER_HELP_FUNCTION(getName, RoutingGuideline);
REGISTER_HELP_PROPERTY(name, RoutingGuideline);
REGISTER_HELP(ROUTINGGUIDELINE_NAME_BRIEF, "${ROUTINGGUIDELINE_GETNAME_BRIEF}");
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_GETNAME, R"*(
Returns the name of the Routing Guideline.

@returns name of the RoutingGuideline.
)*");

/**
 * $(ROUTINGGUIDELINE_GETNAME_BRIEF)
 *
 * $(ROUTINGGUIDELINE_GETNAME)
 */
#if DOXYGEN_JS
String RoutingGuideline::getName() {}
#elif DOXYGEN_PY
str RoutingGuideline::get_name() {}
#endif

REGISTER_HELP_FUNCTION(asJson, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_ASJSON, R"*(
Returns the Routing Guideline as a JSON document.

@returns A dictionary representing the Routing Guideline.

The returned Routing Guideline can be modified and fed to parse().
)*");

/**
 * $(ROUTINGGUIDELINE_ASJSON_BRIEF)
 *
 * $(ROUTINGGUIDELINE_ASJSON)
 */
#if DOXYGEN_JS
Dictionary RoutingGuideline::asJson() {}
#elif DOXYGEN_PY
dict RoutingGuideline::as_json() {}
#endif
shcore::Dictionary_t RoutingGuideline::as_json() const {
  assert_valid("asJson");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("asJson")
                .primary_not_required()
                .build();

        impl()->owner()->check_preconditions(conds);

        auto json = shcore::Value(impl()->as_json()).json();

        return shcore::Value::parse(json).as_map();
      },
      false);
}

REGISTER_HELP_FUNCTION(destinations, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_DESTINATIONS, R"*(
List destination classes defined for the Routing Guideline.

@returns ShellResult object with destination classes defined for the Guideline.
)*");

/**
 * $(ROUTINGGUIDELINE_DESTINATIONS_BRIEF)
 *
 * $(ROUTINGGUIDELINE_DESTINATIONS)
 */
#if DOXYGEN_JS
ShellResult RoutingGuideline::destinations() {}
#elif DOXYGEN_PY
ShellResult RoutingGuideline::destinations() {}
#endif
std::shared_ptr<ShellResult> RoutingGuideline::destinations() const {
  assert_valid("destinations");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("destinations")
                .primary_not_required()
                .build();

        impl()->owner()->check_preconditions(conds);

        return std::make_shared<ShellResult>(impl()->get_destinations());
      },
      false);
}

REGISTER_HELP_FUNCTION(routes, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_ROUTES, R"*(
List routes defined for the Routing Guideline.

@returns ShellResult object with routes classes defined for the Guideline.
)*");

/**
 * $(ROUTINGGUIDELINE_ROUTES_BRIEF)
 *
 * $(ROUTINGGUIDELINE_ROUTES)
 */
#if DOXYGEN_JS
ShellResult RoutingGuideline::routes() {}
#elif DOXYGEN_PY
ShellResult RoutingGuideline::routes() {}
#endif
std::shared_ptr<ShellResult> RoutingGuideline::routes() const {
  assert_valid("routes");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("routes")
                .primary_not_required()
                .build();

        impl()->owner()->check_preconditions(conds);

        return std::make_shared<ShellResult>(impl()->get_routes());
      },
      false);
}

REGISTER_HELP_FUNCTION(addDestination, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_ADDDESTINATION, R"*(
Adds a destination class that groups MySQL instances in the Routing Guideline.

@param name Name for the destination class to be added.
@param match An expression string for matching MySQL server instances.
@param options Optional dictionary with additional options.

@returns Nothing.

A destination class specifies an expression that groups together MySQL server
instances from the topology. They can be used to define candidate destinations
for a route to forward client sessions to. Destination classes group instances
based on matching attributes, and each instance can belong to one or more
destination classes.

For an overview on Routing Guidelines, see \\? RoutingGuideline.

<b>Expression Syntax</b>

The matching expression can contain variables related to the MySQL server
instances ('$.server.*') and the Router itself ('$.router.*') and must evaluate
to either true or false. Each variable represents a specific attribute of
the server or the router, allowing fine-grained control over grouping.

The following server-related variables are available:

@li $.server.label: "<hostname>:<port>"
@li $.server.address: Public address of the instance (@@report_host or
'@@hostname')
@li $.server.port: MySQL classic protocol port number
@li $.server.uuid: The `@@server_uuid` of the instance
@li $.server.version: Version number of the instance (MMmmpp) (e.g. 80400)
@li $.server.memberRole: PRIMARY, SECONDARY, or READ_REPLICA (if the instance
is part of an InnoDB Cluster)
@li $.server.tags: A key-value object containing user-defined tags stored in
the topology's metadata for that instance.
@li $.server.clusterName: Name of the Cluster the instance is part of.
@li $.server.clusterSetName: Name of the ClusterSet the instance belongs to.
@li $.server.clusterRole: Role of the Cluster the instance is part of, if it's
in a ClusterSet. One of PRIMARY, REPLICA, or UNDEFINED.
@li $.server.isClusterInvalidated: Indicates if the Cluster the instance
belongs to is invalidated.

The following router-related variables are available:

@li $.router.port.rw: Listening port for classic protocol read-write
connections.
@li $.router.port.ro: Listening port for classic protocol read-only
connections.
@li $.router.port.rw_split: Listening port for classic protocol read-write
splitting connections.
@li $.router.localCluster: Name of the cluster configured as local to the
Router.
@li $.router.hostname: Hostname where the Router is running.
@li $.router.bindAddress: IP address on which the Router is listening.
@li $.router.tags: A key-value object containing user-defined tags stored in
the topology's metadata for that Router instance.
@li $.router.routeName: Name of the Routing plugin used by the Router.
@li $.router.name: Name of the Router.

<b>Examples</b>

rg.add_destination("SecondaryInstances", "$.server.memberRole = 'SECONDARY'")

rg.add_destination("US_Instances", "$.server.address in ['us-east-1.example.com', 'us-west-2.example.com']")

rg.add_destination("MarkedRouters", "$.router.tags.mark = 'special'")
)*");

/**
 * $(ROUTINGGUIDELINE_ADDDESTINATION_BRIEF)
 *
 * $(ROUTINGGUIDELINE_ADDDESTINATION)
 */
#if DOXYGEN_JS
Undefined RoutingGuideline::addDestination(String name, String match,
                                           Dictionary options) {}
#elif DOXYGEN_PY
None RoutingGuideline::add_destination(str name, str match, dict options) {}
#endif
void RoutingGuideline::add_destination(const std::string &name,
                                       const std::string &match,
                                       const Add_destination_options &options) {
  assert_valid("addDestination");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("addDestination")
                .quorum_state(ReplicationQuorum::States::Normal)
                .primary_required()
                .cluster_global_status(Cluster_global_status::OK)
                .build();

        impl()->owner()->check_preconditions(conds);

        impl()->add_destination(name, match, options.dry_run);
      },
      false);
}

REGISTER_HELP_FUNCTION(addRoute, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_ADDROUTE, R"*(
Adds a new route that defines how client sessions matching specific criteria
are routed to defined destinations.

@param name The name of the route to be added.
@param match An expression string for matching incoming client sessions.
@param destinations An array of destination selectors in the form of `strategy(
destination, ...)`.
@param options An optional dictionary with additional options.

@returns Nothing.

This command adds a route that evaluates a given expression against attributes
of incoming client sessions. If the expression matches, the client session is
routed to one of the specified destination instances, based on the defined
routing strategies.

Individual routes can be selectively enabled or disabled using the 'enabled'
flag. If disabled, the route will not be considered during routing decisions.

For an overview on Routing Guidelines, see \\? RoutingGuideline.

<b>Source Expression</b>

The 'match' parameter specifies the client session matching rule. It can
contain variables related to the incoming client session ('$.session.*') and
the router itself ('$.router.*'). The expression must evaluate to either true
or false.

The following session-related variables are available:

@li $.session.targetIP: IP address of the Router the client is connected to.
@li $.session.targetPort: Router port the client is connected to.
@li $.session.sourceIP: IP address the client is connecting from.
@li $.session.user: Username the client is authenticated as.
@li $.session.connectAttrs: Client connection attributes.
@li $.session.schema: Default schema the client has selected.
@li $.session.randomValue: Random value in a range 0.0 <= x < 1.0.

The following router-related variables are available:

@li $.router.port.rw: Listening port for classic protocol read-write
connections.
@li $.router.port.ro: Listening port for classic protocol read-only
connections.
@li $.router.port.rw_split: Listening port for classic protocol Read-Write
splitting connections.
@li $.router.localCluster: Name of the cluster configured as local to the
Router.
@li $.router.hostname: Hostname where Router is running.
@li $.router.bindAddress: IP address on which the Router is listening.
@li $.router.tags: A key-value object containing user-defined tags stored in
the topology's metadata for that Router instance.
@li $.router.routeName: Name of the Routing plugin used by the Router.
@li $.router.name: Name of the Router.

The '$.session.connectAttrs' variable refers to attributes sent by the client
during the connection handshake. These attributes are not simple scalars but
key-value pairs that describe details about the client environment. You can use
these attributes to match specific criteria for routing.

For more information on the available Client connection attributes see
https://dev.mysql.com/doc/refman/en/performance-schema-connection-attribute-tables.html#performance-schema-connection-attributes-available

<b>Destinations</b>

The 'destinations' parameter specifies the list of candidate destination
classes for routing, using one or more routing strategies. A destination list
must be defined in the format '[strategy(destination, ...), ...]', where:

'strategy' is one of the supported routing strategies:

@li first-available: Selects the first available instance in the order specified.
@li round-robin: Distributes connections evenly among available instances.

'destination' is the name of the destination class defined in the Routing
Guideline. Each class is a group of instances matching a specific set of rules.

Destinations are organized in tiers, and MySQL Router will evaluate each tier
in order of priority. If no servers are available in the first tier, it will
proceed to the next until it finds a suitable destination or exhausts the list.

To add a new destination class, use the <<<addDestination>>> method.

<b>Options</b>

The following options can be specified when adding a route:

@li enabled (boolean): Indicates whether the route is active. Set to 'true'
by default.
@li connectionSharingAllowed (boolean): Specifies if the route allows
connection sharing. Set to 'true' by default.
@li dryRun (boolean): If set to 'true', validates the route without applying
it. Disabled by default.
@li 'order' (uinteger): Specifies the position of a route within the Routing
Guideline (lower values indicate higher priority).

<b>Examples</b>

guideline.addRoute('priority_reads', '$.session.user = "myapp"', ['round-robin(Primary)'])

guideline.addRoute('local_reads', '$.session.sourceIP = "192.168.1.10"', ['round-robin(Secondary)'], {enabled: true})

guideline.addRoute('backup_client', '$.session.connectAttrs.program_name =
"mysqldump"', ['first-available(backup_server)'],{ connectionSharingAllowed: false, enabled: true }

guideline.addRoute('tagged', '$.router.tags.marker =
"special"', ['round-robin(Special)'], { connectionSharingAllowed:
true, enabled: true }
);

)*");

/**
 * $(ROUTINGGUIDELINE_ADDROUTE_BRIEF)
 *
 * $(ROUTINGGUIDELINE_ADDROUTE)
 */
#if DOXYGEN_JS
Undefined RoutingGuideline::addRoute(String name, String match,
                                     Array destinations, Dictionary options) {}
#elif DOXYGEN_PY
None RoutingGuideline::add_route(str name, str match, list destinations,
                                 dict options) {}
#endif
void RoutingGuideline::add_route(const std::string &name,
                                 const std::string &source_matcher,
                                 const shcore::Array_t &destinations,
                                 const Add_route_options &options) {
  assert_valid("addRoute");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("destinations")
                .quorum_state(ReplicationQuorum::States::Normal)
                .primary_required()
                .cluster_global_status(Cluster_global_status::OK)
                .build();

        impl()->owner()->check_preconditions(conds);

        impl()->add_route(name, source_matcher, destinations, options.enabled,
                          options.connection_sharing_allowed, options.order,
                          options.dry_run);
      },
      false);
}

REGISTER_HELP_FUNCTION(removeDestination, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_REMOVEDESTINATION, R"*(
Remove a destination class from the Routing Guideline.

@param name Name of the destination class

@returns Nothing
)*");

/**
 * $(ROUTINGGUIDELINE_REMOVEDESTINATION_BRIEF)
 *
 * $(ROUTINGGUIDELINE_REMOVEDESTINATION)
 */
#if DOXYGEN_JS
Undefined RoutingGuideline::removeDestination(String name) {}
#elif DOXYGEN_PY
None RoutingGuideline::remove_destination(str name) {}
#endif
void RoutingGuideline::remove_destination(const std::string &name) {
  assert_valid("removeDestination");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("destinations")
                .quorum_state(ReplicationQuorum::States::Normal)
                .primary_required()
                .cluster_global_status(Cluster_global_status::OK)
                .build();

        impl()->owner()->check_preconditions(conds);

        impl()->remove_destination(name);
      },
      false);
}

REGISTER_HELP_FUNCTION(removeRoute, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_REMOVEROUTE, R"*(
Remove a route from the Routing Guideline.

@param name Name of the route

@returns Nothing
)*");

/**
 * $(ROUTINGGUIDELINE_REMOVEROUTE_BRIEF)
 *
 * $(ROUTINGGUIDELINE_REMOVEROUTE)
 */
#if DOXYGEN_JS
Undefined RoutingGuideline::removeRoute(String name) {}
#elif DOXYGEN_PY
None RoutingGuideline::remove_route(str name) {}
#endif
void RoutingGuideline::remove_route(const std::string &name) {
  assert_valid("removeRoute");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("destinations")
                .quorum_state(ReplicationQuorum::States::Normal)
                .primary_required()
                .cluster_global_status(Cluster_global_status::OK)
                .build();

        impl()->owner()->check_preconditions(conds);

        impl()->remove_route(name);
      },
      false);
}

REGISTER_HELP_FUNCTION(show, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_SHOW, R"*(
Displays a comprehensive summary of the Routing Guideline.

@param options Optional dictionary with additional options.

@returns Nothing

This function describes the Routing Guideline by evaluating expressions and
listing all destinations and routes, including the information about the
servers of the target topology that match the destinations and routes.
It also identifies all destinations not referenced by any route.

Destinations that rely on expressions involving the
target router port or address (e.g., '$.router.port' or '$.router.bindAddress')
may produce results that differ from actual behavior in MySQL Router. Since
this function evaluates the guideline outside the context of the router,
these expressions are not matched against live router runtime data and may
not reflect the router's operational reality. On that case, the option 'router'
must be set to specify the Router instance that should be used for evaluation
of the guideline.

The options dictionary may contain the following attributes:

@li router: Identifier of the Router instance to be used for evaluation.
)*");

/**
 * $(ROUTINGGUIDELINE_SHOW_BRIEF)
 *
 * $(ROUTINGGUIDELINE_SHOW)
 */
#if DOXYGEN_JS
void RoutingGuideline::show(Dictionary options) {}
#elif DOXYGEN_PY
void RoutingGuideline::show(dict options) {}
#endif
void RoutingGuideline::show(const Show_options &options) const {
  assert_valid("show");

  return execute_with_pool(
      [&]() {
        auto conds = Command_conditions::Builder::gen_routing_guideline("show")
                         .primary_not_required()
                         .build();

        impl()->owner()->check_preconditions(conds);

        impl()->show(options);
      },
      false);
}

REGISTER_HELP_FUNCTION(rename, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_RENAME, R"*(
Renames the target Routing Guideline.

@param name The new name to assign to the Routing Guideline.

@returns Nothing.

This command renames the target Routing Guideline.
)*");

/**
 * $(ROUTINGGUIDELINE_RENAME_BRIEF)
 *
 * $(ROUTINGGUIDELINE_RENAME)
 */
#if DOXYGEN_JS
void RoutingGuideline::rename(String name) {}
#elif DOXYGEN_PY
void RoutingGuideline::rename(str name) {}
#endif
void RoutingGuideline::rename(const std::string &name) {
  assert_valid("rename");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("rename")
                .quorum_state(ReplicationQuorum::States::Normal)
                .primary_required()
                .cluster_global_status(Cluster_global_status::OK)
                .build();

        impl()->owner()->check_preconditions(conds);

        impl()->rename(name);
      },
      false);
}

REGISTER_HELP_FUNCTION(setDestinationOption, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_SETDESTINATIONOPTION, R"*(
Updates a specific option for a named destination in the Routing Guideline.

@param destinationName The name of the destination whose option needs to be
updated.
@param option The option to update for the specified destination.
@param value The new value for the specified option.

@returns Nothing.

This command allows modifying the properties of a destination within a Routing
Guideline. It is used to change the matching rule for identifying MySQL server
instances that belong to this destination class.

The following destination options are supported:

@li match (string): The matching rule used to identify servers for this destination class.

<b>Examples</b>

rg.<<<setDestinationOption>>>("EU_Regions", "match", "$.server.address in
['eu-central-1.example.com', 'eu-west-1.example.com']");

rg.<<<setDestinationOption>>>("Primary_Instances", "match",
"$.server.memberRole = 'PRIMARY'");

For more information on destinations, see \\? RoutingGuideline.<<<addDestination>>>.

For more information about Routing Guidelines, see \\? RoutingGuideline.
)*");

/**
 * $(ROUTINGGUIDELINE_SETDESTINATIONOPTION_BRIEF)
 *
 * $(ROUTINGGUIDELINE_SETDESTINATIONOPTION)
 */
#if DOXYGEN_JS
Undefined RoutingGuideline::setDestinationOption(String destinationName,
                                                 String option, String value) {}
#elif DOXYGEN_PY
None RoutingGuideline::set_destination_option(str destinationName, str option,
                                              str value) {}
#endif
void RoutingGuideline::set_destination_option(
    const std::string &destination_name, const std::string &option,
    const shcore::Value &value) {
  assert_valid("setDestinationOption");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("destinations")
                .quorum_state(ReplicationQuorum::States::Normal)
                .primary_required()
                .cluster_global_status(Cluster_global_status::OK)
                .build();

        impl()->owner()->check_preconditions(conds);

        impl()->set_destination_option(destination_name, option, value);
      },
      false);
}

REGISTER_HELP_FUNCTION(setRouteOption, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_SETROUTEOPTION, R"*(
Updates a specific option for a named route in the Routing Guideline.

@param routeName The name of the route whose option needs to be updated.
@param option The option to update for the specified route.
@param value The new value for the specified option.

@returns Nothing.

This command allows modifying the properties of a route within a Routing
Guideline. It is used to change the matching rule for client sessions, update
the destinations, enable or disable the route, or change its priority.

The following route options are supported:

@li match (string): The client connection matching rule.
@li destinations (string): A list of destinations using routing strategies in
the format `strategy(destination1, destination2, ...)`, ordered by priority,
highest to lowest
@li enabled (boolean): Set to true to enable the route, or false to disable it.
@li connectionSharingAllowed (boolean): Set to true to enable connection
sharing, or false to disable it.
@li order (uinteger): Specifies the position of a route within the Routing
Guideline (lower values indicate higher priority).

<b>Examples</b>

rg.<<<setRouteOption>>>("read_traffic", "match", "$.session.user = 'readonly_user'");

rg.<<<setRouteOption>>>("write_traffic", "enabled", true);

For more information on routes definition, see \\? RoutingGuideline.<<<addRoute>>>.

For more information about Routing Guidelines, see \\? RoutingGuideline.
)*");

/**
 * $(ROUTINGGUIDELINE_SETROUTEOPTION_BRIEF)
 *
 * $(ROUTINGGUIDELINE_SETROUTEOPTION)
 */
#if DOXYGEN_JS
Undefined RoutingGuideline::setRouteOption(String routeName, String option,
                                           String value) {}
#elif DOXYGEN_PY
None RoutingGuideline::set_route_option(str routeName, str option, str value) {}
#endif
void RoutingGuideline::set_route_option(const std::string &route_name,
                                        const std::string &option,
                                        const shcore::Value &value) {
  assert_valid("setRouteOption");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("destinations")
                .quorum_state(ReplicationQuorum::States::Normal)
                .primary_required()
                .cluster_global_status(Cluster_global_status::OK)
                .build();

        impl()->owner()->check_preconditions(conds);

        impl()->set_route_option(route_name, option, value);
      },
      false);
}

REGISTER_HELP_FUNCTION(copy, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_COPY, R"*(
Creates a full copy of the target Routing Guideline with a new name.

@param name The name for the new Routing Guideline.

@returns A Routing Guideline object.

This command creates a duplicate of the target Routing Guideline,
including all its routes and destination classes. The new guideline must have
a unique name within the topology, and the name must follow the same naming
rules defined for Routing Guidelines. This command is useful for quickly
creating new guidelines based on existing ones, without manually duplicating
all the definitions.

<b>Example</b>

rg_copy = rg.copy("newGuideline");

The example above creates a new Routing Guideline named 'newGuideline' with the
same configuration as the original guideline.

For more information about Routing Guidelines, see \\? RoutingGuideline.
)*");

/**
 * $(ROUTINGGUIDELINE_COPY_BRIEF)
 *
 * $(ROUTINGGUIDELINE_COPY)
 */
#if DOXYGEN_JS
RoutingGuideline RoutingGuideline::copy(String name) {}
#elif DOXYGEN_PY
RoutingGuideline RoutingGuideline::copy(str name) {}
#endif
std::shared_ptr<RoutingGuideline> RoutingGuideline::copy(
    const std::string &name) {
  assert_valid("copy");

  return execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("destinations")
                .quorum_state(ReplicationQuorum::States::Normal)
                .primary_required()
                .cluster_global_status(Cluster_global_status::OK)
                .build();

        impl()->owner()->check_preconditions(conds);

        return std::make_shared<RoutingGuideline>(impl()->copy(name));
      },
      false);
}

REGISTER_HELP_FUNCTION(export, RoutingGuideline);
REGISTER_HELP_FUNCTION_TEXT(ROUTINGGUIDELINE_EXPORT, R"*(
Exports the Routing Guideline to a file.

@param file The file path where the Routing Guideline will be saved.

@returns Nothing.

This command exports a complete copy of the Routing Guideline to a file in JSON
format. The output file can then be used for backup, sharing, or applying the
Routing Guideline to another topology.

For more information on Routing Guidelines, see \? RoutingGuideline.
)*");

/**
 * $(ROUTINGGUIDELINE_EXPORT_BRIEF)
 *
 * $(ROUTINGGUIDELINE_EXPORT)
 */
#if DOXYGEN_JS
Undefined RoutingGuideline::export(String file) {}
#elif DOXYGEN_PY
None RoutingGuideline::export(str file) {}
#endif
void RoutingGuideline::export_to_file(const std::string &file_path) {
  assert_valid("export");

  execute_with_pool(
      [&]() {
        auto conds =
            Command_conditions::Builder::gen_routing_guideline("export")
                .primary_not_required()
                .build();

        impl()->owner()->check_preconditions(conds);

        impl()->export_to_file(file_path);
      },
      false);
}

}  // namespace mysqlsh::dba
