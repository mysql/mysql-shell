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

//@<> rejoinInstance(read_replica) - Bad options (should fail)
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri3, {dryRun: "yes"})}, "Argument #2: Option 'dryRun' Bool expected, but value is String", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri3, {timeout: "yes"})}, "Argument #2: Option 'timeout' Integer expected, but value is String", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri3, {timeout: -1})}, "Argument #2 timeout option must be >= 0", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri3, {recoveryMethod: -1})}, "Argument #2: Option 'recoveryMethod' is expected to be of type String, but is Integer", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri3, {recoveryProgress: "foo"})}, "Argument #2: Option 'recoveryProgress' Integer expected, but value is String", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri3, {cloneDonor: -1})}, "Argument #2: Option 'cloneDonor' is expected to be of type String, but is Integer", "TypeError");

// Instance does not belong to the Cluster
EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri4)}, "The instance '" + __endpoint4 + "' does not belong to the cluster: 'cluster'.", "RuntimeError");

// --- Positive tests

// Stop replication on read-replica at instance3
var session3 = mysql.getSession(__sandbox_uri3);

session3.runSql("STOP replica");
testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "OFF");

var status = cluster.status();
print(status);

EXPECT_EQ("WARNING: Read Replica's replication channel is stopped. Use Cluster.rejoinInstance() to restore it.", status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"][0]);
EXPECT_EQ("OFFLINE", status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["status"]);

//@<> rejoinInstance(read_replica) - using defaults - success
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"], undefined);

//@<> rejoinInstance(read_replica) - channel stopped and user dropped - success
var instance3_uuid = session3.runSql("select @@global.server_uuid").fetchOne()[0];

var rr3_replication_account = session.runSql("select (attributes->>'$.readReplicaReplicationAccountUser') from mysql_innodb_cluster_metadata.v2_instances where mysql_server_uuid = '" + instance3_uuid + "'").fetchOne()[0];

var session1 = mysql.getSession(__sandbox_uri1);
session1.runSql("drop user " + rr3_replication_account);

session3.runSql("STOP replica");
testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "OFF");
session3.runSql("START replica");

var status = cluster.status();
print(status);

