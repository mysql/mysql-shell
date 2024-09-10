//@ {VER(>=8.0.23)}

function setup_replica(session, source_port, channel_name) {
  if (channel_name == undefined)
    channel_name = "";
  session.runSql("CHANGE REPLICATION SOURCE TO source_host='localhost', source_port=/*(*/?/*)*/, source_user='root', source_password='root', source_auto_position=1 FOR CHANNEL ?", [source_port, channel_name]);
  session.runSql("START REPLICA FOR CHANNEL ?", [channel_name]);
  while (true) {
    r = session.runSql("SHOW REPLICA STATUS").fetchOne();
    if (r.Replica_IO_Running != 'Connecting')
      break;
  }
}

function stop_replica(session) {
  session.runSql("STOP REPLICA");
  session.runSql("RESET REPLICA ALL");
}

//@<> INCLUDE read_replicas_utils.inc

//@<> Setup + Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1], {"manualStartOnBoot": true});
var session = scene.session
var cluster = scene.cluster

testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);

//@<> addReplicaInstance() - Bad options (should fail)
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(123); }, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance("localhost:70000"); }, "Invalid URI: Port is out of the valid range: 0 - 65535", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri6); }, "Could not open connection to 'localhost:" + __mysql_sandbox_port6 + "': Can't connect to MySQL server on 'localhost:" + __mysql_sandbox_port6 + "'", "MySQL Error");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {invalidOption: true})}, "Argument #2: Invalid options: invalidOption", "ArgumentError");

// Options, wrong value types
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {dryRun: "yes"})}, "Argument #2: Option 'dryRun' Bool expected, but value is String", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {label: 1})}, "Argument #2: Option 'label' is expected to be of type String, but is Integer", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {replicationSources: [1]})}, "Argument #2: Invalid typecast: String expected, but value is Integer", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {recoveryMethod: 1})}, "Argument #2: Option 'recoveryMethod' is expected to be of type String, but is Integer", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {recoveryMethod: ""})}, "Argument #2: Invalid value for option recoveryMethod: ", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {recoveryMethod: "foobar"})}, "Argument #2: Invalid value for option recoveryMethod: foobar", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {recoveryProgress: "foo"})}, "Argument #2: Option 'recoveryProgress' Integer expected, but value is String", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {recoveryProgress: -1})}, "Argument #2: Invalid value '-1' for option 'recoveryProgress'. It must be an integer in the range [0, 2].", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {timeout: "foo"})}, "Argument #2: Option 'timeout' Integer expected, but value is String", "TypeError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {timeout: -1})}, "Argument #2 timeout option must be >= 0", "ArgumentError");

// Options, not valid values
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {replicationSources: [__endpoint3]})}, "Source does not belong to the Cluster", "MYSQLSH");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {replicationSources: [__endpoint3, "primary"]})}, "Invalid address format in 'primary'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {replicationSources: ["secondary", __endpoint2]})}, "Invalid address format in 'secondary'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses", "ArgumentError");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3, {replicationSources: ["secondary", "primary"]})}, "Invalid address format in 'secondary'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses", "ArgumentError");

//@<> Root password doesn't match (should fail)
run_nolog(session3, "set password='bla'");

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3)}, "Could not open connection to 'localhost:" + __mysql_sandbox_port3 + "': Access denied for user 'root'@'localhost'", "MySQL Error");

run_nolog(session3, "set password='root'");

//@<> FR2.4: The target instance must not have any pre-existing replication channels configured.
setup_replica(session3, __mysql_sandbox_port2);

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3)}, "Unsupported active replication channel.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The target instance '${hostname}:${__mysql_sandbox_port3}' has asynchronous (source-replica) replication channel(s) configured which is not supported in InnoDB Cluster Read-Replicas.`);

stop_replica(session3);

//@<> FR2.3: The target instance must not belong to an InnoDB Cluster or ReplicaSet, i.e. must be a standalone instance
shell.connect(__sandbox_uri3);
// Cleanup any bits of metadata schema hanging since this instance was a replica of a cluster member before
reset_instance(session3);

dba.createCluster("myCluster");

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3)}, "Target instance already part of an InnoDB Cluster", "MYSQLSH");

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri1)}, "Target instance already part of this InnoDB Cluster", "MYSQLSH");

reset_instance(session3);

dba.createReplicaSet("myReplicaSet");

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3)}, "Target instance already part of an InnoDB ReplicaSet", "MYSQLSH");

reset_instance(session3);

