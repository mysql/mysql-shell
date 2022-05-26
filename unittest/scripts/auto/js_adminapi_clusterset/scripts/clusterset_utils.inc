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

function is_replica_stopped(session) {
  r = session.runSql("SHOW REPLICA STATUS").fetchOne();
  if (r.Replica_IO_Running == "No" && r.Replica_SQL_Running == "No")
    return true;

  return false;
}

function stop_replica(session) {
  session.runSql("STOP REPLICA");
  session.runSql("RESET REPLICA ALL");
}

function reset_instance(session) {
    rows = session.runSql("SELECT managed_type, managed_name FROM performance_schema.replication_asynchronous_connection_failover_managed").fetchAll();
    for (row of rows) {
      try {
        session.runSql("select asynchronous_connection_failover_delete_managed(?,?)", [row[0], row[1]]);
      } catch (e) {}
    }
    try {
      session.runSql("STOP group_replication");
    } catch (e) {}
    session.runSql("SET GLOBAL super_read_only=0");
    try {
      session.runSql("SELECT group_replication_reset_member_actions()");
    } catch (e) {}
    session.runSql("STOP REPLICA");
    try {
      session.runSql("STOP REPLICA FOR CHANNEL 'clusterset_replication'");
    } catch (e) {}
    session.runSql("SET GLOBAL super_read_only=0");
    session.runSql("SET GLOBAL read_only=0");
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
    session.runSql("RESET MASTER");
    session.runSql("RESET SLAVE ALL");
}

function wipeout_cluster(psession, cluster_or_urls) {
  if (type(cluster_or_urls) == "Array") {
    var instances = cluster_or_urls;
    var row = psession.runSql("select c.cluster_name from mysql_innodb_cluster_metadata.clusters c join mysql_innodb_cluster_metadata.instances i on c.cluster_id = i.cluster_id where i.address = ?", [instances[0]]).fetchOne();
    if (row)
      cluster_name = row[0];
    else
      cluster_name = null;
  } else {
    var instances = [];
    for (var instance in cluster_or_urls.status()["defaultReplicaSet"]["topology"]) {
      instances.push(instance);
    }
    var cluster_name = cluster_or_urls.name;
  }

  // stop GR and delete all traces of the cluster in a clusterset
  for (var instance of instances) {
    var session = mysql.getSession("mysql://root:root@"+instance);
    reset_instance(session);
    session.close();
    if (cluster_name)
      psession.runSql("delete from mysql_innodb_cluster_metadata.instances where address = ?", [instance]);
  }

  if (cluster_name)
    psession.runSql("delete from mysql_innodb_cluster_metadata.clusters where cluster_name = ?", [cluster_name]);
}

function connect_cluster_primary(cluster) {
  return mysql.getSession("root:root@"+cluster.status()["defaultReplicaSet"]["primary"]);
}

function inject_applier_error(master, slave) {
    // Will cause an applier error on the slave when it's added,
    // because existing schema will be created
    slave.runSql("SET GLOBAL super_read_only=0");
    run_nolog(slave, "CREATE SCHEMA testdb");
    slave.runSql("SET GLOBAL super_read_only=1");
    master.runSql("CREATE SCHEMA testdb");
}

function inject_errant_gtid(slave) {
    slave.runSql("set @gtid_before=@@gtid_executed");

    // Force the slave to have a transaction that doesn't exist in the master
    slave.runSql("SET GLOBAL super_read_only=0");
    var before = slave.runSql("select @@gtid_executed").fetchOne()[0];
    // Dropping a DB that does not exists is enough to create a new trx.
    slave.runSql("DROP SCHEMA IF EXISTS errant_trx_db");
    var gtid = slave.runSql("select gtid_subtract(@@gtid_executed, ?)", [before]).fetchOne()[0];
    slave.runSql("SET GLOBAL super_read_only=1");
    return gtid;
}

function inject_purged_gtids(master) {
    master.runSql("CREATE SCHEMA somechange");
    master.runSql("DROP SCHEMA somechange");
    master.runSql("FLUSH BINARY LOGS");
    master.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(), INTERVAL 1 DAY)");
    EXPECT_NE("", master.runSql("SELECT @@global.gtid_purged").fetchOne()[0]);
}

function inject_empty_trx(session, trx_gtid) {
    session.runSql("SET GTID_NEXT='" + trx_gtid + "'");
    session.runSql("BEGIN");
    session.runSql("COMMIT");
    session.runSql("SET GTID_NEXT='AUTOMATIC'");
}

