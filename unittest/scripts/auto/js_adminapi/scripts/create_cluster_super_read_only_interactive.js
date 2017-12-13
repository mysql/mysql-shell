
//@ Initialization

testutil.deploySandbox(__mysql_sandbox_port1, "root");


function rebuild_sandbox() {
  session.close();
  testutil.destroySandbox(__mysql_sandbox_port1);
  testutil.deploySandbox(__mysql_sandbox_port1, "root");
  shell.connect(__sandbox_uri1);
}

// ==== Create cluster

//@ prepare create_cluster.read_only_no_prompts
shell.connect(__sandbox_uri1);
// super_read_only is OFF, no active sessions
set_sysvar("super_read_only", 0);

// It must call createCluster with no additional options since the interactive
// layer did not require to resolve anything

//@<OUT> create_cluster.read_only_no_prompts
var c = dba.createCluster('dev');

//@ prepare create_cluster.read_only_no_flag_prompt_yes
c.disconnect();
rebuild_sandbox();

// super_read_only is ON, no active sessions
set_sysvar("super_read_only", 1);

// The answer to the prompt about continuing cleaning up the read only
testutil.expectPrompt("*", "y");

//@<OUT> create_cluster.read_only_no_flag_prompt_yes
var c = dba.createCluster('dev');

EXPECT_EQ('OFF', get_sysvar("super_read_only"));

//@ prepare create_cluster.read_only_no_flag_prompt_no
c.disconnect();
rebuild_sandbox();

// super_read_only is ON, no active sessions
set_sysvar("super_read_only", 1);

// The answer to the prompt about continuing cleaning up the read only
testutil.expectPrompt("*", "n");

//@<OUT> create_cluster.read_only_no_flag_prompt_no
// Since no expectation is set for create_cluster, a call to it would raise
// an exception
var c = dba.createCluster('dev');

//@ prepare create_cluster.read_only_invalid_flag_value
rebuild_sandbox();

//@# create_cluster.read_only_invalid_flag_value

// Since no expectation is set for create_cluster, a call to it would raise
// an exception
var c = dba.createCluster('dev', {clearReadOnly: 'NotABool'});

//@ prepare create_cluster.read_only_flag_true
rebuild_sandbox();

// super_read_only is ON, no active sessions
set_sysvar("super_read_only", 1);

//@<OUT> create_cluster.read_only_flag_true
var c = dba.createCluster('dev', {clearReadOnly: true});

EXPECT_OUTPUT_NOT_CONTAINS("The MySQL instance at 'localhost:"+__mysql_sandbox_port1+"' currently has the super_read_only");

EXPECT_EQ('OFF', get_sysvar("super_read_only"));

//@ prepare create_cluster.read_only_flag_false
c.disconnect();
rebuild_sandbox();
// super_read_only is ON, no active sessions
set_sysvar("super_read_only", 1);

//@# create_cluster.read_only_flag_false
var c = dba.createCluster('dev', {clearReadOnly: false});

EXPECT_OUTPUT_NOT_CONTAINS("Adding Seed Instance...");

EXPECT_EQ('ON', get_sysvar("super_read_only"));

//@ Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
