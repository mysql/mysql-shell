function get_server_uuid(session) {
    var result = session.runSql("SELECT @@GLOBAL.server_uuid");
    var row = result.fetchOne();
    return row[0];
}

function get_server_id(session) {
    var result = session.runSql("SELECT @@GLOBAL.server_id");
    var row = result.fetchOne();
    return row[0];
}

function count_in_metadata_schema() {
    var result = session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances");
    var row = result.fetchOne();
    return row[0];
}

function get_metadata_topology_mode() {
    var result = session.runSql("SELECT primary_mode FROM mysql_innodb_cluster_metadata.clusters");
    var row = result.fetchOne();
    return row[0];
}

function set_auto_increment_to_unused_values(uri) {
    var s = mysql.getSession(uri);
    s.runSql("SET @@GLOBAL.auto_increment_increment = 99");
    s.runSql("SET @@GLOBAL.auto_increment_offset = 99");
    s.close();
}

function check_auto_increment_settings(uri) {
    var s = mysql.getSession(uri);
    var result = s.runSql("SELECT @@GLOBAL.auto_increment_increment");
    var row = result.fetchOne();
    print("auto_increment_increment: " + row[0] +"\n");
    result = s.runSql("SELECT @@GLOBAL.auto_increment_offset");
    row = result.fetchOne();
    print("auto_increment_offset: " + row[0] +"\n");
    s.close();
}

//@ Initialize.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
var mycnf_path2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
var mycnf_path3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);

//@ Get needed Server_Ids and UUIDs.
shell.connect(__sandbox_uri1);
instance1_id = get_server_id(session);
shell.connect(__sandbox_uri2);
instance2_uuid = get_server_uuid(session);
instance2_id = get_server_id(session);
session.close();
shell.connect(__sandbox_uri3);
instance3_uuid = get_server_uuid(session);
instance3_id = get_server_id(session);
session.close();

//@ Configure sandboxes.
dba.configureInstance(__sandbox_uri1, {clusterAdmin:'root', mycnfPath: mycnf_path1});
dba.configureInstance(__sandbox_uri2, {clusterAdmin:'root', mycnfPath: mycnf_path2});
dba.configureInstance(__sandbox_uri3, {clusterAdmin:'root', mycnfPath: mycnf_path3});

//@ Create cluster.
shell.connect(__hostname_uri1);
var cluster = dba.createCluster("c", {gtidSetIsComplete: true});
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
cluster.status();

//@ No-op.
cluster.rescan();

//@ WL10644 - TSF2_6: empty addInstances throw ArgumentError.
cluster.rescan({addInstances: []});

//@ WL10644 - TSF2_8: invalid addInstances list throw ArgumentError.
cluster.rescan({addInstances: [,]});
cluster.rescan({addInstances: ["localhost"]});
cluster.rescan({addInstances: ["localhost:3300", ":3301"]});
cluster.rescan({addInstances: ["localhost:3300", "@", "myhost:3301"]});
cluster.rescan({addInstances: [{}]});
cluster.rescan({addInstances: [{host: "myhost"}]});
cluster.rescan({addInstances: [{host: "myhost", port:3300}, {host: ""}, {host: "localhost", port:3301}]});

//@ WL10644: Duplicated values for addInstances.
cluster.rescan({addInstances: ["localhost:3301", "localhost:3300", "localhost:3301"]});
cluster.rescan({addInstances: [{host: "localhost", port: "3301", user: "root"}, {host: "localhost", port: "3300"}, {host: "localhost", port: "3301"}]});
cluster.rescan({addInstances: ["localhost:3301", "localhost:3300", {host: "localhost", port: "3301"}]});

//@ WL10644 - TSF2_9: invalid value with addInstances throw ArgumentError.
cluster.rescan({addInstances: "invalid"});

//@ WL10644 - TSF2_7: "auto" is case insensitive, no error.
cluster.rescan({addInstances: "AuTo"});

//@ WL10644: Invalid type used for addInstances.
cluster.rescan({addInstances: {}});
cluster.rescan({addInstances: true});
cluster.rescan({addInstances: 123});

//@<> WL10644 - TSF2_10: not active member in addInstances throw RuntimeError.
cluster.rescan({addInstances: ["localhost:1111"]});

//@<> WL10644 - TSF2_11: warning for already members in addInstances.
var member_address = hostname + ":" + __mysql_sandbox_port1;
cluster.rescan({addInstances: [member_address]});

//@ WL10644 - TSF3_6: empty removeInstances throw ArgumentError.
cluster.rescan({removeInstances: []});