EXPECT_CONTAINS(`WARNING: Read Replica's replication channel stopped with a connection error: 'Error connecting to source '${rr3_replication_account}@${__endpoint1}'. This was attempt 1/10, with a delay of 3 seconds between attempts. Message: Access denied for user '${rr3_replication_account}'`, status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"][0]);
EXPECT_CONTAINS("Use Cluster.rejoinInstance() to restore it.", status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"][0])
EXPECT_EQ("ERROR", status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["status"]);

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"], undefined);


//@<> rejoinInstance(read_replica) - channel stopped and user password mismatch - success
session.runSql(`ALTER USER '${rr3_replication_account}'@'%' IDENTIFIED BY 'password123'`);

session3.runSql("STOP replica");
testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "OFF");
session3.runSql("START replica");

var status = cluster.status();
print(status);

EXPECT_CONTAINS(`WARNING: Read Replica's replication channel stopped with a connection error: 'Error connecting to source '${rr3_replication_account}@${__endpoint1}'. This was attempt 1/10, with a delay of 3 seconds between attempts. Message: Access denied for user '${rr3_replication_account}'`, status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"][0]);
EXPECT_CONTAINS("Use Cluster.rejoinInstance() to restore it.", status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"][0])
EXPECT_EQ("ERROR", status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["status"]);

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"], undefined);

//@<> rejoinInstance(read_replica) - channel misconfigured - success
session3.runSql("STOP replica");
session3.runSql("change replication source to source_connection_auto_failover=0 for channel 'read_replica_replication'");
session3.runSql("START replica");

var status = cluster.status();
print(status);

EXPECT_EQ(`WARNING: Read Replica's replication channel is misconfigured: automatic failover is disabled. Use Cluster.rejoinInstance() to fix it.`, status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"][0]);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["status"]);

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"], undefined);

//@<> rejoinInstance(read_replica) - channel missing - success
session3.runSql("STOP replica");
session3.runSql("RESET REPLICA ALL FOR CHANNEL 'read_replica_replication'");

var status = cluster.status();
print(status);

EXPECT_EQ(`WARNING: Read Replica's replication channel is missing. Use Cluster.rejoinInstance() to restore it.`, status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"][0]);
EXPECT_EQ("OFFLINE", status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["status"]);

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"], undefined);

//@<> rejoinInstance(read_replica) - super_read_only disabled - success
session3.runSql("set global super_read_only=0");

var status = cluster.status();
print(status);

EXPECT_EQ(`WARNING: Instance is a Read-Replica but super_read_only option is OFF. Use Cluster.rejoinInstance() to fix it.`, status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"][0]);

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint3]["instanceErrors"], undefined);

//@<> rejoinInstance(read_replica) - errant trx - error
var errant = inject_errant_gtid(session3);
session3.runSql("STOP replica");

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri3); }, "Instance provisioning required", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`* Checking transaction state of the instance...`);
EXPECT_OUTPUT_CONTAINS(`WARNING: A GTID set check of the MySQL instance at '${hostname}:${__mysql_sandbox_port3}' determined that it contains transactions that do not originate from the cluster, which must be discarded before it can join the cluster.`);
EXPECT_OUTPUT_CONTAINS(`WARNING: Discarding these extra GTID events can either be done manually or by completely overwriting the state of ${hostname}:${__mysql_sandbox_port3} with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);
EXPECT_OUTPUT_CONTAINS(`Having extra GTID events is not expected, and it is recommended to investigate this further and ensure that the data can be removed prior to choosing the clone recovery method.`);
EXPECT_OUTPUT_CONTAINS(`ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target cluster.`);

// Reconcile the transaction set
inject_empty_trx(session1, errant);

// --- Options tests

//@<> rejoinInstance(read_replica) - wrong password in options - error
var __sandbox_uri3_wrong = "mysql://root:toor@localhost:3336";

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri3_wrong); }, "Invalid target instance specification", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them`);

//@<> rejoinInstance(read_replica) - prompt required - interactive false - error
shell.options.useWizards=1;

session3.runSql("reset master");

var session2 = mysql.getSession(__sandbox_uri2);

session1.runSql("FLUSH BINARY LOGS");
session1.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");
session2.runSql("FLUSH BINARY LOGS");
session2.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri3, {interactive: false}); }, "Instance provisioning required", "MYSQLSH");

//@<> rejoinInstance(read_replica) - interactive false, recoveryMethod: clone - success
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3, {interactive: false, recoveryMethod: "clone"}); });

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

//@<> rejoinInstance(read_replica) - dryRun
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP REPLICA");

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3, {dryRun: 1}); });

EXPECT_OUTPUT_CONTAINS(`Rejoining Read-Replica '${__endpoint3}' to Cluster 'cluster'.`);
EXPECT_OUTPUT_CONTAINS(`* Checking transaction state of the instance...
The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '${__endpoint3}' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.
`);
EXPECT_OUTPUT_CONTAINS(`The incremental state recovery may be safely used if you are sure all updates ever executed in the cluster were done with GTIDs enabled, there are no purged transactions and the new instance contains the same GTID set as the cluster or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.`);
EXPECT_OUTPUT_CONTAINS(`Incremental state recovery was selected because it seems to be safely usable.`);
EXPECT_OUTPUT_CONTAINS(`** Changing replication source of ${__endpoint3} to ${__endpoint1}`);
EXPECT_OUTPUT_CONTAINS(`Read-Replica '${__endpoint3}' successfully rejoined to the Cluster 'cluster'.`)
EXPECT_OUTPUT_CONTAINS(`dryRun finished.`);

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1, true);

shell.options.useWizards=0;

// --- preconditions tests

