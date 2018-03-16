// Assumptions: smart deployment rountines available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

// Enable MTS and set invalid settings for GR (instance 1).
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET GLOBAL slave_parallel_workers = 1");
session.runSql("SET GLOBAL slave_parallel_type = 'DATABASE'");
session.runSql("SET GLOBAL slave_preserve_commit_order = 'ON'");
session.close();

// Enable MTS and set invalid settings for GR (instance 2).
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
session.runSql("SET GLOBAL slave_parallel_workers = 1");
session.runSql("SET GLOBAL slave_parallel_type = 'LOGICAL_CLOCK'");
session.runSql("SET GLOBAL slave_preserve_commit_order = 'OFF'");
session.close();

// Enable MTS and set invalid settings for GR (instance 3).
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
session.runSql("SET GLOBAL slave_parallel_workers = 1");
session.runSql("SET GLOBAL slave_preserve_commit_order = 'OFF'");
session.runSql("SET GLOBAL slave_parallel_type = 'DATABASE'");
session.close();

// Connect to seed instance.
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> check instance with invalid parallel type.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, password:'root', user: 'root'});

//@ Create cluster (fail: parallel type check fail).
var cluster = dba.createCluster('mtsCluster', {memberSslMode:'DISABLED'});

//@ fix config including parallel type
dba.configureLocalInstance({host: localhost, port: __mysql_sandbox_port1, password:'root', user: 'root'});

//@ Create cluster (succeed this time).
var cluster = dba.createCluster('mtsCluster', {memberSslMode:'DISABLED'});

//@<OUT> check instance with invalid commit order.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port2, password:'root', user: 'root'});

//@ Adding instance to cluster (fail: commit order wrong).
cluster.addInstance({host: localhost, port: __mysql_sandbox_port2, password:'root', user: 'root'});

//@<OUT> check instance with invalid type and commit order.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port3, password:'root', user: 'root'});

//@ configure instance and update type and commit order with valid values.
dba.configureLocalInstance({host: localhost, port: __mysql_sandbox_port3, password:'root', user: 'root'}, {mycnfPath:'mybad.cnf'});

//@<OUT> check instance, no invalid values after configure.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port3, password:'root', user: 'root'});

//@ Adding instance to cluster (succeed: nothing to update).
add_instance_to_cluster(cluster, __mysql_sandbox_port3);
wait_slave_state(cluster, uri3, "ONLINE");

session.close();
cluster.disconnect();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