function filter_gtids_from(gtidset, uuid) {
  var result = [];
  var gtidlist = gtidset.split(",");
  for (var gtid of gtidlist) {
    gtid = gtid.trim();
    if (!gtid.startsWith(uuid)) {
      result.push(gtid);
    }
  }
  return result.join(",");
}

function ensure_cs_replication_channel_ready(replica_uri, master_port) {
  var replica_session = mysql.getSession(replica_uri);

  wait_channel_reconnecting(replica_session, "clusterset_replication");
  wait_channel_ready(replica_session, master_port, "clusterset_replication");

  replica_session.close();
}

function invalidate_cluster(cluster, pcluster) {
  var session = mysql.getSession("mysql://root:root@"+pcluster.status()["defaultReplicaSet"]["primary"]);
  var cluster_name = type(cluster) == "String" ? cluster : cluster.name;

  var csid = session.runSql("select clusterset_id from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0];
  var c1id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name=?", [pcluster.name]).fetchOne()[0];
  var c2id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters where cluster_name=?", [cluster_name]).fetchOne()[0];

  session.runSql("start transaction");
  session.runSql("call mysql_innodb_cluster_metadata.v2_cs_primary_changed(?, ?, '{}')", [csid, c1id]);
  session.runSql("call mysql_innodb_cluster_metadata.v2_cs_add_invalidated_member(?, ?)", [csid, c2id]);
  session.runSql("commit");

  session.close();
}

function delete_last_view(session) {
  session.runSql("begin");
  view_id = session.runSql("select max(view_id) from mysql_innodb_cluster_metadata.clusterset_views").fetchOne()[0];
  session.runSql("delete from mysql_innodb_cluster_metadata.clusterset_members where view_id=?", [view_id]);
  session.runSql("delete from mysql_innodb_cluster_metadata.clusterset_views where view_id=?", [view_id]);
  session.runSql("commit");
}

function CHECK_GTID_CONSISTENT(master_session, replica_session) {
  var row = replica_session.runSql("select @@gtid_executed, @@group_replication_view_change_uuid, @@group_replication_group_name").fetchOne();
  var view_uuid = row[1];
  var replica_gtid = filter_gtids_from(row[0], view_uuid);
  var master_gtid = master_session.runSql("select @@gtid_executed").fetchOne()[0];
  var diff = master_session.runSql("select gtid_subtract(?, ?)", [replica_gtid, master_gtid]).fetchOne()[0];
  EXPECT_EQ("", diff, `gtid_subtract(replica, master)   replica=${replica_gtid} group_name=${row[2]} view_uuid=${row[1]} master=${master_gtid}`);
}

function CHECK_ROUTER_OPTIONS_REMOVED_CLUSTER(session) {
  var res = session.runSql("select options->>'$.target_cluster' as target_cluster from mysql_innodb_cluster_metadata.routers");
    while (r = res.fetchOne()) {
      EXPECT_EQ("primary", r["target_cluster"]);
    }
}

function CHECK_CLUSTER_MEMBER(session, is_primary_member, primary_cluster, values) {
  var uri = session.uri;
  // check SRO/RO
  var sro = session.runSql("select @@global.super_read_only").fetchOne()[0];
  EXPECT_EQ(is_primary_member && primary_cluster == null ? 0 : 1, sro, uri+".super_read_only");

  var view_change_uuid = values["view_change_uuid"];

  // check skip_replica_start
  var skip_replica_start = session.runSql("SELECT variable_value FROM performance_schema.persisted_variables WHERE variable_name = 'skip_replica_start'").fetchOne()[0];
  EXPECT_EQ("ON", skip_replica_start, uri+".skip_replica_start");

  // group_replication_view_change_uuid has to be the same in the whole group
  var r = session.runSql("select @@global.group_replication_view_change_uuid").fetchOne();
  EXPECT_EQ(view_change_uuid, r[0], uri+".view_change_uuid");

  // check member action config
  var found_sro = false;
  var found_failover = false;
  var res = session.runSql("select name, event, enabled, type, priority, error_handling from performance_schema.replication_group_member_actions");
  while (r = res.fetchOne()) {
    if ("mysql_disable_super_read_only_if_primary" == r["name"]) {
      EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.sro.event");
      EXPECT_EQ(primary_cluster==null ? 1 : 0, r["enabled"], uri+".member_action.sro.enabled");
      EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.sro.type");
      EXPECT_EQ(1, r["priority"], uri+".member_action.sro.priority");
      EXPECT_EQ("IGNORE", r["error_handling"], uri+".member_action.sro.error_handling");
      found_sro = true;
    } else if ("mysql_start_failover_channels_if_primary" == r["name"]) {
      EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.failover.event");
      EXPECT_EQ(1, r["enabled"], uri+".member_action.failover.enabled");
      EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.failover.type");
      EXPECT_EQ(10, r["priority"], uri+".member_action.failover.priority");
      EXPECT_EQ("CRITICAL", r["error_handling"], uri+".member_action.failover.error_handling");
      found_failover = true;
    }
    else {
      println("UNEXPECTED member action: ", r["name"], "at", uri)
    }
  }
  EXPECT_TRUE(found_sro, uri+".member_action.sro");
  EXPECT_TRUE(found_failover, uri+".member_action.failover");

  // check replication channels
  var res = session.runSql("select c.host, c.port, c.user, c.source_connection_auto_failover, s.service_state, s.source_uuid from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='clusterset_replication'");
  var r = res.fetchOne();
  if (primary_cluster!=null && is_primary_member) {
    primary = shell.parseUri(primary_cluster.status()["defaultReplicaSet"]["primary"]);
    EXPECT_EQ("ON", r["service_state"], uri+".ar.service_state");
    EXPECT_EQ(hostname, r["host"], uri+".ar.host");
    EXPECT_EQ(primary["port"], r["port"], uri+".ar.port");
  } else {
    if (r != null)
      EXPECT_EQ("OFF", r["service_state"], uri+".async repl.service_state");
    else
      EXPECT_EQ(null, r, uri+".async repl")
  }

  // Check metadata consistency
  var server_uuid = session.runSql("select @@global.server_uuid").fetchOne()[0];
  var cluster_id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.v2_instances where mysql_server_uuid='" + server_uuid + "'").fetchOne()[0];

  EXPECT_NE(null, cluster_id);

  var res = session.runSql("select cluster_id, cluster_name, member_role, master_cluster_id, invalidated from mysql_innodb_cluster_metadata.v2_cs_members where cluster_id ='" + cluster_id + "'");

  while (r = res.fetchOne()) {
    EXPECT_EQ(cluster_id, r["cluster_id"], uri+".v2_cs_members.cluster_id");

    if (primary_cluster==null) {
      EXPECT_EQ("PRIMARY", r["member_role"], uri+".v2_cs_members.member_role");
      EXPECT_EQ(null, r["master_cluster_id"], uri+".v2_cs_members.master_cluster_id");
    } else {
      EXPECT_EQ("REPLICA", r["member_role"], uri+".v2_cs_members.member_role");

      var master_cluster_id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.v2_cs_members where member_role = 'PRIMARY' and not invalidated").fetchOne()[0];
      EXPECT_EQ(master_cluster_id, r["master_cluster_id"], uri+".v2_cs_members.master_cluster_id");
    }

    EXPECT_EQ(0, r["invalidated"], uri+".v2_cs_members.invalidated");
  }

  // check GR recovery accounts exist and are in MD
  if (!is_primary_member) {
    // Get the recovery account
    var recovery_account = session.runSql("select user from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='group_replication_recovery'").fetchOne()[0];
    EXPECT_NE(recovery_account, null, uri+".recovery_account-gr");
    var md_recovery_account = session.runSql("select (attributes->>'$.recoveryAccountUser') FROM mysql_innodb_cluster_metadata.instances  WHERE mysql_server_uuid = '" + server_uuid + "'").fetchOne()[0];
    EXPECT_NE(md_recovery_account, null, uri+".recovery_account-md");

    EXPECT_EQ(recovery_account, md_recovery_account, uri+".recovery_account");
  }

  // check CS replication account exists and is in MD
  if (primary_cluster!=null && is_primary_member) {
    var cs_replication_account = session.runSql("select user from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='clusterset_replication'").fetchOne()[0];
    EXPECT_NE(cs_replication_account, null, uri+".repl_account-pfs");
    var md_cs_replication_account = session.runSql("select (attributes->>'$.replicationAccountUser') from mysql_innodb_cluster_metadata.clusters where cluster_id = '" + cluster_id + "'").fetchOne()[0];
    EXPECT_NE(md_cs_replication_account, null, uri+".repl_account-md");

    EXPECT_EQ(cs_replication_account, md_cs_replication_account, uri+".repl_account");
  }
}

function CHECK_PRIMARY_CLUSTER(uris, cluster, values) {
  var view_change_uuid;

  if (values === undefined)
    values = {};

  // ensure cluster handlers will point to the correct MD server
  // (undefined means same as groupInformationSourceMember)
  EXPECT_EQ(undefined, cluster.status()["metadataServer"], cluster.name+".metadata server");

  for (var uri of uris) {
    var session = mysql.getSession(uri);
    println("CHECK PRIMARY CLUSTER", uri, "PRIMARY=", uris[0]);

    var row = session.runSql("select member_role, member_state from performance_schema.replication_group_members where member_id=@@server_uuid").fetchOne();
    var role = row[0];
    var state = row[1];

    EXPECT_EQ("ONLINE", state, uri+".member_state");

    if (uri == uris[0]) {
      // Get the view_change_uuid and check if not null
      view_change_uuid = session.runSql("select @@global.group_replication_view_change_uuid").fetchOne()[0];
      EXPECT_NE(null, view_change_uuid, uri+".view_change_uuid")

      EXPECT_EQ("PRIMARY", role, uri+".role_of_primary");

      values["view_change_uuid"] = view_change_uuid;

      var row = session.runSql("select @@port, @@server_uuid").fetchOne();
      var master_port = row[0];
      var master_uuid = row[1];
    } else {
      EXPECT_EQ("SECONDARY", role, uri+".role_of_secondary");
    }

    row = session.runSql("select * from performance_schema.replication_connection_configuration where channel_name='clusterset_replication'").fetchOne();
    EXPECT_EQ(null, row, uri+".async_channel_shouldnt_exist");

    // Check replication channel is OK
    row = session.runSql("select * from performance_schema.replication_connection_status where channel_name='clusterset_replication'").fetchOne();
    // EXPECT_NE(null, row, uri+".clusterset_replication");
    // EXPECT_EQ(master_user, row["Master_User"], uri+".clusterset_replication.Master_User");
    // EXPECT_EQ("No", row["Slave_IO_Running"], uri+".clusterset_replication.Slave_IO_Running");
    // EXPECT_EQ("No", row["Slave_SQL_Running"], uri+".clusterset_replication.Slave_SQL_Running");

    CHECK_CLUSTER_MEMBER(session, uri == uris[0], null, values);

    // check replication
    var res = session.runSql("select * from performance_schema.replication_asynchronous_connection_failover_managed where channel_name='clusterset_replication'");
    var r = res.fetchOne();
    EXPECT_EQ(null, r, uri+".failover channel")

    // check member action config
    var res = session.runSql("select name, event, enabled, type, priority, error_handling from performance_schema.replication_group_member_actions");
    while (r = res.fetchOne()) {
      if ("mysql_start_failover_channels_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.event");
        EXPECT_EQ(1, r["enabled"], uri+".member_action.enabled");
        EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.type");
        EXPECT_EQ(10, r["priority"], uri+".member_action.priority");
        EXPECT_EQ("CRITICAL", r["error_handling"], uri+".member_action.error_handling");
      } else if ("mysql_disable_super_read_only_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.event");
        EXPECT_EQ(1, r["enabled"], uri+".member_action.enabled");
        EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.type");
        EXPECT_EQ(1, r["priority"], uri+".member_action.priority");
        EXPECT_EQ("IGNORE", r["error_handling"], uri+".member_action.error_handling");
      }
       else {
        println("UNEXPECTED member action: ", r["name"], "at", uri)
      }
    }

    // Check if instance supports SSL or not
    var have_ssl = session.runSql("SELECT @@have_ssl").fetchOne()[0];

    // check metadata consistency
    var res = session.runSql("select async_topology_type, clusterset_id, domain_name, attributes->>'$.opt_clusterSetReplicationSslMode' as ssl_mode, router_options->>'$.target_cluster' as target_cluster, router_options->>'$.invalidated_cluster_policy' as invalidated_cluster_policy from mysql_innodb_cluster_metadata.v2_cs_clustersets");
    while (r = res.fetchOne()) {
      EXPECT_EQ("SINGLE-PRIMARY-TREE", r["async_topology_type"], uri+".v2_cs_members.v2_cs_clustersets");
      EXPECT_NE(null, r["clusterset_id"], uri+".v2_cs_members.v2_cs_clustersets");
      if (values && values["domain"])
        EXPECT_EQ(values["domain"], r["domain_name"], uri+".v2_cs_members.v2_cs_clustersets");

      if (have_ssl == "YES") {
        EXPECT_EQ("REQUIRED", r["ssl_mode"], uri+".v2_cs_members.v2_cs_clustersets");
      } else {
        EXPECT_EQ("DISABLED", r["ssl_mode"], uri+".v2_cs_members.v2_cs_clustersets");
      }

      EXPECT_EQ("primary", r["target_cluster"], uri+".v2_cs_members.v2_cs_clustersets");
      EXPECT_EQ("drop_all", r["invalidated_cluster_policy"], uri+".v2_cs_members.v2_cs_clustersets");
    }

    // If the cluster already has at least one replica, there shall be a replication channel so we must verify the SSL configuration accordingly
    var connection_type = session.runSql("select connection_type from performance_schema.threads where PROCESSLIST_COMMAND='Binlog Dump GTID'").fetchOne();

    if (connection_type != null) {
      if (have_ssl == "YES") {
        // SSL is supported so it must be enabled
        EXPECT_EQ("SSL/TLS", connection_type[0], uri+".connection_type")
      } else {
        // SSL is not supported so it must be disabled
        EXPECT_EQ("TCP/IP", connection_type[0], uri+".connection_type")
      }
    }

    session.close();
  }
}

