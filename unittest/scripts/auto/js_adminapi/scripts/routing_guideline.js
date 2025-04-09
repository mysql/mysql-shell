function get_route(rg, name) {
    var routes = rg.asJson()["routes"];
    for (var r in routes) {
        r = routes[r];
        if (r["name"] == name)
            return r;
    }
    return null;
}

//@<> INCLUDE routing_guideline_tests.inc

//@<> Setup + Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var cluster = scene.cluster

//@<> createRoutingGuideline() - Bad options (should fail)
STDCHECK_ARGTYPES(cluster.createRoutingGuideline, 1, ["abc"], [{}], [{}]);

EXPECT_THROWS(function(){ cluster.createRoutingGuideline("xx/ -");}, "Routing Guideline name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (xx/ -)");
EXPECT_THROWS(function(){ cluster.createRoutingGuideline("1xx");}, "Routing Guideline name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (1xx)");
EXPECT_THROWS(function(){ cluster.createRoutingGuideline("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");}, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx: The Routing Guideline name can not be greater than 63 characters.");

EXPECT_THROWS(function(){ cluster.createRoutingGuideline("test", null, {force: true});}, "The 'json' option is required when 'force' is enabled");

//@<> createRoutingGuideline() - OK
var rg;
EXPECT_NO_THROWS(function() { rg = cluster.createRoutingGuideline("default_cluster_rg");} );

EXPECT_OUTPUT_CONTAINS(`NOTE: Routing guideline 'default_cluster_rg' won't be made active by default. To activate this guideline, please use .setRoutingOption() with the option 'guideline'.`);
EXPECT_OUTPUT_CONTAINS(`Routing Guideline 'default_cluster_rg' successfully created.`);

EXPECT_THROWS(function(){ cluster.createRoutingGuideline("default_cluster_rg");}, "A Routing Guideline with the name 'default_cluster_rg' already exists");

// Verify that the Guideline was NOT set as active in the Metadata
var cid = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

var rgid = session.runSql("select guideline_id from mysql_innodb_cluster_metadata.routing_guidelines").fetchOne()[0];

var rgid_router_options = session.runSql("select json_unquote(router_options->'$.guideline') from mysql_innodb_cluster_metadata.clusters where cluster_id=?", [cid]).fetchOne()[0];

EXPECT_EQ(null, rgid_router_options);

var router_options = cluster.routerOptions();
EXPECT_EQ(false, router_options["configuration"]["routing_rules"].hasOwnProperty("guideline"));

// Manually set the guideline as the default
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = JSON_SET(router_options, '$.guideline', ?) WHERE cluster_id = ?", [rgid, cid]);

//@<> createRoutingGuideline() - another one
EXPECT_NO_THROWS(function() { cluster.createRoutingGuideline("anotherguideline");} );

EXPECT_OUTPUT_CONTAINS(`NOTE: Routing Guideline 'default_cluster_rg' is the guideline currently active in the 'Cluster'.`);
EXPECT_OUTPUT_CONTAINS(`Routing Guideline 'anotherguideline' successfully created.`);

// Verify the new one is not the active one
var router_options = cluster.routerOptions();
EXPECT_EQ("default_cluster_rg", router_options["configuration"]["routing_rules"]["guideline"]);

// Run common tests for <RoutingGuideline>
const expected_default_cluster_guideline_show_output = `
Routing Guideline: 'default_cluster_rg'
Cluster: 'cluster'

Routes
------
  - rw
    + Match: "$.session.targetPort = $.router.port.rw"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - ro
    + Match: "$.session.targetPort = $.router.port.ro"
    + Destinations:
      * None (Secondary)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

Destination Classes
-------------------
  - Primary:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - Secondary:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - ReadReplica:
    + Match: "$.server.memberRole = READ_REPLICA"
    + Instances:
      * None

Unreferenced servers
--------------------
  - None
`;

COMMON_RG_TESTS(rg, "default_cluster_rg", expected_default_cluster_guideline_destinations, expected_default_cluster_guideline_destinations_match, expected_default_cluster_guideline_routes, expected_default_cluster_guideline_routes_match, expected_default_cluster_guideline_routes_destinations, default_cluster_guideline, expected_default_cluster_guideline_show_output);

//@<> createRoutingGuideline() - using the 'json' option + dryRun + invalid json
const invalid_guideline =
{
    "destinations": [
        {
            "match": "$.server.memberRole = INVALID",
            "name": "ReadReplica"
        }
    ],
    "name": "invalid",
    "routes": [
    ],
    "version": "1.1"
}

EXPECT_THROWS(function(){ cluster.createRoutingGuideline("fromjson", invalid_guideline);}, "Invalid guideline document: Errors while parsing routing guidelines document:\n- destinations[0].match: type error, = operator, the type of left operand does not match right, expected ROLE but got STRING in '$.server.memberRole = INVALID'\n- destinations[0]: 'match' field not defined\n- routes: field is expected to be a non empty array\n- no destination classes defined by the document\n- no routes defined by the document");

const invalid_guideline_invalid_names =
{
    "destinations": [
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadR eplica"
        }
    ],
    "name": "foo",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "ReadR eplica"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "read replica"
        }
    ],
    "version": "1.1"
}

EXPECT_THROWS(function(){ cluster.createRoutingGuideline("fromjson", invalid_guideline_invalid_names);}, "Invalid guideline document: ArgumentError: Destination name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (ReadR eplica)");

const invalid_guideline_invalid_names_more =
{
    "destinations": [
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadReplica"
        }
    ],
    "name": "foo",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "ReadReplica"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "read replica"
        }
    ],
    "version": "1.1"
}

EXPECT_THROWS(function(){ cluster.createRoutingGuideline("fromjson", invalid_guideline_invalid_names_more);}, "Invalid guideline document: ArgumentError: Route name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (read replica)");

//@<> createRoutingGuideline() - using the 'json' option + valid json
EXPECT_NO_THROWS(function() { rg_from_json = cluster.createRoutingGuideline("fromjson", default_cluster_guideline);} );

default_cluster_guideline["name"] = "fromjson";

const expected_show_output_from_json = `
Routing Guideline: 'fromjson'
Cluster: 'cluster'

Routes
------
  - rw
    + Match: "$.session.targetPort = $.router.port.rw"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - ro
    + Match: "$.session.targetPort = $.router.port.ro"
    + Destinations:
      * None (Secondary)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

Destination Classes
-------------------
  - Primary:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - Secondary:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - ReadReplica:
    + Match: "$.server.memberRole = READ_REPLICA"
    + Instances:
      * None
`;

COMMON_RG_TESTS(rg_from_json, "fromjson", expected_default_cluster_guideline_destinations, expected_default_cluster_guideline_destinations_match, expected_default_cluster_guideline_routes, expected_default_cluster_guideline_routes_match, expected_default_cluster_guideline_routes_destinations, default_cluster_guideline, expected_show_output_from_json);

//@<> createRoutingGuideline() - using the 'json' option + valid json + duplicate name
EXPECT_THROWS(function(){ cluster.createRoutingGuideline("fromjson", default_cluster_guideline); }, "A Routing Guideline with the name 'fromjson' already exists");

//@<> createRoutingGuideline() - using the 'json' option + valid json + duplicate name + force
EXPECT_NO_THROWS(function() { rg_from_json2 = cluster.createRoutingGuideline("fromjson", default_cluster_guideline, {force: 1});} );

COMMON_RG_TESTS(rg_from_json, "fromjson", expected_default_cluster_guideline_destinations, expected_default_cluster_guideline_destinations_match, expected_default_cluster_guideline_routes, expected_default_cluster_guideline_routes_match, expected_default_cluster_guideline_routes_destinations, default_cluster_guideline, expected_show_output_from_json);

// restore the name
default_cluster_guideline["name"] = "default_cluster_rg";

// TESTS for read_only_targets and corresponding default RGs

//@<> createRoutingGuideline() + read_only_targets=read_replicas

// set read_only_targets = read_replicas
EXPECT_NO_THROWS(function() { cluster.setRoutingOption("read_only_targets", "read_replicas");} );

EXPECT_NO_THROWS(function() { rg_rr = cluster.createRoutingGuideline("default_cluster_read_replicas_rg");} );

const expected_show_output_default_cluster_read_replicas_rg = `
Routing Guideline: 'default_cluster_read_replicas_rg'
Cluster: 'cluster'

Routes
------
  - rw
    + Match: "$.session.targetPort = $.router.port.rw"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - ro
    + Match: "$.session.targetPort = $.router.port.ro"
    + Destinations:
      * None (ReadReplica)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

Destination Classes
-------------------
  - Primary:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - Secondary:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - ReadReplica:
    + Match: "$.server.memberRole = READ_REPLICA"
    + Instances:
      * None

Unreferenced servers
--------------------
  - None
`;

COMMON_RG_TESTS(rg_rr, "default_cluster_read_replicas_rg", expected_default_cluster_guideline_destinations, expected_default_cluster_guideline_destinations_match, expected_default_cluster_guideline_routes, expected_default_cluster_guideline_routes_match, expected_cluster_guideline_ro_targets_read_replicas_routes_destinations, cluster_guideline_ro_targets_read_replicas, expected_show_output_default_cluster_read_replicas_rg);

//@<> createRoutingGuideline() + read_only_targets=all

// set read_only_targets = all
EXPECT_NO_THROWS(function() { cluster.setRoutingOption("read_only_targets", "all");} );

EXPECT_NO_THROWS(function() { rg_all = cluster.createRoutingGuideline("default_cluster_all_rg");} );

