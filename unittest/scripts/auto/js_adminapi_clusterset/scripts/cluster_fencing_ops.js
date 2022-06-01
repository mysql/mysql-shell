//@ {VER(>=8.0.27)}

// Positive and negative tests for:
//
//  - <Cluster>.fenceAllTraffic()
//  - <Cluster>.fenceWrites()
//  - <Cluster>.unfenceWrites()

function validate_fenced_all_traffic(uris) {
  for (var uri of uris) {
    var session = mysql.getSession(uri);
    EXPECT_EQ("OFFLINE", session.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0], uri+".group_replication_stopped");
    EXPECT_EQ(1, session.runSql("select @@super_read_only").fetchOne()[0], uri+".super_read_only");
    EXPECT_EQ(1, session.runSql("select @@offline_mode").fetchOne()[0], uri+".offline_mode");
    session.close();
  }
}

function validate_fenced_write_traffic(uris) {
  for (var uri of uris) {
    var session = mysql.getSession(uri);
    EXPECT_EQ(1, session.runSql("select @@super_read_only").fetchOne()[0], uri+".super_read_only");

    var res = session.runSql("select name, event, enabled, type, priority, error_handling from performance_schema.replication_group_member_actions");
    while (r = res.fetchOne()) {
      if ("mysql_disable_super_read_only_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.sro.event");
        EXPECT_EQ(0, r["enabled"], uri+".member_action.sro.enabled");
        EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.sro.type");
        EXPECT_EQ(1, r["priority"], uri+".member_action.sro.priority");
        EXPECT_EQ("IGNORE", r["error_handling"], uri+".member_action.sro.error_handling");
      }
    }

    session.close();
  }
}

function validate_unfenced(uris, primary_member) {
  for (var uri of uris) {
    var session = mysql.getSession(uri);

    if (uri == primary_member) {
      EXPECT_EQ(0, session.runSql("select @@super_read_only").fetchOne()[0], uri+".super_read_only");
    } else {
      EXPECT_EQ(1, session.runSql("select @@super_read_only").fetchOne()[0], uri+".super_read_only");
    }

    EXPECT_EQ(0, session.runSql("select @@offline_mode").fetchOne()[0], uri+".offline_mode");

    var res = session.runSql("select name, event, enabled, type, priority, error_handling from performance_schema.replication_group_member_actions");
    while (r = res.fetchOne()) {
      if ("mysql_disable_super_read_only_if_primary" == r["name"]) {
        EXPECT_EQ("AFTER_PRIMARY_ELECTION", r["event"], uri+".member_action.sro.event");
        EXPECT_EQ(1, r["enabled"], uri+".member_action.sro.enabled");
        EXPECT_EQ("INTERNAL", r["type"], uri+".member_action.sro.type");
        EXPECT_EQ(1, r["priority"], uri+".member_action.sro.priority");
        EXPECT_EQ("IGNORE", r["error_handling"], uri+".member_action.sro.error_handling");
      }
    }

    session.close();
  }
}

function cluster1(status) {
  return json_find_key(status, "cluster");
}

//@<> INCLUDE clusterset_utils.inc

//@<> Deploy and configure instances
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

//@<> create Admin accounts
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri4, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri5, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });

__sandbox_uri1="mysql://admin:bla@localhost:"+__mysql_sandbox_port1;
__sandbox_uri2="mysql://admin:bla@localhost:"+__mysql_sandbox_port2;
__sandbox_uri3="mysql://admin:bla@localhost:"+__mysql_sandbox_port3;
__sandbox_uri4="mysql://admin:bla@localhost:"+__mysql_sandbox_port4;
__sandbox_uri5="mysql://admin:bla@localhost:"+__mysql_sandbox_port5;

//@<> Create cluster
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", {gtidSetIsComplete:1}); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3); });

//@<> unfenceWrites() - Standalone Cluster (must fail)
EXPECT_THROWS_TYPE(function(){ cluster.unfenceWrites(); }, "This function is not available through a session to an instance that belongs to an InnoDB Cluster that is not a member of an InnoDB ClusterSet", "MYSQLSH");

//@<> fenceWrites() - Standalone Cluster (must fail)
EXPECT_THROWS_TYPE(function() { cluster.fenceWrites(); }, "This function is not available through a session to an instance that belongs to an InnoDB Cluster that is not a member of an InnoDB ClusterSet", "MYSQLSH");

