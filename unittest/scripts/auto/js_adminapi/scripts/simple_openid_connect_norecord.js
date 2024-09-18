//@ {hasAuthEnvironment('OPENID_CONNECT')}

//@<> Setup
if (debugAuthPlugins()) {
    dba.verbose = 1;
}

var OPENID_CONNECT_USER = "openid-test";
var OPENID_CONNECT_SUBJECT = "openid-test-subject";
var OPENID_CONNECT_PROVIDER = "openid-test-issuer";

var cnfg_path = os.path.join(__data_path, "auth", "openid-test-jwk-cfg");
var token_path = os.path.join(__data_path, "auth", "openid-test-jwt.txt");

var server_conf = getAuthServerConfig('OPENID_CONNECT');

testutil.deploySandbox(__mysql_sandbox_port1, "root", server_conf);
var openid_available = 
    isAuthMethodSupported('OPENID_CONNECT', __sandbox_uri1);

if(openid_available) {
    testutil.deploySandbox(__mysql_sandbox_port2, "root", server_conf);
    testutil.deploySandbox(__mysql_sandbox_port3, "root", server_conf);
    openid_available = 
        isAuthMethodSupported('OPENID_CONNECT', __sandbox_uri2) &&
        isAuthMethodSupported('OPENID_CONNECT', __sandbox_uri3);
    if(!openid_available) {
        testutil.destroySandbox(__mysql_sandbox_port1);
        testutil.destroySandbox(__mysql_sandbox_port2);
        testutil.destroySandbox(__mysql_sandbox_port3);
        testutil.skip("OpenId Connect is not available in all sandboxes, skipping test");
    }
}
else {
    testutil.destroySandbox(__mysql_sandbox_port1);
    testutil.skip("OpenId Connect is not available, skipping test");
}

