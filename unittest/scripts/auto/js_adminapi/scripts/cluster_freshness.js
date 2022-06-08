// Various tests to confirm that operations performed on the Cluster object
// are always done on the most up-to-date view of the topology, i.e. the
// correct metadata server and primary instance

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster

//@<> Remove the primary instance from the Cluster and attempt any operation after
cluster.removeInstance(__sandbox_uri1);

// Operations won't fail since since primary discovery takes place with every call
var s;
EXPECT_NO_THROWS(function() { s = cluster.status(); });
EXPECT_EQ(s["defaultReplicaSet"]["primary"], __endpoint2);

// Add back the former primary
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();
cluster.addInstance(__sandbox_uri1, {recoveryMethod: "incremental"});

// Get the Cluster object from a secondary
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

// Remove the primary
cluster.removeInstance(__sandbox_uri2);

// Operations won't fail since since primary discovery takes place with every call
var s;
EXPECT_NO_THROWS(function() { s = cluster.status(); });
EXPECT_EQ(s["defaultReplicaSet"]["primary"], __endpoint1);

//@<> Switch the primary and attempt any operation after {VER(>=8.0)}
cluster.addInstance(__sandbox_uri2);
cluster.setPrimaryInstance(__sandbox_uri2);

var s;
EXPECT_NO_THROWS(function() { s = cluster.status(); });
EXPECT_EQ(s["defaultReplicaSet"]["primary"], __endpoint2);

EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri3); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3); });
EXPECT_NO_THROWS(function() { cluster.setPrimaryInstance(__sandbox_uri3); });
// Set the memberWeight of sandbox1 to the maximum to ensure that's the one
// elected when we remove the primary
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__sandbox_uri1, "memberWeight", 100); });
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri3); });
EXPECT_NO_THROWS(function() { s = cluster.status(); });
EXPECT_EQ(s["defaultReplicaSet"]["primary"], __endpoint1);

//@<> Cleanup
scene.destroy();