const expected_show_output_default_cluster_all_rg = `
Routing Guideline: 'default_cluster_all_rg'
Cluster: 'cluster'

Routes
------
  - rw
    + Match: "$.session.targetPort = $.router.port.rw"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - ro
    + Match: "$.session.targetPort = $.router.port.ro"
    + Destinations:
      * None (Secondary)
      * None (ReadReplica)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

Destination Classes
-------------------
  - Primary:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - Secondary:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - ReadReplica:
    + Match: "$.server.memberRole = READ_REPLICA"
    + Instances:
      * None

Unreferenced servers
--------------------
  - None
`;

COMMON_RG_TESTS(rg_all, "default_cluster_all_rg", expected_default_cluster_guideline_destinations, expected_default_cluster_guideline_destinations_match, expected_default_cluster_guideline_routes, expected_default_cluster_guideline_routes_match, expected_cluster_guideline_ro_targets_all_routes_destinations, cluster_guideline_ro_targets_all, expected_show_output_default_cluster_all_rg);

// Restore default
EXPECT_NO_THROWS(function() { cluster.setRoutingOption("read_only_targets", null);} );

//@<> removeRoutingGuideline() - Bad options (should fail)
STDCHECK_ARGTYPES(cluster.removeRoutingGuideline, 1, ["abc"]);

//@<> removeRoutingGuideline() - Guideline does not exist (should fail)
EXPECT_THROWS(function(){ cluster.removeRoutingGuideline("unexisting");}, "Routing Guideline 'unexisting' does not exist");

//@<> removeRoutingGuideline() - Guideline in use by the topology (should fail)
EXPECT_THROWS(function(){ cluster.removeRoutingGuideline("default_cluster_rg");}, "Routing Guideline is in use and cannot be deleted");

EXPECT_OUTPUT_CONTAINS(`Routing Guideline 'default_cluster_rg' is currently being used by the topology.`);

//@<> removeRoutingGuideline() - OK
EXPECT_NO_THROWS(function() { rg = cluster.removeRoutingGuideline("anotherguideline");} );

EXPECT_NO_THROWS(function() { rg = cluster.removeRoutingGuideline("default_cluster_read_replicas_rg");} );

EXPECT_NO_THROWS(function() { rg = cluster.removeRoutingGuideline("default_cluster_all_rg");} );

// Getter tests

// Create a new RG
EXPECT_NO_THROWS(function() { cluster.createRoutingGuideline("newrg");} );

//@<> getRoutingGuideline() - no name: get the active one
EXPECT_NO_THROWS(function() { rg = cluster.getRoutingGuideline();} );

// Run some basic tests
var name;
EXPECT_NO_THROWS(function() { name = rg.getName(); });

EXPECT_EQ("default_cluster_rg", name);
EXPECT_EQ("default_cluster_rg", rg.name);

var json_defaults;
EXPECT_NO_THROWS(function() { json_defaults = rg.asJson(); });

EXPECT_JSON_EQ(default_cluster_guideline, json_defaults);

//@<> getRoutingGuideline() - get a specific one
EXPECT_NO_THROWS(function() { rg = cluster.getRoutingGuideline("newrg");} );

// Run some basic tests
var name;
EXPECT_NO_THROWS(function() { name = rg.getName(); });

EXPECT_EQ("newrg", name);
EXPECT_EQ("newrg", rg.name);

//@<> routingGuidelines()
var rgs;
EXPECT_NO_THROWS(function() { rgs = cluster.routingGuidelines(); } );

var row = rgs.fetchOne();
EXPECT_EQ("default_cluster_rg", row[0]);
EXPECT_EQ(1, row[1]);
EXPECT_EQ("rw,ro", row[2]);
EXPECT_EQ("Primary,Secondary,ReadReplica", row[3]);

row = rgs.fetchOne();
EXPECT_EQ("fromjson", row[0]);
EXPECT_EQ(0, row[1]);
EXPECT_EQ("rw,ro", row[2]);
EXPECT_EQ("Primary,Secondary,ReadReplica", row[3]);

row = rgs.fetchOne();
EXPECT_EQ("newrg", row[0]);
EXPECT_EQ(0, row[1]);
EXPECT_EQ("rw,ro", row[2]);
EXPECT_EQ("Primary,Secondary,ReadReplica", row[3]);

EXPECT_EQ(null, rgs.fetchOne());

//@<> routingGuidelines() - Add another guideline
var rg2;
EXPECT_NO_THROWS(function() { rg2 = cluster.createRoutingGuideline("rg2");} );

var rgs;
EXPECT_NO_THROWS(function() { rgs = cluster.routingGuidelines(); } );

var row = rgs.fetchOne();
EXPECT_EQ("default_cluster_rg", row[0]);
EXPECT_EQ(1, row[1]);
EXPECT_EQ("rw,ro", row[2]);
EXPECT_EQ("Primary,Secondary,ReadReplica", row[3]);

row = rgs.fetchOne();
EXPECT_EQ("fromjson", row[0]);
EXPECT_EQ(0, row[1]);
EXPECT_EQ("rw,ro", row[2]);
EXPECT_EQ("Primary,Secondary,ReadReplica", row[3]);

row = rgs.fetchOne();
EXPECT_EQ("newrg", row[0]);
EXPECT_EQ(0, row[1]);
EXPECT_EQ("rw,ro", row[2]);
EXPECT_EQ("Primary,Secondary,ReadReplica", row[3]);

row = rgs.fetchOne();
EXPECT_EQ("rg2", row[0]);
EXPECT_EQ(0, row[1]);
EXPECT_EQ("rw,ro", row[2]);
EXPECT_EQ("Primary,Secondary,ReadReplica", row[3]);

EXPECT_EQ(null, rgs.fetchOne());

//@<> rename() - bad args
STDCHECK_ARGTYPES(rg2.rename, 1, ["test"]);

//@<> rename - invalid name
EXPECT_THROWS(function(){ rg2.rename("xx/ -");}, "Routing Guideline name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (xx/ -)");
EXPECT_THROWS(function(){ rg2.rename("1xx");}, "Routing Guideline name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (1xx)");
EXPECT_THROWS(function(){ rg2.rename("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");}, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx: The Routing Guideline name can not be greater than 63 characters.");

//@<> rename - duplicate name
EXPECT_THROWS(function(){ rg2.rename("default_cluster_rg");}, "A Routing Guideline with the name 'default_cluster_rg' already exists");
EXPECT_THROWS(function(){ rg2.rename("newrg");}, "A Routing Guideline with the name 'newrg' already exists");

//@<> rename - OK
EXPECT_NO_THROWS(function() { rg2.rename("foo"); })
EXPECT_OUTPUT_CONTAINS(`Successfully renamed Routing Guideline 'rg2' to 'foo'`);

EXPECT_EQ("foo", rg2.name);
EXPECT_EQ("foo", rg2.asJson()["name"]);

EXPECT_NO_THROWS(function() { rg2.rename("rg2"); })
EXPECT_EQ("rg2", rg2.name);

//@<> addDestination - std bad arg type and count variations
STDCHECK_ARGTYPES(rg2.addDestination, 2, ["test"], ["true"], [{}]);

//@<> addDestination
rg2.addDestination("test1", "$.server.memberRole = PRIMARY")
rg2.addDestination("test2", "$.server.memberRole = SECONDARY")

// Key-value types must be auto-escaped only if version is 1.0, for example:
//   - "$.router.tags.key = 'value\" must be stored as
//   "$.router.tags.key = '\"value\"'"
//
// The following is a negative test
rg2.addDestination("tags", "$.server.tags.foo = 'bar'")

expected_destinations = ["Primary", "Secondary", "ReadReplica", "test1", "test2", "tags"];
expected_destinations_match = ["$.server.memberRole = PRIMARY",
                                     "$.server.memberRole = SECONDARY",
                                     "$.server.memberRole = READ_REPLICA",
                                     "$.server.memberRole = PRIMARY",
                                     "$.server.memberRole = SECONDARY",
                                     "$.server.tags.foo = 'bar'"];

expected_routes = ["rw", "ro"];
expected_routes_match = ["$.session.targetPort = $.router.port.rw",
                                "$.session.targetPort = $.router.port.ro"];
expected_routes_destinations = ["round-robin(Primary)",
                                       "round-robin(Secondary), round-robin(Primary)"];

expected_rg_asjson =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadReplica"
        },
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "test1"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "test2"
        },
        {
            "match": "$.server.tags.foo = 'bar'",
            "name": "tags"
        }
    ],
    "name": "rg2",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.1"
}

expected_show_output = `
Routing Guideline: 'rg2'
Cluster: 'cluster'

Routes
------
  - rw
    + Match: "$.session.targetPort = $.router.port.rw"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - ro
    + Match: "$.session.targetPort = $.router.port.ro"
    + Destinations:
      * None (Secondary)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

Destination Classes
-------------------
  - Primary:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - Secondary:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - ReadReplica:
    + Match: "$.server.memberRole = READ_REPLICA"
    + Instances:
      * None

  - test1:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - test2:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - tags:
    + Match: "$.server.tags.foo = 'bar'"
    + Instances:
      * None

Unreferenced servers
--------------------
  - None
`;

COMMON_RG_TESTS(rg2, "rg2", expected_destinations, expected_destinations_match, expected_routes, expected_routes_match, expected_routes_destinations, expected_rg_asjson, expected_show_output);

//@<> addDestination - dryRun
var dests = rg2.asJson()["destinations"];

rg2.addDestination("newdest", "true", {dryRun:1});

EXPECT_OUTPUT_CONTAINS("dryRun enabled, guideline not updated.");

EXPECT_EQ(dests, rg2.asJson()["destinations"]);

