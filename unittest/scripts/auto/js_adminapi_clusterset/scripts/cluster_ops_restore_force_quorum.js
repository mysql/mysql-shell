//@ {VER(>=8.0.27)}

// Tests various positive and negative scenarios for InnoDB Cluster restore operations on Clusters that are part of a ClusterSet:
//   - <Cluster>.forceQuorumUsingPartitionOf()

function force_quorum_loss(ports) {
  for (var port of ports) {
    testutil.killSandbox(port);
    testutil.waitMemberState(port, "UNREACHABLE,(MISSING)");
  }

  EXPECT_EQ("NO_QUORUM", dba.getCluster().status()["defaultReplicaSet"]["status"]);
}

function rejoin_instances(ports, cluster) {
  for (var port of ports) {
    testutil.startSandbox(port);
    if (cluster) {
      var uri = hostname + ":" + port;
      cluster.rejoinInstance(uri);
    }
    testutil.waitMemberState(port, "ONLINE");
  }
}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup + Create primary cluster + add Replica Cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], {"manualStartOnBoot": true});
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");

replicacluster = cs.createReplicaCluster(__sandbox_uri3, "replica", {"manualStartOnBoot": true});
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

rejoin_instances([__mysql_sandbox_port4, __mysql_sandbox_port5], replicacluster);

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

rejoin_instances([__mysql_sandbox_port3, __mysql_sandbox_port5], replicacluster)

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

rejoin_instances([__mysql_sandbox_port2, __mysql_sandbox_port4], cluster)

shell.connect(__sandbox_uri3);
replicacluster = dba.getCluster();

ensure_cs_replication_channel_ready(__sandbox_uri3, __mysql_sandbox_port1);

CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri5], cluster, replicacluster);

//@<> Forcing the quorum of the PRIMARY Cluster must ensure the replication stream is kept - Use secondary in partition
shell.connect(__sandbox_uri4);
force_quorum_loss([__mysql_sandbox_port1, __mysql_sandbox_port2])

cluster = dba.getCluster();
cluster.forceQuorumUsingPartitionOf(__sandbox_uri4);

rejoin_instances([__mysql_sandbox_port1, __mysql_sandbox_port2], cluster)

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
testutil.stopGroup([__mysql_sandbox_port1, __mysql_sandbox_port2]);

disable_auto_rejoin(__mysql_sandbox_port4);
testutil.killSandbox(__mysql_sandbox_port4);

replicacluster = dba.getCluster();

EXPECT_NO_THROWS(function(){ replicacluster.forceQuorumUsingPartitionOf(__sandbox_uri3); });

var session3 = mysql.getSession(__sandbox_uri3);

var sro3 = session3.runSql("select @@global.super_read_only").fetchOne()[0];

EXPECT_EQ(1, sro3);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
