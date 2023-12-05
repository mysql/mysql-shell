//@ {VER(>=8.0.23)}

//@<> INCLUDE read_replicas_utils.inc

//@<> Setup + Create cluster + add read-replicas
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], {"manualStartOnBoot": true});
var session = scene.session
var cluster = scene.cluster

testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
var session3 = mysql.getSession(__sandbox_uri4);
var session4 = mysql.getSession(__sandbox_uri4);

// Add a Read-Replica with defaults
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });

testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "ON");

//@<> removeInstance() - Bad options (should fail)
EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__sandbox_uri3, {dryRun: "yes"})}, "Argument #2: Option 'dryRun' Bool expected, but value is String", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__sandbox_uri3, {timeout: "yes"})}, "Argument #2: Option 'timeout' Integer expected, but value is String", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__sandbox_uri3, {timeout: -1})}, "Argument #2 timeout option must be >= 0", "ArgumentError");

// Instance does not belong to the Cluster
EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__sandbox_uri4)}, "Cluster.removeInstance: Metadata for instance localhost:" +__mysql_sandbox_port4 + " not found", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance localhost:${__mysql_sandbox_port4} does not belong to the cluster.`);

//@<> Account password doesn't match (should fail)
__sandbox_uri3_wrong_pwd = "mysql://root:wrong@localhost:" + __mysql_sandbox_port3;

EXPECT_THROWS(function () { cluster.removeInstance(__sandbox_uri3_wrong_pwd)}, `Could not open connection to 'localhost:${__mysql_sandbox_port3}': Access denied for user 'root'@'localhost'`);

// --- Positive tests

// Add a Read-Replica with replicationSources: __endpoint2, __endpoint1
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint2, __endpoint1]}); });

testutil.waitReplicationChannelState(__mysql_sandbox_port4, "read_replica_replication", "ON");

//@<> removeInstance(read_replica) - dryRun
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri3, {dryRun: 1}); });

EXPECT_OUTPUT_CONTAINS(`Removing Read-Replica '${__endpoint3}' from the Cluster 'cluster'.`);
EXPECT_OUTPUT_CONTAINS(`* Waiting for the Read-Replica to synchronize with the Cluster...`);
EXPECT_OUTPUT_CONTAINS(`* Stopping and deleting the Read-Replica managed replication channel...`);
EXPECT_OUTPUT_CONTAINS(`Read-Replica '${__endpoint3}' successfully removed from the Cluster 'cluster'.`)
EXPECT_OUTPUT_CONTAINS(`dryRun finished.`);

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

//@<> removeInstance(secondary) - dryRun
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri2, {dryRun: 1}); });

EXPECT_OUTPUT_CONTAINS(`The instance will be removed from the InnoDB Cluster.`)
EXPECT_OUTPUT_CONTAINS(`* Waiting for instance '${__endpoint2}' to synchronize with the primary...`);
EXPECT_OUTPUT_CONTAINS(`The instance 'localhost:${__mysql_sandbox_port2}' was successfully removed from the cluster.`);
EXPECT_OUTPUT_CONTAINS(`dryRun finished.`);

//@<> removeInstance(read_replica) - replicationSources: primary - using defaults - success
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri3); });

CHECK_REMOVED_READ_REPLICA(__sandbox_uri3, cluster);

//@<> removeInstance(read_replica) - replicationSources: list - using defaults - success
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri4); });

CHECK_REMOVED_READ_REPLICA(__sandbox_uri4, cluster);

// Re-add the Read-Replicas back to the Cluster
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });
testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "ON");

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint2, __endpoint1]}); });
testutil.waitReplicationChannelState(__mysql_sandbox_port4, "read_replica_replication", "ON");

// --- timeout tests

// FR6.3.2: timeout: maximum number of seconds to wait for the instance to sync up with the PRIMARY. If reached, the operation is rolled-back. Default is unlimited (zero).

//@<> Attempt to remove a read-replica but with limited timeout
session1 = mysql.getSession(__sandbox_uri1);
session3 = mysql.getSession(__sandbox_uri3);
session1.runSql("create schema if not exists testing");
session1.runSql("create table if not exists testing.data (k int primary key auto_increment, v longtext)");
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);
session3.runSql("lock tables testing.data read");
session1.runSql("insert into testing.data values (default, repeat('#', 10))");

EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__sandbox_uri3, {timeout: 2})}, `Timeout reached waiting for transactions from ${hostname}:${__mysql_sandbox_port1} to be applied on instance '${__endpoint3}'`, "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${__endpoint3}' failed to synchronize its transaction set with the Cluster. There might be too many transactions to apply or some replication error. You may increase the transaction sync timeout with the option 'timeout' or use the 'force' option to ignore the timeout, however, it might leave the instance in an inconsistent state and can lead to errors if you want to reuse it.`);

