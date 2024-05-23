//@ {VER(>=8.0.27)}

//@<> INCLUDE gr_utils.inc

//@<> Initialization
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], {gtidSetIsComplete: true});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});

cluster = scene.cluster;
scene.session.close();

cset = cluster.createClusterSet("cset");

shell.connect(__sandbox_uri1);

rcluster = cset.createReplicaCluster(__sandbox_uri3, "rcluster", {recoveryMethod:"clone"});

//@<> BUG #33235502 Make sure that account errors aren't shown and/or fixed in replica clusters

shell.connect(__sandbox_uri3);

rcluster.addInstance(__sandbox_uri4, {recoveryMethod: "clone", waitRecovery:0});
testutil.waitMemberState(__mysql_sandbox_port4, "ONLINE");

cset.setPrimaryCluster("rcluster");

session4 = mysql.getSession(__sandbox_uri4);
var instance4_id = session4.runSql("SELECT @@server_id").fetchOne()[0];
session4.close();

// cluster should not show any erros, but replica should
status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);

status = rcluster.status();
EXPECT_TRUE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"].length, 1);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"][0], `WARNING: Incorrect recovery account (mysql_innodb_cluster_${instance4_id}) being used. Use Cluster.rescan() to repair.`);

// this must not change anything
WIPE_STDOUT();
cluster.rescan();
EXPECT_OUTPUT_NOT_CONTAINS("Dropping unused recovery account: ");
EXPECT_OUTPUT_NOT_CONTAINS("Fixing incorrect recovery account 'mysql_innodb_cluster_");

// confirm that everything is the same
status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);

status = rcluster.status();
EXPECT_TRUE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"].length, 1);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"][0], `WARNING: Incorrect recovery account (mysql_innodb_cluster_${instance4_id}) being used. Use Cluster.rescan() to repair.`);

// fix the problem
WIPE_STDOUT();
rcluster.rescan();
EXPECT_OUTPUT_CONTAINS(`Fixing incorrect recovery account 'mysql_innodb_cluster_${instance4_id}' in instance '${hostname}:${__mysql_sandbox_port3}'`)

// confirm that everything is correct
status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);

status = rcluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]);

//@<> rescan() should detect inconsistencies in the ClusterSet metadata {!__dbug_off}

// All metadata updated are atomic, to ensure consistency. The operations are
// executed inside transactions:
//   - Create topology (Cluster, ReplicaSet, ClusterSet)
//   - Add instances to the topology
//   - Remove instances from the topology
//   - etc.
//
// However, there is 1 exception:
//   - Adding a new Replica Cluster to a ClusterSet
//
//  That operations is split into 2 transactions: Creating a new Cluster (with its Metadata entries) and adding that Cluster to the ClusterSet's metadata.
//  If one fails, the Metadata is inconsistent.

// Remove the Replica Cluster
cset.removeCluster("cluster");

// Set the debug trap to the point in time right before calling
// v2_cs_member_added and to fail the undo
testutil.dbugSet("+d,dba_create_replica_cluster_fail_setup_cs_settings");
testutil.dbugSet("+d,dba_create_replica_cluster_fail_revert");

// Create a Replica Cluster
cset = dba.getClusterSet();
EXPECT_THROWS_TYPE(function(){ cluster = cset.createReplicaCluster(__sandbox_uri1, "cluster", {recoveryMethod:"clone"}); }, "debug", "LogicError");

// Retrying the operation will fail
EXPECT_THROWS(function() { cset.createReplicaCluster(__sandbox_uri1, "cluster", {recoveryMethod:"clone"}); }, "Target instance already part of an InnoDB Cluster", "MYSQLSH");

// Unset the debug trap
testutil.dbugSet("");

// rescan() must fix the Metadata inconsistencies
primary = dba.getCluster();

EXPECT_NO_THROWS(function() { primary.rescan(); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
WARNING: The Cluster 'cluster' was detected in the ClusterSet Metadata, but it does not appear to be a valid member of the ClusterSet.

Result of the rescanning operation for the 'rcluster' cluster:
{
    "metadataConsistent": false,
    "name": "rcluster",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": []
}

WARNING: Metadata inconsistencies detected, possibly due to one or more failed commands. Please review the topology status before proceeding.
Use the 'repairMetadata' option to repair the Metadata inconsistencies.
`);
WIPE_STDOUT();

// Dissolve the dangling Cluster
shell.connect(__sandbox_uri1);
dangling = dba.getCluster();

EXPECT_NO_THROWS(function() { dangling.dissolve(); });

// Test the prompt
shell.options["useWizards"] = true;

testutil.expectPrompt("The invalid Cluster(s) listed above will be removed from the metadata. Do you want to proceed? [Y/n]:", "n");

EXPECT_NO_THROWS(function() { primary.rescan(); });

shell.options["useWizards"] = false;

// Repair the MD inconsistencies
EXPECT_NO_THROWS(function() { primary.rescan({repairMetadata: true}); });

EXPECT_OUTPUT_CONTAINS("Repairing Metadata inconsistencies...");
EXPECT_SHELL_LOG_CONTAINS(`Removing Cluster 'cluster' from the Metadata.`);
EXPECT_SHELL_LOG_CONTAINS(`Removing Instance '${__endpoint1}' from the Metadata.`);
WIPE_STDOUT();
WIPE_SHELL_LOG();

EXPECT_NO_THROWS(function() { primary.rescan(); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
Result of the rescanning operation for the 'rcluster' cluster:
{
    "metadataConsistent": true,
    "name": "rcluster",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": []
}
`)

// Retry the createReplicaCluster() using the instance from the dangling cluster
EXPECT_NO_THROWS(function() { cset.createReplicaCluster(__sandbox_uri1, "cluster", {recoveryMethod:"clone"}); });

//@<> Cleanup
session.close()
scene.destroy();

testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
