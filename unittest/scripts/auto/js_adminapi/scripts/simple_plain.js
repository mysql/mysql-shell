// Plain cluster test, use as a template for other tests that check
// a specific feature/aspect across the whole API

// This should combine all server options that are supported (ie won't be reconfigured away)
// and not mutually exclusive. Other tests derived from this should have this removed.

k_mycnf_options = {
    sql_mode: 'NO_BACKSLASH_ESCAPES,ANSI_QUOTES,NO_AUTO_VALUE_ON_ZERO', 
    autocommit:1,
    require_secure_transport:1,
    loose_group_replication_consistency:"BEFORE",
}
// TODO - also add PAD_CHAR_TO_FULL_LENGTH to sql_mode, but fix #34961015 1st

if (__version_num > 80000) {
    k_mycnf_options = {
        plugin_load:"validate_password" + (__os_type != "windows" ? ".so" : ".dll"),
        loose_validate_password_policy:"STRONG",
        ...k_mycnf_options
    }
}

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

function get_uri(port, pwd) {
    if (pwd === undefined) {
        return `mysql://root@localhost:${port}`
    }
    else {
        return `mysql://root:${pwd}@localhost:${port}`
    }
}

//@<> Setup

var pwd = "root";
var pwdAdmin = "bla";
var pwdExtra = "boo";
if (__version_num > 80000) {
    pwd = __secure_password;
    pwdAdmin = "C0mPL1CAT3D_pa22w0rd_adm1n";
    pwdExtra = "C0mPL1CAT3D_pa22w0rd_3x7ra";
}

// override default sql_mode to test that we always override it
testutil.deployRawSandbox(__mysql_sandbox_port1, pwd, k_mycnf_options);
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, pwd, k_mycnf_options);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
shell.options.useWizards = false;

session1 = mysql.getSession(__sandbox_uri1, pwd);
session2 = mysql.getSession(__sandbox_uri2, pwd);

expected_pids1 = get_open_sessions(session1);
expected_pids2 = get_open_sessions(session2);

//@<> configureLocalInstance
EXPECT_DBA_THROWS_PROTOCOL_ERROR("Dba.configureLocalInstance", dba.configureLocalInstance, get_uri(__mysql_sandbox_port1, pwd), {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});

WIPE_OUTPUT();

EXPECT_NO_THROWS(function(){ dba.configureLocalInstance(get_uri(__mysql_sandbox_port1, pwd), {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)}); });

if (testutil.versionCheck(__version, "<", "8.0.0")) {
    EXPECT_OUTPUT_CONTAINS(`The instance '127.0.0.1:${__mysql_sandbox_port1}' was configured to be used in an InnoDB Cluster.`);
}

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

testutil.restartSandbox(__mysql_sandbox_port1);

session1 = mysql.getSession(__sandbox_uri1, pwd);
expected_pids1 = get_open_sessions(session1);

//@<> configureInstance
EXPECT_DBA_THROWS_PROTOCOL_ERROR("Dba.configureInstance", dba.configureInstance, get_uri(__mysql_sandbox_port2, pwd), {clusterAdmin:"admin", clusterAdminPassword:pwdAdmin});

WIPE_OUTPUT();

EXPECT_NO_THROWS(function(){ dba.configureInstance(get_uri(__mysql_sandbox_port2, pwd), {clusterAdmin:"admin", clusterAdminPassword:pwdAdmin}); });

if (testutil.versionCheck(__version, ">=", "8.0.0")) {
    EXPECT_OUTPUT_CONTAINS(`The instance '127.0.0.1:${__mysql_sandbox_port2}' is valid for InnoDB Cluster usage.`);
}

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> getCluster() - no session - useWizards = false
EXPECT_THROWS(function() { dba.getCluster();}, `An open session is required to perform this operation.`);

//@<> getCluster() - no session - useWizards = true
shell.options.useWizards = true;
EXPECT_THROWS(function() { dba.getCluster();}, `An open session is required to perform this operation.`);
shell.options.useWizards = false;

//@<> createCluster
shell.connect(__sandbox_uri1, pwd);

expected_pids1 = get_open_sessions(session1);

