// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

function print_instances_count_for_gr() {
    var result = session.runSql("SELECT COUNT(*) FROM performance_schema.replication_group_members");
    var row = result.fetchOne();
    print(row[0] + "\n");
}

function print_gr_start_on_boot() {
    var result = session.runSql("SHOW VARIABLES LIKE 'group_replication_start_on_boot';");
    var row = result.fetchOne();
    print(row[1] + "\n");
}

//@ Connect
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ create cluster
var cluster = dba.createCluster('dev');

//@ Adding instance
cluster.addInstance(__sandbox_uri2);

// Waiting for the second added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// Answer the prompt to continue without creating root@% (option 3).
testutil.expectPrompt("*", "3");

//@ Configure instance on port1.
var cnfPath1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port1, {mycnfPath: cnfPath1, dbPassword:'root'});

// Answer the prompt to continue without creating root@% (option 3).
testutil.expectPrompt("*", "3");
// Answer the prompt to disable super_read_only (y)
testutil.expectPrompt("*", "y");

//@ Configure instance on port2.
var cnfPath2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port2, {mycnfPath: cnfPath2, dbPassword:'root'});

//@<OUT> Number of instance according to GR.
print_instances_count_for_gr();

//@ Failure: remove_instance - invalid uri
cluster.removeInstance('@localhost:' + __mysql_sandbox_port2);

//@<OUT> Cluster status
cluster.status();

//@ Remove instance failure due to wrong credentials
cluster.removeInstance({Host: "localhost", PORT: __mysql_sandbox_port2, User: "foo", PassWord: "bar"});

//@<OUT> Cluster status after remove failed
cluster.status();

//@ Removing instance
cluster.removeInstance('root:root@localhost:' + __mysql_sandbox_port2);

//@<OUT> Cluster status after removal
cluster.status();

//@<OUT> Number of instance according to GR after removal.
// Regression for BUG#26796118 : INSTANCE REJOINS GR GROUP AFTER REMOVEINSTANCE() AND RESTART
print_instances_count_for_gr();

//@ Stop instance on port2.
// Regression for BUG#26796118 : INSTANCE REJOINS GR GROUP AFTER REMOVEINSTANCE() AND RESTART
testutil.stopSandbox(__mysql_sandbox_port2, "root");

//@ Restart instance on port2.
// Regression for BUG#26796118 : INSTANCE REJOINS GR GROUP AFTER REMOVEINSTANCE() AND RESTART
testutil.startSandbox(__mysql_sandbox_port2);

//@ Connect to restarted instance.
session.close();
cluster.disconnect();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});

//@<OUT> Confirm that GR start on boot is disabled {VER(>=8.0.4)}.
// Regression for BUG#26796118 : INSTANCE REJOINS GR GROUP AFTER REMOVEINSTANCE() AND RESTART
// NOTE: Cannot count instance for GR due to a SET PERSIST bug (BUG#26495619).
// This test check is only valid for server version >= 8.0.4.
print_gr_start_on_boot();

//@ Connect back to seed instance and get cluster.
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster('dev');

//@ Adding instance on port2 back
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
cluster.addInstance(__sandbox_uri2);

// Waiting for the instance on port2 to become online
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Stop instance on port2
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
testutil.stopSandbox(__mysql_sandbox_port2, "root");

// Waiting for the instance on port2 to be found missing
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@<OUT> Cluster status after instance on port2 is stopped
cluster.status();

//@ Error removing stopped instance on port2
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
cluster.removeInstance('root:root@localhost:' + __mysql_sandbox_port2);

//@ Remove stopped instance on port2 with force option
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
// BUG#26986141 : REMOVEINSTANCE DOES NOT ACCEPT A SECOND PARAMETER
cluster.removeInstance('root@localhost:' + __mysql_sandbox_port2, {force: true, PassWord: "root"});

//@<OUT> Cluster status after removal of instance on port2
cluster.status();

//@ Error removing last instance
// Regression for BUG#25226130 : REMOVAL OF SEED NODE BREAKS DISSOLVE
cluster.removeInstance('root:root@localhost:' + __mysql_sandbox_port1);

//@ Dissolve cluster with success
// Regression for BUG#25226130 : REMOVAL OF SEED NODE BREAKS DISSOLVE
cluster.dissolve({force: true});

cluster.disconnect();

// We must use clearReadOnly because the cluster was dissolved
// (BUG#26422638)

//@ Cluster re-created with success
// Regression for BUG#25226130 : REMOVAL OF SEED NODE BREAKS DISSOLVE
var cluster = dba.createCluster('dev', {clearReadOnly: true});

session.close();
cluster.disconnect();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
// Note: set quiet kill to true when destroying the 2nd sandbox to avoid
//       displaying expected errors, since it was already stopped previously in
//       the test.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2, true);
testutil.destroySandbox(__mysql_sandbox_port3);