//@<> Attempt to rejoin an unreachable read-replica
testutil.killSandbox(__mysql_sandbox_port3);

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__endpoint3)}, "Could not open connection to '" + __endpoint3 + "': Can't connect to MySQL server on '" + __endpoint3 + "'", "MySQL Error");
EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to connect to the target instance '${__endpoint3}'. Please verify the connection settings, make sure the instance is available and try again.`);

//@<> Attempt to rejoin a read-replica from a different cluster

// Create new cluster on instance4
shell.connect(__sandbox_uri4);

EXPECT_NO_THROWS(function() { cluster2 = dba.createCluster("cluster2"); });

// Remove instance3 from cluster1 and add it to cluster2
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint3, {force: true}); });

testutil.startSandbox(__mysql_sandbox_port3);

// The transaction-set is divergent so clone must be used
EXPECT_NO_THROWS(function() { cluster2.addReplicaInstance(__sandbox_uri3, {recoveryMethod: "clone"}); });

CHECK_READ_REPLICA(__sandbox_uri3, cluster2, "primary", __endpoint4);

session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP replica");

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__endpoint3)}, "The instance '" + __endpoint3 + "' does not belong to the cluster: 'cluster'.", "RuntimeError");

//@<> Attempt to rejoin a read-replica without configured replicationSources
session3.runSql("START replica");

EXPECT_NO_THROWS(function() { cluster2.removeInstance(__endpoint3); });
EXPECT_NO_THROWS(function() { cluster2.dissolve({force: true}); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__endpoint3, {recoveryMethod: "clone"}); });
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint4, {recoveryMethod: "clone", replicationSources: [__endpoint3]}); });

session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("STOP replica");

EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint3); });

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__endpoint4)}, "No replication sources configured", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to rejoin Read-Replica '${__endpoint4}' to the Cluster: There are no available replication sources. Use Cluster.setInstanceOption() with the option 'replicationSources' to update the instance's sources.`);

//@<> Attempt to rejoin a read-replica with OFFLINE replicationSources
cluster.addReplicaInstance(__endpoint3, {replicationSources: [__endpoint2]})

session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP replica");
session2.runSql("STOP group_replication");
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

// Establish the global session to the read-replica and attempt to getCluster()
shell.connect(__sandbox_uri3);
EXPECT_NO_THROWS(function() { cluster = dba.getCluster(); });

WIPE_STDOUT()

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__endpoint3)}, "Source is not ONLINE", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to rejoin Read-Replica '${__endpoint3}' to the Cluster: Unable to use '${__endpoint2}' as source, instance's state is 'OFFLINE'. Use Cluster.setInstanceOption() with the option 'replicationSources' to update the instance's sources.`)

//@<> Attempt to rejoin a read-replica with unreachable replicationSources
testutil.killSandbox(__mysql_sandbox_port2);

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__endpoint3)}, "Source is not reachable", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to rejoin Read-Replica '${__endpoint3}' to the Cluster: Unable to use '${__endpoint2}' as source, instance is unreachable. Use Cluster.setInstanceOption() with the option 'replicationSources' to update the instance's sources.`)

//@<> Attempt to rejoin a read-replica with a source no longer part of the cluster
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint2, {force: true}); });

testutil.startSandbox(__mysql_sandbox_port2);

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__endpoint3)}, "Source does not belong to the Cluster", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to rejoin Read-Replica '${__endpoint3}' to the Cluster: Unable to use '${__endpoint2}' as source, instance does not belong to the Cluster. Use Cluster.setInstanceOption() with the option 'replicationSources' to update the instance's sources.`)

//@<> Update the replicationSources of a Read-Replica with the current sources no longer part of the cluster (update to primary)
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint3, "replicationSources", "primary"); });

