//@{VER(>=8.0.27) && !__dbug_off}
//@<> Setup   
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {"report_host": hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {"report_host": hostname});

shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster1", {gtidSetIsComplete:1});
c1.addInstance(__sandbox_uri2);

cs = c1.createClusterSet("cs");
c2 = cs.createReplicaCluster(__sandbox_uri3, "cluster2");
c2.addInstance(__sandbox_uri4);

c3 = cs.createReplicaCluster(__sandbox_uri5, "cluster3");

//@<> ensure that rebooting a replica replicating from the wrong cluster doesn't get errants 
cs.setPrimaryCluster("cluster3");

// this will cause setPrimary to quit before it update replicas
testutil.dbugSet("+d,set_primary_cluster_skip_update_replicas");
cs.setPrimaryCluster("cluster2");

// reboot will introduce a view change as per bug#33120210
shell.connect(__sandbox_uri5);
session.runSql("stop group_replication");
dba.rebootClusterFromCompleteOutage();
testutil.waitMemberState(__mysql_sandbox_port5, "ONLINE");

testutil.waitMemberTransactions(__mysql_sandbox_port1, __mysql_sandbox_port5);

testutil.dbugSet("");

shell.connect(__sandbox_uri5);
status = cs.status({"extended": 2});

EXPECT_EQ("OK", status["clusters"]["cluster1"]["transactionSetConsistencyStatus"]);
EXPECT_EQ("OK_MISCONFIGURED", status["clusters"]["cluster1"]["globalStatus"]);
EXPECT_EQ(undefined, status["clusters"]["cluster2"]["transactionSetConsistencyStatus"]);
EXPECT_EQ("OK", status["clusters"]["cluster3"]["transactionSetConsistencyStatus"]);

// rejoin misconfigured cluster should fix it and also fix the errant trxs
cs.rejoinCluster("cluster1");

shell.connect(__sandbox_uri1);
wait_channel_ready(session, __mysql_sandbox_port3, "clusterset_replication");

status = cs.status({"extended": 2});

EXPECT_EQ("OK", status["clusters"]["cluster1"]["transactionSetConsistencyStatus"]);
EXPECT_EQ("OK", status["clusters"]["cluster1"]["globalStatus"]);
EXPECT_EQ(undefined, status["clusters"]["cluster2"]["transactionSetConsistencyStatus"]);
EXPECT_EQ("OK", status["clusters"]["cluster3"]["transactionSetConsistencyStatus"]);

//@<> Destroy  
testutil.dbugSet("");
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
