//@ {VER(>=8.0.27)}

// Tests removeCluster() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

//@<> INCLUDE clusterset_utils.inc

// Negative tests based on environment and params
//-----------------------------------------------

//@<> Setup + Create primary cluster + add Replica Cluster
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port6, "root", {report_host: hostname});

session1 = mysql.getSession(__sandbox_uri1);
session4 = mysql.getSession(__sandbox_uri4);
session6 = mysql.getSession(__sandbox_uri6);

var clusterset = cluster.createClusterSet("myClusterSet");

var replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});

//@<> Bad options (should fail)
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster()}, "Invalid number of arguments, expected 1 to 2 but got 0", "ArgumentError");
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster(null)}, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster(123)}, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("foo bar")}, "Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (foo bar)", "ArgumentError");
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("my::clusterset")}, "Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (my::clusterset)", "ArgumentError");
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("")}, "The Cluster name cannot be empty.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("1234567890123456789012345678901234567890123456789012345678901234")}, "The Cluster name can not be greater than 63 characters", "ArgumentError");
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("myreplica", {"badOption": true})}, "Argument #2: Invalid options: badOption", "ArgumentError");
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("myreplica", {timeout: -1})}, "Argument #2 timeout option must be >= 0", "ArgumentError");

//FR11: The operation must fail if clusterName refers to the PRIMARY cluster or the PRIMARY cluster is unavailable (global status different than OK)

//@<> Remove primary cluster (should fail)
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("cluster")}, "The Cluster 'cluster' is the PRIMARY Cluster of the ClusterSet.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: Cannot remove the PRIMARY Cluster of the ClusterSet.");

//@<> Remove a non-existing cluster (should fail)
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("foobar")}, "The cluster with the name 'foobar' does not exist.", "MYSQLSH");

//@<> removeCluster() - dryRun
EXPECT_NO_THROWS(function() {clusterset.removeCluster("replicacluster", {dryRun: 1}); });

EXPECT_OUTPUT_CONTAINS(`The Cluster 'replicacluster' will be removed from the InnoDB ClusterSet.`);
EXPECT_OUTPUT_CONTAINS(`NOTE: The Cluster's Group Replication configurations will be kept and the Cluster will become an independent entity.`);
EXPECT_OUTPUT_CONTAINS(`* Waiting for the Cluster to synchronize with the PRIMARY Cluster...`);
EXPECT_OUTPUT_CONTAINS(`* Updating topology`);
EXPECT_OUTPUT_CONTAINS(`* Stopping and deleting ClusterSet managed replication channel...`);
EXPECT_OUTPUT_CONTAINS(`The Cluster 'replicacluster' was removed from the ClusterSet.`);
EXPECT_OUTPUT_CONTAINS(`dryRun finished.`);

//@<> Remove a cluster that is a targetCluster for Routers (should fail)
// Insert some data into the routers table and set the Cluster as targetCluster for a router
var __cluster_id = session1.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name='replicacluster'").fetchOne()[0];
var __cluster_group_name = session4.runSql("select @@group_replication_group_name").fetchOne()[0];
session1.runSql("INSERT INTO mysql_innodb_cluster_metadata.v2_routers (address, product_name, router_name) VALUES ('foo', 'MySQL Router', 'router_primary')");
session1.runSql("INSERT INTO mysql_innodb_cluster_metadata.v2_routers (address, product_name, router_name) VALUES ('bar', 'MySQL Router', 'router_secondary')");
session1.runSql("UPDATE mysql_innodb_cluster_metadata.v2_routers SET attributes = JSON_SET(JSON_SET(JSON_SET(JSON_SET(JSON_SET(IF(attributes IS NULL, '{}', attributes), '$.RWEndpoint', '6446'), '$.ROEndpoint', '6447'), '$.RWXEndpoint', '6448'), '$.ROXEndpoint', '6449'), '$.MetadataUser', 'router'), options = JSON_SET(IF(options IS NULL, '{}', options), '$.targetCluster', ?), version = '8.0.25', cluster_id = ? WHERE router_id = 1", [__cluster_group_name, __cluster_id]);
session1.runSql("UPDATE mysql_innodb_cluster_metadata.v2_routers SET attributes = JSON_SET(JSON_SET(JSON_SET(JSON_SET(JSON_SET(IF(attributes IS NULL, '{}', attributes), '$.RWEndpoint', '6446'), '$.ROEndpoint', '6447'), '$.RWXEndpoint', '6448'), '$.ROXEndpoint', '6449'), '$.MetadataUser', 'router'), options = JSON_SET(IF(options IS NULL, '{}', options), '$.targetCluster', ?), version = '8.0.25', cluster_id = ? WHERE router_id = 2", [__cluster_group_name, __cluster_id]);

EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("replicacluster"); }, "The Cluster 'replicacluster' is an active Target Cluster for Routing operations.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: The following Routers are using the Cluster 'replicacluster' as a target cluster: [foo::router_primary, bar::router_secondary]. Please ensure no Routers are using the cluster as target with .setRoutingOption()");

