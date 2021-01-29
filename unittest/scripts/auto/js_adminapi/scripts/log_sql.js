// WL#13294 : option to log AdminAPI SQL
//
// Test SQL logging feature for adminAPI operations.
//
// NOTE: Do not verify all executed SQL statements, otherwise it will make this
//       test extremely hard to maintain.

var check_instance_sql = [
    "SELECT `major`, `minor`, `patch` FROM `mysql_innodb_cluster_metadata`.`schema_version`",
    "SELECT PRIVILEGE_TYPE FROM INFORMATION_SCHEMA.USER_PRIVILEGES WHERE GRANTEE=",
    "show GLOBAL variables where `variable_name` in ('server_id')",
    "show GLOBAL variables where `variable_name` in ('gtid_mode')"
];

var configure_instance_sql = [
    "select @@port, @@datadir",
    "SELECT DISTINCT grantee FROM information_schema.user_privileges WHERE grantee like",
    "show GLOBAL variables where `variable_name` in ('binlog_format')",
    "SET * `binlog_format` = 'ROW'"
];

var create_cluster_sql = [
    "SELECT @@hostname, @@report_host",
    "SELECT plugin_status FROM information_schema.plugins WHERE plugin_name = 'group_replication'",
    "START GROUP_REPLICATION",
    "CHANGE MASTER TO MASTER_USER = /*(*/ 'mysql_innodb_cluster_*' /*)*/, MASTER_PASSWORD = **** FOR CHANNEL 'group_replication_recovery'",
    "CHANGE REPLICATION SOURCE TO SOURCE_USER = /*(*/ 'mysql_innodb_cluster_*' /*)*/, SOURCE_PASSWORD = **** FOR CHANNEL 'group_replication_recovery'",
];

var add_instance_sql = [
    "select cluster_type from `mysql_innodb_cluster_metadata`.v2_this_instance",
    "show GLOBAL variables where `variable_name` in ('group_replication_ssl_mode')",
    "show GLOBAL variables where `variable_name` in ('server_id')",
    "CREATE USER IF NOT EXISTS '*'@'%' IDENTIFIED BY **** PASSWORD EXPIRE NEVER",
    "SET * `group_replication_single_primary_mode` = 'ON'",
    "INSERT INTO mysql_innodb_cluster_metadata.instances"
];

var check_instance_state_sql = [
    "SELECT @@GLOBAL.GTID_EXECUTED",
    "SELECT @@GLOBAL.GTID_PURGED"
];

var rejoin_instance_sql = [
    "SELECT",
    "START GROUP_REPLICATION"
];

var get_cluster_sql = [
    "SELECT @@group_replication_group_name group_name, NULLIF(CONCAT(''/*!80025, @@group_replication_view_change_uuid*/), '') group_view_change_uuid,  @@group_replication_single_primary_mode single_primary,  @@server_uuid,  member_state,  (SELECT    sum(IF(member_state in ('ONLINE', 'RECOVERING'), 1, 0)) > sum(1)/2   FROM performance_schema.replication_group_members  WHERE member_id = @@server_uuid OR member_state <> 'ERROR' ) has_quorum, COALESCE(/*!80002 member_role = 'PRIMARY', NULL AND */     NOT @@group_replication_single_primary_mode OR     member_id = (select variable_value       from performance_schema.global_status       where variable_name = 'group_replication_primary_member') ) is_primary FROM performance_schema.replication_group_members WHERE member_id = @@server_uuid",
    "SELECT @@group_replication_group_name"
];

var status_sql = [
    "select count(*) from performance_schema.replication_group_members where MEMBER_ID = @@server_uuid AND MEMBER_STATE NOT IN ('OFFLINE', 'UNREACHABLE')",
    "SELECT ",
    "show GLOBAL variables where `variable_name` in ('super_read_only')",
    "INSERT *"
];

var describe_sql = [
    "SELECT @@group_replication_group_name group_name, NULLIF(CONCAT(''/*!80025, @@group_replication_view_change_uuid*/), '') group_view_change_uuid,  @@group_replication_single_primary_mode single_primary,",
    "SELECT"
];