//@<> addDestination - bad name
EXPECT_THROWS(function () { rg2.addDestination("test1", "true") }, "Destination 'test1' already exists in the Routing Guideline")
EXPECT_THROWS(function () { rg2.addDestination("", "true") }, "The Destination name cannot be empty.")
EXPECT_THROWS(function () { rg2.addDestination("123", "true") }, "Destination name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (123)")
EXPECT_THROWS(function () { rg2.addDestination("prim$ary", "true") }, "Destination name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (prim$ary)")

//@<> addDestination - bad expr
EXPECT_THROWS(function () { rg2.addDestination("test", "") }, "match: field is expected to be a non empty string")
EXPECT_THROWS(function () { rg2.addDestination("test", "primary") }, "match: match does not evaluate to boolean")
EXPECT_THROWS(function () { rg2.addDestination("test", "==") }, "match: syntax error, unexpected =, expecting end of expression or error (character 1)")
EXPECT_THROWS(function () { rg2.addDestination("test", "$$memberRole = primary") }, "match: syntax error, $ not starting variable reference (character 1)")
EXPECT_THROWS(function () { rg2.addDestination("test", "$.server.memberRole = 'primary'") }, "match: type error, = operator, the type of left operand does not match right, expected ROLE but got STRING in '$.server.memberRole = 'primary''")

EXPECT_THROWS(function () { rg2.addDestination("test", "$.server.badvar = 123") }, "match: undefined variable: server.badvar in '$.server.badvar'")
EXPECT_THROWS(function () { rg2.addDestination("test", "$.router.badvar = 123") }, "match: undefined variable: router.badvar in '$.router.badvar'")
EXPECT_THROWS(function () { rg2.addDestination("test", "$.badvar = 123") }, "match: undefined variable: badvar in '$.badvar'")

// try to add a destination that depends on a session variable (forbidden)
EXPECT_THROWS(function () { rg2.addDestination("test", "$.session.sourceIP = '10.1.1.1'") }, "match: session.sourceIP may not be used in 'destinations' context")

//@<> no new destinations should've been added
EXPECT_EQ(json_defaults["destinations"].length + 3, rg2.asJson()["destinations"].length);

//@<> addRoute - std bad arg type and count variations
STDCHECK_ARGTYPES(rg2.addRoute, 3, ["test"], ["true"], [["first-available(test1)"]], [{}]);

//@<> addRoute
rg2.addRoute("test1", "$.session.targetPort in (6446, 6448)", ["first-available(Primary)"])
rg2.addRoute("test2", "$.session.targetPort in (6447, 6449)", ["round-robin(test1,test2)", "round-robin(Primary)"])

// Key-value types must be auto-escaped only if version is 1.0, for example:
//   - "$.router.tags.key = 'value\" must be stored as
//   "$.router.tags.key = '\"value\"'"
//
// The following is a negative test
rg2.addRoute("routetags", "$.router.tags.key = 'value'", ["round-robin(ReadReplica)"])

expected_destinations = ["Primary", "Secondary", "ReadReplica", "test1", "test2", "tags"];
expected_destinations_match = ["$.server.memberRole = PRIMARY",
                                     "$.server.memberRole = SECONDARY",
                                     "$.server.memberRole = READ_REPLICA",
                                     "$.server.memberRole = PRIMARY",
                                     "$.server.memberRole = SECONDARY",
                                     "$.server.tags.foo = 'bar'"];

expected_routes = ["rw", "ro", "test1", "test2", "routetags"];
expected_routes_match = [
  "$.session.targetPort = $.router.port.rw",
  "$.session.targetPort = $.router.port.ro",
  "$.session.targetPort in (6446, 6448)",
  "$.session.targetPort in (6447, 6449)",
  "$.router.tags.key = 'value'"
];
expected_routes_destinations = [
  "round-robin(Primary)",
  "round-robin(Secondary), round-robin(Primary)",
  "first-available(Primary)",
  "round-robin(test1, test2), round-robin(Primary)",
  "round-robin(ReadReplica)"
];

expected_rg_asjson =
{
    "destinations": [
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = READ_REPLICA",
            "name": "ReadReplica"
        },
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "test1"
        },
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "test2"
        },
        {
            "match": "$.server.tags.foo = 'bar'",
            "name": "tags"
        }
    ],
    "name": "rg2",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "first-available"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort in (6446, 6448)",
            "name": "test1"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "test1",
                        "test2"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                },
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 1,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort in (6447, 6449)",
            "name": "test2"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "ReadReplica"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.router.tags.key = 'value'",
            "name": "routetags"
        }
    ],
    "version": "1.1"
}

expected_show_output = `
Routing Guideline: 'rg2'
Cluster: 'cluster'

Routes
------
  - rw
    + Match: "$.session.targetPort = $.router.port.rw"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - ro
    + Match: "$.session.targetPort = $.router.port.ro"
    + Destinations:
      * None (Secondary)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - test1
    + Match: "$.session.targetPort in (6446, 6448)"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - test2
    + Match: "$.session.targetPort in (6447, 6449)"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (test1)
      * None (test2)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - routetags
    + Match: "$.router.tags.key = 'value'"
    + Destinations:
      * None (ReadReplica)

Destination Classes
-------------------
  - Primary:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - Secondary:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - ReadReplica:
    + Match: "$.server.memberRole = READ_REPLICA"
    + Instances:
      * None

  - test1:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - test2:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - tags:
    + Match: "$.server.tags.foo = 'bar'"
    + Instances:
      * None

Unreferenced servers
--------------------
  - None
`;

COMMON_RG_TESTS(rg2, "rg2", expected_destinations, expected_destinations_match, expected_routes, expected_routes_match, expected_routes_destinations, expected_rg_asjson, expected_show_output);

//@<> addRoute - bad name
EXPECT_THROWS(function () { rg2.addRoute("test1", "true", ["first-available(test1)"]) }, "Route 'test1' already exists in the Routing Guideline")

EXPECT_THROWS(function () { rg2.addRoute("", "true", ["first-available(test1)"]) }, "The Route name cannot be empty.")
EXPECT_THROWS(function () { rg2.addRoute("123", "true", ["first-available(test1)"]) }, "Route name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (123)")
EXPECT_THROWS(function () { rg2.addRoute("prim$ary", "true", ["first-available(test1)"]) }, "Route name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (prim$ary)")

//@<> addRoute - bad expr
EXPECT_THROWS(function () { rg2.addRoute("test", "", ["round-robin(test1)"]) }, "match: field is expected to be a non empty string")
EXPECT_THROWS(function () { rg2.addRoute("test", "primary", ["round-robin(test1)"]) }, "match: match does not evaluate to boolean")
EXPECT_THROWS(function () { rg2.addRoute("test", "==", ["round-robin(test1)"]) }, "match: syntax error, unexpected =, expecting end of expression or error (character 1)")
EXPECT_THROWS(function () { rg2.addRoute("test", "$$memberRole = primary", ["round-robin(test1)"]) }, "match: syntax error, $ not starting variable reference (character 1)")
EXPECT_THROWS(function () { rg2.addRoute("test", "$.router.port.rw = 'ab'", ["round-robin(test1)"]) }, "match: type error, = operator, the type of left operand does not match right, expected NUMBER but got STRING in '$.router.port.rw = 'ab''")

// try to add a route that depends on a server variable (forbidden)
EXPECT_THROWS(function () { rg2.addRoute("test", "$.server.memberRole = PRIMARY", ["round-robin(test1)"]) }, "match: server.memberRole may not be used in 'routes' context")

//@<> addRoute - bad dests
EXPECT_THROWS(function () { rg2.addRoute("test", "true", ["test1"]) }, "Invalid syntax for destination selector: ")
EXPECT_THROWS(function () { rg2.addRoute("test", "true", ["round-robin(test1)", ["a"]]) }, "String expected, but value is Array")
EXPECT_THROWS(function () { rg2.addRoute("test", "true", []) }, "destinations: field is expected to be a non empty array")
EXPECT_THROWS(function () { rg2.addRoute("test", "true", ["round-robin"]) }, "Invalid syntax for destination selector: ")
EXPECT_THROWS(function () { rg2.addRoute("test", "true", ["best(test1)"]) }, "unexpected value 'best', supported strategies: round-robin, first-available")

for (x of ["round_robin(test1)", "round-robin(test1,)", "round-robin(test1", "round-robin test1", "round-robin(test1))", "round-robin[test1]"]) {
    EXPECT_THROWS(function () { rg2.addRoute("test", "true", [x]) }, "Invalid syntax for destination selector: ")
    EXPECT_THROWS(function () { rg2.addRoute("test", "true", ["round-robin(test1)", x]) }, "Invalid syntax for destination selector: ")
}

EXPECT_THROWS(function () { rg2.addRoute("test", "true", ["round-robin(test1),invalid"]) }, "Invalid syntax for destination selector: ")

EXPECT_THROWS(function () { rg2.addRoute("test", "true", ["round-robin(baddest)"]) }, "Invalid destination 'baddest' referenced in route")

//@<> no new routes should've been added
EXPECT_EQ(json_defaults["routes"].length + 3, rg2.asJson()["routes"].length);

//@<> addRoute - enabled
rg2.addRoute("backup2", "$.session.user = 'backup'", ["round-robin(Secondary)", "round-robin(Primary)"], { "enabled": false });

EXPECT_EQ("backup2", rg2.asJson()["routes"][5]["name"])
EXPECT_EQ(false, rg2.asJson()["routes"][5]["enabled"])

//@<> addRoute - connection_sharing_allowed
EXPECT_EQ(true, rg2.asJson()["routes"][5]["connectionSharingAllowed"])

rg2.addRoute("backup4", "$.session.user = 'backup'", ["round-robin(Secondary)", "round-robin(Primary)"], { "connectionSharingAllowed": false });

