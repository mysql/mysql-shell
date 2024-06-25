//@ {VER(>=8.0.27)}

//@<> INCLUDE dba_utils.inc
//@<> INCLUDE clusterset_utils.inc

//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

//@<> Create clusterset
shell.connect(__sandbox_uri1);
c1 = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
c1.addInstance(__sandbox_uri2);
c1.addReplicaInstance(__sandbox_uri3);

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "replica");
c2.addInstance(__sandbox_uri5);

//@<> Validate arguments
EXPECT_THROWS(function(){ cs.dissolve(1); }, "Argument #1 is expected to be a map");
EXPECT_THROWS(function(){ cs.dissolve(1,2); }, "Invalid number of arguments, expected 0 to 1 but got 2");
EXPECT_THROWS(function(){ cs.dissolve(""); }, "Argument #1 is expected to be a map");
EXPECT_THROWS(function(){ cs.dissolve({foobar: true}); }, "Invalid options: foobar");
EXPECT_THROWS(function(){ cs.dissolve({force: "whatever"}); }, "Option 'force' Bool expected, but value is String");
EXPECT_THROWS(function(){ cs.dissolve({timeout: "foo"}); }, "Option 'timeout' Integer expected, but value is String");
EXPECT_THROWS(function(){ cs.dissolve({timeout: -2}); }, "Argument #1 timeout option must be >= 0");

//@<> Dissolve clusterset in interactive mode (cancel)
shell.options.useWizards=1;

testutil.expectPrompt("Are you sure you want to dissolve the ClusterSet?", "n");
EXPECT_THROWS(function() {
    cs.dissolve();
}, "Operation canceled by user.");

EXPECT_OUTPUT_CONTAINS("The ClusterSet has the following registered Clusters and instances:");
EXPECT_OUTPUT_CONTAINS_MULTILINE(`{
    "clusters": {
        "cluster": {
            "clusterRole": "PRIMARY",
            "topology": [
                {
                    "address": "${hostname}:${__mysql_sandbox_port1}",
                    "label": "${hostname}:${__mysql_sandbox_port1}",
                    "role": "HA"
                },
                {
                    "address": "${hostname}:${__mysql_sandbox_port2}",
                    "label": "${hostname}:${__mysql_sandbox_port2}",
                    "role": "HA"
                },
                {
                    "address": "${hostname}:${__mysql_sandbox_port3}",
                    "label": "${hostname}:${__mysql_sandbox_port3}",
                    "replicationSources": [
                        "PRIMARY"
                    ],
                    "role": "READ_REPLICA"
                }
            ]
        },
        "replica": {
            "clusterRole": "REPLICA",
            "topology": [
                {
                    "address": "${hostname}:${__mysql_sandbox_port4}",
                    "label": "${hostname}:${__mysql_sandbox_port4}",
                    "role": "HA"
                },
                {
                    "address": "${hostname}:${__mysql_sandbox_port5}",
                    "label": "${hostname}:${__mysql_sandbox_port5}",
                    "role": "HA"
                }
            ]
        }
    },
    "domainName": "cs",
    "primaryCluster": "cluster"
}`);

EXPECT_OUTPUT_CONTAINS_MULTILINE("WARNING: You are about to dissolve the whole ClusterSet, this operation cannot be reverted. All Clusters will be removed from the ClusterSet, all members removed from their corresponding Clusters, replication will be stopped, internal recovery user accounts and the ClusterSet / Cluster metadata will be dropped. User data will be kept intact in all instances.");

//@<> Dissolve clusterset in interactive mode + dryRun (success)

testutil.expectPrompt("Are you sure you want to dissolve the ClusterSet?", "y");
EXPECT_NO_THROWS(function() { cs.dissolve({dryRun: true}); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`* Waiting for all reachable REPLICA Clusters to apply received transactions...

* Dissolving the ClusterSet...

* Waiting for all reachable REPLICA Clusters to synchronize with the PRIMARY Cluster...
* Synchronizing all members and dissolving cluster 'replica'...
* Synchronizing all members and dissolving cluster 'cluster'...

The ClusterSet has been dissolved, user data was left intact.

dryRun finished.
`);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], c1, c2);

//@<> Dissolve clusterset in interactive mode (success)

