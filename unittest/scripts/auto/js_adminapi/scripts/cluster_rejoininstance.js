// BUG#29305551: ADMINAPI FAILS TO DETECT INSTANCE IS RUNNING ASYNCHRONOUS REPLICATION
//
// dba.checkInstance() reports that a target instance which is running the Slave
// SQL and IO threads is valid for InnoDB cluster usage.
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
var c = dba.createCluster('test');

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
var c = dba.createCluster('test');
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
