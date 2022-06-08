// Various tests to confirm that operations performed on the ClusterSet
// object are always done on the most up-to-date view of the topology,
// i.e. the correct metadata server and global primary instance

//@ {VER(>=8.0.27)}

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
var cluster = scene.cluster

//@<> Remove Cluster from ClusterSet after double switchover

// Create a ClusterSet + 1 Replica Cluster
clusterset = cluster.createClusterSet("cs");
replica_cluster = clusterset.createReplicaCluster(__sandbox_uri3, "cluster2");
replica_cluster.addInstance(__sandbox_uri4);

// Do a switchover and then switchover back again to the initial Primary
clusterset.setPrimaryCluster("cluster2");
clusterset.setPrimaryCluster("cluster");

// Remove the primary instance of the Replica Cluster
replica_cluster.removeInstance(__sandbox_uri3);

// Remove the Replica Cluster from the ClusterSet (must succeed)
EXPECT_NO_THROWS(function() { clusterset.removeCluster("cluster2"); });

//@<> Switchover after switching Cluster primaries
// Create the Replica cluster again
replica_cluster = clusterset.createReplicaCluster(__sandbox_uri3, "cluster2");
replica_cluster.addInstance(__sandbox_uri4, {recoveryMethod: "clone"});

// Switch the primary instance of the replica and primary clusters
replica_cluster.setPrimaryInstance(__endpoint4);
cluster.setPrimaryInstance(__sandbox_uri2);

// Switchover the primary cluster (must succeed)
EXPECT_NO_THROWS(function() { clusterset.setPrimaryCluster("cluster2"); });

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);