testutil.expectPrompt("Are you sure you want to dissolve the ClusterSet?", "y");
EXPECT_NO_THROWS(function() { cs.dissolve(); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`* Waiting for all reachable REPLICA Clusters to apply received transactions...

* Dissolving the ClusterSet...

* Waiting for all reachable REPLICA Clusters to synchronize with the PRIMARY Cluster...
* Synchronizing all members and dissolving cluster 'replica'...
* Synchronizing all members and dissolving cluster 'cluster'...

The ClusterSet has been dissolved, user data was left intact.`);

EXPECT_OUTPUT_NOT_CONTAINS("dryRun finished.");

shell.options.useWizards=0;

shell.connect(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);
var session4 = mysql.getSession(__sandbox_uri2);
var session5 = mysql.getSession(__sandbox_uri3);

CHECK_DISSOLVED_CLUSTER(session, session);
CHECK_DISSOLVED_CLUSTER(session, session2);
CHECK_DISSOLVED_CLUSTER(session, session3);
CHECK_DISSOLVED_CLUSTER(session4, session4);
CHECK_DISSOLVED_CLUSTER(session4, session5);

session2.close();
session3.close();
session4.close();
session5.close();

//@<> Check calling 'dissolve' again
EXPECT_THROWS(function(){
    cs.dissolve();
}, "The ClusterSet object is disconnected. Please use dba.getClusterSet() to obtain a fresh handle.");

//@<> Dissolve fails because one instance (read-replica) is not reachable.

// re-create clusterset
c1 = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
c1.addInstance(__sandbox_uri2);
c1.addReplicaInstance(__sandbox_uri3);

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "replica");
c2.addInstance(__sandbox_uri5);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], c1, c2);

// disable read-replica
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

disable_auto_rejoin(__mysql_sandbox_port3);

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

WIPE_OUTPUT();

EXPECT_THROWS(function() {
    cs.dissolve();
}, "Failed to dissolve the ClusterSet: Unable to connect to all instances.");

EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to connect to instance '${hostname}:${__mysql_sandbox_port3}'. Please verify connection credentials and make sure the instance is available.`);

//@<> Dissolve should work if one instance (read-replica) is not reachable + force: true
EXPECT_NO_THROWS(function(){ cs.dissolve({force: true}); });

EXPECT_OUTPUT_CONTAINS(`NOTE: The instance '${hostname}:${__mysql_sandbox_port3}' is not reachable and it will only be removed from the metadata. Please take any necessary actions to make sure that the instance will not start/rejoin the cluster if brought back online.`);

//@<> Dissolve fails because one instance (read-replica) is not ONLINE.

// re-create clusterset
testutil.startSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri3);
reset_instance(session);

shell.connect(__sandbox_uri1);

c1 = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
c1.addInstance(__sandbox_uri2);
c1.addReplicaInstance(__sandbox_uri3);

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "replica");
c2.addInstance(__sandbox_uri5);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], c1, c2);

// disable read-replica
shell.connect(__sandbox_uri3);
session.runSql("STOP REPLICA FOR CHANNEL 'read_replica_replication'");
testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "OFF");

WIPE_OUTPUT();

EXPECT_THROWS(function() {
    cs.dissolve();
}, "Failed to dissolve the ClusterSet: Detected instances with (MISSING) state.");

EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port3}' cannot be removed because it is on a 'OFFLINE' state. Please bring the instance back ONLINE and try to dissolve the ClusterSet again. If the instance is permanently not reachable, then please use <ClusterSet>.dissolve() with the 'force' option enabled to proceed with the operation and only remove the instance from the ClusterSet Metadata.`);

//@<> Dissolve should work if one instance (read-replica) is not ONLINE + force: true
EXPECT_NO_THROWS(function(){ cs.dissolve({force: true}); });

EXPECT_OUTPUT_CONTAINS(`NOTE: The instance '${hostname}:${__mysql_sandbox_port3}' is 'OFFLINE' and it will only be removed from the metadata. Please take any necessary actions to make sure that the instance will not start/rejoin the cluster if brought back online.`);

//@<> Dissolve fails because one instance (non read-replica) is unreachable.

// re-create clusterset
shell.connect(__sandbox_uri3);
reset_instance(session);

shell.connect(__sandbox_uri1);

c1 = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
c1.addInstance(__sandbox_uri2);
c1.addReplicaInstance(__sandbox_uri3);

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "replica");
c2.addInstance(__sandbox_uri5);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], c1, c2);

testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

EXPECT_THROWS(function() {
    cs.dissolve();
}, "Failed to dissolve the ClusterSet: Detected instances with (MISSING) state.");

EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' cannot be removed because it is on a '(MISSING)' state. Please bring the instance back ONLINE and try to dissolve the ClusterSet again. If the instance is permanently not reachable, then please use <ClusterSet>.dissolve() with the 'force' option enabled to proceed with the operation and only remove the instance from the ClusterSet Metadata.`);

