// Plain cluster test, use as a template for other tests that check
// a specific feature/aspect across the whole API

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
shell.options.useWizards = false;

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

expected_pids1 = get_open_sessions(session1);
expected_pids2 = get_open_sessions(session2);

//@<> configureLocalInstance

dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

testutil.restartSandbox(__mysql_sandbox_port1);

session1 = mysql.getSession(__sandbox_uri1);
expected_pids1 = get_open_sessions(session1);

//@<> configureInstance
dba.configureInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> createCluster
shell.connect(__sandbox_uri1);

expected_pids1 = get_open_sessions(session1);

cluster = dba.createCluster("mycluster");

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> status
status = cluster.status();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> checkInstanceState
cluster.checkInstanceState(__sandbox_uri2);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> describe
cluster.describe();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> disconnect
cluster.disconnect();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> getCluster
cluster = dba.getCluster();

//@<> addInstance
cluster.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> removeInstance
cluster.removeInstance(__sandbox_uri2);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

cluster.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

//@<> setPrimaryInstance {VER(>=8.0.0)}
cluster.setPrimaryInstance(__sandbox_uri2);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

cluster.setPrimaryInstance(__sandbox_uri1);

//@<> rejoinInstance
session2.runSql("STOP group_replication");

cluster.rejoinInstance(__sandbox_uri2);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> forceQuorumUsingPartitionOf
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING),UNREACHABLE");

cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);

check_open_sessions(session1, expected_pids1);

testutil.startSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2);

expected_pids2 = get_open_sessions(session2);

cluster.rejoinInstance(__sandbox_uri2);

//@<> rebootClusterFromCompleteOutage
session1.runSql("STOP group_replication");
session2.runSql("STOP group_replication");

cluster = dba.rebootClusterFromCompleteOutage();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

// TODO(alfredo) - the reboot should auto-rejoin all members
cluster.rejoinInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> setOption
cluster.setOption("clusterName", "clooster");

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

custom_weigth=50;

//@<> setInstanceOption {VER(>=8.0.0)}
custom_weigth=20;
cluster.setInstanceOption(__sandbox_uri1, "memberWeight", custom_weigth);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> options
cluster.options();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> switchToMultiPrimaryMode {VER(>=8.0.0)}
cluster.switchToMultiPrimaryMode();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> switchToSinglePrimaryMode {VER(>=8.0.0)}
cluster.switchToSinglePrimaryMode();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

cluster.setPrimaryInstance(__sandbox_uri1);

shell.connect(__sandbox_uri1);
cluster = dba.getCluster(); // shouldn't be necessary

//@<> rescan
// delete sb2 from the metadata so that rescan picks it up
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port2]);

cluster.rescan();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

cluster.listRouters();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> removeRouterMetadata
cluster.removeRouterMetadata("routerhost1::system");

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

cluster.listRouters();

//@<> createCluster(adopt)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
cluster = dba.createCluster("adopted", {adoptFromGR:true});

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

cluster.status();

//@<> dissolve
cluster.dissolve();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
