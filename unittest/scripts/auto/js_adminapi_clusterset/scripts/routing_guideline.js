//@<> INCLUDE routing_guideline_tests.inc

//@<> Setup + Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1]);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
var session = scene.session
var cluster = scene.cluster

//@<> createRoutingGuideline()
var rg;
EXPECT_NO_THROWS(function() { rg = cluster.createRoutingGuideline("default_cluster_rg");} );

//@<> upgrade the Cluster to ClusterSet - check default RG upgrade
EXPECT_NO_THROWS(function() { clusterset = cluster.createClusterSet("cs");} );

EXPECT_OUTPUT_CONTAINS(`* Upgrading Default Routing Guideline 'default_cluster_rg'...`);

//@<> The Routing Guideline object created on the Cluster must be invalidated when the Cluster becomes a ClusterSet
EXPECT_THROWS(function(){ rg.show(); }, "Invalid Routing Guideline object");
EXPECT_OUTPUT_CONTAINS(`ERROR: The Routing Guideline object was created when the Cluster was not part of a ClusterSet. Since the Cluster now belongs to a ClusterSet, this object is invalid. Please obtain a new Routing Guideline object using <ClusterSet>.getRoutingGuideline().`);

const expected_default_clusterset_guideline_show_output = `
Routing Guideline: 'default_cluster_rg'
ClusterSet: 'cs'

Routes
------
  - rw
    + Match: "$.session.targetPort = $.router.port.rw"
    + Destinations:
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

  - ro
    + Match: "$.session.targetPort = $.router.port.ro"
    + Destinations:
      * None (PrimaryClusterSecondary)
      * ${hostname}:${__mysql_sandbox_port1} (Primary)

Destination Classes
-------------------
  - Primary:
    + Match: "$.server.memberRole = PRIMARY AND $.server.clusterRole = PRIMARY"
    + Instances:
      * ${hostname}:${__mysql_sandbox_port1}

  - PrimaryClusterSecondary:
    + Match: "$.server.memberRole = SECONDARY AND $.server.clusterRole = PRIMARY"
    + Instances:
      * None

  - PrimaryClusterReadReplica:
    + Match: "$.server.memberRole = READ_REPLICA AND $.server.clusterRole = PRIMARY"
    + Instances:
      * None

Unreferenced servers
--------------------
  - None
`;

//@<> setRoutingOption(guideline) - global OK
EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("guideline", "default_cluster_rg");} );

EXPECT_OUTPUT_CONTAINS(`Routing option 'guideline' successfully updated.`);
EXPECT_OUTPUT_CONTAINS(`Routing Guideline 'default_cluster_rg' has been enabled and is now the active guideline for the topology.`);

rg = clusterset.getRoutingGuideline();
COMMON_RG_TESTS(rg, "default_cluster_rg", expected_default_clusterset_guideline_destinations, expected_default_clusterset_guideline_destinations_match, expected_default_clusterset_guideline_routes, expected_default_clusterset_guideline_routes_match, expected_default_clusterset_guideline_routes_destinations, default_clusterset_guideline, expected_default_clusterset_guideline_show_output);

// Confirm the guideline is the active one
var router_options = clusterset.routerOptions();
EXPECT_EQ("default_cluster_rg", router_options["configuration"]["routing_rules"]["guideline"]);

// All management operations for Routing Guidelines on a Cluster that is a member of a ClusterSet must be blocked. The user must be informed to perform the operation using the parent ClusterSet object instead

//@<> cluster.createRoutingGuideline not available for clusterset members
EXPECT_THROWS(function(){ cluster.createRoutingGuideline("test"); }, "Function not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of a ClusterSet, use <ClusterSet>.createRoutingGuideline() instead.");

//@<> cluster.getRoutingGuideline not available for clusterset members
EXPECT_THROWS(function(){ cluster.getRoutingGuideline("test"); }, "Function not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of a ClusterSet, use <ClusterSet>.getRoutingGuideline() instead.");

//@<> cluster.routingGuidelines not available for clusterset members
EXPECT_THROWS(function(){ cluster.routingGuidelines(); }, "Function not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of a ClusterSet, use <ClusterSet>.routingGuidelines() instead.");

//@<> cluster.removeRoutingGuideline not available for clusterset members
EXPECT_THROWS(function(){ cluster.removeRoutingGuideline("test"); }, "Function not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of a ClusterSet, use <ClusterSet>.removeRoutingGuideline() instead.");

//@<> cluster.importRoutingGuideline not available for clusterset members
EXPECT_THROWS(function(){ cluster.importRoutingGuideline("test"); }, "Function not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of a ClusterSet, use <ClusterSet>.importRoutingGuideline() instead.");

// Switchover and failover tests - Ensure routing guidelines ops aren't affected

EXPECT_NO_THROWS(function() { rc = clusterset.createReplicaCluster(__sandbox_uri2, "replica");} );

//@<> cluster.getRoutingGuideline not available for clusterset members (replica cluster)
EXPECT_THROWS(function(){ rc.getRoutingGuideline("test"); }, "Function not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'replica' is a member of a ClusterSet, use <ClusterSet>.getRoutingGuideline() instead.");

//@<> routing guideline management ops should be available after switchover
EXPECT_NO_THROWS(function() { clusterset.setPrimaryCluster("replica"); } );
EXPECT_NO_THROWS(function() { rg.addDestination("test_d1", "$.server.memberRole = PRIMARY"); })
EXPECT_NO_THROWS(function() { rg.addRoute("test1", "$.session.targetPort in (6446, 6448)", ["first-available(Primary)"]); })

var routes;
EXPECT_NO_THROWS(function() { routes = rg.routes().fetchAll(); });
EXPECT_JSON_EQ(["rw", "ro","test1"], group_fields(routes, 0), ".expected_routes");

var destinations;
EXPECT_NO_THROWS(function() { destinations = rg.destinations().fetchAll(); });
EXPECT_JSON_EQ([
  "Primary",
  "PrimaryClusterSecondary",
  "PrimaryClusterReadReplica",
  "test_d1"
], group_fields(destinations, 0), ".expected_destinations");

//@<> routing guideline management ops should be available after failover

// Make the PRIMARY cluster unavailable
testutil.killSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
clusterset = dba.getClusterSet();

EXPECT_NO_THROWS(function() { clusterset.forcePrimaryCluster("cluster"); } );

EXPECT_NO_THROWS(function() { rg.addDestination("test_d2", "$.server.memberRole = PRIMARY"); })
EXPECT_NO_THROWS(function() { rg.addRoute("test2", "$.session.targetPort in (6446, 6448)", ["first-available(Primary)"]); })

var routes;
EXPECT_NO_THROWS(function() { routes = rg.routes().fetchAll(); });
EXPECT_JSON_EQ(["rw", "ro", "test1", "test2"], group_fields(routes, 0), ".expected_routes");

var destinations;
EXPECT_NO_THROWS(function() { destinations = rg.destinations().fetchAll(); });
EXPECT_JSON_EQ([
  "Primary",
  "PrimaryClusterSecondary",
  "PrimaryClusterReadReplica",
  "test_d1",
  "test_d2"
], group_fields(destinations, 0), ".expected_destinations");

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);