function connectToPrimary() {
    shell.connect({
        'host': 'localhost',
        'schema': 'information_schema',
        'port': `${__mysql_sandbox_port1}`,
        'user': `${OPENID_CONNECT_USER}`,
        'authentication-openid-connect-client-id-token-file': `${token_path}`,
    });
}
function connectToSecondary() {
    shell.connect({
        'host': 'localhost',
        'schema': 'information_schema',
        'port': `${__mysql_sandbox_port2}`,
        'user': `${OPENID_CONNECT_USER}`,
        'authentication-openid-connect-client-id-token-file': `${token_path}`,
    });
}
function checkStdOpenIdOutputErrors() {
    EXPECT_OUTPUT_NOT_CONTAINS("The path to ID token file is not set.");
    EXPECT_OUTPUT_NOT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`);
}

//@<> setup openid connect config and accounts
session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

EXPECT_NO_THROWS(function() {
    const sessions = [session1, session2, session3];
    sessions.forEach(session => {
        session.runSql(`SET PERSIST authentication_openid_connect_configuration='file://${cnfg_path}'`);
        session.runSql("SET sql_log_bin = 0");
        session.runSql(`CREATE USER '${OPENID_CONNECT_USER}' IDENTIFIED WITH authentication_openid_connect BY '{"identity_provider":"${OPENID_CONNECT_PROVIDER}","user":"${OPENID_CONNECT_SUBJECT}"}';`);
        session.runSql(`GRANT ALL PRIVILEGES ON *.* TO '${OPENID_CONNECT_USER}' WITH GRANT OPTION;`);
        session.runSql("SET sql_log_bin = 1");
    });
}, "OpenId Connect Setup");

//@<> createCluster
connectToPrimary();

EXPECT_NO_THROWS(function() {
    var cluster = dba.createCluster("clus", { gtidSetIsComplete: 1 });

    cluster.addInstance('localhost:'+__mysql_sandbox_port2,{'recoveryMethod':'clone','memberWeight':80});
    cluster.addInstance('localhost:'+__mysql_sandbox_port3,{'recoveryMethod':'incremental','memberWeight':80});
});

checkStdOpenIdOutputErrors();
session.close();


//@<> getCluster, Connecting to primary instance using openid connect
connectToPrimary();
var result = session.runSql('SELECT current_user()').fetchOne()[0];

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
});

checkStdOpenIdOutputErrors();
EXPECT_CONTAINS(`${OPENID_CONNECT_USER}`, result);
session.close();

//@<> BUG#37069820 - getCluster, Connecting to secondary instance using openid connect
connectToSecondary();

var result = session.runSql('SELECT current_user()').fetchOne()[0];

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
});

checkStdOpenIdOutputErrors();
EXPECT_CONTAINS(`${OPENID_CONNECT_USER}`, result);
session.close();

//@<> remove instance
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.removeInstance('localhost:'+__mysql_sandbox_port3);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> addReplicaInstance
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.addReplicaInstance('localhost:'+__mysql_sandbox_port3);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> addInstance
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.removeInstance('localhost:'+__mysql_sandbox_port3);
    cluster.addInstance('localhost:'+__mysql_sandbox_port3);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> status
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    status = cluster.status();

    EXPECT_FALSE("shellConnectError" in status["defaultReplicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port1}`]);
    EXPECT_FALSE("shellConnectError" in status["defaultReplicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port2}`]);
    EXPECT_FALSE("shellConnectError" in status["defaultReplicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port3}`]);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> rejoinInstance
EXPECT_NO_THROWS(function() {
    connectToPrimary();
    session.runSql("stop group_replication");

    connectToSecondary();
    cluster = dba.getCluster();

    cluster.rejoinInstance('localhost:'+__mysql_sandbox_port1);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> setPrimaryInstance
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.setPrimaryInstance('localhost:'+__mysql_sandbox_port1);
    cluster.setPrimaryInstance('localhost:'+__mysql_sandbox_port2);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> rescan
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();

    // remove an instance from the MD
    session.runSql("delete from mysql_innodb_cluster_metadata.instances where instance_name=?", ["127.0.0.1:" + __mysql_sandbox_port3]);

    cluster.rescan();

    cluster.addInstance('localhost:'+__mysql_sandbox_port3);
});

EXPECT_OUTPUT_CONTAINS(`A new instance '127.0.0.1:${__mysql_sandbox_port3}' was discovered in the cluster.`)
checkStdOpenIdOutputErrors();
session.close();

//@<> describe
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.describe();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> options
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.options();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> setOption
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.setOption("expelTimeout", 5);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> setInstanceOption
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.setInstanceOption('localhost:'+__mysql_sandbox_port1, "memberWeight", 42);
    cluster.options();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> switchToMultiPrimaryMode
connectToSecondary();
EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.switchToMultiPrimaryMode();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> switchToSinglePrimaryMode
connectToSecondary();
EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.switchToSinglePrimaryMode();
    cluster.setPrimaryInstance('localhost:'+__mysql_sandbox_port1);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> execute
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.execute("select 1", "a")
});

checkStdOpenIdOutputErrors();
session.close();

//@<> forceQuorum
connectToSecondary();
cluster = dba.getCluster();

session1.runSql("stop group_replication");
connectToSecondary();
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING)");

testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

cluster = dba.getCluster();
cluster.forceQuorumUsingPartitionOf('localhost:'+__mysql_sandbox_port2);

cluster.rejoinInstance('localhost:'+__mysql_sandbox_port1);

testutil.startSandbox(__mysql_sandbox_port3);

checkStdOpenIdOutputErrors();
session.close();

//@<> rebootClusterFromCompleteOutage
testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2,__mysql_sandbox_port3]);

connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.rebootClusterFromCompleteOutage("clus");
    status = cluster.status();

    EXPECT_FALSE("shellConnectError" in status["defaultReplicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port1}`]);
    EXPECT_FALSE("shellConnectError" in status["defaultReplicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port2}`]);
    EXPECT_FALSE("shellConnectError" in status["defaultReplicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port3}`]);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> listRouters
EXPECT_NO_THROWS(function() {
    connectToPrimary();
    cluster = dba.getCluster();
    cluster.setPrimaryInstance('localhost:'+__mysql_sandbox_port1);

    cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
    session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
    session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

    connectToSecondary();
    cluster = dba.getCluster();
    cluster.listRouters();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> removeRouterMetadata
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.removeRouterMetadata("routerhost1::system");

    cluster.listRouters();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> createCluster(adopt)
connectToPrimary();

EXPECT_NO_THROWS(function() {
    session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
    cluster = dba.createCluster("adopted", {adoptFromGR:true});

    testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

    status = cluster.status();

    EXPECT_FALSE("shellConnectError" in status["defaultReplicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port1}`]);
    EXPECT_FALSE("shellConnectError" in status["defaultReplicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port2}`]);
    EXPECT_FALSE("shellConnectError" in status["defaultReplicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port3}`]);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> dissolve
connectToSecondary();

EXPECT_NO_THROWS(function() {
    cluster = dba.getCluster();
    cluster.dissolve();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> Cleanup
if (debugAuthPlugins()) {
    println(testutil.catFile(testutil.getSandboxLogPath(__mysql_sandbox_port1)));
}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
