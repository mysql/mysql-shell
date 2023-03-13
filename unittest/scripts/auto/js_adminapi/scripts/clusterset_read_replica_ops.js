//@ {VER(>=8.0.27)}

//@<> INCLUDE read_replicas_utils.inc

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port6, "root", {report_host: hostname});
var cluster = scene.cluster

// Create a ClusterSet + 1 Replica Cluster
clusterset = cluster.createClusterSet("cs");
replica_cluster = clusterset.createReplicaCluster(__sandbox_uri3, "cluster2");
replica_cluster.addInstance(__sandbox_uri4);

// Attach a read-replica to the primary cluster
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint5); });

CHECK_READ_REPLICA(__sandbox_uri5, cluster, "primary", __endpoint1);

// Attach a read-replica to the replica cluster
EXPECT_NO_THROWS(function() { replica_cluster.addReplicaInstance(__endpoint6, {replicationSources: "secondary"}); });

CHECK_READ_REPLICA(__sandbox_uri6, replica_cluster, "secondary", __endpoint4);

//@<> Rejoin to a clusterset a replica cluster with errors and read-replicas attached
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP REPLICA FOR CHANNEL 'clusterset_replication'");

var session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("STOP group_replication");

var status = clusterset.status();
EXPECT_TRUE("clusterErrors" in status["clusters"]["cluster2"]);
EXPECT_EQ("WARNING: Replication from the Primary Cluster not in expected state", status["clusters"]["cluster2"]["clusterErrors"][0]);
EXPECT_TRUE("statusText" in status["clusters"]["cluster2"]);
EXPECT_EQ("Cluster is NOT tolerant to any failures. 1 member is not active.", status["clusters"]["cluster2"]["statusText"]);

EXPECT_NO_THROWS(function() { clusterset.rejoinCluster("cluster2"); });

status = clusterset.status();
EXPECT_FALSE("clusterErrors" in status["clusters"]["cluster2"]);
EXPECT_FALSE("statusText" in status["clusters"]["cluster2"]);

EXPECT_NO_THROWS(function() { replica_cluster.rejoinInstance(__sandbox_uri4); });

//@<> Rejoining a read-replica to a replica cluster should succeed under high load

// simulate lag by introducing delay in the replication channel with
// SOURCE_DELAY
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP REPLICA FOR CHANNEL 'clusterset_replication'");
session3.runSql("CHANGE REPLICATION SOURCE TO SOURCE_DELAY=5 FOR CHANNEL 'clusterset_replication'");
session3.runSql("START REPLICA FOR CHANNEL 'clusterset_replication'");

// Stop the read-replica
var session6 = mysql.getSession(__sandbox_uri6);
session6.runSql("stop replica");

// Rejoin it back to the cluster
EXPECT_NO_THROWS(function() { replica_cluster.rejoinInstance(__endpoint6); });

//@<> Adding a read-replica to a replica cluster should succeed under high load
EXPECT_NO_THROWS(function() { replica_cluster.removeInstance(__endpoint6); });

CHECK_REMOVED_READ_REPLICA(__sandbox_uri6, replica_cluster);

EXPECT_NO_THROWS(function() { replica_cluster.addReplicaInstance(__endpoint6, {replicationSources: "secondary"}); });

CHECK_READ_REPLICA(__sandbox_uri6, replica_cluster, "secondary", __endpoint4);

// Remove the delay
//session3.runSql("STOP REPLICA FOR CHANNEL 'clusterset_replication'");
//session3.runSql("CHANGE REPLICATION SOURCE TO SOURCE_DELAY=0 FOR CHANNEL 'clusterset_replication'");
//session3.runSql("START REPLICA FOR CHANNEL 'clusterset_replication'");

//@<> Rejoining a read-replica to a replica cluster should succeed even if the channel was reset
session6.runSql("stop replica");
session6.runSql("reset replica all");

EXPECT_NO_THROWS(function() { replica_cluster.rejoinInstance(__endpoint6, {recoveryMethod: "clone"}); });

//@<> Removing a broken read-replica from a replica cluster that was promoted to primary should succeed
var session6 = mysql.getSession(__sandbox_uri6);
session6.runSql("stop replica");
session6.runSql("reset replica all");

EXPECT_NO_THROWS(function() { clusterset.setPrimaryCluster("cluster2"); });