// Confirm the rollback was done
CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

// unlock the table to allow the sync
session3.runSql("unlock tables");

//@<> Attempt to remove the read-replica but with unlimited timeout
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri3, {timeout: 0}); });

CHECK_REMOVED_READ_REPLICA(__sandbox_uri3, cluster);

// Re-add the Read-Replica back to the Cluster
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });
testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "ON");

// --- force tests

// Shutdown the instance to make the read-replica unreachable
testutil.killSandbox(__mysql_sandbox_port3);

EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__endpoint3)}, "Can't connect to MySQL server on '" + __endpoint3 + "'", "MySQL Error");

EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to connect to the target instance '${__endpoint3}'. Please verify the connection settings, make sure the instance is available and try again.`);
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${__endpoint3}' is not reachable and cannot be safely removed from the cluster.
To safely remove the instance from the Cluster, make sure the instance is back ONLINE and try again. If you are sure the instance is permanently unable to rejoin the Cluster and no longer connectable, use the 'force' option to remove it from the metadata.`);

//@<> Attempt to remove the unreachable read-replica with force: false
EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__endpoint3, {force: false})}, "Can't connect to MySQL server on '" + __endpoint3 + "'", "MySQL Error");

EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to connect to the target instance '${__endpoint3}'. Please verify the connection settings, make sure the instance is available and try again.`);
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${__endpoint3}' is not reachable and cannot be safely removed from the cluster.
To safely remove the instance from the Cluster, make sure the instance is back ONLINE and try again. If you are sure the instance is permanently unable to rejoin the Cluster and no longer connectable, use the 'force' option to remove it from the metadata.`);

//@<> Attempt to remove the unreachable read-replica with interactive
shell.options.useWizards=1;
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)? [y/N]: ", "n");
EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__endpoint3)}, "Can't connect to MySQL server on '" + __endpoint3 + "'", "MySQL Error");
shell.options.useWizards=0;

//@<> Attempt to remove the unreachable read-replica with force: true
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint3, {force: 1}); });
EXPECT_OUTPUT_CONTAINS(`NOTE: The instance '${__endpoint3}' is not reachable and it will only be removed from the metadata. Please take any necessary actions to ensure the instance will not rejoin the cluster if brought back online.`);

// Check that the instance was removed from the Cluster's metadata
var r = session1.runSql("select address FROM mysql_innodb_cluster_metadata.instances WHERE address = '" + __endpoint3 + "'").fetchOne();

EXPECT_EQ(null, r, uri+".force_removal")

// It should be possible to add the instance back to the Cluster
testutil.startSandbox(__mysql_sandbox_port3);

//@<> Attempt to add the read-replica that was forcefully removed back to the cluster
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint3); });

// --- Re-use a removed read-replica

// Remove the read-replica
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint3); });

//@<> Attempt to add the read-replica as a cluster secondary
EXPECT_NO_THROWS(function() { cluster.addInstance(__endpoint3); });

// Remove it
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint3); });

// Add the instance back as a read-replica
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint3); });

// Remove the read-replica
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint3); });

//@<> Attempt to create a Replica Cluster on the read-replica
EXPECT_NO_THROWS(function() { cs = cluster.createClusterSet("cs"); });

EXPECT_NO_THROWS(function() { cs.createReplicaCluster(__sandbox_uri3, "replica_cluster"); });

// --- tests for replicationSources list automatically updated

// Remove replica cluster
EXPECT_NO_THROWS(function() { cs.removeCluster("replica_cluster"); });

// Add instance 3 as a secondary
EXPECT_NO_THROWS(function() { cluster.addInstance(__endpoint3); });

testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

// Add instance 5 as a read-replica with all cluster members as replicationSources
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri5, {replicationSources: [__endpoint1, __endpoint2, __endpoint3]}); });

