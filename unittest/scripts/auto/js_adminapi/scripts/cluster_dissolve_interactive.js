// Assumptions: smart deployment routines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

//@ Create cluster, disable interactive mode.
// WL11889 FR1_01: new interactive option to disable interactive mode.
shell.connect(__sandbox_uri1);
var c = dba.createCluster('c');

//@<> Dissolve cluster, disable interactive mode.
// WL11889 FR1_01: new interactive option to disable interactive mode.
// WL11889 FR3_01: force option not needed to remove cluster.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
c.dissolve({interactive: false});

//@ Create cluster, with unreachable instances.
var c = dba.createCluster('c', {clearReadOnly: true});

//@ Add instance on port2.
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Add instance on port3.
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Reset persisted gr_start_on_boot on instance3 {VER(>=8.0.11)}
// NOTE: This trick is required to reuse unreachable servers in tests and avoid
// them from automatically rejoining the group.
// Changing the my.cnf is not enough since persisted variables have precedence.
session.close();
c.disconnect();
shell.connect(__sandbox_uri3);
session.runSql("RESET PERSIST group_replication_start_on_boot");
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');

//@ Stop instance on port3 and wait for it to be unreachable (missing).
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@<> Dissolve stopped because instance is unreachable and user answer 'no' to continue.
// WL11889 FR7_02: answering 'no' to continue is equivalent to force: false.
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:", "no");
c.dissolve();

//@<> Dissolve stopped because one instance is unreachable (force: false).
// WL11889 FR7_03: error if force: false and any instance is unreachable (interactive mode).
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
c.dissolve({force: false});

//@<> Dissolve continues because instance is unreachable and user answer 'yes' to continue.
// WL11889 FR7_01: answering 'yes' to continue is equivalent to force: true.
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:", "yes");
c.dissolve();

//@ Dissolve post action on unreachable instance (ensure GR is not started)
// NOTE: This is not enough for server version >= 8.0.11 because persisted
// variables take precedence, therefore reseting group_replication_start_on_boot
// is also needed.
testutil.changeSandboxConf(__mysql_sandbox_port3, 'group_replication_start_on_boot', 'OFF');

//@ Restart instance on port3
testutil.startSandbox(__mysql_sandbox_port3);

//@ Create cluster, instance with replication errors.
c = dba.createCluster('c', {clearReadOnly: true});

//@ Add instance on port2, again.
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Add instance on port3, again.
c.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Connect to instance on port2 to introduce a replication error
// IMPORTANT NOTE: GR members with replication errors are automatically removed
// from the GR group after a short (non-deterministic) time, becoming
// unreachable '(MISSING)'. Therefore, this replication error scenario is
// temporary and created immediately before each dissolve in an attempt to
// verify the desired test case. If the test fail because the instance is
// unreachable then remove this test case since it cannot be tested in a
// deterministic way.
// WL11889 - FR9_01
session.close();
c.disconnect();
shell.connect(__sandbox_uri2);
session.runSql("SET sql_log_bin = 0");
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("CREATE USER 'replication_error'@'%' IDENTIFIED BY 'somepass';");
session.runSql("SET GLOBAL super_read_only = 1");
session.runSql("SET sql_log_bin = 1");

//@ Avoid server from aborting (killing itself) {VER(>=8.0.12)}
// NOTE: Starting from version 8.0.12 GR will automatically abort (kill) the
// mysql server if a replication error occurs. The GR exit state action need
// to be changed to avoid this.
// WL11889 - FR9_01 and FR10_01
session.runSql('SET GLOBAL group_replication_exit_state_action = READ_ONLY');

//@ Connect to seed and get cluster
// WL11889 - FR9_01
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');

//@ Change shell option dba.gtidWaitTimeout to 1 second (to issue error faster)
// WL11889 - FR9_01 and FR10_01
shell.options["dba.gtidWaitTimeout"] = 1;

//@ Execute trx that will lead to error on instance2
// WL11889 - FR9_01
session.runSql("CREATE USER 'replication_error'@'%' IDENTIFIED BY 'somepass';");

//@<> Dissolve stopped because instance cannot catch up with cluster and user answer 'n' to continue.
// WL11889 FR9_01: error, instance cannot catch up (answering 'n' to continue = force: false)
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:", "n");
c.dissolve();

//@ Connect to instance on port2 to fix error and add new one
// IMPORTANT NOTE: GR members with replication errors are automatically removed
// from the GR group after a short (non-deterministic) time, becoming
// unreachable '(MISSING)'. Therefore, this replication error scenario is
// temporary and created immediately before each dissolve in an attempt to
// verify the desired test case. If the test fail because the instance is
// unreachable then remove this test case since it cannot be tested in a
// deterministic way.
// WL11889 - FR9_01
session.close();
c.disconnect();
shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
session.runSql("SET sql_log_bin = 0");
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("DROP USER 'replication_error'@'%'");
session.runSql("CREATE USER 'replication_error_2'@'%' IDENTIFIED BY 'somepass';");
session.runSql("SET GLOBAL super_read_only = 1");
session.runSql("SET sql_log_bin = 1");
session.runSql("START GROUP_REPLICATION");

//@ Connect to seed and get cluster again
// WL11889 - FR9_01
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Execute trx that will lead to error on instance2, again
// WL11889 - FR9_01
session.runSql("CREATE USER 'replication_error_2'@'%' IDENTIFIED BY 'somepass';");

//@<> Dissolve stopped because instance cannot catch up with cluster (force: false).
// WL11889 FR9_01: error, instance cannot catch up (force: false)
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
c.dissolve({force: false});

//@ Connect to instance on port2 to fix error and add new one, one last time
// IMPORTANT NOTE: GR members with replication errors are automatically removed
// from the GR group after a short (non-deterministic) time, becoming
// unreachable '(MISSING)'. Therefore, this replication error scenario is
// temporary and created immediately before each dissolve in an attempt to
// verify the desired test case. If the test fail because the instance is
// unreachable then remove this test case since it cannot be tested in a
// deterministic way.
// WL11889 - FR10_01
session.close();
c.disconnect();
shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
session.runSql("SET sql_log_bin = 0");
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("DROP USER 'replication_error_2'@'%'");
session.runSql("CREATE USER 'replication_error_3'@'%' IDENTIFIED BY 'somepass';");
session.runSql("SET GLOBAL super_read_only = 1");
session.runSql("SET sql_log_bin = 1");
session.runSql("START GROUP_REPLICATION");

//@ Connect to seed and get cluster one last time
// WL11889 - FR10_01
session.close();
shell.connect(__sandbox_uri1);
var c = dba.getCluster('c');
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Execute trx that will lead to error on instance2, one last time
// WL11889 - FR10_01
session.runSql("CREATE USER 'replication_error_3'@'%' IDENTIFIED BY 'somepass';");

//@<> Dissolve continues because instance cannot catch up with cluster and user answer 'y' to continue.
// WL11889 FR10_01: continue (no error), instance cannot catch up (answering 'y' to continue = force: true)
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:", "y");
c.dissolve();

//@ Finalization
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