//@<> Remove a cluster (success)
//Clear up the Routers having the replica cluster as target
session1.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET options = json_set(options, '$.targetCluster', 'primary') WHERE cluster_id = ?", [__cluster_id]);

CHECK_GTID_CONSISTENT(session1, session4);

EXPECT_NO_THROWS(function() {clusterset.removeCluster("replicacluster", {timeout: 0}); });

CHECK_ROUTER_OPTIONS_REMOVED_CLUSTER(session1);

// entry in clusters table will be removed in the clusterset but not in the removed cluster
var res = session1.runSql("select * from mysql_innodb_cluster_metadata.clusters order by cluster_name");
row = res.fetchOne();
EXPECT_EQ("cluster", row["cluster_name"]);
EXPECT_EQ(null, res.fetchOne());

res = session4.runSql("select * from mysql_innodb_cluster_metadata.clusters order by cluster_name");
row = res.fetchOne();
EXPECT_EQ("cluster", row["cluster_name"]);
row = res.fetchOne();
EXPECT_EQ("replicacluster", row["cluster_name"]);
EXPECT_EQ(null, res.fetchOne());

var res = session1.runSql("select * from mysql_innodb_cluster_metadata.v2_cs_members order by cluster_name");
row = res.fetchOne();
EXPECT_EQ("cluster", row["cluster_name"]);
EXPECT_EQ("PRIMARY", row["member_role"]);
EXPECT_EQ(0, row["invalidated"]);
EXPECT_EQ(null, res.fetchOne());

// The Cluster should not belong to the ClusterSet in the Cluster's metadata too
var res = session4.runSql("select * from mysql_innodb_cluster_metadata.v2_cs_members order by cluster_name");
row = res.fetchOne();
EXPECT_EQ("cluster", row["cluster_name"]);
EXPECT_EQ("PRIMARY", row["member_role"]);
EXPECT_EQ(0, row["invalidated"]);
EXPECT_EQ(null, res.fetchOne());

CHECK_REMOVED_CLUSTER([__sandbox_uri4], cluster, "replicacluster");

//<> Remove Cluster but with limited sync timeout
var replicacluster2 = clusterset.createReplicaCluster(__sandbox_uri5, "replicacluster2", {recoveryMethod: "clone"});

session1 = mysql.getSession(__sandbox_uri1);
session5 = mysql.getSession(__sandbox_uri5);
session1.runSql("create schema if not exists testing");
session1.runSql("create table if not exists testing.data (k int primary key auto_increment, v longtext)");
testutil.waitMemberTransactions(__mysql_sandbox_port5, __mysql_sandbox_port1);
session5.runSql("lock tables testing.data read");
session1.runSql("insert into testing.data values (default, repeat('#', 10))");

EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("replicacluster2", {timeout: 2})}, `Timeout reached waiting for transactions from ${hostname}:${__mysql_sandbox_port1} to be applied on instance '${hostname}:${__mysql_sandbox_port5}'`, "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: The Cluster failed to synchronize its transaction set with the PRIMARY Cluster. You may increase the transaction sync timeout with the option 'timeout' or use the 'force' option to ignore the timeout.");

// unlock the table to allow the sync
session5.runSql("unlock tables");
EXPECT_NO_THROWS(function() {clusterset.removeCluster("replicacluster2", {timeout: 2}); });
CHECK_REMOVED_CLUSTER([__sandbox_uri5], cluster, "replicacluster");

//@<> Create a ClusterSet on the removed Cluster (should succeed)
shell.connect(__sandbox_uri4);
removed_cluster = dba.getCluster()
EXPECT_NO_THROWS(function() {removed_cluster.createClusterSet("newClusterSet"); });

//@<> Remove a Cluster that does not belong to the ClusterSet (should fail)
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("replicacluster")}, "The cluster with the name 'replicacluster' does not exist.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: The Cluster 'replicacluster' does not exist or does not belong to the ClusterSet.");

//@<> Create a Replica Cluster from a Removed Cluster (should fail)
EXPECT_THROWS_TYPE(function(){clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"})}, "Target instance already part of an InnoDB Cluster", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port4}' is already part of an InnoDB Cluster. A new Replica Cluster must be created on a standalone instance.`);

//FR12: The operation must fail if the PRIMARY instance of the target cluster is not reachable unless the force:true option is given.

wipeout_cluster(session1, replicacluster);

//@<> Remove cluster where repl channel is down
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});

// Validate the replica cluster was created successfully
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);

session4.runSql("stop replica for channel 'clusterset_replication'");

EXPECT_THROWS(function(){clusterset.removeCluster("replicacluster");}, "ClusterSet Replication Channel not in expected state");
EXPECT_OUTPUT_CONTAINS("ERROR: The ClusterSet Replication channel has an invalid state 'OFF'. Use the 'force' option to ignore this check.");

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

//@<> Remove a cluster where repl channel is not existent (reseted)
session4.runSql("stop replica for channel 'clusterset_replication'");
session4.runSql("reset replica all for channel 'clusterset_replication'");

