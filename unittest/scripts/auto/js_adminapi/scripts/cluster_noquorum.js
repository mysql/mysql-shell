
// Trying to do cluster operations on a cluster that has no quorum

//@ Init
var scene = ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
// Make sandbox1 the surviving member
scene.make_no_quorum([__mysql_sandbox_port1]);

//@ addInstance
cluster.addInstance(__sandbox_uri3);

//@ removeInstance
cluster.removeInstance(__sandbox_uri3);

//@ rejoinInstance
cluster.rejoinInstance(__sandbox_uri3);

//@ dissolve
cluster.dissolve();

//@ checkInstanceState
cluster.checkInstanceState(__sandbox_uri3);

//@ rescan
cluster.rescan();

//@ status (OK)
// WL#13084 - TSF3_1: mode is 'R/O' if there is no quorum on reachable members.
cluster.status();

//@ describe (OK)
cluster.describe();

//@ Fini
scene.destroy();


// BUG#25267603: addInstance() tries to use offline node after lost quorum
//@<> BUG#25267603: Init cluster.
var scene = ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

//@<> BUG#25267603: Loose quorum, kill all except instance 1.
scene.make_no_quorum([__mysql_sandbox_port1]);

//@<> BUG#25267603: force (reestablish) quorum.
cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);

//@<> BUG#25267603: add a new instance to the cluster.
cluster.addInstance(__sandbox_uri3);

//@ BUG#25267603: remove the primary instance from the cluster.
cluster.removeInstance(__sandbox_uri1);

//@<> BUG#25267603: connect to remaing instance and get the cluster.
shell.connect(__sandbox_uri3);
cluster = dba.getCluster();

//@ BUG#25267603: add the old primary instance back to the cluster.
cluster.addInstance(__sandbox_uri1);

//@<> BUG#25267603: clean-up.
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port3);