EXPECT_OUTPUT_CONTAINS(`Setting the value of 'replicationSources' to 'primary' in the instance: '${__endpoint3}' ...`)
EXPECT_OUTPUT_CONTAINS(`WARNING: To update the replication channel with the changes the Read-Replica must be reconfigured using Cluster.rejoinInstance().`)
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'replicationSources' to 'primary' in the cluster member: '${__endpoint3}'.`)

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

//@<> Update the replicationSources of a Read-Replica without any configured sources (update to secondary)

// Add a secondary
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", "secondary"); });

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri4); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary", __endpoint2);

//@<> Update the replicationSources of a Read-Replica from "primary" to a list of sources (single source)
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint3, "replicationSources", [__endpoint1]); });

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

CHECK_READ_REPLICA(__sandbox_uri3, cluster, [__endpoint1], __endpoint1);

//@<> Update the replicationSources of a Read-Replica from "primary" to a list of sources (multiple sources)
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", [__endpoint2, __endpoint1]); });

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri4); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint2, __endpoint1], __endpoint2);

//@<> Update the replicationSources of a Read-Replica to invert the list of sources
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", [__endpoint1, __endpoint2]); });

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri4); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint1, __endpoint2], __endpoint1);

//@<> Update the replicationSources of a Read-Replica to add a source
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri3); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3); });

EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", [__endpoint1, __endpoint2, __endpoint3]); });

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri4); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint1, __endpoint2, __endpoint3], __endpoint1);

//@<> Update the replicationSources of a Read-Replica to invert some sources and keep others
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", [__endpoint3, __endpoint2, __endpoint1]); });

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri4); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint3, __endpoint2, __endpoint1], __endpoint3);

// Establish the global session to a read-replica and attempt to getCluster()
shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function() { cluster = dba.getCluster(); });

//@<> Rejoining a read-replica configured with replicationSources: "secondary" when there is only 1 Cluster member should succeed
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint3); });
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", "secondary"); });
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint4); });
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary", __endpoint2);

session4.runSql("stop replica");

var status = cluster.status();
print(status);

EXPECT_EQ("WARNING: Read Replica's replication channel is stopped. Use Cluster.rejoinInstance() to restore it.", status["defaultReplicaSet"]["topology"][__endpoint2]["readReplicas"][__endpoint4]["instanceErrors"][0]);
EXPECT_EQ("OFFLINE", status["defaultReplicaSet"]["topology"][__endpoint2]["readReplicas"][__endpoint4]["status"]);

EXPECT_EQ(status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"], {});

EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint2); });

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint4); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary", __endpoint1);

// cloneDonor tests

// Rejoin a read-replica configured with replication-sources "primary" but using a secondary as cloneDonor
EXPECT_NO_THROWS(function() { cluster.addInstance(__endpoint2); });
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", "primary"); });
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint4); });
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

session4.runSql("STOP replica");

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint4, {recoveryMethod: "clone", cloneDonor: __endpoint2}); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

// Rejoin a read-replica configured with replication-sources set to a specific member but using another as cloneDonor
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", [__endpoint1]); });
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint4); });

session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("STOP replica");

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint4, {recoveryMethod: "clone", cloneDonor: __endpoint2}); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint1], __endpoint1);

// --- timeout tests

// FR10.1.5: `timeout`: maximum number of seconds to wait for the instance to sync up with the PRIMARY after it's provisioned and the replication channel is established. If reached, the operation is rolled-back. Default is unlimited (zero).

var session1 = mysql.getSession(__sandbox_uri1);
var session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("STOP replica");
session1.runSql("create schema if not exists testing");
session1.runSql("create table if not exists testing.data (k int primary key auto_increment, v longtext)");
session1.runSql("insert into testing.data values (default, repeat('#', 1000))");

//@<> Attempt to rejoin a read-replica but with limited timeout {!__dbug_off}
testutil.dbugSet("+d,dba_sync_transactions_timeout");

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri4, {timeout: 1, recoveryMethod: "incremental"}); }, `Timeout reached waiting for all received transactions to be applied on instance '${hostname}:${__mysql_sandbox_port4}' (debug)`, "MYSQLSH");

EXPECT_OUTPUT_CONTAINS("WARNING: The Read-Replica failed to synchronize its transaction set with the Cluster. You may increase or disable the transaction sync timeout with the option 'timeout'");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");

testutil.dbugSet("");

// Confirm the revert was done
CHECK_READ_REPLICA(__sandbox_uri4, cluster, [], undefined, true, true);

//@<> Attempt to rejoin a read-replica but with unlimited timeout {!__dbug_off}
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri4, {timeout: 0, recoveryMethod: "incremental"}); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint1], __endpoint1);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
