
testutil.deploySandbox(__mysql_sandbox_port1, 'root');
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

// Create cluster
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

var cluster = dba.createCluster('Cluster');

testutil.makeFileReadOnly(testutil.getSandboxConfPath(__mysql_sandbox_port1));

//@<OUT> Error no write privileges
var cnfPath = testutil.getSandboxConfPath(__mysql_sandbox_port1).split("\\").join("\\\\");
var __sandbox1_conf_path = cnfPath;
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1, {mycnfPath:cnfPath, password:'root'});

// Close session
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
