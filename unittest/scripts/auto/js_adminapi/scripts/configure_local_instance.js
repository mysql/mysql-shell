//@ deploy the sandbox
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname}, {createRemoteRoot:false});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

var mycnf = testutil.getSandboxConfPath(__mysql_sandbox_port1);

//@ ConfigureLocalInstance should fail if there's no session nor parameters provided
dba.configureLocalInstance();

//@<OUT> Interactive_dba_configure_local_instance read_only_no_prompts
dba.configureLocalInstance(__sandbox_uri1, {interactive: false, mycnfPath:mycnf, clusterAdmin:'root', clusterAdminPassword:'root'});

shell.connect(__sandbox_uri1);
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ(1, get_sysvar(session, "super_read_only"));

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes 8.0 {VER(>=8.0.11)}
set_sysvar(session, "super_read_only", 0);
session.runSql("RESET PERSIST enforce_gtid_consistency");
session.runSql("RESET PERSIST gtid_mode");
session.runSql("RESET PERSIST server_id");
set_sysvar(session, "super_read_only", 1);
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
testutil.expectPrompt("Do you want to disable super_read_only and continue?", "y");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root2', clusterAdminPassword:'root'});

set_sysvar(session, "super_read_only", 1);
EXPECT_EQ(1, get_sysvar(session, "super_read_only"));

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes 5.7 {VER(<8.0.11)}
testutil.expectPrompt("Do you want to disable super_read_only and continue?", "y");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root2', clusterAdminPassword:'root'});

set_sysvar(session, "super_read_only", 1);
EXPECT_EQ(1, get_sysvar(session, "super_read_only"));

//@ test configureLocalInstance providing clusterAdminPassword without clusterAdmin
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdminPassword:'root'});

//@ test configureLocalInstance providing clusterAdminPassword and an existing clusterAdmin
dba.configureLocalInstance(__sandbox_uri1, { interactive: true, mycnfPath: mycnf, clusterAdmin: 'root2', clusterAdminPassword: 'whatever' });

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_no 8.0 {VER(>=8.0.11)}
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
testutil.expectPrompt("Do you want to disable super_read_only and continue?", "n");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root3', clusterAdminPassword:'root'});

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_no 5.7 {VER(<8.0.11)}
testutil.expectPrompt("Do you want to disable super_read_only and continue?", "n");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root3', clusterAdminPassword:'root'});

//@ Interactive_dba_configure_local_instance read_only_invalid_flag_value
// Since no expectation is set for create_cluster, a call to it would raise
// an exception
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root4', clusterAdminPassword:'root', clearReadOnly: 'NotABool'});

//@<OUT> Interactive_dba_configure_local_instance read_only_flag_true 8.0 {VER(>=8.0.11)}
session.runSql("RESET PERSIST enforce_gtid_consistency");
session.runSql("RESET PERSIST gtid_mode");
session.runSql("RESET PERSIST server_id");
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ(1, get_sysvar(session, "super_read_only"));
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root5', clusterAdminPassword:'root', clearReadOnly: true});

EXPECT_OUTPUT_NOT_CONTAINS("The MySQL instance at 'localhost:"+__mysql_sandbox_port1+"' currently has the super_read_only");

//@<OUT> Interactive_dba_configure_local_instance read_only_flag_true 5.7 {VER(<8.0.11)}
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ(1, get_sysvar(session, "super_read_only"));
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root5', clusterAdminPassword:'root', clearReadOnly: true});

EXPECT_OUTPUT_NOT_CONTAINS("The MySQL instance at 'localhost:"+__mysql_sandbox_port1+"' currently has the super_read_only");

