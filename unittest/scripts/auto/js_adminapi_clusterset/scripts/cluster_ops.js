//@ {VER(>=8.0.27)}

// Tests various positive and negative scenarios for InnoDB Cluster operations
// on Clusters that are part of a ClusterSet.

//@<> INCLUDE clusterset_utils.inc

//@<> Setup + Create primary cluster + add Replica Cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");

replicacluster = cs.createReplicaCluster(__sandbox_uri3, "replica");
replicacluster.addInstance(__sandbox_uri4);

CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], cluster, replicacluster);

// Cluster topology changes that affect the replication channel between cluster update the replication channels accordingly:
//   - When the primary instance is removed (of either REPLICA or PRIMARY cluster).
//   - When the primary instance is changed (of either REPLICA or PRIMARY cluster).
//   - When the quorum is forced (of REPLICA cluster).
//   - When the cluster is rebooted from complete outage.

//@<> Removing the primary instance of the REPLICA Cluster must ensure the replication stream is kept and reset all CS settings
replicacluster.removeInstance(__sandbox_uri3);

shell.connect(__sandbox_uri4);
replicacluster = dba.getCluster();

CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], cluster);
CHECK_REMOVED_INSTANCE(__sandbox_uri3);

//@<> Removing the primary instance of the PRIMARY Cluster must ensure the replication stream is kept and reset all CS settings
cluster.removeInstance(__sandbox_uri1);
CHECK_REMOVED_INSTANCE(__sandbox_uri1);

shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

shell.connect(__sandbox_uri4);
replicacluster = dba.getCluster();

ensure_cs_replication_channel_ready(__sandbox_uri2, __mysql_sandbox_port2);

CHECK_PRIMARY_CLUSTER([__sandbox_uri2], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);

// Add back the instances to the clusters
cluster.addInstance(__sandbox_uri1);

// Test setting super_read_only to false in the removed member and add it back to verify it's enabled back
var session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("SET GLOBAL super_read_only=0");

replicacluster.addInstance(__sandbox_uri3);

CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> Changing the primary instance of the PRIMARY Cluster must ensure the replication stream is kept
cluster.setPrimaryInstance(__sandbox_uri1);

shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

shell.connect(__sandbox_uri4);
replicacluster = dba.getCluster();

ensure_cs_replication_channel_ready(__sandbox_uri4, __mysql_sandbox_port1);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> Changing the primary instance of the REPLICA Cluster must ensure the replication stream is kept
replicacluster.setPrimaryInstance(__sandbox_uri3);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], cluster, replicacluster);

//@<> rejoinInstance on a replica cluster
shell.connect(__sandbox_uri3);
replicacluster = dba.getCluster();

var session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("STOP group_replication");

// Disable skip_replica_start to verify that rejoin enables it back
session4.runSql("SET PERSIST_ONLY skip_replica_start=0");
// Disable start_on_boot
session4.runSql("SET PERSIST_ONLY group_replication_start_on_boot=0");
testutil.restartSandbox(__mysql_sandbox_port4);

EXPECT_NO_THROWS(function() { replicacluster.rejoinInstance(__sandbox_uri4); });

testutil.waitMemberState(__mysql_sandbox_port4, "ONLINE");

CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], cluster, replicacluster);
CHECK_REJOINED_INSTANCE(__sandbox_uri4, cluster, false);

// GTID sync operations must use the primary instance of the PRIMARY cluster as the source, the sync happens in:
//   - cluster.removeInstance()

//@<> Verify that a transaction sync happens before the removal of the instance in a REPLICA cluster
shell.connect(__sandbox_uri1);
var session4 = mysql.getSession(__sandbox_uri4);

// Remove instance from REPLICA Cluster
replicacluster.removeInstance(__sandbox_uri4);
CHECK_GTID_CONSISTENT(session, session4);
CHECK_REPLICA_CLUSTER([__sandbox_uri3], cluster, replicacluster);
CHECK_REMOVED_INSTANCE(__sandbox_uri4);

