//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup + Create primary cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
var session3 = mysql.getSession(__sandbox_uri3);
var session4 = mysql.getSession(__sandbox_uri4);

// decrease global gtid sync timeout
shell.options["dba.gtidWaitTimeout"] = 10;

//@<> createClusterSet()
var cluster_set = cluster.createClusterSet("myClusterSet");

//@<> createReplicaCluster - Bad options (should fail)
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster()}, "Invalid number of arguments, expected 2 to 3 but got 0", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster("")}, "Invalid number of arguments, expected 2 to 3 but got 1", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(123, "")}, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster("localhost:70000", "")}, "Invalid URI: Port is out of the valid range: 0 - 65535", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(null, "")}, "Argument #1 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "")}, "The Cluster name cannot be empty.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, 123)}, "Argument #2 is expected to be a string", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "my::replica")}, "Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (my::replica)", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "")}, "Cluster name cannot be empty", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "1234567890123456789012345678901234567890123456789012345678901234")}, "The Cluster name can not be greater than 63 characters", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "testReplicaCluster", {invalidOption: true})}, "Argument #3: Invalid options: invalidOption", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "testReplicaCluster", {timeout: -1})}, "Argument #3 timeout option must be >= 0", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri6, "myreplica")}, "Could not open connection to 'localhost:"+__mysql_sandbox_port6+"': Can't connect to MySQL server on 'localhost:"+__mysql_sandbox_port6+"'", "MySQL Error");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri1, "myreplica")}, "Target instance already part of an InnoDB Cluster", "MYSQLSH");

//@<> Root password doesn't match (should fail)
run_nolog(session3, "set password='bla'");

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "myreplica")}, "Could not open connection to 'localhost:"+__mysql_sandbox_port3+"': Access denied for user 'root'@'localhost'", "MySQL Error");

run_nolog(session3, "set password='root'");

//FR4.2: The clusterName must be the name of the REPLICA cluster. The name must follow the InnoDB Cluster naming requirements and be unique within the ClusterSet.
//@<> createReplicaCluster - Cluster name already in use not allowed
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "cluster")}, "Cluster name already in use.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: A Cluster named 'cluster' already exists in the ClusterSet. Please use a different name.");

// BUG#33298735 clusterset: inconsistent case in/sensitivity in clustername
// Verify combinations of the same name using lower and uppercase to test that the check is case insensitive
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "Cluster")}, "Cluster name already in use.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: A Cluster named 'cluster' already exists in the ClusterSet. Please use a different name.");

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "clusteR")}, "Cluster name already in use.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: A Cluster named 'cluster' already exists in the ClusterSet. Please use a different name.");

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "CLUSTER")}, "Cluster name already in use.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS("ERROR: A Cluster named 'cluster' already exists in the ClusterSet. Please use a different name.");

// FR5.3: The target instance must not have any pre-existing replication channels configured.
setup_replica(session3, __mysql_sandbox_port4);

//@<> Pre-existing replication channels are not supported
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "myReplicaCluster")}, "Unsupported active replication channel.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The target instance '${hostname}:${__mysql_sandbox_port3}' has asynchronous (source-replica) replication channel(s) configured which is not supported in InnoDB ClusterSet.`);

//FR5.4: The target instance must not belong to an InnoDB Cluster or ReplicaSet, i.e. must be a standalone instance.

//@<> Target instance must be standalone
stop_replica(session3);
shell.connect(__sandbox_uri3);
// Cleanup any bits of metadata schema hanging since this instance was a replica of a cluster member before
reset_instance(session3);

var cluster = dba.createCluster("myCluster");

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "myReplicaCluster")}, `Target instance already part of an InnoDB Cluster`, "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' is already part of an InnoDB Cluster. A new Replica Cluster must be created on a standalone instance.`);

cluster.createClusterSet("cs");

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "myReplicaCluster")}, `Target instance already part of an InnoDB Cluster`, "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' is already part of an InnoDB Cluster. A new Replica Cluster must be created on a standalone instance.`);

// Shutdown the Cluster
session.runSql("STOP group_replication;")
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "myReplicaCluster")}, `Target instance already part of an InnoDB Cluster`, "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' is already part of an InnoDB Cluster. A new Replica Cluster must be created on a standalone instance.`);

// GR only
shell.connect(__sandbox_uri3);
cluster = dba.rebootClusterFromCompleteOutage();
session3.runSql("drop schema mysql_innodb_cluster_metadata");
EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "myReplicaCluster")}, `Target instance already part of a Group Replication group`, "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' is already part of a Group Replication group. A new Replica Cluster must be created on a standalone instance.`);

// target has a replicaset
reset_instance(session3);
rs = dba.createReplicaSet("rs");

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "myReplicaCluster")}, `Target instance already part of an InnoDB ReplicaSet`, "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' is already part of an InnoDB ReplicaSet. A new Replica Cluster must be created on a standalone instance.`);

//@<> createReplicaCluster() - dryRun
reset_instance(session3);

EXPECT_EQ(0, session3.runSql("select @@super_read_only").fetchOne()[0]);
EXPECT_EQ("", session3.runSql("select @@gtid_executed").fetchOne()[0]);

EXPECT_NO_THROWS(function() { replica = cluster_set.createReplicaCluster(__sandbox_uri3, "myReplicaCluster2", {dryRun: 1}); });

// dryRun returns null
EXPECT_EQ(null, replica);

// regression test: ensure SRO didn't change
EXPECT_EQ(0, session3.runSql("select @@super_read_only").fetchOne()[0]);
EXPECT_EQ("", session3.runSql("select @@gtid_executed").fetchOne()[0]);