EXPECT_EQ("backup4", rg2.asJson()["routes"][6]["name"])
EXPECT_EQ(false, rg2.asJson()["routes"][6]["connectionSharingAllowed"])

//@<> addRoute - dryRun
before = rg2.asJson();

rg2.addRoute("newroute", "false", ["round-robin(Secondary)"], { dryRun: 1 });

EXPECT_OUTPUT_CONTAINS("dryRun enabled, guideline not updated.");

EXPECT_EQ(before, rg2.asJson());

//@<> addRoute - whitespaces on destination classes should be allowed
EXPECT_NO_THROWS(function() { rg2.addRoute("whitespaces", "true", [" round-robin(Secondary)"], { dryRun: 1 }); } );
EXPECT_NO_THROWS(function() { rg2.addRoute("whitespaces", "true", ["round-robin(Secondary) "], { dryRun: 1 }); } );
EXPECT_NO_THROWS(function() { rg2.addRoute("whitespaces", "true", [" round-robin(Primary,     Secondary) "], { dryRun: 1 }); } );
EXPECT_NO_THROWS(function() { rg2.addRoute("whitespaces", "true", [" round-robin(    Primary,     Secondary) "], { dryRun: 1 }); } );
EXPECT_NO_THROWS(function() { rg2.addRoute("whitespaces", "true", [" round-robin     (    Primary,     Secondary ) "], { dryRun: 1 }); } );

EXPECT_THROWS(function() { rg2.addRoute("whitespaces", "true", [" round-robin     (    Primary.     Secondary) "], { dryRun: 1 }); }, "Invalid syntax for destination selector:");

//@<> addRoute - quotes on destination classes should be allowed
EXPECT_NO_THROWS(function() { rg2.addRoute("quotes", "true", [" round-robin('Secondary')"], { dryRun: 1 }); } );
EXPECT_NO_THROWS(function() { rg2.addRoute('quotes', 'true', ['round-robin("Secondary")   '], { dryRun: 1 }); } );
EXPECT_NO_THROWS(function() { rg2.addRoute("quotes", "true", [" round-robin('Primary', 'Secondary') "], { dryRun: 1 }); } );
EXPECT_NO_THROWS(function() { rg2.addRoute("quotes", "true", [" round-robin(    \"Primary\",     \"Secondary\") "], { dryRun: 1 }); } );
EXPECT_NO_THROWS(function() { rg2.addRoute("quotes", "true", [" round-robin     (    'Primary',     \'Secondary\' ) "], { dryRun: 1 }); } );

EXPECT_THROWS(function() { rg2.addRoute("quotes", "true", [" round-robin('Primary'. 'Secondary') "], { dryRun: 1 }); }, "Invalid syntax for destination selector:");

//@<> removeDestination() - bad args
STDCHECK_ARGTYPES(rg2.removeDestination, 1, ["xx"]);

//@<> removeDestination() - Destination not found
EXPECT_THROWS(function() { rg2.removeDestination("invalid"); }, "Destination 'invalid' not found.");
//@<> removeDestination() - Destination in use
EXPECT_THROWS(function() { rg2.removeDestination("Primary"); }, "Destination in use by one or more Routes");
EXPECT_OUTPUT_CONTAINS("ERROR: Destination 'Primary' is in use by route(s): backup2, backup4, ro, rw, test1, test2");

//@<> removeDestination() - OK
// Add dummy destination
rg2.addDestination("foo", "$.server.address = '192.168.5.5'")
EXPECT_JSON_EQ([
  "Primary",
  "Secondary",
  "ReadReplica",
  "test1",
  "test2",
  "tags",
  "foo"
], group_fields(rg2.destinations().fetchAll(), 0));

EXPECT_NO_THROWS(function() { rg2.removeDestination("foo"); } );

EXPECT_JSON_EQ([
  "Primary",
  "Secondary",
  "ReadReplica",
  "test1",
  "test2",
  "tags"
], group_fields(rg2.destinations().fetchAll(), 0));

//@<> removeRoute() - bad args
STDCHECK_ARGTYPES(rg2.removeRoute, 1, ["xx"]);

//@<> removeRoute() - Route not found
EXPECT_THROWS(function() { rg2.removeRoute("invalid"); }, "Route 'invalid' not found");

//@<> removeRoute() - OK
EXPECT_NO_THROWS(function() { rg2.removeRoute("test2"); } );

EXPECT_JSON_EQ([
  "rw",
  "ro",
  "test1",
  "routetags",
  "backup2",
  "backup4"
], group_fields(rg2.routes().fetchAll(), 0));

//@<> removeRoute() - Last route
EXPECT_NO_THROWS(function() { rg3 = cluster.createRoutingGuideline("rg3"); } );
EXPECT_NO_THROWS(function() { rg3.removeRoute("ro"); } );

EXPECT_THROWS(function() { rg3.removeRoute("rw"); }, "Cannot remove the last route of a Routing Guideline");
EXPECT_OUTPUT_CONTAINS("Route 'rw' is the only route in the Guideline and cannot be removed. A Routing Guideline must contain at least one route.");

EXPECT_NO_THROWS(function(){ cluster.removeRoutingGuideline("rg3"); });

//@<> setRoutingOption(guideline) - wrong args

// Add routers to the metadata
var cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

var router1 = "routerhost1::system";

session.runSql(
  "UPDATE `mysql_innodb_cluster_metadata`.`clusters` SET router_options = ? WHERE cluster_id = ?",
  [JSON.stringify(router_options_json), cluster_id]
);

session.runSql(
  "INSERT INTO `mysql_innodb_cluster_metadata`.`routers` VALUES (?, ?, ?, ?, ?, NULL, ?, ?, NULL, NULL)",
  [
    1, // ID
    "system", // Name
    "MySQL Router", // Description
    "routerhost1", // Hostname
    "9.2.0", // Version
    JSON.stringify(router_configuration_json), // Configuration
    cluster_id, // Cluster ID
  ]
);

var router2 = "routerhost2::system";

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.1.0', '2024-02-14 11:22:33', '{\"bootstrapTargetType\": \"cluster\", \"ROEndpoint\": \"mysqlro.sock\", \"RWEndpoint\": \"mysql.sock\", \"ROXEndpoint\": \"mysqlxro.sock\", \"RWXEndpoint\": \"mysqlx.sock\"}', ?, NULL, NULL)", [cluster_id]);

// Check args
EXPECT_THROWS(function(){ cluster.setRoutingOption("guideline", 1234);}, "Invalid value for routing option 'guideline'. Accepted value must be valid Routing Guideline name.");

EXPECT_THROWS(function(){ cluster.setRoutingOption("guideline", "unexisting");}, "Routing Guideline 'unexisting' does not exist");

EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "guideline", "unexisting");}, "Routing Guideline 'unexisting' does not exist");

EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "guideline", 123);}, "Invalid value for routing option 'guideline'. Accepted value must be valid Routing Guideline name.");

//@<> setRoutingOption(guideline) - Guideline does not exist (should fail)
EXPECT_THROWS(function(){ cluster.setRoutingOption("guideline", "unexisting");}, "Routing Guideline 'unexisting' does not exist");

//@<> setRoutingOption(guideline) - Some routers don't support it (should fail)

// Set it globally
EXPECT_THROWS(function(){ cluster.setRoutingOption("guideline", "rg2");}, "The following Routers in the cluster do not support the Routing Guideline and must be upgraded: routerhost2::system");

// Set it per Router
EXPECT_THROWS(function(){ cluster.setRoutingOption("routerhost2::system", "guideline", "rg2");}, "The following Routers in the cluster do not support the Routing Guideline and must be upgraded: routerhost2::system");

//@<> setRoutingOption(guideline) - Setting for a Router that supports it while others don't should be accepted
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("routerhost1::system", "guideline", "newrg");} );

// Make routerhost2::system support RGs

session.runSql("DELETE from mysql_innodb_cluster_metadata.routers where router_id=2");

