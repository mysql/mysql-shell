// Regression for BUG#29265869: DBA.REBOOTCLUSTERFROMCOMPLETEOUTAGE OVERRIDES SOME OF THE PERSISTED GR SETTINGS
// - rebootClusterFromCompleteOutage() should not change any GR setting.

//@ BUG29265869 - Deploy sandboxes.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
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
var c = dba.createCluster("test", {expelTimeout: expel_timeout, exitStateAction: exit_state, failoverConsistency: consistency, localAddress: local_address1, ipWhitelist: ip_white_list80, memberWeight: member_weight1, groupName: grp_name});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@ BUG29265869 - Create cluster with custom GR settings for 5.7. {VER(<8.0.0)}
shell.connect(__sandbox_uri1);
var c = dba.createCluster("test", {localAddress: local_address1, ipWhitelist: ip_white_list57, groupName: grp_name});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@ BUG29265869 - Add instance with custom GR settings. {VER(>=8.0.16)}
c.addInstance(__sandbox_uri2, {exitStateAction: exit_state, localAddress: local_address2, ipWhitelist: ip_white_list80, memberWeight: member_weight2});
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
c.options();

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

//@ BUG29265869 - Reboot cluster from complete outage.
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});

// Waiting for the instances to become online
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> BUG29265869 - Show cluster options after reboot.
c.options();

//@<> BUG29265869 - clean-up
//NOTE: Do not destroy the sandboxes so they can be used on the following test
session.close();

// BUG#29305551: ADMINAPI FAILS TO DETECT INSTANCE IS RUNNING ASYNCHRONOUS REPLICATION
//
// dba.checkInstance() reports that a target instance which is running the Slave
// SQL and IO threads is valid for InnoDB cluster usage.
//
// As a consequence, the AdminAPI fails to detects that an instance has
// asynchronous replication running and both addInstance() and rejoinInstance()
// fail with useless/unfriendly errors on this scenario. There's not even
// information on how to overcome the issue.

//@<> BUG#29305551: Initialization
c.dissolve({force: true});

//@<> BUG#29305551: Create cluster
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', {clearReadOnly: true});

//@<> BUG#29305551: Add instance to the cluster
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
session.close();

//@<> BUG#29305551: Reset gr_start_on_boot on all instances
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);

//@<> BUG#29305551: Kill all cluster members.
c.disconnect();
shell.connect(__sandbox_uri1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
session.close();
testutil.killSandbox(__mysql_sandbox_port1);

//@<> BUG#29305551: Start the members again.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);

//@<> BUG#29305551: Setup asynchronous replication
shell.connect(__sandbox_uri1);
// Create Replication user
session.runSql("SET GLOBAL super_read_only=0");
session.runSql("CREATE USER 'repl'@'%' IDENTIFIED BY 'password' REQUIRE SSL");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';");

// Set async channel on instance2
session.close();
shell.connect(__sandbox_uri2);

session.runSql("CHANGE MASTER TO MASTER_HOST='" + hostname + "', MASTER_PORT=" + __mysql_sandbox_port1 + ", MASTER_USER='repl', MASTER_PASSWORD='password', MASTER_AUTO_POSITION=1, MASTER_SSL=1");
session.runSql("START SLAVE");

//@ BUG#29305551 - Reboot cluster from complete outage, rejoin fails
var c = dba.rebootClusterFromCompleteOutage("test", {rejoinInstances: [uri2]});

//@ BUG#29305551: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
