//@ {VER(>=8.0.29)}

// Plain ClusterSet test, use as a template for other tests that check
// a specific feature/aspect across the whole API

// also checks disconnect()

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname,log_error_verbosity:3});

testutil.deployRawSandbox(__mysql_sandbox_port4, 'root',
    {
        report_host: hostname,
        "log-error-verbosity": "3"
    });

shell.options.useWizards = false;

//@<> configureInstances
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri4, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });

__sandbox_uri1="mysql://admin:bla@localhost:"+__mysql_sandbox_port1;
__sandbox_uri2="mysql://admin:bla@localhost:"+__mysql_sandbox_port2;
__sandbox_uri3="mysql://admin:bla@localhost:"+__mysql_sandbox_port3;
__sandbox_uri4="mysql://admin:bla@localhost:"+__mysql_sandbox_port4;

testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port3);
testutil.restartSandbox(__mysql_sandbox_port4);

//
session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri4);

function get_clients(s, ignore_ids) {
  var ignore_ids = "connection_id(),"+ignore_ids;
  if (shell.getSession().uri == s.uri)
    ignore_ids += ","+shell.getSession().runSql("select connection_id()").fetchOne()[0];
  return s.runSql("select * from performance_schema.threads where type = 'FOREGROUND' and processlist_user = 'root' and name = 'thread/sql/one_connection' and processlist_id not in ("+ignore_ids+") order by processlist_id").fetchAll();
}

var existing_sessions = {};
function BEGIN_CHECK_NO_ZOMBIES() {
  existing_sessions[__mysql_sandbox_port1] = get_clients(session1, session1.runSql("select connection_id()").fetchOne()[0]);
  existing_sessions[__mysql_sandbox_port2] = get_clients(session2, session2.runSql("select connection_id()").fetchOne()[0]);
  existing_sessions[__mysql_sandbox_port3] = get_clients(session3, session3.runSql("select connection_id()").fetchOne()[0]);
  existing_sessions[__mysql_sandbox_port4] = get_clients(session4, session4.runSql("select connection_id()").fetchOne()[0]);
}

function END_CHECK_NO_ZOMBIES(cs) {
  cs.disconnect();

  function filter(l) {
    var out = [];
    for (row of l) {
      out.push(row["PROCESSLIST_ID"]);
    }
    return out;
  }

  function wait_get_clients(session, expected, ignore_ids) {
    for (i =0; i < 10; i++) {
      plist = get_clients(session, ignore_ids);
      if (filter(plist) == expected)
        break;
      os.sleep(0.5);
    }
    return plist;
  }

  function EXPECT_PROCESSLIST(session, sandbox) {
    var expected = filter(existing_sessions[sandbox]);
    plist = wait_get_clients(session, expected, session.runSql("select connection_id()").fetchOne()[0]);
    EXPECT_EQ(expected, filter(plist), "sandbox"+sandbox+": "+JSON.stringify(plist));
  }

  EXPECT_PROCESSLIST(session1, __mysql_sandbox_port1);
  EXPECT_PROCESSLIST(session2, __mysql_sandbox_port2);
  EXPECT_PROCESSLIST(session3, __mysql_sandbox_port3);
  EXPECT_PROCESSLIST(session4, __mysql_sandbox_port4);
}

//@<> create Primary Cluster
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", {"communicationStack": "MYSQL"}); });

//@<> createClusterSet
BEGIN_CHECK_NO_ZOMBIES();

EXPECT_NO_THROWS(function() {cs = cluster.createClusterSet("clusterset"); });

EXPECT_STDOUT_CONTAINS_MULTILINE(`
A new ClusterSet will be created based on the Cluster 'cluster'.

* Validating Cluster 'cluster' for ClusterSet compliance.

* Creating InnoDB ClusterSet 'clusterset' on 'cluster'...

* Updating metadata...

ClusterSet successfully created. Use ClusterSet.createReplicaCluster() to add Replica Clusters to it.
`);

END_CHECK_NO_ZOMBIES(cs);

//@<> validate primary cluster
CHECK_PRIMARY_CLUSTER([__sandbox_uri1], cluster)

//@<> dba.getClusterSet()

BEGIN_CHECK_NO_ZOMBIES();

var clusterset;
EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_NE(clusterset, null);

END_CHECK_NO_ZOMBIES(clusterset);

//@<> cluster.getClusterSet()

BEGIN_CHECK_NO_ZOMBIES();

var cs;
EXPECT_NO_THROWS(function() {cs = cluster.getClusterSet(); });
EXPECT_NE(cs, null);