const router_config = {
  ROEndpoint: "mysqlro.sock",
  RWEndpoint: "mysql.sock",
  ROXEndpoint: "mysqlxro.sock",
  RWXEndpoint: "mysqlx.sock",
  MetadataUser: "mysql_router1_mc1zc8dzj7yn",
  Configuration: {
    io: {
      backend: "linux_epoll",
      threads: 0,
    },
    common: {
      name: "system",
      user: "",
      read_timeout: 30,
      client_ssl_key: "/home/miguel/work/testbase/router_test/data/router-key.pem",
      client_ssl_cert: "/home/miguel/work/testbase/router_test/data/router-cert.pem",
      client_ssl_mode: "PREFERRED",
      connect_timeout: 5,
      server_ssl_mode: "PREFERRED",
      server_ssl_verify: "DISABLED",
      max_total_connections: 512,
      unknown_config_option: "error",
      router_require_enforce: true,
      max_idle_server_connections: 64,
    },
    loggers: {
      filelog: {
        level: "info",
        filename: "mysqlrouter.log",
        destination: "",
        timestamp_precision: "second",
      },
    },
    endpoints: {
      bootstrap_ro: {
        protocol: "classic",
        bind_port: 6447,
        bind_address: "0.0.0.0",
        destinations: "metadata-cache://tst/?role=SECONDARY",
        named_socket: "",
        server_ssl_ca: "",
        client_ssl_key: "/home/miguel/work/testbase/router_test/data/router-key.pem",
        server_ssl_crl: "",
        client_ssl_cert: "/home/miguel/work/testbase/router_test/data/router-cert.pem",
        client_ssl_mode: "PREFERRED",
        connect_timeout: 5,
        max_connections: 0,
        server_ssl_mode: "PREFERRED",
        routing_strategy: "round-robin-with-fallback",
        client_ssl_cipher: "",
        client_ssl_curves: "",
        net_buffer_length: 16384,
        server_ssl_capath: "",
        server_ssl_cipher: "",
        server_ssl_curves: "",
        server_ssl_verify: "DISABLED",
        thread_stack_size: 1024,
        connection_sharing: false,
        max_connect_errors: 100,
        server_ssl_crlpath: "",
        client_ssl_dh_params: "",
        client_connect_timeout: 9,
        connection_sharing_delay: 1.0,
      },
      bootstrap_rw: {
        protocol: "classic",
        bind_port: 6446,
        bind_address: "0.0.0.0",
        destinations: "metadata-cache://tst/?role=PRIMARY",
        named_socket: "",
        server_ssl_ca: "",
        client_ssl_key: "/home/miguel/work/testbase/router_test/data/router-key.pem",
        server_ssl_crl: "",
        client_ssl_cert: "/home/miguel/work/testbase/router_test/data/router-cert.pem",
        client_ssl_mode: "PREFERRED",
        connect_timeout: 5,
        max_connections: 0,
        server_ssl_mode: "PREFERRED",
        routing_strategy: "first-available",
        client_ssl_cipher: "",
        client_ssl_curves: "",
        net_buffer_length: 16384,
        server_ssl_capath: "",
        server_ssl_cipher: "",
        server_ssl_curves: "",
        server_ssl_verify: "DISABLED",
        thread_stack_size: 1024,
        connection_sharing: false,
        max_connect_errors: 100,
        server_ssl_crlpath: "",
        client_ssl_dh_params: "",
        client_connect_timeout: 9,
        connection_sharing_delay: 1.0,
      },
      bootstrap_x_ro: {
        protocol: "x",
        bind_port: 6449,
        bind_address: "0.0.0.0",
        destinations: "metadata-cache://tst/?role=SECONDARY",
        named_socket: "",
        server_ssl_ca: "",
        client_ssl_key: "/home/miguel/work/testbase/router_test/data/router-key.pem",
        server_ssl_crl: "",
        client_ssl_cert: "/home/miguel/work/testbase/router_test/data/router-cert.pem",
        client_ssl_mode: "PREFERRED",
        connect_timeout: 5,
        max_connections: 0,
        server_ssl_mode: "PREFERRED",
        routing_strategy: "round-robin-with-fallback",
        client_ssl_cipher: "",
        client_ssl_curves: "",
        net_buffer_length: 16384,
        server_ssl_capath: "",
        server_ssl_cipher: "",
        server_ssl_curves: "",
        server_ssl_verify: "DISABLED",
        thread_stack_size: 1024,
        connection_sharing: false,
        max_connect_errors: 100,
        server_ssl_crlpath: "",
        client_ssl_dh_params: "",
        client_connect_timeout: 9,
        connection_sharing_delay: 1.0,
      },
      bootstrap_x_rw: {
        protocol: "x",
        bind_port: 6448,
        bind_address: "0.0.0.0",
        destinations: "metadata-cache://tst/?role=PRIMARY",
        named_socket: "",
        server_ssl_ca: "",
        client_ssl_key: "/home/miguel/work/testbase/router_test/data/router-key.pem",
        server_ssl_crl: "",
        client_ssl_cert: "/home/miguel/work/testbase/router_test/data/router-cert.pem",
        client_ssl_mode: "PREFERRED",
        connect_timeout: 5,
        max_connections: 0,
        server_ssl_mode: "PREFERRED",
        routing_strategy: "first-available",
        client_ssl_cipher: "",
        client_ssl_curves: "",
        net_buffer_length: 16384,
        server_ssl_capath: "",
        server_ssl_cipher: "",
        server_ssl_curves: "",
        server_ssl_verify: "DISABLED",
        thread_stack_size: 1024,
        connection_sharing: false,
        max_connect_errors: 100,
        server_ssl_crlpath: "",
        client_ssl_dh_params: "",
        client_connect_timeout: 9,
        connection_sharing_delay: 1.0,
      },
    },
    http_server: {
      ssl: true,
      port: 8443,
      ssl_key: "/home/miguel/work/testbase/router_test/data/router-key.pem",
      ssl_cert: "/home/miguel/work/testbase/router_test/data/router-cert.pem",
      ssl_cipher: "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384",
      bind_address: "0.0.0.0",
      ssl_curves: "",
      ssl_dh_params: "",
      static_folder: "",
    },
    rest_configs: {
      rest_api: { require_realm: "" },
      rest_router: { require_realm: "default_auth_realm" },
      rest_routing: { require_realm: "default_auth_realm" },
      rest_metadata_cache: { require_realm: "default_auth_realm" },
    },
    routing_rules: {
      read_only_targets: "secondaries",
      stats_updates_frequency: -1,
      use_replica_primary_as_rw: false,
      invalidated_cluster_policy: "drop_all",
      unreachable_quorum_allowed_traffic: "none",
    },
    metadata_cache: {
      ttl: 0.5,
      user: "mysql_router1_mc1zc8dzj7yn",
      read_timeout: 30,
      connect_timeout: 5,
      auth_cache_ttl: -1.0,
      auth_cache_refresh_interval: 2.0,
    },
    connection_pool: {
      idle_timeout: 5,
      max_idle_server_connections: 64,
    },
    destination_status: {
      error_quarantine_interval: 1,
      error_quarantine_threshold: 1,
    },
    http_authentication_realm: {
      name: "default_realm",
      method: "basic",
      backend: "default_auth_backend",
      require: "valid-user",
    },
    http_authentication_backends: {
      default_auth_backend: { backend: "metadata_cache", filename: "" },
    },
  },
  RWSplitEndpoint: "6450",
  bootstrapTargetType: "cluster",
  LocalCluster: "cluster",
  CurrentRoutingGuideline: "default_cluster_rg",
  SupportedRoutingGuidelinesVersion: "1.1",
};

session.runSql(
  `
  INSERT INTO \`mysql_innodb_cluster_metadata\`.\`routers\`
  VALUES (
    2,
    'system',
    'MySQL Router',
    'routerhost2',
    '8.4.0',
    '2024-02-14 11:22:33',
    ?,
    ?,
    NULL,
    NULL
  )
  `,
  [JSON.stringify(router_config), cluster_id]
);

//@<> setRoutingOption(guideline) - global OK
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("guideline", "rg2");} );

EXPECT_OUTPUT_CONTAINS(`Routing option 'guideline' successfully updated.`);
EXPECT_OUTPUT_CONTAINS(`Routing Guideline 'rg2' has been enabled and is now the active guideline for the topology.`);

//@<> setRoutingOption(read_only_targets) should print a warning when a guideline is active
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("read_only_targets", "all");} );
EXPECT_OUTPUT_CONTAINS(`WARNING: The 'read_only_targets' setting has no effect because the Routing Guideline 'rg2' is currently active in the topology. Routing Guidelines take precedence over this configuration.`);

//@<> setRoutingOption(guideline) - Guideline already active - no-op
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("guideline", "rg2");} );
EXPECT_OUTPUT_CONTAINS(`Routing Guideline 'rg2' is already active in the topology.`);

// Verify that the Guideline was set as active in the Metadata
var cid = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

var rgid = session.runSql("select guideline_id from mysql_innodb_cluster_metadata.routing_guidelines where name = 'rg2'").fetchOne()[0];

var rgid_router_options = session.runSql("select json_unquote(router_options->'$.guideline') from mysql_innodb_cluster_metadata.clusters where cluster_id=?", [cid]).fetchOne()[0];

EXPECT_EQ(rgid_router_options, rgid);

// Confirm in routerOptions()
router_options = cluster.routerOptions();
EXPECT_EQ("rg2", router_options["configuration"]["routing_rules"]["guideline"]);

//@<> setRoutingOption(guideline) - router individually OK
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption(router1, "guideline", "newrg");} );

// Verify that the Guideline was set in the router's metadata
var rgid = session.runSql("select guideline_id from mysql_innodb_cluster_metadata.routing_guidelines where name = 'newrg'").fetchOne()[0];

var rgid_router_options = session.runSql("select json_unquote(options->'$.guideline') from mysql_innodb_cluster_metadata.routers where router_id=1").fetchOne()[0];

EXPECT_EQ(rgid_router_options, rgid);

// Confirm in routerOptions()
router_options = cluster.routerOptions();
EXPECT_EQ("newrg", router_options["routers"][router1]["configuration"]["routing_rules"]["guideline"]);

//@<> routerOptions(extended:2) should not include guideline for Routers that do not support it

// Make routerhost2::system NOT support RGs again
session.runSql("DELETE from mysql_innodb_cluster_metadata.routers where router_id=2");
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.1.0', '2024-02-14 11:22:33', '{\"bootstrapTargetType\": \"cluster\", \"ROEndpoint\": \"mysqlro.sock\", \"RWEndpoint\": \"mysql.sock\", \"ROXEndpoint\": \"mysqlxro.sock\", \"RWXEndpoint\": \"mysqlx.sock\"}', ?, NULL, NULL)", [cluster_id]);

router_options = cluster.routerOptions();
EXPECT_EQ("newrg", router_options["routers"][router1]["configuration"]["routing_rules"]["guideline"]);
EXPECT_EQ(undefined, router_options["routers"][router2]["configuration"]["routing_rules"])

router_options = cluster.routerOptions({extended:1});
EXPECT_EQ("newrg", router_options["routers"][router1]["configuration"]["routing_rules"]["guideline"]);
EXPECT_EQ(undefined, router_options["routers"][router2]["configuration"]["routing_rules"])

