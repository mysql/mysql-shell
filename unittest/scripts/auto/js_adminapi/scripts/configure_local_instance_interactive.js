//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

var mycnf = testutil.getSandboxConfPath(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);

//@<OUT> Interactive_dba_configure_local_instance read_only_no_prompts
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath:mycnf, clusterAdmin:'root', clusterAdminPassword:'root'});

//@<OUT> Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes
set_sysvar(session, "super_read_only", 1);

// The answer to the prompt about continuing cleaning up the read only
testutil.expectPrompt("*", "y");

dba.configureLocalInstance(__sandbox_uri1, {mycnfPath:mycnf, clusterAdmin:'root', clusterAdminPassword:'root'});

//@<OUT> Interactive_dba_configure_local_instance read_only_no_flag_prompt_no
set_sysvar(session, "super_read_only", 1);

// The answer to the prompt about continuing cleaning up the read only
testutil.expectPrompt("*", "n");

dba.configureLocalInstance(__sandbox_uri1, {mycnfPath:mycnf, clusterAdmin:'root', clusterAdminPassword:'root'});

//@# Interactive_dba_configure_local_instance read_only_invalid_flag_value
// Since no expectation is set for create_cluster, a call to it would raise
// an exception
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath:mycnf, clusterAdmin:'root', clusterAdminPassword:'root', clearReadOnly: 'NotABool'});

//@ Interactive_dba_configure_local_instance read_only_flag_true
set_sysvar(session, "super_read_only", 1);
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath:mycnf, clusterAdmin:'root', clusterAdminPassword:'root', clearReadOnly: true});

EXPECT_OUTPUT_NOT_CONTAINS("The MySQL instance at 'localhost:"+__mysql_sandbox_port1+"' currently has the super_read_only");

//@# Interactive_dba_configure_local_instance read_only_flag_false
set_sysvar(session, "super_read_only", 1);
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath:mycnf, clusterAdmin:'root', clusterAdminPassword:'root', clearReadOnly: false});

EXPECT_OUTPUT_CONTAINS("Validating instance...", false);
EXPECT_OUTPUT_NOT_CONTAINS("The instance 'localhost:"+__mysql_sandbox_port1+"' is valid for Cluster usage");

//@ Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
