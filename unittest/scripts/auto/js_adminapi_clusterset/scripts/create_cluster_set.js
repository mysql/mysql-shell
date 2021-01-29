//@ {VER(>=8.0.27)}

// Tests createClusterSet() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

// enable interactive by default
shell.options['useWizards'] = true;

//@<> INCLUDE clusterset_utils.inc

// Negative tests based on environment and params
//-----------------------------------------------

//@<> Setup + Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var session = scene.session
var cluster = scene.cluster

//@<> Bad options (should fail)
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet()}, "Invalid number of arguments, expected 1 to 2 but got 0", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet(null)}, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet(123)}, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("foo bar")}, "ClusterSet name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (foo bar)", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("my::clusterset")}, "ClusterSet name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (my::clusterset)", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("")}, "The ClusterSet name cannot be empty.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("1234567890123456789012345678901234567890123456789012345678901234")}, "The ClusterSet name can not be greater than 63 characters", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("testCS", {clusterSetReplicationSslMode: 1})}, "Argument #2: Option 'clusterSetReplicationSslMode' is expected to be of type String, but is Integer", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("testCS", {clusterSetReplicationSslMode: "YES"})}, "Argument #2: Invalid value for clusterSetReplicationSslMode option. Supported values: AUTO,DISABLED,REQUIRED.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("testCS", {invalidOption: true})}, "Argument #2: Invalid options: invalidOption", "ArgumentError");

//@<> On a disconnected Cluster
cluster.disconnect();
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("testCS")}, "The cluster object is disconnected. Please use dba.getCluster to obtain a fresh cluster handle.", "RuntimeError");
cluster = dba.getCluster()

// Preconditions to become a ClusterSet

// FR2.5: The Cluster is not running in multi-primary mode.
//@<> The cluster must not be multi-primary
cluster.switchToMultiPrimaryMode()
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("testCS")}, "Unsupported topology-mode: multi-primary.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: The cluster is running in multi-primary mode that is not supported in InnoDB ClusterSet.");
cluster.switchToSinglePrimaryMode()

// FR2.4: Unknown replication channels are not allowed at any member of the target Cluster.
//@<> The cluster must not have replication channels configured
var session2 = mysql.getSession(__sandbox_uri2);
setup_replica(session2, __mysql_sandbox_port1);
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("testCS")}, "Unsupported active replication channel.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: Cluster member '${hostname}:${__mysql_sandbox_port2}' has asynchronous (source-replica) replication channel(s) configured which is not supported in InnoDB ClusterSet.`);

session2.runSql("STOP REPLICA FOR CHANNEL ''");
session2.runSql("RESET REPLICA ALL FOR CHANNEL ''");

// FR2.1: The Cluster must be available (status OK/OK_PARTIAL/OK_NO_TOLERANCE/OK_NO_TOLERANCE_PARTIAL)
// and the PRIMARY instance must be reachable.
//@<> The cluster must be available
scene.make_no_quorum([__mysql_sandbox_port1]);
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("testCS")}, "Invalid Cluster status: NO_QUORUM.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("Target cluster status is 'NO_QUORUM' which is not valid for InnoDB ClusterSet.");

// TODO: FR2.2: All Cluster members are running MySQL version >= 8.0.27.
// TODO: FR2.3: The Cluster's Metadata is compatible: version >= 2.1.0.

//@<> Restore back the cluster
cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//FR2.6: The Cluster must have group_replication_view_change_uuid set and stored in the Metadata, otherwise fail with an error
//indicating to run <Cluster>.rescan() to fix it.

//@<> The Cluster must have group_replication_view_change_uuid set and stored in the Metadata schema
var view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
EXPECT_NE(view_change_uuid, "AUTOMATIC");
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_REMOVE(attributes, '$.group_replication_view_change_uuid')");
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("testCS")}, "group_replication_view_change_uuid not configured", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("The cluster's group_replication_view_change_uuid is not stored in the Metadata. Please use <Cluster>.rescan() to update the metadata.");

// Revert the removal of group_replication_view_change_uuid from the Metadata
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = JSON_SET(attributes, '$.group_replication_view_change_uuid', '" +view_change_uuid + "')");

//@<> Create ClusterSet - dryRun
EXPECT_NO_THROWS(function(){cs = cluster.createClusterSet("testCS", {dryRun: 1})});

EXPECT_EQ(null, cs);

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
A new ClusterSet will be created based on the Cluster 'cluster'.

* Validating Cluster 'cluster' for ClusterSet compliance.

NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.
* Creating InnoDB ClusterSet 'testCS' on 'cluster'...

* Updating metadata...
dryRun finished.
`);

//@<> Create ClusterSet - OK
EXPECT_NO_THROWS(function(){cluster.createClusterSet("testCS")});

//@<> Cluster must not be part of a ClusterSet
EXPECT_THROWS_TYPE(function(){cluster.createClusterSet("testCS")}, "This function is not available through a session to an InnoDB Cluster that belongs to an InnoDB ClusterSet", "MYSQLSH");

//FR3.4: It must ensure no replication threads are automatically started on any member of the target Cluster, when restarted,by enabling skip_replica_start.
var skip_replica_start = session.runSql("SELECT variable_value FROM performance_schema.persisted_variables WHERE variable_name = 'skip_replica_start'").fetchOne()[0];
EXPECT_EQ("ON", skip_replica_start);

//@<> Cleanup
scene.destroy();