router_options = cluster.routerOptions({extended:2});
EXPECT_EQ("newrg", router_options["routers"][router1]["configuration"]["routing_rules"]["guideline"]);
EXPECT_EQ(undefined, router_options["routers"][router2]["configuration"]["routing_rules"]["guideline"])

// Make routerhost2::system support RGs again
session.runSql("DELETE from mysql_innodb_cluster_metadata.routers where router_id=2");
session.runSql(
  `
  INSERT INTO \`mysql_innodb_cluster_metadata\`.\`routers\`
  VALUES (
    2,
    'system',
    'MySQL Router',
    'routerhost2',
    '8.4.0',
    '2024-02-14 11:22:33',
    ?,
    ?,
    NULL,
    NULL
  )
  `,
  [JSON.stringify(router_config), cluster_id]
);

//@<> removeRoutingGuideline() - Guideline in use by a Router (should fail)
EXPECT_THROWS(function(){ cluster.removeRoutingGuideline("newrg");}, "Routing Guideline is in use and cannot be deleted");
EXPECT_OUTPUT_CONTAINS("ERROR: Routing Guideline 'newrg' is currently being used by Router 'routerhost1::system'");

//@<> setRoutingOption(guideline) - global OK - no change in routers
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("guideline", "default_cluster_rg");} );

// Confirm the change globally
router_options = cluster.routerOptions();
EXPECT_EQ("default_cluster_rg", router_options["configuration"]["routing_rules"]["guideline"]);

// Confirm routers remain unchanged
EXPECT_EQ("newrg", router_options["routers"][router1]["configuration"]["routing_rules"]["guideline"]);

//@<> setRoutingOption(guideline) - disable globally OK
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("guideline", null);} );

// Confirm in routerOptions()
router_options = cluster.routerOptions();
EXPECT_FALSE("guideline" in router_options["configuration"]["routing_rules"]);

// Confirm routers remain unchanged
EXPECT_EQ("newrg", router_options["routers"][router1]["configuration"]["routing_rules"]["guideline"]);

// Enable back
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("guideline", "rg2");} );

