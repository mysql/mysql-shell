// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, "loose_mysql_native_password": "ON"});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

function print_instances_count_for_gr() {
    var result = session.runSql("SELECT COUNT(*) FROM performance_schema.replication_group_members");
    var row = result.fetchOne();
    print(row[0] + "\n");
}

function host_exist_in_metadata_schema(port) {
    var result = session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances WHERE instance_name = '" + hostname + ":" + port + "';");
    var row = result.fetchOne();
    return row[0] != 0;
}

//@ Connect
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ create cluster
var cluster = dba.createCluster('dev', {gtidSetIsComplete: true});

//@ remove instance not in MD but reachable when there's just 1 (should fail)
cluster.removeInstance(__hostname_uri3);
shell.options.useWizards=0;
cluster.removeInstance(__hostname_uri3);
shell.options.useWizards=1;

//@ Adding instance
cluster.addInstance(__sandbox_uri2);

// Waiting for the second added instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ remove instance not in MD but reachable when there are 2 (should fail)
cluster.removeInstance(__hostname_uri3);
shell.options.useWizards=0;
cluster.removeInstance(__hostname_uri3);
shell.options.useWizards=1;

//@ Configure instance on port1 to persist auto-rejoin settings {VER(<8.0.11)}
var cnfPath1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureInstance("root@localhost:"+__mysql_sandbox_port1, {mycnfPath: cnfPath1, password:'root'});

//@ Configure instance on port2 to persist auto-rejoin settings {VER(<8.0.11)}
var cnfPath2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureInstance("root@localhost:"+__mysql_sandbox_port2, {mycnfPath: cnfPath2, password:'root'});

//@<OUT> Number of instance according to GR.
print_instances_count_for_gr();

//@ Failure: remove_instance - invalid uri
// WL#11862 - FR1_3
cluster.removeInstance('@localhost:' + __mysql_sandbox_port2);

//@<OUT> Cluster status
cluster.status();

//@<> Remove instance failure due to wrong credentials
// WL#11862 - FR1_2, FR2_2
cluster.removeInstance({Host: hostname, PORT: __mysql_sandbox_port2, User: "foo", PassWord: "bar"});

//@<OUT> Cluster status after remove failed
// WL#11862 - FR1_2
cluster.status();

//@<> Removing instance
// WL#11862 - FR1_1, FR2_1
cluster.removeInstance('root:root@localhost:' + __mysql_sandbox_port2);

//@<OUT> Cluster status after removal
// WL#11862 - FR1_1
cluster.status();

//@ Adding instance on port2 back (interactive use)
// WL#11862 - FR3_1
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Adding instance on port3 (interactive use)
// WL#11862 - FR3_1
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Removing instance (interactive: true)
// WL#11862 - FR3_1
cluster.removeInstance(__sandbox_uri2);

//@<OUT> Removing instance (interactive: false)
// WL#11862 - FR3_1
shell.options.useWizards=0;
cluster.removeInstance(__sandbox_uri3);
shell.options.useWizards=1;

//@ Adding instance on port2 back (interactive: true, force: false)
// WL#11862 - FR3_2
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Adding instance on port3 back (interactive: true, force: true)
// WL#11862 - FR3_3
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Removing instance (interactive: true, force: false)
// WL#11862 - FR3_2
cluster.removeInstance(__sandbox_uri2, {force: false});

//@<OUT> Removing instance (interactive: true, force: true)
// WL#11862 - FR3_3
cluster.removeInstance(__sandbox_uri3, {force: true});

//@ Adding instance on port2 back (interactive: false, force: false)
// WL#11862 - FR3_4
shell.options.useWizards=0;
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Adding instance on port3 back (interactive: false, force: true)
// WL#11862 - FR3_4
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Removing instance (interactive: false, force: false)
// WL#11862 - FR3_4
cluster.removeInstance(__sandbox_uri2, {force: false});

//@<OUT> Removing instance (interactive: false, force: true)
// WL#11862 - FR3_4
cluster.removeInstance(__sandbox_uri3, {force: true});
shell.options.useWizards=1;

