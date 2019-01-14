// Assumptions: smart deployment rountines available

var __sandbox_uri1 = "mysql://root:root@localhost:"+__mysql_sandbox_port1;
var __sandbox_uri2 = "mysql://root:root@localhost:"+__mysql_sandbox_port2;
var __sandbox_uri3 = "mysql://root:root@localhost:"+__mysql_sandbox_port3;
var __sandbox_xuri1 = "mysqlx://root:root@localhost:"+__mysql_sandbox_port1+"0";
var __sandbox_xuri2 = "mysqlx://root:root@localhost:"+__mysql_sandbox_port2+"0";
var __sandbox_xuri3 = "mysqlx://root:root@localhost:"+__mysql_sandbox_port3+"0";

// Auto-detected protocol URIs
var __sandbox_xuri1_ = "root:root@localhost:"+__mysql_sandbox_port1+"0";
var __sandbox_xuri2_ = "root:root@localhost:"+__mysql_sandbox_port2+"0";
var __sandbox_xuri3_ = "root:root@localhost:"+__mysql_sandbox_port3+"0";


//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Setup 2 member cluster
var cluster = dba.createCluster('dev', {memberSslMode: 'DISABLED'});

cluster.addInstance({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// wait until sandbox2 replicates the cluster metadata from sb1
testutil.waitMemberTransactions(__mysql_sandbox_port2);

// session is stored on the cluster object so changing the global session should not affect cluster operations
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
session.close();

//@<OUT> cluster status
cluster.status();
cluster.disconnect();

//@<OUT> cluster session closed: no longer error
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
cluster = dba.getCluster();
session.close();
cluster.status();

//##### WL11251 tests - Single-Primary

//@# disconnect the cluster object
cluster.disconnect();
cluster.status();


//@ SP - getCluster() on primary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster();
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SP - getCluster() on secondary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
var cluster = dba.getCluster();
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SP - getCluster() on primary with connectToPrimary: true
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SP - getCluster() on secondary with connectToPrimary: true
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SP - getCluster() on primary with connectToPrimary: false
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SP - getCluster() on secondary with connectToPrimary: false
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX - getCluster() on session to primary
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX - getCluster() on session to secondary
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port2*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX - getCluster() on session to primary (no redirect)
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX - getCluster() on session to secondary (no redirect)
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port2*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX implicit - getCluster() on session to primary
shell.connect({host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX implicit - getCluster() on session to secondary
shell.connect({host: localhost, port: __mysql_sandbox_port2*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX implicit - getCluster() on session to primary (no redirect)
shell.connect({host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX implicit - getCluster() on session to secondary (no redirect)
shell.connect({host: localhost, port: __mysql_sandbox_port2*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SP - Connect with no options and ensure it will connect to the specified member
testutil.callMysqlsh([__sandbox_uri1, "-e", "shell.status()"]);

//@ SP - Connect with no options and ensure it will connect to the specified member 2
testutil.callMysqlsh([__sandbox_uri2, "-e", "shell.status()"]);

//@ SP - Connect with --redirect-primary while connected to the primary
testutil.callMysqlsh([__sandbox_uri1, "--redirect-primary", "-e", "shell.status()"]);

//@ SP - Connect with --redirect-primary while connected to a secondary
testutil.callMysqlsh([__sandbox_uri2, "--redirect-primary", "-e", "shell.status()"]);

//@ SP - Connect with --redirect-primary while connected to a non-cluster member (error)
testutil.callMysqlsh([__sandbox_uri3, "--redirect-primary", "-e", "shell.status()"]);

//@ SP - Connect with --redirect-secondary while connected to the primary
testutil.callMysqlsh([__sandbox_uri1, "--redirect-secondary", "-e", "shell.status()"]);

//@ SP - Connect with --redirect-secondary while connected to a secondary
testutil.callMysqlsh([__sandbox_uri2, "--redirect-secondary", "-e", "shell.status()"]);

//@ SP - Connect with --redirect-secondary while connected to a non-cluster member (error)
testutil.callMysqlsh([__sandbox_uri3, "--redirect-secondary", "-e", "shell.status()"]);

//@ SPX - Connect with no options and ensure it will connect to the specified member
testutil.callMysqlsh([__sandbox_xuri1, "-e", "shell.status()"]);

//@ SPX - Connect with no options and ensure it will connect to the specified member 2
testutil.callMysqlsh([__sandbox_xuri2, "-e", "shell.status()"]);

//@ SPX - Connect with --redirect-primary while connected to the primary
testutil.callMysqlsh([__sandbox_xuri1, "--redirect-primary", "-e", "shell.status()"]);

//@ SPX - Connect with --redirect-primary while connected to a secondary
testutil.callMysqlsh([__sandbox_xuri2, "--redirect-primary", "-e", "shell.status()"]);

//@ SPX - Connect with --redirect-primary while connected to a non-cluster member (error)
testutil.callMysqlsh([__sandbox_xuri3, "--redirect-primary", "-e", "shell.status()"]);

//@ SPX - Connect with --redirect-secondary while connected to the primary
testutil.callMysqlsh([__sandbox_xuri1, "--redirect-secondary", "-e", "shell.status()"]);

//@ SPX - Connect with --redirect-secondary while connected to a secondary
testutil.callMysqlsh([__sandbox_xuri2, "--redirect-secondary", "-e", "shell.status()"]);

//@ SPX - Connect with --redirect-secondary while connected to a non-cluster member (error)
testutil.callMysqlsh([__sandbox_xuri3, "--redirect-secondary", "-e", "shell.status()"]);

//@ SPX implicit - Connect with no options and ensure it will connect to the specified member
testutil.callMysqlsh([__sandbox_xuri1_, "-e", "shell.status()"]);

//@ SPX implicit - Connect with no options and ensure it will connect to the specified member 2
testutil.callMysqlsh([__sandbox_xuri2_, "-e", "shell.status()"]);

//@ SPX implicit - Connect with --redirect-primary while connected to the primary
testutil.callMysqlsh([__sandbox_xuri1_, "--redirect-primary", "-e", "shell.status()"]);

//@ SPX implicit - Connect with --redirect-primary while connected to a secondary
// Also ensures the redirected session type is the right type
testutil.callMysqlsh([__sandbox_xuri2_, "--redirect-primary", "-e", "shell.status()"]);

//@ SPX implicit - Connect with --redirect-primary while connected to a non-cluster member (error)
testutil.callMysqlsh([__sandbox_xuri3_, "--redirect-primary", "-e", "shell.status()"]);

//@ SPX implicit - Connect with --redirect-secondary while connected to the primary
testutil.callMysqlsh([__sandbox_xuri1_, "--redirect-secondary", "-e", "shell.status()"]);

//@ SPX implicit - Connect with --redirect-secondary while connected to a secondary
testutil.callMysqlsh([__sandbox_xuri2_, "--redirect-secondary", "-e", "shell.status()"]);

//@ SPX implicit - Connect with --redirect-secondary while connected to a non-cluster member (error)
testutil.callMysqlsh([__sandbox_xuri3_, "--redirect-secondary", "-e", "shell.status()"]);

//@ SP - Connect with --cluster 1
testutil.callMysqlsh([__sandbox_uri1, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SP - Connect with --cluster 2
testutil.callMysqlsh([__sandbox_uri2, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SP - Connect with --cluster py
testutil.callMysqlsh([__sandbox_uri1, "--py", "-e", "print cluster.status()", "--cluster"]);

//@ SP - Connect with --cluster on a non-cluster member + cmd (error)
testutil.callMysqlsh([__sandbox_uri3, "--js", "-e", "println(cluster.status())", "--cluster"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ SP - Connect with --cluster on a non-cluster member interactive (error)
testutil.callMysqlsh([__sandbox_uri3, "--js", "--interactive", "-e", "println(cluster.status())", "--cluster"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ SP - Connect with --cluster on a non-cluster member (error)
testutil.callMysqlsh([__sandbox_uri3, "--js", "--cluster"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ SP - Connect with --cluster + --redirect-primary 1
testutil.callMysqlsh([__sandbox_uri1, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SP - Connect with --cluster + --redirect-primary 2
testutil.callMysqlsh([__sandbox_uri2, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SP - Connect with --cluster + --redirect-secondary 1
testutil.callMysqlsh([__sandbox_uri1, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ SP - Connect with --cluster + --redirect-secondary 2
testutil.callMysqlsh([__sandbox_uri2, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster 1
testutil.callMysqlsh([__sandbox_xuri1, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster 2
testutil.callMysqlsh([__sandbox_xuri2, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster py
testutil.callMysqlsh([__sandbox_xuri2, "--py", "-e", "print cluster.status()", "--cluster"]);

//@ SPX - Connect with --cluster on a non-cluster member (error)
testutil.callMysqlsh([__sandbox_xuri3, "--js", "-e", "println(cluster.status())", "--cluster"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ SPX - Connect with --cluster + --redirect-primary 1
testutil.callMysqlsh([__sandbox_xuri1, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster + --redirect-primary 2
testutil.callMysqlsh([__sandbox_xuri2, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster + --redirect-secondary 1
testutil.callMysqlsh([__sandbox_xuri1, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster + --redirect-secondary 2
testutil.callMysqlsh([__sandbox_xuri2, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX implicit - Connect with --cluster 1
testutil.callMysqlsh([__sandbox_xuri1_, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX implicit - Connect with --cluster 2
testutil.callMysqlsh([__sandbox_xuri2_, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX implicit - Connect with --cluster py
testutil.callMysqlsh([__sandbox_xuri2_, "--py", "-e", "print cluster.status()", "--cluster"]);

//@ SPX implicit - Connect with --cluster on a non-cluster member (error)
testutil.callMysqlsh([__sandbox_xuri3_, "--js", "-e", "println(cluster.status())", "--cluster"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ SPX implicit - Connect with --cluster + --redirect-primary 1
testutil.callMysqlsh([__sandbox_xuri1_, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX implicit - Connect with --cluster + --redirect-primary 2
testutil.callMysqlsh([__sandbox_xuri2_, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX implicit - Connect with --cluster + --redirect-secondary 1
testutil.callMysqlsh([__sandbox_xuri1_, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX implicit - Connect with --cluster + --redirect-secondary 2
testutil.callMysqlsh([__sandbox_xuri2_, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);


//@ SP - Dissolve the single-primary cluster while still connected to a secondary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
cluster = dba.getCluster();
cluster.removeInstance({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
// dissolve will drop the metadata from sandbox1
cluster.dissolve({force:true});
// this should drop metadata from sandbox2 (where we're connected to)
//TODO(.) broken dba.dropMetadataSchema({force:true});
session.runSql("set sql_log_bin=0");
session.runSql("set global super_read_only = 0");
session.runSql("drop schema mysql_innodb_cluster_metadata");
session.runSql("reset master");
session.runSql("set sql_log_bin=1");

//@ Finalization
// This should now disconnect the cluster, even after dissolved
cluster.disconnect();
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