//@<> FR2.5: The target instance server_id and server_uuid must be unique in the topology, including among OFFLINE or unreachable members.
var session3 = mysql.getSession(__sandbox_uri3);
var session1 = mysql.getSession(__sandbox_uri1);
var original_server_id = session3.runSql("SELECT @@server_id").fetchOne()[0];
var server_id = session1.runSql("SELECT @@server_id").fetchOne()[0];

session3.runSql("SET PERSIST server_id=" + server_id);

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3)}, "Invalid server_id.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The target instance '${hostname}:${__mysql_sandbox_port3}' has a 'server_id' already being used by instance '${hostname}:${__mysql_sandbox_port1}'`);

// Restore server_id
session3.runSql("SET PERSIST server_id=" + original_server_id);

// Test server_uuid too
var server_uuid = session1.runSql("SELECT @@server_uuid").fetchOne()[0];
var original_server_uuid = session3.runSql("SELECT @@server_uuid").fetchOne();
original_server_uuid = original_server_uuid[0];
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "server_uuid", server_uuid);
testutil.startSandbox(__mysql_sandbox_port3);

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri3)}, "Invalid server_uuid.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The target instance '${hostname}:${__mysql_sandbox_port3}' has a 'server_uuid' already being used by instance '${hostname}:${__mysql_sandbox_port1}'`);

// Restore server_uuid
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "server_uuid", original_server_uuid);
testutil.startSandbox(__mysql_sandbox_port3);
session3 = mysql.getSession(__sandbox_uri3);

//@<> .addReadReplica() on a cluster with more than one member and not having quorum.
cluster.addInstance(__sandbox_uri2);
cluster.addInstance(__sandbox_uri3);

// Loose quorum
shell.connect(__sandbox_uri1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING),UNREACHABLE");
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING),UNREACHABLE");

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4); }, "There is no quorum to perform the operation", "MYSQLSH");

// Restore quorum
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);

//FR1.3.3: replicationSources: The list of sources for the Read Replica. All sources must be ONLINE Cluster members.

//<> .addReadReplica() with a replicationSources consisting of a Cluster member not ONLINE must fail
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint3]}); }, "Source is not ONLINE", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to use '${__endpoint3}' as a source: instance's state is 'OFFLINE'`);

//<> .addReadReplica() with a replicationSources set to 'secondary' on a Cluster with no secondaries ONLINE must fail
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: "secondary"}); }, "No ONLINE SECONDARY members available", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: Unable to set the 'replicationSources' to 'secondary': the Cluster does not have any ONLINE SECONDARY member.")

// --- Positive tests

//@<> Restore Cluster: rejoin all instances
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

// FR1.2 - The `instance` parameter must be the "host:port" of the target MySQL instance that will be added as Read Replica.

//@<> addReplicaInstance() - dryRun
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {dryRun: 1}); });

