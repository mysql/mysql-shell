function reset_read_replica(uri, cluster) {
  var session = mysql.getSession(uri);
  session.runSql("SET GLOBAL super_read_only=0");
  session.runSql("SET GLOBAL read_only=0");

  try {
    session.runSql("STOP REPLICA FOR CHANNEL 'read_replica_replication'");
  } catch (e) {}

  var master_session = connect_cluster_primary(cluster)[0];
  var group_name = master_session.runSql("select @@group_replication_group_name").fetchOne()[0];

  try {
    session.runSql("SELECT asynchronous_connection_failover_delete_managed('read_replica_replication', ?)", [group_name]);
  } catch (e) {}

  var cluster_name = cluster.status()["clusterName"];

  var cluster_id = master_session.runSql("select cluster_id from mysql_innodb_cluster_metadata.v2_clusters where cluster_name=?", [cluster_name]).fetchOne()[0];

  var res = session.runSql("select address from mysql_innodb_cluster_metadata.v2_instances where cluster_id=?", [cluster_id]).fetchAll();

  for (var r in res) {
    var address = shell.parseUri(res[r][0]);

    try {
      var host = address["host"];
      var port = address["port"];

      session.runSql("SELECT asynchronous_connection_failover_delete_source('read_replica_replication', ?, ?, '')", [host, port]).fetchAll();
    } catch (e) {
      continue;
    }
  }

  session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
  var r = session.runSql("SHOW SCHEMAS");
  var rows = r.fetchAll();
  for (var i in rows) {
    var row = rows[i];
      if (["mysql", "performance_schema", "sys", "information_schema"].includes(row[0]))
          continue;
    session.runSql("DROP SCHEMA "+row[0]);
  }

  var r = session.runSql("SELECT user,host FROM mysql.user");
  var rows = r.fetchAll();
  for (var i in rows) {
    var row = rows[i];
      if (["mysql.sys", "mysql.session", "mysql.infoschema"].includes(row[0]))
        continue;
      if (row[0] == "root" && (row[1] == "localhost" || row[1] == "%"))
        continue;
    session.runSql("DROP USER ?@?", [row[0], row[1]]);
  }

  session.runSql("RESET " + get_reset_binary_logs_keyword());
  session.runSql("RESET " + get_replica_keyword() + " ALL");

  var server_uuid = session.runSql("select @@global.server_uuid").fetchOne()[0];
  master_session.runSql("delete from mysql_innodb_cluster_metadata.instances where mysql_server_uuid = ?", [server_uuid]);
}

function connect_cluster_primary(cluster) {
  var status = cluster.status();

  if (status["defaultReplicaSet"]["topologyMode"] == "Multi-Primary") {
    uri = "root:root@"+status["groupInformationSourceMember"];
  } else {
    uri = "root:root@"+status["defaultReplicaSet"]["primary"];
  }

  uri += "?connect-timeout=1"
  s = mysql.getSession(uri);
  s.runSql("SET SESSION sql_mode=''");
  return [s, uri];
}

function is_replica_stopped(session) {
  r = session.runSql("SHOW REPLICA STATUS").fetchOne();
  if (!r) return true;

  if (r.Replica_IO_Running == "No" && r.Replica_SQL_Running == "No")
    return true;

  return false;
}

function CHECK_REMOVED_READ_REPLICA(uri, cluster) {
  println("CHECK REMOVED READ-REPLICA", uri);

  var session = mysql.getSession(uri);
  var master_session = connect_cluster_primary(cluster)[0];

  // Check replication is stopped
  EXPECT_TRUE(is_replica_stopped(session), uri+".is_replica_stopped");

  // Check replication configs are unset
  var res = session.runSql("select * from performance_schema.replication_asynchronous_connection_failover_managed where channel_name='read_replica_replication'");
    var r = res.fetchOne();
    EXPECT_EQ(null, r, uri+".failover channel")

  // Check replication channel is reset
  var res = session.runSql("select c.host, c.port, c.user, c.source_connection_auto_failover, s.service_state, s.source_uuid from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='read_replica_replication'");
  var r = res.fetchOne();
  EXPECT_EQ(null, r, uri+".async repl")

  // Check cluster replication user was removed
  var unexpected_users = master_session.runSql("select user from mysql.user where user like 'mysql_innodb_replica_%' and user not in (select attributes->>'$.readReplicaReplicationAccountUser' from mysql_innodb_cluster_metadata.instances)").fetchAll();
  EXPECT_EQ([], unexpected_users, "unexpected_rr_users");

  // Check instance does not belong to the Cluster's metadata
  var server_uuid = session.runSql("select @@global.server_uuid").fetchOne()[0];
  var r = master_session.runSql("select mysql_server_uuid FROM mysql_innodb_cluster_metadata.instances WHERE mysql_server_uuid = '" + server_uuid + "'").fetchOne();
  EXPECT_EQ(null, r, uri+".metadata consistency")
}

