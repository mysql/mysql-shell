//@ {VER(>=8.0.27)}

// Tests various positive and negative scenarios for .getClusterSet()

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");

rc = cs.createReplicaCluster(__sandbox_uri3, "replica");
//rc.addInstance(__sandbox_uri4);

//@<> dba.getClusterSet() from a secondary member of the primary cluster
shell.connect(__sandbox_uri2);

EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_NE(clusterset, null);

//@<> dba.getCluster() on the primary cluster
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() {c = dba.getCluster(); });
EXPECT_NE(c, null);
EXPECT_EQ(c.status()["clusterName"], "cluster");

//@<> dba.getCluster() on the replica cluster
shell.connect(__sandbox_uri3);
EXPECT_NO_THROWS(function() {c = dba.getCluster(); });
EXPECT_NE(c, null);
EXPECT_EQ(c.status()["clusterName"], "replica");

//@<> dba.getCluster("replica")
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() {c = dba.getCluster("replica"); });
EXPECT_NE(c, null);
EXPECT_EQ(c.status()["clusterName"], "replica");

//@<> dba.getCluster("cluster")
shell.connect(__sandbox_uri3);
EXPECT_NO_THROWS(function() {c = dba.getCluster("cluster"); });
EXPECT_NE(c, null);
EXPECT_EQ(c.status()["clusterName"], "cluster");

//TODO(miguel): check the Clusterset.status output

//@<> Cluster.getClusterSet() from a secondary member of the primary cluster
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

EXPECT_NO_THROWS(function() {clusterset = cluster.getClusterSet(); });
EXPECT_NE(clusterset, null);
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> cluster.status() should indicate the right metadata server
var status = cluster.status();
EXPECT_EQ(status["groupInformationSourceMember"], __endpoint1);
// The metadataServer is the same as groupInformationSourceMember so it's not shown
EXPECT_EQ(status["metadataServer"], undefined);

//@<> dba.getClusterSet() from a member of the replica cluster
shell.connect(__sandbox_uri3);

EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_NE(clusterset, null);
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//TODO(miguel): check the Clusterset.status output

//@<> Cluster.getClusterSet() from a member of the replica cluster
rc = dba.getCluster();

EXPECT_NO_THROWS(function() {clusterset = rc.getClusterSet(); });
EXPECT_NE(clusterset, null);
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");

//@<> replica_cluster.status() should indicate the right metadata server
var status = rc.status();
EXPECT_EQ(status["groupInformationSourceMember"], __endpoint3);
EXPECT_EQ(status["metadataServer"], __endpoint1);

//@<> dba.getClusterSet() and dba.getCluster() from a member of the replica cluster with primary cluster NO_QUORUM
shell.connect(__sandbox_uri1);
scene.make_no_quorum([__mysql_sandbox_port1]);

shell.connect(__sandbox_uri3);
EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_OUTPUT_CONTAINS("WARNING: The PRIMARY Cluster lost the quorum, topology changes will not be allowed");
EXPECT_OUTPUT_CONTAINS("NOTE: To restore the Cluster and ClusterSet operations, restore the quorum on the PRIMARY Cluster using forceQuorumUsingPartitionOf()");
EXPECT_NE(clusterset, null);

EXPECT_NO_THROWS(function() {replica = dba.getCluster(); });
EXPECT_OUTPUT_CONTAINS("The PRIMARY Cluster 'cluster' lost the quorum, topology changes will not be allowed on the InnoDB Cluster 'replica'");
EXPECT_OUTPUT_CONTAINS("NOTE: To restore the Cluster and ClusterSet operations, restore the quorum on the PRIMARY Cluster using forceQuorumUsingPartitionOf()");
EXPECT_NE(replica, null);

//@<> dba.getClusterSet() and dba.getCluster() from a member of the replica cluster with primary cluster offline

// Shutdown the whole primary cluster
var session1 = mysql.getSession(__sandbox_uri1);
session1.runSql("STOP group_replication");

EXPECT_NO_THROWS(function() {replica = dba.getCluster(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of PRIMARY Cluster 'cluster', topology changes will not be allowed on the InnoDB Cluster 'replica'");
EXPECT_NE(replica, null);

EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of PRIMARY Cluster 'cluster', topology changes will not be allowed on the InnoDB Cluster 'replica'");

EXPECT_NE(clusterset, null);

// Restore the primary cluster
shell.connect(__sandbox_uri1);
dba.rebootClusterFromCompleteOutage("cluster");

//@<> dba.getClusterSet() from an invalidated Cluster must print a warning
rc_to_invalidate = cs.createReplicaCluster(__sandbox_uri5, "replica_to_invalidate");
invalidate_cluster(rc_to_invalidate, cluster);
shell.connect(__sandbox_uri5);
EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Cluster 'replica_to_invalidate' was INVALIDATED and must be removed from the ClusterSet.");

//@<> cluster.getClusterSet() from an invalidated Cluster must print a warning
EXPECT_NO_THROWS(function() {clusterset = rc_to_invalidate.getClusterSet(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Cluster 'replica_to_invalidate' was INVALIDATED and must be removed from the ClusterSet.");

// The operation must work even if the PRIMARY cluster is not reachable, as long as
// the target instance is a reachable member of an InnoDB Cluster that is part of a ClusterSet.
testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri3);

//@<> dba.getClusterSet() from a member of the replica cluster with the primary cluster down
EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of the PRIMARY Cluster, topology changes will not be allowed");
EXPECT_NE(clusterset, null);

//@<> dba.getCluster() from a member of the replica cluster with the primary cluster down
EXPECT_NO_THROWS(function() {replica = dba.getCluster(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of PRIMARY Cluster 'cluster', topology changes will not be allowed on the InnoDB Cluster 'replica'");
EXPECT_NE(clusterset, null);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