EXPECT_OUTPUT_CONTAINS(`Setting up '${__endpoint4}' as a Read Replica of Cluster 'cluster'.`);
EXPECT_OUTPUT_CONTAINS(`* Checking transaction state of the instance...`);
EXPECT_OUTPUT_CONTAINS(`NOTE: The target instance '${hostname}:${__mysql_sandbox_port4}' has not been pre-provisioned (GTID set is empty), but the cluster was configured to assume that incremental state recovery can correctly provision it in this case.`);
EXPECT_OUTPUT_CONTAINS(`The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '${hostname}:${__mysql_sandbox_port4}' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);
EXPECT_OUTPUT_CONTAINS(`Incremental state recovery was selected because it seems to be safely usable.`);
EXPECT_OUTPUT_CONTAINS(`* Configuring Read-Replica managed replication channel...`);
EXPECT_OUTPUT_CONTAINS(`** Changing replication source of ${hostname}:${__mysql_sandbox_port4} to ${hostname}:${__mysql_sandbox_port1}`);
EXPECT_OUTPUT_CONTAINS(`'${hostname}:${__mysql_sandbox_port4}' successfully added as a Read-Replica of Cluster 'cluster'.`);
EXPECT_OUTPUT_CONTAINS(`dryRun finished.`);

// Use the same check for removed read-replicas, since it checks the instance is standalone and got the replication configurations removed
CHECK_REMOVED_READ_REPLICA(__sandbox_uri4, cluster);

//@<> addReplicaInstance() - using defaults - success
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4); });

// Wait for the read-replica to fetch all data from the cluster
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

//@<> reboot fom a read-replica must switch or fail if explicitly picked

//make sure that the read-replica is cloned
cluster.removeInstance(__sandbox_uri4);

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {recoveryMethod: "clone", cloneDonor: __endpoint1}); });
EXPECT_OUTPUT_CONTAINS(`NOTE: ${hostname}:${__mysql_sandbox_port4} is being cloned from ${hostname}:${__mysql_sandbox_port1}`);

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

testutil.stopGroup([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);

shell.connect(__sandbox_uri1);

EXPECT_THROWS(function(){
    cluster = dba.rebootClusterFromCompleteOutage("cluster", {primary:`${hostname}:${__mysql_sandbox_port5}`});
}, `The requested instance '${hostname}:${__mysql_sandbox_port5}' isn't part of the members of the Cluster.`);

EXPECT_THROWS(function(){
    cluster = dba.rebootClusterFromCompleteOutage("cluster", {primary:`${hostname}:${__mysql_sandbox_port4}`});
}, "The seed instance is invalid because it's a Read-Replica instance.");
EXPECT_OUTPUT_CONTAINS(`Cluster instances: '${hostname}:${__mysql_sandbox_port1}' (OFFLINE), '${hostname}:${__mysql_sandbox_port2}' (OFFLINE), '${hostname}:${__mysql_sandbox_port3}' (OFFLINE)`);
EXPECT_OUTPUT_CONTAINS(`The requested instance '${hostname}:${__mysql_sandbox_port4}' cannot be used as the new seed because it's a Read-Replica. Select an appropriate instance with the 'primary' option, or, let the command select the most appropriate one by not using the option.`);

shell.connect(__sandbox_uri4);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage("cluster"); });
EXPECT_OUTPUT_NOT_CONTAINS(`Cluster instances: '${hostname}:${__mysql_sandbox_port1}' (OFFLINE), '${hostname}:${__mysql_sandbox_port2}' (OFFLINE), '${hostname}:${__mysql_sandbox_port3}' (OFFLINE), '${hostname}:${__mysql_sandbox_port4}' (OFFLINE)`);
EXPECT_OUTPUT_CONTAINS(`Cluster instances: '${hostname}:${__mysql_sandbox_port1}' (OFFLINE), '${hostname}:${__mysql_sandbox_port2}' (OFFLINE), '${hostname}:${__mysql_sandbox_port3}' (OFFLINE)`);
EXPECT_OUTPUT_CONTAINS(`Switching over to instance '${hostname}:${__mysql_sandbox_port1}' to be used as seed because the target instance '${hostname}:${__mysql_sandbox_port4}' is a Read-Replica.`)

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

shell.connect(__sandbox_uri1);
reset_read_replica(__sandbox_uri4, cluster);

//@<> .addReplicaInstance() with the target instance being a former InnoDB Cluster member
cluster.removeInstance(__sandbox_uri3);

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });

testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

reset_read_replica(__sandbox_uri3, cluster);

//@<> .addReplicaInstance() with the target instance being a former InnoDB ReplicaSet
shell.connect(__sandbox_uri4);
rs = dba.createReplicaSet("testRS");
rs.addInstance(__sandbox_uri3, {recoveryMethod: "clone"});
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port4);
rs.removeInstance(__sandbox_uri3);

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, {recoveryMethod: "clone"}); });

testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

CHECK_READ_REPLICA(__sandbox_uri3, cluster, "primary", __endpoint1);

var session4 = mysql.getSession(__sandbox_uri4);
reset_instance(session4);

//<> .addReadReplica() with replicationSources including a Read-Replica must fail
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint2, __endpoint3]}); }, "Invalid source type: Read-Replica", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to use '${__endpoint3}' as a source: instance is a Read-Replica of the Cluster`);

//<> .addReadReplica() with replicationSources including duplicates must fail
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint2, __endpoint2]}); }, "Duplicate entries detected in 'replicationSources'", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to use '${__endpoint3}' as a source: instance is a Read-Replica of the Cluster`);

EXPECT_OUTPUT_CONTAINS(`Found multiple entries for '${__endpoint2}': Each entry in the 'replicationSources' list must be unique. Please remove any duplicates before retrying the command.`);

// Restore Cluster
reset_read_replica(__sandbox_uri3, cluster);
cluster.addInstance(__sandbox_uri3, {recoveryMethod: "incremental"});

// FR1.3.2: `label`: an identifier for the Read Replica being added, used in the output of status() and describe().

//@<> addReplicaInstance() - using different combinations of 'label'
var label_1 = "label1";
var label_2 = "label2";

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {label: label_1}); });
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