//@ Adding instance on port2 back (force: false)
// WL#11862 - FR5_1
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Adding instance on port3 back (force: true)
// WL#11862 - FR5_2
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Removing instance (force: false)
// WL#11862 - FR5_1
cluster.removeInstance(__sandbox_uri2, {force: false});

//@<OUT> Instance (force: false) does not exist in cluster metadata
// WL#11862 - FR5_1
host_exist_in_metadata_schema(__mysql_sandbox_port2);

//@<OUT> Removing instance (force: true)
// WL#11862 - FR5_2
cluster.removeInstance(__sandbox_uri3, {force: true});

//@<OUT> Instance (force: true) does not exist in cluster metadata
// WL#11862 - FR5_2
host_exist_in_metadata_schema(__mysql_sandbox_port3);

//@<OUT> Number of instance according to GR after removal.
// Regression for BUG#26796118 : INSTANCE REJOINS GR GROUP AFTER REMOVEINSTANCE() AND RESTART
print_instances_count_for_gr();

//@<> Remove instance post actions, to ensure it does not rejoin cluster {VER(<8.0.11)}
testutil.changeSandboxConf(__mysql_sandbox_port2, 'group_replication_start_on_boot', 'OFF');

//@ Stop instance on port2.
// Regression for BUG#26796118 : INSTANCE REJOINS GR GROUP AFTER REMOVEINSTANCE() AND RESTART
testutil.stopSandbox(__mysql_sandbox_port2);

//@ Restart instance on port2.
// Regression for BUG#26796118 : INSTANCE REJOINS GR GROUP AFTER REMOVEINSTANCE() AND RESTART
testutil.startSandbox(__mysql_sandbox_port2);

//@ Connect to restarted instance3.
// WL#11862 - FR5_2
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri3);

//@<OUT> Removed instance3 does not exist on its Metadata.
// Regression for BUG#27817894: REMOVEINSTANCE DOES NOT REMOVE TARGET INSTANCE FROM METADATA
// WL#11862 - FR5_2
exist_in_metadata_schema();

//@ Connect to restarted instance2.
// WL#11862 - FR5_1
session.close();
shell.connect(__sandbox_uri2);

//@<OUT> Removed instance2 does not exist on its Metadata.
// Regression for BUG#27817894: REMOVEINSTANCE DOES NOT REMOVE TARGET INSTANCE FROM METADATA
// WL#11862 - FR5_1
exist_in_metadata_schema();

//@<OUT> Confirm that GR start on boot is disabled {VER(>=8.0.11)}.
// Regression for BUG#26796118 : INSTANCE REJOINS GR GROUP AFTER REMOVEINSTANCE() AND RESTART
// NOTE: Cannot count instance for GR due to a SET PERSIST bug (BUG#26495619).
// This test check is only valid for server version >= 8.0.11.
print(get_sysvar(session, "group_replication_start_on_boot"));

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

//@ Adding instance on port3 back
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Reset persisted gr_start_on_boot on instances {VER(>=8.0.11)}
// NOTE: This trick is required to reuse unreachable servers in tests and avoid
// them from automatically rejoining the group.
// Changing the my.cnf is not enough since persited variables have precedence.
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri2);
session.runSql("RESET PERSIST group_replication_start_on_boot");
session.close();
shell.connect(__sandbox_uri3);
session.runSql("RESET PERSIST group_replication_start_on_boot");
session.close();
shell.connect(__sandbox_uri1);
var cluster = dba.getCluster('dev');

//@ Stop instance on port2
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
testutil.stopSandbox(__mysql_sandbox_port2);

// Waiting for the instance on port2 to be found missing
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@ Stop instance on port3
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@<OUT> Cluster status after instance on port2 is stopped
cluster.status();

//@ Error removing stopped instance on port2 using alternative host not in Metadata (no prompt)
// BUG#30580393: error focusing on instance not being reachable (in MD but registered with a different host).
cluster.removeInstance('root:root@'+hostname_ip+':' + __mysql_sandbox_port2);

//@ Error removing stopped instance on port2 using alternative host not in Metadata and wrong pwd (no prompt)
// BUG#30580393: error focusing on instance not being reachable, wrong credentials (in MD but registered with a different host).
cluster.removeInstance('root:wrong@'+hostname_ip+':' + __mysql_sandbox_port2);