//@<> Dissolve should work if one instance (non read-replica) is unreachable + force: true
EXPECT_NO_THROWS(function(){ cs.dissolve({force: true}); });

//@<> Dissolve fails because replica cluster doesn't have quorum

testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri5);
reset_instance(session);
shell.connect(__sandbox_uri4);
reset_instance(session);
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);

shell.connect(__sandbox_uri1);
reset_instance(session);

c1 = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
c1.addInstance(__sandbox_uri2);

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri3, "replica");
c2.addInstance(__sandbox_uri4);
c2.addInstance(__sandbox_uri5);

// loose quorun
shell.connect(__sandbox_uri4);
session.runSql("STOP group_replication;");
testutil.killSandbox(__mysql_sandbox_port5);

shell.connect(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port4, "(MISSING),UNREACHABLE");
testutil.waitMemberState(__mysql_sandbox_port5, "(MISSING),UNREACHABLE");

WIPE_OUTPUT();

EXPECT_THROWS(function(){
    cs.dissolve();
}, "One or more REPLICA Clusters are unreachable.");

EXPECT_OUTPUT_CONTAINS("ERROR: Could not connect to PRIMARY of cluster 'replica': MYSQLSH 51011: Cluster 'replica' has no quorum");
EXPECT_STDOUT_CONTAINS_MULTILINE("ERROR: Unable to connect to Cluster 'replica'. Please make sure the cluster is reachable. You can also choose to skip this error using the 'force: true' option, but it might leave the cluster in an inconsistent state and lead to errors if you want to reuse it.");

//@<> Dissolve succeeds if replica cluster doesn't have quorum + force

EXPECT_NO_THROWS(function(){ cs.dissolve({force: true}); });

EXPECT_OUTPUT_NOT_CONTAINS("ERROR: Could not connect to PRIMARY of cluster 'replica':");
EXPECT_OUTPUT_CONTAINS("NOTE: Unable to connect to Cluster 'replica'. For all reachable instances of the Cluster, replication will be stopped and, if possible, configuration reset. However, internal replication accounts and metadata will remain unchanged, thus for those instances to be re-used the metadata schema must be dropped.");

EXPECT_OUTPUT_NOT_CONTAINS("* Waiting for all reachable REPLICA Clusters to apply received transactions..."); //because the only replica cluster is unreachable

EXPECT_OUTPUT_CONTAINS("* Dissolving the ClusterSet...");

EXPECT_OUTPUT_NOT_CONTAINS("* Waiting for all reachable REPLICA Clusters to synchronize with the PRIMARY Cluster..."); //because the only replica cluster is unreachable
EXPECT_OUTPUT_CONTAINS("* Stopping replication on reachable members of 'replica'...");
EXPECT_OUTPUT_CONTAINS("* Synchronizing all members and dissolving cluster 'cluster'...");
EXPECT_OUTPUT_CONTAINS("The ClusterSet has been dissolved, user data was left intact.");

shell.connect(__sandbox_uri3);
EXPECT_EQ("OFFLINE", session.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);

shell.connect(__sandbox_uri4);
EXPECT_EQ("OFFLINE", session.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);

testutil.startSandbox(__mysql_sandbox_port5);

//@<> Dissolve fails due to timeout {!__dbug_off}

// re-create clusterset

shell.connect(__sandbox_uri5);
reset_instance(session);
shell.connect(__sandbox_uri4);
reset_instance(session);
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);

shell.connect(__sandbox_uri1);
reset_instance(session);

c1 = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
c1.addInstance(__sandbox_uri2);
c1.addReplicaInstance(__sandbox_uri3);

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "replica");
c2.addInstance(__sandbox_uri5);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2], c1);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5], c1, c2);

testutil.dbugSet("+d,dba_sync_transactions_timeout");

EXPECT_THROWS(function() {
    cs.dissolve();
}, `Timeout reached waiting for all received transactions to be applied on instance '${hostname}:${__mysql_sandbox_port4}'`);

testutil.dbugSet("");

EXPECT_OUTPUT_CONTAINS(`The Cluster 'replica' failed to synchronize its transaction set with the PRIMARY Cluster. You may increase the transaction sync timeout with the option 'timeout' or use the 'force' option to ignore the timeout.`);

//@<> Dissolve should succeed if timeout + force: true {!__dbug_off}
testutil.dbugSet("+d,dba_sync_transactions_timeout");

EXPECT_NO_THROWS(function(){ cs.dissolve({force: true}); });

testutil.dbugSet("");

