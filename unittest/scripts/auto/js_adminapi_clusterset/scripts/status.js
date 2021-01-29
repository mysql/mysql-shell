//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port6, "root", {report_host:hostname});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);
session6 = mysql.getSession(__sandbox_uri6);

// prepare a 1/2/1 clusterset
shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1, manualStartOnBoot:1});

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "cluster2", {manualStartOnBoot:1});
c3 = cs.createReplicaCluster(__sandbox_uri6, "cluster3", {manualStartOnBoot:1});

// c1.addInstance(__sandbox_uri2);
c2.addInstance(__sandbox_uri5);

function cluster1(status) {
  return json_find_key(status, "cluster1");
}

function cluster2(status) {
  return json_find_key(status, "cluster2");
}

function cluster3(status) {
  return json_find_key(status, "cluster3");
}

function instance1(status) {
  return json_find_key(status, __address1h);
}

function instance2(status) {
  return json_find_key(status, __address2h);
}

function instance3(status) {
  return json_find_key(status, __address3h);
}

function instance4(status) {
  return json_find_key(status, __address4h);
}

function instance5(status) {
  return json_find_key(status, __address5h);
}

function instance6(status) {
  return json_find_key(status, __address6h);
}

function delete_last_view(session) {
  session.runSql("begin");
  view_id = session.runSql("select max(view_id) from mysql_innodb_cluster_metadata.clusterset_views").fetchOne()[0];
  session.runSql("delete from mysql_innodb_cluster_metadata.clusterset_members where view_id=?", [view_id]);
  session.runSql("delete from mysql_innodb_cluster_metadata.clusterset_views where view_id=?", [view_id]);
  session.runSql("commit");
}


//@ Plain
cs.status();

EXPECT_THROWS(function(){cs.status(123)}, "");

//@# describe()
cs.describe();


// TODO(alfredo) enable after 12805
//@ after switchover {false}
cs.setPrimaryCluster("cluster2");
cs.status();
cs.setPrimaryCluster("cluster1");
cs.status();
c1.setPrimaryInstance(__sandbox_uri2);
cs.status();

c1.setPrimaryInstance(__sandbox_uri1);

//@ Extended 1

function CHECK_EXTENDED_1(s) {
EXPECT_EQ("PRIMARY", cluster1(s)["topology"][__address1h]["memberRole"]);
EXPECT_EQ("R/W", cluster1(s)["topology"][__address1h]["mode"]);
EXPECT_EQ("ONLINE", cluster1(s)["topology"][__address1h]["status"]);
EXPECT_NE(null, cluster1(s)["topology"][__address1h]["version"]);
EXPECT_EQ("String", type(cluster1(s)["transactionSet"]));
EXPECT_EQ(undefined, cluster1(s)["transactionSetConsistencyStatus"]);
EXPECT_EQ(undefined, cluster1(s)["transactionSetErrantGtidSet"]);
EXPECT_EQ(undefined, cluster1(s)["transactionSetMissingGtidSet"]);

EXPECT_EQ("PRIMARY", cluster2(s)["topology"][__address4h]["memberRole"]);
EXPECT_EQ("R/O", cluster2(s)["topology"][__address4h]["mode"]);
EXPECT_EQ("ONLINE", cluster2(s)["topology"][__address4h]["status"]);
EXPECT_NE(null, cluster2(s)["topology"][__address4h]["version"]);

EXPECT_EQ("SECONDARY", cluster2(s)["topology"][__address5h]["memberRole"]);
EXPECT_EQ("R/O", cluster2(s)["topology"][__address5h]["mode"]);
EXPECT_EQ("ONLINE", cluster2(s)["topology"][__address5h]["status"]);
EXPECT_NE(null, cluster2(s)["topology"][__address5h]["version"]);

EXPECT_EQ("String", type(cluster2(s)["transactionSet"]));
EXPECT_EQ("OK", cluster2(s)["transactionSetConsistencyStatus"]);
EXPECT_EQ("String", type(cluster2(s)["transactionSetErrantGtidSet"]));
EXPECT_EQ("String", type(cluster2(s)["transactionSetMissingGtidSet"]));

EXPECT_EQ(undefined, cluster1(s)["clusterSetReplication"]);

EXPECT_EQ(__address1h, cluster2(s)["clusterSetReplication"]["source"]);
EXPECT_EQ("APPLIED_ALL", cluster2(s)["clusterSetReplication"]["applierStatus"]);
EXPECT_EQ("ON", cluster2(s)["clusterSetReplication"]["receiverStatus"]);

EXPECT_EQ(__address1h, cluster3(s)["clusterSetReplication"]["source"]);
EXPECT_EQ("APPLIED_ALL", cluster3(s)["clusterSetReplication"]["applierStatus"]);
EXPECT_EQ("ON", cluster3(s)["clusterSetReplication"]["receiverStatus"]);
}

