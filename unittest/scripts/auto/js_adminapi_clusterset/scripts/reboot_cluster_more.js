//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup + Create primary cluster + add Replica Cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port6, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");

replica = cs.createReplicaCluster(__sandbox_uri4, "replica");
replica.addInstance(__sandbox_uri5);
replica.addInstance(__sandbox_uri6);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], cluster, replica);
CHECK_CLUSTER_SET(session);

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

//@<> FR1.1 + FR3.1
// Rebooting an INVALIDATED cluster with unreachable and not offline members
// If the Cluster belongs to a ClusterSet and is INVALIDATED, instances are not automatically rejoined

//Shutdown the primary
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);

//Failover to the replica cluster
shell.connect(__sandbox_uri4);
cluster = dba.getClusterSet();
cluster.forcePrimaryCluster("replica");

//Reboot the old-primary cluster
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

EXPECT_NO_THROWS(function(){ old_primary = dba.rebootClusterFromCompleteOutage("cluster", {dryRun: true}); });
EXPECT_OUTPUT_CONTAINS("NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.");
EXPECT_OUTPUT_CONTAINS("Skipping rejoining remaining instances because the Cluster belongs to a ClusterSet and is INVALIDATED. Please add or remove the instances after the Cluster is rejoined to the ClusterSet.");
EXPECT_OUTPUT_CONTAINS("dryRun finished.");
EXPECT_OUTPUT_NOT_CONTAINS("The Cluster was successfully rebooted.");

testutil.wipeAllOutput();

EXPECT_NO_THROWS(function(){ old_primary = dba.rebootClusterFromCompleteOutage("cluster"); });
EXPECT_OUTPUT_CONTAINS("Skipping rejoining remaining instances because the Cluster belongs to a ClusterSet and is INVALIDATED. Please add or remove the instances after the Cluster is rejoined to the ClusterSet.");
EXPECT_OUTPUT_CONTAINS("The Cluster was successfully rebooted.");
EXPECT_OUTPUT_NOT_CONTAINS("Rejoining Cluster into its original ClusterSet...");

status = old_primary.status();
EXPECT_EQ("Cluster was invalidated by the ClusterSet it belongs to. 2 members are not active.", status["defaultReplicaSet"]["statusText"]);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"]);

// put the cluster back
testutil.startSandbox(__mysql_sandbox_port3);

shell.connect(__sandbox_uri4);

cs = dba.getClusterSet();
cs.rejoinCluster("cluster");

EXPECT_NO_THROWS(function(){ old_primary.rejoinInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function(){ old_primary.rejoinInstance(__sandbox_uri3); });

cluster = dba.getCluster();

CHECK_PRIMARY_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster, old_primary);
CHECK_CLUSTER_SET(session);

testutil.waitMemberTransactions(__mysql_sandbox_port1, __mysql_sandbox_port4);

//@<> FR3.2 The 'force' option must be forbidden when the Cluster belongs to a ClusterSet and is INVALIDATED.

//Shutdown the primary
disable_auto_rejoin(__mysql_sandbox_port4);
disable_auto_rejoin(__mysql_sandbox_port5);
disable_auto_rejoin(__mysql_sandbox_port6);

testutil.killSandbox(__mysql_sandbox_port4);
testutil.killSandbox(__mysql_sandbox_port5);
testutil.killSandbox(__mysql_sandbox_port6);

//Failover to the replica cluster
shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();
cs.forcePrimaryCluster("cluster");

//Reboot the old-primary cluster
testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port5);
testutil.startSandbox(__mysql_sandbox_port6);

shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port5, "(MISSING)");
testutil.waitMemberState(__mysql_sandbox_port6, "(MISSING)");

EXPECT_THROWS(function() {
    old_primary = dba.rebootClusterFromCompleteOutage("replica", {force: true});
}, "The 'force' option cannot be used in a Cluster that belongs to a ClusterSet and is INVALIDATED.");

// put the cluster back
EXPECT_NO_THROWS(function(){ old_primary = dba.rebootClusterFromCompleteOutage("replica"); });

shell.connect(__sandbox_uri1);

cs = dba.getClusterSet();
EXPECT_NO_THROWS(function(){ cs.rejoinCluster("replica"); });

// Add some data to the primary Cluster
session.runSql("create schema test");
session.runSql("create table test.data (a int primary key auto_increment, data longtext)");
for (i = 0; i < 20; i++) {
    session.runSql("insert into test.data values (default, repeat('x', 4*1024*1024))");
}

shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function(){ old_primary.rejoinInstance(__sandbox_uri5); });
testutil.waitMemberState(__mysql_sandbox_port5, "ONLINE");
EXPECT_NO_THROWS(function(){ old_primary.rejoinInstance(__sandbox_uri6); });
testutil.waitMemberState(__mysql_sandbox_port6, "ONLINE");

shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], cluster, old_primary);
CHECK_CLUSTER_SET(session);

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

//@<> FR3.2 The 'force' option is allowed if the Cluster belongs to a ClusterSet and is an INVALIDATED replica.

shell.connect(__sandbox_uri4);
replica = dba.getCluster();
replica.removeInstance(__sandbox_uri6);
testutil.waitMemberState(__mysql_sandbox_port6, "OFFLINE (MISSING)");

cs.createReplicaCluster(__sandbox_uri6, "replica2");

disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);
testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);

disable_auto_rejoin(__mysql_sandbox_port4);
disable_auto_rejoin(__mysql_sandbox_port5);
testutil.killSandbox(__mysql_sandbox_port4);
testutil.killSandbox(__mysql_sandbox_port5);

shell.connect(__sandbox_uri6);
cs = dba.getClusterSet();

EXPECT_NO_THROWS(function(){ cs.forcePrimaryCluster("replica2", {invalidateReplicaClusters:["replica"]}); });

testutil.startSandbox(__mysql_sandbox_port5);
testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port3);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);

EXPECT_THROWS(function(){
    cluster = dba.rebootClusterFromCompleteOutage("cluster", {force: true});
}, "The 'force' option cannot be used in a Cluster that belongs to a ClusterSet and is INVALIDATED.");

EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage("cluster"); });

cs.rejoinCluster("cluster");

cluster = dba.getCluster();
cluster.rejoinInstance(__sandbox_uri2);
cluster.rejoinInstance(__sandbox_uri3);

shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica"); });

EXPECT_OUTPUT_CONTAINS("Skipping rejoining remaining instances because the Cluster belongs to a ClusterSet and is INVALIDATED. Please add or remove the instances after the Cluster is rejoined to the ClusterSet.");
EXPECT_OUTPUT_NOT_CONTAINS("Rejoining Cluster into its original ClusterSet...");

EXPECT_NO_THROWS(function(){ cs.rejoinCluster("replica"); });

wait_channel_ready(session, __mysql_sandbox_port6, "clusterset_replication");

EXPECT_NO_THROWS(function(){ replica.rejoinInstance(__sandbox_uri5); });

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port6);
testutil.waitMemberTransactions(__mysql_sandbox_port5, __mysql_sandbox_port4);

// Rejoin creates a VCLE, so let's reconcile the GTID-set already
cs.rejoinCluster("replica");

shell.connect(__sandbox_uri6);
replica2 = dba.getCluster();

CHECK_PRIMARY_CLUSTER([__sandbox_uri6], replica2);
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], replica2, cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], replica2, replica);
CHECK_CLUSTER_SET(session);

// put the cluster back again
cs.setPrimaryCluster("cluster");
cs.removeCluster("replica2");

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port6, "OFFLINE");

shell.connect(__sandbox_uri4);
replica = dba.getCluster();
replica.addInstance(__sandbox_uri6);

testutil.waitMemberTransactions(__mysql_sandbox_port6, __mysql_sandbox_port4);

shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], cluster, replica);
CHECK_CLUSTER_SET(session);

//@<> FR6: The command must automatically rejoin a Replica Cluster to its ClusterSet by ensuring the ClusterSet replication channel is configured in all members of the Cluster.

testutil.stopGroup([__mysql_sandbox_port4, __mysql_sandbox_port5, __mysql_sandbox_port6]);

shell.connect(__sandbox_uri4);

EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica", {dryRun: true}); });
EXPECT_OUTPUT_CONTAINS("NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.");
EXPECT_OUTPUT_CONTAINS("Rejoining Cluster into its original ClusterSet...");
EXPECT_OUTPUT_CONTAINS("dryRun finished.");
EXPECT_OUTPUT_NOT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port5}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_NOT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port6}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_NOT_CONTAINS("Cluster 'replica' was rejoined to the clusterset");
EXPECT_OUTPUT_NOT_CONTAINS("The Cluster was successfully rebooted.");

testutil.wipeAllOutput();

EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica"); });
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port5}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port6}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_CONTAINS("Rejoining Cluster into its original ClusterSet...");
EXPECT_OUTPUT_CONTAINS("Cluster 'replica' was rejoined to the clusterset");
EXPECT_OUTPUT_CONTAINS("The Cluster was successfully rebooted.");

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], cluster, replica);
CHECK_CLUSTER_SET(session);

//@<> FR6: Reboot Replica by connecting to a secondary member

testutil.stopGroup([__mysql_sandbox_port4, __mysql_sandbox_port5, __mysql_sandbox_port6]);

shell.connect(__sandbox_uri5);

EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica"); });
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port4}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port6}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_CONTAINS("Rejoining Cluster into its original ClusterSet...");
EXPECT_OUTPUT_CONTAINS("Cluster 'replica' was rejoined to the clusterset");
EXPECT_OUTPUT_CONTAINS("The Cluster was successfully rebooted.");

