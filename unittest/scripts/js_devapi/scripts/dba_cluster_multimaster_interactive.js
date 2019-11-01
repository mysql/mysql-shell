// Assumptions: smart deployment rountines available

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

//@<OUT> Dba: createCluster multiPrimary with interaction, cancel
dba.createCluster('devCluster', {multiPrimary: true});

//@<OUT> Dba: createCluster multiPrimary with interaction, ok
dba.createCluster('devCluster', {multiPrimary: true});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
var Cluster = dba.getCluster('devCluster');

//@ Dissolve cluster
Cluster.dissolve({force: true});
Cluster.disconnect();

//@<OUT> Dba: createCluster multiMaster with interaction, regression for BUG#25926603
dba.createCluster('devCluster', {multiMaster: true, gtidSetIsComplete: true});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
Cluster = dba.getCluster('devCluster');

//@ Cluster: addInstance with interaction, error
add_instance_options['port'] = __mysql_sandbox_port1;
add_instance_options['user'] = 'root';
Cluster.addInstance(add_instance_options, add_instance_extra_opts);

//@<OUT> Cluster: addInstance with interaction, ok
Cluster.addInstance(__sandbox_uri2);

//@<OUT> Cluster: addInstance 3 with interaction, ok
Cluster.addInstance(__sandbox_uri3);

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

//@<OUT> Dba: createCluster multiPrimary with interaction 2, ok
dba.createCluster('devCluster', {multiPrimary: true, memberSslMode: 'REQUIRED', gtidSetIsComplete: true});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
Cluster = dba.getCluster('devCluster');

//@<OUT> Cluster: addInstance with interaction, ok 2
Cluster.addInstance(__sandbox_uri2);

//@<OUT> Cluster: addInstance with interaction, ok 3
Cluster.addInstance(__sandbox_uri3);

//@<OUT> Cluster: status: success
Cluster.status();

// Rejoin tests

session.close();
shell.connect(__sandbox_uri3);

//@ Disable group_replication_start_on_boot if version >= 8.0.11 {VER(>=8.0.11)}
session.runSql("SET PERSIST group_replication_start_on_boot=OFF");

//@# Dba: stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
session.close();
shell.connect(__sandbox_uri1);
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
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3}, {memberSslMode: 'REQUIRED'});

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status();

Cluster.dissolve({force: true})

// Disable super-read-only (BUG#26422638)
shell.connect(__sandbox_uri1);
session.runSql("SET GLOBAL SUPER_READ_ONLY = 0;");
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