function CHECK_REPLICA_CLUSTER(uris, pcluster, cluster, values) {
  var view_change_uuid;

  if (values === undefined) {
    values = {
      "domain": "clusterset"
    };
  }

  var primary_session = connect_cluster_primary(pcluster);
  var row = primary_session.runSql("select @@port, @@server_uuid").fetchOne();
  var master_port = row[0];
  var master_uuid = row[1];

  // ensure cluster handlers will point to the correct MD server
  // (undefined means same as groupInformationSourceMember)
  EXPECT_EQ(pcluster.status()["defaultReplicaSet"]["primary"], cluster.status()["metadataServer"], cluster.name+".metadata server");

  for (var uri of uris) {
    try {
      var session = mysql.getSession(uri);
    } catch (e) {
      println(uri, e);
      throw e;
    }
    println("CHECK REPLICA CLUSTER", uri, "PRIMARY=", uris[0]);

    var row = session.runSql("select member_role, member_state from performance_schema.replication_group_members where member_id=@@server_uuid").fetchOne();
    EXPECT_NE(null, row, uri+"/replication_group_members\t\t"+JSON.stringify(row));
    var role = row[0];
    var state = row[1];

    EXPECT_EQ("ONLINE", state, uri+".member_state");

    if (uri == uris[0]) {
      // Get the view_change_uuid and check if not null
      view_change_uuid = session.runSql("select @@global.group_replication_view_change_uuid").fetchOne()[0];
      EXPECT_NE(null, view_change_uuid, uri+".view_change_uuid")

      EXPECT_EQ("PRIMARY", role, uri+".role_of_primary");

      values["view_change_uuid"] = view_change_uuid;

      master_user = session.runSql("select attributes->>'$.replicationAccountUser' from mysql_innodb_cluster_metadata.clusters where cluster_name=?", [cluster.name]).fetchOne()[0];
    } else {
      EXPECT_EQ("SECONDARY", role, uri+".role_of_secondary");
    }

    row = session.runSql("show replica status for channel 'clusterset_replication'").fetchOne();
    if (uri == uris[0]) {
      EXPECT_NE(null, row, uri+".clusterset_replication");
      EXPECT_EQ(master_port, row["Source_Port"], uri+".clusterset_replication.Source_Port");
      EXPECT_EQ(master_uuid, row["Source_UUID"], uri+".clusterset_replication.Source_UUID");
      EXPECT_EQ(master_user, row["Source_User"], uri+".clusterset_replication.Source_User");
      EXPECT_EQ("Yes", row["Replica_IO_Running"], uri+".clusterset_replication.Replica_IO_Running");
      EXPECT_EQ(values["Replica_SQL_Running"] ? values["Replica_SQL_Running"] : "Yes", row["Replica_SQL_Running"], uri+".clusterset_replication.Replica_SQL_Running");
    } else {
      EXPECT_NE(null, row, uri+".clusterset_replication");
      EXPECT_EQ(master_user, row["Source_User"], uri+".clusterset_replication.Source_User");
      EXPECT_EQ("No", row["Replica_IO_Running"], uri+".clusterset_replication.Replica_IO_Running");
      EXPECT_EQ(values["Replica_SQL_Running"] ? values["Replica_SQL_Running"] : "No", row["Replica_SQL_Running"], uri+".clusterset_replication.Replica_SQL_Running");
    }

    CHECK_CLUSTER_MEMBER(session, uri == uris[0], pcluster, values);

    CHECK_GTID_CONSISTENT(primary_session, session);

    // check failover list if this is a primary
    var res = session.runSql("select * from performance_schema.replication_asynchronous_connection_failover_managed where channel_name='clusterset_replication'");
    var r = res.fetchOne();

    EXPECT_EQ("GroupReplication", r["MANAGED_TYPE"], uri+".failover.managed_type");
    EXPECT_EQ(pcluster.status({extended:1})["defaultReplicaSet"]["groupName"], r["MANAGED_NAME"], uri+".failover.managed_name");

    EXPECT_EQ(null, res.fetchOne(), uri+".failover (only 1 row expected)");
    session.close();
  }
  primary_session.close();
}

