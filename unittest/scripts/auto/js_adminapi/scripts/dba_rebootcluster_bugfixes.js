// Regression for BUG#29265869: DBA.REBOOTCLUSTERFROMCOMPLETEOUTAGE OVERRIDES SOME OF THE PERSISTED GR SETTINGS
// - rebootClusterFromCompleteOutage() should not change any GR setting.

//@ BUG29265869 - Deploy sandboxes.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
var auto_rejoin_tries = 2;
var expel_timeout = 1;
var exit_state = "ABORT_SERVER";
var consistency = "BEFORE_ON_PRIMARY_FAILOVER";
var local_address1 = hostname + ":" + (__mysql_sandbox_port3 * 10 + 1).toString();
var ip_white_list80 = hostname_ip + "," + real_hostname;
var ip_white_list57 = hostname_ip;
var member_weight1 = 15;
var grp_name = "a1efe13d-20c3-11e9-9b77-3c6aa7197def";
var local_address2 = hostname + ":" + (__mysql_sandbox_port3).toString();
var member_weight2 = 75;
var uri1 = hostname + ":" + __mysql_sandbox_port1;
var uri2 = hostname + ":" + __mysql_sandbox_port2;

//@ BUG29265869 - Create cluster with custom GR settings. {VER(>=8.0.16)}
shell.connect(__sandbox_uri1);
var c;

if (__version_num < 80027) {
  c = dba.createCluster("test", {expelTimeout: expel_timeout, exitStateAction: exit_state, failoverConsistency: consistency, localAddress: local_address1, ipWhitelist: ip_white_list80, memberWeight: member_weight1, groupName: grp_name, autoRejoinTries: auto_rejoin_tries, gtidSetIsComplete: true});
} else {
  c = dba.createCluster("test", {expelTimeout: expel_timeout, exitStateAction: exit_state, failoverConsistency: consistency, localAddress: local_address1, ipWhitelist: ip_white_list80, memberWeight: member_weight1, groupName: grp_name, autoRejoinTries: auto_rejoin_tries, gtidSetIsComplete: true, communicationStack: "xcom"});
}
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@ BUG29265869 - Create cluster with custom GR settings for 5.7. {VER(<8.0.0)}
shell.connect(__sandbox_uri1);
var c = dba.createCluster("test", {localAddress: local_address1, ipWhitelist: ip_white_list57, groupName: grp_name, gtidSetIsComplete: true});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@ BUG29265869 - Add instance with custom GR settings. {VER(>=8.0.16)}
c.addInstance(__sandbox_uri2, {exitStateAction: exit_state, localAddress: local_address2, ipWhitelist: ip_white_list80, memberWeight: member_weight2,  autoRejoinTries: auto_rejoin_tries});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ BUG29265869 - Add instance with custom GR settings for 5.7. {VER(<8.0.0)}
c.addInstance(__sandbox_uri2, {localAddress: local_address2, ipWhitelist: ip_white_list57, memberWeight: member_weight2});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ BUG29265869 - Persist GR settings for 5.7. {VER(<8.0.0)}
var sandbox_cnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: sandbox_cnf1});
var sandbox_cnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: sandbox_cnf2});

//@<OUT> BUG29265869 - Show initial cluster options.
normalize_cluster_options(c.options());

session.close();

//@<> Reset gr_start_on_boot on all instances
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);

//@ BUG29265869 - Kill all cluster members.
c.disconnect();
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
session.close();
testutil.killSandbox(__mysql_sandbox_port1);

//@ BUG29265869 - Start the members again.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);

//@ BUG29265869 - connect to instance.
shell.connect(__sandbox_uri1);

//@ BUG29265869 - Reboot cluster from complete outage and BUG30501978 no provision output shown.
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});

// Waiting for the instances to become online
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> BUG29265869 - Show cluster options after reboot.
normalize_cluster_options(c.options());

//@<> BUG29265869 - clean-up
//NOTE: Do not destroy the sandboxes so they can be used on the following test
session.close();

// BUG#29305551: ADMINAPI FAILS TO DETECT INSTANCE IS RUNNING ASYNCHRONOUS REPLICATION
//
// dba.checkInstance() reports that a target instance which is running the Slave
// SQL and IO threads is valid to be used in an InnoDB cluster.
//
// As a consequence, the AdminAPI fails to detects that an instance has
// asynchronous replication running and both addInstance() and rejoinInstance()
// fail with useless/unfriendly errors on this scenario. There's not even
// information on how to overcome the issue.