//@<> fenceAllTraffic() - Standalone Cluster
EXPECT_NO_THROWS(function() { cluster.fenceAllTraffic(); });

EXPECT_STDOUT_CONTAINS_MULTILINE(`
The Cluster 'cluster' will be fenced from all traffic

* Enabling super_read_only on the primary '${__endpoint1}'...
* Enabling offline_mode on the primary '${__endpoint1}'...
* Enabling offline_mode on '${__endpoint2}'...
* Stopping Group Replication on '${__endpoint2}'...
* Enabling offline_mode on '${__endpoint3}'...
* Stopping Group Replication on '${__endpoint3}'...
* Stopping Group Replication on the primary '${__endpoint1}'...

Cluster successfully fenced from all traffic
`);

validate_fenced_all_traffic([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3]);

//@<> rebootClusterFromCompleteOutage() on a standalone fenced cluster from all traffic
shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("cluster"); });
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> createClusterSet()
EXPECT_NO_THROWS(function() { cs = cluster.createClusterSet("testCS"); });

//@<> unfenceWrites() - Primary Cluster (must fail)
EXPECT_THROWS_TYPE(function(){ cluster.unfenceWrites(); }, "Cluster not fenced to write traffic", "MYSQLSH");

//@<> fenceWrites() - Primary Cluster
EXPECT_NO_THROWS(function() { cluster.fenceWrites(); });

EXPECT_STDOUT_CONTAINS_MULTILINE(`
The Cluster 'cluster' will be fenced from write traffic

* Disabling automatic super_read_only management on the Cluster...
* Enabling super_read_only on '${__endpoint1}'...
* Enabling super_read_only on '${__endpoint2}'...
* Enabling super_read_only on '${__endpoint3}'...

NOTE: Applications will now be blocked from performing writes on Cluster 'cluster'. Use <Cluster>.unfenceWrites() to resume writes if you're certain a split-brain is not in effect.

Cluster successfully fenced from write traffic
`);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);
validate_fenced_write_traffic([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3]);

//@<> verify allowed ops primary cluster fenced to writes
// Cluster ops
EXPECT_NO_THROWS(function() { cluster.describe(); });
EXPECT_NO_THROWS(function() { cluster.options(); });
EXPECT_NO_THROWS(function() { cluster.status(); });
EXPECT_NO_THROWS(function() { dba.getCluster(); });
// ClusterSet ops
EXPECT_NO_THROWS(function() { cs.routingOptions(); });
EXPECT_NO_THROWS(function() { cs.listRouters(); });
EXPECT_NO_THROWS(function() { cs.status(); });
EXPECT_NO_THROWS(function() { cs.describe(); });
EXPECT_NO_THROWS(function() { dba.getClusterSet(); });

