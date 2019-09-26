// BUG#29305551: ADMINAPI FAILS TO DETECT INSTANCE IS RUNNING ASYNCHRONOUS REPLICATION
//
// dba.checkInstance() reports that a target instance which is running the Slave
// SQL and IO threads is valid to be used in an InnoDB cluster.
//
// As a consequence, the AdminAPI fails to detects that an instance has
// asynchronous replication running and both addInstance() and rejoinInstance()
// fail with useless/unfriendly errors on this scenario. There's not even
// information on how to overcome the issue.

//@ BUG#29305551: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@<> BUG#29305551: Create cluster
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', {gtidSetIsComplete: true});

//@<> AddInstance
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Simulate the instance dropping the GR group
session.close();
shell.connect(__sandbox_uri2);
session.runSql("STOP group_replication");
session.close();
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@<> BUG#29305551: Setup asynchronous replication
// Create Replication user
shell.connect(__sandbox_uri1);
session.runSql("CREATE USER 'repl'@'%' IDENTIFIED BY 'password' REQUIRE SSL");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';");

// Set async channel on instance2
session.close();
shell.connect(__sandbox_uri2);

session.runSql("CHANGE MASTER TO MASTER_HOST='" + hostname + "', MASTER_PORT=" + __mysql_sandbox_port1 + ", MASTER_USER='repl', MASTER_PASSWORD='password', MASTER_AUTO_POSITION=1, MASTER_SSL=1");
session.runSql("START SLAVE");

//@ rejoinInstance async replication error
c.rejoinInstance(__sandbox_uri2);

//@ BUG#29305551: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// BUG#29754915: rejoininstance fails if one or more cluster members have the recovering state
// NOTE: This issue was fixed from the Group Replication side for MySQL 8.0.17 by BUG#29754967.
//@ BUG#29754915: deploy sandboxes.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

//@ BUG#29754915: create cluster.
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', {gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ BUG#29754915: keep instance 2 in RECOVERING state by setting a wrong recovery user.
session.close();
shell.connect(__sandbox_uri2);
session.runSql("CHANGE MASTER TO MASTER_USER = 'not_exist', MASTER_PASSWORD = '' FOR CHANNEL 'group_replication_recovery'");
session.runSql("STOP GROUP_REPLICATION");
session.runSql("START GROUP_REPLICATION");
testutil.waitMemberState(__mysql_sandbox_port2, "RECOVERING");

//@ BUG#29754915: stop Group Replication on instance 3.
session.close();
shell.connect(__sandbox_uri3);
session.runSql("STOP GROUP_REPLICATION");

//@ BUG#29754915: get cluster to try to rejoin instance 3.
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster();

//@<> BUG#29754915: rejoin instance 3 successfully.
c.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> BUG#29754915: confirm cluster status.
c.status();

//@ BUG#29754915: clean-up.
c.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//@<> Initialization IPv6 addresses supported WL#12758 {VER(>= 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: "::1"});
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();
shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
session.close();
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

//@<> IPv6 addresses are supported on rejoinInstance ipWhitelist WL#12758 {VER(>= 8.0.14)}
var ip_white_list = "::1, 127.0.0.1";
cluster.rejoinInstance(__sandbox_uri2, {ipWhitelist:ip_white_list});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();
shell.connect(__sandbox_uri2);
EXPECT_EQ(ip_white_list, get_sysvar(session, "group_replication_ip_whitelist"));

//@<> Cleanup IPv6 addresses supported WL#12758 {VER(>= 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> Initialization canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();
shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
session.close();
testutil.changeSandboxConf(__mysql_sandbox_port2, "report_host", "::1");
testutil.restartSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
cluster.rejoinInstance(__sandbox_uri2);

//@ IPv6 on ipWhitelist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.changeSandboxConf(__mysql_sandbox_port2, "report_host", "127.0.0.1");
testutil.restartSandbox(__mysql_sandbox_port2);
cluster.rejoinInstance(__sandbox_uri2, {ipWhitelist: "::1, 127.0.0.1"});

//@<> Cleanup canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);


//@<> Initialization (BUG#30174191). {VER(<8.0.0)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@<> Create cluster (BUG#30174191). {VER(<8.0.0)}
shell.connect(__sandbox_uri1);
var c = dba.createCluster("cluster", {gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);

//NOTE: Changes not persisted on purpose (BUG#30174191).
//@<> Shutdown the primary for a new one to be elected (BUG#30174191). {VER(<8.0.0)}
testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING)");
var c = dba.getCluster();

//@<> Restart the previous instance (BUG#30174191). {VER(<8.0.0)}
testutil.startSandbox(__mysql_sandbox_port1);

//@<> Rejoin instance to cluster (BUG#30174191). {VER(<8.0.0)}
c.rejoinInstance(__sandbox_uri1);

//@<> Check auto increment settings (BUG#30174191). {VER(<8.0.0)}
var s1 = mysql.getSession(__sandbox_uri1);
var s2 = mysql.getSession(__sandbox_uri2);

var s1_aii = get_sysvar(s1, 'auto_increment_increment');
var s1_aio = get_sysvar(s1, 'auto_increment_offset');
EXPECT_EQ(1, s1_aii);
EXPECT_EQ(2, s1_aio);

var s2_aii = get_sysvar(s2, 'auto_increment_increment');
var s2_aio = get_sysvar(s2, 'auto_increment_offset');
EXPECT_EQ(1, s2_aii);
EXPECT_EQ(2, s2_aio);

//@<> Clean-up (BUG#30174191). {VER(<8.0.0)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// BUG#29953812 : ADD_INSTANCE() PICKY ABOUT GTID_EXECUTED, REJOIN_INSTANCE() NOT: DATA NOT COPIED
// Verify that rejoinInstance fails when:
//   - Instance data has diverged
//   - Instance is irrecoverable

//@<> Initialization (BUG#29953812)
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
var cluster = scene.cluster;
var session = scene.session;
var session2 = mysql.getSession(__sandbox_uri2);

var gtid_executed = session.runSql("SELECT @@global.gtid_executed").fetchOne()[0];

// Force the instance to drop out of the group
session2.runSql("stop group_replication;")

// Simulate errant transactions
session2.runSql("RESET MASTER");
session2.runSql("SET GLOBAL gtid_purged=?", [gtid_executed+",00025721-1111-1111-1111-111111111111:1"]);

//@ Rejoin instance fails if the target instance contains errant transactions (BUG#29953812) {VER(>=8.0.17)}
cluster.rejoinInstance(__sandbox_uri2);

//@ Rejoin instance fails if the target instance contains errant transactions 5.7 (BUG#29953812) {VER(<8.0.17)}
cluster.rejoinInstance(__sandbox_uri2);

//@ Rejoin instance fails if the target instance has an empty gtid-set (BUG#29953812)
session2.runSql("RESET MASTER");

cluster.rejoinInstance(__sandbox_uri2);

//@ Rejoin instance fails if the transactions were purged from the cluster (BUG#29953812)
session.runSql("FLUSH BINARY LOGS");
session.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");
session2.runSql("RESET MASTER");

cluster.rejoinInstance(__sandbox_uri2);

//@<> Finalization (BUG#29953812)
session.close();
session2.close();
scene.destroy();
