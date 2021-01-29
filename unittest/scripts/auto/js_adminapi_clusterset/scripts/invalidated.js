//@ {VER(>=8.0.27)}

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.deploySandbox(__mysql_sandbox_port4, "root");
testutil.deploySandbox(__mysql_sandbox_port5, "root");
testutil.deploySandbox(__mysql_sandbox_port6, "root");

shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1, manualStartOnBoot:1});

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri2, "cluster2", {manualStartOnBoot:1});
c3 = cs.createReplicaCluster(__sandbox_uri3, "cluster3", {manualStartOnBoot:1});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri4);
session5 = mysql.getSession(__sandbox_uri5);
session5 = mysql.getSession(__sandbox_uri6);

// failover from c1 to c2 then from c2 to c3
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

shell.connect(__sandbox_uri2);
cs = dba.getClusterSet();
cs.forcePrimaryCluster("cluster2");

c3 = dba.getCluster("cluster3");

// create c4 after the failover
c4 = cs.createReplicaCluster(__sandbox_uri4, "cluster4", {manualStartOnBoot:1});

// add sb5 to c3 after the failover
c3.addInstance(__sandbox_uri5);

testutil.stopSandbox(__mysql_sandbox_port2, {wait:1});

shell.connect(__sandbox_uri4);
cs = dba.getClusterSet();
cs.forcePrimaryCluster("cluster3");

cs.setPrimaryCluster("cluster4");

// c1=sb1, c2=sb2, c3=sb3,sb5, c4=sb4

// Topology as seen from cluster3 and cluster4:
//    cluster1=invalidated, cluster2=invalidated, cluster3=replica, cluster4=primary
// Topology as seen from cluster2:
//    cluster1=invalidated, cluster2=primary, cluster3=replica, cluster4=replica
// Topology as seen from cluster1:
//    cluster1=primary, cluster2=replica, cluster3=replica

testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);

cs.status();

//@<> getClusterSet() from c4
shell.connect(__sandbox_uri4);
c = dba.getClusterSet();

var s = c.status();

EXPECT_EQ("cluster4", s["primaryCluster"]);
EXPECT_EQ("INVALIDATED", s["clusters"]["cluster1"]["globalStatus"]);
EXPECT_EQ("INVALIDATED", s["clusters"]["cluster2"]["globalStatus"]);
EXPECT_EQ("OK", s["clusters"]["cluster3"]["globalStatus"]);
EXPECT_EQ("OK", s["clusters"]["cluster4"]["globalStatus"]);

//@<> getClusterSet() from c3
shell.connect(__sandbox_uri3);
c = dba.getClusterSet();

var s = c.status();

EXPECT_EQ("cluster4", s["primaryCluster"]);
EXPECT_EQ("INVALIDATED", s["clusters"]["cluster1"]["globalStatus"]);
EXPECT_EQ("INVALIDATED", s["clusters"]["cluster2"]["globalStatus"]);
EXPECT_EQ("OK", s["clusters"]["cluster3"]["globalStatus"]);
EXPECT_EQ("OK", s["clusters"]["cluster4"]["globalStatus"]);

//@<> getClusterSet() from c2
shell.connect(__sandbox_uri2);
c = dba.getClusterSet();

var s = c.status();

EXPECT_EQ("cluster4", s["primaryCluster"]);
EXPECT_EQ("INVALIDATED", s["clusters"]["cluster1"]["globalStatus"]);
EXPECT_EQ("INVALIDATED", s["clusters"]["cluster2"]["globalStatus"]);
EXPECT_EQ("OK", s["clusters"]["cluster3"]["globalStatus"]);
EXPECT_EQ("OK", s["clusters"]["cluster4"]["globalStatus"]);

//@<> getClusterSet() from c1
shell.connect(__sandbox_uri1);
c = dba.getClusterSet();

var s = c.status();

EXPECT_EQ("cluster4", s["primaryCluster"]);
EXPECT_EQ("INVALIDATED", s["clusters"]["cluster1"]["globalStatus"]);
EXPECT_EQ("INVALIDATED", s["clusters"]["cluster2"]["globalStatus"]);
EXPECT_EQ("OK", s["clusters"]["cluster3"]["globalStatus"]);
EXPECT_EQ("OK", s["clusters"]["cluster4"]["globalStatus"]);

//@<> getClusterSet() from c1 while cluster3 is down
// cluster4 doesn't exist for c1, but it should be discovered via c2


//@<> getClusterSet() from c1 while cluster3 and cluster4 are down
// should think that cluster2 is the primary and cluster1 is invalidated