EXPECT_THROWS(function(){clusterset.removeCluster("replicacluster");}, " Replication channel does not exist", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: The ClusterSet Replication channel could not be found at the Cluster 'replicacluster'. Use the 'force' option to ignore this check.");

//@<> Remove cluster where repl channel is down + force
wipeout_cluster(session1, replicacluster);
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});
session4.runSql("stop replica for channel 'clusterset_replication'");

shell.options["logLevel"]=8
clusterset.removeCluster("replicacluster", {force:1});

CHECK_REMOVED_CLUSTER([__sandbox_uri4], cluster, "replicacluster");

wipeout_cluster(session1, replicacluster);

//@<> Remove cluster when replica has applier error (should fail)
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster");

values = {
  "domain": "myClusterSet"
};
CHECK_PRIMARY_CLUSTER([__sandbox_uri1], cluster, values);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster, values);

session4 = mysql.getSession(__sandbox_uri4);

inject_applier_error(session1, session4);

values["Replica_SQL_Running"] = "No";

EXPECT_THROWS(function(){clusterset.removeCluster("replicacluster");}, "ClusterSet Replication Channel not in expected state");

EXPECT_OUTPUT_CONTAINS("The ClusterSet Replication channel has an invalid state 'APPLIER_ERROR'. Use the 'force' option to ignore this check.");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], cluster, values);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster, values);

wipeout_cluster(session1, replicacluster);

//@<> Remove OFFLINE cluster (should fail)
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});

session4.runSql("stop group_replication");

EXPECT_OUTPUT_CONTAINS("");

EXPECT_THROWS(function(){clusterset.removeCluster("replicacluster");}, "PRIMARY instance of Cluster 'replicacluster' is unavailable:");

//@<> Remove OFFLINE cluster + force
clusterset.removeCluster("replicacluster", {force:1});

CHECK_REMOVED_CLUSTER([__sandbox_uri4], cluster, "replicacluster");

wipeout_cluster(session1, [__address4r]);

//@<> Remove unreachable cluster (should fail)
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});

session4.runSql("shutdown");

EXPECT_THROWS(function(){clusterset.removeCluster("replicacluster");}, "PRIMARY instance of Cluster 'replicacluster' is unavailable: 'MYSQLSH 51014: Could not connect to a PRIMARY member of cluster 'replicacluster'");

//@<> Remove unreachable cluster + force
clusterset.removeCluster("replicacluster", {force:1});

testutil.startSandbox(__mysql_sandbox_port4);
session4 = mysql.getSession(__sandbox_uri4);
wipeout_cluster(session1, [__address4r]);

//@<> Remove partial OFFLINE cluster
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});
replicacluster.addInstance(__sandbox_uri6);

session6.runSql("stop group_replication");

clusterset.removeCluster("replicacluster")

//@<> Remove NO_QUORUM cluster (should fail)
wipeout_cluster(session1, [__address4r, __address6r]);
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster");
replicacluster.addInstance(__sandbox_uri6);

testutil.killSandbox(__mysql_sandbox_port6);
shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port6, "UNREACHABLE");

EXPECT_THROWS(function(){clusterset.removeCluster("replicacluster");}, "PRIMARY instance of Cluster 'replicacluster' is unavailable: 'MYSQLSH 51011: Cluster 'replicacluster' has no quorum'");

//@<> Remove NO_QUORUM cluster + force (should still fail) {false}
// TODO enable it back after wl11894
// we can't remove a NO_QUORUM cluster because STOP REPLICA and most things will just freeze
EXPECT_THROWS(function(){clusterset.removeCluster("replicacluster");}, "PRIMARY instance of Cluster 'replicacluster' is unavailable: 'MYSQLSH 51011: Cluster 'replicacluster' has no quorum'");

//@<> temporary recovery until test above is fixed
replicacluster.forceQuorumUsingPartitionOf(__sandbox_uri4);
clusterset.removeCluster("replicacluster");

//@<> Remove cluster with errant trxs
wipeout_cluster(session1, [__address4r]);

replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});

inject_errant_gtid(session4);

clusterset.removeCluster("replicacluster");

wipeout_cluster(session1, replicacluster);

//@<> Remove invalidated cluster
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});

cs_id = session1.runSql("select clusterset_id from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0];
pc_id = session1.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name='cluster'").fetchOne()[0];
rc_id = session1.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name='replicacluster'").fetchOne()[0];

session1.runSql("call mysql_innodb_cluster_metadata.v2_cs_primary_changed(?, ?, '{}')", [cs_id, pc_id]);
session1.runSql("call mysql_innodb_cluster_metadata.v2_cs_add_invalidated_member(?, ?)", [cs_id, rc_id]);

//clusterset.status();
clusterset.removeCluster("replicacluster");

wipeout_cluster(session1, replicacluster);

//@<> Remove cluster when PRIMARY has purged trxs, which will be unsyncable (should fail)
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});

session4.runSql("flush tables with read lock");
inject_purged_gtids(session1);
session4.runSql("unlock tables");

clusterset.removeCluster("replicacluster");

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
