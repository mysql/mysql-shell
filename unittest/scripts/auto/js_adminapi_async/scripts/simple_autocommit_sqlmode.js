//@<> {VER(>=8.0.11)}

// Full rs test using autocommit=0 and sql_mode with weird non-default values

function get_open_sessions(session) {
    var r = session.runSql("SELECT processlist_id FROM performance_schema.threads WHERE processlist_user is not null");
    pids = []
    while (row = r.fetchOne()) {
        pids.push(row[0]);
    }
    return pids;
}

// function to detect that a command has closed all sessions it has opened
function check_open_sessions(session, ignore_pids) {
    var r = session.runSql("SELECT processlist_id, processlist_user, processlist_info FROM performance_schema.threads WHERE processlist_user is not null");
    var flag = false;
    if (row = r.fetchOne()) {
        if (!ignore_pids.includes(row[0])) {
            if (!flag) {
                flag = true;
                println("Unexpected sessions open:");
            }
            println(row[0], "\t", row[1], "\t", row[2]);
        }
    }
    if (flag) {
        println();
        testutil.fail("DB session leak detected");
    }
}

//@<> Setup

var mycnf = {};
mycnf["autocommit"] = 0;
mycnf["sql_mode"] = "ANSI_QUOTES,NO_AUTO_VALUE_ON_ZERO,NO_BACKSLASH_ESCAPES";
// BUG: setting sql_mode to PAD_CHAR_TO_FULL_LENGTH doesn't work because 
// precondition checks use the main shell session, which we don't change.
// The checks have to be changed to use their own session.

testutil.deployRawSandbox(__mysql_sandbox_port1, "root", mycnf);
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", mycnf);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", mycnf);
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.options.useWizards = false;

session1 = mysql.getSession(__sandbox_uri1);
session1.runSql("set autocommit=1");
session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("set autocommit=1");
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("set autocommit=1");

expected_pids1 = get_open_sessions(session1);
expected_pids2 = get_open_sessions(session2);
expected_pids3 = get_open_sessions(session3);

//@<> configureReplicaSetInstance + create admin user

dba.configureReplicaSetInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

testutil.restartSandbox(__mysql_sandbox_port1);

session1 = mysql.getSession(__sandbox_uri1);
session1.runSql("set autocommit=1");
expected_pids1 = get_open_sessions(session1);

//@<> configureReplicaSetInstance
dba.configureReplicaSetInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"});
dba.configureReplicaSetInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> createReplicaSet
shell.connect(__sandbox_uri1);
session.runSql("set autocommit=1");

expected_pids1 = get_open_sessions(session1);

rs = dba.createReplicaSet("myrs");

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> status
rs.status();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> disconnect
rs.disconnect();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> getReplicaSet
rs = dba.getReplicaSet();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> addInstance (incremental)
rs.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> addInstance (clone) {VER(>=8.0.17)}
rs.addInstance(__sandbox_uri2, {recoveryMethod:'clone'});

session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("set autocommit=1");

expected_pids2 = get_open_sessions(session2);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> addInstance (no clone) {VER(<8.0.17)}
rs.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> removeInstance
rs.removeInstance(__sandbox_uri2);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

rs.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

//@<> setPrimaryInstance
rs.setPrimaryInstance(__sandbox_uri3);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> forcePrimaryInstance (prepare)
testutil.stopSandbox(__mysql_sandbox_port3, {wait:1});
rs = dba.getReplicaSet();

rs.status();

//@<> forcePrimaryInstance
rs.forcePrimaryInstance(__sandbox_uri1);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> rejoinInstance
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitSandboxAlive(__mysql_sandbox_port3);

session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("set autocommit=1");
expected_pids3 = get_open_sessions(session3);

rs.rejoinInstance(__sandbox_uri3);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> rejoinInstance (clone) {VER(>=8.0.17)}
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");
rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"clone"});

session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("set autocommit=1");
expected_pids3 = get_open_sessions(session3);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (1, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

rs.listRouters();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@<> removeRouterMetadata
rs.removeRouterMetadata("routerhost1::system");

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

rs.listRouters();

//@<> createReplicaSet(adopt)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
rs = dba.createReplicaSet("adopted", {adoptFromAR:true});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

rs.status();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
