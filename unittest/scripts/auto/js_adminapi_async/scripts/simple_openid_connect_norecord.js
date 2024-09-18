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
        'schema': 'mysql',
        'port': `${__mysql_sandbox_port1}`,
        'user': `${OPENID_CONNECT_USER}`,
        'authentication-openid-connect-client-id-token-file': `${token_path}`,
    });
}
function connectToSecondary() {
    shell.connect({
        'host': 'localhost',
        'schema': 'mysql',
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

//@<> createReplicaSet
connectToPrimary();

EXPECT_NO_THROWS(function() {
    rs = dba.createReplicaSet("myrs");
    rs.addInstance('localhost:'+__mysql_sandbox_port2, {recoveryMethod:'incremental'});
});

checkStdOpenIdOutputErrors();
session.close();

//@<> getReplicaSet, Connecting to primary instance using openid connect
connectToSecondary();
var result = session.runSql('SELECT current_user()').fetchOne()[0];

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
});

checkStdOpenIdOutputErrors();
EXPECT_CONTAINS(`${OPENID_CONNECT_USER}`, result);
session.close();

//@<> status
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    status = rs.status();

    EXPECT_FALSE("shellConnectError" in status["replicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port1}`]);
    EXPECT_FALSE("shellConnectError" in status["replicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port2}`]);
});

EXPECT_OUTPUT_NOT_CONTAINS("Could not open connection to '127.0.0.1");
checkStdOpenIdOutputErrors();
session.close();

//@<> addInstance
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.addInstance('localhost:'+__mysql_sandbox_port3, {recoveryMethod:'clone'});
});

checkStdOpenIdOutputErrors();
session.close();

//@<> removeInstance
connectToSecondary();
EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.removeInstance('localhost:'+__mysql_sandbox_port3);
    rs.addInstance('localhost:'+__mysql_sandbox_port3, {recoveryMethod:'incremental'});
});

checkStdOpenIdOutputErrors();
session.close();

//@<> setPrimaryInstance
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.setPrimaryInstance('localhost:'+__mysql_sandbox_port3);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> forcePrimaryInstance (prepare)
testutil.stopSandbox(__mysql_sandbox_port3, {wait:1});

connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    status = rs.status();

    EXPECT_FALSE("shellConnectError" in status["replicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port1}`]);
    EXPECT_FALSE("shellConnectError" in status["replicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port2}`]);
    EXPECT_FALSE("shellConnectError" in status["replicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port3}`]);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> forcePrimaryInstance
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.forcePrimaryInstance('localhost:'+__mysql_sandbox_port1);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> rejoinInstance
testutil.startSandbox(__mysql_sandbox_port3);

connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.rejoinInstance('localhost:'+__mysql_sandbox_port3);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> execute
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.execute("select 1", "a")
});

checkStdOpenIdOutputErrors();
session.close();

//@<> describe
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.describe();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> options
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.options();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> setOption
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.setOption("tag:wide_user_tag", "some_tag");
});

checkStdOpenIdOutputErrors();
session.close();

//@<> setInstanceOption
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.setInstanceOption('localhost:'+__mysql_sandbox_port1, "tag:instance_user_tag", "instance_tag");
    rs.options();
});

checkStdOpenIdOutputErrors();
session.close();


//@<> listRouters
EXPECT_NO_THROWS(function() {
    connectToPrimary();
    cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
    session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
    session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

    connectToSecondary();
    rs = dba.getReplicaSet();
    rs.listRouters();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> removeRouterMetadata
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.removeRouterMetadata("routerhost1::system");

    rs.listRouters();
});

checkStdOpenIdOutputErrors();
session.close();

//@<> createReplicaSet(adopt)
connectToPrimary();
EXPECT_NO_THROWS(function() {
    session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");

    rs = dba.createReplicaSet("adopted", {adoptFromAR:true});

    testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
    testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

    status = rs.status();

    EXPECT_FALSE("shellConnectError" in status["replicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port1}`]);
    EXPECT_FALSE("shellConnectError" in status["replicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port2}`]);
    EXPECT_FALSE("shellConnectError" in status["replicaSet"]["topology"][`127.0.0.1:${__mysql_sandbox_port3}`]);
});

checkStdOpenIdOutputErrors();
session.close();

//@<> dissolve
connectToSecondary();

EXPECT_NO_THROWS(function() {
    rs = dba.getReplicaSet();
    rs.dissolve();
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
