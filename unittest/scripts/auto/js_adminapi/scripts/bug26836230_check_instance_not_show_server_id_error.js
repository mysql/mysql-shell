// Deploy instances (with invalid server_id).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"server_id": "0", "report_host": hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname});
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "server_id");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port2);

var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);

//@<OUT> checkInstanceConfiguration with server_id error.
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});

//@<OUT> configureLocalInstance server_id updated but needs restart.
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

//@<OUT> configureLocalInstance still indicate that a restart is needed.
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

//@ Restart sandbox 1.
testutil.restartSandbox(__mysql_sandbox_port1);

//@<OUT> configureLocalInstance no issues after restart for sandobox 1.
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

// Regression tests with instance with no server_id in the option file
//@<OUT> checkInstanceConfiguration no server_id in my.cnf (error). {VER(>=8.0.3)}
dba.checkInstanceConfiguration(__sandbox_uri2, {mycnfPath: mycnf2});

//@<OUT> configureLocalInstance no server_id in my.cnf (needs restart). {VER(>=8.0.3)}
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: mycnf2});

//@<OUT> configureLocalInstance no server_id in my.cnf (still needs restart). {VER(>=8.0.3)}
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: mycnf2});

//@ Restart sandbox 2. {VER(>=8.0.3)}
testutil.restartSandbox(__mysql_sandbox_port2);

//@<OUT> configureLocalInstance no issues after restart for sandbox 2. {VER(>=8.0.3)}
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: mycnf2});

// Clean-up deployed instances.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
