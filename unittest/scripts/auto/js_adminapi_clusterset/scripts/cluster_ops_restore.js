//@ {VER(>=8.0.27)}

// Tests various positive and negative scenarios for InnoDB Cluster restore operations
// on Clusters that are part of a ClusterSet:
//   - dba.rebootClusterFromCompleteOutage()
//   - <Cluster>.forceQuorumUsingPartitionOf()

function force_quorum_loss(ports) {
  for (var port of ports) {
    testutil.killSandbox(port);
    testutil.waitMemberState(port, "UNREACHABLE,(MISSING)");
  }

  EXPECT_EQ("NO_QUORUM", dba.getCluster().status()["defaultReplicaSet"]["status"]);
}

function rejoin_instances(ports) {
  for (var port of ports) {
    testutil.startSandbox(port);
    testutil.waitMemberState(port, "ONLINE");
  }
}

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
replicacluster.addInstance(__sandbox_uri5);

CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4, __sandbox_uri5], cluster, replicacluster);

//@<> Forcing the quorum of a REPLICA Cluster must ensure the replication stream is kept - Use primary in partition
shell.connect(__sandbox_uri3);
force_quorum_loss([__mysql_sandbox_port4, __mysql_sandbox_port5])

// Disable SRO on the primary to test that forceQuorum enables it back
session.runSql("SET GLOBAL super_read_only=0");

replicacluster.forceQuorumUsingPartitionOf(__sandbox_uri3);

ensure_cs_replication_channel_ready(__sandbox_uri3, __mysql_sandbox_port3);

CHECK_REPLICA_CLUSTER([__sandbox_uri3], cluster, replicacluster);

rejoin_instances([__mysql_sandbox_port4, __mysql_sandbox_port5]);

CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4, __sandbox_uri5], cluster, replicacluster);

//@<> Forcing the quorum of a REPLICA Cluster must ensure the replication stream is kept - Use secondary in partition
shell.connect(__sandbox_uri4);
force_quorum_loss([__mysql_sandbox_port3, __mysql_sandbox_port5])

// Disable SRO on the secondary to test that forceQuorum enables it back
session.runSql("SET GLOBAL super_read_only=0");

replicacluster = dba.getCluster();
replicacluster.forceQuorumUsingPartitionOf(__sandbox_uri4);

ensure_cs_replication_channel_ready(__sandbox_uri4, __mysql_sandbox_port4);

replicacluster = dba.getCluster();

CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);

rejoin_instances([__mysql_sandbox_port3, __mysql_sandbox_port5])

CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3, __sandbox_uri5], cluster, replicacluster);

//@<> Forcing the quorum of the PRIMARY Cluster must ensure the replication stream is kept - Use primary in partition
replicacluster.removeInstance(__sandbox_uri4);

ensure_cs_replication_channel_ready(__sandbox_uri4, __mysql_sandbox_port3);

shell.connect(__sandbox_uri1);
cluster = dba.getCluster();
cluster.addInstance(__sandbox_uri4, {recoveryMethod: "clone"});

force_quorum_loss([__mysql_sandbox_port2, __mysql_sandbox_port4])

cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);
cluster = dba.getCluster()

rejoin_instances([__mysql_sandbox_port2, __mysql_sandbox_port4])

shell.connect(__sandbox_uri3);
replicacluster = dba.getCluster();

ensure_cs_replication_channel_ready(__sandbox_uri3, __mysql_sandbox_port1);

CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri5], cluster, replicacluster);

//@<> Forcing the quorum of the PRIMARY Cluster must ensure the replication stream is kept - Use secondary in partition
shell.connect(__sandbox_uri4);
force_quorum_loss([__mysql_sandbox_port1, __mysql_sandbox_port2])

cluster = dba.getCluster();
cluster.forceQuorumUsingPartitionOf(__sandbox_uri4);

rejoin_instances([__mysql_sandbox_port1, __mysql_sandbox_port2])

shell.connect(__sandbox_uri3);
replicacluster = dba.getCluster();

ensure_cs_replication_channel_ready(__sandbox_uri3, __mysql_sandbox_port4);

CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri5], cluster, replicacluster);

//@<> Forcing the quorum of a REPLICA Cluster when PRIMARY is OFFLINE should be possible and SRO must be kept