// Add back the instance to the cluster
replicacluster.addInstance(__sandbox_uri4, {recoveryMethod: "clone"});
var session4 = mysql.getSession(__sandbox_uri4);

//@<> Remove instance and add it back as a new replica cluster via incremental {false}
replicacluster.removeInstance(__sandbox_uri4);

nc = cs.createReplicaCluster(__sandbox_uri4, "newcluster", {recoveryMethod:"incremental"});

//@<> Put it back {false}
cs.removeCluster("newcluster");

// session4.runSql("stop group_replication");

replicacluster.addInstance(__sandbox_uri4);


//@<> Remove instance from cluster and add to another one
// the instance will have view change GTIDs from the old cluster that don't exist in the primary
replicacluster.removeInstance(__sandbox_uri4);

c3 = cs.createReplicaCluster(__sandbox_uri5, "cluster3", {recoveryMethod:"incremental"});
c3.addInstance(__sandbox_uri4);

c3.removeInstance(__sandbox_uri4);

// <Cluster>.rejoinInstance() must verify the value of skip_slave_start and enabled it if necessary and if the cluster belongs to a ClusterSet

//@<> rejoinInstance on a primary cluster
var session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("STOP group_replication");

// Disable skip_replica_start to verify that rejoin enables it back
session2.runSql("SET PERSIST_ONLY skip_replica_start=0");
// Disable start_on_boot
session2.runSql("SET PERSIST_ONLY group_replication_start_on_boot=0");
testutil.restartSandbox(__mysql_sandbox_port2);

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], cluster);
CHECK_REJOINED_INSTANCE(__sandbox_uri2);

//@<> createCluster() must generate and set a value for group_replication_view_change_uuid
var session4 = mysql.getSession(__sandbox_uri4);
reset_instance(session4);
testutil.stopSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri4);
dba.createCluster("newcluster");

var view_change_uuid = session.runSql("select @@global.group_replication_view_change_uuid").fetchOne()[0];
EXPECT_NE(null, view_change_uuid);

var view_change_uuid_md = session.runSql("select (attributes->>'$.group_replication_view_change_uuid') from mysql_innodb_cluster_metadata.clusters where cluster_name='newcluster'").fetchOne()[0];
EXPECT_EQ(view_change_uuid_md, view_change_uuid);

//@<> cluster.rescan() must detect if the target cluster does not have group_replication_view_change_uuid set

// Reset the value of view_change_uuid to the default
session.runSql("SET PERSIST group_replication_view_change_uuid=DEFAULT");

// Clear up the info about view_change_uuid in the Metadata
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = json_remove(attributes, '$.group_replication_view_change_uuid')");

// Restart the instance and reboot it from complete outage
testutil.restartSandbox(__mysql_sandbox_port4);
cluster = dba.rebootClusterFromCompleteOutage();

EXPECT_NO_THROWS(function() { cluster.rescan(); });
EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is not set");
EXPECT_OUTPUT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
EXPECT_OUTPUT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");

// Do not restart yet and confirm that cluster.rescan() detects a restart is needed for the change to be effective
EXPECT_NO_THROWS(function() { cluster.rescan(); });
EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is set but not yet effective");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");

// Restart the instance and reboot it from complete outage after rescan() changed group_replication_view_change_uuid
testutil.restartSandbox(__mysql_sandbox_port4);
cluster = dba.rebootClusterFromCompleteOutage();

shell.connect(__sandbox_uri4);
var view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
EXPECT_NE(view_change_uuid, "AUTOMATIC");

var view_change_uuid_md = session.runSql("select (attributes->>'$.group_replication_view_change_uuid') from mysql_innodb_cluster_metadata.clusters where cluster_name='newcluster'").fetchOne()[0];
EXPECT_EQ(view_change_uuid, view_change_uuid_md);

//@<> cluster.rescan() must not warn or act if the target cluster has group_replication_view_change_uuid set
EXPECT_NO_THROWS(function() { cluster.rescan(); });
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is not set");
EXPECT_OUTPUT_NOT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
EXPECT_OUTPUT_NOT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