//@ WL10644 - TSF3_8: invalid removeInstances list throw ArgumentError.
cluster.rescan({removeInstances: [,]});
cluster.rescan({removeInstances: ["localhost"]});
cluster.rescan({removeInstances: ["localhost:3300", ":3301"]});
cluster.rescan({removeInstances: ["localhost:3300", "@", "myhost:3301"]});
cluster.rescan({removeInstances: [{}]});
cluster.rescan({removeInstances: [{host: "myhost"}]});
cluster.rescan({removeInstances: [{host: "myhost", port:3300}, {port:3301}]});
cluster.rescan({removeInstances: [{host: "myhost", port:3300}, {user: "root"}, {host: "localhost", port:3301}]});
cluster.rescan({removeInstances: [{host: "myhost", port:3300}, {host: ""}, {host: "localhost", port:3301}]});

//@ WL10644: Duplicated values for removeInstances.
cluster.rescan({removeInstances: ["localhost:3301", "localhost:3300", "localhost:3301"]});
cluster.rescan({removeInstances: [{host: "localhost", port: "3301", user: "root"}, {host: "localhost", port: "3300"}, {host: "localhost", port: "3301"}]});
cluster.rescan({removeInstances: ["localhost:3301", "localhost:3300", {host: "localhost", port: "3301"}]});

//@ WL10644 - TSF3_9: invalid value with removeInstances throw ArgumentError.
cluster.rescan({removeInstances: "invalid"});

//@ WL10644 - TSF3_7: "auto" is case insensitive, no error.
cluster.rescan({removeInstances: "aUtO"});

//@ WL10644: Invalid type used for removeInstances.
cluster.rescan({removeInstances: {}});
cluster.rescan({removeInstances: true});
cluster.rescan({removeInstances: 123});

//@<> WL10644 - TSF3_10: active member in removeInstances throw RuntimeError.
cluster.rescan({removeInstances: [member_address]});

//@<> WL10644 - TSF3_11: warning for not members in removeInstances.
cluster.rescan({removeInstances: ["localhost:1111"]});

//@ WL10644: Duplicated values between addInstances and removeInstances.
cluster.rescan({addInstances: ["localhost:3300", "localhost:3302", "localhost:3301"], removeInstances: ["localhost:3301", "localhost:3300"]});
cluster.rescan({addInstances: [{host: "localhost", port: "3301", user: "root"}], removeInstances: [{host: "localhost", port: "3300"}, {host: "localhost", port: "3301"}]});
cluster.rescan({addInstances: ["localhost:3301", "localhost:3300"], removeInstances: [{host: "localhost", port: "3301"}]});

//@ Remove instance on port 2 and 3 from MD but keep it in the group.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [hostname+":"+__mysql_sandbox_port2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [hostname+":"+__mysql_sandbox_port3]);
cluster.status();

//@ addInstance should fail and suggest a rescan.
cluster.addInstance(__hostname_uri2);

//@<> WL10644 - TSF2_1: Rescan with addInstances:[complete_valid_list].
var member_address2 = hostname + ":" + __mysql_sandbox_port2;
var member_address3 = hostname + ":" + __mysql_sandbox_port3;
cluster.rescan({addInstances: [member_address2, member_address3]});

//@<> WL10644 - TSF2_1: Validate that the instances were added.
cluster.status();

//@<> WL10644 - TSF2_2: Remove instances on port 2 and 3 from MD again.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address3]);
cluster.status();

//@<> WL10644 - TSF2_2: Rescan with addInstances:[incomplete_valid_list] and interactive:true.
testutil.expectPrompt("Would you like to add it to the cluster metadata? [Y/n]:", "y");
cluster.rescan({addInstances: [member_address2], interactive: true});

//@<> WL10644 - TSF2_2: Validate that the instances were added.
cluster.status();

//@<> WL10644 - TSF2_3: Remove instances on port 2 and 3 from MD again.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address3]);
cluster.status();

//@<> WL10644 - TSF2_3: Rescan with addInstances:[incomplete_valid_list] and interactive:false.
cluster.rescan({addInstances: [member_address2], interactive: false});

//@<> WL10644 - TSF2_3: Validate that the instances were added.
cluster.status();

//@<> WL10644 - TSF2_4: Remove instances on port 2 from MD.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address2]);
cluster.status();

//@<> WL10644 - TSF2_4: Rescan with addInstances:"auto" and interactive:true.
cluster.rescan({addInstances: "AUTO", interactive: true});

//@<> WL10644 - TSF2_4: Validate that the instances were added.
cluster.status();

