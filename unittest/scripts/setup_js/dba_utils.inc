function is_replica_stopped(session) {
  r = session.runSql("SHOW REPLICA STATUS").fetchOne();
  if (!r) return true;

  if (r.Replica_IO_Running == "No" && r.Replica_SQL_Running == "No")
    return true;

  return false;
}

function group_replication_installed(session) {
    var row = session.runSql("select plugin_status from information_schema.plugins where plugin_name='group_replication'").fetchOne();
    if (row)
        return row[0] == "ACTIVE";
    return false;
}

function CHECK_DISSOLVED_CLUSTER(primary, target_session) {
  // GR must be stopped
  EXPECT_EQ("OFFLINE", primary.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);

  // MD schema must not exist
  var row = primary.runSql("show schemas like 'mysql_innodb_cluster_metadata'").fetchOne();
  EXPECT_EQ(null, row, "metadata_schema_found");

  // No recovery accounts must exist: mysql_innodb_cluster_%
  var recovery_accounts = target_session.runSql("select user from mysql.user where user like 'mysql_innodb_cluster_%'").fetchAll();
  EXPECT_EQ([], recovery_accounts, "unexpected_recovery_accounts");

  // No ClusterSet replication accounts must exist: mysql_innodb_cs_%
  var cs_accounts = target_session.runSql("select user from mysql.user where user like 'mysql_innodb_cs_%'").fetchAll();
  EXPECT_EQ([], cs_accounts, "unexpected_clusterset_accounts");

  // No Read-Replica replication accounts must exist: mysql_innodb_cs_%
  var cs_accounts = target_session.runSql("select user from mysql.user where user like 'mysql_innodb_replica_%'").fetchAll();
  EXPECT_EQ([], cs_accounts, "unexpected_read_replica_accounts");

  // Verify if member actions were reset to the defaults
  if (__version_num >= 80027 && group_replication_installed(target_session)) {
    var res = target_session.runSql("select name, event, enabled, type, priority, error_handling from performance_schema.replication_group_member_actions");
    while (r = res.fetchOne()) {
      if ("mysql_disable_super_read_only_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], target_session.uri+".member_action.sro.event");
        EXPECT_EQ(1, r["enabled"], target_session.uri+".member_action.sro.enabled");
        EXPECT_EQ("INTERNAL", r["type"], target_session.uri+".member_action.sro.type");
        EXPECT_EQ(1, r["priority"], target_session.uri+".member_action.sro.priority");
        EXPECT_EQ("IGNORE", r["error_handling"], target_session.uri+".member_action.sro.error_handling");
      } else if ("mysql_start_failover_channels_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], target_session.uri+".member_action.failover.event");
        EXPECT_EQ(1, r["enabled"], target_session.uri+".member_action.failover.enabled");
        EXPECT_EQ("INTERNAL", r["type"], target_session.uri+".member_action.failover.type");
        EXPECT_EQ(10, r["priority"], target_session.uri+".member_action.failover.priority");
        EXPECT_EQ("CRITICAL", r["error_handling"], target_session.uri+".member_action.failover.error_handling");
      } else {
        println("UNEXPECTED member action: ", r["name"], "at", target_session.uri)
      }
    }
  }

  // Verify that there are not GR member actions
  var count_async_conn_failover = session.runSql("SELECT count(*) FROM performance_schema.replication_asynchronous_connection_failover").fetchOne()[0];
  EXPECT_EQ(0, count_async_conn_failover, "unexpected_async_conn_failover");

  var count_async_conn_failover_managed = session.runSql("SELECT count(*) FROM performance_schema.replication_asynchronous_connection_failover_managed").fetchOne()[0];
  EXPECT_EQ(0, count_async_conn_failover_managed, "unexpected_async_conn_failover_managed");

  // Verify that skip_replica_start / skip_start_start is disabled
  var count_skip_replica_start = session.runSql("SELECT count(*) FROM performance_schema.persisted_variables WHERE ((variable_name = 'skip_replica_start') OR (variable_name = 'skip_start_start')) AND (variable_value <> 'OFF')").fetchOne()[0];
  EXPECT_EQ(0, count_skip_replica_start, "unexpected_skip_replica_start");

  if (__version_num >= 80023) {
    // Verify if there are no read-replica channels configured

    // Check replication is stopped
    EXPECT_TRUE(is_replica_stopped(target_session), ".is_replica_stopped");

    // Check replication configs are unset
    var res = target_session.runSql("select * from performance_schema.replication_asynchronous_connection_failover_managed where channel_name='read_replica_replication'");
      var r = res.fetchOne();
      EXPECT_EQ(null, r, ".failover channel")

    // Check replication channel is reset
    var res = target_session.runSql("select c.host, c.port, c.user, c.source_connection_auto_failover, s.service_state, s.source_uuid from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='read_replica_replication'");
    var r = res.fetchOne();
    EXPECT_EQ(null, r, ".async repl")
  }
}
