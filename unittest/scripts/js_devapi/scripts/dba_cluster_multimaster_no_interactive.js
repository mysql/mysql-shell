// Assumptions: smart deployment rountines available

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

//@ Dba: createCluster multiPrimary, ok
var cluster = dba.createCluster('devCluster', {multiPrimary: true, force: true, clearReadOnly: true});
cluster.disconnect();

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
var Cluster = dba.getCluster('devCluster');

//@ Dissolve cluster
Cluster.dissolve({force: true});

Cluster.disconnect();

//@ Dba: createCluster multiMaster with interaction, regression for BUG#25926603
Cluster = dba.createCluster('devCluster', {multiMaster: true, force: true, clearReadOnly: true, gtidSetIsComplete: true});

Cluster.disconnect();

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
Cluster = dba.getCluster('devCluster');

//@ Cluster: addInstance 2
Cluster.addInstance(__sandbox_uri2);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Cluster: addInstance 3
Cluster.addInstance(__sandbox_uri3);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

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

//@ Dba: createCluster multiPrimary 2, ok
Cluster = dba.createCluster('devCluster', {multiPrimary: true, force: true, memberSslMode: 'REQUIRED', clearReadOnly: true, gtidSetIsComplete: true});

Cluster.disconnect();

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
var Cluster = dba.getCluster('devCluster');

//@ Cluster: addInstance 2 again
Cluster.addInstance(__sandbox_uri2);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Cluster: addInstance 3 again
Cluster.addInstance(__sandbox_uri3);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

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

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

// start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

//@ Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost", schema: 'abs', "authMethod":56});
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance("localhost:3306");

//@#: Dba: rejoin instance 3 ok
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, password:"root"}, {memberSslMode: 'REQUIRED'});

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

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
