//@{!__dbug_off}

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, server_id:11});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, server_id:22});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname, server_id:33});

//@<> Make sure that variables are reverted in case the command fails (createCluster) {VER(>= 8.0.17)}

shell.connect(__sandbox_uri1);

ensure_plugin_enabled("group_replication", session);

set_sysvar(session, "auto_increment_increment", 56);
set_sysvar(session, "auto_increment_offset", 77);
set_sysvar(session, "group_replication_ssl_mode", "REQUIRED");
set_sysvar(session, "group_replication_recovery_use_ssl", "ON");
set_sysvar(session, "group_replication_group_name", "54e7f981-610d-4d36-b344-3b943416eadd");
old_group_replication_communication_stack = get_sysvar(session, "group_replication_communication_stack");

testutil.dbugSet("+d,dba_revert_trigger_exception");
EXPECT_THROWS(function() {
    dba.createCluster("cluster");
}, "Exception while creating the cluster.");
testutil.dbugSet("");

EXPECT_SHELL_LOG_CONTAINS("createCluster() failed: Trying to revert changes...");

EXPECT_EQ(56, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(77, get_sysvar(session, "auto_increment_offset"));
EXPECT_EQ("REQUIRED", get_sysvar(session, "group_replication_ssl_mode"));
EXPECT_NE(0, get_sysvar(session, "group_replication_recovery_use_ssl"));
EXPECT_EQ("54e7f981-610d-4d36-b344-3b943416eadd", get_sysvar(session, "group_replication_group_name"));
EXPECT_EQ(old_group_replication_communication_stack, get_sysvar(session, "group_replication_communication_stack"));

reset_instance(session);

//@<> Make sure that variables are reverted in case the command fails (addInstance) {VER(>= 8.0.17)}

shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster"); });

shell.connect(__sandbox_uri2);

ensure_plugin_enabled("group_replication", session);

set_sysvar(session, "auto_increment_increment", 57);
set_sysvar(session, "auto_increment_offset", 78);
set_sysvar(session, "group_replication_ssl_mode", "REQUIRED");
set_sysvar(session, "group_replication_recovery_use_ssl", "ON");
set_sysvar(session, "group_replication_group_name", "c7c091ef-9b7a-4e61-b319-04e24cf14fc8");
old_group_replication_communication_stack = get_sysvar(session, "group_replication_communication_stack");

testutil.dbugSet("+d,dba_revert_trigger_exception");
EXPECT_THROWS(function() {
    cluster.addInstance(__sandbox_uri2, { recoveryMethod: "clone" });
}, "Exception while adding instance.");
testutil.dbugSet("");

shell.connect(__sandbox_uri2);

EXPECT_EQ(57, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(78, get_sysvar(session, "auto_increment_offset"));
EXPECT_EQ("REQUIRED", get_sysvar(session, "group_replication_ssl_mode"));
EXPECT_NE(0, get_sysvar(session, "group_replication_recovery_use_ssl"));
// EXPECT_EQ("c7c091ef-9b7a-4e61-b319-04e24cf14fc8", get_sysvar(session, "group_replication_group_name"));
EXPECT_EQ(old_group_replication_communication_stack, get_sysvar(session, "group_replication_communication_stack"));

//@<> Make sure that variables are reverted in case the command fails (rejoinInstance) {VER(>= 8.0.17)}

EXPECT_NO_THROWS(function(){ cluster.addInstance(__sandbox_uri2, { recoveryMethod: "clone"}); });

shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");

set_sysvar(session, "auto_increment_increment", 57);
set_sysvar(session, "auto_increment_offset", 78);
set_sysvar(session, "group_replication_ssl_mode", "DISABLED");
set_sysvar(session, "group_replication_recovery_use_ssl", "OFF");

testutil.dbugSet("+d,dba_revert_trigger_exception");
EXPECT_THROWS(function() {
    cluster.rejoinInstance(__sandbox_uri2);
}, "Exception while rejoining instance.");
testutil.dbugSet("");

EXPECT_EQ(57, get_sysvar(session, "auto_increment_increment"));
EXPECT_EQ(78, get_sysvar(session, "auto_increment_offset"));
EXPECT_EQ("DISABLED", get_sysvar(session, "group_replication_ssl_mode"));
EXPECT_EQ(0, get_sysvar(session, "group_replication_recovery_use_ssl"));

/**
TODO: add revert tests for remaining cluster scenarios, mainly:
 - addReplicaInstance
 - removeInstance
 - rescan
 - resetRecoveryAccountsPassword
 - setPrimaryInstance
 - switchToMultiPrimaryMode
 - switchToSinglePrimaryMode
*/

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