// BUG#32197222: ADMINAPI CREATECLUSTER() SHOULD NOT ALLOW EXISTING ASYNC REPLICATION CHANNELS
//
// With BUG#29305551, dba.checkInstanceConfiguration() was extended to
// include a check to verify if asynchronous replication is configured and
// running on the target instance and print a warning if that's the case.
// On top of that, the same check is used in <Cluster>.addInstance() and
// <Cluster>.rejoinInstance() to terminate the commands with an error if
// such scenario is verified.
// The same check is also used in dba.rebootClusterFromCompleteOutage()
// whenever there are instances to be rejoined to the cluster.
//
// However, dba.createCluster() and rebootClusterFromCompleteOutage() were
// skipping that test.
//
// dba.rebootClusterFromCompleteOutage() must fail if asynchronous replication
// is running on the target instance

//@<> BUG#29305551: Initialization
c.dissolve({force: true});

//@<> BUG#29305551: Create cluster
shell.connect(__sandbox_uri1);
var c;
if (__version_num < 80027) {
  c = dba.createCluster('test', {clearReadOnly: true, gtidSetIsComplete: true});
} else {
  c = dba.createCluster('test', {clearReadOnly: true, gtidSetIsComplete: true, communicationStack: "xcom"});
}

//@<> BUG#29305551: Add instance to the cluster
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();

//@<> Update the configuration files again {VER(<8.0.0)}
var sandbox_cnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: sandbox_cnf1});
var sandbox_cnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: sandbox_cnf2});


//@<> BUG#29305551: Reset gr_start_on_boot on all instances
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);

//@<> BUG#29305551: Kill all cluster members.
c.disconnect();
session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session1.runSql("stop group_replication");
session2.runSql("stop group_replication");

//@<> BUG#29305551: Setup asynchronous replication
shell.connect(__sandbox_uri1);
// Create Replication user
session.runSql("SET GLOBAL super_read_only=0");
session.runSql("CREATE USER 'repl'@'%' IDENTIFIED BY 'password' REQUIRE SSL");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';");

//@<> BUG#29305551: setup asynchronous replication on the target instance
session.runSql("CHANGE MASTER TO MASTER_HOST='test', MASTER_PORT=3306, MASTER_USER='foo', MASTER_PASSWORD='bar'");

//@ BUG#29305551: Reboot cluster from complete outage must fail if async replication is configured on the target instance
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});

//@<> BUG#29305551: clean-up for the next test
session.runSql("RESET SLAVE ALL");

// Set async channel on instance2
session.close();
shell.connect(__sandbox_uri2);

session.runSql("RESET SLAVE ALL");
session.runSql("CHANGE MASTER TO MASTER_HOST='" + hostname + "', MASTER_PORT=" + __mysql_sandbox_port1 + ", MASTER_USER='repl', MASTER_PASSWORD='password', MASTER_AUTO_POSITION=1, MASTER_SSL=1");
session.runSql("START SLAVE");

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

//@ Reboot cluster from complete outage, secondary runs async replication = should succeed, but rejoin fail
shell.connect(__sandbox_uri1);
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});
c.status();

// BUG#32197197: ADMINAPI DOES NOT PROPERLY CHECK FOR PRECONFIGURED REPLICATION CHANNELS
//
// Even if replication is not running but configured, the warning/error has to
// be provided as implemented in BUG#29305551
session.runSql("STOP group_replication");
shell.connect(__sandbox_uri2);
session.runSql("STOP SLAVE");

//@ Reboot cluster from complete outage, secondary runs async replication = should succeed, but rejoin fail with channels stopped
shell.connect(__sandbox_uri1);
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});
c.status();

//@<> BUG#29305551: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//@<> ensure rebootCluster picks the member with the most total transactions, not just the executed ones

// this tests Bug #31673163	REBOOTCLUSTERFROMCOMPLETEOUTAGE() IS MISSING CHECK FOR PENDING CERT TRANSACTIONS
// procedure:
// 1 - have a 3 member cluster
// 2 - FTWRL in secondary sb2 to allow transactions to be received but not applied
// 3 - execute transaction at primary
// 4 - stop GR at other secondary sb3
// 5 - execute another transaction at primary
// 6 - shutdown primary
// 7 - rebootCluster using sb3.
// At this point, gtid_executed at sb3 will be bigger than at sb2, but gtid_executed + received will be bigger at sb2.
// The correct behaviour is for sb2 to become the primary, not sb3.

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', {clearReadOnly: true, gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

disable_auto_rejoin(session, __mysql_sandbox_port1);
disable_auto_rejoin(session2, __mysql_sandbox_port2);
disable_auto_rejoin(session3, __mysql_sandbox_port3);

session2.runSql("flush tables with read lock");

session.runSql("create schema testschema1");

testutil.waitMemberTransactions(__mysql_sandbox_port3);
session3.runSql("stop group_replication");

session.runSql("create schema testschema2");

// ensure that the transaction arrived at the secondary but it didn't get applied yet
wait(10, 0.1, function() {
  var diff = session2.runSql("SELECT gtid_subtract(received_transaction_set, @@global.gtid_executed) diff FROM performance_schema.replication_connection_status WHERE channel_name='group_replication_applier'").fetchOne()[0];
  println("received but not executed gtids:", diff);
  return '' != diff;
});

