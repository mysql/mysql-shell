//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host:hostname});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri3);

// prepare a 2/2 clusterset
shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1});

cs = c1.createClusterSet("cs");
c2 = cs.createReplicaCluster(__sandbox_uri3, "cluster2");

c1.addInstance(__sandbox_uri2);
c2.addInstance(__sandbox_uri4);


function cluster1(status) {
  return json_find_key(status, "cluster1");
}

function cluster2(status) {
  return json_find_key(status, "cluster2");
}

function instance1(status) {
  return json_find_key(status, hostname+":"+__mysql_sandbox_port1);
}

function instance2(status) {
  return json_find_key(status, hostname+":"+__mysql_sandbox_port2);
}

function instance3(status) {
  return json_find_key(status, hostname+":"+__mysql_sandbox_port3);
}

function instance4(status) {
  return json_find_key(status, hostname+":"+__mysql_sandbox_port4);
}

//@<> check that extended 3 has configs properly shown

// defaults
var s = cs.status({extended:3});
EXPECT_EQ(3, cluster2(s)["clusterSetReplication"]["options"]["connectRetry"]);
EXPECT_EQ(0, cluster2(s)["clusterSetReplication"]["options"]["delay"]);
EXPECT_EQ(30, cluster2(s)["clusterSetReplication"]["options"]["heartbeatPeriod"]);
EXPECT_EQ(10, cluster2(s)["clusterSetReplication"]["options"]["retryCount"]);

// customized
session3.runSql("stop replica for channel 'clusterset_replication'");
session3.runSql("change replication source to source_connect_retry=1, source_retry_count=5 for channel 'clusterset_replicatiton'");
session3.runSql("start replica for channel 'clusterset_replication'");
var s = cs.status({extended:3});
EXPECT_EQ(3, cluster2(s)["clusterSetReplication"]["options"]["connectRetry"]);
EXPECT_EQ(0, cluster2(s)["clusterSetReplication"]["options"]["delay"]);
EXPECT_EQ(30, cluster2(s)["clusterSetReplication"]["options"]["heartbeatPeriod"]);
EXPECT_EQ(10, cluster2(s)["clusterSetReplication"]["options"]["retryCount"]);

//@<> Check missing transactions
session3.runSql("flush tables with read lock");
session1.runSql("create schema abcd");

s = cs.status();
EXPECT_EQ("HEALTHY", s["status"]);

EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["transactionSet"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster2(s)["transactionSetConsistencyStatus"]);
EXPECT_EQ(undefined, cluster2(s)["clusterErrors"]);
EXPECT_EQ(undefined, cluster2(s)["transactionSetErrantGtidSet"]);
EXPECT_EQ(undefined, cluster2(s)["transactionSetMissingGtidSet"]);

s = cs.status({extended:1});
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_NE(undefined, cluster1(s)["transactionSet"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("OK", cluster2(s)["transactionSetConsistencyStatus"]);
EXPECT_EQ("", cluster2(s)["transactionSetErrantGtidSet"]);
EXPECT_EQ("String", type(cluster2(s)["transactionSetMissingGtidSet"]));

session3.runSql("unlock tables");

//@<> Errant transactions
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);
errant = inject_errant_gtid(session3);

s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);

EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["transactionSet"]);

