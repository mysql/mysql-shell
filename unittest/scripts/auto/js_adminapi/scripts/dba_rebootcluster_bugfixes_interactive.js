var uri2 = hostname + ":" + __mysql_sandbox_port2;
var uri3 = hostname + ":" + __mysql_sandbox_port3;

//@ Deploy sandboxes
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

// BUG#28207565: DBA.REBOOTCLUSTERFROMCOMPLETEOUTAGE DOES NOT USE DEFAULT
// CLUSTER
//
// On the default interactive mode, whenever using the function
// dba.rebootClusterFromCompleteOutage without any parameter, the function
// should assume the cluster to be rebooted is the default one but it
// doesn't and fails with an error specifying the cluster name '' does not
// exist.
//
// The issue is caused by the handling of the interactive layer on which if
// the user wants to rejoin the instances detected back to the cluster, or
// remove from the metadata, a dictionary of options is added to the
// function
// callback causing on a failure on the detection of the default cluster
// which is based on the call of the function without any parameters.

//@ Deploy cluster
shell.connect(__sandbox_uri1);
var c = dba.createCluster("myCluster", {memberSslMode: 'DISABLED'});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
var mycnf3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port2, {mycnfPath: mycnf2});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port3, {mycnfPath: mycnf3});

//@<OUT> Check cluster status before reboot
c.status();

session.close();

//@<> Reset gr_start_on_boot on all instances
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

//@ Kill all cluster members
shell.connect(__sandbox_uri1);
c.disconnect();
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
session.close();
testutil.killSandbox(__mysql_sandbox_port1);

//@ Start the members again
testutil.startSandbox(__mysql_sandbox_port3);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);

// Reboot cluster from complete outage, not specifying the name

shell.connect(__sandbox_uri1);
testutil.expectPrompt("Would you like to rejoin it to the cluster? [y/N]: ", "y");
testutil.expectPrompt("Would you like to rejoin it to the cluster? [y/N]: ", "y");

//@ Reboot cluster from complete outage, not specifying the name
var c = dba.rebootClusterFromCompleteOutage();

// Waiting for the instances to become online
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Check cluster status after reboot
c.status();

//@<> Finalization
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
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
c.dissolve();

//@<> BUG#29305551: Create cluster
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', {clearReadOnly: true, memberSslMode: 'DISABLED'});

//@<> BUG#29305551: Add instances to the cluster
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
session.close();

//@ BUG#29305551: persist GR configuration settings for 5.7 servers {VER(<8.0.11)}
var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var mycnf2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
var mycnf3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port1, {mycnfPath: mycnf1});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port2, {mycnfPath: mycnf2});
dba.configureLocalInstance('root:root@localhost:' + __mysql_sandbox_port3, {mycnfPath: mycnf3});


//@<> BUG#29305551: Reset gr_start_on_boot on all instances
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

//@<> BUG#29305551: Kill all cluster members.
c.disconnect();
shell.connect(__sandbox_uri1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");
session.close();
testutil.killSandbox(__mysql_sandbox_port1);

//@<> BUG#29305551: Start the members again.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
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
session.close();

//@ BUG#29305551 - Reboot cluster from complete outage, rejoin fails
shell.connect(__sandbox_uri1);
testutil.expectPrompt("Would you like to rejoin it to the cluster? [y/N]: ", "y");
testutil.expectPrompt("Would you like to rejoin it to the cluster? [y/N]: ", "y");
var c = dba.rebootClusterFromCompleteOutage("test");

// Waiting for the instances to become online
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Check cluster status after reboot - uri2 should not be present
c.status();

//@ BUG#29305551: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
