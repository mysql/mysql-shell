//@<> deploy the sandbox
var uri1 = localhost + ":" + __mysql_sandbox_port1;
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname}, {createRemoteRoot:false});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf = testutil.getSandboxConfPath(__mysql_sandbox_port1);

// BUG#31491092 reports that when the validate_password plugin is installed, setupAdminAccount() fails
// with an error indicating the password does not match the current policy requirements. This happened
// because the function was creating the account with 2 separate transactions: one to create the user
// without any password, and another to change the password of the created user
session1 = mysql.getSession(__sandbox_uri1);

//@<> Install the validate_password plugin to verify the fix for BUG#31491092
ensure_plugin_enabled("validate_password", session1, "validate_password");
// configure the validate_password plugin for the lowest policy
session1.runSql('SET GLOBAL validate_password_policy=\'LOW\'');
session1.runSql('SET GLOBAL validate_password_length=1');

//@<> With validate_password plugin enabled, an error must be thrown when the password does not satisfy the requirements
EXPECT_THROWS_TYPE(function(){dba.configureInstance(__sandbox_uri1, {mycnfPath:mycnf, clusterAdmin:'admin', clusterAdminPassword:'foo'});}, __endpoint1 + ": Your password does not satisfy the current policy requirements", "MYSQLSH");

//@ configureInstance custom cluster admin and password
var root_uri1 = "root@" + uri1;
testutil.expectPrompt("Please select an option [1]: ", "2");
testutil.expectPrompt("Account Name: ", "repl_admin");

// BUG#29634790 Selecting Option #2 With Dba.configureinstance - No Option To Enter Password
testutil.expectPassword("Password for new account: ", "sample");
testutil.expectPassword("Confirm password: ", "sample");
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]: ", "y");
if (__version_num >= 80011) {
    testutil.expectPrompt("Do you want to restart the instance after configuring it? [y/n]: ", "n");
}

dba.configureInstance(__sandbox_uri1, {interactive: true, mycnfPath: mycnf});
testutil.assertNoPrompts();

// Uninstall the validate_password plugin: negative and positive tests done
ensure_plugin_disabled("validate_password", session1, "validate_password");
session1.close();

//@ test connection with custom cluster admin and password
shell.connect('repl_admin:sample@' + uri1);
session.close();

//@ test configureInstance providing clusterAdminPassword without clusterAdmin
dba.configureInstance(__sandbox_uri1, { interactive: true, clusterAdminPassword: "whatever" });

//@ test configureInstance providing clusterAdminPassword and an existing clusterAdmin
dba.configureInstance(__sandbox_uri1, { interactive: true, clusterAdmin: "repl_admin", clusterAdminPassword: "whatever" });

//@ configureInstance custom cluster admin and no password
var root_uri1 = "root@" + uri1;
testutil.expectPrompt("Please select an option [1]: ", "2");
testutil.expectPrompt("Account Name: ", "repl_admin2");
testutil.expectPassword("Password for new account: ", "");
testutil.expectPassword("Confirm password: ", "");
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]: ", "y");
if (__version_num >= 80011) {
    testutil.expectPrompt("Do you want to restart the instance after configuring it? [y/n]: ", "n");
}

dba.configureInstance(__sandbox_uri1, {interactive:true, mycnfPath: mycnf});

//@ test connection with custom cluster admin and no password
var uri_repl_admin = "repl_admin2:@" + uri1;
shell.connect(uri_repl_admin);
session.close();

//@<> configuring applierWorkerThreads in versions lower that 8.0.23 (should fail) {VER(<8.0.23)}
EXPECT_THROWS(function(){dba.configureInstance(uri_repl_admin, {applierWorkerThreads: 5});}, "Option 'applierWorkerThreads' not supported on target server version: '" + __version + "'");

//@<> Verify that the default value for applierWorkerThreads was set (4) {VER(>=8.0.23)}
EXPECT_EQ(4, get_sysvar(__mysql_sandbox_port1, "slave_parallel_workers"));

//@<> Change the value of applierWorkerThreads {VER(>=8.0.23)}
dba.configureInstance(uri_repl_admin, {applierWorkerThreads: 10, restart: true})
testutil.waitSandboxAlive(__mysql_sandbox_port1);
EXPECT_EQ(10, get_sysvar(__mysql_sandbox_port1, "slave_parallel_workers"));

// Verify that configureInstance() enables parallel-appliers on a cluster member that doesn't have them enabled (upgrade scenario)

//@<> Create a cluster {VER(>=8.0.23)}
shell.connect(uri_repl_admin);
dba.createCluster("test");

//@<> Manually disable some parallel-applier settings {VER(>=8.0.23)}
session.runSql("RESET PERSIST slave_preserve_commit_order");
session.runSql("RESET PERSIST slave_parallel_workers");
session.runSql("SET global slave_preserve_commit_order=OFF");
session.runSql("SET global slave_parallel_workers=0");

//@<OUT> Verify that configureInstance() detects and fixes the wrong settings {VER(>=8.0.23)}
dba.configureInstance();
testutil.waitSandboxAlive(__mysql_sandbox_port1);

//@<> Verify that the default value for applierWorkerThreads was set and the wrong config fixed {VER(>=8.0.23)}
EXPECT_EQ(4, get_sysvar(__mysql_sandbox_port1, "slave_parallel_workers"));
EXPECT_EQ(1, get_sysvar(__mysql_sandbox_port1, "slave_preserve_commit_order"));

//@<> cleanup
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Initialization canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@<OUT> canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
dba.configureInstance(__sandbox_uri1);

//@<> Cleanup canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Initialization canonical IPv4 addresses are supported WL#12758
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "127.0.0.1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@<OUT> canonical IPv4 addresses are supported WL#12758
dba.configureInstance(__sandbox_uri1);

//@<> Cleanup canonical IPv4 addresses are supported WL#12758
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Initialization IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@ IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
dba.configureInstance(__sandbox_uri1);

//@<> Cleanup IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.destroySandbox(__mysql_sandbox_port1);