var set_option_sql = [
    "select cluster_type from `mysql_innodb_cluster_metadata`.v2_this_instance",
    "UPDATE mysql_innodb_cluster_metadata.clusters"
];

var set_instance_option_sql = [
    "SELECT COUNT(*) as count",
    "UPDATE mysql_innodb_cluster_metadata.instances SET instance_name"
];

var options_sql = [
    "select count(*) from performance_schema.replication_group_members where MEMBER_ID = @@server_uuid AND MEMBER_STATE NOT IN ('OFFLINE', 'UNREACHABLE')",
    "show GLOBAL variables where `variable_name` in ('group_replication_group_name')"
];

var rescan_sql = [
    "SELECT i.instance_id, i.cluster_id, c.group_name",
    "SELECT COALESCE(@@report_host, @@hostname)",
    "INSERT INTO mysql_innodb_cluster_metadata.instances"
];

var set_primary_instance_sql = [
    "SELECT m.member_id, m.member_state, m.member_host, m.member_port, m.member_role, m.member_version, s.view_id FROM performance_schema.replication_group_members m",
    "SELECT group_replication_set_as_primary(*)"
];

var switch_multi_primary_sql = [
    "SELECT group_replication_switch_to_multi_primary_mode()",
    "SET PERSIST `auto_increment_increment` = 7",
    "UPDATE mysql_innodb_cluster_metadata.clusters"
];

var switch_single_primary_sql = [
    "SELECT group_replication_switch_to_single_primary_mode(*)",
    "SET PERSIST `auto_increment_increment` = 1",
    "UPDATE mysql_innodb_cluster_metadata.clusters"
];

var force_quorum_sql = [
    "select cluster_type from `mysql_innodb_cluster_metadata`.v2_this_instance",
    "SET GLOBAL `group_replication_force_members` = '*'",
    "SET GLOBAL `group_replication_force_members` = ''"
];

var reboot_cluster_sql = [
    "SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata",
    "SET GLOBAL `group_replication_bootstrap_group` = 'ON'",
    "START GROUP_REPLICATION",
    "SELECT member_state FROM performance_schema.replication_group_members WHERE member_id = @@server_uuid"
];

var remove_instance_sql = [
    "select count(*) from performance_schema.replication_group_members where MEMBER_ID = @@server_uuid AND MEMBER_STATE NOT IN ('OFFLINE', 'UNREACHABLE')",
    "DROP USER IF EXISTS '*'@'%'",
    "DELETE FROM mysql_innodb_cluster_metadata.instances WHERE ",
    "STOP GROUP_REPLICATION",
];

var dissolve_sql = [
    "SELECT WAIT_FOR_EXECUTED_GTID_SET(*)",
    "DROP USER IF EXISTS '*'@'%'",
    "DELETE from mysql_innodb_cluster_metadata.clusters",
    "STOP GROUP_REPLICATION",
];

var drop_metadata_sql = [
    "select count(*) from performance_schema.replication_group_members where MEMBER_ID = @@server_uuid",
    "DROP SCHEMA IF EXISTS `mysql_innodb_cluster_metadata`"
];

// ---------- WL#13294 (dba.logSql = 0) ----------

//@<> WL#13294: Initialization (dba.logSql = 0).
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname}, {createRemoteRoot:true});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
var mycnf3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

//@<> WL#13294: set dba.logSql = 0.
\option dba.logSql = 0

//@<> WL#13294: set some configuration incorrectly (that can be fixed without restart) (dba.logSql = 0).
session.runSql("SET sql_log_bin=0");
session.runSql("SET GLOBAL binlog_format = STATEMENT");
session.runSql("SET sql_log_bin=1");

//@<> WL#13294: check instance (dba.logSql = 0).
WIPE_SHELL_LOG();
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_sql[3]);