function CHECK_REMOVED_CLUSTER(uris, pcluster, cluster_name) {

  for (var uri of uris) {
    var session = mysql.getSession(uri);
    println("CHECK REMOVED CLUSTER", uri, uris[0]);

    if (uri == uris[0]) {
      var master_session = connect_cluster_primary(pcluster);

      CHECK_GTID_CONSISTENT(master_session, session);

      var row = master_session.runSql("select * from mysql_innodb_cluster_metadata.clusters where cluster_name=?", [cluster_name]).fetchOne();
      EXPECT_EQ(null, row, "cluster md check for "+cluster_name);

      master_session.close();
    }

    // check member action config
    var res = session.runSql("select name, event, enabled, type, priority, error_handling from performance_schema.replication_group_member_actions");
    while (r = res.fetchOne()) {
      if ("mysql_disable_super_read_only_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.sro.event");
        EXPECT_EQ(1, r["enabled"], uri+".member_action.sro.enabled");
        EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.sro.type");
        EXPECT_EQ(1, r["priority"], uri+".member_action.sro.priority");
        EXPECT_EQ("IGNORE", r["error_handling"], uri+".member_action.sro.error_handling");
      } else if ("mysql_start_failover_channels_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.failover.event");
        EXPECT_EQ(1, r["enabled"], uri+".member_action.failover.enabled");
        EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.failover.type");
        EXPECT_EQ(10, r["priority"], uri+".member_action.failover.priority");
        EXPECT_EQ("CRITICAL", r["error_handling"], uri+".member_action.failover.error_handling");
      } else {
        println("UNEXPECTED member action: ", r["name"], "at", uri)
      }
    }

    // check replication channels
    var res = session.runSql("select c.host, c.port, c.user, c.source_connection_auto_failover, s.service_state, s.source_uuid from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='clusterset_replication'");
    var r = res.fetchOne();
    EXPECT_EQ(null, r, uri+".async repl")

    // check SRO at primary
    if (uri == uris[0]) {
      var sro = session.runSql("select @@global.super_read_only").fetchOne()[0];
      EXPECT_TRUE(sro, uri+".super_read_only");
    }

    // check skip_replica_start
    var skip_replica_start = session.runSql("SELECT variable_value FROM performance_schema.persisted_variables WHERE variable_name = 'skip_replica_start'").fetchOne()[0];
    EXPECT_EQ("OFF", skip_replica_start, uri+".skip_replica_start");

    // check cluster replication user was removed
    var unexpected_users = session.runSql("select user from mysql.user where user like 'mysql_innodb_cs_%' and user not in (select attributes->>'$.replicationAccountUser' u from mysql_innodb_cluster_metadata.clusters)").fetchAll();
    EXPECT_EQ([], unexpected_users, "unexpected_cs_users");

  session.close();
  }
}

