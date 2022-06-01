//@ {VER(>=8.0.27)}

// Tests removeCluster() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

//@<> INCLUDE clusterset_utils.inc
//@<> INCLUDE dba_utils.inc

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
replicacluster.addInstance(__sandbox_uri6);

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
session1.runSql("UPDATE mysql_innodb_cluster_metadata.v2_routers SET attributes = JSON_SET(JSON_SET(JSON_SET(JSON_SET(JSON_SET(IF(attributes IS NULL, '{}', attributes), '$.RWEndpoint', 'mysql.sock'), '$.ROEndpoint', '6447'), '$.RWXEndpoint', ''), '$.ROXEndpoint', '6449'), '$.MetadataUser', 'router'), options = JSON_SET(IF(options IS NULL, '{}', options), '$.target_cluster', ?), version = '8.0.25', cluster_id = ? WHERE router_id = 1", [__cluster_group_name, __cluster_id]);
session1.runSql("UPDATE mysql_innodb_cluster_metadata.v2_routers SET attributes = JSON_SET(JSON_SET(JSON_SET(JSON_SET(JSON_SET(IF(attributes IS NULL, '{}', attributes), '$.RWEndpoint', '6446'), '$.ROEndpoint', 'mysqlro.sock'), '$.RWXEndpoint', '6448'), '$.ROXEndpoint', ''), '$.MetadataUser', 'router'), options = JSON_SET(IF(options IS NULL, '{}', options), '$.target_cluster', ?), version = '8.0.25', cluster_id = ? WHERE router_id = 2", [__cluster_group_name, __cluster_id]);

EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("replicacluster"); }, "The Cluster 'replicacluster' is an active Target Cluster for Routing operations.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: The following Routers are using the Cluster 'replicacluster' as a target cluster: [foo::router_primary, bar::router_secondary]. Please ensure no Routers are using the cluster as target with .setRoutingOption()");

//@<> Remove a cluster (success)
//Clear up the Routers having the replica cluster as target
session1.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET options = json_set(options, '$.target_cluster', 'primary') WHERE cluster_id = ?", [__cluster_id]);

CHECK_GTID_CONSISTENT(session1, session4);

EXPECT_NO_THROWS(function() {clusterset.removeCluster("replicacluster", {timeout: 0}); });
EXPECT_OUTPUT_CONTAINS("* Reconciling internally generated GTIDs...");

CHECK_ROUTER_OPTIONS_REMOVED_CLUSTER(session1);

// entry in clusters table will be removed in the clusterset and in the removed cluster
var res = session1.runSql("select * from mysql_innodb_cluster_metadata.clusters order by cluster_name");
row = res.fetchOne();
EXPECT_EQ("cluster", row["cluster_name"]);
EXPECT_EQ(null, res.fetchOne());

res = session4.runSql("select * from mysql_innodb_cluster_metadata.clusters order by cluster_name");
row = res.fetchOne();
EXPECT_EQ("cluster", row["cluster_name"]);
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

// The Cluster members should not belong to the instances table anymore
var res = session1.runSql("select address from mysql_innodb_cluster_metadata.instances");
row = res.fetchOne();
EXPECT_EQ(__endpoint1, row["address"]);
EXPECT_EQ(null, res.fetchOne());

var res = session4.runSql("select address from mysql_innodb_cluster_metadata.instances");
row = res.fetchOne();
EXPECT_EQ(__endpoint1, row["address"]);
EXPECT_EQ(null, res.fetchOne());

// Group Replication should be stopped on the Cluster
EXPECT_EQ("OFFLINE", session4.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);

CHECK_REMOVED_CLUSTER([__sandbox_uri4, __sandbox_uri6], cluster, "replicacluster");

//@<> Re-create the Replica Cluster on the primary instance (GTID-SET should have been reconciled regarding VCLEs).
var replica_vcle_primary;
EXPECT_NO_THROWS(function() {replica_vcle_primary = clusterset.createReplicaCluster(__sandbox_uri4, "replica_vcle_primary"); });

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replica_vcle_primary);
CHECK_CLUSTER_SET(session);

EXPECT_NO_THROWS(function() {clusterset.removeCluster("replica_vcle_primary", {timeout: 0}); });
EXPECT_OUTPUT_CONTAINS("* Reconciling internally generated GTIDs...");

//@<> Re-create the Replica Cluster on a secondary instance (GTID-SET should have been reconciled regarding VCLEs)
var replica_vcle_secondary;
EXPECT_NO_THROWS(function() {replica_vcle_secondary = clusterset.createReplicaCluster(__sandbox_uri6, "replica_vcle_secondary"); });

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri6], cluster, replica_vcle_secondary);
CHECK_CLUSTER_SET(session);

EXPECT_NO_THROWS(function() {clusterset.removeCluster("replica_vcle_secondary", {timeout: 0}); });
EXPECT_OUTPUT_CONTAINS("* Reconciling internally generated GTIDs...");

//@<> Re-create the Replica Cluster on a single-member Cluster (GTID-SET should have been reconciled regarding VCLEs).
CHECK_REMOVED_CLUSTER([__sandbox_uri4], cluster, "replica_vcle_primary");

EXPECT_NO_THROWS(function() {replica_vcle_primary = clusterset.createReplicaCluster(__sandbox_uri4, "replica_vcle_primary"); });

CHECK_PRIMARY_CLUSTER([__sandbox_uri1], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replica_vcle_primary);
CHECK_CLUSTER_SET(session);