//@ Interactive_dba_configure_local_instance read_only_flag_false 8.0 {VER(>=8.0.11)}
// Since the clusterAdmin 'root' was successfully created in the previous call
// we need to specify a different user, otherwise there's nothing to be done so
// the expected output regarding super-read-only won't be printed
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ(1, get_sysvar(session, "super_read_only"));
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
EXPECT_THROWS(function(){dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root6', clusterAdminPassword:'root', clearReadOnly: false})}, "Server in SUPER_READ_ONLY mode");

EXPECT_OUTPUT_NOT_CONTAINS("The instance 'localhost:"+__mysql_sandbox_port1+"' is valid for Cluster usage");

//@ Interactive_dba_configure_local_instance read_only_flag_false 5.7 {VER(<8.0.11)}
// Since the clusterAdmin 'root' was successfully created in the previous call
// we need to specify a different user, otherwise there's nothing to be done so
// the expected output regarding super-read-only won't be printed
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ(1, get_sysvar(session, "super_read_only"));
EXPECT_THROWS(function(){dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root6', clusterAdminPassword:'root', clearReadOnly: false})}, "Server in SUPER_READ_ONLY mode");

EXPECT_OUTPUT_NOT_CONTAINS("The instance 'localhost:"+__mysql_sandbox_port1+"' is valid for Cluster usage");

//@ Cleanup raw sandbox
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@ deploy sandbox, change dynamic variable values on the configuration and make it read-only (BUG#27702439)
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname}, {createRemoteRoot:false});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_checksum",
    "CRC32");
var mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);
testutil.makeFileReadOnly(mycnf_path);

//@<OUT> Interactive_dba_configure_local_instance should ask for creation of new configuration file and then ask user to copy it. (BUG#27702439)
//answer to create a new root@% account
testutil.expectPrompt("Please select an option [1]: ", "1");
testutil.expectPrompt("Account Host:", "%");
testutil.expectPrompt("Output path for updated configuration file:", mycnf_path+"2");
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
dba.configureLocalInstance(__sandbox_uri1, {interactive:true});

//@ Cleanup (BUG#27702439)
testutil.destroySandbox(__mysql_sandbox_port1);

//@ Deploy raw sandbox BUG#29725222 {VER(>= 8.0.17)}
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@<OUT> Run configure and restart instance BUG#29725222 {VER(>= 8.0.17)}
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "y");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true});

//@<OUT> Confirm changes were applied and everything is fine BUG#29725222 {VER(>= 8.0.17)}
testutil.waitSandboxAlive(__mysql_sandbox_port1);
dba.configureLocalInstance(__sandbox_uri1, {interactive:true});

//@ Cleanup BUG#29725222 {VER(>= 8.0.17)}
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Deploy sandbox WL#12758 IPv6 {VER(>= 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@<OUT> canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
dba.configureLocalInstance(__sandbox_uri1);

//@<> Cleanup WL#12758 IPV6 {VER(>= 8.0.14)}
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Deploy sandbox WL#12758 IPv4
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "127.0.0.1"});
var mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@<OUT> canonical IPv4 addresses are supported WL#12758
dba.configureLocalInstance(__sandbox_uri1,  {mycnfPath:mycnf_path});

//@<> Cleanup WL#12758 IPv4
testutil.destroySandbox(__mysql_sandbox_port1);

// DO NOT add tests which do not require __dbug_off to be 0 below this line

//@ Deploy raw sandbox, and check that configureLocalInstance is using the config path from interactive prompt (BUG#29554251) {VER(< 8.0.0) && __dbug_off == 0}
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", { report_host: hostname });
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);
// Override the get_default_config_paths function behavior to return empty list of config_paths.
testutil.dbugSet("+d,override_mycnf_default_path");

//@<OUT> Interactive_dba_configure_local_instance where we pass the configuration file path via wizard. (BUG#29554251) {VER(< 8.0.0) && __dbug_off == 0}
testutil.expectPrompt("Please specify the path to the MySQL configuration file: ", mycnf_path);
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
dba.configureLocalInstance(__sandbox_uri1, { interactive: true });

//@<OUT> Confirm binlog_checksum is wrote to config file (BUG#30171090) {VER(< 8.0.0) && __dbug_off == 0}
var match_list = testutil.grepFile(mycnf_path, "binlog_checksum");
print(match_list);

//@<OUT> Confirm that changes were applied to config file (BUG#29554251) {VER(< 8.0.0) && __dbug_off == 0}
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
dba.configureLocalInstance(__sandbox_uri1, { interactive: true, mycnfPath: mycnf_path });

//@ Cleanup (BUG#29554251) {VER(< 8.0.0) && __dbug_off == 0}
testutil.destroySandbox(__mysql_sandbox_port1);

// DO NOT add tests which do not require __dbug_off to be 0 above this line
// DO NOT add any tests below this line
