// Preconditions checks regarding Routing Guidelines

function restore_quorum() {
  cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);
  testutil.startSandbox(__mysql_sandbox_port2);
  cluster.rejoinInstance(__sandbox_uri2);
  testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
  testutil.startSandbox(__mysql_sandbox_port3);
  cluster.rejoinInstance(__sandbox_uri3);
  testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
}

//@<> Setup + Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var session = scene.session
var cluster = scene.cluster

//@<> createRoutingGuideline() is blocked when there's no quorum
scene.make_no_quorum([__mysql_sandbox_port1]);

EXPECT_THROWS(function(){ cluster.createRoutingGuideline("foo"); }, "There is no quorum to perform the operation");

//@<> removeRoutingGuideline() is blocked when there's no quorum
EXPECT_THROWS(function(){ cluster.removeRoutingGuideline("foo"); }, "There is no quorum to perform the operation");

//@<> importRoutingGuideline() is blocked when there's no quorum
EXPECT_THROWS(function(){ cluster.importRoutingGuideline("foo"); }, "There is no quorum to perform the operation");

// Restore quorum
restore_quorum();

//@<> createRoutingGuideline() is NOT blocked when there's quorum
EXPECT_NO_THROWS(function(){ cluster.createRoutingGuideline("foo");});

// Loose quorum again
scene.make_no_quorum([__mysql_sandbox_port1]);

//@<> getRoutingGuideline() is NOT blocked when there's no quorum
var rg;
EXPECT_NO_THROWS(function(){ rg = cluster.getRoutingGuideline("foo"); });

//@<> routingGuidelines() is NOT blocked when there's no quorum
EXPECT_NO_THROWS(function(){ cluster.routingGuidelines(); });

//@<> routingguideline.rename() is blocked when there's no quorum
EXPECT_THROWS(function(){ rg.rename("bar"); }, "There is no quorum to perform the operation");

//@<> routingguideline.addDestination() is blocked when there's no quorum
EXPECT_THROWS(function(){ rg.addDestination("test1", "$.server.memberRole = PRIMARY"); }, "There is no quorum to perform the operation");

//@<> routingguideline.removeDestination() is blocked when there's no quorum
EXPECT_THROWS(function(){ rg.removeDestination("test1"); }, "There is no quorum to perform the operation");

//@<> routingguideline.setDestinationOption() is blocked when there's no quorum
EXPECT_THROWS(function(){ rg.setDestinationOption("test1", "match", "$.server.memberRole = PRIMARY"); }, "There is no quorum to perform the operation");

//@<> routingguideline.addRoute() is blocked when there's no quorum
EXPECT_THROWS(function(){ rg.addRoute("test1", "$.session.targetPort in (6446, 6448)", ["first-available(Primary)"]); }, "There is no quorum to perform the operation");

//@<> routingguideline.removeRoute() is blocked when there's no quorum
EXPECT_THROWS(function(){ rg.removeRoute("test1"); }, "There is no quorum to perform the operation");

//@<> routingguideline.setRouteOption() is blocked when there's no quorum
EXPECT_THROWS(function(){ rg.setRouteOption("ro", "match", "$.router.hostname = 'test'"); }, "There is no quorum to perform the operation");

//@<> routingguideline.copy() is blocked when there's no quorum
EXPECT_THROWS(function(){ rg2 = rg.copy("imacopy"); }, "There is no quorum to perform the operation");

//@<> destinations() is NOT blocked when there's no quorum
EXPECT_NO_THROWS(function(){ rg.destinations();});

//@<> routes() is NOT blocked when there's no quorum
EXPECT_NO_THROWS(function(){ rg.routes();});

//@<> show() is NOT blocked when there's no quorum
EXPECT_NO_THROWS(function(){ rg.show();});

//@<> as_json() is NOT blocked when there's no quorum
EXPECT_NO_THROWS(function(){ rg.asJson();});

//@<> export() is NOT blocked when there's no quorum
EXPECT_NO_THROWS(function(){ rg.export("rg2.json");});

// ClusterSet scenarios

