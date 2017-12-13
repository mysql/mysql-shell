
// Trying to do cluster operations on a cluster that has no quorum

//@ Init
var scene = ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2])
// Make sandbox1 the surviving member
scene.make_no_quorum([__mysql_sandbox_port1])

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
cluster.status();

//@ describe (OK)
cluster.describe();

//@ Fini
scene.destroy();