//@<> WL10644 - TSF2_5: Remove instances on port 2 and 3 from MD again.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address3]);
cluster.status();

//@<> WL10644 - TSF2_5: Rescan with addInstances:"auto" and interactive:false.
cluster.rescan({addInstances: "auto", interactive: false});

//@<> WL10644 - TSF2_5: Validate that the instances were added.
cluster.status();

//@ WL10644 - TSF3_1: Disable GR in persisted settings {VER(>=8.0.11)}.
//NOTE: GR configurations are not updated on my.cnf therefore GR settings
//      are lost when the server is stopped.
var s2 = mysql.getSession(__sandbox_uri2);
s2.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s2.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s2.close();
var s3 = mysql.getSession(__sandbox_uri3);
s3.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s3.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s3.close();

//@<> WL10644 - TSF3_1: Stop instances on port 2 and 3 (no metadata MD changes).
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.status();

//@<> WL10644 - TSF3_1: Number of instances in the MD before rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_1: Rescan with removeInstances:[complete_valid_list].
cluster.rescan({removeInstances: [member_address2, member_address3]});

//@<> WL10644 - TSF3_1: Number of instances in the MD after rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_1: Validate that the instances were removed.
cluster.status();

//@ WL10644 - TSF3_2: Start instances and add them back to the cluster.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ WL10644 - TSF3_2: Disable GR in persisted settings {VER(>=8.0.11)}.
//NOTE: GR configurations are not updated on my.cnf therefore GR settings
//      are lost when the server is stopped.
var s2 = mysql.getSession(__sandbox_uri2);
s2.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s2.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s2.close();
var s3 = mysql.getSession(__sandbox_uri3);
s3.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s3.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s3.close();

//@<> WL10644 - TSF3_2: Stop instances on port 2 and 3 (no metadata MD changes).
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.status();

//@<> WL10644 - TSF3_2: Number of instances in the MD before rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_2: Rescan with removeInstances:[incomplete_valid_list] and interactive:true.
testutil.expectPrompt("Would you like to remove it from the cluster metadata? [Y/n]:", "y");
var member_fqdn_address2 = hostname + ":" + __mysql_sandbox_port2;
var member_fqdn_address3 = hostname + ":" + __mysql_sandbox_port3;
cluster.rescan({removeInstances: [member_fqdn_address2], interactive: true});

//@<> WL10644 - TSF3_2: Number of instances in the MD after rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_2: Validate that the instances were removed.
cluster.status();

//@ WL10644 - TSF3_3: Start instances and add them back to the cluster.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ WL10644 - TSF3_3: Disable GR in persisted settings {VER(>=8.0.11)}.
//NOTE: GR configurations are not updated on my.cnf therefore GR settings
//      are lost when the server is stopped.
var s2 = mysql.getSession(__sandbox_uri2);
s2.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s2.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s2.close();
var s3 = mysql.getSession(__sandbox_uri3);
s3.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s3.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s3.close();

//@<> WL10644 - TSF3_3: Stop instances on port 2 and 3 (no metadata MD changes).
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.status();

//@<> WL10644 - TSF3_3: Number of instances in the MD before rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_3: Rescan with removeInstances:[incomplete_valid_list] and interactive:false.
cluster.rescan({removeInstances: [member_fqdn_address2], interactive: false});

//@<> WL10644 - TSF3_3: Number of instances in the MD after rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_3: Validate that the instances were removed.
cluster.status();

//@ WL10644 - TSF3_4: Start instance on port 2 and add it back to the cluster.
//NOTE: Not need for instance 3 (no removed from MD in previous test).
testutil.startSandbox(__mysql_sandbox_port2);
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ WL10644 - TSF3_4: Disable GR in persisted settings {VER(>=8.0.11)}.
//NOTE: GR configurations are not updated on my.cnf therefore GR settings
//      are lost when the server is stopped.
var s2 = mysql.getSession(__sandbox_uri2);
s2.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s2.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s2.close();

//@<> WL10644 - TSF3_4: Stop instances on port 2 (no metadata MD changes).
//NOTE: Not need for instance 3 (no removed from MD in previous test).
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
cluster.status();

//@<> WL10644 - TSF3_4: Number of instances in the MD before rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_4: Rescan with removeInstances:"auto" and interactive:true.
cluster.rescan({removeInstances: "auto", interactive: true});

//@<> WL10644 - TSF3_4: Number of instances in the MD after rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_4: Validate that the instances were removed.
cluster.status();