println("transactions at sb1:");
session.runSql("SELECT received_transaction_set, @@global.gtid_executed FROM performance_schema.replication_connection_status WHERE channel_name='group_replication_applier'");

testutil.killSandbox(__mysql_sandbox_port1);

println("transactions at sb2 before restart:");
session2.runSql("show schemas");
session2.runSql("SELECT received_transaction_set, @@global.gtid_executed FROM performance_schema.replication_connection_status WHERE channel_name='group_replication_applier'");

// we can't just stop GR now, because stop GR will try to apply the pending transactions
testutil.killSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2);

// now, we have:
// primary sb1 is shutdown
// secondary sb2 is OFFLINE
// secondary sb3 is OFFLINE, has bigger GTID_EXECUTED but smaller received_transaction_set

println("transactions at sb2:");
session2.runSql("SELECT received_transaction_set, @@global.gtid_executed FROM performance_schema.replication_connection_status WHERE channel_name='group_replication_applier'");
println("transactions at sb3:");
session3.runSql("SELECT received_transaction_set, @@global.gtid_executed FROM performance_schema.replication_connection_status WHERE channel_name='group_replication_applier'");

// try reboot while connected to sb3 (should fail)
shell.connect(__sandbox_uri3);
EXPECT_THROWS(function(){dba.rebootClusterFromCompleteOutage("test");}, "The active session instance ("+hostname+":"+__mysql_sandbox_port3+") isn't the most updated in comparison with the ONLINE instances of the Cluster's metadata. Please use the most up to date instance: '"+hostname+":"+__mysql_sandbox_port2+"'.");

// try reboot while connected to sb2 (should pass)
shell.connect(__sandbox_uri2);
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances:[__sandbox_uri3]});
c.status();

session.runSql("stop group_replication"); // stop at sb1

//@ Reboot cluster from complete outage, seed runs async replication = should pass
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("stop group_replication");

shell.connect(__sandbox_uri2);
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: []});

//@<> BUG#31673163: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

//@<> BUG30501978 create cluster and add some data to it
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
var c = dba.createCluster("test", {gtidSetIsComplete: true});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");


//@<> BUG30501978 - Persist GR settings for 5.7. {VER(<8.0.0)}
var sandbox_cnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: sandbox_cnf1});
var sandbox_cnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: sandbox_cnf2});

//@<> BUG30501978 Reset gr_start_on_boot on all instances and kill all cluster members
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);

// kill all cluster members
session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session1.runSql("stop group_replication");
session2.runSql("stop group_replication");

//@<> BUG30501978 - Insert errant transactions on instance 2
EXPECT_EQ(session1.runSql("SELECT @@GLOBAL.gtid_executed"), session2.runSql("SELECT @@GLOBAL.gtid_executed"));

// disable sro
session2.runSql("SET GLOBAL super_read_only=0");
session2.runSql("CREATE DATABASE ERRANTDB2");
// GTIds do not match
EXPECT_NE(session1.runSql("SELECT @@GLOBAL.gtid_executed"), session2.runSql("SELECT @@GLOBAL.gtid_executed"));

//@ BUG30501978 - Reboot cluster from complete outage fails with informative message saying current session is not the most up to date
shell.connect(__sandbox_uri1);
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});

//@ BUG30501978 - Reboot cluster from complete outage fails with informative message saying there is a gtid mismatch
// Insert transactions so that neither instance contains all of the gtids of the other
session1.runSql("SET GLOBAL super_read_only=0");
session1.runSql("CREATE DATABASE ERRANTDB1");
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});

//@<> BUG#32112864 - REBOOTCLUSTERFROMCOMPLETEOUTAGE() DOES NOT EXCLUDE INSTANCES IF IN OPTION "REMOVEINSTANCES" LIST
EXPECT_NO_THROWS(function(){var c = dba.rebootClusterFromCompleteOutage("test", {removeInstances: [uri2]})});

//@<> Verify instance2 was removed from the cluster
EXPECT_FALSE(exist_in_metadata_schema(__mysql_sandbox_port2));

//@<> Clean instance 2 errant transactions
session2.runSql("DROP DATABASE ERRANTDB2");
session2.runSql("RESET MASTER");

//@<> Add instance2 back to the cluster
c.addInstance(__sandbox_uri2);

//@ BUG30501978 - Reboot cluster from complete outage fails with informative message saying to run rejoinInstance
session1.runSql("STOP group_replication");
session2.runSql("STOP group_replication");
session1.runSql("FLUSH BINARY LOGS");
session1.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");
session2.runSql("RESET MASTER");
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});

//@<> BUG30501978: Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