//@<> listRouters - check LocalCluster, CurrentRoutingGuideline, supportedRoutingGuidelinesVersion are included
cluster.listRouters();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster",
    "routers": {
        "routerhost1::system": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost1",
            "lastCheckIn": null,
            "localCluster": "devCluster",
            "roPort": "6447",
            "roXPort": "6449",
            "rwPort": "6446",
            "rwSplitPort": "6450",
            "rwXPort": "6448",
            "supportedRoutingGuidelinesVersion": "1.1",
            "version": "9.2.0"
        },
        "routerhost2::system": {
            "currentRoutingGuideline": "default_cluster_rg",
            "hostname": "routerhost2",
            "lastCheckIn": "2024-02-14 11:22:33",
            "localCluster": "cluster",
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "rwPort": "mysql.sock",
            "rwSplitPort": "6450",
            "rwXPort": "mysqlx.sock",
            "supportedRoutingGuidelinesVersion": "1.1",
            "version": "8.4.0"
        }
    }
}
`);

//@<> setDestinationOption - bad args
EXPECT_THROWS(function () { rg2.setDestinationOption("test1", "otherOption", true) }, "Invalid value for destination option, accepted values: match")

EXPECT_THROWS(function () { rg2.setDestinationOption("test1", "match", true) }, "Invalid value for destination option 'match', value is expected to be a string.")

EXPECT_THROWS(function () { rg2.setDestinationOption("test1", "match", 123) }, "Invalid value for destination option 'match', value is expected to be a string.")

EXPECT_THROWS(function () { rg2.setDestinationOption("test1", "match", {"foo": "bar"}) }, "Invalid value for destination option 'match', value is expected to be a string.")

//@<> setDestinationOption - destination unexisting (should fail)
EXPECT_THROWS(function () { rg2.setDestinationOption("foo", "match", true) }, "Destination 'foo' not found")

//@<> setDestinationOption - match value invalid (should fail)
EXPECT_THROWS(function () { rg2.setDestinationOption("test1", "match", "$.server.badvar = 123") }, "match: undefined variable: server.badvar in '$.server.badvar'")

//@<> setDestinationOption - OK
EXPECT_NO_THROWS(function(){ rg2.setDestinationOption("test1", "match", "$.server.memberRole = PRIMARY");} );

// Key-value types must be auto-escaped only if version is 1.0, for example:
//   - "$.router.tags.key = 'value\" must be stored as
//   "$.router.tags.key = '\"value\"'"
//
// The following is a negative test

//@<> setDestinationOption - key-value types must be escaped
EXPECT_NO_THROWS(function(){ rg2.setDestinationOption("tags", "match", "$.server.tags.foo = 'baz'");} );

EXPECT_EQ("$.server.tags.foo = 'baz'", rg2.asJson()["destinations"][5]["match"]);

expected_destinations_match_changed = [
  "$.server.memberRole = PRIMARY",
  "$.server.memberRole = SECONDARY",
  "$.server.memberRole = READ_REPLICA",
  "$.server.memberRole = PRIMARY",
  "$.server.memberRole = SECONDARY",
  "$.server.tags.foo = 'baz'"
];

var destinations;
EXPECT_NO_THROWS(function() { destinations = rg2.destinations().fetchAll(); });

EXPECT_JSON_EQ(expected_destinations, group_fields(destinations, 0), ".expected_destinations");
EXPECT_JSON_EQ(expected_destinations_match_changed, group_fields(destinations, 1), ".expected_destinations_match");

//@<> setRouteOption - bad args
EXPECT_THROWS(function () { rg2.setRouteOption("rw", "wrongOption", true) }, "Invalid value for route option, accepted values: match, destinations, enabled")

EXPECT_THROWS(function () { rg2.setRouteOption("rw", "match", true) }, "Invalid value for route option 'match', value is expected to be a string.")

EXPECT_THROWS(function () { rg2.setRouteOption("rw", "destinations", "123") }, "Invalid value for route option 'destinations', value is expected to be an array")

EXPECT_THROWS(function () { rg2.setRouteOption("rw", "enabled", "[123]") }, "Invalid value for route option 'enabled', value is expected to be a bool.")

EXPECT_THROWS(function () { rg2.setRouteOption("rw", "connectionSharingAllowed", -1) }, "Invalid value for route option 'connectionSharingAllowed', value is expected to be a bool.")

//@<> setRouteOption - route unexisting (should fail)
EXPECT_THROWS(function () { rg2.setRouteOption("foo", "match", true) }, "Route 'foo' not found")

//@<> setRouteOption - match value invalid (should fail)
EXPECT_THROWS(function () { rg2.setRouteOption("rw", "match", "$.session.foobar") }, "match: undefined variable: session.foobar in '$.session.foobar'")

//@<> setRouteOption - destination value invalid (should fail)
EXPECT_THROWS(function () { rg2.setRouteOption("ro", "destinations", ["round-robin(Foo)"]) }, "Invalid destination 'Foo' referenced in route 'ro'")

//@<> setRouteOption - OK - match
EXPECT_NO_THROWS(function(){ rg2.setRouteOption("ro", "match", "$.router.hostname = 'test'");} );

expected_routes_match_changed = [
  "$.session.targetPort = $.router.port.rw",
  "$.router.hostname = 'test'",
  "$.session.targetPort in (6446, 6448)",
  "$.router.tags.key = 'value'",
  "$.session.user = 'backup'",
  "$.session.user = 'backup'"];

expected_routes_new = [
  "rw",
  "ro",
  "test1",
  "routetags",
  "backup2",
  "backup4"
];

// Confirm in routes()
var routes;
EXPECT_NO_THROWS(function() { routes = rg2.routes().fetchAll(); });

EXPECT_JSON_EQ(expected_routes_new, group_fields(routes, 0), ".expected_routes");
EXPECT_JSON_EQ(expected_routes_match_changed, group_fields(routes, 3), ".expected_routes_match");

//@<> setRouteOption - OK - enabled
EXPECT_JSON_EQ([1, 1, 1, 1, 0, 1], group_fields(rg2.routes().fetchAll(), 1))

EXPECT_NO_THROWS(function(){ rg2.setRouteOption("rw", "enabled", false);} );

EXPECT_JSON_EQ([0, 1, 1, 1, 0, 1], group_fields(rg2.routes().fetchAll(), 1))

//@<> setRouteOption - OK - destinations
EXPECT_NO_THROWS(function(){ rg2.setRouteOption("ro", "destinations", ["round-robin(ReadReplica)", "round-robin(Primary)"]);} );

expected_routes_destinations_changed = [
    "round-robin(Primary)",
    "round-robin(ReadReplica), round-robin(Primary)",
    "first-available(Primary)",
    "round-robin(ReadReplica)",
    "round-robin(Secondary), round-robin(Primary)",
    "round-robin(Secondary), round-robin(Primary)"
];

//@<> setRouteOption - whitespaces on destination classes should be allowed
EXPECT_NO_THROWS(function() { rg2.setRouteOption("ro", "destinations", [" round-robin(Secondary)"]); } );
EXPECT_NO_THROWS(function() { rg2.setRouteOption("ro", "destinations", ["round-robin(Secondary) "]); } );
EXPECT_NO_THROWS(function() { rg2.setRouteOption("ro", "destinations", [" round-robin(Primary,     Secondary) "]); } );
EXPECT_NO_THROWS(function() { rg2.setRouteOption("ro", "destinations", [" round-robin(    Primary,     Secondary) "]); } );
EXPECT_NO_THROWS(function() { rg2.setRouteOption("ro", "destinations", [" round-robin     (    Primary,     Secondary ) "]); } );

EXPECT_THROWS(function() { rg2.setRouteOption("ro", "destinations", [" round-robin     (    Primary.     Secondary) "]); }, "Invalid syntax for destination selector:");

EXPECT_NO_THROWS(function(){ rg2.setRouteOption("ro", "destinations", ["round-robin(ReadReplica)", "round-robin(Primary)"]);} );

//@<> setRouteOption - quotes on destination classes should be allowed
EXPECT_NO_THROWS(function() { rg2.setRouteOption("ro", "destinations", ["round-robin('Secondary')"]); } );
EXPECT_NO_THROWS(function() { rg2.setRouteOption("ro", "destinations", ["round-robin('Secondary', \'Primary\')"]); } );
EXPECT_NO_THROWS(function() { rg2.setRouteOption('ro', 'destinations', ['round-robin("Secondary",     \"Primary\")']); } );

EXPECT_JSON_EQ([
  "round-robin(Primary)",
  "round-robin(Secondary, Primary)",
  "first-available(Primary)",
  "round-robin(ReadReplica)",
  "round-robin(Secondary), round-robin(Primary)",
  "round-robin(Secondary), round-robin(Primary)"
], group_fields(rg2.routes().fetchAll(), 4));

EXPECT_NO_THROWS(function(){ rg2.setRouteOption("ro", "destinations", ["round-robin(ReadReplica)", "round-robin(Primary)"]);} );

var routes;
EXPECT_NO_THROWS(function() { routes = rg2.routes().fetchAll(); });

EXPECT_JSON_EQ(expected_routes_destinations_changed, group_fields(routes, 4), ".expected_destinations_match");

// Key-value types must be auto-escaped only if version is 1.0, for example:
//   - "$.router.tags.key = 'value\" must be stored as
//   "$.router.tags.key = '\"value\"'"
//
// The following is a negative test

//@<> setRouteOption - key-value types must be escaped
EXPECT_NO_THROWS(function(){ rg2.setRouteOption("routetags", "match", "$.router.tags.key = 'foobar'");} );

EXPECT_EQ("$.router.tags.key = 'foobar'", rg2.asJson().routes[3]["match"])

//@<> copy() - bad args
STDCHECK_ARGTYPES(rg2.copy, 1, ["test"]);

//@<> copy() - Invalid name (should fail)
EXPECT_THROWS(function(){ rg2.copy("xx/ -");}, "Routing Guideline name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (xx/ -)");
EXPECT_THROWS(function(){ rg2.copy("1xx");}, "Routing Guideline name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (1xx)");
EXPECT_THROWS(function(){ rg2.copy("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");}, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx: The Routing Guideline name can not be greater than 63 characters.");

//@<> copy() - Name already exists (should fail)
EXPECT_THROWS(function(){ rg2.copy("rg2");}, "A Routing Guideline with the name 'rg2' already exists.");

//@<> copy() - OK
var rg2_copy;
EXPECT_NO_THROWS(function(){ rg2_copy = rg2.copy("imacopy");} );

EXPECT_EQ(rg2_copy.name, "imacopy");

var rg2_json = rg2.asJson()
rg2_json["name"] = "imacopy"

EXPECT_EQ(rg2_json, rg2_copy.asJson());

// Ensure names are properly set
EXPECT_EQ("rg2", rg2.asJson()["name"])
EXPECT_EQ("imacopy", rg2_copy.asJson()["name"])

//@<> export() - bad args
STDCHECK_ARGTYPES(rg2.export, 1, ["test"]);

//@<> export() - filePath empty
EXPECT_THROWS(function(){ rg2.export("");}, "The output file path cannot be empty");

//@<> export() - file already exists
var file_path = "rg.json"
testutil.createFile("rg.json", "");

EXPECT_THROWS(function(){ rg2.export(file_path);}, `The file '${file_path}' already exists. Please provide a different file path.`);

testutil.rmfile(file_path);

//@<> export() - filepath not writable
unwritable_path = os.path.join(os.getcwd(), "foobar");
testutil.mkdir(unwritable_path);
testutil.chmod(unwritable_path, 0555);

EXPECT_THROWS(function(){ rg2.export(unwritable_path);}, `Failed to open ${unwritable_path} for writing.`);

//@<> export() - OK
var rg2_path = "rg2.json";
EXPECT_NO_THROWS(function(){ rg2.export(rg2_path);} );

// Read and normalize file content
var file_content = testutil.catFile(rg2_path);
var file_json = JSON.parse(file_content);

EXPECT_EQ(file_json, rg2.asJson());

EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("guideline", "imacopy");} );
EXPECT_NO_THROWS(function(){ cluster.removeRoutingGuideline("rg2");} );

//@<> import_routing_guideline() - filepath empty
EXPECT_THROWS(function(){ cluster.importRoutingGuideline("");}, "The input file path cannot be empty");

//@<> import_routing_guideline() - filepath doesn't exist
var file_path = __os_type == "windows" ? "\\\\?\\foo\\bar" : "/foo/bar";

EXPECT_THROWS(function () { cluster.importRoutingGuideline(file_path); }, `${file_path}: No such file or directory`);

//@<> import_routing_guideline() - filepath does not point to an existing file
EXPECT_THROWS(function(){ cluster.importRoutingGuideline("foo");}, "foo: No such file or directory");

//@<> import_routing_guideline() - invalid file - empty
var invalid_path = "invalid.json";
testutil.createFile(invalid_path, "");

EXPECT_THROWS(function(){ cluster.importRoutingGuideline("invalid.json");}, "Invalid Routing Guideline document: 'invalid.json' is empty.");

testutil.rmfile(invalid_path);

//@<> import_routing_guideline() - invalid file - empty json
var invalid_path = "invalid.json";
testutil.createFile(invalid_path, "{}");

EXPECT_THROWS(function(){ cluster.importRoutingGuideline("invalid.json");}, "Invalid Routing Guideline document: 'invalid.json' is not a valid file or contains no valid documents");

testutil.rmfile(invalid_path);

//@<> import_routing_guideline() - invalid file - multiple docs
var invalid_path = "invalid.json";
testutil.createFile(invalid_path, "{}{}{}");

EXPECT_THROWS(function(){ cluster.importRoutingGuideline("invalid.json");}, "Invalid Routing Guideline document: 'invalid.json' contains multiple documents, which is not supported.");

testutil.rmfile(invalid_path);

//@<> import_routing_guideline() - invalid version in file

// save the original version
original_version = file_json.version;

// change it to test
file_json.version = "a.b";

// write to file
var rg2_invalid_path = "rg2_invalid.json";
testutil.createFile(rg2_invalid_path, JSON.stringify(file_json, null, 2));

EXPECT_THROWS(function(){ cluster.importRoutingGuideline(rg2_invalid_path);}, "Invalid routing guideline version 'a.b'");

testutil.rmfile(rg2_invalid_path);

// change again, unsupported version
file_json.version = "2.0";

// write to file
testutil.createFile(rg2_invalid_path, JSON.stringify(file_json, null, 2));

EXPECT_THROWS(function(){ cluster.importRoutingGuideline(rg2_invalid_path);}, "Routing guideline version 2.0 is not supported, upgrade MySQL Shell");

testutil.rmfile(rg2_invalid_path);

// change again, patch version not allowed
file_json.version = "1.0.0";

testutil.createFile(rg2_invalid_path, JSON.stringify(file_json, null, 2));

EXPECT_THROWS(function(){ cluster.importRoutingGuideline(rg2_invalid_path);}, "Invalid routing guideline version format '1.0.0': expected format is 'x.y'");

testutil.rmfile(rg2_invalid_path);

// change again, extras not allowed
file_json.version = "1.0-foobar";

testutil.createFile(rg2_invalid_path, JSON.stringify(file_json, null, 2));

EXPECT_THROWS(function(){ cluster.importRoutingGuideline(rg2_invalid_path);}, "Invalid routing guideline version format '1.0-foobar': expected format is 'x.y'");

testutil.rmfile(rg2_invalid_path);

// restore the original version
file_json.version = original_version;

//@<> import_routing_guideline() - invalid json (naming)
var rg2_invalid_naming = "rg2_invalid.json";

testutil.createFile(rg2_invalid_naming, JSON.stringify(invalid_guideline_invalid_names, null, 2));

EXPECT_THROWS(function(){ rg2 = cluster.importRoutingGuideline(rg2_invalid_naming);}, "Invalid guideline document: ArgumentError: Destination name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (ReadR eplica)");

testutil.rmfile(rg2_invalid_naming);

//@<> import_routing_guideline() - OK
EXPECT_NO_THROWS(function(){ rg2 = cluster.importRoutingGuideline(rg2_path);});

// Read and normalize file content
var file_content = testutil.catFile(rg2_path);
var file_json = JSON.parse(file_content);

EXPECT_EQ(file_json, rg2.asJson());

// Confirm .show()
expected_show_output = `
Routing Guideline: 'rg2'
Cluster: 'cluster'

Routes
------
  - rw
    + Match: "$.session.targetPort = $.router.port.rw"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - ro
    + Match: "$.router.hostname = 'test'"
    + Destinations:
      * None (ReadReplica)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - test1
    + Match: "$.session.targetPort in (6446, 6448)"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - routetags
    + Match: "$.router.tags.key = 'foobar'"
    + Destinations:
      * None (ReadReplica)

  - backup2
    + Match: "$.session.user = 'backup'"
    + Destinations:
      * None (Secondary)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - backup4
    + Match: "$.session.user = 'backup'"
    + Destinations:
      * None (Secondary)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

Destination Classes
-------------------
  - Primary:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - Secondary:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - ReadReplica:
    + Match: "$.server.memberRole = READ_REPLICA"
    + Instances:
      * None

  - test1:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - test2:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - tags:
    + Match: "$.server.tags.foo = 'baz'"
    + Instances:
      * None