EXPECT_EQ("OK_NOT_CONSISTENT", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("INCONSISTENT", cluster2(s)["transactionSetConsistencyStatus"]);
EXPECT_EQ("There are 1 transactions that were executed in this instance that did not originate from the PRIMARY.", cluster2(s)["transactionSetConsistencyStatusText"]);
EXPECT_EQ(["ERROR: Errant transactions detected"], cluster2(s)["clusterErrors"]);
EXPECT_EQ(undefined, cluster2(s)["transactionSetErrantGtidSet"]);
EXPECT_EQ(undefined, cluster2(s)["transactionSetMissingGtidSet"])

s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_NE(undefined, cluster1(s)["transactionSet"]);

EXPECT_EQ("OK_NOT_CONSISTENT", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("INCONSISTENT", cluster2(s)["transactionSetConsistencyStatus"]);
EXPECT_EQ("There are 1 transactions that were executed in this instance that did not originate from the PRIMARY.", cluster2(s)["transactionSetConsistencyStatusText"]);
EXPECT_EQ(errant, cluster2(s)["transactionSetErrantGtidSet"]);
EXPECT_EQ("", cluster2(s)["transactionSetMissingGtidSet"]);

inject_empty_trx(session1, errant);

//@<> Bad repl channel at PRIMARY of PC (stopped)
session1.runSql("change replication source to source_host='localhost', source_port=?, source_user='root', source_password='root' for channel 'clusterset_replication'", [__mysql_sandbox_port3]);

s = cs.status();
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

var s = cs.status({extended:1});
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

//@<> Bad repl channel at PRIMARY of PC (running)
session1.runSql("start replica for channel 'clusterset_replication'");

s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK_MISCONFIGURED", cluster1(s)["globalStatus"]);
EXPECT_EQ("MISCONFIGURED", cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["WARNING: Unexpected replication channel 'clusterset_replication' at Primary Cluster"], cluster1(s)["clusterErrors"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster2(s)["clusterErrors"]);

var s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK_MISCONFIGURED", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ("MISCONFIGURED", cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_NE(undefined, json_find_key(cluster1(s), "clusterSetReplication"));
EXPECT_EQ(["WARNING: Unexpected replication channel 'clusterset_replication' at Primary Cluster"], cluster1(s)["clusterErrors"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_NE(undefined, json_find_key(cluster2(s), "clusterSetReplication"));
EXPECT_EQ(undefined, cluster2(s)["clusterErrors"]);

session1.runSql("stop replica for channel 'clusterset_replication'");
session1.runSql("reset replica all for channel 'clusterset_replication'");

//@<> Extra channel at PRIMARY of PC
session1.runSql("change replication source to source_host='localhost', source_port=?, source_user='root', source_password='root' for channel ''", [__mysql_sandbox_port3]);
session1.runSql("start replica for channel ''");

s = cs.status();
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

s = cs.status({extended:1});
EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["ERROR: Unrecognized replication channel '' found. Unmanaged repliication channels are not supported."], instance1(s)["instanceErrors"]);

EXPECT_EQ("OK", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("OK", cluster2(s)["clusterSetReplicationStatus"]);

session1.runSql("stop replica for channel ''");
session1.runSql("reset replica all for channel ''");

//@<> Channel stopped
session3.runSql("stop replica for channel 'clusterset_replication'");

s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK_NOT_REPLICATING", cluster2(s)["globalStatus"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

var s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterErrors"]);

EXPECT_EQ("OK_NOT_REPLICATING", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("m.Map", type(json_find_key(cluster2(s), "clusterSetReplication")));
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

session3.runSql("start replica for channel 'clusterset_replication'");

//@<> Channel io stopped
session3.runSql("stop replica io_thread for channel 'clusterset_replication'");

s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK_NOT_REPLICATING", cluster2(s)["globalStatus"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterErrors"]);

EXPECT_EQ("OK_NOT_REPLICATING", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("OFF", json_find_key(cluster2(s), "clusterSetReplication")["receiverStatus"]);
EXPECT_EQ("APPLYING", json_find_key(cluster2(s), "clusterSetReplication")["applierStatus"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

session3.runSql("start replica io_thread for channel 'clusterset_replication'");

//@<> Channel applier stopped

session3.runSql("stop replica sql_thread for channel 'clusterset_replication'");

s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK_NOT_REPLICATING", cluster2(s)["globalStatus"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterErrors"]);

EXPECT_EQ("OK_NOT_REPLICATING", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("STOPPED", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("ON", json_find_key(cluster2(s), "clusterSetReplication")["receiverStatus"]);
EXPECT_EQ("OFF", json_find_key(cluster2(s), "clusterSetReplication")["applierStatus"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

session3.runSql("start replica sql_thread for channel 'clusterset_replication'");

//@<> Channel error

user = session3.runSql("show replica status for channel 'clusterset_replication'").fetchOne()['Source_User'];

session3.runSql("stop replica for channel 'clusterset_replication'");
session3.runSql("change replication source to source_user='baduser' for channel 'clusterset_replication'");
session3.runSql("start replica for channel 'clusterset_replication'");

s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK_NOT_REPLICATING", cluster2(s)["globalStatus"]);
EXPECT_EQ("ERROR", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterErrors"]);

EXPECT_EQ("OK_NOT_REPLICATING", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("ERROR", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ("ERROR", json_find_key(cluster2(s), "clusterSetReplication")["receiverStatus"]);
EXPECT_EQ("APPLIED_ALL", json_find_key(cluster2(s), "clusterSetReplication")["applierStatus"]);
EXPECT_EQ(1045, json_find_key(cluster2(s), "clusterSetReplication")["receiverLastErrorNumber"]);
EXPECT_EQ(["WARNING: Replication from the Primary Cluster not in expected state"], cluster2(s)["clusterErrors"]);

session3.runSql("stop replica for channel 'clusterset_replication'");
session3.runSql("change replication source to source_user='baduser' for channel ?", [user]);
session3.runSql("start replica for channel 'clusterset_replication'");

//@<> Channel missing
session3.runSql("stop replica for channel 'clusterset_replication'");
session3.runSql("reset replica all for channel 'clusterset_replication'");

s = cs.status();
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);

EXPECT_EQ("OK_MISCONFIGURED", cluster2(s)["globalStatus"]);
EXPECT_EQ("MISSING", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(["WARNING: Replication channel from the Primary Cluster is missing"], cluster2(s)["clusterErrors"]);

var s = cs.status({extended:1});
EXPECT_EQ("AVAILABLE", s["status"]);
EXPECT_EQ("OK", cluster1(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster1(s)["status"]);
EXPECT_EQ(undefined, cluster1(s)["clusterSetReplicationStatus"]);
EXPECT_EQ(undefined, cluster1(s)["clusterErrors"]);

EXPECT_EQ("OK_MISCONFIGURED", cluster2(s)["globalStatus"]);
EXPECT_EQ("OK_NO_TOLERANCE", cluster2(s)["status"]);
EXPECT_EQ("MISSING", cluster2(s)["clusterSetReplicationStatus"]);
EXPECT_EQ({}, json_find_key(cluster2(s), "clusterSetReplication"));
EXPECT_EQ(["WARNING: Replication channel from the Primary Cluster is missing"], cluster2(s)["clusterErrors"]);


//@<> Destroy
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);