// Assumptions: smart deployment rountines available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, server_id: 1111});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, server_id: 2222});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);

//@ it's not possible to adopt from GR without existing group replication
dba.createCluster('testCluster', {adoptFromGR: true});

// create cluster with two instances and drop metadata schema

//@ Create group by hand
session2 = mysql.getSession(__sandbox_uri2);

session.runSql("CREATE USER mysql_innodb_cluster_r1@'%' IDENTIFIED BY 'aaa'");
session.runSql("GRANT ALL ON *.* TO mysql_innodb_cluster_r1@'%'");
session.runSql("CREATE USER some_user@'%' IDENTIFIED BY 'aaa'");
session.runSql("GRANT ALL ON *.* TO some_user@'%'");

ensure_plugin_enabled("group_replication", session);
ensure_plugin_enabled("group_replication", session2);

session.runSql("SET GLOBAL group_replication_group_name='6ed4243e-88b9-11e9-8eac-7281347dd9c6'");
session2.runSql("SET GLOBAL group_replication_group_name='6ed4243e-88b9-11e9-8eac-7281347dd9c6'");
session.runSql("SET GLOBAL group_replication_local_address='localhost:"+__mysql_sandbox_gr_port1+"'");
session2.runSql("SET GLOBAL group_replication_local_address='localhost:"+__mysql_sandbox_gr_port2+"'");
session2.runSql("SET GLOBAL group_replication_group_seeds='"+hostname+":"+__mysql_sandbox_gr_port1+"'");
session2.runSql("SET GLOBAL group_replication_recovery_use_ssl=1");
session.runSql("CHANGE MASTER TO master_user='mysql_innodb_cluster_r1', master_password='aaa' FOR CHANNEL 'group_replication_recovery'");
session2.runSql("CHANGE MASTER TO master_user='some_user', master_password='aaa' FOR CHANNEL 'group_replication_recovery'");
session.runSql("SET GLOBAL group_replication_bootstrap_group=1");
session.runSql("START group_replication");
session.runSql("SET GLOBAL group_replication_bootstrap_group=0");
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
session2.runSql("START group_replication");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Establish a session using the hostname (set for report_host)
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the real hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Create cluster adopting from GR
var cluster = dba.createCluster('testCluster', {adoptFromGR: true});

testutil.waitMemberTransactions(__mysql_sandbox_port2);

// Fix for BUG#28054500 expects that mysql_innodb_cluster_r* accounts are auto-deleted
// when adopting.

//@<OUT> Confirm new replication users were created and replaced existing ones, but didn't drop the old ones that don't belong to shell (WL#12773 FR1.5 and FR3)
// sandbox1
shell.dumpRows(session.runSql("SELECT user,host FROM mysql.user WHERE user like 'mysql_inno%' or user = 'some_user'"), "tabbed");
shell.dumpRows(session.runSql("SELECT instance_name,attributes FROM mysql_innodb_cluster_metadata.instances ORDER BY instance_id"), "tabbed");
shell.dumpRows(session.runSql("SELECT user_name as recovery_user_name FROM mysql.slave_master_info WHERE channel_name='group_replication_recovery'"), "tabbed");
// sandbox2
shell.dumpRows(session2.runSql("SELECT user,host FROM mysql.user WHERE user like 'mysql_inno%' or user = 'some_user'"), "tabbed");
shell.dumpRows(session2.runSql("SELECT instance_name,attributes FROM mysql_innodb_cluster_metadata.instances ORDER BY instance_id"), "tabbed");
shell.dumpRows(session2.runSql("SELECT user_name as recovery_user_name FROM mysql.slave_master_info WHERE channel_name='group_replication_recovery'"), "tabbed");

//@<> Check cluster status
var status = cluster.status();
EXPECT_EQ("testCluster", status["clusterName"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])

//@ dissolve the cluster
cluster.dissolve({force: true});

//@<> persist configs in 5.7 {VER(<8.0.0)}
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port1)});
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port2)});

//@<> ensure SRO stays 1 after server restart (dissolve shouldn't matter) {VER(>=8.0.11)}
// covers  Bug #30545872	CONFLICTING TRANSACTION SETS FOLLOWING COMPLETE OUTAGE OF INNODB CLUSTER
testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

EXPECT_EQ(1, session1.runSql("SELECT @@global.super_read_only").fetchOne()[0]);
EXPECT_EQ(1, session2.runSql("SELECT @@global.super_read_only").fetchOne()[0]);

//@ it's not possible to adopt from GR when cluster was dissolved
dba.createCluster('testCluster', {adoptFromGR: true});

// Close session
session.close();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
