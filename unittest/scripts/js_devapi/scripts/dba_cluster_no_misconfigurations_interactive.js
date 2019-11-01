// Assumptions: smart deployment routines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@ connect
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> create cluster no misconfiguration: ok
var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});

session.close();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