EXPECT_OUTPUT_CONTAINS("* Waiting for all reachable REPLICA Clusters to apply received transactions...");
EXPECT_OUTPUT_CONTAINS(`ERROR: The Cluster 'replica' failed to synchronize its transaction set with the PRIMARY Cluster. You may increase the transaction sync timeout with the option 'timeout' or use the 'force' option to ignore the timeout.`);
EXPECT_OUTPUT_CONTAINS(`WARNING: Transaction sync failed but ignored because of 'force' option: MYSQLSH 51157: Timeout reached waiting for all received transactions to be applied on instance `);

EXPECT_OUTPUT_CONTAINS("* Dissolving the ClusterSet...");

EXPECT_OUTPUT_CONTAINS("* Waiting for all reachable REPLICA Clusters to synchronize with the PRIMARY Cluster...");
EXPECT_OUTPUT_CONTAINS("* Synchronizing all members and dissolving cluster 'replica'...");
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port4}' failed to synchronize its transaction set with its Cluster: Timeout reached waiting for all received transactions to be applied on instance '${hostname}:${__mysql_sandbox_port4}' (debug)`);
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port5}' failed to synchronize its transaction set with its Cluster: Timeout reached waiting for all received transactions to be applied on instance '${hostname}:${__mysql_sandbox_port5}' (debug)`);
EXPECT_OUTPUT_CONTAINS("* Synchronizing all members and dissolving cluster 'cluster'...");
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port3}' failed to synchronize its transaction set with its Cluster: Timeout reached waiting for all received transactions to be applied on instance '${hostname}:${__mysql_sandbox_port3}' (debug)`);
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port1}' failed to synchronize its transaction set with its Cluster: Timeout reached waiting for all received transactions to be applied on instance '${hostname}:${__mysql_sandbox_port1}' (debug)`);
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' failed to synchronize its transaction set with its Cluster: Timeout reached waiting for all received transactions to be applied on instance '${hostname}:${__mysql_sandbox_port2}' (debug)`);

EXPECT_OUTPUT_CONTAINS("The ClusterSet has been dissolved, user data was left intact.");

//@<> Dissolve fail due to error in replication cleanup {!__dbug_off}
shell.connect(__sandbox_uri5);
reset_instance(session);
shell.connect(__sandbox_uri4);
reset_instance(session);
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);

shell.connect(__sandbox_uri1);
reset_instance(session);

c1 = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
c1.addInstance(__sandbox_uri2);
c1.addReplicaInstance(__sandbox_uri3);

cs = c1.createClusterSet("cs");

c2 = cs.createReplicaCluster(__sandbox_uri4, "replica");
c2.addInstance(__sandbox_uri5);

WIPE_OUTPUT();

testutil.dbugSet("+d,dba_dissolve_replication_error");

EXPECT_NO_THROWS(function(){ cs.dissolve(); });

EXPECT_OUTPUT_CONTAINS("* Waiting for all reachable REPLICA Clusters to apply received transactions...");

EXPECT_OUTPUT_CONTAINS("* Dissolving the ClusterSet...");

EXPECT_OUTPUT_CONTAINS("* Waiting for all reachable REPLICA Clusters to synchronize with the PRIMARY Cluster...");
EXPECT_OUTPUT_CONTAINS("* Synchronizing all members and dissolving cluster 'replica'...");
EXPECT_OUTPUT_CONTAINS(`WARNING: An error occurred when trying to remove instance '${hostname}:${__mysql_sandbox_port4}' from cluster 'replica'. The instance might have been left active, requiring manual action to fully dissolve the cluster.`);
EXPECT_OUTPUT_CONTAINS(`WARNING: An error occurred when trying to remove instance '${hostname}:${__mysql_sandbox_port5}' from cluster 'replica'. The instance might have been left active, requiring manual action to fully dissolve the cluster.`);
EXPECT_OUTPUT_CONTAINS("* Synchronizing all members and dissolving cluster 'cluster'...");
EXPECT_OUTPUT_CONTAINS(`WARNING: Unable to remove read-replica replication channel for instance '${hostname}:${__mysql_sandbox_port3}': Error while removing read-replica async replication channel`);
EXPECT_OUTPUT_CONTAINS(`WARNING: An error occurred when trying to remove instance '${hostname}:${__mysql_sandbox_port2}' from cluster 'cluster'. The instance might have been left active, requiring manual action to fully dissolve the cluster.`);
EXPECT_OUTPUT_CONTAINS(`WARNING: An error occurred when trying to remove instance '${hostname}:${__mysql_sandbox_port1}' from cluster 'cluster'. The instance might have been left active, requiring manual action to fully dissolve the cluster.`);
EXPECT_OUTPUT_CONTAINS("The ClusterSet has been dissolved, user data was left intact.");

testutil.dbugSet("");

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