Unreferenced servers
--------------------
  - None
`;

EXPECT_NO_THROWS(function() { rg2.show(); });
EXPECT_OUTPUT_CONTAINS_MULTILINE(expected_show_output);

//@<> .show() must require the 'router' option when a destination includes $.router.* vars.

// Add a new destination with a $.router var
rg.addDestination("RouterVar", "$.server.memberRole = SECONDARY AND $.server.clusterName = $.router.localCluster")

// Check the error
EXPECT_THROWS(function(){ rg.show(); }, "Option 'router' not set: The Routing Guideline contains destinations configured to use router-specific variables.");
EXPECT_OUTPUT_CONTAINS("ERROR: The Routing Guideline contains destinations configured to use router-specific variables (e.g., '$.router.localCluster'), but the 'router' option is not set. This may cause potential mismatches with the live router behavior. Please set the 'router' option to specify the Router that should be used for evaluation.");

// Confirm no error when the option is given
EXPECT_NO_THROWS(function() { rg.show({router: router1}); });

rg.removeDestination("RouterVar");

//@<> import_routing_guideline() - Duplicate guideline
EXPECT_THROWS(function(){ cluster.importRoutingGuideline(rg2_path);}, "A Routing Guideline with the name 'rg2' already exists");

//@<> import_routing_guideline() - Duplicate guideline - force:1
EXPECT_NO_THROWS(function(){ rg2 = cluster.importRoutingGuideline(rg2_path, {force: 1});});

// Confirm it's all OK
EXPECT_EQ(file_json, rg2.asJson());

// another
var test_import_valid = "test_import.json";
var test_import_asjson =
{
    "destinations": [
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        }
    ],
    "name": "test_import",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        }
    ],
    "version": "1.1"
}

testutil.createFile(test_import_valid, JSON.stringify(test_import_asjson, null, 2));

//@<> import_routing_guideline() - OK another (using force should not affect it)
EXPECT_NO_THROWS(function(){ test_import = cluster.importRoutingGuideline(test_import_valid, {force: 1});});

EXPECT_OUTPUT_CONTAINS(`Routing Guideline 'test_import' successfully imported.`);

EXPECT_EQ(test_import_asjson, test_import.asJson());

//@<> import_routing_guideline() - dryRun - invalid guideline
var test_import_invalid = "test_import_invalid.json";
var test_import_invalid_asjson =
{
    "destinations": [
        {
            "match": "$.server.memberRole = WRONG",
            "name": "Secondary"
        }
    ],
    "name": "test_import",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        }
    ],
    "version": "1.1"
}

testutil.createFile(test_import_invalid, JSON.stringify(test_import_invalid_asjson, null, 2));

EXPECT_THROWS(function(){ test_import_obj = cluster.importRoutingGuideline(test_import_invalid);}, "Invalid guideline document: Errors while parsing routing guidelines document:\n- destinations[0].match: type error, = operator, the type of left operand does not match right, expected ROLE but got STRING in '$.server.memberRole = WRONG'\n- destinations[0]: 'match' field not defined\n- no destination classes defined by the document");

//@<> import_routing_guideline() - If no other guideline exists, the imported one must NOT be set as the active one

// Remove all guidelines
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("guideline", null);});
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption(router1, "guideline", null);});
EXPECT_NO_THROWS(function(){ cluster.removeRoutingGuideline("default_cluster_rg");});
EXPECT_NO_THROWS(function(){ cluster.removeRoutingGuideline("fromjson"); });
EXPECT_NO_THROWS(function(){ cluster.removeRoutingGuideline("test_import");});
EXPECT_NO_THROWS(function(){ cluster.removeRoutingGuideline("newrg");});
EXPECT_NO_THROWS(function(){ cluster.removeRoutingGuideline("imacopy");});
EXPECT_NO_THROWS(function(){ cluster.removeRoutingGuideline("rg2"); });


// Import the guideline from file
EXPECT_NO_THROWS(function(){ test_import = cluster.importRoutingGuideline(test_import_valid);});

// Confirm that the guideline was NOT set as the active one
var router_options = cluster.routerOptions();
EXPECT_EQ(false, router_options["configuration"]["routing_rules"].hasOwnProperty("guideline"));

//@<> import_routing_guideline() - 'rename' and 'force' cannot be use simultaneously
EXPECT_THROWS(function(){ cluster.importRoutingGuideline(test_import_valid, {force: true, rename: "foo"});}, "Options 'force' and 'rename' are mutually exclusive");

//@<> import_routing_guideline() - 'rename' must allow renaming a guideline that has a name which is duplicate of an existing guideline

// Rename with the same name must fail
EXPECT_THROWS(function(){ cluster.importRoutingGuideline(test_import_valid, { rename: "test_import"});}, "A Routing Guideline with the name 'test_import' already exists");

// Rename with an empty value must fail
EXPECT_THROWS(function(){ cluster.importRoutingGuideline(test_import_valid, { rename: ""});}, "Option 'rename' cannot be empty.");

// Rename with a valid non-duplicate name must succeed
EXPECT_NO_THROWS(function(){ test_import = cluster.importRoutingGuideline(test_import_valid, { rename: "test_import_new_name"});});

// Confirm that the guideline was NOT set as the active one
var router_options = cluster.routerOptions();
EXPECT_EQ(false, router_options["configuration"]["routing_rules"].hasOwnProperty("guideline"));

// Confirm the guideline was renamed
EXPECT_EQ("test_import_new_name", test_import.name);

// Confirm the guideline was imported into the topology
var rgs;
EXPECT_NO_THROWS(function() { rgs = cluster.routingGuidelines(); } );

var row = rgs.fetchOne();
EXPECT_EQ("test_import", row[0]);
row = rgs.fetchOne();
EXPECT_EQ("test_import_new_name", row[0]);

// Backward compatibility tests

//@<> Create a guideline with version 1.0
var guideline_1_0 =
{
    "destinations": [
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        }
    ],
    "name": "guideline_one_oh",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        }
    ],
    "version": "1.0"
}

EXPECT_NO_THROWS(function() { rg_one_oh = cluster.createRoutingGuideline("guideline_one_oh", guideline_1_0);} );

//@<> auto-escape tests

// Key-value types must be auto-escaped only if version is 1.0, for example:
//   - "$.router.tags.key = 'value\" must be stored as
//   "$.router.tags.key = '\"value\"'"

rg_one_oh.addDestination("tags", "$.server.tags.foo = 'bar'")

rg_one_oh.addRoute("routetags", "$.router.tags.key = 'value'", ["round-robin(Secondary)"])

expected_destinations = ["Secondary", "Primary", "tags"];
expected_destinations_match = ["$.server.memberRole = SECONDARY", "$.server.memberRole = PRIMARY", "$.server.tags.foo = 'bar'"];

expected_routes = ["ro", "rw", "routetags"];
expected_routes_match = [
  "$.session.targetPort = $.router.port.ro",
  "$.session.targetPort = $.router.port.rw",
  "$.router.tags.key = 'value'"
];
expected_routes_destinations = [
  "round-robin(Secondary)",
  "round-robin(Primary)",
  "round-robin(Secondary)"
];

expected_rg_asjson =
{
    "destinations": [
        {
            "match": "$.server.memberRole = SECONDARY",
            "name": "Secondary"
        },
        {
            "match": "$.server.memberRole = PRIMARY",
            "name": "Primary"
        },
        {
            "match": "$.server.tags.foo = '\"bar\"'",
            "name": "tags"
        }
    ],
    "name": "guideline_one_oh",
    "routes": [
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.ro",
            "name": "ro"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Primary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.session.targetPort = $.router.port.rw",
            "name": "rw"
        },
        {
            "connectionSharingAllowed": true,
            "destinations": [
                {
                    "classes": [
                        "Secondary"
                    ],
                    "priority": 0,
                    "strategy": "round-robin"
                }
            ],
            "enabled": true,
            "match": "$.router.tags.key = '\"value\"'",
            "name": "routetags"
        }
    ],
    "version": "1.0"
}

expected_show_output = `
Routing Guideline: 'guideline_one_oh'
Cluster: 'cluster'

Routes
------
  - ro
    + Match: "$.session.targetPort = $.router.port.ro"
    + Destinations:
      * None (Secondary)

  - rw
    + Match: "$.session.targetPort = $.router.port.rw"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - routetags
    + Match: "$.router.tags.key = 'value'"
    + Destinations:
      * None (Secondary)

Destination Classes
-------------------
  - Secondary:
    + Match: "$.server.memberRole = SECONDARY"
    + Instances:
      * None

  - Primary:
    + Match: "$.server.memberRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - tags:
    + Match: "$.server.tags.foo = 'bar'"
    + Instances:
      * None

Unreferenced servers
--------------------
  - None
`;

COMMON_RG_TESTS(rg_one_oh, "guideline_one_oh", expected_destinations, expected_destinations_match, expected_routes, expected_routes_match, expected_routes_destinations, expected_rg_asjson, expected_show_output);

EXPECT_NO_THROWS(function(){ rg_one_oh.setDestinationOption("tags", "match", "$.server.tags.foo = 'baz'");} );

EXPECT_EQ("$.server.tags.foo = '\"baz\"'", rg_one_oh.asJson()["destinations"][2]["match"]);

EXPECT_NO_THROWS(function(){ rg_one_oh.setRouteOption("routetags", "match", "$.router.tags.key = 'foobar'");} );

EXPECT_EQ("$.router.tags.key = '\"foobar\"'", rg_one_oh.asJson().routes[2]["match"])

//@<> Cleanup
testutil.rmfile(rg2_path);
testutil.rmfile(test_import_valid);
testutil.rmfile(test_import_invalid);
scene.destroy();