s = cs.status({extended:1});

CHECK_EXTENDED_1(s);

//@<> Extended 2
s = cs.status({extended:2});

CHECK_EXTENDED_1(s);

EXPECT_EQ("ONLINE", cluster1(s)["topology"][__address1h]["memberState"]);
EXPECT_EQ([], cluster1(s)["topology"][__address1h]["fenceSysVars"]);
EXPECT_EQ(36, cluster1(s)["topology"][__address1h]["memberId"].length);

EXPECT_EQ("ONLINE", cluster2(s)["topology"][__address4h]["memberState"]);
EXPECT_EQ(["read_only", "super_read_only"], cluster2(s)["topology"][__address4h]["fenceSysVars"]);
EXPECT_EQ(36, cluster2(s)["topology"][__address4h]["memberId"].length);

EXPECT_EQ("ONLINE", cluster2(s)["topology"][__address5h]["memberState"]);
EXPECT_EQ(["read_only", "super_read_only"], cluster2(s)["topology"][__address4h]["fenceSysVars"]);
EXPECT_EQ(36, cluster2(s)["topology"][__address4h]["memberId"].length);
EXPECT_EQ(15, cluster2(s)["clusterSetReplication"]["receiverTimeSinceLastMessage"].length);

//@<> Extended 3
s = cs.status({extended:3});

CHECK_EXTENDED_1(s);

EXPECT_EQ(undefined, cluster1(s)["clusterSetReplication"]);
EXPECT_EQ(3, cluster2(s)["clusterSetReplication"]["options"]["connectRetry"]);
EXPECT_EQ(0, cluster3(s)["clusterSetReplication"]["options"]["delay"]);
EXPECT_EQ(30, cluster3(s)["clusterSetReplication"]["options"]["heartbeatPeriod"]);
EXPECT_EQ(10, cluster3(s)["clusterSetReplication"]["options"]["retryCount"]);

EXPECT_EQ("HEALTHY", s["status"]);

// Issues in HEALTHY clusterset
// ----------------------------

//@<> Wrong SRO at PRIMARY of PC
// This isn't necessarily an error
session1.runSql("set global super_read_only=1");
s = cs.status();
EXPECT_EQ("HEALTHY", s["status"]);

EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

var s = cs.status({extended:1});
EXPECT_EQ("R/O", cluster1(s)["topology"][__address1h]["mode"]);
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);

session1.runSql("set global super_read_only=0");

//@<> Wrong SRO at SECONDARY of PC
c1.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

session2.runSql("set global super_read_only=0");

s = cs.status();
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

var s = cs.status({extended:1});
EXPECT_EQ("R/W", cluster1(s)["topology"][__address2h]["mode"]);
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);

EXPECT_EQ(["WARNING: Instance is NOT the global PRIMARY but super_read_only option is OFF. Errant transactions and inconsistencies may be accidentally introduced."], instance2(s)["instanceErrors"]);

session2.runSql("set global super_read_only=1");

c1.removeInstance(__sandbox_uri2);


//@<> Wrong SRO at PRIMARY of RC
session4.runSql("set global super_read_only=0");
s = cs.status();
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

var s = cs.status({extended:1});
EXPECT_EQ("R/W", cluster2(s)["topology"][__address4h]["mode"]);
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);

EXPECT_EQ(["WARNING: Instance is NOT the global PRIMARY but super_read_only option is OFF. Errant transactions and inconsistencies may be accidentally introduced."], instance4(s)["instanceErrors"]);

session4.runSql("set global super_read_only=1");

//@<> Wrong SRO at SECONDARY of RC
session5.runSql("set global super_read_only=0");
s = cs.status();
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

var s = cs.status({extended:1});
EXPECT_EQ("R/W", cluster2(s)["topology"][__address5h]["mode"]);
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);

EXPECT_EQ(["WARNING: Instance is NOT the global PRIMARY but super_read_only option is OFF. Errant transactions and inconsistencies may be accidentally introduced."], instance5(s)["instanceErrors"]);

session5.runSql("set global super_read_only=1");

// Cluster Statuses
// ----------------

//@<> RC partial OFFLINE
session5.runSql("stop group_replication");
s = cs.status();

EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

