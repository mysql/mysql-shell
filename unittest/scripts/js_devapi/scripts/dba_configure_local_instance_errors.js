
testutil.deploySandbox(__mysql_sandbox_port1, 'root');
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
