
//@ Initialization

testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);

var cluster = dba.createCluster("cluster");

function rebuild_cluster() {
  session.close();
  testutil.destroySandbox(__mysql_sandbox_port1);
  testutil.deploySandbox(__mysql_sandbox_port1, "root");
  shell.connect(__sandbox_uri1);
  var cluster = dba.createCluster("cluster");
  cluster.disconnect();
}

//---- Drop Metadata Schema Tests

//@ Interactive_dba_drop_metadata_schemaread_only_no_prompts
// Sets the required statements for the session

// super_read_only is OFF, no active sessions
set_sysvar("super_read_only", 0);

dba.dropMetadataSchema({force: true});

EXPECT_OUTPUT_CONTAINS("Metadata Schema successfully removed.");


//@ prepare Interactive_dba_drop_metadata_schemaread_only_no_flag_prompt_yes
rebuild_cluster();
// super_read_only is ON, no active sessions
set_sysvar("super_read_only", 1);

// The answer to the prompt about continuing cleaning up the read only
testutil.expectPrompt("*", "y");

//@<OUT> Interactive_dba_drop_metadata_schemaread_only_no_flag_prompt_yes
dba.dropMetadataSchema({force: true});

//@ prepare Interactive_dba_drop_metadata_schemaread_only_no_flag_prompt_no
rebuild_cluster();
// super_read_only is ON, no active sessions
set_sysvar("super_read_only", 1);

// The answer to the prompt about continuing cleaning up the read only
testutil.expectPrompt("*", "n");

//@<OUT> Interactive_dba_drop_metadata_schemaread_only_no_flag_prompt_no
// Since no expectation is set for create_cluster, a call to it would raise
// an exception
dba.dropMetadataSchema({force: true});

//@ prepare Interactive_dba_drop_metadata_schemaread_only_invalid_flag_value
rebuild_cluster();

//@# Interactive_dba_drop_metadata_schemaread_only_invalid_flag_value

// Since no expectation is set for create_cluster, a call to it would raise
// an exception
dba.dropMetadataSchema({force: true, clearReadOnly: 'NotABool'});

//@ prepare Interactive_dba_drop_metadata_schemaread_only_flag_true
rebuild_cluster();

// super_read_only is ON, no active sessions
set_sysvar("super_read_only", 1);

//@ Interactive_dba_drop_metadata_schemaread_only_flag_true

// It must call the Non Interactive checkInstanceConfiguration, we choose to
dba.dropMetadataSchema({force: true, clearReadOnly: true});

EXPECT_OUTPUT_CONTAINS("Metadata Schema successfully removed.", false);

EXPECT_OUTPUT_NOT_CONTAINS("The MySQL instance at 'localhost:<<<sb_port_1>>>' currently has the super_read_only");


//@ prepare Interactive_dba_drop_metadata_schemaread_only_flag_false
rebuild_cluster();
// super_read_only is ON, no active sessions
set_sysvar("super_read_only", 1);

//@# Interactive_dba_drop_metadata_schemaread_only_flag_false
dba.dropMetadataSchema({force: true, clearReadOnly: false});

EXPECT_OUTPUT_NOT_CONTAINS("Metadata Schema successfully removed.");

rebuild_cluster();

//@ Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
