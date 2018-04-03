// Deploy instance (with bind_address set).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"bind_address": "0.0.0.0", report_host:hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
//@<OUT> checkInstanceConfiguration with bind_address set no error
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});

// connect to the instance
shell.connect(__sandbox_uri1);

//@<OUT> createCluster from instance with bind_address set
dba.createCluster("testCluster");

// Clean-up instance
testutil.destroySandbox(__mysql_sandbox_port1);
