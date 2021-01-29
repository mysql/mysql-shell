
function reset_instance(session) {
    session.runSql("STOP SLAVE");
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

function reset_provision_instance(session, master) {
    reset_instance(session);

    setup_slave(session, shell.parseUri(master.uri).port);
    session.runSql("SELECT wait_for_executed_gtid_set(?)", [master.runSql("SELECT @@gtid_executed").fetchOne()[0]]);
    session.runSql("STOP SLAVE");
    session.runSql("RESET SLAVE ALL");
}

function strip_status(status) {
    for (var t in status.replicaSet.topology) {
        if (status.replicaSet.topology[t].replication) {
            delete status.replicaSet.topology[t].replication.replicationLag;
            delete status.replicaSet.topology[t].replication.receiverTimeSinceLastMessage;

            delete status.replicaSet.topology[t].replication.applierQueuedTransactionSet;
            delete status.replicaSet.topology[t].replication.applierQueuedTransactionSetSize;
        }
    }
    return status;
}

function strip_slave_status(status) {
    copy = Object.assign({}, status);
    copy["Retrieved_Gtid_Set"] = undefined;
    copy["Executed_Gtid_Set"] = undefined;
    return copy;
}


function rebuild_rs(gtid_set_is_complete) {
    if (gtid_set_is_complete == undefined) gtid_set_is_complete = true;

    println("BEGIN REBUILD_RS");
    shell.connect(__sandbox_uri1);
    var s2 = mysql.getSession(__sandbox_uri2);
    var s3 = mysql.getSession(__sandbox_uri3);
    reset_instance(session);
    reset_instance(s2);
    reset_instance(s3);
    var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:gtid_set_is_complete});
    rs.addInstance(__sandbox_uri2);
    rs.addInstance(__sandbox_uri3);
    s2.close();
    s3.close();
    println("END REBUILD_RS");
    return rs;
}

function rebuild_rs_delayed(delay, gtid_set_is_complete) {
    if (gtid_set_is_complete == undefined) gtid_set_is_complete = true;

    println("BEGIN REBUILD_RS");
    shell.connect(__sandbox_uri1);
    var s2 = mysql.getSession(__sandbox_uri2);
    var s3 = mysql.getSession(__sandbox_uri3);
    reset_instance(session);
    reset_instance(s2);
    reset_instance(s3);
    var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:gtid_set_is_complete});
    rs.addInstance(__sandbox_uri2);
    rs.addInstance(__sandbox_uri3);
    // Make sure MD changes are applied on all instances before delaying instance 2.
    testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
    testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);
    var used_delay = [delay ? delay : 60];
    println("Adding " + used_delay + "s delay to sb2");
    s2.runSql("STOP SLAVE");
    s2.runSql("CHANGE MASTER TO master_delay=?", used_delay);
    s2.runSql("START SLAVE");
    s2.close();
    s3.close();
    println("END REBUILD_RS");
    return rs;
}

function start_standalone_gr(session, grport) {
    var dllext = (__os_type != "windows" ? "so" : "dll");
    session.runSql("install plugin group_replication soname /*(*/'group_replication."+dllext+"'/*)*/");
    session.runSql("SET GLOBAL group_replication_bootstrap_group=1");
    session.runSql("SET GLOBAL group_replication_group_name=uuid()");
    session.runSql("SET GLOBAL group_replication_local_address='localhost:"+grport+"'");
    session.runSql("START group_replication");
}

function stop_standalone_gr(session) {
    session.runSql("STOP group_replication");
    session.runSql("RESET SLAVE ALL");
}

function setup_slave(session, master_port, channel_name) {
    if (channel_name == undefined)
        channel_name = "";
    session.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root', master_auto_position=1 FOR CHANNEL ?", [master_port, channel_name]);
    session.runSql("START SLAVE FOR CHANNEL ?", [channel_name]);
    while (true) {
        r = session.runSql("SHOW SLAVE STATUS").fetchOne();
        if (r.Slave_IO_Running != 'Connecting')
            break;
    }
}

function wait_slave(session) {
    while (true) {
        r = session.runSql("SHOW SLAVE STATUS").fetchOne();
        if (r.Slave_SQL_Running_State == "<<<__replica_keyword_capital>>> has read all relay log; waiting for more updates")
            break;
    }
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
    // Force the slave to have a transaction that doesn't exist in the master
    slave.runSql("SET GLOBAL super_read_only=0");
    // Dropping a DB that does not exists is enough to create a new trx.
    slave.runSql("DROP SCHEMA IF EXISTS errant_trx_db");
    slave.runSql("SET GLOBAL super_read_only=1");
}

function inject_purged_gtids(master) {
    master.runSql("CREATE SCHEMA somechange");
    master.runSql("DROP SCHEMA somechange");
    master.runSql("FLUSH BINARY LOGS");
    master.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(), INTERVAL 1 DAY)");
    EXPECT_NE("", master.runSql("SELECT @@global.gtid_purged").fetchOne()[0]);
}


function repl_snapshot(session) {
    var slaves = session.runSql("SELECT * FROM mysql.slave_master_info").fetchAll();
    var users = session.runSql("SELECT user,host,authentication_string FROM mysql.user").fetchAll();
    var sro = get_sysvar(session, "super_read_only");
    var ro = get_sysvar(session, "read_only");

    if (session.runSql("SHOW SCHEMAS LIKE 'mysql_innodb_cluster_metadata'").fetchOne()) {
        var clusters = session.runSql("SELECT * FROM mysql_innodb_cluster_metadata.clusters").fetchAll();
        var instances = session.runSql("SELECT * FROM mysql_innodb_cluster_metadata.instances").fetchAll();
        var members = session.runSql("SELECT * FROM mysql_innodb_cluster_metadata.v2_ar_members").fetchAll();

        return {"super_read_only":sro, "read_only":ro, "slave_master_info": slaves, "users":users, "clusters":clusters, "instances": instances, "members": members};
    } else {
        return {"super_read_only":sro, "read_only":ro, "slave_master_info": slaves, "users":users};
    }
}

function inject_empty_trx(session, trx_gtid) {
    session.runSql("SET GTID_NEXT='" + trx_gtid + "'");
    session.runSql("BEGIN");
    session.runSql("COMMIT");
    session.runSql("SET GTID_NEXT='AUTOMATIC'");
}
