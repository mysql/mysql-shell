// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

var mysql = require('mysql');

function setupInstance(connection, super_read_only) {
	var tmpSession = mysql.getClassicSession(connection);

	tmpSession.runSql('SET GLOBAL super_read_only = 0');
	tmpSession.runSql('SET sql_log_bin=0');
	tmpSession.runSql('CREATE USER \'root\'@\'%\' IDENTIFIED BY \'root\'');
	tmpSession.runSql('GRANT ALL PRIVILEGES ON *.* TO \'root\'@\'%\' WITH GRANT OPTION');
	tmpSession.runSql('SET sql_log_bin=1');
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
var res = dba.configureLocalInstance(connection1);

//@<OUT> Configures the instance, read only set, no prompt
var res = dba.configureLocalInstance(connection2, {clearReadOnly: true});

//@<OUT> Configures the instance, no prompt
var res = dba.configureLocalInstance(connection3);

//@<OUT> Creates Cluster succeeds, answers 'yes' on read only prompt
shell.connect(connection1);
var cluster = dba.createCluster('sample');

//@<OUT> Adds a read only instance
cluster.addInstance(connection2);
wait_slave_state(cluster, uri2, "ONLINE");

// Wait for the second added instance to fetch all the replication data
wait_sandbox_in_metadata(__mysql_sandbox_port2);

//@<OUT> Adds other instance
cluster.addInstance(connection3);
wait_slave_state(cluster, uri3, "ONLINE");

// Wait for the third added instance to fetch all the replication data
wait_sandbox_in_metadata(__mysql_sandbox_port3);

// Rejoin instance
testutil.stopSandbox(__mysql_sandbox_port3, 'root');
wait_slave_state(cluster, uri3, "(MISSING)");
testutil.startSandbox(__mysql_sandbox_port3);
ensureSuperReadOnly(connection3);
//@<OUT> Rejoins an instance
cluster.rejoinInstance(connection3);

delete cluster;
session.close();

// killSandboxInstance does not wait until the process is actually killed
// before returning, so the function does not fit this use-case.
// OTOH stopSandboxInstance waits until the MySQL classic port is not listening
// anymore, but the x-protocol port may take a bit longer. As so, we must use
// testutil.startSandbox() to make sure the instance is restarted.

//@ Stop sandbox 1
testutil.stopSandbox(__mysql_sandbox_port1, 'root');

//@ Stop sandbox 2
testutil.stopSandbox(__mysql_sandbox_port2, 'root');

//@ Stop sandbox 3
testutil.stopSandbox(__mysql_sandbox_port3, 'root');

//@ Start sandbox 1
testutil.startSandbox(__mysql_sandbox_port1);

//@ Start sandbox 2
testutil.startSandbox(__mysql_sandbox_port2);

//@ Start sandbox 3
testutil.startSandbox(__mysql_sandbox_port3);

//@<OUT> Reboot the cluster
shell.connect(connection1);
session.runSql('set global super_read_only=ON');
var cluster = dba.rebootClusterFromCompleteOutage("sample");
wait_slave_state(cluster, uri2, "ONLINE");
wait_slave_state(cluster, uri3, "ONLINE");

// Close session
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