var session4 = mysql.getSession(__sandbox_uri4);
var server_uuid = session4.runSql("select @@global.server_uuid").fetchOne()[0];
var label = session.runSql("select label from mysql_innodb_cluster_metadata.v2_instances where mysql_server_uuid='" + server_uuid + "'").fetchOne()[0];

EXPECT_EQ(label_1, label);

reset_read_replica(__sandbox_uri4, cluster);

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {label: label_2}); });
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

var session4 = mysql.getSession(__sandbox_uri4);
var server_uuid = session4.runSql("select @@global.server_uuid").fetchOne()[0];
var label = session.runSql("select label from mysql_innodb_cluster_metadata.v2_instances where mysql_server_uuid='" + server_uuid + "'").fetchOne()[0];

EXPECT_EQ(label_2, label);

reset_read_replica(__sandbox_uri4, cluster);

//@<> addReplicaInstance() - check for invalid 'label' values

EXPECT_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {label: "läbel"}); }, "The label can only contain alphanumerics or the '_', '.', '-', ':' characters.");
EXPECT_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {label: "! la?be1"}); }, "The label can only start with an alphanumeric or the '_' character.");
EXPECT_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {label: "la bel"}); }, "The label can only contain alphanumerics or the '_', '.', '-', ':' characters.");

//@<> addReplicaInstance() - check for duplicate 'label' values

EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__sandbox_uri3, "label", "foo"); });
EXPECT_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {label: "foo"}); }, `Instance '${hostname}:${__mysql_sandbox_port3}' is already using label 'foo'.`);

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {label: "bar"}); });
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

EXPECT_THROWS(function() { cluster.setInstanceOption(__sandbox_uri4, "label", "foo"); }, `Instance '${hostname}:${__mysql_sandbox_port3}' is already using label 'foo'.`);

reset_read_replica(__sandbox_uri4, cluster);

// FR1.3.3: replicationSources: The list of sources for the Read Replica. All sources must be ONLINE Cluster members.
// FR1.3.1.1: If set to "primary", the list is automatically managed by Group Replication and the primary member is used as source; This is the default behaviour.
// FR1.3.1.2: If set to "secondary", the list is automatically managed by Group Replication and a secondary member is used as source.

//@<> addReplicaInstance() - using 'replicationSources: "primary'
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: "primary"}); });
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

reset_read_replica(__sandbox_uri4, cluster);

//@<> addReplicaInstance() - using 'replicationSources: "secondary'
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: "secondary"}); });
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port2);
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary", [__endpoint2, __endpoint3]);

// FR29: It must be possible to obtain a topology handle while connected to a Read Replica, using dba.getCluster(), dba.getClusterSet(), or <Cluster>.getClusterSet().

//@<> getCluster() while connected to a read-replica

// Establish the global session to the read-replica
shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function() { cluster = dba.getCluster(); });

//@<> getCluster() while connected to a read-replica and the cluster lost quorum
shell.connect(__sandbox_uri1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function() { cluster = dba.getCluster(); });

// Restore quorum
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
EXPECT_NO_THROWS(function() { cluster.forceQuorumUsingPartitionOf(__sandbox_uri1); });

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint2); });
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint3); });

//@<> addReplicaInstance() - using 'replicationSources: "[__endpoint3, __endpoint1]]'
reset_read_replica(__sandbox_uri4, cluster);

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint3, __endpoint1]}); });
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port3);
CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint3, __endpoint1], [__endpoint3]);

// Establish the global session to the read-replica and attempt to getCluster()
shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function() { cluster = dba.getCluster(); });

reset_read_replica(__sandbox_uri4, cluster);

//<> Read-Replica with replicationSources "primary" follows the primary when failover happens

// Add a Read-Replica with replicationSources set to "primary"
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: "primary"}); });
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 100);
testutil.killSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
cluster = dba.getCluster();
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING)");
testutil.waitReadReplicaState(__mysql_sandbox_port4, "ONLINE");

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint2);

// Switch the primary manually and verify the read-replica follows it

// Re-add instance1 back to the cluster
testutil.startSandbox(__mysql_sandbox_port1);
cluster.rejoinInstance(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

EXPECT_NO_THROWS(function() { cluster.setPrimaryInstance(__endpoint3); });
testutil.waitReadReplicaState(__mysql_sandbox_port4, "ONLINE");

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint3);

reset_read_replica(__sandbox_uri4, cluster);

//<> Read-Replica with replicationSources "secondary" follows the secondaries when the current one becomes unavailable

