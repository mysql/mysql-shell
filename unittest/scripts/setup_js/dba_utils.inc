function CHECK_DISSOLVED_CLUSTER(primary) {
  // GR must be stopped
  EXPECT_EQ("OFFLINE", primary.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);

  // MD schema must not exist
  var row = primary.runSql("show schemas like 'mysql_innodb_cluster_metadata'").fetchOne();
  EXPECT_EQ(null, row, "metadata_schema_found");

  // No recovery accounts must exist: mysql_innodb_cluster_%
  var recovery_accounts = session.runSql("select user from mysql.user where user like 'mysql_innodb_cluster_%'").fetchAll();
  EXPECT_EQ([], recovery_accounts, "unexpected_recovery_accounts");

  // No ClusterSet replication accounts must exist: mysql_innodb_cs_%
  var cs_accounts = session.runSql("select user from mysql.user where user like 'mysql_innodb_cs_%'").fetchAll();
  EXPECT_EQ([], cs_accounts, "unexpected_clusterset_accounts");
}
