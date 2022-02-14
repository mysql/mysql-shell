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

//@<> BUG#30339460: Initialization.
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname}, {createRemoteRoot:false});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);

//@ BUG#30339460: Use configureInstance to create the Admin user.
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]: ", "y");
if (__version_num >= 80011) {
    testutil.expectPrompt("Do you want to restart the instance after configuring it? [y/n]: ", "n");
}
dba.configureInstance(__sandbox_uri1, { interactive: true, clusterAdmin: "admin_user", clusterAdminPassword: "admin_pwd", mycnfPath: mycnf_path });

//@<> BUG#30339460: Revert some previous configureInstance() change to require some settings to be fixed.
shell.connect(__sandbox_uri1);
if (__version_num >= 80011) {
    session.runSql("RESET PERSIST");
}
//session.runSql("SET GLOBAL binlog_checksum = 'CRC32'");
//session.runSql("SET GLOBAL enforce_gtid_consistency = 'OFF'");
//session.runSql("SET GLOBAL gtid_mode = 'OFF'");
session.runSql("SET GLOBAL binlog_checksum = DEFAULT");
session.runSql("SET GLOBAL enforce_gtid_consistency = DEFAULT");
session.runSql("SET GLOBAL gtid_mode = DEFAULT");
session.runSql("SET GLOBAL server_id = DEFAULT");

//@ BUG#30339460: Use configureInstance with the Admin user (no error).
var admin_uri = "admin_user:admin_pwd@" + hostname + ":" + __mysql_sandbox_port1;
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]: ", "y");
if (__version_num >= 80011) {
    testutil.expectPrompt("Do you want to restart the instance after configuring it? [y/n]: ", "n");
}
dba.configureInstance(admin_uri, { interactive: true, mycnfPath: mycnf_path});

//@<> BUG#30339460: clean-up.
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> dba.configureInstance does not error if using a single clusterAdmin account with netmask BUG#31018091
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);
var ip_mask = hostname_ip.split(".").splice(0,3).join(".") + ".0";
dba.configureInstance(__sandbox_uri1, {clusterAdmin: "'admin'@'"+ ip_mask + "/255.255.255.0'", clusterAdminPassword: "pwd"});
var cluster_admin_uri= "mysql://admin:pwd@" + hostname_ip + ":" + __mysql_sandbox_port1;
shell.connect(cluster_admin_uri);
c = dba.configureInstance();
EXPECT_STDERR_EMPTY();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> need to have a raw sandbox
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

//@<> dba.configureInstance does not require binlog_checksum to be NONE BUG#31329024 {VER(>= 8.0.21)}
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_checksum", "CRC32");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
c = dba.configureInstance(__sandbox_uri1);
// does not contain output about binlog_checksum on the configuration table
EXPECT_STDOUT_NOT_CONTAINS("| binlog_checksum | CRC32         | NONE           | Update the server variable and the config file |");
session.close();

//@<> dba.configureInstance (and also dba.checkInstanceConfiguration) cannot print "log_bin" twice BUG#31964706 {VER(< 8.0.3)}
shell.options.useWizards=0;

testutil.removeFromSandboxConf(__mysql_sandbox_port1, "log_bin");
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "disable_log_bin");
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "skip_log_bin");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port1);

var mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var mycnf_path_out = "mytmp.cnf";
try {testutil.rmfile(mycnf_path_out);} catch (err) {}

dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath:mycnf_path});
var count = testutil.fetchCapturedStdout(false).match(/\| log_bin/g).length;
EXPECT_EQ(count, 1, "Only a single line with 'log_bin' is expected");
EXPECT_STDOUT_CONTAINS("| log_bin                          | <not set>     | <no value>     | Update the config file and restart the server  |");
EXPECT_OUTPUT_NOT_CONTAINS("| disable_log_bin ");
EXPECT_OUTPUT_NOT_CONTAINS("| skip_log_bin ");

