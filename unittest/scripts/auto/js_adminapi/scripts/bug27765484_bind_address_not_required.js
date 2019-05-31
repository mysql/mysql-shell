// Deploy instance (with bind_address set).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"bind_address": "0.0.0.0", report_host:hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
//@<OUT> checkInstanceConfiguration with bind_address set no error
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});

// connect to the instance
shell.connect(__sandbox_uri1);

//@<OUT> createCluster from instance with bind_address set
dba.createCluster("testCluster", {gtidSetIsComplete: true});

// Clean-up instance
// Need to shutdown first, because destroySandbox() will not do anything if the port is not listening
// Since this is listening on 0.0.0.0, the check won't work.
session.runSql("shutdown");
testutil.destroySandbox(__mysql_sandbox_port1);