function CHECK_REMOVED_INSTANCE(uri) {
  var session = mysql.getSession(uri);
  println("CHECK REMOVED INSTANCE", uri);

  // check member action config
  var res = session.runSql("select name, event, enabled, type, priority, error_handling from performance_schema.replication_group_member_actions");
  while (r = res.fetchOne()) {
    if ("mysql_disable_super_read_only_if_primary" == r["name"]) {
      EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.sro.event");
      EXPECT_EQ(1, r["enabled"], uri+".member_action.sro.enabled");
      EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.sro.type");
      EXPECT_EQ(1, r["priority"], uri+".member_action.sro.priority");
      EXPECT_EQ("IGNORE", r["error_handling"], uri+".member_action.sro.error_handling");
    } else if ("mysql_start_failover_channels_if_primary" == r["name"]) {
      EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.failover.event");
      EXPECT_EQ(1, r["enabled"], uri+".member_action.failover.enabled");
      EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.failover.type");
      EXPECT_EQ(10, r["priority"], uri+".member_action.failover.priority");
      EXPECT_EQ("CRITICAL", r["error_handling"], uri+".member_action.failover.error_handling");
    } else {
      println("UNEXPECTED member action: ", r["name"], "at", uri)
    }
  }

  // check replication channel
  var res = session.runSql("select c.host, c.port, c.user, c.source_connection_auto_failover, s.service_state, s.source_uuid from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='clusterset_replication'");
  var r = res.fetchOne();
  EXPECT_EQ(null, r, uri+".async repl")
}