testutil.wipeAllOutput();
dba.configureInstance(__sandbox_uri1, {mycnfPath:mycnf_path, outputMycnfPath:mycnf_path_out});
var count = testutil.fetchCapturedStdout(false).match(/\| log_bin/g).length;
EXPECT_EQ(count, 1, "Only a single line with 'log_bin' is expected");
EXPECT_STDOUT_CONTAINS("| log_bin                          | <not set>     | <no value>     | Update the config file and restart the server  |");
EXPECT_OUTPUT_NOT_CONTAINS("| disable_log_bin ");
EXPECT_OUTPUT_NOT_CONTAINS("| skip_log_bin ");

//@<> dba.configureInstance (and also dba.checkInstanceConfiguration) cannot print "log_bin" twice and must show "skip_log_bin" BUG#31964706 {VER(< 8.0.3)}
testutil.changeSandboxConf(__mysql_sandbox_port1, "skip_log_bin", "ON");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port1);

dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath:mycnf_path});
var count = testutil.fetchCapturedStdout(false).match(/\| log_bin/g).length;
EXPECT_EQ(count, 1, "Only a single line with 'log_bin' is expected");
EXPECT_STDOUT_CONTAINS("| log_bin                          | <not set>     | <no value>     | Update the config file and restart the server  |");
EXPECT_STDOUT_CONTAINS("| skip_log_bin                     | ON            | <not set>      | Update the config file and restart the server  |");

testutil.wipeAllOutput();
dba.configureInstance(__sandbox_uri1, {mycnfPath:mycnf_path, outputMycnfPath:mycnf_path_out});
var count = testutil.fetchCapturedStdout(false).match(/\| log_bin/g).length;
EXPECT_EQ(count, 1, "Only a single line with 'log_bin' is expected");
EXPECT_STDOUT_CONTAINS("| log_bin                          | <not set>     | <no value>     | Update the config file and restart the server  |");
EXPECT_STDOUT_CONTAINS("| skip_log_bin                     | ON            | <not set>      | Update the config file and restart the server  |");

//@<> dba.configureInstance (and also dba.checkInstanceConfiguration) cannot print "log_bin" twice and must show "skip_log_bin" and "disable_log_bin" - no config BUG#31964706 {VER(< 8.0.3)}
dba.checkInstanceConfiguration(__sandbox_uri1);
var count = testutil.fetchCapturedStdout(false).match(/\| log_bin/g).length;
EXPECT_EQ(count, 1, "Only a single line with 'log_bin' is expected");
EXPECT_STDOUT_CONTAINS("| log_bin                          | <not set>     | <no value>     | Update the config file and restart the server |");
EXPECT_STDOUT_CONTAINS("| disable_log_bin                  | <not set>     | <not set>      | Update the config file and restart the server |");
EXPECT_STDOUT_CONTAINS("| skip_log_bin                     | <not set>     | <not set>      | Update the config file and restart the server |");

testutil.wipeAllOutput();
EXPECT_THROWS(function () { dba.configureInstance(__sandbox_uri1) }, "Dba.configureInstance: Unable to update configuration");
var count = testutil.fetchCapturedStdout(false).match(/\| log_bin/g).length;
EXPECT_EQ(count, 1, "Only a single line with 'log_bin' is expected");
EXPECT_STDOUT_CONTAINS("| log_bin                          | <not set>     | <no value>     | Update the config file and restart the server |");
EXPECT_STDOUT_CONTAINS("| disable_log_bin                  | <not set>     | <not set>      | Update the config file and restart the server |");
EXPECT_STDOUT_CONTAINS("| skip_log_bin                     | <not set>     | <not set>      | Update the config file and restart the server |");

//@<> check if dba.configureInstance is successful BUG#31964706 {VER(< 8.0.3)}
EXPECT_NO_THROWS(function () { dba.configureInstance(__sandbox_uri1, {mycnfPath:mycnf_path}) });
EXPECT_OUTPUT_CONTAINS("The instance '" + hostname + ":" + __mysql_sandbox_port1 + "' was configured to be used in an InnoDB cluster.");

testutil.rmfile(mycnf_path_out);
shell.options.useWizards=1;

//@<> dba.configureInstance (and also dba.checkInstanceConfiguration) cannot show "log_bin", "skip_log_bin" or "disable_log_bin" BUG#31964706 {VER(>= 8.0.3)}
shell.options.useWizards=0;