//@ Error removing stopped instance on port2 (no prompt if interactive is false)
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
shell.options.useWizards=0;
cluster.removeInstance('root:root@'+hostname+':' + __mysql_sandbox_port2);
shell.options.useWizards=1;

//@ Error removing stopped instance on port2 (no prompt if force is used)
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
cluster.removeInstance('root:root@'+hostname+':' + __mysql_sandbox_port2, {force: false});

//@ Error removing stopped instance on port2
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)?", "n");
cluster.removeInstance('root:root@'+hostname+':' + __mysql_sandbox_port2);

//@ Remove stopped instance on port2 with force option
// Regression for BUG#24916064 : CAN NOT REMOVE STOPPED SERVER FROM A CLUSTER
// BUG#26986141 : REMOVEINSTANCE DOES NOT ACCEPT A SECOND PARAMETER
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)?", "y");
cluster.removeInstance('root:root@'+hostname+':' + __mysql_sandbox_port2);

//@<OUT> Confirm instance2 is not in cluster metadata
// WL#11862 - FR5_3
host_exist_in_metadata_schema(__mysql_sandbox_port2);

//@ Remove unreachable instance (interactive: false, force: false) - error
// WL#11862 - FR4_5
shell.options.useWizards=0;
cluster.removeInstance(__hostname_uri3, {force: false});
shell.options.useWizards=1;

//@ Remove unreachable instance (interactive: false, force: true) - success
// WL#11862 - FR4_5
shell.options.useWizards=0;
cluster.removeInstance(__hostname_uri3, {force: true});
shell.options.useWizards=1;

//@ remove instance not in MD and unreachable, interactive true (should fail)
// BUG#30580393: error focusing on instance not being reachable (since it could be in MD with a different host).
cluster.removeInstance(__hostname_uri3);

//@ remove instance not in MD and unreachable, interactive false (should fail)
// BUG#30580393: error focusing on instance not being reachable (since it could be in MD with a different host).
shell.options.useWizards=0;
cluster.removeInstance(__hostname_uri3);
shell.options.useWizards=1;

//@<OUT> Cluster status after removal of instance on port2 and port3
cluster.status();

//@ Remove instances post actions since remove when unreachable (ensure they do not rejoin cluster)
// NOTE: This is not enough for server version >= 8.0.11 because persisted variables take precedence.
testutil.changeSandboxConf(__mysql_sandbox_port2, 'group_replication_start_on_boot', 'OFF');
testutil.changeSandboxConf(__mysql_sandbox_port3, 'group_replication_start_on_boot', 'OFF');

//@ Restart instance on port2 again.
testutil.startSandbox(__mysql_sandbox_port2);

//@ Restart instance on port3 again.
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitSandboxAlive(__mysql_sandbox_port3);

//@ remove reachable instance but not MD, interactive false (should fail)
// BUG#30580393: only issue error indicating that instance does not belong to replicaset when reachable.
shell.options.useWizards=0;
cluster.removeInstance(__hostname_uri3);
shell.options.useWizards=1;

//@ remove reachable instance but not MD, interactive true (should fail)
// BUG#30580393: only issue error indicating that instance does not belong to replicaset when reachable.
cluster.removeInstance(__hostname_uri3);

//@ Connect to instance2 (removed unreachable)
// WL#11862 - FR5_3
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri2);

//@<OUT> Confirm instance2 is still in its metadata
// WL#11862 - FR5_3
exist_in_metadata_schema();

//@ Connect to seed instance and get cluster again
session.close();
shell.connect(__sandbox_uri1);
var cluster = dba.getCluster('dev');

//@ Adding instance on port2 again
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Reset persisted gr_start_on_boot on instance2 {VER(>=8.0.11)}
// NOTE: This trick is required to reuse unreachable servers in tests and avoid
// them from automatically rejoining the group.
// Changing the my.cnf is not enough since persited variables have precedence.
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri2);
session.runSql("RESET PERSIST group_replication_start_on_boot");
session.close();
shell.connect(__sandbox_uri1);
var cluster = dba.getCluster('dev');