function CHECK_REJOINED_INSTANCE(uri, primary_cluster, is_primary_member) {
  var session = mysql.getSession(uri);
  println("CHECK REJOINED INSTANCE", uri);

  // check replication channel
  var res = session.runSql("select c.host, c.port, c.user, c.source_connection_auto_failover, s.service_state, s.source_uuid from performance_schema.replication_connection_status s join performance_schema.replication_connection_configuration c on s.channel_name = c.channel_name where s.channel_name='clusterset_replication'");
  var r = res.fetchOne();
  if (primary_cluster) {
    if (is_primary_member) {
      primary = shell.parseUri(primary_cluster.status()["defaultReplicaSet"]["primary"]);
      EXPECT_EQ("ON", r["service_state"], uri+".ar.service_state");
      EXPECT_EQ(hostname, r["host"], uri+".ar.host");
      EXPECT_EQ(primary["port"], r["port"], uri+".ar.port");
    } else {
      EXPECT_EQ("OFF", r["service_state"], uri+".ar.service_state");
      EXPECT_EQ(hostname, r["host"], uri+".ar.host");
      EXPECT_EQ(primary["port"], r["port"], uri+".ar.port");
    }
  } else {
    if (r != null)
      EXPECT_EQ("OFF", r["service_state"], uri+".async repl.service_state");
    else
      EXPECT_EQ(null, r, uri+".async repl")
  }
}