END_CHECK_NO_ZOMBIES(cs);

//@<> status()

BEGIN_CHECK_NO_ZOMBIES();

cs = dba.getClusterSet();
cs.status();

END_CHECK_NO_ZOMBIES(cs);

//@<> status(1)

BEGIN_CHECK_NO_ZOMBIES();

cs = dba.getClusterSet();
cs.status({extended:1});

END_CHECK_NO_ZOMBIES(cs);

//@<> status(2)

BEGIN_CHECK_NO_ZOMBIES();

cs = dba.getClusterSet();
cs.status({extended:2});

END_CHECK_NO_ZOMBIES(cs);

//@<> status(3)

BEGIN_CHECK_NO_ZOMBIES();

cs = dba.getClusterSet();
cs.status({extended:3});

END_CHECK_NO_ZOMBIES(cs);

//@<> describe()
BEGIN_CHECK_NO_ZOMBIES();

cs = dba.getClusterSet();
cs.describe();

END_CHECK_NO_ZOMBIES(cs);

//@<> createReplicaCluster() - incremental recovery
// SRO might be set in the instance so we must ensure the proper handling of it in createReplicaCluster.
session4.runSql("SET PERSIST super_read_only=true");
var replicacluster;

BEGIN_CHECK_NO_ZOMBIES();

clusterset = dba.getClusterSet();

EXPECT_NO_THROWS(function() {replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "incremental", "communicationStack": "MYSQL"}); });

EXPECT_STDOUT_CONTAINS_MULTILINE(`
Setting up replica 'replicacluster' of cluster 'cluster' at instance '${hostname}:${__mysql_sandbox_port4}'.

A new InnoDB Cluster will be created on instance '${hostname}:${__mysql_sandbox_port4}'.

Disabling super_read_only mode on instance '${hostname}:${__mysql_sandbox_port4}'.
Validating instance configuration at localhost:${__mysql_sandbox_port4}...

This instance reports its own address as ${hostname}:${__mysql_sandbox_port4}

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '${hostname}:${__mysql_sandbox_port4}'. Use the localAddress option to override.


* Checking transaction state of the instance...

NOTE: The target instance '${hostname}:${__mysql_sandbox_port4}' has not been pre-provisioned (GTID set is empty). The Shell is unable to decide whether replication can completely recover its state.

Incremental state recovery selected through the recoveryMethod option

Creating InnoDB Cluster 'replicacluster' on '${hostname}:${__mysql_sandbox_port4}'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

* Configuring ClusterSet managed replication channel...
** Changing replication source of ${hostname}:${__mysql_sandbox_port4} to ${hostname}:${__mysql_sandbox_port1}

* Waiting for instance '${hostname}:${__mysql_sandbox_port4}' to synchronize with PRIMARY Cluster...


* Updating topology

* Waiting for the Cluster to synchronize with the PRIMARY Cluster...



Replica Cluster 'replicacluster' successfully created on ClusterSet 'clusterset'.
`);

//@<> validate replica cluster - incremental recovery
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);

replicacluster.disconnect();

END_CHECK_NO_ZOMBIES(clusterset);

replicacluster = dba.getCluster("replicacluster");
clusterset = dba.getClusterSet();

// Test regular InnoDB Cluster operations on the PRIMARY Cluster:
//  - Add instance
//  - Remove instance
//  - Rejoin instance
//  - Rescan
//  - Reset recovery account
//  - Set instance option
//  - Set option
//  - Set primary instance
//  - Setup admin account
//  - Setup router account

shell.connect(__sandbox_uri1);
cluster = dba.getCluster();

//@<> addInstance on primary cluster
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"}); });
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri3, {recoveryMethod: "clone"}); });
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> rejoinInstance on a primary cluster
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP group_replication");
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri3); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> rescan() on a primary cluster
// delete sb3 from the metadata so that rescan picks it up
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port3]);

EXPECT_NO_THROWS(function() { cluster.rescan({addInstances: "auto"}); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> resetRecoveryAccountsPassword() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.resetRecoveryAccountsPassword(); });
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);

//@<> setInstanceOption() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25); });

//@<> setOption() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.setOption("memberWeight", 50); });

//@<> setupAdminAccount() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.setupAdminAccount("cadmin@'%'", {password:"boo"}); });

//@<> setupRouterAccount() on a primary cluster
EXPECT_NO_THROWS(function() { cluster.setupRouterAccount("router@'%'", {password:"boo"}); });

