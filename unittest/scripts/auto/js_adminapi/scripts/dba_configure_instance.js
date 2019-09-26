//@<> deploy the sandbox
var uri1 = localhost + ":" + __mysql_sandbox_port1;
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname}, {createRemoteRoot:false});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf = testutil.getSandboxConfPath(__mysql_sandbox_port1);

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
shell.connect('repl_admin2:@' + uri1);
session.close();

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
