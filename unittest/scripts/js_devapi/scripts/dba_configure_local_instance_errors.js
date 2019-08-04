
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@ ConfigureLocalInstance should fail if there's no session nor parameters provided
dba.configureLocalInstance();

// Create cluster
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

var cluster = dba.createCluster('Cluster');

testutil.makeFileReadOnly(testutil.getSandboxConfPath(__mysql_sandbox_port1));

//@# Error no write privileges {VER(<8.0.11)}
var cnfPath = testutil.getSandboxConfPath(__mysql_sandbox_port1).split("\\").join("\\\\");
var __sandbox1_conf_path = cnfPath;
// This call is for persisting stuff like group_seeds, not configuring the instance
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1, {mycnfPath:cnfPath, password:'root'});

//@ Close session
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Initialization IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@ IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
dba.configureLocalInstance(__sandbox_uri1);

//@<> Cleanup IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.destroySandbox(__mysql_sandbox_port1);
