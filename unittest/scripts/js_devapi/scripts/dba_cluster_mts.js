// Assumptions: smart deployment rountines available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

// Enable MTS and set invalid settings for GR (instance 1).
shell.connect(__sandbox_uri1);
session.runSql("SET GLOBAL slave_parallel_workers = 1");
session.runSql("SET GLOBAL slave_parallel_type = 'DATABASE'");
session.runSql("SET GLOBAL slave_preserve_commit_order = 'ON'");
session.close();

// Enable MTS and set invalid settings for GR (instance 2).
shell.connect(__sandbox_uri2);
session.runSql("SET GLOBAL slave_parallel_workers = 1");
session.runSql("SET GLOBAL slave_parallel_type = 'LOGICAL_CLOCK'");
session.runSql("SET GLOBAL slave_preserve_commit_order = 'OFF'");
session.close();

// Enable MTS and set invalid settings for GR (instance 3).
shell.connect(__sandbox_uri3);
session.runSql("SET GLOBAL slave_parallel_workers = 1");
session.runSql("SET GLOBAL slave_preserve_commit_order = 'OFF'");
session.runSql("SET GLOBAL slave_parallel_type = 'DATABASE'");
session.close();

// Connect to seed instance.
shell.connect(__sandbox_uri1);

//@<OUT> check instance with invalid parallel type.
dba.checkInstanceConfiguration(__sandbox_uri1);

//@ Create cluster (fail: parallel type check fail).
var cluster = dba.createCluster('mtsCluster', {memberSslMode:'DISABLED'});

//@ fix config including parallel type
dba.configureLocalInstance(__sandbox_uri1);

//@ Create cluster (succeed this time).
var cluster = dba.createCluster('mtsCluster', {memberSslMode:'DISABLED', gtidSetIsComplete: true});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@<OUT> check instance with invalid commit order.
dba.checkInstanceConfiguration(__sandbox_uri2);

//@ Adding instance to cluster (fail: commit order wrong).
cluster.addInstance(__sandbox_uri2);

//@<OUT> check instance with invalid type and commit order.
dba.checkInstanceConfiguration(__sandbox_uri3);

//@ configure instance and update type and commit order with valid values.
dba.configureLocalInstance(__sandbox_uri3, {mycnfPath:'mybad.cnf'});

//@<OUT> check instance, no invalid values after configure.
dba.checkInstanceConfiguration(__sandbox_uri3);

//@ Adding instance to cluster (succeed: nothing to update).
cluster.addInstance(__sandbox_uri3)
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

session.close();
cluster.disconnect();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
