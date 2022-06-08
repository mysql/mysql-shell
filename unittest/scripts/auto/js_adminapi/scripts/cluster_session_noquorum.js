// Assumptions: smart deployment rountines available

var __sandbox_uri1 = "mysql://root:root@localhost:"+__mysql_sandbox_port1;
var __sandbox_uri2 = "mysql://root:root@localhost:"+__mysql_sandbox_port2;
var __sandbox_xuri1 = "mysqlx://root:root@localhost:"+__mysql_sandbox_port1+"0";
var __sandbox_xuri2 = "mysqlx://root:root@localhost:"+__mysql_sandbox_port2+"0";

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

// Setup 2 member cluster and kill secondary
var cluster = dba.createCluster('dev', {memberSslMode: 'DISABLED', gtidSetIsComplete: true});
cluster.addInstance({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");

//@<OUT> Check status
cluster.status();

//@ Reconnect
session.close();
shell.connect(__sandbox_uri1);
cluster.disconnect();

//@<OUT> getCluster() again (ok)
var cluster = dba.getCluster();
cluster.status();
cluster.disconnect();

//@<> getCluster() and connectToPrimary:true (OK: deprecated: auto-redirect primary)
EXPECT_NO_THROWS(function() { cluster = dba.getCluster(null, {connectToPrimary:true}); });

//@<OUT> getCluster() and connectToPrimary:false (OK: deprecated: auto-redirect primary)
var cluster = dba.getCluster(null, {connectToPrimary:false});
cluster.status();
cluster.disconnect();

//@<OUT> Connect shell to surviving member with --redirect-primary (fail)
var rc = testutil.callMysqlsh([__sandbox_uri1, "--redirect-primary", "-e", "shell.status()"]);
EXPECT_EQ(1, rc);

//@<OUT> Connect shell to surviving member with --redirect-secondary (fail)
var rc = testutil.callMysqlsh([__sandbox_uri1, "--redirect-secondary", "-e", "shell.status()"]);
EXPECT_EQ(1, rc);

//@<OUT> Connect shell to surviving member with --cluster (ok)
var rc = testutil.callMysqlsh([__sandbox_uri1, "--cluster", "-e", "println(cluster.status())"]);
EXPECT_EQ(0, rc);

//@ Create new cluster and now kill the primary
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

// Setup 2 member cluster and kill primary
var cluster = dba.createCluster('dev', {memberSslMode: 'DISABLED', gtidSetIsComplete: true});
cluster.addInstance({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.killSandbox(__mysql_sandbox_port1);

session.close();
// Reconnect to secondary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
testutil.waitMemberState(__mysql_sandbox_port1, "UNREACHABLE");

cluster.disconnect();

//@<OUT> 2 getCluster() again (ok)
var cluster = dba.getCluster();
cluster.status();

cluster.disconnect();
//@<> 2 getCluster() and connectToPrimary:true (OK: deprecated: auto-redirect primary)
EXPECT_NO_THROWS(function() { cluster = dba.getCluster(null, {connectToPrimary:true}); });

//@<OUT> 2 getCluster() and connectToPrimary:false (OK: deprecated: auto-redirect primary)
var cluster = dba.getCluster(null, {connectToPrimary:false});
cluster.status();

//@<OUT> 2 Connect shell to surviving member with --redirect-primary (fail)
var rc = testutil.callMysqlsh([__sandbox_uri2, "--redirect-primary", "-e", "shell.status()"]);
EXPECT_EQ(1, rc);

//@<OUT> 2 Connect shell to surviving member with --redirect-secondary (fail)
var rc = testutil.callMysqlsh([__sandbox_uri2, "--redirect-secondary", "-e", "shell.status()"]);
EXPECT_EQ(1, rc);

//@<OUT> 2 Connect shell to surviving member with --cluster (ok)
var rc = testutil.callMysqlsh([__sandbox_uri2, "--cluster", "-e", "println(cluster.status())"]);
EXPECT_EQ(0, rc);


//@ Finalization
session.close();
// This should now disconnect the cluster, even after dissolved
cluster.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