function CHECK_INVALIDATED_CLUSTER(uris, pcluster, cluster) {
  var session = mysql.getSession("root:root@"+pcluster.status()["defaultReplicaSet"]["primary"]);

  // check metadata
  var row = session.runSql("select * from mysql_innodb_cluster_metadata.v2_cs_members where cluster_name = ?", [cluster.name]).fetchOne();
  EXPECT_NE(null, row, cluster.name+".md");
  EXPECT_EQ(1, row["invalidated"], cluster.name+".md.invalidated");

  session.close();
}

function CHECK_INVALIDATED_CLUSTER_NAMED(session, cluster_name) {
  // check metadata
  var row = session.runSql("select * from mysql_innodb_cluster_metadata.v2_cs_members where cluster_name = ?", [cluster_name]).fetchOne();
  EXPECT_NE(null, row, cluster_name+".md");
  EXPECT_EQ(1, row["invalidated"], cluster_name+".md.invalidated");
}


function CHECK_CLUSTER_SET(session) {
  // check that there are no GR accounts that don't belong to anything
  var bogus_users = session.runSql("select user from mysql.user where user like 'mysql_innodb_cluster_%' and user not in (select attributes->>'$.recoveryAccountUser' u from mysql_innodb_cluster_metadata.v2_instances)").fetchAll();
  EXPECT_EQ([], bogus_users, "unexpected_gr_users");

  // check that there are no AR accounts that don't belong to anything
  var bogus_users = session.runSql("select user from mysql.user where user like 'mysql_innodb_cs_%' and user not in (select attributes->>'$.replicationAccountUser' u from mysql_innodb_cluster_metadata.clusters)").fetchAll();
  EXPECT_EQ([], bogus_users, "unexpected_ar_users");
}