testutil.waitMemberTransactions(__mysql_sandbox_port5, __mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri5, __sandbox_uri4, __sandbox_uri6], cluster, replica);
CHECK_CLUSTER_SET(session);

//<> FR6.1.1: The Replica Cluster must not have been forcefully removed from its ClusterSet.

disable_auto_rejoin(__mysql_sandbox_port4);
disable_auto_rejoin(__mysql_sandbox_port5);
disable_auto_rejoin(__mysql_sandbox_port6);

testutil.killSandbox(__mysql_sandbox_port4);
testutil.killSandbox(__mysql_sandbox_port5);
testutil.killSandbox(__mysql_sandbox_port6);

shell.connect(__sandbox_uri1);

cset = dba.getClusterSet();
cset.removeCluster("replica", {force: true});

testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port5);
testutil.startSandbox(__mysql_sandbox_port6);

testutil.wipeAllOutput();

shell.connect(__sandbox_uri4);

EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica", {dryRun: true}); });
EXPECT_OUTPUT_CONTAINS("NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.");
EXPECT_OUTPUT_CONTAINS("dryRun finished.");
EXPECT_OUTPUT_NOT_CONTAINS("Rejoining Cluster into its original ClusterSet...");

testutil.wipeAllOutput();

EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica"); });
EXPECT_OUTPUT_NOT_CONTAINS("Rejoining Cluster into its original ClusterSet...");

status = replica.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port4}`]["status"]);
EXPECT_EQ_ONEOF(["ONLINE", "RECOVERING"], status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port5}`]["status"]);
EXPECT_EQ_ONEOF(["ONLINE", "RECOVERING"], status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port6}`]["status"]);

// put the cluster set back
replica.dissolve();

testutil.waitMemberState(__mysql_sandbox_port4, "OFFLINE (MISSING)");
testutil.waitMemberState(__mysql_sandbox_port5, "OFFLINE (MISSING)");
testutil.waitMemberState(__mysql_sandbox_port6, "OFFLINE (MISSING)");

EXPECT_NO_THROWS(function(){ replica = cset.createReplicaCluster(__sandbox_uri4, "replica", {recoveryMethod: "clone"}); });
EXPECT_NO_THROWS(function(){ replica.addInstance(__sandbox_uri5, {recoveryMethod: "clone"}); });
EXPECT_NO_THROWS(function(){ replica.addInstance(__sandbox_uri6, {recoveryMethod: "clone"}); });

//<> FR6.1.2: The ClusterSet's Primary Cluster must be reachable and available

disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);

testutil.stopGroup([__mysql_sandbox_port4, __mysql_sandbox_port5, __mysql_sandbox_port6]);

shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica"); });
EXPECT_OUTPUT_CONTAINS("Skipping rejoining remaining instances because the Cluster belongs to a ClusterSet and its global status is not OK. Please add or remove the instances after the Cluster is rejoined to the ClusterSet.");
EXPECT_OUTPUT_CONTAINS("WARNING: Unable to rejoin Cluster to the ClusterSet (primary Cluster is unreachable). Please call ClusterSet.rejoinCluster() to manually rejoin this Cluster back into the ClusterSet.");

status = replica.status();
EXPECT_EQ("Cluster is NOT tolerant to any failures. 2 members are not active.", status["defaultReplicaSet"]["statusText"]);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port4}`]["status"]);

testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage("cluster"); });

cset = dba.getClusterSet();
cset.status({extended:1});

EXPECT_NO_THROWS(function(){ cset.rejoinCluster("replica"); });

EXPECT_NO_THROWS(function(){ replica.rejoinInstance(__sandbox_uri5); });
EXPECT_NO_THROWS(function(){ replica.rejoinInstance(__sandbox_uri6); });

cset.status({extended:1});

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], cluster, replica);
CHECK_CLUSTER_SET(session);

//@<> Rebooting a Replica Cluster should succeed to rejoin back its members even though there are missing transactions from the Primary Cluster
testutil.stopGroup([__mysql_sandbox_port4, __mysql_sandbox_port5, __mysql_sandbox_port6]);

shell.connect(__sandbox_uri1);

// Add some data to the Primary Cluster
session.runSql("create schema test2");
session.runSql("create table test2.data (a int primary key auto_increment, data longtext)");
for (i = 0; i < 20; i++) {
    session.runSql("insert into test2.data values (default, repeat('x', 4*1024*1024))");
}

shell.connect(__sandbox_uri4);

EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage(); });
testutil.waitMemberState(__mysql_sandbox_port5, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port6, "ONLINE");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

testutil.waitMemberState(__mysql_sandbox_port5, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port6, "ONLINE");

CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], cluster, replica);
CHECK_CLUSTER_SET(session);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