//@<> removeInstance on primary cluster
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri3); });
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], cluster)

//@<> addInstance on replica cluster
EXPECT_NO_THROWS(function() { replicacluster.addInstance(__sandbox_uri3, {recoveryMethod: "clone"}); });
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> rejoinInstance on a replica cluster
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP group_replication");
shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function() { replicacluster.rejoinInstance(__sandbox_uri3); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> rescan() on a replica cluster
// delete sb3 from the metadata so that rescan picks it up
shell.connect(__sandbox_uri1);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port3]);

EXPECT_NO_THROWS(function() { replicacluster.rescan({addInstances: "auto"}); });
shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> resetRecoveryAccountsPassword() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.resetRecoveryAccountsPassword(); });
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri3], cluster, replicacluster);

//@<> setInstanceOption() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.setInstanceOption(__sandbox_uri3, "memberWeight", 25); });

//@<> setOption() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.setOption("memberWeight", 50); });

//@<> setupAdminAccount() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.setupAdminAccount("cadminreplica@'%'", {password:"boo"}); });

//@<> setupRouterAccount() on a replica cluster
EXPECT_NO_THROWS(function() { replicacluster.setupRouterAccount("routerreplica@'%'", {password:"boo"}); });

//@<> removeInstance on replica cluster
EXPECT_NO_THROWS(function() { replicacluster.removeInstance(__sandbox_uri3); });
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);

//@<> removeCluster()
EXPECT_NO_THROWS(function() {clusterset.removeCluster("replicacluster"); });

EXPECT_STDOUT_CONTAINS_MULTILINE(`
The Cluster 'replicacluster' will be removed from the InnoDB ClusterSet.

* Waiting for the Cluster to synchronize with the PRIMARY Cluster...


* Updating topology
* Waiting for the Cluster to synchronize the Metadata updates with the PRIMARY Cluster...


* Stopping and deleting ClusterSet managed replication channel...
* Dissolving the Cluster...
* Reconciling internally generated GTIDs...
* Instance '${__endpoint4}' is attempting to leave the cluster...

The Cluster 'replicacluster' was removed from the ClusterSet.
`);

//@<> validate remove cluster
CHECK_REMOVED_CLUSTER([__sandbox_uri4], cluster, "replicacluster");

//@<> createReplicaCluster() - clone recovery
var allowlist = "127.0.0.1," + hostname_ip;
EXPECT_NO_THROWS(function() {replicacluster = clusterset.createReplicaCluster(__sandbox_uri4, "replicacluster", {recoveryMethod: "clone", "communicationStack": "XCOM", ipAllowlist: allowlist}); });
session4 = mysql.getSession(__sandbox_uri4);

//@<> validate replica cluster - clone recovery
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replicacluster);

//@<> setPrimaryCluster
shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

cs.status({extended:1});

cs.setPrimaryCluster("replicacluster");

//@<> forcePrimaryCluster
session4.runSql("shutdown");

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();

cs.forcePrimaryCluster("cluster");

//@<> rejoinCluster
testutil.startSandbox(__mysql_sandbox_port4);
session4 = mysql.getSession(__sandbox_uri4);
shell.connect(__sandbox_uri4);
dba.rebootClusterFromCompleteOutage();

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();
cs.rejoinCluster("replicacluster");

//@<> disconnect
cs.disconnect();

EXPECT_THROWS(function(){cs.createReplicaCluster(__sandbox_uri4, "xx")}, "The ClusterSet object is disconnected. Please use dba.getClusterSet() to obtain a fresh handle.");
EXPECT_THROWS(function(){cs.removeCluster("xx")}, "The ClusterSet object is disconnected. Please use dba.getClusterSet() to obtain a fresh handle.");
EXPECT_THROWS(function(){cs.status()}, "The ClusterSet object is disconnected. Please use dba.getClusterSet() to obtain a fresh handle.");
EXPECT_THROWS(function(){cs.describe()}, "The ClusterSet object is disconnected. Please use dba.getClusterSet() to obtain a fresh handle.");
EXPECT_THROWS(function(){cs.setPrimaryCluster("xx")}, "The ClusterSet object is disconnected. Please use dba.getClusterSet() to obtain a fresh handle.");
EXPECT_THROWS(function(){cs.forcePrimaryCluster("xx")}, "The ClusterSet object is disconnected. Please use dba.getClusterSet() to obtain a fresh handle.");
EXPECT_EQ("clusterset", cs.getName());
EXPECT_NO_THROWS(function(){cs.disconnect()});

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
