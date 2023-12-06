//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.options.useWizards=true;

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

//@<> Configures the instance, no read only prompts (automatically cleared)
//NOTE: super-read-only is only required to be disable if we create the clusterAdmin user
EXPECT_NO_THROWS(function() { dba.configureInstance(connection1, {clusterAdmin: "testUser", clusterAdminPassword: "testUserPwd"}); });

EXPECT_OUTPUT_CONTAINS(`Configuring local MySQL instance listening at port ${__mysql_sandbox_port1} for use in an InnoDB Cluster...`);
EXPECT_OUTPUT_CONTAINS(`NOTE: Instance detected as a sandbox.`);
EXPECT_OUTPUT_CONTAINS(`Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.`);
EXPECT_OUTPUT_CONTAINS(`This instance reports its own address as ${hostname}:${__mysql_sandbox_port1}`);
EXPECT_OUTPUT_CONTAINS(`Assuming full account name 'testUser'@'%' for testUser`);
if (__version_num >= 80023)
    EXPECT_OUTPUT_CONTAINS(`applierWorkerThreads will be set to the default value of 4.`);
EXPECT_OUTPUT_CONTAINS(`Disabled super_read_only on the instance '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`Creating user testUser@%.`);
EXPECT_OUTPUT_CONTAINS(`Account testUser@% was successfully created.`);
EXPECT_OUTPUT_CONTAINS(`Enabling super_read_only on the instance '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port1}' is valid for InnoDB Cluster usage.`);


//@<> Configures the instance, read only set, no prompt
EXPECT_NO_THROWS(function() { dba.configureInstance(connection2, {clusterAdmin: "testUser", clusterAdminPassword: "testUserPwd", clearReadOnly: true}); });

EXPECT_OUTPUT_CONTAINS(`Configuring local MySQL instance listening at port ${__mysql_sandbox_port2} for use in an InnoDB Cluster...`);
EXPECT_OUTPUT_CONTAINS(`NOTE: Instance detected as a sandbox.`);
EXPECT_OUTPUT_CONTAINS(`Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.`);
EXPECT_OUTPUT_CONTAINS(`This instance reports its own address as ${hostname}:${__mysql_sandbox_port2}`);
EXPECT_OUTPUT_CONTAINS(`Assuming full account name 'testUser'@'%' for testUser`);
if (__version_num >= 80023)
    EXPECT_OUTPUT_CONTAINS(`applierWorkerThreads will be set to the default value of 4.`);
EXPECT_OUTPUT_CONTAINS(`Disabled super_read_only on the instance '${hostname}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`Creating user testUser@%.`);
EXPECT_OUTPUT_CONTAINS(`Account testUser@% was successfully created.`);
EXPECT_OUTPUT_CONTAINS(`Enabling super_read_only on the instance '${hostname}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' is valid for InnoDB Cluster usage.`);

//@<> Configures the instance, no prompt
EXPECT_NO_THROWS(function() { dba.configureInstance(connection3, {clusterAdmin: "testUser", clusterAdminPassword: "testUserPwd"}); });

EXPECT_OUTPUT_CONTAINS(`Configuring local MySQL instance listening at port ${__mysql_sandbox_port3} for use in an InnoDB Cluster...`);
EXPECT_OUTPUT_CONTAINS(`NOTE: Instance detected as a sandbox.`);
EXPECT_OUTPUT_CONTAINS(`Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.`);
EXPECT_OUTPUT_CONTAINS(`This instance reports its own address as ${hostname}:${__mysql_sandbox_port3}`);
EXPECT_OUTPUT_CONTAINS(`Assuming full account name 'testUser'@'%' for testUser`);
if (__version_num >= 80023)
    EXPECT_OUTPUT_CONTAINS(`applierWorkerThreads will be set to the default value of 4.`);
EXPECT_OUTPUT_NOT_CONTAINS(`Disabled super_read_only on the instance '${hostname}:${__mysql_sandbox_port3}'`);
EXPECT_OUTPUT_CONTAINS(`Creating user testUser@%.`);
EXPECT_OUTPUT_CONTAINS(`Account testUser@% was successfully created.`);
EXPECT_OUTPUT_NOT_CONTAINS(`Enabling super_read_only on the instance '${hostname}:${__mysql_sandbox_port3}'`);
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port3}' is valid for InnoDB Cluster usage.`);


//@<> Connect
shell.connect(connection1);

//@<> Creates Cluster succeeds (should auto-clear)
var cluster;

EXPECT_NO_THROWS(function() { cluster = dba.createCluster('sample', {gtidSetIsComplete: true}); });

EXPECT_OUTPUT_CONTAINS(`Disabling super_read_only mode on instance '${hostname}:${__mysql_sandbox_port1}'`);

//@<> Adds a read only instance
ensureSuperReadOnly(connection2);
EXPECT_NO_THROWS(function() { cluster.addInstance(connection2); });

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@<> Adds other instance
EXPECT_NO_THROWS(function() {  cluster.addInstance(connection3); });

// Wait for the third added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port3);

// Rejoin instance
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
testutil.startSandbox(__mysql_sandbox_port3);
ensureSuperReadOnly(connection3);
//@<> Rejoins an instance
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(connection3); });

cluster.disconnect();
session.close();

//@<> persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
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

//@<> Reset gr_start_on_boot on all instances
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

//@<> Reboot the cluster - no read only prompts (automatically cleared)
shell.connect(connection1);
session.runSql('set global super_read_only=ON');
EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("sample"); });
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

EXPECT_OUTPUT_CONTAINS(`Restoring the Cluster 'sample' from complete outage...`);
EXPECT_OUTPUT_CONTAINS(`The Cluster was successfully rebooted.`);

//@<> Cleanup
session.close();
cluster.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