function are_arrays_equal(a, b) {
  a = Array.isArray(a) ? a : [];
  b = Array.isArray(b) ? b : [];
  return a.length === b.length && a.every(el => b.includes(el));
}

function check_gtid_consistent(source, replica) {
  var source_gtid = source.runSql("select @@gtid_executed").fetchOne()[0];
  var replica_gtid = replica.runSql("select @@gtid_executed").fetchOne()[0];

  var diff = source.runSql("select gtid_subtract(?, ?)", [replica_gtid, source_gtid]).fetchOne()[0];

  EXPECT_EQ("", diff, `gtid_subtract(replica, master); replica=${replica_gtid}; master=${source_gtid}`);
}

function CHECK_READ_REPLICA(uri, cluster, configured_source_list, current_sources, ignore_channel_status = false, removed_channel = false) {
  println("CHECK READ-REPLICA", uri);

  var session = mysql.getSession(uri);

  var primary = connect_cluster_primary(cluster);
  var primary_session = primary[0];
  var primary_uri = primary[1];

  // It takes ~5 seconds for the group membership to be fetched by the managed replication channel
  os.sleep(5);

  // Check replication channel
  var res = session.runSql("select c.host, c.port, c.user, c.source_connection_auto_failover, s.service_state, s.source_uuid from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='read_replica_replication'");
  var r = res.fetchOne();

  primary_parsed = shell.parseUri(primary_uri);
  var primary_uri = primary_parsed["host"] + ":" + primary_parsed["port"];

  if (current_sources !== undefined) {
    if (type(current_sources) == "Array") {
      var hosts = [];
      var ports = [];

      for (source of current_sources) {
        var source_parsed = shell.parseUri(source);
        hosts.push(source_parsed["host"]);
        ports.push(source_parsed["port"]);
      }

      EXPECT_TRUE(hosts.includes(r["host"]), uri+".rr.host");
      EXPECT_TRUE(ports.includes(r["port"]), uri+".rr.port");
    } else {
      var current_source_session = mysql.getSession("mysql://root:root@"+current_sources);
      var current_source_uuid = current_source_session.runSql("select @@global.server_uuid").fetchOne()[0];
      EXPECT_EQ(current_source_uuid, r["source_uuid"], uri+".source_uuid");
    }
  } else {
    if (configured_source_list == "primary") {
      EXPECT_EQ(primary_parsed["host"], r["host"], uri+".rr.host");
      EXPECT_EQ(primary_parsed["port"], r["port"], uri+".rr.port");
    }
  }

  if (!removed_channel) {
    EXPECT_EQ("1", r["source_connection_auto_failover"], uri+".rr.source_connection_auto_failover");
  } else {
    EXPECT_EQ(null, r);
  }

  if (!ignore_channel_status && !removed_channel) {
    EXPECT_EQ("ON", r["service_state"], uri+".rr.service_state");
  }

  // Get the source list
  var configured_sources = session.runSql("select host, port from performance_schema.replication_asynchronous_connection_failover").fetchAll();

  var configured_sources_array = [];

  for (i = 0; i < configured_sources.length; i++) {
    var source = configured_sources[i][0] + ":" + configured_sources[i][1];
    configured_sources_array.push(source);
  }

  if (!removed_channel) {
    var cluster_members = primary_session.runSql("SELECT member_host, member_port FROM performance_schema.replication_group_members").fetchAll();

    var cluster_members_array = [];

    for (i = 0; i < cluster_members.length; i++) {
      var host = cluster_members[i][0];
      var port = cluster_members[i][1];
      var address = host + ":" + port;
      cluster_members_array.push(address);
    }

    // Remove read-replica from the list
    var rr_parsed = shell.parseUri(uri);
    var rr = hostname + ":" + rr_parsed["port"];

    var i = cluster_members_array.indexOf(rr);
    if (i !== -1) cluster_members_array.splice(i, 1);

    var source = r["host"] + ":" + r["port"];

    if (!ignore_channel_status && !removed_channel) {
      EXPECT_TRUE(cluster_members_array.includes(source), uri+".source_belongs_to_cluster")
    }

    if (!removed_channel) {
      if (type(configured_source_list) == "Array") {
        EXPECT_TRUE(are_arrays_equal(configured_source_list, configured_sources_array), uri+".sources_are_the_configured_ones")
      } else {
        EXPECT_TRUE(are_arrays_equal(cluster_members_array, configured_sources_array), uri+".sources_are_all_secondary")
      }
    }
}

  // Check instance belongs to the metadata
  var server_uuid = session.runSql("select @@global.server_uuid").fetchOne()[0];
  var res = session.runSql("select cluster_id, instance_type from mysql_innodb_cluster_metadata.v2_instances where mysql_server_uuid='" + server_uuid + "'");

  var cluster_name = cluster.status()["clusterName"];
  var cluster_id = primary_session.runSql("select cluster_id from mysql_innodb_cluster_metadata.v2_clusters where cluster_name='" + cluster_name + "'").fetchOne()[0];

  while (r = res.fetchOne()) {
    EXPECT_EQ(cluster_id, r["cluster_id"], uri+".v2_instances.cluster_id");
    EXPECT_EQ("read-replica", r["instance_type"], uri+".v2_instances.instance_type");
  }

  // check RR replication account exists and is in MD
  if (!removed_channel) {
    var rr_replication_account = session.runSql("select user from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='read_replica_replication'").fetchOne()[0];
    EXPECT_NE(rr_replication_account, null, uri+".repl_account-pfs");
  }

  var md_rr_replication_account = session.runSql("select (attributes->>'$.readReplicaReplicationAccountUser') from mysql_innodb_cluster_metadata.v2_instances where mysql_server_uuid = '" + server_uuid + "'").fetchOne()[0];
  EXPECT_NE(md_rr_replication_account, null, uri+".repl_account-md");

  if (!removed_channel) {
    EXPECT_EQ(rr_replication_account, md_rr_replication_account, uri+".repl_account");
  }

  // check RR replication account username has the right prefix 'mysql_innodb_replica_'
  var server_id = session.runSql("select @@global.server_id").fetchOne()[0];
  var expected_account = "mysql_innodb_replica_" + server_id;

  EXPECT_EQ(expected_account, md_rr_replication_account, uri+".repl_account_format");

  // check RR replication account password is using caching_sha2_password
  var pwd_plugin = session.runSql("select plugin from mysql.user where User=?", [md_rr_replication_account]).fetchOne()[0];

  EXPECT_EQ("caching_sha2_password", pwd_plugin, uri+".repl_account_pwd_plugin");

  // check if SRO is enabled
  var sro = session.runSql("select @@global.super_read_only").fetchOne()[0];
  EXPECT_TRUE(sro, uri+".super_read_only");

  // Check GTID consistency
  check_gtid_consistent(primary_session, session);
}

function inject_errant_gtid(slave) {
    // Force the slave to have a transaction that doesn't exist in the master
    slave.runSql("SET GLOBAL super_read_only=0");
    var before = slave.runSql("select @@gtid_executed").fetchOne()[0];
    // Dropping a DB that does not exists is enough to create a new trx.
    slave.runSql("DROP SCHEMA IF EXISTS errant_trx_db");
    var gtid = slave.runSql("select gtid_subtract(@@gtid_executed, ?)", [before]).fetchOne()[0];
    slave.runSql("SET GLOBAL super_read_only=1");
    return gtid;
}

function inject_empty_trx(session, trx_gtid) {
    session.runSql("SET GTID_NEXT='" + trx_gtid + "'");
    session.runSql("BEGIN");
    session.runSql("COMMIT");
    session.runSql("SET GTID_NEXT='AUTOMATIC'");
}