var cluster;
EXPECT_NO_THROWS(function(){ cluster = dba.createCluster("mycluster"); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> status
status = cluster.status();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> checkInstanceState
EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("Cluster.checkInstanceState", cluster.checkInstanceState, get_uri(__mysql_sandbox_port2, pwd));

EXPECT_NO_THROWS(function(){ cluster.checkInstanceState(get_uri(__mysql_sandbox_port2, pwd)); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> describe
cluster.describe();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> disconnect
EXPECT_NO_THROWS(function(){ cluster.disconnect(); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> getCluster
EXPECT_NO_THROWS(function(){ cluster = dba.getCluster(); });

//@<> addInstance unsupported X-protocol error
EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("Cluster.addInstance", cluster.addInstance, get_uri(__mysql_sandbox_port2), {recoveryMethod:'incremental'});

//@<> addInstance using clone recovery {VER(>=8.0.17)}
EXPECT_NO_THROWS(function(){ cluster.addInstance(get_uri(__mysql_sandbox_port2), {recoveryMethod:'clone'}); });

EXPECT_OUTPUT_CONTAINS(`Clone based recovery selected through the recoveryMethod option`);
EXPECT_OUTPUT_CONTAINS(`The instance '127.0.0.1:${__mysql_sandbox_port2}' was successfully added to the cluster.`);

session2 = mysql.getSession(__sandbox_uri2, pwd);
expected_pids2 = get_open_sessions(session2);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> addInstance using incremental recovery {VER(<8.0.17)}
EXPECT_NO_THROWS(function(){ cluster.addInstance(get_uri(__mysql_sandbox_port2), {recoveryMethod:'incremental'}); });

EXPECT_OUTPUT_CONTAINS(`The instance '127.0.0.1:${__mysql_sandbox_port2}' was successfully added to the cluster.`);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> removeInstance
EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("Cluster.removeInstance", cluster.removeInstance, get_uri(__mysql_sandbox_port2));

EXPECT_NO_THROWS(function(){ cluster.removeInstance(get_uri(__mysql_sandbox_port2)); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

EXPECT_NO_THROWS(function(){ cluster.addInstance(get_uri(__mysql_sandbox_port2), {recoveryMethod:'incremental'}); });

//@<> setPrimaryInstance {VER(>=8.0.0)}
CHECK_MYSQLX_EXPECT_THROWS_ERROR(`The instance '${get_mysqlx_endpoint(get_uri(__mysql_sandbox_port2))}' does not belong to the Cluster: 'mycluster'.`, cluster.setPrimaryInstance, get_uri(__mysql_sandbox_port2));

EXPECT_NO_THROWS(function(){ cluster.setPrimaryInstance(get_uri(__mysql_sandbox_port2)); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

EXPECT_NO_THROWS(function(){ cluster.setPrimaryInstance(get_uri(__mysql_sandbox_port1)); });

//@<> rejoinInstance
session2.runSql("STOP group_replication");

EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("Cluster.rejoinInstance", cluster.rejoinInstance, get_uri(__mysql_sandbox_port2));

EXPECT_NO_THROWS(function(){ cluster.rejoinInstance(get_uri(__mysql_sandbox_port2)); });

//@<> skip_replica_start is kept unchanged if the instance does not belong to a ClusterSet {VER(>=8.0.27)}
var skip_replica_start = session2.runSql("SELECT @@skip_replica_start").fetchOne()[0];
EXPECT_EQ(0, skip_replica_start);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> forceQuorumUsingPartitionOf
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING),UNREACHABLE");

EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("Cluster.forceQuorumUsingPartitionOf", cluster.forceQuorumUsingPartitionOf, get_uri(__mysql_sandbox_port1, pwd));

EXPECT_NO_THROWS(function(){ cluster.forceQuorumUsingPartitionOf(get_uri(__mysql_sandbox_port1, pwd)); });

check_open_sessions(session1, expected_pids1);

testutil.startSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2, pwd);

expected_pids2 = get_open_sessions(session2);

EXPECT_NO_THROWS(function(){ cluster.rejoinInstance(get_uri(__mysql_sandbox_port2)); });

//@<> rebootClusterFromCompleteOutage
testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2], pwd);

EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage(); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

// TODO(alfredo) - the reboot should auto-rejoin all members
EXPECT_NO_THROWS(function(){ cluster.rejoinInstance(get_uri(__mysql_sandbox_port2)); });
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> setOption
EXPECT_NO_THROWS(function(){ cluster.setOption("clusterName", "clooster"); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

custom_weigth=50;

//@<> setInstanceOption {VER(>=8.0.0)}
custom_weigth=20;

EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("Cluster.setInstanceOption", cluster.setInstanceOption, get_uri(__mysql_sandbox_port1), "memberWeight", custom_weigth);

EXPECT_NO_THROWS(function(){ cluster.setInstanceOption(get_uri(__mysql_sandbox_port1), "memberWeight", custom_weigth); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> options
cluster.options();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> switchToMultiPrimaryMode {VER(>=8.0.0)}
EXPECT_NO_THROWS(function(){ cluster.switchToMultiPrimaryMode(); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> switchToSinglePrimaryMode {VER(>=8.0.0)}
EXPECT_NO_THROWS(function(){ cluster.switchToSinglePrimaryMode(); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

EXPECT_NO_THROWS(function(){ cluster.setPrimaryInstance(get_uri(__mysql_sandbox_port1)); });

shell.connect(__sandbox_uri1, pwd);
cluster = dba.getCluster(); // shouldn't be necessary

//@<> rescan

// delete sb2 from the metadata so that rescan picks it up
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port2]);

EXPECT_NO_THROWS(function(){ cluster.rescan(); });
EXPECT_OUTPUT_CONTAINS(`A new instance '127.0.0.1:${__mysql_sandbox_port2}' was discovered in the cluster.`);

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (1, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

cluster.listRouters();

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> removeRouterMetadata
cluster.removeRouterMetadata("routerhost1::system");

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

cluster.listRouters();


//@<> setupRouterAccount
cluster.setupRouterAccount("router@'%'", {password:pwdExtra});

session.runSql("show grants for router@'%'");

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> setupAdminAccount
cluster.setupAdminAccount("cadmin@'%'", {password:pwdExtra});

session.runSql("show grants for cadmin@'%'");

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> createCluster(adopt)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");

var cluster;
EXPECT_NO_THROWS(function(){ cluster = dba.createCluster("adopted", {adoptFromGR:true}); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

cluster.status();

//@<> dissolve
EXPECT_NO_THROWS(function(){ cluster.dissolve(); });

check_open_sessions(session1, expected_pids1);
check_open_sessions(session2, expected_pids2);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
