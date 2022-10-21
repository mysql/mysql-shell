//@ {VER(>=8.0.11)}

// Plain replicaset setup test, use as a template for other tests that check
// a specific feature/aspect across the whole API

// simple_plain should be _norecord to force at least one direct execution of the API in all platforms

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
testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.options.useWizards = false;

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

expected_pids1 = get_open_sessions(session1);
expected_pids2 = get_open_sessions(session2);
expected_pids3 = get_open_sessions(session3);

//@ configureReplicaSetInstance + create admin user
EXPECT_DBA_THROWS_PROTOCOL_ERROR("Dba.configureReplicaSetInstance", dba.configureReplicaSetInstance, __sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

dba.configureReplicaSetInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

testutil.restartSandbox(__mysql_sandbox_port1);

session1 = mysql.getSession(__sandbox_uri1);
expected_pids1 = get_open_sessions(session1);

//@ configureReplicaSetInstance
dba.configureReplicaSetInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"});
dba.configureReplicaSetInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@ createReplicaSet
shell.connect(__sandbox_uri1);

expected_pids1 = get_open_sessions(session1);

rs = dba.createReplicaSet("myrs");
check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@ status
rs.status();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@ disconnect
rs.disconnect();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@ getReplicaSet
rs = dba.getReplicaSet();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@ addInstance (incremental)
EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("ReplicaSet.addInstance", rs.addInstance, __sandbox_uri3, {recoveryMethod:'incremental'});

rs.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

EXPECT_REPLICAS_USE_SSL(session1, 1);

//@ addInstance (clone) {VER(>=8.0.17)}
rs.addInstance(__sandbox_uri2, {recoveryMethod:'clone'});

session2 = mysql.getSession(__sandbox_uri2);
expected_pids2 = get_open_sessions(session2);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

EXPECT_REPLICAS_USE_SSL(session1, 2);

//@ addInstance (no clone) {VER(<8.0.17)}
rs.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

EXPECT_REPLICAS_USE_SSL(session1, 2);

//@ removeInstance
rs.removeInstance(__sandbox_uri2);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

rs.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

EXPECT_REPLICAS_USE_SSL(session1, 2);

//@ setPrimaryInstance
EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("ReplicaSet.setPrimaryInstance", rs.setPrimaryInstance, __sandbox_uri3);

rs.setPrimaryInstance(__sandbox_uri3);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

EXPECT_REPLICAS_USE_SSL(session3, 2);

//@ forcePrimaryInstance (prepare)
testutil.stopSandbox(__mysql_sandbox_port3, {wait:1});
rs = dba.getReplicaSet();

rs.status();

//@ forcePrimaryInstance
EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("ReplicaSet.forcePrimaryInstance", rs.forcePrimaryInstance, __sandbox_uri1);

rs.forcePrimaryInstance(__sandbox_uri1);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

EXPECT_REPLICAS_USE_SSL(session1, 1);

//@ rejoinInstance
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitSandboxAlive(__mysql_sandbox_port3);

session3 = mysql.getSession(__sandbox_uri3);
expected_pids3 = get_open_sessions(session3);

EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("ReplicaSet.rejoinInstance", rs.rejoinInstance, __sandbox_uri3);

rs.rejoinInstance(__sandbox_uri3);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

EXPECT_REPLICAS_USE_SSL(session1, 2);

//@ rejoinInstance (clone) {VER(>=8.0.17)}
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");

rs.rejoinInstance(__sandbox_uri3, {recoveryMethod:"clone"});

session3 = mysql.getSession(__sandbox_uri3);
expected_pids3 = get_open_sessions(session3);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

EXPECT_REPLICAS_USE_SSL(session1, 2);

//@ listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

rs.listRouters();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

//@ removeRouterMetadata
rs.removeRouterMetadata("routerhost1::system");

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

rs.listRouters();

//@ createReplicaSet(adopt)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
rs = dba.createReplicaSet("adopted", {adoptFromAR:true});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);
check_open_sessions(session3, expected_pids3);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1)
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1)

rs.status();

EXPECT_REPLICAS_USE_SSL(session1, 2);

//@<> createReplicaSet without ssl in seed
reset_instance(session1);
reset_instance(session2);
reset_instance(session3);

testutil.changeSandboxConf(__mysql_sandbox_port2, "skip_ssl", "1");
testutil.changeSandboxConf(__mysql_sandbox_port3, "skip_ssl", "1");

testutil.restartSandbox(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port3);

session2 = mysql.getSession(__sandbox_uri2 + "?ssl-mode=DISABLED&get-server-public-key=1");
session3 = mysql.getSession(__sandbox_uri3 + "?ssl-mode=DISABLED&get-server-public-key=1");

shell.connect(__sandbox_uri2);
rs = dba.createReplicaSet("rs", {gtidSetIsComplete:1});

rs.addInstance(__sandbox_uri3);
rs.addInstance(__sandbox_uri1);

rs.setPrimaryInstance(__sandbox_uri1);
rs.setPrimaryInstance(__sandbox_uri2);

//@<> createReplicaSet with ssl in seed
reset_instance(session1);
reset_instance(session2);
reset_instance(session3);

shell.connect(__sandbox_uri1);
rs = dba.createReplicaSet("rs", {gtidSetIsComplete:1});

EXPECT_THROWS(function(){rs.addInstance(__sandbox_uri3);}, `Instance '127.0.0.1:${__mysql_sandbox_port3}' does not support TLS and cannot join a replicaset with TLS (encryption) enabled.`);
EXPECT_THROWS(function(){rs.addInstance(__sandbox_uri2);}, `Instance '127.0.0.1:${__mysql_sandbox_port2}' does not support TLS and cannot join a replicaset with TLS (encryption) enabled.`);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
