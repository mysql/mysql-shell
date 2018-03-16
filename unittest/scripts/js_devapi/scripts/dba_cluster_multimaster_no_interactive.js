// Assumptions: wait_slave_state is defined

testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.connect("mysql://root:root@localhost:"+__mysql_sandbox_port3);
create_root_from_anywhere(session, true);

shell.connect("mysql://root:root@localhost:"+__mysql_sandbox_port2);
create_root_from_anywhere(session, true);

shell.connect("mysql://root:root@localhost:"+__mysql_sandbox_port1);
create_root_from_anywhere(session, true);

//@ Dba: createCluster multiMaster, ok
if (__have_ssl)
  var cluster = dba.createCluster('devCluster', {multiMaster: true, force: true, memberSslMode: 'REQUIRED', clearReadOnly: true});
else
  var cluster = dba.createCluster('devCluster', {multiMaster: true, force: true, memberSslMode: 'DISABLED', clearReadOnly: true});

cluster.disconnect();

var Cluster = dba.getCluster('devCluster');

//@ Cluster: addInstance 2
add_instance_to_cluster(Cluster, __mysql_sandbox_port2);

wait_slave_state(Cluster, uri2, "ONLINE");

//@ Cluster: addInstance 3
add_instance_to_cluster(Cluster, __mysql_sandbox_port3);

wait_slave_state(Cluster, uri3, "ONLINE");

//@<OUT> Cluster: describe cluster with instance
Cluster.describe();

//@<OUT> Cluster: status cluster with instance
Cluster.status();

//@ Cluster: removeInstance 2
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2});

//@<OUT> Cluster: describe removed member
Cluster.describe();

//@<OUT> Cluster: status removed member
Cluster.status();

//@ Cluster: removeInstance 3
Cluster.removeInstance(uri3);

//@ Cluster: Error cannot remove last instance
Cluster.removeInstance(uri1);

//@ Dissolve cluster with success
Cluster.dissolve({force: true});

Cluster.disconnect();

//@ Dba: createCluster multiMaster 2, ok
if (__have_ssl)
    Cluster = dba.createCluster('devCluster', {multiMaster: true, force: true, memberSslMode: 'REQUIRED', clearReadOnly: true});
else
    Cluster = dba.createCluster('devCluster', {multiMaster: true, force: true, memberSslMode: 'DISABLED', clearReadOnly: true});

Cluster.disconnect();

var Cluster = dba.getCluster('devCluster');


//@ Cluster: addInstance 2 again
add_instance_to_cluster(Cluster, __mysql_sandbox_port2);

wait_slave_state(Cluster, uri2, "ONLINE");

//@ Cluster: addInstance 3 again
add_instance_to_cluster(Cluster, __mysql_sandbox_port3);

wait_slave_state(Cluster, uri3, "ONLINE");

//@<OUT> Cluster: status: success
Cluster.status();

// Rejoin tests

//@# Dba: stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
// if (__sandbox_dir)
//   dba.stopSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir, password: 'root'});
// else
//   dba.stopSandboxInstance(__mysql_sandbox_port3, {password: 'root'});
testutil.stopSandbox(__mysql_sandbox_port3);

wait_slave_state(Cluster, uri3, ["(MISSING)"]);

// start instance 3
//try_restart_sandbox(__mysql_sandbox_port3);
testutil.startSandbox(__mysql_sandbox_port3);

//@ Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost", schema: 'abs', "authMethod":56});
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance("localhost:3306");

//@#: Dba: rejoin instance 3 ok
if (__have_ssl)
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, password:"root"}, {memberSslMode: 'REQUIRED'});
else
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, password: "root"}, {memberSslMode: 'DISABLED'});

wait_slave_state(Cluster, uri3, "ONLINE");

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status();

Cluster.dissolve({'force': true})

//@ Cluster: disconnect should work, tho
Cluster.disconnect();

// Close session
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