//@ WL10644 - TSF3_5: Start instances and add them back to the cluster.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ WL10644 - TSF3_5: Disable GR in persisted settings {VER(>=8.0.11)}.
//NOTE: GR configurations are not updated on my.cnf therefore GR settings
//      are lost when the server is stopped.
var s2 = mysql.getSession(__sandbox_uri2);
s2.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s2.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s2.close();
var s3 = mysql.getSession(__sandbox_uri3);
s3.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s3.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s3.close();

//@<> WL10644 - TSF3_5: Stop instances on port 2 and 3 (no metadata MD changes).
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.status();

//@<> WL10644 - TSF3_5: Number of instances in the MD before rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_5: Rescan with removeInstances:"auto" and interactive:false.
cluster.rescan({removeInstances: "AUTO", interactive: false});

//@<> WL10644 - TSF3_5: Number of instances in the MD after rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_5: Validate that the instances were removed.
cluster.status();

//@ WL10644: Start instances and add them back to the cluster again.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@ WL10644 - TSF4_1: Change the topology mode in the MD to the wrong value.
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = 'mm'");

//@<> WL10644 - TSF4_1: Topology mode in MD before rescan().
get_metadata_topology_mode();

//@<> WL10644 - TSF4_1: Rescan with updateTopologyMode:false and change needed.
cluster.rescan({updateTopologyMode: false});

//@<> WL10644 - TSF4_1: Check topology mode in MD after rescan().
get_metadata_topology_mode();

//@ WL10644 - TSF4_5: Set auto_increment settings to unused values.
set_auto_increment_to_unused_values(__sandbox_uri1);
set_auto_increment_to_unused_values(__sandbox_uri2);
set_auto_increment_to_unused_values(__sandbox_uri3);

//@<> WL10644 - TSF4_2: Topology mode in MD before rescan().
get_metadata_topology_mode();

//@<> WL10644 - TSF4_2: status() error because topology mode changed.
// NOTE: dba.getCluster() needed for new metadata info to be loaded by cluster object.
cluster = dba.getCluster();
cluster.status();

//@<> WL10644 - TSF4_2: Rescan with updateTopologyMode:true and change needed.
cluster.rescan({updateTopologyMode: true});

//@<> WL10644 - TSF4_2: Check topology mode in MD after rescan().
get_metadata_topology_mode();

//@<> WL10644 - TSF4_2: status() succeeds after rescan() updates topology mode.
cluster.status();

//@<> WL10644 - TSF4_5: Check auto_increment settings after change to single-primary.
check_auto_increment_settings(__sandbox_uri1);
check_auto_increment_settings(__sandbox_uri2);
check_auto_increment_settings(__sandbox_uri3);

//@<> Create multi-primary cluster.
// NOTE: Cluster re-created for test to work with both 5.7 and 8.0 servers.
cluster.dissolve();
var cluster = dba.createCluster("c", {multiPrimary: true, clearReadOnly: true, force: true, gtidSetIsComplete: true});
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
cluster.status();

//@ WL10644 - TSF4_3: Change the topology mode in the MD to the wrong value.
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = 'pm'");

//@<> WL10644 - TSF4_3: Topology mode in MD before rescan().
get_metadata_topology_mode();

//@ WL10644 - TSF4_6: Set auto_increment settings to unused values.
set_auto_increment_to_unused_values(__sandbox_uri1);
set_auto_increment_to_unused_values(__sandbox_uri2);
set_auto_increment_to_unused_values(__sandbox_uri3);

//@<> WL10644 - TSF4_3: Rescan with interactive:true and change needed.
testutil.expectPrompt("Would you like to update it in the cluster metadata? [Y/n]: ", "y");
cluster.rescan({interactive: true});

//@<> WL10644 - TSF4_3: Check topology mode in MD after rescan().
get_metadata_topology_mode();

//@<> WL10644 - TSF4_6: Check auto_increment settings after change to multi-primary.
offset1 = 1 + instance1_id % 7;
offset2 = 1 + instance2_id % 7;
offset3 = 1 + instance3_id % 7;
check_auto_increment_settings(__sandbox_uri1);
check_auto_increment_settings(__sandbox_uri2);
check_auto_increment_settings(__sandbox_uri3);

//@ WL10644 - TSF4_4: Change the topology mode in the MD to the wrong value.
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = 'pm'");

//@<> WL10644 - TSF4_4: Topology mode in MD before rescan().
get_metadata_topology_mode();

//@<> WL10644 - TSF4_4: Rescan with interactive:false and change needed.
cluster.rescan({interactive: false});

//@<> WL10644 - TSF4_4: Check topology mode in MD after rescan().
get_metadata_topology_mode();

//@ Finalize.
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