// This is not ideal, but is the expected behaviour given the limitations.

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.stopSandbox(__mysql_sandbox_port4);
testutil.stopSandbox(__mysql_sandbox_port5);

shell.connect(__sandbox_uri1);
c = dba.getClusterSet();

var s = c.status();

// in reality it's cluster4, but there's no way to get that in this scenario
EXPECT_EQ("cluster2", s["primaryCluster"]);
EXPECT_EQ("INVALIDATED", s["clusters"]["cluster1"]["globalStatus"]);
EXPECT_EQ("OFFLINE", s["clusters"]["cluster2"]["status"]);
EXPECT_EQ("UNREACHABLE", s["clusters"]["cluster3"]["status"]);
EXPECT_EQ("UNREACHABLE", s["clusters"]["cluster4"]["status"]);

//@<> getClusterSet() from c1 while cluster2, cluster3 and cluster4 are down
// should think that cluster1 is still primary

// This is not ideal, but is the expected behaviour given the limitations.

testutil.stopSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
c = dba.getClusterSet();

var s = c.status();

// in reality it's cluster4, but there's no way to get that in this scenario
EXPECT_EQ("cluster1", s["primaryCluster"]);
EXPECT_EQ("OFFLINE", s["clusters"]["cluster1"]["status"]);
EXPECT_EQ("UNREACHABLE", s["clusters"]["cluster2"]["status"]);
EXPECT_EQ("UNREACHABLE", s["clusters"]["cluster3"]["status"]);
EXPECT_EQ(undefined, s["clusters"]["cluster4"]);

//@<> remove cluster3 from clusterset
testutil.startSandbox(__mysql_sandbox_port5);
testutil.startSandbox(__mysql_sandbox_port3);
testutil.startSandbox(__mysql_sandbox_port4);

shell.connect(__sandbox_uri4);
c4 = dba.rebootClusterFromCompleteOutage();

shell.connect(__sandbox_uri3);
c3 = dba.rebootClusterFromCompleteOutage();

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
wait_channel_ready(session, __mysql_sandbox_port4, "clusterset_replication");

c3.status();
c3.rejoinInstance(__sandbox_uri5);

cs = dba.getClusterSet();
cs.removeCluster("cluster3");

shell.connect(__sandbox_uri4);
cs = dba.getClusterSet();

s = cs.status();

EXPECT_EQ("cluster4", s["primaryCluster"]);
EXPECT_EQ("OFFLINE", s["clusters"]["cluster1"]["status"]);
EXPECT_EQ("UNREACHABLE", s["clusters"]["cluster2"]["status"]);
EXPECT_EQ(undefined, s["clusters"]["cluster3"]);
EXPECT_EQ("OK", s["clusters"]["cluster4"]["globalStatus"]);

//@<> get clusterset from cluster1 while cluster2 down
// no way to get to cluster4
shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();
var s = cs.status();

EXPECT_EQ("cluster1", s["primaryCluster"]);
EXPECT_EQ("OFFLINE", s["clusters"]["cluster1"]["status"]);
EXPECT_EQ("UNREACHABLE", s["clusters"]["cluster2"]["status"]);
EXPECT_EQ("OK_NO_TOLERANCE", s["clusters"]["cluster3"]["status"]);
EXPECT_EQ(undefined, s["clusters"]["cluster4"]);

//@<> get clusterset from cluster1 while cluster2 up
// should reach cluster4 via cluster2 (even if it's not ONLINE)
testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

s = cs.status();

EXPECT_EQ("cluster4", s["primaryCluster"]);
EXPECT_EQ("OFFLINE", s["clusters"]["cluster1"]["status"]);
EXPECT_EQ("OFFLINE", s["clusters"]["cluster2"]["status"]);
EXPECT_EQ(undefined, s["clusters"]["cluster3"]);
EXPECT_EQ("OK", s["clusters"]["cluster4"]["globalStatus"]);

// XXX doesn't work because of bug in WL13815 (bug#33081064)
//@<> get clusterset from cluster1 while cluster2 up (but removed from cs) {false}
// should fail and be stuck to cluster1

cs.removeCluster("cluster2", {force:1});

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

s = cs.status();

EXPECT_EQ("cluster1", s["primaryCluster"]);
EXPECT_EQ("OFFLINE", s["clusters"]["cluster1"]["status"]);
EXPECT_EQ("UNREACHABLE", s["clusters"]["cluster2"]["status"]);
EXPECT_EQ("OK_NO_TOLERANCE", s["clusters"]["cluster3"]["status"]);
EXPECT_EQ(undefined, s["clusters"]["cluster4"]);


//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
