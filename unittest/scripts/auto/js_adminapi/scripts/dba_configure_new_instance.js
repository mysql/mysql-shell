// Assumptions: New sandboxes deployed with no server id in the config file.
// Regression for BUG#26818744 : MYSQL SHELL DOESN'T ADD THE SERVER_ID ANYMORE

var __sandbox_dir = testutil.getSandboxPath();
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

// Remove the server id information from the configuration files.
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "server_id");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "server_id");
testutil.removeFromSandboxConf(__mysql_sandbox_port3, "server_id");

// In MySQL 5.7, a server ID had to be specified when binary logging was enabled, or the server would not start. {VER(<8.0)}
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "log_bin");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "log_bin");
testutil.removeFromSandboxConf(__mysql_sandbox_port3, "log_bin");

// Restart sandbox instances.
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.stopSandbox(__mysql_sandbox_port3);

testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);

//@ Configure instance on port 1.
//TODO: The test should check for BUG#26836230
var cnfPath1 = os.path.join(__sandbox_dir, __mysql_sandbox_port1.toString(), "my.cnf");
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port1, {mycnfPath: cnfPath1, password:'root'});

//@ Configure instance on port 2.
//TODO: The test should check for BUG#26836230
var cnfPath2 = os.path.join(__sandbox_dir, __mysql_sandbox_port2.toString(), "my.cnf");
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port2, {mycnfPath: cnfPath2, password:'root'});

//@ Configure instance on port 3.
//TODO: The test should check for BUG#26836230
var cnfPath3 = os.path.join(__sandbox_dir, __mysql_sandbox_port3.toString(), "my.cnf");
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port3, {mycnfPath: cnfPath3, password:'root'});

testutil.stopSandbox(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.stopSandbox(__mysql_sandbox_port3);

//@ Restart instance on port 1 to apply new server id settings.
testutil.startSandbox(__mysql_sandbox_port1);

//@ Restart instance on port 2 to apply new server id settings.
testutil.startSandbox(__mysql_sandbox_port2);

//@ Restart instance on port 3 to apply new server id settings.
testutil.startSandbox(__mysql_sandbox_port3);

//@ Connect to instance on port 1.
shell.connect(__sandbox_uri1);

//@ Create cluster with success.
var cluster = dba.createCluster('bug26818744', {memberSslMode: 'REQUIRED', clearReadOnly: true, gtidSetIsComplete: true});

//@ Add instance on port 2 to cluster with success.
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Add instance on port 3 to cluster with success.
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Remove instance on port 2 from cluster with success.
cluster.removeInstance('root:root@localhost:' + __mysql_sandbox_port2);

//@ Remove instance on port 3 from cluster with success.
cluster.removeInstance('root:root@localhost:' + __mysql_sandbox_port3);

//@ Dissolve cluster with success
cluster.dissolve({force: true});

cluster.disconnect();
// Close session
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