// Instance 1 will be the secondary acting as source for the read-replica
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: "secondary"}); });
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port3);
testutil.waitReadReplicaState(__mysql_sandbox_port4, "ONLINE");
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary",  [__endpoint1]);

// Make instance 1 unavailable
testutil.killSandbox(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING)");

testutil.waitReadReplicaState(__mysql_sandbox_port4, "ONLINE");
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary", [__endpoint2]);

reset_read_replica(__sandbox_uri4, cluster);

//<> Read-Replica with replicationSources "secondary" should keep operating when the cluster switches to multi-primary

// Re-add instance1 back to the cluster
testutil.startSandbox(__mysql_sandbox_port1);
cluster.rejoinInstance(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: "secondary"}); });

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);
testutil.waitReadReplicaState(__mysql_sandbox_port4, "ONLINE");

EXPECT_NO_THROWS(function() { cluster.switchToMultiPrimaryMode(); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary", [__endpoint3, __endpoint1, __endpoint2]);

// Make instance 1 unavailable
testutil.killSandbox(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING)");

testutil.waitReadReplicaState(__mysql_sandbox_port4, "ONLINE");

cluster = dba.getCluster();
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary", [__endpoint3, __endpoint2]);

// Re-add instance1 back to the cluster
testutil.startSandbox(__mysql_sandbox_port1);
cluster.rejoinInstance(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

// Switch the cluster to single-primary mode and pick instance1 to be the primary
EXPECT_NO_THROWS(function() { cluster.switchToSinglePrimaryMode(__sandbox_uri1); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary", [__endpoint2, __endpoint3]);

reset_read_replica(__sandbox_uri4, cluster);

//<> Read-Replica with replicationSources set to a specific member should not attempt to replicate from a different one when that one becomes unreachable
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint3]}); });

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port3);
testutil.waitReadReplicaState(__mysql_sandbox_port4, "ONLINE");

CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint3], [__endpoint3]);

// Make it fail quicker on failover
var session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("stop replica for channel 'read_replica_replication'");
session4.runSql("change replication source to source_connect_retry=1, source_retry_count=1 for channel 'read_replica_replication'");
session4.runSql("start replica for channel 'read_replica_replication'");

// Make instance 3 unavailable
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

testutil.waitReadReplicaState(__mysql_sandbox_port4, "CONNECTING");
CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint3], [__endpoint3], true);

reset_read_replica(__sandbox_uri4, cluster);

//<> Read-Replica with replicationSources set to a specific list of members should not attempt to replicate from a different one when those become unreachable

// Re-add instance3 back to the cluster
testutil.startSandbox(__mysql_sandbox_port3);
cluster.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint2, __endpoint3]}); });

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port2);
testutil.waitReadReplicaState(__mysql_sandbox_port4, "ONLINE");

CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint2, __endpoint3], [__endpoint2]);

// Make instance 3 and 2 unavailable
shell.connect(__sandbox_uri1);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE,(MISSING)");

testutil.waitReadReplicaState(__mysql_sandbox_port4, "CONNECTING");
CHECK_READ_REPLICA(__sandbox_uri4, cluster, [__endpoint2, __endpoint3], [__endpoint2, __endpoint3], true);

// Clean-up test
cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);
testutil.startSandbox(__mysql_sandbox_port2);
cluster.rejoinInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.startSandbox(__mysql_sandbox_port3);
cluster.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

reset_read_replica(__sandbox_uri4, cluster);

// --- recoveryMethod tests

// FR1.3.4: `recoveryMethod`: to select the provisioning method used: incremental, clone or auto. The option behavior is equal to Cluster's or ReplicaSet's behavior and the default value is auto.

//@<> Add a read replica to an InnoDB Cluster where no binary logs have been purged using recovery method set to incremental - should succeed
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {recoveryMethod: "incremental"}); });

EXPECT_OUTPUT_CONTAINS("Incremental state recovery selected through the recoveryMethod option");
EXPECT_OUTPUT_CONTAINS(`'${hostname}:${__mysql_sandbox_port4}' successfully added as a Read-Replica of Cluster 'cluster'.`);

testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

reset_read_replica(__sandbox_uri4, cluster);