// Remove the cluster to clean-up for the proceeding tests
EXPECT_NO_THROWS(function() {clusterset.removeCluster("replica_vcle_primary", {timeout: 0}); });
EXPECT_OUTPUT_CONTAINS("* Reconciling internally generated GTIDs...");

//@<> Remove Cluster but with limited sync timeout
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

//@<> Create a Cluster on the removed Cluster (should succeed)
shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function() {newcluster = dba.createCluster("newCluster"); });

//@<> Add one of the Removed Cluster members to the newly created cluster
EXPECT_NO_THROWS(function() {newcluster.addInstance(__sandbox_uri6); });

//@<> Remove a Cluster that does not belong to the ClusterSet (should fail)
EXPECT_THROWS_TYPE(function(){clusterset.removeCluster("replicacluster")}, "The cluster with the name 'replicacluster' does not exist.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: The Cluster 'replicacluster' does not exist or does not belong to the ClusterSet.");

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
session4.runSql("reset replica all for channel 'clusterset_replication'");

shell.options["logLevel"]=8
clusterset.removeCluster("replicacluster", {force:1});

CHECK_REMOVED_CLUSTER([__sandbox_uri4], cluster, "replicacluster");

// Check the cluster was dissolved
EXPECT_EQ("OFFLINE", session4.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);

// Attempting to create a Cluster on the instance must fail since its Metadata wasn't dropped
EXPECT_THROWS(function(){dba.createCluster("newCluster");}, "Unable to create cluster. The instance '<<<__endpoint4>>>' has a populated Metadata schema and belongs to that Metadata. Use either dba.dropMetadataSchema() to drop the schema, or dba.rebootClusterFromCompleteOutage() to reboot the cluster from complete outage.", "RuntimeError");

// Reboot the Cluster from Complete Outage
EXPECT_NO_THROWS(function() {dba.rebootClusterFromCompleteOutage(); });
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster 'replicacluster' appears to have been removed from the ClusterSet 'myClusterSet', however its own metadata copy wasn't properly updated during the removal");

EXPECT_NO_THROWS(function() {c = dba.getCluster(); });

// Dissolve the Cluster and verify all accounts and metadata are dropped (BUG#33239404)

// Verify that the member actions are reset to the defaults too. The scenario on which the member actions wouldn't be reset is the one on which a Cluster is removed from a ClusterSet using the force option because it's unreachable. In this test-suite we cannot do that and since removeCluster() can contact the cluster to be removed it will reset the member actions. So to simulate that, we manually change the values of the member actions
session4.runSql("SELECT group_replication_disable_member_action('mysql_start_failover_channels_if_primary', 'AFTER_PRIMARY_ELECTION')");
session4.runSql("SELECT group_replication_disable_member_action('mysql_disable_super_read_only_if_primary', 'AFTER_PRIMARY_ELECTION')");

c.dissolve();
CHECK_DISSOLVED_CLUSTER(session);

reset_instance(session4);

//@<> Dissolve cluster that was the primary and got invalidated
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster");
clusterset.setPrimaryCluster("replicacluster");

testutil.stopSandbox(__mysql_sandbox_port4);
clusterset.forcePrimaryCluster("cluster");
testutil.startSandbox(__mysql_sandbox_port4);
session4 = mysql.getSession(__sandbox_uri4);
clusterset.removeCluster("replicacluster", {force:1});

shell.connect(__sandbox_uri4);
rc = dba.rebootClusterFromCompleteOutage();
EXPECT_OUTPUT_NOT_CONTAINS("Rejoining Cluster into its original ClusterSet..."); // since the cluster is INVALIDATED, it won't join the set

//@<> getCluster() from the invalidated PC after reboot
// (Bug#33166307)
cc = dba.getCluster();
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster 'replicacluster' appears to have been removed from the ClusterSet 'myClusterSet', however its own metadata copy wasn't properly updated during the removal");

//@<> dissolve invalidated primary cluster with the handle from the reboot
// (Bug#33166307)
rc.dissolve();
// Dissolve the Cluster and verify all accounts and metadata are dropped (BUG#33239404)
CHECK_DISSOLVED_CLUSTER(session);

reset_instance(session4);

//@<> Remove cluster when replica has applier error (should fail)

// We have to change the server_id of the instance before adding it as a replica
// otherwise a replication bug will prevent transactions originating from this
// instance to be replicated back to it, even after a reset master all
session4.runSql("set persist server_id=1221");

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

//@<> Remove NO_QUORUM cluster + force (should still fail)
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

//@<> Remove invalidated cluster
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "clone"});

cs_id = session1.runSql("select clusterset_id from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0];
pc_id = session1.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name='cluster'").fetchOne()[0];
rc_id = session1.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name='replicacluster'").fetchOne()[0];

session1.runSql("call mysql_innodb_cluster_metadata.v2_cs_primary_changed(?, ?, '{}')", [cs_id, pc_id]);
session1.runSql("call mysql_innodb_cluster_metadata.v2_cs_add_invalidated_member(?, ?)", [cs_id, rc_id]);

//clusterset.status();
clusterset.removeCluster("replicacluster");

//@<> Remove cluster when PRIMARY has purged trxs, which will be unsyncable (should fail)
replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental"});

session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("flush tables with read lock");
inject_purged_gtids(session1);
session4.runSql("unlock tables");

clusterset.removeCluster("replicacluster");

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
