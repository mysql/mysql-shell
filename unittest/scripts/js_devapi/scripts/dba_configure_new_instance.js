// Assumptions: New sandboxes deployed with no server id in the config file.
// Regression for BUG#26818744 : MYSQL SHELL DOESN'T ADD THE SERVER_ID ANYMORE

testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
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
testutil.stopSandbox(__mysql_sandbox_port1, "root");
testutil.stopSandbox(__mysql_sandbox_port2, "root");
testutil.stopSandbox(__mysql_sandbox_port3, "root");

testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);

//@ Configure instance on port 1.
var cnfPath1 = __sandbox_dir + __mysql_sandbox_port1 + "/my.cnf";
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port1, {mycnfPath: cnfPath1, dbPassword:'root'});

//@ Configure instance on port 2.
var cnfPath2 = __sandbox_dir + __mysql_sandbox_port2 + "/my.cnf";
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port2, {mycnfPath: cnfPath2, dbPassword:'root'});

//@ Configure instance on port 3.
var cnfPath3 = __sandbox_dir + __mysql_sandbox_port3 + "/my.cnf";
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port3, {mycnfPath: cnfPath3, dbPassword:'root'});

testutil.stopSandbox(__mysql_sandbox_port1, 'root');
testutil.stopSandbox(__mysql_sandbox_port2, 'root');
testutil.stopSandbox(__mysql_sandbox_port3, 'root');

//@ Restart instance on port 1 to apply new server id settings.
testutil.startSandbox(__mysql_sandbox_port1);

//@ Restart instance on port 2 to apply new server id settings.
testutil.startSandbox(__mysql_sandbox_port2);

//@ Restart instance on port 3 to apply new server id settings.
testutil.startSandbox(__mysql_sandbox_port3);

//@ Connect to instance on port 1.
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Create cluster with success.
if (__have_ssl)
    var cluster = dba.createCluster('bug26818744', {memberSslMode: 'REQUIRED', clearReadOnly: true});
else
    var cluster = dba.createCluster('bug26818744', {memberSslMode: 'DISABLED', clearReadOnly: true});

//@ Add instance on port 2 to cluster with success.
add_instance_to_cluster(cluster, __mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Add instance on port 3 to cluster with success.
add_instance_to_cluster(cluster, __mysql_sandbox_port3);
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
