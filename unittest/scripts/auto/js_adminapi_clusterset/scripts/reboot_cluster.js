//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

// Tests various scenarios for dba.rebootClusterFromCompleteOutage() in InnoDB ClusterSet scenarios

//@<> Setup + Create primary cluster + add Replica Cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port6, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");

replicacluster = cs.createReplicaCluster(__sandbox_uri4, "replica");
replicacluster.addInstance(__sandbox_uri5);
replicacluster.addInstance(__sandbox_uri6);

CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], cluster, replicacluster);

//@<> Rebooting from complete outage a REPLICA Cluster in interactive mode
shell.options.useWizards=1

disable_auto_rejoin(__mysql_sandbox_port4);
disable_auto_rejoin(__mysql_sandbox_port5);
disable_auto_rejoin(__mysql_sandbox_port6);

testutil.killSandbox(__mysql_sandbox_port5);
testutil.killSandbox(__mysql_sandbox_port4);
testutil.killSandbox(__mysql_sandbox_port6);

testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port6);

shell.connect(__sandbox_uri4);

testutil.expectPrompt("Would you like to rejoin it to the cluster? [y/N]:", "y");
testutil.expectPrompt("Would you like to remove it from the cluster's metadata? [y/N]:", "n");
EXPECT_NO_THROWS(function() {replicacluster = dba.rebootClusterFromCompleteOutage("replica"); })

// Check that the cluster rejoined the ClusterSet
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], cluster, replicacluster);

//@<> Rebooting from complete outage a REPLICA Cluster using the options 'rejoinInstances' and 'removeInstances'
shell.options.useWizards=0
disable_auto_rejoin(__mysql_sandbox_port4);
disable_auto_rejoin(__mysql_sandbox_port6);

testutil.killSandbox(__mysql_sandbox_port4);
testutil.killSandbox(__mysql_sandbox_port6);

testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port6);

shell.connect(__sandbox_uri4);

EXPECT_NO_THROWS(function() {replicacluster = dba.rebootClusterFromCompleteOutage("replica", {rejoinInstances: [__endpoint6], removeInstances: [__endpoint5]}); })

// Check that the cluster rejoined the ClusterSet
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri6], cluster, replicacluster);

//Shutdown the PRIMARY Cluster
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

shell.connect(__sandbox_uri3);
testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);

//Failover to the replica cluster
shell.connect(__sandbox_uri4);
cs = dba.getClusterSet();
cs.forcePrimaryCluster("replica");
cluster = dba.getCluster();

//Reboot the old-primary cluster
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);

//@<> Rebooting from complete outage a PRIMARY Cluster that has been invalidated using the options 'rejoinInstances' and 'removeInstances' (must fail)
EXPECT_THROWS_TYPE(function(){ former_primary = dba.rebootClusterFromCompleteOutage("cluster", {rejoinInstances: [__endpoint3], removeInstances: [__endpoint2]}); }, "removeInstances and/or rejoinInstances options cannot be used for Invalidated Clusters", "ArgumentError");
EXPECT_OUTPUT_CONTAINS("Please add or remove the instances after the Cluster is rejoined to the ClusterSet");
EXPECT_OUTPUT_CONTAINS("ERROR: Cannot proceed using 'removeInstances' and/or 'removeInstances': The Cluster is INVALIDATED");

//@<> Rebooting from complete outage a PRIMARY Cluster that has been invalidated without the options 'rejoinInstances' and 'removeInstances' (OK)
EXPECT_NO_THROWS(function(){ former_primary = dba.rebootClusterFromCompleteOutage("cluster"); });

EXPECT_OUTPUT_CONTAINS("* Waiting for seed instance to become ONLINE...");
CHECK_PRIMARY_CLUSTER([__sandbox_uri1], former_primary);

//@<> Rejoin the former PRIMARY Cluster back to the ClusterSet
EXPECT_NO_THROWS(function(){ cs.rejoinCluster("cluster"); });

// Check that the cluster rejoined the ClusterSet as a REPLICA
CHECK_REPLICA_CLUSTER([__sandbox_uri1], cluster, former_primary);

// Rejoin instance3 back to the cluster
EXPECT_NO_THROWS(function(){ former_primary.rejoinInstance(__sandbox_uri3); });

// Remove instance2 from the cluster's metadata
EXPECT_NO_THROWS(function(){ former_primary.removeInstance(__endpoint2, {force: true}); });

// Check that the cluster status as a REPLICA
CHECK_REPLICA_CLUSTER([__sandbox_uri1, __sandbox_uri3], cluster, former_primary);

// Add instance2 to the PRIMARY Cluster
testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri4);
c = dba.getCluster();
c.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});

//Shutdown the PRIMARY Cluster
disable_auto_rejoin(__mysql_sandbox_port4);
disable_auto_rejoin(__mysql_sandbox_port6);
disable_auto_rejoin(__mysql_sandbox_port2);

shell.connect(__sandbox_uri2);
testutil.killSandbox(__mysql_sandbox_port6);
testutil.killSandbox(__mysql_sandbox_port4);
testutil.killSandbox(__mysql_sandbox_port2);

//Failover to the replica cluster
shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();
cs.forcePrimaryCluster("cluster");
cluster = dba.getCluster();

//Reboot the old-primary cluster
testutil.startSandbox(__mysql_sandbox_port4);
testutil.startSandbox(__mysql_sandbox_port6);
testutil.startSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri4);

//@<> Rebooting from complete outage a PRIMARY Cluster that has been invalidated in interactive mode
shell.options.useWizards=1
EXPECT_NO_THROWS(function(){ former_primary = dba.rebootClusterFromCompleteOutage("replica"); });
testutil.assertNoPrompts();

EXPECT_OUTPUT_CONTAINS("The instance '<<<__endpoint6>>>' was part of the cluster configuration but the Cluster is invalidated. Please rejoin the instance after the Cluster is rejoined to the ClusterSet");
EXPECT_OUTPUT_CONTAINS("The instance '<<<__endpoint2>>>' was part of the cluster configuration but the Cluster is invalidated. Please rejoin the instance after the Cluster is rejoined to the ClusterSet");

// Rejoin the Cluster back to the ClusterSet
EXPECT_NO_THROWS(function(){ cs.rejoinCluster("replica"); });

// Check that the cluster rejoined the ClusterSet
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, former_primary);

// Rejoin back instances 2 and 6
former_primary.rejoinInstance(__sandbox_uri2);
former_primary.rejoinInstance(__sandbox_uri6);

CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri2, __sandbox_uri6], cluster, former_primary);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