EXPECT_THROWS_TYPE(function() { replica_cluster.removeInstance(__endpoint6); }, "Read-Replica Replication channel does not exist", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: The Read-Replica Replication channel could not be found. Use the 'force' option to ignore this check.`);

EXPECT_NO_THROWS(function() { replica_cluster.removeInstance(__endpoint6, {force: true}); });

CHECK_REMOVED_READ_REPLICA(__sandbox_uri6, replica_cluster);

//@<> Removing a read-replica should be forbidden if the replica does not belong to the target Cluster (in ClusterSet)

// Restore ClusterSet
reset_instance(session6);

EXPECT_NO_THROWS(function() { replica_cluster.addReplicaInstance(__endpoint6); });

EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__endpoint6); }, "Instance does not belong to the Cluster", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${__endpoint6}' does not belong to the Cluster 'cluster'.`);

EXPECT_THROWS_TYPE(function() { replica_cluster.removeInstance(__endpoint5); }, "Instance does not belong to the Cluster", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${__endpoint5}' does not belong to the Cluster 'cluster2'.`);

EXPECT_NO_THROWS(function() { replica_cluster.removeInstance(__endpoint6); });

EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint5); });

//@<> Adding a read-replica to a cluster that already belongs to another cluster of the ClusterSet should error-out
EXPECT_THROWS_TYPE(function() { replica_cluster.addReplicaInstance(__endpoint3); }, "Target instance already part of this InnoDB Cluster", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${__endpoint3}' is already part of this Cluster. A new Read-Replica must be created on a standalone instance.`);

EXPECT_THROWS_TYPE(function() { replica_cluster.addReplicaInstance(__endpoint1); }, "Target instance already part of an InnoDB Cluster", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${__endpoint1}' is already part of a Cluster of the ClusterSet. A new Read-Replica must be created on a standalone instance.`);

//@<> Clusterset.status(extended:1) should include read-replicas
EXPECT_NO_THROWS(function() { replica_cluster.addReplicaInstance(__endpoint5); });

CHECK_READ_REPLICA(__sandbox_uri5, replica_cluster, "primary", __endpoint3);

status = clusterset.status({extended:1});
print(status);

read_replica1 = status["clusters"]["cluster2"]["topology"][__endpoint3]["readReplicas"][__endpoint5]

EXPECT_EQ(__endpoint5, read_replica1["address"]);
EXPECT_EQ("READ_REPLICA", read_replica1["role"]);
EXPECT_EQ("ONLINE", read_replica1["status"]);
EXPECT_EQ(__version, read_replica1["version"]);

//@<> Switchover should not affect read-replicas nor status
EXPECT_NO_THROWS(function() { clusterset.setPrimaryCluster("cluster"); });

EXPECT_NO_THROWS(function() { replica_cluster.addInstance(__endpoint6); });

status = clusterset.status({extended:1});
print(status);

read_replica1 = status["clusters"]["cluster2"]["topology"][__endpoint3]["readReplicas"][__endpoint5]

EXPECT_EQ(__endpoint5, read_replica1["address"]);
EXPECT_EQ("READ_REPLICA", read_replica1["role"]);
EXPECT_EQ("ONLINE", read_replica1["status"]);
EXPECT_EQ(__version, read_replica1["version"]);

//@<> rebootClusterFromCompleteOutage() should not auto-rejoin read-replicas
EXPECT_NO_THROWS(function() { replica_cluster.fenceAllTraffic(); });

shell.connect(__sandbox_uri3);

EXPECT_NO_THROWS(function() { replica_cluster = dba.rebootClusterFromCompleteOutage(); });

status = clusterset.status({extended:1});
print(status);

read_replica1 = status["clusters"]["cluster2"]["topology"][__endpoint3]["readReplicas"][__endpoint5]

EXPECT_EQ(__endpoint5, read_replica1["address"]);
EXPECT_EQ("OFFLINE", read_replica1["status"]);

//@<> rejoinInstance() should clear offline_mode in read-replicas
EXPECT_NO_THROWS(function() { replica_cluster.rejoinInstance(__endpoint5); });

session5 = mysql.getSession(__sandbox_uri5);

EXPECT_EQ(0, session5.runSql("select @@offline_mode").fetchOne()[0]);

//@<> getCluster() while connected to a read-replica of a Replica Cluster should return that cluster and not any other
shell.connect(__sandbox_uri5);

EXPECT_NO_THROWS(function() { c = dba.getCluster(); });

var status = c.status();

EXPECT_EQ("cluster2", status["clusterName"]);
EXPECT_EQ("REPLICA", status["clusterRole"]);

scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
