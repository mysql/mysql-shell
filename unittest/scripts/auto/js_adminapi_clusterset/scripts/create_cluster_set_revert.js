//@{!__dbug_off && VER(>=8.0.27)}

function retry_with_success() {
  // Retry the operation, should succeed now
  EXPECT_NO_THROWS(function() { cs = cluster.createClusterSet("myClusterSet"); });

  // Verify it's OK
  CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], cluster);
  CHECK_CLUSTER_SET(session);

  // Clean-up for the next test
  session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
  session.runSql("STOP group_replication");
  session2.runSql("STOP group_replication");

  cluster = dba.createCluster("cluster");
  cluster.addInstance(__sandbox_uri2);
}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup + Create primary cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
var session = scene.session
var cluster = scene.cluster
var session2 = mysql.getSession(__sandbox_uri2);

//@<> createClusterSet() - failure post primary cluster promotion
testutil.dbugSet("+d,dba_create_cluster_set_fail_post_primary_promotion");

EXPECT_THROWS_TYPE(function(){ cluster.createClusterSet("myClusterSet"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");
EXPECT_SHELL_LOG_CONTAINS("Revert: Clearing skip_replica_start");

// Confirm that skip_replica_start was cleared up on all cluster members
var skip_replica_start1 = session.runSql("SELECT @@skip_replica_start").fetchOne()[0];
var skip_replica_start2 = session.runSql("SELECT @@skip_replica_start").fetchOne()[0];

EXPECT_EQ(skip_replica_start1, 0, "unexpected_skip_replica_start_at_sb1");
EXPECT_EQ(skip_replica_start2, 0, "unexpected_skip_replica_start_at_sb2");

// Unset the debug trap
testutil.dbugSet("");

retry_with_success();

//@<> createClusterSet() - failure post creation of replication user
testutil.dbugSet("+d,dba_create_cluster_set_fail_post_create_replication_user");

EXPECT_THROWS_TYPE(function(){ cluster.createClusterSet("myClusterSet"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS("NOTE: Reverting changes...");
EXPECT_OUTPUT_CONTAINS("Changes successfully reverted.");
EXPECT_SHELL_LOG_CONTAINS("Revert: Clearing skip_replica_start");

// Confirm that the ClusterSet replication user was dropped
var cs_users = session.runSql("select user from mysql.user where user like 'mysql_innodb_cs_%'").fetchAll();
EXPECT_EQ([], cs_users, "unexpected_cs_users");

// Unset the debug trap
testutil.dbugSet("");

retry_with_success();

//@<> Cleanup
scene.destroy();
