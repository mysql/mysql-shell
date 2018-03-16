// Deploy instances (with not supported storage engines).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"disabled_storage_engines": "MyISAM,BLACKHOLE,ARCHIVE", report_host:hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
//@<OUT> checkInstanceConfiguration with disabled_storage_engines error.
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});

//@<OUT> configureLocalInstance disabled_storage_engines updated but needs restart.
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

//@<OUT> configureLocalInstance still indicate that a restart is needed.
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

//@ Restart sandbox.
testutil.restartSandbox(__mysql_sandbox_port1);

//@<OUT> configureLocalInstance no issues after restart.
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

//@ Remove disabled_storage_engines option from configuration and restart.
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "disabled_storage_engines");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port1);

//@<OUT> checkInstanceConfiguration no disabled_storage_engines in my.cnf (error).
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});

//@<OUT> configureLocalInstance no disabled_storage_engines in my.cnf (needs restart).
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

//@<OUT> configureLocalInstance no disabled_storage_engines in my.cnf (still needs restart).
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

//@ Restart sandbox again.
testutil.restartSandbox(__mysql_sandbox_port1);

//@<OUT> configureLocalInstance no issues again after restart.
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

// Clean-up deployed instances.
testutil.destroySandbox(__mysql_sandbox_port1);