//@ Stop instance on port2 again
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@ Adding instance on port3 again
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ Stop instance on port3 again
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@<> Remove unreachable instance (interactive: true, force: false) - error
// WL#11862 - FR4_3
cluster.removeInstance(__hostname_uri2, {force: false});

//@<> Remove unreachable instance (interactive: false) - error
// WL#11862 - FR4_4
shell.options.useWizards=0;
EXPECT_THROWS(function() { cluster.removeInstance(__hostname_uri3); }, ["Cluster.removeInstance: Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port3)>>>'", "Cluster.removeInstance: Lost connection to MySQL server at "]);
shell.options.useWizards=1;

EXPECT_STDOUT_CONTAINS_ONE_OF(["WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port3)>>>'", "WARNING: MySQL Error 2013 (HY000): Lost connection to MySQL server at "])
EXPECT_STDOUT_CONTAINS_MULTILINE("ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' is not reachable and cannot be safely removed from the cluster.\nTo safely remove the instance from the Cluster, make sure the instance is back ONLINE and try again. If you are sure the instance is permanently unable to rejoin the Cluster and no longer connectable, use the 'force' option to remove it from the metadata.");

//@<> Remove unreachable instance (interactive: true, answer NO) - error
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)?", "n");
cluster.removeInstance(__hostname_uri2);

//@<> Remove unreachable instance (interactive: true, answer YES) - success
// WL#11862 - FR4_1
testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)?", "y");
cluster.removeInstance(__hostname_uri2);

//@<> Remove unreachable instance (interactive: true, force: true) - success
// WL#11862 - FR4_2
cluster.removeInstance(__hostname_uri3, {force: true});

//@ Remove instance post actions (ensure they do not rejoin cluster)
testutil.changeSandboxConf(__mysql_sandbox_port2, 'group_replication_start_on_boot', 'OFF');

//@ Restart instance on port2 one last time
testutil.startSandbox(__mysql_sandbox_port2);

//@ Adding instance on port2 one last time
// WL#11862 - FR7_1 and F8_1
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Connect to instance2 to introduce a replication error
// WL#11862 - FR7_1 and F8_1
session.close();
cluster.disconnect();
shell.connect(__sandbox_uri2);

//@ Introduce a replication error at instance 2
// WL#11862 - FR7_1 and F8_1
session.runSql("SET sql_log_bin = 0");
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("DROP DATABASE mysql_innodb_cluster_metadata");
session.runSql("SET GLOBAL super_read_only = 1");
session.runSql("SET sql_log_bin = 1");

//@ Connect to seed instance and get cluster on last time
// WL#11862 - FR7_1 and F8_1
session.close();
shell.connect(__sandbox_uri1);
var cluster = dba.getCluster('dev');

//@ Change shell option dba.gtidWaitTimeout to 5 second
// WL#11862 - FR7_1
shell.options["dba.gtidWaitTimeout"] = 5;

//@<> Remove instance with replication error - error
// WL#11862 - FR7_1
cluster.removeInstance(__sandbox_uri2);

//@ Change shell option dba.gtidWaitTimeout to 10 second
// WL#11862 - FR8_1
shell.options["dba.gtidWaitTimeout"] = 10;

//@<> Remove instance with replication error (force: true) - success
// WL#11862 - FR8_1
cluster.removeInstance(__sandbox_uri2, {force: true});

//@ Error removing last instance
// Regression for BUG#25226130 : REMOVAL OF SEED NODE BREAKS DISSOLVE
cluster.removeInstance('root:root@localhost:' + __mysql_sandbox_port1);

//@ Dissolve cluster with success
// Regression for BUG#25226130 : REMOVAL OF SEED NODE BREAKS DISSOLVE
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
cluster.dissolve({force: true});

cluster.disconnect();

// We must use clearReadOnly because the cluster was dissolved
// (BUG#26422638)

//@ Cluster re-created with success
// Regression for BUG#25226130 : REMOVAL OF SEED NODE BREAKS DISSOLVE
var cluster = dba.createCluster('dev', {gtidSetIsComplete: true});

session.close();
cluster.disconnect();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
// Note: set quiet kill to true when destroying a sandbox to avoid
//       displaying expected errors, since it was already stopped previously in
//       the test.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2, true);
testutil.destroySandbox(__mysql_sandbox_port3, true);
