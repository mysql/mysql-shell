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
if (__version_num < 80011) {
    testutil.expectPrompt("Please specify the path to the MySQL configuration file: ", mycnf);
}
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]: ", "y");
if (__version_num >= 80011) {
    testutil.expectPrompt("Do you want to restart the instance after configuring it? [y/n]: ", "n");
}

dba.configureInstance(__sandbox_uri1, { interactive: true });
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
if (__version_num < 80011) {
    testutil.expectPrompt("Please specify the path to the MySQL configuration file: ", mycnf);
}
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]: ", "y");
if (__version_num >= 80011) {
    testutil.expectPrompt("Do you want to restart the instance after configuring it? [y/n]: ", "n");
}

dba.configureInstance(__sandbox_uri1, {interactive:true});

//@ test connection with custom cluster admin and no password
shell.connect('repl_admin2:@' + uri1);
session.close();

//@<> cleanup
testutil.destroySandbox(__mysql_sandbox_port1);