s = cs.status({extended:1});
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["NOTE: group_replication is stopped."], instance5(s)["instanceErrors"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

//@<> RC OFFLINE

session4.runSql("stop group_replication");
s = cs.status();

EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OFFLINE", cluster2(s)["status"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

//@<> RC NO_QUORUM

shell.connect(__sandbox_uri4);
c2 = dba.rebootClusterFromCompleteOutage();
testutil.waitMemberState(__mysql_sandbox_port4, "ONLINE");
ensure_cs_replication_channel_ready(__sandbox_uri4, __mysql_sandbox_port1);

c2.rejoinInstance(__sandbox_uri5);
testutil.waitMemberState(__mysql_sandbox_port5, "ONLINE");

testutil.killSandbox(__mysql_sandbox_port5);
testutil.waitMemberState(__mysql_sandbox_port5, "UNREACHABLE");

c2.status();

s = cs.status();

EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["status"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Could not find ONLINE members forming a quorum. Cluster will be unable to perform updates until it's restored."], cluster2(s)["clusterErrors"]);
EXPECT_EQ("NO_QUORUM", cluster2(s)["status"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster3(s)["status"]);

s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("NO_QUORUM", cluster2(s)["status"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_GT(instance5(s)["shellConnectError"].length, 0);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

//@<> RC UNREACHABLE
testutil.killSandbox(__mysql_sandbox_port4);

s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterErrors"]);
EXPECT_EQ(undefined, cluster1(s)["status"]);

EXPECT_EQ("UNKNOWN", cluster2(s)["globalStatus"]);
EXPECT_EQ("UNKNOWN", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Could not connect to any ONLINE members but there are unreachable instances that could still be ONLINE."], cluster2(s)["clusterErrors"]);
EXPECT_EQ("UNREACHABLE", cluster2(s)["status"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster3(s)["clusterErrors"]);
EXPECT_EQ(undefined, cluster3(s)["status"]);

s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("UNKNOWN", cluster2(s)["globalStatus"]);
EXPECT_EQ("UNREACHABLE", cluster2(s)["status"]);
EXPECT_EQ("UNKNOWN", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_GT(instance4(s)["shellConnectError"].length, 0);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

//@<> RC INVALIDATED (and unreachable)

// disable auto-start because cancelling it is slow
testutil.killSandbox(__mysql_sandbox_port4);
testutil.killSandbox(__mysql_sandbox_port5);

invalidate_cluster("cluster2", c1);

s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["status"]);

EXPECT_EQ("INVALIDATED", cluster2(s)["globalStatus"]);
EXPECT_EQ("UNKNOWN", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("UNREACHABLE", cluster2(s)["status"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster3(s)["status"]);

var s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("INVALIDATED", cluster2(s)["globalStatus"]);
EXPECT_EQ("UNREACHABLE", cluster2(s)["status"]);
EXPECT_EQ("UNKNOWN", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_GT(instance4(s)["shellConnectError"].length, 0);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

//@<> RC INVALIDATED
testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port5);
session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);

delete_last_view(session1);

shell.connect(__sandbox_uri4);
c2 = dba.rebootClusterFromCompleteOutage();
// XXX workaround for reboot not setting SRO
session.runSql("set global super_read_only=1"); 

// wait for MD to be replicated to the RC (otherwise rejoin may still think its INVALIDATED)
// maybe the precond check should be using the primary instead, tho
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

c2.rejoinInstance(__sandbox_uri5);
invalidate_cluster("cluster2", c1);


s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["status"]);

EXPECT_EQ("INVALIDATED", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["WARNING: Cluster was invalidated and must be either removed from the ClusterSet or rejoined"], cluster2(s)["clusterErrors"]);
EXPECT_EQ("INVALIDATED", cluster2(s)["status"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster3(s)["status"]);

var s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("INVALIDATED", cluster2(s)["globalStatus"]);
EXPECT_EQ("INVALIDATED", cluster2(s)["status"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

// ensure status via cluster.status() also shows INVALIDATED
shell.connect(__sandbox_uri4);
c = dba.getCluster();
s = c.status();
EXPECT_EQ("INVALIDATED", s["defaultReplicaSet"]["status"]);

//@<> from a member of the invalidated cluster
shell.connect(__sandbox_uri4);
dba.getClusterSet().status();

EXPECT_OUTPUT_CONTAINS("WARNING: Cluster 'cluster2' was INVALIDATED and must be removed from the ClusterSet.");

//@ describe with invalidated cluster
cs.describe();

delete_last_view(session1);

// Post Failover

// TODO(alfredo) - requires wl12805

//@<> from valid cluster {false}
cs.setPrimaryCluster("cluster3");

session6.runSql("shutdown");

cs.forcePrimaryCluster("cluster1");

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

cs.status();

cs.status({extended:1});

//@<> from valid cluster while invalid cluster is back {false}
testutil.startSandbox(__mysql_sandbox_port6);
shell.connect(__sandbox_uri6);
c3 = dba.rebootClusterFromCompleteOutage();

shell.connect(__sandbox_uri4);
cs = dba.getClusterSet();

cs.status();

cs.status({extended:1});

//@<> from the invalidated cluster {false}

shell.connect(__sandbox_uri6);
cs = dba.getClusterSet();

cs.status();

cs.status({extended:1});


//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