// Disable SRO on all members to verify that forceQuorum enables it back
var session3 = mysql.getSession(__sandbox_uri3);
var session5 = mysql.getSession(__sandbox_uri5);

session3.runSql("SET GLOBAL super_read_only=0")
session5.runSql("SET GLOBAL super_read_only=0")

force_quorum_loss([__mysql_sandbox_port5])

// Make the PRIMARY cluster unavailable
testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port4);

replicacluster = dba.getCluster();

EXPECT_NO_THROWS(function(){ replicacluster.forceQuorumUsingPartitionOf(__sandbox_uri3); });

rejoin_instances([__mysql_sandbox_port5]);

var session3 = mysql.getSession(__sandbox_uri3);
var session5 = mysql.getSession(__sandbox_uri5);

var sro3 = session3.runSql("select @@global.super_read_only").fetchOne()[0];
var sro5 = session5.runSql("select @@global.super_read_only").fetchOne()[0];

EXPECT_EQ(1, sro3);
EXPECT_EQ(1, sro5);

//@<> Rebooting from complete outage a PRIMARY Cluster
testutil.startSandbox(__mysql_sandbox_port1)
testutil.startSandbox(__mysql_sandbox_port2)
testutil.startSandbox(__mysql_sandbox_port4)

shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("cluster", {rejoinInstances: [__endpoint2, __endpoint4]}); });

//@<> Rebooting from complete outage a REPLICA Cluster with PRIMARY OK
disable_auto_rejoin(__mysql_sandbox_port3);
disable_auto_rejoin(__mysql_sandbox_port5);

shell.connect(__sandbox_uri3);
testutil.killSandbox(__mysql_sandbox_port5);
testutil.waitMemberState(__mysql_sandbox_port5, "(MISSING),UNREACHABLE");
testutil.killSandbox(__mysql_sandbox_port3);

testutil.startSandbox(__mysql_sandbox_port3)
testutil.startSandbox(__mysql_sandbox_port5)

shell.connect(__sandbox_uri3);

// BUG#33223867: dba.rebootClusterFromCompleteOutage() is failing for replica cluster in CS
// When using the rejoinInstances option in rebootCluster*() of a Replica Cluster, the command would fail since it wouldn't wait
// for the seed instance to become ONLINE within the group before trying to rejoin the remaining instances
EXPECT_NO_THROWS(function() { replicacluster = dba.rebootClusterFromCompleteOutage("replica", {rejoinInstances: [__endpoint5]})});
EXPECT_OUTPUT_CONTAINS("* Waiting for seed instance to become ONLINE...");

var sro3 = session.runSql("select @@global.super_read_only").fetchOne()[0];
EXPECT_EQ(1, sro3);

ensure_cs_replication_channel_ready(__sandbox_uri3, __mysql_sandbox_port4);
ensure_cs_replication_channel_ready(__sandbox_uri5, __mysql_sandbox_port4);

// Check that the cluster rejoined the ClusterSet
CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri5], cluster, replicacluster);

//@<> Rebooting from complete outage a REPLICA Cluster with PRIMARY NOT_OK

// shutdown REPLICA
disable_auto_rejoin(__mysql_sandbox_port3);
disable_auto_rejoin(__mysql_sandbox_port5);

shell.connect(__sandbox_uri3);
testutil.killSandbox(__mysql_sandbox_port5);
testutil.waitMemberState(__mysql_sandbox_port5, "(MISSING),UNREACHABLE");
testutil.killSandbox(__mysql_sandbox_port3);

testutil.startSandbox(__mysql_sandbox_port3)
testutil.startSandbox(__mysql_sandbox_port5)

// shutdown PRIMARY
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port4);

shell.connect(__sandbox_uri1);

testutil.killSandbox(__mysql_sandbox_port4);
testutil.waitMemberState(__mysql_sandbox_port4, "(MISSING),UNREACHABLE");
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING),UNREACHABLE");
testutil.killSandbox(__mysql_sandbox_port1);

testutil.startSandbox(__mysql_sandbox_port1)
testutil.startSandbox(__mysql_sandbox_port2)
testutil.startSandbox(__mysql_sandbox_port4)

shell.connect(__sandbox_uri3);

EXPECT_NO_THROWS(function() { replicacluster = dba.rebootClusterFromCompleteOutage("replica"); });

var sro3 = session.runSql("select @@global.super_read_only").fetchOne()[0];
EXPECT_EQ(1, sro3);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
