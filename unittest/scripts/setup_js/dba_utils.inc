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

  // Verify if member actions were reset to the defaults
  if (__version_num >= 80027) {
    var res = session.runSql("select name, event, enabled, type, priority, error_handling from performance_schema.replication_group_member_actions");
    while (r = res.fetchOne()) {
      if ("mysql_disable_super_read_only_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], session.uri+".member_action.sro.event");
        EXPECT_EQ(1, r["enabled"], session.uri+".member_action.sro.enabled");
        EXPECT_EQ("INTERNAL", r["type"], session.uri+".member_action.sro.type");
        EXPECT_EQ(1, r["priority"], session.uri+".member_action.sro.priority");
        EXPECT_EQ("IGNORE", r["error_handling"], session.uri+".member_action.sro.error_handling");
      } else if ("mysql_start_failover_channels_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], session.uri+".member_action.failover.event");
        EXPECT_EQ(1, r["enabled"], session.uri+".member_action.failover.enabled");
        EXPECT_EQ("INTERNAL", r["type"], session.uri+".member_action.failover.type");
        EXPECT_EQ(10, r["priority"], session.uri+".member_action.failover.priority");
        EXPECT_EQ("CRITICAL", r["error_handling"], session.uri+".member_action.failover.error_handling");
      } else {
        println("UNEXPECTED member action: ", r["name"], "at", session.uri)
      }
    }
  }
}
