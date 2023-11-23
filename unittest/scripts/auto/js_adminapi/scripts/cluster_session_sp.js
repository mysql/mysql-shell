// Assumptions: smart deployment routines available

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
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Setup 2 member cluster
var cluster = dba.createCluster('dev', {memberSslMode: 'DISABLED', gtidSetIsComplete: true});

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

//@ SP - getCluster() on primary with connectToPrimary: true: deprecated - auto-redirect to primary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
EXPECT_OUTPUT_CONTAINS("WARNING: Option 'connectToPrimary' is deprecated and it will be removed in a future release. Shell will attempt to connect to the primary member of the Cluster by default and fallback to a secondary when not possible");

// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SP - getCluster() on secondary with connectToPrimary: true: deprecated - auto-redirect to primary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});

// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SP - getCluster() on primary with connectToPrimary: false: deprecated - auto-redirect to primary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
EXPECT_OUTPUT_CONTAINS("WARNING: Option 'connectToPrimary' is deprecated and it will be removed in a future release. Shell will attempt to connect to the primary member of the Cluster by default and fallback to a secondary when not possible");

// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SP - getCluster() on secondary with connectToPrimary: false: deprecated - auto-redirect to primary
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX - getCluster() on session to primary: true: deprecated - auto-redirect to primary
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX - getCluster() on session to secondary: true: deprecated - auto-redirect to primary
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port2*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX - getCluster() on session to primary: false: deprecated - auto-redirect to primary
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX - getCluster() on session to secondary: false: deprecated - auto-redirect to primary
shell.connect({scheme:'mysqlx', host: localhost, port: __mysql_sandbox_port2*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX implicit - getCluster() on session to primary: true: deprecated - auto-redirect to primary
shell.connect({host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX implicit - getCluster() on session to secondary: true: deprecated - auto-redirect to primary
shell.connect({host: localhost, port: __mysql_sandbox_port2*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:true});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX implicit - getCluster() on session to primary: false: deprecated - auto-redirect to primary
shell.connect({host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();
cluster.disconnect();

//@ SPX implicit - getCluster() on session to secondary: false: deprecated - auto-redirect to primary
shell.connect({host: localhost, port: __mysql_sandbox_port2*10, user: 'root', password: 'root'});
var cluster = dba.getCluster(null, {connectToPrimary:false});
// check the shell is connected to where we expect
shell.status();
// check the cluster is connected to where we expect
cluster.status();

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
testutil.callMysqlsh([__sandbox_uri1, "--py", "-e", "print(cluster.status())", "--cluster"]);

//@ SP - Connect with --cluster on a non-cluster member + cmd (error)
testutil.callMysqlsh([__sandbox_uri3, "--js", "-e", "println(cluster.status())", "--cluster"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ SP - Connect with --cluster on a non-cluster member interactive (error)
testutil.callMysqlsh([__sandbox_uri3, "--js", "--interactive", "-e", "println(cluster.status())", "--cluster"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ SP - Connect with --cluster on a non-cluster member (error)
testutil.callMysqlsh([__sandbox_uri3, "--js", "--cluster"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ SP - Connect with --replicaset, expect error
testutil.callMysqlsh([__sandbox_uri1, "--js", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SP - Connect with --cluster + --redirect-primary 1
testutil.callMysqlsh([__sandbox_uri1, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SP - Connect with --cluster + --redirect-primary 2
testutil.callMysqlsh([__sandbox_uri2, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SP - Connect with --cluster + --redirect-secondary 1
testutil.callMysqlsh([__sandbox_uri1, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ SP - Connect with --cluster + --redirect-secondary 2
testutil.callMysqlsh([__sandbox_uri2, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ SP - Connect with --replicaset + --redirect-primary 1, expect error
testutil.callMysqlsh([__sandbox_uri1, "--js", "--redirect-primary", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SP - Connect with --replicaset + --redirect-primary 2, expect error
testutil.callMysqlsh([__sandbox_uri2, "--js", "--redirect-primary", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SP - Connect with --replicaset + --redirect-secondary 1, expect error
testutil.callMysqlsh([__sandbox_uri1, "--js", "--redirect-secondary", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SP - Connect with --replicaset + --redirect-secondary 2, expect error
testutil.callMysqlsh([__sandbox_uri2, "--js", "--redirect-secondary", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SPX - Connect with --cluster 1
testutil.callMysqlsh([__sandbox_xuri1, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster 2
testutil.callMysqlsh([__sandbox_xuri2, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster py
testutil.callMysqlsh([__sandbox_xuri2, "--py", "-e", "print(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster on a non-cluster member (error)
testutil.callMysqlsh([__sandbox_xuri3, "--js", "-e", "println(cluster.status())", "--cluster"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ SPX - Connect with --replicaset, expect error [USE: SP - Connect with --replicaset, expect error]
testutil.callMysqlsh([__sandbox_xuri1, "--js", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SPX - Connect with --cluster + --redirect-primary 1
testutil.callMysqlsh([__sandbox_xuri1, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster + --redirect-primary 2
testutil.callMysqlsh([__sandbox_xuri2, "--js", "--redirect-primary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster + --redirect-secondary 1
testutil.callMysqlsh([__sandbox_xuri1, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --cluster + --redirect-secondary 2
testutil.callMysqlsh([__sandbox_xuri2, "--js", "--redirect-secondary", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX - Connect with --replicaset + --redirect-primary 1, expect error [USE: SP - Connect with --replicaset + --redirect-primary 1, expect error]
testutil.callMysqlsh([__sandbox_xuri1, "--js", "--redirect-primary", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SPX - Connect with --replicaset + --redirect-primary 2, expect error
testutil.callMysqlsh([__sandbox_xuri2, "--js", "--redirect-primary", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SPX - Connect with --replicaset + --redirect-secondary 1, expect error
testutil.callMysqlsh([__sandbox_xuri1, "--js", "--redirect-secondary", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SPX - Connect with --replicaset + --redirect-secondary 2, expect error [USE: SP - Connect with --replicaset + --redirect-secondary 2, expect error]
testutil.callMysqlsh([__sandbox_xuri2, "--js", "--redirect-secondary", "-e", "println(replicaset.status())", "--replicaset"]);

//@ SPX implicit - Connect with --cluster 1
testutil.callMysqlsh([__sandbox_xuri1_, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX implicit - Connect with --cluster 2
testutil.callMysqlsh([__sandbox_xuri2_, "--js", "-e", "println(cluster.status())", "--cluster"]);

//@ SPX implicit - Connect with --cluster py
testutil.callMysqlsh([__sandbox_xuri2_, "--py", "-e", "print(cluster.status())", "--cluster"]);

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

//@<> SP - getCluster() on session to secondary with primary gone but still reported as primary - auto-redirect to secondary
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberTransactions(__mysql_sandbox_port3);

shell.connect(__sandbox_uri2);
testutil.killSandbox(__mysql_sandbox_port1);

EXPECT_NO_THROWS(function() { cluster = dba.getCluster(); });
EXPECT_OUTPUT_CONTAINS("WARNING: Error connecting to Cluster: MYSQLSH 51004: Unable to connect to the primary member of the Cluster: 'Can't connect to MySQL server on");
EXPECT_OUTPUT_CONTAINS("Retrying getCluster() using a secondary member");

testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING),UNREACHABLE");

//@ SP - getCluster() on session to secondary with primary failover not complete - auto-redirect to secondary {VER(>=8.0)}
var cluster = dba.getCluster();
EXPECT_OUTPUT_CONTAINS("WARNING: Error connecting to Cluster: MYSQLSH 51004: Unable to find a primary member in the Cluster");
EXPECT_OUTPUT_CONTAINS("Retrying getCluster() using a secondary member");

shell.status();
cluster.status();

//@ SP - Dissolve the single-primary cluster while still connected to a secondary
testutil.startSandbox(__mysql_sandbox_port1);

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

if (__version_num < 80000) {
  cluster.rejoinInstance(__sandbox_uri1);
}

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

cluster.disconnect();

session.runSql("select * from performance_schema.replication_group_members");
cluster = dba.getCluster();
cluster.removeInstance({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
// dissolve will drop the metadata from sandbox1
cluster.dissolve({force:true});
// this should drop metadata from sandbox2 (where we're connected to)
//TODO(.) broken dba.dropMetadataSchema({force:true});
session.runSql("set sql_log_bin=0");
session.runSql("set global super_read_only = 0");
session.runSql("drop schema mysql_innodb_cluster_metadata");
session.runSql("RESET " + get_reset_binary_logs_keyword());
session.runSql("set sql_log_bin=1");

//@ Finalization
// This should now disconnect the cluster, even after dissolved
cluster.disconnect();
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