//@<> Attempt to add an instance as read-replica that has transactions not present in the cluster - no-interactive
var session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("set global super_read_only=0");
session4.runSql("create schema errant");

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4); }, "Instance provisioning required", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`* Checking transaction state of the instance...`);
EXPECT_OUTPUT_CONTAINS(`WARNING: A GTID set check of the MySQL instance at '${hostname}:${__mysql_sandbox_port4}' determined that it contains transactions that do not originate from the cluster, which must be discarded before it can join the cluster.`);
EXPECT_OUTPUT_CONTAINS(`WARNING: Discarding these extra GTID events can either be done manually or by completely overwriting the state of '${hostname}:${__mysql_sandbox_port4}' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);
EXPECT_OUTPUT_CONTAINS(`Having extra GTID events is not expected, and it is recommended to investigate this further and ensure that the data can be removed prior to choosing the clone recovery method.`);
EXPECT_OUTPUT_CONTAINS(`ERROR: The target instance must be either cloned or fully provisioned before it can be added to the target cluster.`);

//@<> Attempt to add an instance as read-replica that has transactions not present in the cluster - interactive
shell.options.useWizards=1;
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
EXPECT_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4); }, "Cancelled");
shell.options.useWizards=0;

//@<> Attempt to add an instance as read-replica when binlogs were purged in the cluster and recoveryMethod: auto - non-interactive

// clean the errant trxs
session4.runSql("drop schema errant");
session4.runSql("RESET " + get_reset_binary_logs_keyword());

// purge binlogs
var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);
session1.runSql("FLUSH BINARY LOGS");
session1.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");
session2.runSql("FLUSH BINARY LOGS");
session2.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");
session3.runSql("FLUSH BINARY LOGS");
session3.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4, {recoveryMethod: "auto"}); }, "Instance provisioning required", "MYSQLSH");

//@<> Attempt to add an instance as read-replica when binlogs were purged in the cluster and recoveryMethod: auto - interactive
shell.options.useWizards=1;
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Clone): ", "a");
EXPECT_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4); }, "Cancelled");

// --- cloneDonor tests

//@<> addReplicaInstance: recoveryMethod: clone, automatic donor not valid {!__dbug_off}
testutil.dbugSet("+d,dba_clone_version_check_fail");

EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4, {recoveryMethod: "clone"}); }, "Instance '" + __endpoint1 + "' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (" + __version + ").", "MYSQLSH");

//@<> addReplicaInstance: recoveryMethod: clone, cloneDonor not valid {!__dbug_off}
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4, {recoveryMethod: "clone", cloneDonor: __endpoint2}); }, "Instance '" + __endpoint2 + "' cannot be a donor because its version (8.0.17) isn't compatible with the recipient's (" + __version + ").", "MYSQLSH");

testutil.dbugSet("");

//@<> addReplicaInstance: recoveryMethod: clone, cloneDonor valid {!__dbug_off}
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {recoveryMethod: "clone", cloneDonor: __endpoint3}); });

EXPECT_OUTPUT_CONTAINS(`NOTE: ${hostname}:${__mysql_sandbox_port4} is being cloned from ${hostname}:${__mysql_sandbox_port3}`)

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri4); });

session4 = mysql.getSession(__sandbox_uri4);
reset_instance(session4);

// --- timeout tests

// FR1.3.6: `timeout`: maximum number of seconds to wait for the instance to sync up with the PRIMARY after it's provisioned and the replication channel is established. If reached, the operation is rolled-back. Default is unlimited (zero).

//@<> Attempt to add a read-replica but with limited timeout {!__dbug_off}

// Re-create cluster
scene.destroy();

var scene = new ClusterScenario([__mysql_sandbox_port1], {"manualStartOnBoot": true});
var session = scene.session
var cluster = scene.cluster
var session4 = mysql.getSession(__sandbox_uri4);

session.runSql("create schema if not exists testing");
session.runSql("create table if not exists testing.data (k int primary key auto_increment, v longtext)");
session.runSql("insert into testing.data values (default, repeat('#', 1000))");

testutil.dbugSet("+d,dba_sync_transactions_timeout");
EXPECT_THROWS_TYPE(function() { cluster.addReplicaInstance(__sandbox_uri4, {timeout: 1, recoveryMethod: "incremental"}); }, `Timeout reached waiting for all received transactions to be applied on instance '${hostname}:${__mysql_sandbox_port4}' (debug)`, "MYSQLSH");

EXPECT_OUTPUT_CONTAINS("WARNING: The Read-Replica failed to synchronize its transaction set with the Cluster. You may increase or disable the transaction sync timeout with the option 'timeout'");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");

testutil.dbugSet("");

//@<> Attempt to add a read-replica but with unlimited timeout {!__dbug_off}
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {timeout: 0, recoveryMethod: "incremental"}); });

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
