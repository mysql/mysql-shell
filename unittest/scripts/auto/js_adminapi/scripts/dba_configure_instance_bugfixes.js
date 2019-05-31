// Tests for configure instance (and check instance) bugs

//@ BUG#28727505: Initialization.
// Deploy sandbox with GTID_MODE disabled on option file.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"gtid_mode": "OFF", report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);

// Enable GTID_MODE on option file after starting sandbox.
testutil.changeSandboxConf(__mysql_sandbox_port1, "gtid_mode", "ON");

//@<> BUG#28727505: configure instance 5.7. {VER(<8.0.11)}
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path});

//@<> BUG#28727505: configure instance again 5.7. {VER(<8.0.11)}
// Report "Update read-only variable and restart the server" for 5.7,
// because we cannot ensure that the provided option file is the one really used
// by the server.
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path});

//@<> BUG#28727505: configure instance. {VER(>=8.0.11)}
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path});

//@<> BUG#28727505: configure instance again. {VER(>=8.0.11)}
// Report "Restart the server" for 8.0 supporting SET PERSIST,
// because persisted variables have precedence over option files and we can
// ensure that the server settings was changed and only a restart is needed.
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path});

//@ BUG#28727505: clean-up.
testutil.destroySandbox(__mysql_sandbox_port1);

//@ BUG#29765093: Initialization. {VER(>=8.0.11)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);

//@ BUG#29765093: Change some persisted (only) values. {VER(>=8.0.11)}
shell.connect(__sandbox_uri1);
session.runSql("SET @@PERSIST_ONLY.binlog_format=statement");
session.runSql("SET @@GLOBAL.binlog_format=mixed");
session.runSql("SET @@PERSIST_ONLY.gtid_mode=off");

//@<> BUG#29765093: check instance configuration instance. {VER(>=8.0.11)}
dba.checkInstanceConfiguration(__sandbox_uri1);

//@<> BUG#29765093: configure instance. {VER(>=8.0.11)}
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path});

//@<> BUG#29765093: configure instance again. {VER(>=8.0.11)}
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path});

//@ BUG#29765093: clean-up. {VER(>=8.0.11)}
testutil.destroySandbox(__mysql_sandbox_port1);
