//@ {VER(>=8.0.27)}

// Tests various positive and negative scenarios for .getClusterSet()
// and .getCluster() on ClusterSets

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], {manualStartOnBoot:1});
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");

rc = cs.createReplicaCluster(__sandbox_uri3, "replica");
rc.addInstance(__sandbox_uri4);

//@<> dba.getClusterSet() from a secondary member of the primary cluster
shell.connect(__sandbox_uri2);

EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_NE(clusterset, null);

//@<> dba.getCluster() on the primary cluster
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() {c = dba.getCluster(); });
EXPECT_NE(c, null);
EXPECT_EQ(c.status()["clusterName"], "cluster");

//@<> dba.getCluster() using the replica cluster name while connected to the primary cluster
EXPECT_NO_THROWS(function() {c = dba.getCluster("replica"); });
EXPECT_NE(c, null);
EXPECT_EQ(c.status()["clusterName"], "replica");

//@<> dba.getCluster() bad options while connected to the primary cluster
EXPECT_THROWS_TYPE(function(){dba.getCluster("foobar")}, "The cluster with the name 'foobar' does not exist.", "MYSQLSH");
EXPECT_THROWS_TYPE(function(){dba.getCluster("")}, "The cluster with the name '' does not exist.", "MYSQLSH");
EXPECT_THROWS_TYPE(function(){dba.getCluster(123)}, " Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){dba.getCluster("foo", 123, "bar")}, "Invalid number of arguments, expected 0 to 2 but got 3", "ArgumentError");

//@<> dba.getCluster() on the replica cluster
shell.connect(__sandbox_uri3);
EXPECT_NO_THROWS(function() {c = dba.getCluster(); });
EXPECT_NE(c, null);
EXPECT_EQ(c.status()["clusterName"], "replica");

//@<> dba.getCluster() using the primary cluster name while connected to the replica cluster
EXPECT_NO_THROWS(function() {c = dba.getCluster("cluster"); });
EXPECT_NE(c, null);
EXPECT_EQ(c.status()["clusterName"], "cluster");

//@<> dba.getCluster() bad options while connected to the replica cluster
EXPECT_THROWS_TYPE(function(){dba.getCluster("foobar")}, "The cluster with the name 'foobar' does not exist.", "MYSQLSH");
EXPECT_THROWS_TYPE(function(){dba.getCluster("")}, "The cluster with the name '' does not exist.", "MYSQLSH");
EXPECT_THROWS_TYPE(function(){dba.getCluster(123)}, " Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){dba.getCluster("foo", 123, "bar")}, "Invalid number of arguments, expected 0 to 2 but got 3", "ArgumentError");


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

//@<> getClusterSet() from a member of the replica cluster after primary instance changes
cluster.setPrimaryInstance(__sandbox_uri2);
WIPE_OUTPUT();
EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_OUTPUT_EMPTY();

cluster.setPrimaryInstance(__sandbox_uri1);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_OUTPUT_EMPTY();

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

// The information should be obtained from the new primary
s = clusterset.status();
EXPECT_EQ("cluster", s["primaryCluster"]);

//@<> cluster.getClusterSet() from an invalidated Cluster must print a warning
EXPECT_NO_THROWS(function() {clusterset = rc_to_invalidate.getClusterSet(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Cluster 'replica_to_invalidate' was INVALIDATED and must be removed from the ClusterSet.");

// The information should be obtained from the new primary
s = clusterset.status();
EXPECT_EQ("cluster", s["primaryCluster"]);

//@<> dba.getCluster() from an instance that belongs to an invalidated Cluster must print a warning
EXPECT_NO_THROWS(function() {invalidate_cluster = dba.getCluster(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Cluster 'replica_to_invalidate' was INVALIDATED and must be removed from the ClusterSet.");

//@<> dba.getCluster(name) from an instance that belongs to an invalidated Cluster must print a warning
EXPECT_NO_THROWS(function() {invalidate_cluster = dba.getCluster("replica_to_invalidate"); });
EXPECT_OUTPUT_CONTAINS("WARNING: Cluster 'replica_to_invalidate' was INVALIDATED and must be removed from the ClusterSet.");

//@<> dba.getCluster(name) from an instance that belongs to the primary Cluster must print a warning
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() {invalidate_cluster = dba.getCluster("replica_to_invalidate"); });
EXPECT_OUTPUT_CONTAINS("WARNING: Cluster 'replica_to_invalidate' was INVALIDATED and must be removed from the ClusterSet.");

// The operation must work even if the PRIMARY cluster is not reachable, as long as
// the target instance is a reachable member of an InnoDB Cluster that is part of a ClusterSet.
testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri3);

//@<> dba.getClusterSet() from a member of the replica cluster with the primary cluster down
EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_NE(clusterset, null);

//@<> dba.getCluster() from a member of the replica cluster with the primary cluster down
EXPECT_NO_THROWS(function() {replica = dba.getCluster(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Could not connect to any member of PRIMARY Cluster 'cluster', topology changes will not be allowed on the InnoDB Cluster 'replica'");
EXPECT_NE(clusterset, null);

// dba.getCluster() detect if the instance to which the Shell is connected to is part of a ClusterSet
// and if so, obtain the most up-to-date Metadata copy to validate if the target cluster is still a
// valid member cluster, otherwise print a warning and recommend the user to remove it.

// Restore the primary cluster
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
dba.rebootClusterFromCompleteOutage("cluster");

// Stop the replication channel on the Replica Cluster
var session3 = mysql.getSession(__sandbox_uri3);
var session4 = mysql.getSession(__sandbox_uri4);

session3.runSql("STOP replica");
session4.runSql("STOP replica");

// Remove the Cluster from the ClusterSet
cs = dba.getClusterSet();
cs.removeCluster("replica", {force: true});

//@<> dba.getCluster() on a Cluster that is no longer a ClusterSet member but its Metadata indicates that is
shell.connect(__sandbox_uri3);
EXPECT_NO_THROWS(function() {replica = dba.getCluster(); });
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster 'replica' appears to have been removed from the ClusterSet 'domain', however its own metadata copy wasn't properly updated during the removal");
EXPECT_NE(replica, null);
EXPECT_EQ(replica.status()["groupInformationSourceMember"], __endpoint3);
replica.status();
EXPECT_EQ(replica.status()["metadataServer"], undefined); // undefined means the same as groupInformationSourceMember

//@<> dba.getClusterSet() on a instance that belongs to a Cluster that is no longer a ClusterSet member but its Metadata indicates that is
EXPECT_THROWS_TYPE(function() {dba.getClusterSet();}, "Cluster is not part of a ClusterSet" , "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster 'replica' appears to have been removed from the ClusterSet 'domain', however its own metadata copy wasn't properly updated during the removal");

//@<> dba.getCluster() on a Cluster member of a ClusterSet that group_replication_group_name does not match the Metadata
shell.connect(__sandbox_uri1);
var bogus_group_name = "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_SET(attributes, '$.group_replication_group_name', ?)", [bogus_group_name]);

EXPECT_THROWS_TYPE(function(){dba.getCluster()}, `Unable to get an InnoDB cluster handle. The instance '${hostname}:${__mysql_sandbox_port1}' may belong to a different cluster from the one registered in the Metadata since the value of 'group_replication_group_name' does not match the one registered in the Metadata: possible split-brain scenario. Please retry while connected to another member of the cluster.`, "RuntimeError");

// Restore the group_replication_group_name value in the Metadata
//var current_group_name = session.runSql("SELECT @@group_replication_group_name").fetchOne()[0];
//session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_SET(attributes, '$.group_replication_group_name', ?)", [current_group_name]);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