// Restore quorum
restore_quorum();

cluster.removeInstance(__sandbox_uri3);

clusterset = cluster.createClusterSet("cs");
replica_cluster = clusterset.createReplicaCluster(__sandbox_uri3, "replica_cluster");

//@<> clusterset.createRoutingGuideline() is blocked when the primary is unavailable
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri3);
replica_cluster = dba.getCluster();

EXPECT_THROWS(function(){ replica_cluster.createRoutingGuideline("foo"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> clusterset.removeRoutingGuideline() is blocked when the primary is unavailable
EXPECT_THROWS(function(){ replica_cluster.removeRoutingGuideline("foo"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> clusterset.importRoutingGuideline() is blocked when the primary is unavailable
EXPECT_THROWS(function(){ replica_cluster.importRoutingGuideline("foo"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

// Restore primary
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage()
clusterset = dba.getClusterSet();

// Stop the Replica's channel, to ensure that even if the Replica isn't replicating, the following scenarios are OK
shell.connect(__sandbox_uri3);
session.runSql("stop replica for channel 'clusterset_replication'");

//@<> clusterset.createRoutingGuideline() is NOT blocked when there's quorum at the primary and replica clusters are not replicating
var rg2;
EXPECT_NO_THROWS(function(){ rg2 = clusterset.createRoutingGuideline("bar");});

// Loose the primary again
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri3);
clusterset = dba.getClusterSet();

//@<> getRoutingGuideline() is NOT blocked when primary is not available (clusterset)
var rg;
EXPECT_NO_THROWS(function(){ rg2 = clusterset.getRoutingGuideline("foo"); });

//@<> routingGuidelines() is NOT blocked when primary is not available (clusterset)
EXPECT_NO_THROWS(function(){ clusterset.routingGuidelines(); });

//@<> routingguideline.rename() is blocked when primary is not available (clusterset)
EXPECT_THROWS(function(){ rg2.rename("bar"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> routingguideline.addDestination() is blocked when primary is not available (clusterset)
EXPECT_THROWS(function(){ rg2.addDestination("test1", "$.server.memberRole = PRIMARY"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> routingguideline.removeDestination() is blocked when primary is not available (clusterset)
EXPECT_THROWS(function(){ rg2.removeDestination("test1"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> routingguideline.setDestinationOption() is blocked when primary is not available (clusterset)
EXPECT_THROWS(function(){ rg2.setDestinationOption("test1", "match", "$.server.memberRole = PRIMARY"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> routingguideline.addRoute() is blocked when primary is not available (clusterset)
EXPECT_THROWS(function(){ rg2.addRoute("test1", "$.session.targetPort in (6446, 6448)", ["first-available(Primary)"]); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> routingguideline.removeRoute() is blocked when primary is not available (clusterset)
EXPECT_THROWS(function(){ rg2.removeRoute("test1"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> routingguideline.setRouteOption() is blocked when primary is not available (clusterset)
EXPECT_THROWS(function(){ rg2.setRouteOption("ro", "match", "$.router.hostname = 'test'"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> routingguideline.copy() is blocked when primary is not available
EXPECT_THROWS(function(){ rg3 = rg2.copy("imacopy"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: Could not connect to any ONLINE member of Primary Cluster.");

//@<> destinations() is NOT blocked when primary is not available (clusterset)
EXPECT_NO_THROWS(function(){ rg2.destinations();});

//@<> routes() is NOT blocked when primary is not available (clusterset)
EXPECT_NO_THROWS(function(){ rg2.routes();});

//@<> show() is NOT blocked when primary is not available (clusterset)
EXPECT_NO_THROWS(function(){ rg2.show();});

//@<> as_json() is NOT blocked when primary is not available (clusterset)
EXPECT_NO_THROWS(function(){ rg2.asJson();});

//@<> export() is NOT blocked when primary is not available (clusterset)
EXPECT_NO_THROWS(function(){ rg2.export("rg3.json");});

//@<> Cleanup
scene.destroy();
testutil.rmfile("rg2.json");
testutil.rmfile("rg3.json");
