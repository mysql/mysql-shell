//@ {VER(>=8.0.27)}

// Tests all preconditions checks regarding ClusterSet

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");
cs.createReplicaCluster(__sandbox_uri2, "replica");

// I4: Cluster topology changes are allowed only if the PRIMARY Cluster is available.

// Make the PRIMARY cluster unavailable
testutil.killSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
rc = dba.getCluster();

//@<> addInstance on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.addInstance(__sandbox_uri3); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> removeInstance on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.removeInstance(__sandbox_uri3); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> rejoinInstance on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.rejoinInstance(__sandbox_uri3); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> rescan on REPLICA Cluster with PRIMARY unavailable
EXPECT_THROWS(function(){ rc.rescan(); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

// ClusterSet topology changes are allowed only if the PRIMARY Cluster is available:
// - createReplicaCluster()
// - removeCluster()
// - TODO: setPrimaryCluster()
// - TODO: rejoinCluster()

//@<> createReplicaCluster() with PRIMARY unavailable
cs = dba.getClusterSet();
EXPECT_THROWS(function(){ cs.createReplicaCluster(__sandbox_uri3, "replica2"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> removeCluster() with PRIMARY unavailable
EXPECT_THROWS(function(){ cs.removeCluster("replica"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
