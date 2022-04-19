//@{!__dbug_off && VER(>=8.0.27)}

function retry_with_success() {
  // Retry the operation, should succeed now
  EXPECT_NO_THROWS(function() { replicacluster = cs.createReplicaCluster(__sandbox_uri2, "replica", {recoveryMethod: "clone"}); });

  // Verify it's OK
  CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replicacluster);

  // Clean-up for the next test
  wipeout_cluster(session, replicacluster);
}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup + Create primary cluster
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
var session2 = mysql.getSession(__sandbox_uri2);

//@<> createClusterSet()
var cs = cluster.createClusterSet("myClusterSet");

// Get the CS replication user created
var __cs_replication_user = session.runSql("select user from mysql.user where user like 'mysql_innodb_cs_%'").fetchOne()[0];

//@<> createReplicaCluster() - failure pre Creating InnoDB Cluster
testutil.dbugSet("+d,dba_create_replica_cluster_fail_pre_create_cluster");

EXPECT_THROWS_TYPE(function(){ cs.createReplicaCluster(__sandbox_uri2, "replica"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");
EXPECT_SHELL_LOG_CONTAINS("Revert: Dropping replication account");

// Confirm that the ClusterSet replication user was dropped
var cs_users = session.runSql("select user from mysql.user where user like 'mysql_innodb_cs_%'").fetchAll();
EXPECT_EQ([[__cs_replication_user]], cs_users, "unexpected_cs_users");

// Unset the debug trap
testutil.dbugSet("");

retry_with_success();

//@<> createReplicaCluster() - failure post Creating InnoDB Cluster
testutil.dbugSet("+d,dba_create_replica_cluster_fail_post_create_cluster");
EXPECT_THROWS_TYPE(function(){ cs.createReplicaCluster(__sandbox_uri2, "replica"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");
EXPECT_SHELL_LOG_CONTAINS("Revert: Dropping Cluster 'replica' from Metadata");
EXPECT_SHELL_LOG_CONTAINS("Revert: Dropping replication account");
EXPECT_SHELL_LOG_CONTAINS(`Revert: Stopping Group Replication on '${hostname}:${__mysql_sandbox_port2}'`);

// Confirm the Cluster was dropped from the Metadata
var row = session.runSql("select * from mysql_innodb_cluster_metadata.clusters where cluster_name='replica'").fetchOne();
EXPECT_EQ(null, row, "cluster md check for fail_post_create_cluster");

// Unset the debug trap
testutil.dbugSet("");

retry_with_success();

//@<> createReplicaCluster() - failure post setting up ClusterSet settings
testutil.dbugSet("+d,dba_create_replica_cluster_fail_setup_cs_settings");
EXPECT_THROWS_TYPE(function(){ cs.createReplicaCluster(__sandbox_uri2, "replica"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");

EXPECT_SHELL_LOG_CONTAINS("revert: Clearing skip_replica_start");
EXPECT_SHELL_LOG_CONTAINS("Revert: Dropping Cluster 'replica' from Metadata");
EXPECT_SHELL_LOG_CONTAINS("Revert: Dropping replication account");
EXPECT_SHELL_LOG_CONTAINS(`Revert: Stopping Group Replication on '${hostname}:${__mysql_sandbox_port2}'`);
EXPECT_SHELL_LOG_CONTAINS("Revert: Removing and resetting ClusterSet settings");

CHECK_REMOVED_CLUSTER([__sandbox_uri2], cluster, "replica");

// Unset the debug trap
testutil.dbugSet("");

retry_with_success();

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);
