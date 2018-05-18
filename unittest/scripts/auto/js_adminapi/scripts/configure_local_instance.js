//@ deploy the sandbox
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

var mycnf = testutil.getSandboxConfPath(__mysql_sandbox_port1);

//@ ConfigureLocalInstance should fail if there's no session nor parameters provided
dba.configureLocalInstance();

//@<OUT> Interactive_dba_configure_local_instance read_only_no_prompts
dba.configureLocalInstance(__sandbox_uri1, {interactive: false, mycnfPath:mycnf, clusterAdmin:'root', clusterAdminPassword:'root'});

shell.connect(__sandbox_uri1);
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes 8.0 {VER(>=8.0.11)}
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
testutil.expectPrompt("Do you want to disable super_read_only and continue?", "y");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root2', clusterAdminPassword:'root'});

set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes 5.7 {VER(<8.0.11)}
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to disable super_read_only and continue?", "y");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root2', clusterAdminPassword:'root'});

set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_no 8.0 {VER(>=8.0.11)}
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "n");
testutil.expectPrompt("Do you want to disable super_read_only and continue?", "n");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root3', clusterAdminPassword:'root'});

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_no 5.7 {VER(<8.0.11)}
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to disable super_read_only and continue?", "n");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root3', clusterAdminPassword:'root'});

//@ Interactive_dba_configure_local_instance read_only_invalid_flag_value
// Since no expectation is set for create_cluster, a call to it would raise
// an exception
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root4', clusterAdminPassword:'root', clearReadOnly: 'NotABool'});

//@<OUT> Interactive_dba_configure_local_instance read_only_flag_true 8.0 {VER(>=8.0.11)}
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "y");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root5', clusterAdminPassword:'root', clearReadOnly: true});

EXPECT_OUTPUT_NOT_CONTAINS("The MySQL instance at 'localhost:"+__mysql_sandbox_port1+"' currently has the super_read_only");

//@<OUT> Interactive_dba_configure_local_instance read_only_flag_true 5.7 {VER(<8.0.11)}
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root5', clusterAdminPassword:'root', clearReadOnly: true});

EXPECT_OUTPUT_NOT_CONTAINS("The MySQL instance at 'localhost:"+__mysql_sandbox_port1+"' currently has the super_read_only");

//@ Interactive_dba_configure_local_instance read_only_flag_false 8.0 {VER(>=8.0.11)}
// Since the clusterAdmin 'root' was successfully created in the previous call
// we need to specify a different user, otherwise there's nothing to be done so
// the expected output regarding super-read-only won't be printed
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
testutil.expectPrompt("Do you want to restart the instance after configuring it?", "y");
EXPECT_THROWS(function(){dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root6', clusterAdminPassword:'root', clearReadOnly: false})}, "Server in SUPER_READ_ONLY mode");

EXPECT_OUTPUT_NOT_CONTAINS("The instance 'localhost:"+__mysql_sandbox_port1+"' is valid for Cluster usage");

//@ Interactive_dba_configure_local_instance read_only_flag_false 5.7 {VER(<8.0.11)}
// Since the clusterAdmin 'root' was successfully created in the previous call
// we need to specify a different user, otherwise there's nothing to be done so
// the expected output regarding super-read-only won't be printed
set_sysvar(session, "super_read_only", 1);
EXPECT_EQ('ON', get_sysvar(session, "super_read_only"));
testutil.expectPrompt("Do you want to perform the required configuration changes?", "y");
EXPECT_THROWS(function(){dba.configureLocalInstance(__sandbox_uri1, {interactive: true, mycnfPath:mycnf, clusterAdmin:'root6', clusterAdminPassword:'root', clearReadOnly: false})}, "Server in SUPER_READ_ONLY mode");

EXPECT_OUTPUT_NOT_CONTAINS("The instance 'localhost:"+__mysql_sandbox_port1+"' is valid for Cluster usage");

//@ Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