EXPECT_OUTPUT_CONTAINS(`Setting up replica 'myReplicaCluster2' of cluster 'cluster' at instance '${hostname}:${__mysql_sandbox_port3}'.`);
EXPECT_OUTPUT_CONTAINS(`A new InnoDB Cluster will be created on instance '${hostname}:${__mysql_sandbox_port3}'.`);
EXPECT_OUTPUT_CONTAINS(`* Checking transaction state of the instance...`);
EXPECT_OUTPUT_CONTAINS(`NOTE: The target instance '${hostname}:${__mysql_sandbox_port3}' has not been pre-provisioned (GTID set is empty), but the clusterset was configured to assume that replication can completely recover the state of new instances.`);
EXPECT_OUTPUT_CONTAINS(`The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '${hostname}:${__mysql_sandbox_port3}' with a physical snapshot from an existing clusterset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);
EXPECT_OUTPUT_CONTAINS(`* Configuring ClusterSet managed replication channel...`);
EXPECT_OUTPUT_CONTAINS(`** Changing replication source of ${hostname}:${__mysql_sandbox_port3} to ${hostname}:${__mysql_sandbox_port1}`);
EXPECT_OUTPUT_CONTAINS(`* Waiting for instance '${hostname}:${__mysql_sandbox_port3}' to synchronize with PRIMARY Cluster...`);
EXPECT_OUTPUT_CONTAINS(`* Updating topology`);
EXPECT_OUTPUT_CONTAINS(`Replica Cluster 'myReplicaCluster2' successfully created on ClusterSet 'myClusterSet'.`);
EXPECT_OUTPUT_CONTAINS(`dryRun finished.`);

//@<> Success
\option dba.logSql = 2
WIPE_SHELL_LOG();

EXPECT_NO_THROWS(function() { replica = cluster_set.createReplicaCluster(__sandbox_uri3, "cluster2"); });

//BUG#34465006: createReplicaCluster() must reset member actions to avoid upgrade scenario bug
EXPECT_SHELL_LOG_CONTAINS("SELECT group_replication_reset_member_actions()");
\option dba.logSql = 0

CHECK_REPLICA_CLUSTER([__sandbox_uri3], scene.cluster, replica);

//@<> Create replica on instance that's already a replica
EXPECT_THROWS(function(){cluster_set.createReplicaCluster(__sandbox_uri3, "cluster3");}, `Target instance already part of an InnoDB Cluster`, "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' is already part of an InnoDB Cluster. A new Replica Cluster must be created on a standalone instance.`);

// GR Options

//FR5.5: The target instance server_id and server_uuid must be unique in the ClusterSet, including among OFFLINE or unreachable members.

//@<> Target cluster server_id must be unique in the ClusterSet
var session3 = mysql.getSession(__sandbox_uri3);
var original_server_id = session4.runSql("SELECT @@server_id").fetchOne()[0];
var server_id = session3.runSql("SELECT @@server_id").fetchOne()[0];

session4.runSql("SET PERSIST server_id="+server_id);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster")}, "Invalid server_id.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The target instance '${hostname}:${__mysql_sandbox_port4}' has a 'server_id' already being used by a member of the ClusterSet.`);

// Restore server_id
session4.runSql("SET PERSIST server_id="+original_server_id);

//@<> Target cluster server_uuid must be unique in the ClusterSet
var original_server_uuid = session4.runSql("SELECT @@server_uuid").fetchOne()[0];
var server_uuid = session3.runSql("SELECT @@server_uuid").fetchOne()[0];
testutil.stopSandbox(__mysql_sandbox_port4);
testutil.changeSandboxConf(__mysql_sandbox_port4, "server_uuid", server_uuid);
testutil.startSandbox(__mysql_sandbox_port4);

EXPECT_THROWS_TYPE(function(){cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster")}, "Invalid server_uuid.", "MYSQLSH");
EXPECT_OUTPUT_CONTAINS(`ERROR: The target instance '${hostname}:${__mysql_sandbox_port4}' has a 'server_uuid' already being used by a member of the ClusterSet.`);

// Restore server_uuid
testutil.stopSandbox(__mysql_sandbox_port4);
testutil.changeSandboxConf(__mysql_sandbox_port4, "server_uuid", original_server_uuid);
testutil.startSandbox(__mysql_sandbox_port4);
session4 = mysql.getSession(__sandbox_uri4);

// Create another ReplicaCluster on the ClusterSet to test that there's only one PRIMARY Cluster

// add some garbage first
session4.runSql("create schema garbage");

//@<> incremental should fail
EXPECT_THROWS(function() { replica2 = cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster2", {recoveryMethod:"incremental"}); }, "Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered");

//@<> default (non-interactive) should fail
EXPECT_THROWS(function() { replica2 = cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster2"); }, "Instance provisioning required");

//@<> default (interactive) should prompt
shell.options.useWizards=1;
testutil.expectPrompt("Please select a recovery method [C]lone/[A]bort (default Abort): ", "a");
EXPECT_THROWS(function() { replica2 = cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster2"); }, "Cancelled");
shell.options.useWizards=0;

//@<> Create 2nd ReplicaCluster via clone
EXPECT_NO_THROWS(function() { replica2 = cluster_set.createReplicaCluster(__sandbox_uri4, "myReplicaCluster2", {recoveryMethod:"clone"}); });

values = { "domain": "myClusterSet" };
CHECK_REPLICA_CLUSTER([__sandbox_uri3], scene.cluster, replica, values);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], scene.cluster, replica2, values);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
