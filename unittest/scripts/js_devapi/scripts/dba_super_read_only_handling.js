// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

var mysql = require('mysql');

function setupInstance(connection, super_read_only) {
    var tmpSession = mysql.getClassicSession(connection);

    tmpSession.runSql('SET GLOBAL super_read_only = 0');
    if (super_read_only) {
        tmpSession.runSql('set global super_read_only=ON');
    }

    tmpSession.close();
}

function ensureSuperReadOnly(connection) {
    var tmpSession = mysql.getClassicSession(connection);
    tmpSession.runSql('set global super_read_only=ON');
    tmpSession.close();
}

var connection1 = {scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'};
var connection2 = {scheme: 'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'};
var connection3 = {scheme: 'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'};

setupInstance(connection1, true);
setupInstance(connection2, true);
setupInstance(connection3, false);

//@<OUT> Configures the instance, answers 'yes' on the read only prompt
//NOTE: super-read-only is only required to be disable if we create the clusterAdmin user
dba.configureInstance(connection1, {clusterAdmin: "testUser", clusterAdminPassword: "testUserPwd"});

//@<OUT> Configures the instance, read only set, no prompt
dba.configureInstance(connection2, {clusterAdmin: "testUser", clusterAdminPassword: "testUserPwd", clearReadOnly: true});

//@<OUT> Configures the instance, no prompt
dba.configureInstance(connection3, {clusterAdmin: "testUser", clusterAdminPassword: "testUserPwd"});


//@<> Connect
shell.connect(connection1);

//@ Creates Cluster succeeds (should auto-clear)
var cluster = dba.createCluster('sample');

//@ Adds a read only instance
ensureSuperReadOnly(connection2);
cluster.addInstance(connection2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Adds other instance
cluster.addInstance(connection3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

// Rejoin instance
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
testutil.startSandbox(__mysql_sandbox_port3);
ensureSuperReadOnly(connection3);
//@ Rejoins an instance
cluster.rejoinInstance(connection3);

cluster.disconnect();
session.close();

//@ persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
var mycnf3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port2, {mycnfPath: mycnf2});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port3, {mycnfPath: mycnf3});


// killSandboxInstance does not wait until the process is actually killed
// before returning, so the function does not fit this use-case.
// OTOH stopSandboxInstance waits until the MySQL classic port is not listening
// anymore, but the x-protocol port may take a bit longer. As so, we must use
// testutil.startSandbox() to make sure the instance is restarted.

//@ Reset gr_start_on_boot on all instances
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

shell.connect(connection1);

testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
session.close();

testutil.stopSandbox(__mysql_sandbox_port1);

testutil.startSandbox(__mysql_sandbox_port1);

testutil.startSandbox(__mysql_sandbox_port2);

testutil.startSandbox(__mysql_sandbox_port3);

//@<OUT> Reboot the cluster
shell.connect(connection1);
session.runSql('set global super_read_only=ON');
var cluster = dba.rebootClusterFromCompleteOutage("sample");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Cleanup
session.close();
cluster.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
