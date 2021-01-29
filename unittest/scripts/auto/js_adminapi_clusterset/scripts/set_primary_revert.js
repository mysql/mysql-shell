//@ {VER(>=8.0.27)}

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host:hostname});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);

session1.runSql("create user admin@'%'");
session1.runSql("grant all on *.* to admin@'%' with grant option");
session1.runSql("revoke super on *.* from admin@'%'");

session1.runSql("create schema test");
session1.runSql("create table test.t1 (k int primary key)");

shell.connect("admin:@localhost:"+__mysql_sandbox_port1);
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1});
c1.addInstance(__sandbox_uri2);

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri3, "cluster2");
c2.addInstance(__sandbox_uri4);

c3 = cs.createReplicaCluster(__sandbox_uri5, "cluster3");

function execute_after(session, delay, sql) {
  session.runSql("create event ev1 on schedule every ? second on completion not preserve disable on slave do "+sql, [delay]);
}

//@<> INCLUDE clusterset_utils.inc

//@<> switch to cluster2, timeout during metadata update

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);

// lock the clusterset_views table to force a timeout while updating it
session1.runSql("lock tables mysql_innodb_cluster_metadata.clusterset_views read");

// Expected outcome:
// 1 - view change trxs get injected (no need to revert)
// 2 - password of primary cluster gets changed (can't revert, but no need either)
// 3 - timeout during metadata change

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2", {timeout:1});}, "ClusterSet.setPrimaryCluster: Failed to execute query on Metadata server "+hostname+":"+__mysql_sandbox_port1+": Lock wait timeout exceeded; try restarting transaction");

session1.runSql("unlock tables");

// ensure nothing broke
status = cs.status();
EXPECT_EQ("HEALTHY", status["status"]);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);

//@<> switch to cluster2, fail during change master after MD update

// this causes change master to fail
session1.runSql("revoke replication_slave_admin on *.* from admin@'%'");

session1.runSql("select * from mysql_innodb_cluster_metadata.v2_cs_members");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2");}, "ClusterSet.setPrimaryCluster: "+hostname+":"+__mysql_sandbox_port2+": Access denied");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);

session1.runSql("grant replication_slave_admin on *.* to admin@'%'");

//@<> switch to cluster2, fail during ftwrl at primary

// this will cause FTWRL to freeze
session1.runSql("lock tables test.t1 read");

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2", {timeout:1});}, "ClusterSet.setPrimaryCluster: "+hostname+":"+__mysql_sandbox_port1+": Lock wait timeout exceeded; try restarting transaction");

session1.runSql("unlock tables");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);

//@<> switch to cluster2, fail during update of a replica cluster

session5.runSql("set global super_read_only=0");
session5.runSql("set sql_log_bin=0");
session5.runSql("revoke replication_slave_admin on *.* from admin@'%'");
session5.runSql("set sql_log_bin=1");
session5.runSql("set global super_read_only=1");

shell.options.verbose=3;
shell.options["dba.logSql"]=1;

EXPECT_THROWS(function(){cs.setPrimaryCluster("cluster2");}, "ClusterSet.setPrimaryCluster: Could not update replication source of "+hostname+":"+__mysql_sandbox_port5);

shell.options.verbose=0;

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri3, __sandbox_uri4], c1, c2);
CHECK_REPLICA_CLUSTER([__sandbox_uri5], c1, c3);

session5.runSql("set global super_read_only=0");
session5.runSql("set sql_log_bin=0");
session5.runSql("grant replication_slave_admin on *.* to   admin@'%'");
session5.runSql("set sql_log_bin=1");
session5.runSql("set global super_read_only=1");

WIPE_OUTPUT();

//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
