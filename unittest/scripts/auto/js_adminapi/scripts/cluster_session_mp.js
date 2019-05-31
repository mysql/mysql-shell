// Assumptions: smart deployment rountines available

var __sandbox_uri1 = "mysql://root:root@localhost:"+__mysql_sandbox_port1;
var __sandbox_uri2 = "mysql://root:root@localhost:"+__mysql_sandbox_port2;
var __sandbox_uri3 = "mysql://root:root@localhost:"+__mysql_sandbox_port3;
var __sandbox_xuri1 = "mysqlx://root:root@localhost:"+__mysql_sandbox_port1+"0";
var __sandbox_xuri2 = "mysqlx://root:root@localhost:"+__mysql_sandbox_port2+"0";
var __sandbox_xuri3 = "mysqlx://root:root@localhost:"+__mysql_sandbox_port3+"0";

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Setup 2 member cluster
var cluster = dba.createCluster('dev', {memberSslMode: 'DISABLED', multiPrimary: true, force: true, clearReadOnly:true, gtidSetIsComplete: true});
cluster.addInstance({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//##### WL11251 tests - Multi-Primary

//@<OUT> Multi-primary check
cluster.status();
cluster.disconnect();
session.close();
//@ MP - getCluster() on primary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster();
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();
session.close();

//@ MP - getCluster() on another primary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
var cluster = dba.getCluster();
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();
session.close();

//@ MP - getCluster() on primary with connectToPrimary: true
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();
session.close();

//@ MP - getCluster() on another primary with connectToPrimary: true
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();
session.close();

//@ MP - getCluster() on primary with connectToPrimary: false
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();
session.close();

//@ MP - getCluster() on another primary with connectToPrimary: false
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();
session.close();

//@ MPX - getCluster() on primary
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();
session.close();

//@ MPX - getCluster() on primary (no redirect)
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();
session.close();

//@ MP - Connect with no options and ensure it will connect to the specified member
testutil.callMysqlsh([__sandbox_uri1, "-e", "shell.status()"]);

//@ MP - Connect with no options and ensure it will connect to the specified member 2
testutil.callMysqlsh([__sandbox_uri2, "-e", "shell.status()"]);

//@ MP - Connect with --redirect-primary while connected to the primary
testutil.callMysqlsh([__sandbox_uri1, "--redirect-primary", "-e", "shell.status()"]);

//@ MP - Connect with --redirect-primary while connected to another primary
testutil.callMysqlsh([__sandbox_uri2, "--redirect-primary", "-e", "shell.status()"]);

//@ MP - Connect with --redirect-secondary (error)
testutil.callMysqlsh([__sandbox_uri1, "--redirect-secondary", "-e", "shell.status()"]);

//@ MPX - Connect with no options and ensure it will connect to the specified member
testutil.callMysqlsh([__sandbox_xuri1, "-e", "shell.status()"]);

//@ MPX - Connect with no options and ensure it will connect to the specified member 2
testutil.callMysqlsh([__sandbox_xuri2, "-e", "shell.status()"]);

//@ MPX - Connect with --redirect-primary while connected to the primary
testutil.callMysqlsh([__sandbox_xuri1, "--redirect-primary", "-e", "shell.status()"]);

//@ MPX - Connect with --redirect-primary while connected to another primary
testutil.callMysqlsh([__sandbox_xuri2, "--redirect-primary", "-e", "shell.status()"]);

//@ MPX - Connect with --redirect-secondary (error)
testutil.callMysqlsh([__sandbox_xuri1, "--redirect-secondary", "-e", "shell.status()"]);


//@ MP - Connect with --cluster 1
testutil.callMysqlsh([__sandbox_uri1, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ MP - Connect with --cluster 2
testutil.callMysqlsh([__sandbox_uri2, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ MP - Connect with --cluster + --redirect-primary 1
testutil.callMysqlsh([__sandbox_uri1, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ MP - Connect with --cluster + --redirect-primary 2
testutil.callMysqlsh([__sandbox_uri2, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ MP - Connect with --cluster + --redirect-secondary (error)
testutil.callMysqlsh([__sandbox_uri1, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ MPX - Connect with --cluster 1
testutil.callMysqlsh([__sandbox_xuri1, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ MPX - Connect with --cluster 2
testutil.callMysqlsh([__sandbox_xuri2, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ MPX - Connect with --cluster + --redirect-primary 1
testutil.callMysqlsh([__sandbox_xuri1, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ MPX - Connect with --cluster + --redirect-primary 2
testutil.callMysqlsh([__sandbox_xuri2, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ MPX - Connect with --cluster + --redirect-secondary (error)
testutil.callMysqlsh([__sandbox_xuri1, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);


//@ Dissolve the multi-primary cluster while connected to sandbox2
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
cluster = dba.getCluster();

cluster.removeInstance({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
// Dissolve cluster from sandbox2
cluster.dissolve({force:true});

//@ Finalization
// This should now disconnect the cluster, even after dissolved
cluster.disconnect();
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