testutil.removeFromSandboxConf(__mysql_sandbox_port1, "log_bin");
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "disable_log_bin");
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "skip_log_bin");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port1);

dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port1)});
EXPECT_OUTPUT_NOT_CONTAINS("| log_bin ");
EXPECT_OUTPUT_NOT_CONTAINS("| disable_log_bin ");
EXPECT_OUTPUT_NOT_CONTAINS("| skip_log_bin ");

dba.configureInstance(__sandbox_uri1, {mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port1)});
EXPECT_OUTPUT_NOT_CONTAINS("| log_bin ");
EXPECT_OUTPUT_NOT_CONTAINS("| disable_log_bin ");
EXPECT_OUTPUT_NOT_CONTAINS("| skip_log_bin ");

//@<> dba.configureInstance (and also dba.checkInstanceConfiguration) must output "skip_log_bin" only BUG#31964706 {VER(>= 8.0.3)}
testutil.changeSandboxConf(__mysql_sandbox_port1, "skip_log_bin", "ON");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port1);

dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port1)});
EXPECT_OUTPUT_NOT_CONTAINS("| log_bin ");
EXPECT_OUTPUT_NOT_CONTAINS("| disable_log_bin ");
EXPECT_OUTPUT_CONTAINS("| skip_log_bin        | ON            | <not set>      | Update the config file and restart the server    |");

dba.configureInstance(__sandbox_uri1, {mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port1)});
EXPECT_OUTPUT_NOT_CONTAINS("| log_bin ");
EXPECT_OUTPUT_NOT_CONTAINS("| disable_log_bin ");
EXPECT_OUTPUT_CONTAINS("| skip_log_bin        | ON            | <not set>      | Update the config file and restart the server    |");

//@<> dba.configureInstance (and also dba.checkInstanceConfiguration) must output "skip_log_bin" and "disable_log_bin" - no config BUG#31964706 {VER(>= 8.0.3)}
dba.checkInstanceConfiguration(__sandbox_uri1);
EXPECT_OUTPUT_NOT_CONTAINS("| log_bin ");
EXPECT_OUTPUT_CONTAINS("| disable_log_bin     | <not set>     | <not set>      | Update the config file and restart the server |");
EXPECT_OUTPUT_CONTAINS("| skip_log_bin        | <not set>     | <not set>      | Update the config file and restart the server |");

EXPECT_THROWS(function () { dba.configureInstance(__sandbox_uri1) }, "Dba.configureInstance: Unable to update configuration");
EXPECT_OUTPUT_NOT_CONTAINS("| log_bin ");
EXPECT_OUTPUT_CONTAINS("| disable_log_bin     | <not set>     | <not set>      | Update the config file and restart the server |");
EXPECT_OUTPUT_CONTAINS("| skip_log_bin        | <not set>     | <not set>      | Update the config file and restart the server |");

//@<> dba.configureInstance must output "log_bin" for restart only {VER(>= 8.0.3)}
EXPECT_NO_THROWS(function () { dba.configureInstance(__sandbox_uri1, {mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port1)}) });
var count = testutil.fetchCapturedStdout(false).match(/\| log_bin/g).length;
EXPECT_EQ(count, 1, "Only a single line with 'log_bin' is expected");
EXPECT_OUTPUT_CONTAINS("| log_bin             | OFF           | ON             | Restart the server |");
EXPECT_OUTPUT_CONTAINS("The instance '" + hostname + ":" + __mysql_sandbox_port1 + "' was configured to be used in an InnoDB cluster.");

shell.options.useWizards=1;

//@<> improve restart message NONE BUG#29634795 {!__dbug_off && VER(>=8.0.11)}
testutil.changeSandboxConf(__mysql_sandbox_port1, "skip_log_bin", "ON");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port1);

testutil.dbugSet("+d,dba_abort_instance_restart");

testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]:", "y");
EXPECT_THROWS(function () { dba.configureInstance(__sandbox_uri1, {restart:true, mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port1)}) }, "Dba.configureInstance: restart aborted (debug)");
EXPECT_OUTPUT_CONTAINS("Please restart MySQL manually (check https://dev.mysql.com/doc/refman/en/restart.html for more details).");

testutil.dbugSet("");

//@<> cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