//@<> verify blocked ops primary cluster fenced to writes
// Cluster ops
EXPECT_THROWS_TYPE(function(){ cluster.addInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.checkInstanceState(__sandbox_uri4); }, "Unable to perform the operation on an InnoDB Cluster with status FENCED_WRITES", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.forceQuorumUsingPartitionOf(__sandbox_uri4); }, "Unable to perform the operation on an InnoDB Cluster with status FENCED_WRITES", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.rejoinInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.removeInstance(__sandbox_uri4); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.removeRouterMetadata("foobar"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.rescan(); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.resetRecoveryAccountsPassword(); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.setInstanceOption(__sandbox_uri4, "foo", "bar"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.setOption("foo", "bar"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.setPrimaryInstance(__sandbox_uri4); }, "Unable to perform the operation on an InnoDB Cluster with status FENCED_WRITES", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.setupAdminAccount("foo"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cluster.setupRouterAccount("foo"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
// ClusterSet ops
EXPECT_THROWS_TYPE(function(){ cs.createReplicaCluster(__sandbox_uri4, "foobar"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cs.removeCluster("foobar"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cs.rejoinCluster("foobar"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cs.setPrimaryCluster("foobar"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cs.forcePrimaryCluster("foobar"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");
EXPECT_THROWS_TYPE(function(){ cs.setRoutingOption("router::test", "option", "value"); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of OK_FENCED_WRITES within the ClusterSet. Operation is not possible when in that state", "RuntimeError");

//@<> status() on Primary Cluster fenced to writes
s = cluster.status();
EXPECT_EQ(s["defaultReplicaSet"]["status"], "FENCED_WRITES");
EXPECT_EQ(s["defaultReplicaSet"]["statusText"], "Cluster is fenced from Write Traffic.");
EXPECT_EQ(s["defaultReplicaSet"]["clusterErrors"][0], "WARNING: Cluster is fenced from Write traffic. Use cluster.unfenceWrites() to unfence the Cluster.");

//@<> clusterset.status() on Primary Cluster fenced to writes
s = cs.status({extended: 1});

EXPECT_EQ(cluster1(s)["globalStatus"], "OK_FENCED_WRITES", );
EXPECT_EQ(cluster1(s)["status"], "FENCED_WRITES", );

EXPECT_EQ("UNAVAILABLE", s["status"]);
EXPECT_EQ(s["statusText"], "Primary Cluster is fenced from write traffic.");

//@<> unfenceWrites() - Primary Cluster Cluster
EXPECT_NO_THROWS(function() { cluster.unfenceWrites(); });

EXPECT_STDOUT_CONTAINS_MULTILINE(`
The Cluster 'cluster' will be unfenced from write traffic

* Enabling automatic super_read_only management on the Cluster...
* Disabling super_read_only on the primary '${hostname}:${__mysql_sandbox_port1}'...

Cluster successfully unfenced from write traffic
`);

validate_unfenced([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], __sandbox_uri1);

s = cluster.status();
EXPECT_EQ(s["defaultReplicaSet"]["status"], "OK");

//@<> clusterset.status() on Primary Cluster just unfenced from writes
s = cs.status({extended: 1});

EXPECT_EQ(cluster1(s)["globalStatus"], "OK", );
EXPECT_EQ(cluster1(s)["status"], "OK", );

EXPECT_EQ("HEALTHY", s["status"]);
EXPECT_EQ(s["statusText"], "All Clusters available.");

//@<> fenceAllTraffic() - Primary Cluster
EXPECT_NO_THROWS(function() { cluster.fenceAllTraffic(); });

validate_fenced_all_traffic([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3]);

//@<> rebootClusterFromCompleteOutage() on a primary fenced cluster from all traffic
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("cluster"); });
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE", true);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE", true);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> Create a Replica Cluster
cs = dba.getClusterSet();
EXPECT_NO_THROWS(function() { replicacluster = cs.createReplicaCluster(__sandbox_uri4, "replica"); });
EXPECT_NO_THROWS(function() { replicacluster.addInstance(__sandbox_uri5, {recoveryMethod: "clone"}); });

//@<> fenceWrites on a Replica Cluster (must fail)
EXPECT_THROWS_TYPE(function(){replicacluster.fenceWrites()}, "The Cluster 'replica' is a REPLICA Cluster of the ClusterSet 'testCS'", "MYSQLSH");
EXPECT_STDOUT_CONTAINS("Unable to fence Cluster from write traffic: operation not permitted on REPLICA Clusters");

//@<> fenceAllTraffic() on a Replica Cluster
EXPECT_NO_THROWS(function() { replicacluster.fenceAllTraffic(); });

validate_fenced_all_traffic([__sandbox_uri4, __sandbox_uri5]);

//@<> rebootClusterFromCompleteOutage() on a fenced replica cluster from all traffic
shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function() { replicacluster = dba.rebootClusterFromCompleteOutage("replica"); });

CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], cluster, replicacluster);

// BUG#33551128: From fence Writes to fence All Traffic
// Fencing from All Traffic a Cluster that was Fenced to Write traffic
// was forbidden by the preconditions check. It must be possible to do
// that operation without first unfencing the Cluster since that implies
// possible unwanted writes during that period of time.

//@<> fenceAllTraffic() on a Primary Cluster fenced from Write traffic
EXPECT_NO_THROWS(function() { cluster.fenceWrites(); });

validate_fenced_write_traffic([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3]);

EXPECT_NO_THROWS(function() { cluster.fenceAllTraffic(); });

validate_fenced_all_traffic([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3]);

//@<> rebootClusterFromCompleteOutage() on a primary fenced cluster from all traffic that was previously fenced from write traffic only
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("cluster"); });

// The Cluster is still fenced to writes so it must be unfenced before rejoining the instances back to it
EXPECT_NO_THROWS(function() { cluster.unfenceWrites(); });

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
