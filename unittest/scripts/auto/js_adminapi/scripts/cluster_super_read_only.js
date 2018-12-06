
// Trying to do cluster operations on a cluster through a member that's RO

//@ Init
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
// Connect to the secondary and get a session to it
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster(null, {connectToPrimary:false});

//@ addInstance
cluster.addInstance(__sandbox_uri3);

//@ removeInstance
cluster.removeInstance(__sandbox_uri1);

//@ rejoinInstance (OK - should fail, but not because of R/O)
cluster.rejoinInstance(__sandbox_uri1);

//@ dissolve
cluster.dissolve();

//@ rescan
cluster.rescan();

//@ status (OK)
cluster.status();

//@ describe (OK)
cluster.describe();

//@ disconnect (OK)
cluster.disconnect();

//@ Fini
session.close();
scene.destroy();
