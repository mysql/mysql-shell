// Deploy instances (with disabled storage engines).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"disabled_storage_engines": "MyISAM,BLACKHOLE,ARCHIVE", report_host:hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
//@<OUT> checkInstanceConfiguration with disabled_storage_engines no error.
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});

//@<OUT> configureInstance disabled_storage_engines no error.
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf1});

//@ Remove disabled_storage_engines option from configuration and restart.
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "disabled_storage_engines");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port1);

//@ Restart sandbox.
testutil.restartSandbox(__mysql_sandbox_port1);

//@<OUT> configureInstance still no issues after restart.
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf1});

// Clean-up deployed instances.
testutil.destroySandbox(__mysql_sandbox_port1);
