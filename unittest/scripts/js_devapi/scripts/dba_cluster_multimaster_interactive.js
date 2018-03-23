// Assumptions: smart deployment rountines available

testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> Dba: createCluster multiMaster with interaction, cancel
dba.createCluster('devCluster', {multiMaster: true, clearReadOnly: true});

//@<OUT> Dba: createCluster multiMaster with interaction, ok
if (__have_ssl)
  dba.createCluster('devCluster', {multiMaster: true, memberSslMode: 'REQUIRED', clearReadOnly: true});
else
  dba.createCluster('devCluster', {multiMaster: true, memberSslMode: 'DISABLED', clearReadOnly: true});

var Cluster = dba.getCluster('devCluster');

//@ Cluster: addInstance with interaction, error
add_instance_options['port'] = __mysql_sandbox_port1;
Cluster.addInstance(add_instance_options, add_instance_extra_opts);

//@<OUT> Cluster: addInstance with interaction, ok
add_instance_to_cluster(Cluster, __mysql_sandbox_port2);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> Cluster: addInstance 3 with interaction, ok
add_instance_to_cluster(Cluster, __mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Cluster: describe1
Cluster.describe();

//@<OUT> Cluster: status1
Cluster.status();

//@ Cluster: removeInstance
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2});

//@<OUT> Cluster: describe2
Cluster.describe();

//@<OUT> Cluster: status2
Cluster.status();

//@ Cluster: remove_instance 3
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port3});

//@ Cluster: Error cannot remove last instance
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port1});

//@ Dissolve cluster with success
Cluster.dissolve({force: true});

//@<OUT> Dba: createCluster multiMaster with interaction 2, ok
if (__have_ssl)
    dba.createCluster('devCluster', {multiMaster: true, memberSslMode: 'REQUIRED', clearReadOnly: true});
else
    dba.createCluster('devCluster', {multiMaster: true, memberSslMode: 'DISABLED', clearReadOnly: true});

var Cluster = dba.getCluster('devCluster');

//@<OUT> Cluster: addInstance with interaction, ok 2
add_instance_to_cluster(Cluster, __mysql_sandbox_port2);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> Cluster: addInstance with interaction, ok 3
add_instance_to_cluster(Cluster, __mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Cluster: status: success
Cluster.status()

// Rejoin tests

//@# Dba: stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
testutil.stopSandbox(__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

// start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

//@: Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance({host: "localhost", schema: "abs", "authMethod":56});
Cluster.rejoinInstance("localhost:3306");

//@<OUT> Cluster: rejoinInstance with interaction, ok
if (__have_ssl)
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3}, {memberSslMode: 'REQUIRED'});
else
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3}, {memberSslMode: 'DISABLED'});

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status();

Cluster.dissolve({force: true})

// Disable super-read-only (BUG#26422638)
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET GLOBAL SUPER_READ_ONLY = 0;");
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