// Remove instance2 from the Cluster and verify the replicationSources are updates on all read-replicas

//@<> Removing a cluster secondary must ensure the read-replicas are updated to remove it from its replicationSources
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint2); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint1]);
CHECK_READ_REPLICA(__sandbox_uri5, cluster, [__endpoint1, __endpoint3]);

//@<> Removing a cluster secondary acting as source for an unreachable read-replica must not update the replicationSources and print a warning

// Make read-replica at instance5 unreachable
testutil.killSandbox(__mysql_sandbox_port5)

EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint3); });

EXPECT_OUTPUT_CONTAINS(`WARNING: Unable to update the sources of Read Replica '${__endpoint5}': instance is unreachable. Use Cluster.setInstanceOption() with the option 'replicationSources' to update the instance's sources.`);

// Bring the read-replica back online
testutil.startSandbox(__mysql_sandbox_port5)

// Confirm that instance3 wasn't removed from the replicationSources
CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint1]);
CHECK_READ_REPLICA(__sandbox_uri5, cluster, [__endpoint1, __endpoint3]);

// Set-up the Cluster to have a secondary member and 2 read-replicas with that secondary as the only source

EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint4); });
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint5); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__endpoint2); });

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint2]}); });

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri5, {replicationSources: [__endpoint2]}); });

EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri2); });

EXPECT_OUTPUT_CONTAINS(`* Removing source '${__endpoint2}' from the replicationSources of '${__endpoint4}'`);
EXPECT_OUTPUT_CONTAINS(`WARNING: The Read Replica '${__endpoint4}' doesn't have any available replication sources. Use Cluster.setInstanceOption() with the option 'replicationSources' to update the instance's sources and rejoin it back to the Cluster using Cluster.rejoinInstance()`);
EXPECT_OUTPUT_CONTAINS(`* Removing source '${__endpoint2}' from the replicationSources of '${__endpoint5}'`);
EXPECT_OUTPUT_CONTAINS(`WARNING: The Read Replica '${__endpoint5}' doesn't have any available replication sources. Use Cluster.setInstanceOption() with the option 'replicationSources' to update the instance's sources and rejoin it back to the Cluster using Cluster.rejoinInstance()`);

// Confirm that the replicationSources of the read-replicas are empty and the replication channels are stopped
CHECK_READ_REPLICA(__sandbox_uri4, cluster, [], undefined, false, true);
CHECK_READ_REPLICA(__sandbox_uri5, cluster, [], undefined, false, true);

//@<> Removing a Read-Replica that got the replication channel reset should fail if the force option is not used
EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__endpoint4)}, "Read-Replica Replication channel does not exist", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The Read-Replica Replication channel could not be found. Use the 'force' option to ignore this check.`);

//@<> Removing a Read-Replica that got the replication channel reset should succeed if the force option is used
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint4, {force: true}); });
EXPECT_OUTPUT_CONTAINS(`WARNING: Ignoring non-existing Read-Replica Replication channel because of 'force' option`);

CHECK_REMOVED_READ_REPLICA(__sandbox_uri4, cluster);

//@<> Removing a Read-Replica that got the replication channel stopped should fail if the force option is not used

// Add read-replica 4 back to the cluster
session4 = mysql.getSession(__sandbox_uri4);
reset_instance(session4);
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint4); });

// Stop the channel
session4.runSql("STOP REPLICA")

EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__endpoint4)}, "Read-Replica Replication channel not in expected state", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The Read-Replica Replication channel has an invalid state: 'OFF'. Use the 'force' option to ignore this check.`);

//@<> Removing a Read-Replica that got the replication channel stopped should succeed if the force option is used
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint4, {force: true}); });
EXPECT_OUTPUT_CONTAINS(`WARNING: The Read-Replica Replication channel has an invalid state: 'OFF'. Ignoring because of 'force' option`);

CHECK_REMOVED_READ_REPLICA(__sandbox_uri4, cluster);

//@<> getCluster() from a read-replica in error/offline state should succeed
shell.connect(__sandbox_uri5);
EXPECT_NO_THROWS(function() { dba.getCluster(); });

//@<> Adding back to the Cluster a Read-Replica that was removed with force should be possible
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint4); });

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
