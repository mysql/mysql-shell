// Assumptions: smart deployment routines available

//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

// Enable MTS and set invalid settings for GR (instance 1).
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET GLOBAL slave_parallel_workers = 1");
session.runSql("SET GLOBAL slave_parallel_type = 'DATABASE'");
session.runSql("SET GLOBAL slave_preserve_commit_order = 'ON'");
session.close();

// Enable MTS and set invalid settings for GR (instance 2).
shell.connect({host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
session.runSql("SET GLOBAL slave_parallel_workers = 1");
session.runSql("SET GLOBAL slave_parallel_type = 'LOGICAL_CLOCK'");
session.runSql("SET GLOBAL slave_preserve_commit_order = 'OFF'");
session.close();

// Enable MTS and set invalid settings for GR (instance 3).
shell.connect({host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
session.runSql("SET GLOBAL slave_parallel_workers = 1");
session.runSql("SET GLOBAL slave_preserve_commit_order = 'OFF'");
session.runSql("SET GLOBAL slave_parallel_type = 'DATABASE'");
session.close();

// Connect to seed instance.
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> check instance with invalid parallel type.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, password:'root'});

//@ Create cluster (succeed: parallel type updated).
var cluster = dba.createCluster('mtsCluster');

//@<OUT> check instance with invalid commit order.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port2, password:'root'});

//@ Adding instance to cluster (succeed: commit order updated).
add_instance_to_cluster(cluster, __mysql_sandbox_port2);
wait_slave_state(cluster, uri2, "ONLINE");

//@<OUT> check instance with invalid type and commit order.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port3, password:'root'});

//@<OUT> configure instance and update type and commit order with valid values.
dba.configureLocalInstance({host: localhost, port: __mysql_sandbox_port3, password:'root'}, {mycnfPath:'mybad.cnf'});

//@<OUT> check instance, no invalid values after configure.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port3, password:'root'});

//@ Adding instance to cluster (succeed: nothing to update).
add_instance_to_cluster(cluster, __mysql_sandbox_port3);
wait_slave_state(cluster, uri3, "ONLINE");

session.close();

//@ Finalization
if (deployed_here)
    cleanup_sandboxes(deployed_here);
