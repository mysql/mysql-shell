//@ {VER(>=8.0.27)}

// Tests various positive and negative scenarios for InnoDB Cluster restore operations on Clusters that are part of a ClusterSet:
//   - dba.rebootClusterFromCompleteOutage()

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

cs = cluster.createClusterSet("domain");

replicacluster = cs.createReplicaCluster(__sandbox_uri3, "replica", {"manualStartOnBoot": true});
replicacluster.addInstance(__sandbox_uri4);

CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], cluster, replicacluster);

// Make the PRIMARY cluster unavailable
testutil.stopGroup([__mysql_sandbox_port1, __mysql_sandbox_port2]);

//@<> Rebooting from complete outage a PRIMARY Cluster
shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("cluster"); });

//@<> Rebooting from complete outage a REPLICA Cluster with PRIMARY OK
testutil.stopGroup([__mysql_sandbox_port3, __mysql_sandbox_port4]);

shell.connect(__sandbox_uri3);

// BUG#33223867: dba.rebootClusterFromCompleteOutage() is failing for replica
// cluster in CS
// When using the rejoinInstances option in rebootCluster*() of a Replica Cluster, the command would fail since it wouldn't wait
// for the seed instance to become ONLINE within the group before trying to rejoin the remaining instances
EXPECT_NO_THROWS(function() { replicacluster = dba.rebootClusterFromCompleteOutage("replica"); });
EXPECT_OUTPUT_CONTAINS("* Waiting for seed instance to become ONLINE...");

var sro3 = session.runSql("select @@global.super_read_only").fetchOne()[0];
EXPECT_EQ(1, sro3);

ensure_cs_replication_channel_ready(__sandbox_uri3, __mysql_sandbox_port1);
ensure_cs_replication_channel_ready(__sandbox_uri4, __mysql_sandbox_port1);

// Check that the cluster rejoined the ClusterSet
CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], cluster, replicacluster);

//@<> Rebooting from complete outage a REPLICA Cluster with PRIMARY NOT_OK

// shutdown REPLICA and PRIMARY
testutil.stopGroup([__mysql_sandbox_port3, __mysql_sandbox_port4, __mysql_sandbox_port1, __mysql_sandbox_port2]);

shell.connect(__sandbox_uri3);

EXPECT_NO_THROWS(function() { replicacluster = dba.rebootClusterFromCompleteOutage("replica"); });
EXPECT_OUTPUT_CONTAINS("WARNING: Unable to rejoin Cluster to the ClusterSet (primary Cluster is unreachable). Please call ClusterSet.rejoinCluster() to manually rejoin this Cluster back into the ClusterSet.");

var sro3 = session.runSql("select @@global.super_read_only").fetchOne()[0];
EXPECT_EQ(1, sro3);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
