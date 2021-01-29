//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname});
// testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port6, "root", {report_host:hostname});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
// session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);
session6 = mysql.getSession(__sandbox_uri6);

// prepare a 2/2/1 clusterset
shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1, manualStartOnBoot:1});

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "cluster2", {manualStartOnBoot:1});
c3 = cs.createReplicaCluster(__sandbox_uri6, "cluster3", {manualStartOnBoot:1});

c1.addInstance(__sandbox_uri2);
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

//@<> PC partial
s = cs.status();
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(__address1h, s["globalPrimaryInstance"]);

EXPECT_EQ(__address1h, cluster1(s)["primary"]);
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

EXPECT_EQ("OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);

//@<> PC NO_QUORUM
shell.connect(__sandbox_uri1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");

// disable connect retries
session4.runSql("stop replica for channel 'clusterset_replication'");
session4.runSql("change replication source to source_connect_retry=1 for channel 'clusterset_replication'");
session4.runSql("start replica for channel 'clusterset_replication'");
session6.runSql("stop replica for channel 'clusterset_replication'");
session6.runSql("change replication source to source_connect_retry=1 for channel 'clusterset_replication'");
session6.runSql("start replica for channel 'clusterset_replication'");

testutil.waitForReplConnectionError(__mysql_sandbox_port4, "clusterset_replication");
testutil.waitForReplConnectionError(__mysql_sandbox_port6, "clusterset_replication");

s = cs.status();
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ(null, s["globalPrimaryInstance"]);

EXPECT_EQ(null, cluster1(s)["primary"]);
EXPECT_EQ("NOT_OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("NO_QUORUM", cluster1(s)["status"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);

s = cs.status({extended:1});
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ(null, s["globalPrimaryInstance"]);

EXPECT_EQ(null, cluster1(s)["primary"]);
EXPECT_EQ("NOT_OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("NO_QUORUM", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_LT(0, cluster1(s)["transactionSet"].length);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("ERROR", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_LT(0, cluster2(s)["transactionSet"].length);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_LT(0, cluster3(s)["transactionSet"].length);

//@<> Both PC and RC NO_QUORUM
testutil.killSandbox(__mysql_sandbox_port5);

shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port5, "UNREACHABLE");

// get cs from the NO_QUORUM primary
shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

// cluster1 is NO_QUORUM with just sb2 ONLINE, cluster2 is NO_QUORUM with just sb4 ONLINE and cluster3 is OK
s = cs.status();
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ(null, s["globalPrimaryInstance"]);

EXPECT_EQ(null, cluster1(s)["primary"]);
EXPECT_EQ("NOT_OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Could not find ONLINE members forming a quorum. Cluster will be unable to perform updates until it's restored."], cluster1(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
// replicationStatus will take a while to switch from OK to ERROR
// EXPECT_EQ("ERROR", cluster2(s)["clusterSetReplicationStatus"]);
// EXPECT_EQ(["ERROR: Could not find ONLINE members forming a quorum. Cluster will be unable to perform updates until it's restored.", "WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
// EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
// EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster3(s)["clusterErrors"]);

s = cs.status({extended:1});
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ(null, s["globalPrimaryInstance"]);

EXPECT_EQ(null, cluster1(s)["primary"]);
EXPECT_EQ("NOT_OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("NO_QUORUM", cluster1(s)["status"]);
EXPECT_EQ(["ERROR: Could not find ONLINE members forming a quorum. Cluster will be unable to perform updates until it's restored."], cluster1(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("NO_QUORUM", cluster2(s)["status"]);
EXPECT_EQ(["ERROR: Could not find ONLINE members forming a quorum. Cluster will be unable to perform updates until it's restored.", "WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster3(s)["clusterErrors"]);

//@<> PC UNREACHABLE, 1xNO_QUORUM RC
testutil.stopSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri4);
cs = dba.getClusterSet();

s = cs.status();
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ(null, s["globalPrimaryInstance"]);

EXPECT_EQ(null, cluster1(s)["primary"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["globalStatus"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Could not connect to any ONLINE members but there are unreachable instances that could still be ONLINE."], cluster1(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster2(s)["transactionSetConsistencyStatus"]);
EXPECT_EQ(["ERROR: Could not find ONLINE members forming a quorum. Cluster will be unable to perform updates until it's restored.", "WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster3(s)["transactionSetConsistencyStatus"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster3(s)["clusterErrors"]);

s = cs.status({extended:1});
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["globalStatus"]);
EXPECT_EQ("UNREACHABLE", cluster1(s)["status"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(null, cluster1(s)["transactionSet"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("NO_QUORUM", cluster2(s)["status"]);
EXPECT_EQ("ERROR", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_LT(0, cluster2(s)["transactionSet"].length);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);
EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_LT(0, cluster3(s)["transactionSet"].length);

//@<> Both PC and RC UNREACHABLE
testutil.stopSandbox(__mysql_sandbox_port4);

// from the remaining ONLINE cluster
shell.connect(__sandbox_uri6);
cs = dba.getClusterSet();

s = cs.status();
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ(null, s["globalPrimaryInstance"]);

EXPECT_EQ(null, cluster1(s)["primary"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["globalStatus"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Could not connect to any ONLINE members but there are unreachable instances that could still be ONLINE."], cluster1(s)["clusterErrors"]);

EXPECT_EQ("UNKNOWN", cluster2(s)["globalStatus"]);
EXPECT_EQ("UNKNOWN", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Could not connect to any ONLINE members but there are unreachable instances that could still be ONLINE."], cluster2(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster3(s)["clusterErrors"]);

s = cs.status({extended:1});
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["globalStatus"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("UNREACHABLE", cluster1(s)["status"]);

EXPECT_EQ("UNKNOWN", cluster2(s)["globalStatus"]);
EXPECT_EQ("UNKNOWN", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("UNREACHABLE", cluster2(s)["status"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);


//@<> PC OFFLINE with 1 UNREACHABLE member -> UNKNOWN
testutil.startSandbox(__mysql_sandbox_port1);

session6.runSql("stop replica for channel 'clusterset_replication'");
session6.runSql("change replication source to source_connect_retry=1 for channel 'clusterset_replication'");
session6.runSql("start replica for channel 'clusterset_replication'");

testutil.waitForReplConnectionError(__mysql_sandbox_port6, "clusterset_replication");

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

s = cs.status();
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ(null, s["globalPrimaryInstance"]);

EXPECT_EQ(null, cluster1(s)["primary"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Could not connect to any ONLINE members but there are unreachable instances that could still be ONLINE."], cluster1(s)["clusterErrors"]);

EXPECT_EQ("UNKNOWN", cluster2(s)["globalStatus"]);
EXPECT_EQ("UNKNOWN", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Could not connect to any ONLINE members but there are unreachable instances that could still be ONLINE."], cluster2(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster3(s)["clusterErrors"]);

testutil.waitForReplConnectionError(__mysql_sandbox_port6, "clusterset_replication");

s = cs.status({extended:1});
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ(null, s["globalPrimaryInstance"]);

EXPECT_EQ(null, cluster1(s)["primary"]);
EXPECT_EQ("UNKNOWN", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("UNREACHABLE", cluster1(s)["status"]);

EXPECT_EQ("UNKNOWN", cluster2(s)["globalStatus"]);
EXPECT_EQ("UNKNOWN", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("UNREACHABLE", cluster2(s)["status"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
// EXPECT_EQ("ERROR", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);



//@<> Both PC and RC OFFLINE
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port5);

// from the remaining ONLINE cluster
shell.connect(__sandbox_uri6);
cs = dba.getClusterSet();

s = cs.status();
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ("NOT_OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Cluster members are reachable but they're all OFFLINE."], cluster1(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Cluster members are reachable but they're all OFFLINE.", "WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster3(s)["clusterErrors"]);

s = cs.status({extended:1});
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ("NOT_OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OFFLINE", cluster1(s)["status"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OFFLINE", cluster2(s)["status"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
EXPECT_EQ("OK", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster3(s)["status"]);

//@<> All clusters OFFLINE or unreachable
session6.runSql("stop group_replication");

// sb6 is the only member still running, although OFFLINE
shell.connect(__sandbox_uri6);
cs = dba.getClusterSet();

s = cs.status();
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ("NOT_OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Cluster members are reachable but they\'re all OFFLINE."], cluster1(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Cluster members are reachable but they\'re all OFFLINE.", "WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
// EXPECT_EQ("STOPPED", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Cluster members are reachable but they're all OFFLINE.", "WARNING: Replication from the Primary Cluster not in expected state"], cluster3(s)["clusterErrors"]);

s = cs.status({extended:1});
EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ("NOT_OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OFFLINE", cluster1(s)["status"]);

EXPECT_EQ("NOT_OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OFFLINE", cluster2(s)["status"]);

EXPECT_EQ("NOT_OK", cluster3(s)["globalStatus"]);
// EXPECT_EQ("STOPPED", cluster3(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OFFLINE", cluster3(s)["status"]);

//@# describe when unavailable
cs.describe();

//@# describe when unreachable
testutil.stopSandbox(__mysql_sandbox_port6);

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

cs.describe();

//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
// testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