//@<> WL#13294: configure instance (dba.logSql = 0).
WIPE_SHELL_LOG();
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf1});
EXPECT_SHELL_LOG_NOT_CONTAINS(configure_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(configure_instance_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(configure_instance_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(configure_instance_sql[3]);

//@<> WL#13294: create cluster (dba.logSql = 0).
WIPE_SHELL_LOG();
var c = dba.createCluster('test', {gtidSetIsComplete: true});
EXPECT_SHELL_LOG_NOT_CONTAINS(create_cluster_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(create_cluster_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(create_cluster_sql[2]);
//@<> WL#13294: create cluster (dba.logSql = 0). < 8.0.23 {VER(<8.0.23)}
EXPECT_SHELL_LOG_NOT_CONTAINS(create_cluster_sql[3]);
//@<> WL#13294: create cluster (dba.logSql = 0). >= 8.0.23 {VER(>=8.0.23)}
EXPECT_SHELL_LOG_NOT_CONTAINS(create_cluster_sql[4]);

//@<> WL#13294: add instance (dba.logSql = 0).
WIPE_SHELL_LOG();
c.addInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_NOT_CONTAINS(add_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(add_instance_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(add_instance_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(add_instance_sql[3]);
EXPECT_SHELL_LOG_NOT_CONTAINS(add_instance_sql[4]);
EXPECT_SHELL_LOG_NOT_CONTAINS(add_instance_sql[5]);

//@<> WL#13294: check instance state (dba.logSql = 0).
WIPE_SHELL_LOG();
c.checkInstanceState(__sandbox_uri3);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_state_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_state_sql[1]);

//@<> WL#13294: add another instance (dba.logSql = 0).
c.addInstance(__sandbox_uri3);

//@<> WL#13294: persist GR configuration settings for 5.7 servers (dba.logSql = 0). {VER(<8.0.11)}
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port2, {mycnfPath: mycnf2});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port3, {mycnfPath: mycnf3});

//@<> WL#13294: Stop GR on an instance to use rejoin instance (dba.logSql = 0).
shell.connect(__sandbox_uri3);
session.runSql("STOP GROUP_REPLICATION");
session.close();
shell.connect(__sandbox_uri1);

//@<> WL#13294: rejoin instance (dba.logSql = 0).
WIPE_SHELL_LOG();
c.rejoinInstance(__sandbox_uri3);
EXPECT_SHELL_LOG_NOT_CONTAINS(rejoin_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(rejoin_instance_sql[1]);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL#13294: get cluster (dba.logSql = 0).
WIPE_SHELL_LOG();
var c = dba.getCluster();
EXPECT_SHELL_LOG_NOT_CONTAINS(get_cluster_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(get_cluster_sql[1]);

//@<> WL#13294: status (dba.logSql = 0).
WIPE_SHELL_LOG();
c.status();
EXPECT_SHELL_LOG_NOT_CONTAINS(status_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(status_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(status_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(status_sql[3]);

//@<> WL#13294: describe (dba.logSql = 0).
WIPE_SHELL_LOG();
c.describe();
EXPECT_SHELL_LOG_NOT_CONTAINS(describe_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(describe_sql[1]);

//@<> WL#13294: set option (dba.logSql = 0).
WIPE_SHELL_LOG();
c.setOption("clusterName", "test_cluster");
EXPECT_SHELL_LOG_NOT_CONTAINS(set_option_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(set_option_sql[1]);

//@<> WL#13294: set instance option (dba.logSql = 0).
WIPE_SHELL_LOG();
c.setInstanceOption(__sandbox_uri3, "label", "instance_3_label");
EXPECT_SHELL_LOG_NOT_CONTAINS(set_instance_option_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(set_instance_option_sql[1]);

//@<> WL#13294: options (dba.logSql = 0).
WIPE_SHELL_LOG();
c.options();
EXPECT_SHELL_LOG_NOT_CONTAINS(options_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(options_sql[1]);

//@<> WL#13294: remove metadata from one instance for rescan() to add it (dba.logSql = 0).
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", ["instance_3_label"]);

//@<> WL#13294: rescan (dba.logSql = 0).
WIPE_SHELL_LOG();
c.rescan({addInstances: "AUTO"});
EXPECT_SHELL_LOG_NOT_CONTAINS(rescan_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(rescan_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(rescan_sql[2]);

//@<> WL#13294: set primary instance (dba.logSql = 0). {VER(>=8.0.13)}
WIPE_SHELL_LOG();
c.setPrimaryInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_NOT_CONTAINS(set_primary_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(set_primary_instance_sql[1]);

//@<> WL#13294: switch to multi-primary (dba.logSql = 0). {VER(>=8.0.13)}
WIPE_SHELL_LOG();
c.switchToMultiPrimaryMode();
EXPECT_SHELL_LOG_NOT_CONTAINS(switch_multi_primary_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(switch_multi_primary_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(switch_multi_primary_sql[2]);

//@<> WL#13294: switch to single-primary (dba.logSql = 0). {VER(>=8.0.13)}
WIPE_SHELL_LOG();
c.switchToSinglePrimaryMode(__sandbox_uri1);
EXPECT_SHELL_LOG_NOT_CONTAINS(switch_single_primary_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(switch_single_primary_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(switch_single_primary_sql[2]);

//@<> WL#13294: disable gr_start_on_boot on all instances to kill (dba.logSql = 0).
//disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);

//@<> WL#13294: lose quorum (dba.logSql = 0).
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE,(MISSING)");
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE,(MISSING)");

//@<> WL#13294: force quorum (dba.logSql = 0).
WIPE_SHELL_LOG();
c.forceQuorumUsingPartitionOf(__sandbox_uri1);
EXPECT_SHELL_LOG_NOT_CONTAINS(force_quorum_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(force_quorum_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(force_quorum_sql[2]);

//@<> WL#13294: restart killed instances (dba.logSql = 0).
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);

//@<> WL#13294: Stop GR on remaining cluster instance (dba.logSql = 0).
session.runSql("STOP GROUP_REPLICATION");

//@<> WL#13294: reboot cluster (dba.logSql = 0).
WIPE_SHELL_LOG();
var instance2 = hostname + ':' + __mysql_sandbox_port2;
var instance3 = hostname + ':' + __mysql_sandbox_port3;
c = dba.rebootClusterFromCompleteOutage("test_cluster", {rejoinInstances: [instance2, instance3]});
EXPECT_SHELL_LOG_NOT_CONTAINS(reboot_cluster_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(reboot_cluster_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(reboot_cluster_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(reboot_cluster_sql[3]);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL#13294: remove instance (dba.logSql = 0).
WIPE_SHELL_LOG();
c.removeInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_NOT_CONTAINS(remove_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(remove_instance_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(remove_instance_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(remove_instance_sql[3]);

//@<> WL#13294: dissolve (dba.logSql = 0).
WIPE_SHELL_LOG();
c.dissolve();
EXPECT_SHELL_LOG_NOT_CONTAINS(dissolve_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(dissolve_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(dissolve_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(dissolve_sql[3]);

//@<> WL#13294: drop metadata (dba.logSql = 0).
session.runSql("SET GLOBAL super_read_only = 'OFF'");
WIPE_SHELL_LOG();
dba.dropMetadataSchema({force: true});
EXPECT_SHELL_LOG_NOT_CONTAINS(drop_metadata_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(drop_metadata_sql[1]);

//@<> WL#13294: Clean-up (dba.logSql = 0).
\option dba.logSql = 0
\option verbose = 0
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);


// ---------- WL#13294 (dba.logSql = 1) ----------

//@<> WL#13294: Initialization (dba.logSql = 1).
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname}, {createRemoteRoot:true});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
var mycnf3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

//@<> WL#13294: set dba.logSql = 1.
\option dba.logSql = 1

//@<> WL#13294: set some configuration incorrectly (that can be fixed without restart) (dba.logSql = 1).
session.runSql("SET sql_log_bin=0");
session.runSql("SET GLOBAL binlog_format = STATEMENT");
session.runSql("SET sql_log_bin=1");

//@<> WL#13294: check instance (dba.logSql = 1).
WIPE_SHELL_LOG();
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_sql[3]);

//@<> WL#13294: configure instance (dba.logSql = 1).
WIPE_SHELL_LOG();
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf1});
EXPECT_SHELL_LOG_NOT_CONTAINS(configure_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(configure_instance_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(configure_instance_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(configure_instance_sql[3]);

//@<> WL#13294: create cluster (dba.logSql = 1).
WIPE_SHELL_LOG();
var c = dba.createCluster('test', {gtidSetIsComplete: true});
EXPECT_SHELL_LOG_NOT_CONTAINS(create_cluster_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(create_cluster_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(create_cluster_sql[2]);
//@<> WL#13294: create cluster (dba.logSql = 1). < 8.0.23 {VER(<8.0.23)}
EXPECT_SHELL_LOG_CONTAINS(create_cluster_sql[3]);
//@<> WL#13294: create cluster (dba.logSql = 1). >= 8.0.23 {VER(>=8.0.23)}
EXPECT_SHELL_LOG_CONTAINS(create_cluster_sql[4]);

//@<> WL#13294: add instance (dba.logSql = 1).
WIPE_SHELL_LOG();
c.addInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_NOT_CONTAINS(add_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(add_instance_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(add_instance_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(add_instance_sql[3]);
EXPECT_SHELL_LOG_CONTAINS(add_instance_sql[4]);
EXPECT_SHELL_LOG_CONTAINS(add_instance_sql[5]);

//@<> WL#13294: check instance state (dba.logSql = 1).
WIPE_SHELL_LOG();
c.checkInstanceState(__sandbox_uri3);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_state_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_state_sql[1]);

//@<> WL#13294: add another instance (dba.logSql = 1).
c.addInstance(__sandbox_uri3);

//@<> WL#13294: persist GR configuration settings for 5.7 servers (dba.logSql = 1). {VER(<8.0.11)}
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port2, {mycnfPath: mycnf2});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port3, {mycnfPath: mycnf3});

//@<> WL#13294: Stop GR on an instance to use rejoin instance (dba.logSql = 1).
shell.connect(__sandbox_uri3);
session.runSql("STOP GROUP_REPLICATION");
session.close();
shell.connect(__sandbox_uri1);

//@<> WL#13294: rejoin instance (dba.logSql = 1).
WIPE_SHELL_LOG();
c.rejoinInstance(__sandbox_uri3);
EXPECT_SHELL_LOG_NOT_CONTAINS(rejoin_instance_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(rejoin_instance_sql[1]);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL#13294: get cluster (dba.logSql = 1).
WIPE_SHELL_LOG();
var c = dba.getCluster();
EXPECT_SHELL_LOG_NOT_CONTAINS(get_cluster_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(get_cluster_sql[1]);

//@<> WL#13294: status (dba.logSql = 1).
WIPE_SHELL_LOG();
c.status();
EXPECT_SHELL_LOG_NOT_CONTAINS(status_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(status_sql[1]);
EXPECT_SHELL_LOG_NOT_CONTAINS(status_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(status_sql[3]);

//@<> WL#13294: describe (dba.logSql = 1).
WIPE_SHELL_LOG();
c.describe();
EXPECT_SHELL_LOG_NOT_CONTAINS(describe_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(describe_sql[1]);

//@<> WL#13294: set option (dba.logSql = 1).
WIPE_SHELL_LOG();
c.setOption("clusterName", "test_cluster");
EXPECT_SHELL_LOG_NOT_CONTAINS(set_option_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(set_option_sql[1]);

//@<> WL#13294: set instance option (dba.logSql = 1).
WIPE_SHELL_LOG();
c.setInstanceOption(__sandbox_uri3, "label", "instance_3_label");
EXPECT_SHELL_LOG_NOT_CONTAINS(set_instance_option_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(set_instance_option_sql[1]);

//@<> WL#13294: options (dba.logSql = 1).
WIPE_SHELL_LOG();
c.options();
EXPECT_SHELL_LOG_NOT_CONTAINS(options_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(options_sql[1]);

//@<> WL#13294: remove metadata from one instance for rescan() to add it (dba.logSql = 1).
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", ["instance_3_label"]);

//@<> WL#13294: rescan (dba.logSql = 1).
WIPE_SHELL_LOG();
c.rescan({addInstances: "AUTO"});
EXPECT_SHELL_LOG_NOT_CONTAINS(rescan_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(rescan_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(rescan_sql[2]);

//@<> WL#13294: set primary instance (dba.logSql = 1). {VER(>=8.0.13)}
WIPE_SHELL_LOG();
c.setPrimaryInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_NOT_CONTAINS(set_primary_instance_sql[0]);
EXPECT_SHELL_LOG_NOT_CONTAINS(set_primary_instance_sql[1]);

//@<> WL#13294: switch to multi-primary (dba.logSql = 1). {VER(>=8.0.13)}
WIPE_SHELL_LOG();
c.switchToMultiPrimaryMode();
EXPECT_SHELL_LOG_NOT_CONTAINS(switch_multi_primary_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(switch_multi_primary_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(switch_multi_primary_sql[2]);

//@<> WL#13294: switch to single-primary (dba.logSql = 1). {VER(>=8.0.13)}
WIPE_SHELL_LOG();
c.switchToSinglePrimaryMode(__sandbox_uri1);
EXPECT_SHELL_LOG_NOT_CONTAINS(switch_single_primary_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(switch_single_primary_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(switch_single_primary_sql[2]);

//@<> WL#13294: disable gr_start_on_boot on all instances to kill (dba.logSql = 1).
//disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);

//@<> WL#13294: lose quorum (dba.logSql = 1).
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE,(MISSING)");
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE,(MISSING)");

//@<> WL#13294: force quorum (dba.logSql = 1).
WIPE_SHELL_LOG();
c.forceQuorumUsingPartitionOf(__sandbox_uri1);
EXPECT_SHELL_LOG_NOT_CONTAINS(force_quorum_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(force_quorum_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(force_quorum_sql[2]);

//@<> WL#13294: restart killed instances (dba.logSql = 1).
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);

//@<> WL#13294: Stop GR on remaining cluster instance (dba.logSql = 1).
session.runSql("STOP GROUP_REPLICATION");

//@<> WL#13294: reboot cluster (dba.logSql = 1).
WIPE_SHELL_LOG();
var instance2 = hostname + ':' + __mysql_sandbox_port2;
var instance3 = hostname + ':' + __mysql_sandbox_port3;
c = dba.rebootClusterFromCompleteOutage("test_cluster", {rejoinInstances: [instance2, instance3]});
EXPECT_SHELL_LOG_NOT_CONTAINS(reboot_cluster_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(reboot_cluster_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(reboot_cluster_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(reboot_cluster_sql[3]);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL#13294: remove instance (dba.logSql = 1).
WIPE_SHELL_LOG();
c.removeInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_NOT_CONTAINS(remove_instance_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(remove_instance_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(remove_instance_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(remove_instance_sql[3]);

//@<> WL#13294: dissolve (dba.logSql = 1).
WIPE_SHELL_LOG();
c.dissolve();
EXPECT_SHELL_LOG_NOT_CONTAINS(dissolve_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(dissolve_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(dissolve_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(dissolve_sql[3]);

//@<> WL#13294: drop metadata (dba.logSql = 1).
session.runSql("SET GLOBAL super_read_only = 'OFF'");
WIPE_SHELL_LOG();
dba.dropMetadataSchema({force: true});
EXPECT_SHELL_LOG_NOT_CONTAINS(drop_metadata_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(drop_metadata_sql[1]);

//@<> WL#13294: Clean-up (dba.logSql = 1).
\option dba.logSql = 0
\option verbose = 0
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);


// ---------- WL#13294 (dba.logSql = 2) ----------

//@<> WL#13294: Initialization (dba.logSql = 2).
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname}, {createRemoteRoot:true});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
var mycnf3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

//@<> WL#13294: set dba.logSql = 2.
\option dba.logSql = 2

//@<> WL#13294: set some configuration incorrectly (that can be fixed without restart) (dba.logSql = 2).
session.runSql("SET sql_log_bin=0");
session.runSql("SET GLOBAL binlog_format = STATEMENT");
session.runSql("SET sql_log_bin=1");

//@<> WL#13294: check instance (dba.logSql = 2).
WIPE_SHELL_LOG();
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});
EXPECT_SHELL_LOG_NOT_CONTAINS(check_instance_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(check_instance_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(check_instance_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(check_instance_sql[3]);

//@<> WL#13294: configure instance (dba.logSql = 2).
WIPE_SHELL_LOG();
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf1});
EXPECT_SHELL_LOG_CONTAINS(configure_instance_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(configure_instance_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(configure_instance_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(configure_instance_sql[3]);

//@<> WL#13294: create cluster (dba.logSql = 2).
WIPE_SHELL_LOG();
var c = dba.createCluster('test', {gtidSetIsComplete: true});
EXPECT_SHELL_LOG_CONTAINS(create_cluster_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(create_cluster_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(create_cluster_sql[2]);
//@<> WL#13294: create cluster (dba.logSql = 2). < 8.0.23 {VER(<8.0.23)}
EXPECT_SHELL_LOG_CONTAINS(create_cluster_sql[3]);
//@<> WL#13294: create cluster (dba.logSql = 2). >= 8.0.23 {VER(>=8.0.23)}
EXPECT_SHELL_LOG_CONTAINS(create_cluster_sql[4]);

//@<> WL#13294: add instance (dba.logSql = 2).
WIPE_SHELL_LOG();
c.addInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_CONTAINS(add_instance_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(add_instance_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(add_instance_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(add_instance_sql[3]);
EXPECT_SHELL_LOG_CONTAINS(add_instance_sql[4]);
EXPECT_SHELL_LOG_CONTAINS(add_instance_sql[5]);

//@<> WL#13294: check instance state (dba.logSql = 2).
WIPE_SHELL_LOG();
c.checkInstanceState(__sandbox_uri3);
EXPECT_SHELL_LOG_CONTAINS(check_instance_state_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(check_instance_state_sql[1]);

//@<> WL#13294: add another instance (dba.logSql = 2).
c.addInstance(__sandbox_uri3);

//@<> WL#13294: persist GR configuration settings for 5.7 servers (dba.logSql = 2). {VER(<8.0.11)}
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port2, {mycnfPath: mycnf2});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port3, {mycnfPath: mycnf3});

//@<> WL#13294: Stop GR on an instance to use rejoin instance (dba.logSql = 2).
shell.connect(__sandbox_uri3);
session.runSql("STOP GROUP_REPLICATION");
session.close();
shell.connect(__sandbox_uri1);

//@<> WL#13294: rejoin instance (dba.logSql = 2).
WIPE_SHELL_LOG();
c.rejoinInstance(__sandbox_uri3);
EXPECT_SHELL_LOG_CONTAINS(rejoin_instance_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(rejoin_instance_sql[1]);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL#13294: get cluster (dba.logSql = 2).
WIPE_SHELL_LOG();
var c = dba.getCluster();
EXPECT_SHELL_LOG_CONTAINS(get_cluster_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(get_cluster_sql[1]);

//@<> WL#13294: status (dba.logSql = 2).
WIPE_SHELL_LOG();
c.status();
EXPECT_SHELL_LOG_CONTAINS(status_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(status_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(status_sql[2]);
EXPECT_SHELL_LOG_NOT_CONTAINS(status_sql[3]);

//@<> WL#13294: describe (dba.logSql = 2).
WIPE_SHELL_LOG();
c.describe();
EXPECT_SHELL_LOG_CONTAINS(describe_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(describe_sql[1]);

//@<> WL#13294: set option (dba.logSql = 2).
WIPE_SHELL_LOG();
c.setOption("clusterName", "test_cluster");
EXPECT_SHELL_LOG_CONTAINS(set_option_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(set_option_sql[1]);

//@<> WL#13294: set instance option (dba.logSql = 2).
WIPE_SHELL_LOG();
c.setInstanceOption(__sandbox_uri3, "label", "instance_3_label");
EXPECT_SHELL_LOG_CONTAINS(set_instance_option_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(set_instance_option_sql[1]);

//@<> WL#13294: options (dba.logSql = 2).
WIPE_SHELL_LOG();
c.options();
EXPECT_SHELL_LOG_CONTAINS(options_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(options_sql[1]);

//@<> WL#13294: remove metadata from one instance for rescan() to add it (dba.logSql = 2).
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", ["instance_3_label"]);

//@<> WL#13294: rescan (dba.logSql = 2).
WIPE_SHELL_LOG();
c.rescan({addInstances: "AUTO"});
EXPECT_SHELL_LOG_CONTAINS(rescan_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(rescan_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(rescan_sql[2]);

//@<> WL#13294: set primary instance (dba.logSql = 2). {VER(>=8.0.13)}
WIPE_SHELL_LOG();
c.setPrimaryInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_CONTAINS(set_primary_instance_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(set_primary_instance_sql[1]);

//@<> WL#13294: switch to multi-primary (dba.logSql = 2). {VER(>=8.0.13)}
WIPE_SHELL_LOG();
c.switchToMultiPrimaryMode();
EXPECT_SHELL_LOG_CONTAINS(switch_multi_primary_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(switch_multi_primary_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(switch_multi_primary_sql[2]);

//@<> WL#13294: switch to single-primary (dba.logSql = 2). {VER(>=8.0.13)}
WIPE_SHELL_LOG();
c.switchToSinglePrimaryMode(__sandbox_uri1);
EXPECT_SHELL_LOG_CONTAINS(switch_single_primary_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(switch_single_primary_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(switch_single_primary_sql[2]);

//@<> WL#13294: disable gr_start_on_boot on all instances to kill (dba.logSql = 2).
//disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);

//@<> WL#13294: lose quorum (dba.logSql = 2).
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE,(MISSING)");
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE,(MISSING)");

//@<> WL#13294: force quorum (dba.logSql = 2).
WIPE_SHELL_LOG();
c.forceQuorumUsingPartitionOf(__sandbox_uri1);
EXPECT_SHELL_LOG_CONTAINS(force_quorum_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(force_quorum_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(force_quorum_sql[2]);

//@<> WL#13294: restart killed instances (dba.logSql = 2).
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);

//@<> WL#13294: Stop GR on remaining cluster instance (dba.logSql = 2).
session.runSql("STOP GROUP_REPLICATION");

//@<> WL#13294: reboot cluster (dba.logSql = 2).
WIPE_SHELL_LOG();
var instance2 = hostname + ':' + __mysql_sandbox_port2;
var instance3 = hostname + ':' + __mysql_sandbox_port3;
c = dba.rebootClusterFromCompleteOutage("test_cluster", {rejoinInstances: [instance2, instance3]});
EXPECT_SHELL_LOG_CONTAINS(reboot_cluster_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(reboot_cluster_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(reboot_cluster_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(reboot_cluster_sql[3]);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL#13294: remove instance (dba.logSql = 2).
WIPE_SHELL_LOG();
c.removeInstance(__sandbox_uri2);
EXPECT_SHELL_LOG_CONTAINS(remove_instance_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(remove_instance_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(remove_instance_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(remove_instance_sql[3]);

//@<> WL#13294: dissolve (dba.logSql = 2).
WIPE_SHELL_LOG();
c.dissolve();
EXPECT_SHELL_LOG_CONTAINS(dissolve_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(dissolve_sql[1]);
EXPECT_SHELL_LOG_CONTAINS(dissolve_sql[2]);
EXPECT_SHELL_LOG_CONTAINS(dissolve_sql[3]);

//@<> WL#13294: drop metadata (dba.logSql = 2).
session.runSql("SET GLOBAL super_read_only = 'OFF'");
WIPE_SHELL_LOG();
dba.dropMetadataSchema({force: true});
EXPECT_SHELL_LOG_CONTAINS(drop_metadata_sql[0]);
EXPECT_SHELL_LOG_CONTAINS(drop_metadata_sql[1]);

//@<> WL#13294: Clean-up (dba.logSql = 2).
\option dba.logSql = 0
\option verbose = 0
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
