//@{!__dbug_off && VER(>=8.0.27)}

function retry_with_success() {
  // Retry the operation, should succeed now
  EXPECT_NO_THROWS(function() { clusterset.removeCluster("replicacluster"); });

  // Verify it's OK
  CHECK_REMOVED_CLUSTER([__sandbox_uri2], cluster, "replicacluster");

  // Clean-up for the next test
  //wipeout_cluster(session, replicacluster);

  // Re-create the Replica Cluster
  replicacluster = clusterset.createReplicaCluster(__sandbox_uri2, "replicacluster", {recoveryMethod: "clone"});
  CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replicacluster);
}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup + Create primary cluster + add Replica Cluster
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
var session2 = mysql.getSession(__sandbox_uri2);

var clusterset = cluster.createClusterSet("myClusterSet");
var replicacluster = clusterset.createReplicaCluster(__sandbox_uri2, "replicacluster", {recoveryMethod: "incremental"});
CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replicacluster);

//@<> removeCluster() - failure post disabling skip_replica_start
testutil.dbugSet("+d,dba_remove_cluster_fail_disable_skip_replica_start");

EXPECT_THROWS_TYPE(function(){ clusterset.removeCluster("replicacluster"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");
EXPECT_SHELL_LOG_CONTAINS("Revert: Enabling skip_replica_start");

CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replicacluster);

// Unset the debug trap
testutil.dbugSet("");

retry_with_success();

//@<> removeCluster() - failure post removal of clusterset member
testutil.dbugSet("+d,dba_remove_cluster_fail_post_cs_member_removed");

EXPECT_THROWS_TYPE(function(){ clusterset.removeCluster("replicacluster"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");
EXPECT_SHELL_LOG_CONTAINS("Revert: Recording back ClusterSet member removed");

CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replicacluster);

// Unset the debug trap
testutil.dbugSet("");

retry_with_success();

//@<> removeCluster() - failure post removal of replication user
testutil.dbugSet("+d,dba_remove_cluster_fail_post_replication_user_removal");

EXPECT_THROWS_TYPE(function(){ clusterset.removeCluster("replicacluster"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");
EXPECT_SHELL_LOG_CONTAINS(`Revert: Creating replication account at '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_SHELL_LOG_CONTAINS("Revert: Recording back ClusterSet member removed");

// Validate a replication account was added back
var cs_users = session.runSql("select user from mysql.user where user like 'mysql_innodb_cs_%'").fetchAll();
EXPECT_NE([], cs_users, "unexpected_cs_users");

CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replicacluster);

// Unset the debug trap
testutil.dbugSet("");

retry_with_success();

//@<> removeCluster() - failure post removal of replica cluster configurations
testutil.dbugSet("+d,dba_remove_cluster_fail_post_replica_removal");

EXPECT_THROWS_TYPE(function(){ clusterset.removeCluster("replicacluster"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");
EXPECT_SHELL_LOG_CONTAINS("Revert: Enabling skip_replica_start");
EXPECT_SHELL_LOG_CONTAINS("Revert: Re-adding Cluster as Replica");
EXPECT_SHELL_LOG_CONTAINS(`Revert: Creating replication account at '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_SHELL_LOG_CONTAINS("Revert: Recording back ClusterSet member removed");
EXPECT_SHELL_LOG_CONTAINS("Revert: Re-creating Cluster recovery accounts");
EXPECT_SHELL_LOG_CONTAINS("Revert: Re-creating Cluster's metadata");

CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replicacluster);

// Confirm that skip_replica_start was enabled back
var skip_replica_start2 = session.runSql("SELECT @@skip_replica_start").fetchOne()[0];

EXPECT_EQ(skip_replica_start2, 0, "unexpected_skip_replica_start_at_sb2");

CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replicacluster);

// Unset the debug trap
testutil.dbugSet("");

retry_with_success();

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);
