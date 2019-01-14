// Note: Cannot be recorded because the configuration file path is read from
// the server. This means the value recorded is the value from the machine where
// traces are recorded and will fail if you run the test cases in replay mode
// on another machine since the path might differ.
if (real_host_is_loopback) {
    testutil.skip("This test requires that the hostname does not resolve to loopback.");
}

//@ deploy raw_sandbox, fix all issues and then disable log_bin (BUG#27305806) {VER(>=8.0.11)}
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

// Remove 'root'@'%' user to allow configureLocalInstance() to create it.
shell.connect(__sandbox_uri1);
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP USER IF EXISTS 'root'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

testutil.expectPrompt("Please select an option [1]: ", "1");
testutil.expectPrompt("Account Host:", "%");
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance(__sandbox_uri1, {interactive:true, mycnfPath: mycnf_path});
testutil.changeSandboxConf(__mysql_sandbox_port1, "skip_log_bin", "");
testutil.restartSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
EXPECT_EQ('OFF', get_sysvar(session, "log_bin"));

//@ ConfigureLocalInstance should detect path of configuration file by looking at the server (BUG#27305806) {VER(>=8.0.11)}
testutil.expectPrompt("Do you want to modify this file?", "y");
testutil.expectPrompt("Do you want to perform the required configuration changes?", "n");
dba.configureLocalInstance(__sandbox_uri1, {interactive:true});

//@ Cleanup (BUG#27305806) {VER(>=8.0.11)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
