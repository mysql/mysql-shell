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

function validate_instance_status(status, instance, expected) {
    topology = status["defaultReplicaSet"]["topology"];
    if (expected == "N/A") {
        EXPECT_FALSE(`${hostname}:${instance}` in topology);
    } else {
        instance_status = topology[`${hostname}:${instance}`];
        if (expected == "OK") {
            EXPECT_EQ(instance_status["status"], "ONLINE");
            EXPECT_FALSE("instanceErrors" in instance_status);
        } else if (expected == "UNMANAGED") {
            EXPECT_EQ(instance_status["status"], "ONLINE");
            EXPECT_TRUE("instanceErrors" in instance_status);
            EXPECT_EQ(instance_status["instanceErrors"][0],
                "WARNING: Instance is not managed by InnoDB cluster. Use cluster.rescan() to repair.");
        } else if (expected == "MISSING" ) {
            EXPECT_EQ(instance_status["status"], "(MISSING)");
            EXPECT_FALSE("instanceErrors" in instance_status);
        }
    }
}

function validate_status(status, instance_data) {
    instance_data.forEach(instance=>
        validate_instance_status(status, instance[0], instance[1]))
}

function reboot_with_view_change_uuid_default(cluster) {
    var instances = [];
    var ports = [];
    var cluster_status = cluster.status();
    var cluster_name = cluster_status["clusterName"];
    var primary = cluster_status["defaultReplicaSet"]["primary"];

    for (var instance in cluster_status["defaultReplicaSet"]["topology"]) {
        instances.push(instance);
    }

    for (var instance of instances) {
        var session = mysql.getSession("mysql://root:root@"+instance);
        session.runSql("SET PERSIST group_replication_view_change_uuid=DEFAULT");
        var row = session.runSql("select @@port").fetchOne();
        ports.push(row[0]);
    }

    primary_session = mysql.getSession("mysql://root:root@"+primary);
    primary_session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = json_remove(attributes, '$.group_replication_view_change_uuid')");

    for (var port of ports) {
        testutil.killSandbox(port);
    }

    for (var port of ports) {
        testutil.startSandbox(port);
    }

    // Remove the primary from the instances list
    var index = instances.indexOf(primary);
    if (index !== -1) {
        instances.splice(index, 1);
    }

    cluster = dba.rebootClusterFromCompleteOutage(cluster_name, {rejoinInstances: [instances.join(",")]});
}

//@ Initialize.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {server_uuid: "cd93e780-b558-11ea-b3de-0242ac130004", report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {server_uuid: "cd93e9a6-b558-11ea-b3de-0242ac130004", report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
var mycnf_path2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {server_uuid: "cd93eab4-b558-11ea-b3de-0242ac130004", report_host: hostname});
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

//@<> Create cluster.
shell.connect(__hostname_uri1);
var cluster = dba.createCluster("c", {gtidSetIsComplete: true});
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port2, "OK"]]);

//@ No-op - Still missing server_id attributes are added
var initial_atts = session.runSql(`SELECT attributes->'$.server_id' AS server_id FROM mysql_innodb_cluster_metadata.instances WHERE address = '${hostname}:${__mysql_sandbox_port3}'`).fetchOneObject();
session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances set attributes=JSON_REMOVE(attributes, '$.server_id') WHERE address= '${hostname}:${__mysql_sandbox_port3}'`);
cluster.rescan();
var final_atts = session.runSql(`SELECT attributes->'$.server_id' AS server_id FROM mysql_innodb_cluster_metadata.instances WHERE address = '${hostname}:${__mysql_sandbox_port3}'`).fetchOneObject();
EXPECT_EQ(initial_atts.server_id, final_atts.server_id);


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

//@<> Remove instance on port 2 and 3 from MD but keep it in the group.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [hostname+":"+__mysql_sandbox_port2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [hostname+":"+__mysql_sandbox_port3]);

validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "UNMANAGED"],
                                   [__mysql_sandbox_port3, "UNMANAGED"]]);

//@<> WL10644 - TSF2_1: Rescan with addInstances:[complete_valid_list].
var member_address2 = hostname + ":" + __mysql_sandbox_port2;
var member_address3 = hostname + ":" + __mysql_sandbox_port3;
cluster.rescan({addInstances: [member_address2, member_address3]});

//@<> WL10644 - TSF2_1: Validate that the instances were added.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

//@<> WL10644 - TSF2_2: Remove instances on port 2 and 3 from MD again.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address3]);

validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "UNMANAGED"],
                                   [__mysql_sandbox_port3, "UNMANAGED"]]);

//@<> WL10644 - TSF2_2: Rescan with addInstances:[incomplete_valid_list] and interactive:true.
testutil.expectPrompt("Would you like to add it to the cluster metadata? [Y/n]:", "y");
cluster.rescan({addInstances: [member_address2], interactive: true});

//@<> WL10644 - TSF2_2: Validate that the instances were added.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

//@<> WL10644 - TSF2_3: Remove instances on port 2 and 3 from MD again.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address3]);

validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "UNMANAGED"],
                                   [__mysql_sandbox_port3, "UNMANAGED"]]);

//@<> WL10644 - TSF2_3: Rescan with addInstances:[incomplete_valid_list] and interactive:false.
cluster.rescan({addInstances: [member_address2], interactive: false});

//@<> WL10644 - TSF2_3: Validate that the instances were added.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "UNMANAGED"]]);

//@<> WL10644 - TSF2_4: Remove instances on port 2 from MD.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address2]);
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "UNMANAGED"],
                                   [__mysql_sandbox_port3, "UNMANAGED"]]);

//@<> WL10644 - TSF2_4: Rescan with addInstances:"auto" and interactive:true.
cluster.rescan({addInstances: "AUTO", interactive: true});

//@<> WL10644 - TSF2_4: Validate that the instances were added.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

//@<> WL10644 - TSF2_5: Remove instances on port 2 and 3 from MD again.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address3]);
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "UNMANAGED"],
                                   [__mysql_sandbox_port2, "UNMANAGED"]]);

//@<> WL10644 - TSF2_5: Rescan with addInstances:"auto" and interactive:false.
cluster.rescan({addInstances: "auto", interactive: false});

//@<> WL10644 - TSF2_5: Validate that the instances were added.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

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
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "MISSING"],
                                   [__mysql_sandbox_port3, "MISSING"]]);

//@<> WL10644 - TSF3_1: Number of instances in the MD before rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_1: Rescan with removeInstances:[complete_valid_list].
cluster.rescan({removeInstances: [member_address2, member_address3]});

//@<> WL10644 - TSF3_1: Number of instances in the MD after rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_1: Validate that the instances were removed.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "N/A"]]);

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
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "MISSING"],
                                   [__mysql_sandbox_port3, "MISSING"]]);

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
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "N/A"]]);

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
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "MISSING"],
                                   [__mysql_sandbox_port3, "MISSING"]]);

//@<> WL10644 - TSF3_3: Number of instances in the MD before rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_3: Rescan with removeInstances:[incomplete_valid_list] and interactive:false.
cluster.rescan({removeInstances: [member_fqdn_address2], interactive: false});

//@<> WL10644 - TSF3_3: Number of instances in the MD after rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_3: Validate that the instances were removed.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "MISSING"]]);

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
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "MISSING"],
                                   [__mysql_sandbox_port3, "MISSING"]]);

//@<> WL10644 - TSF3_4: Number of instances in the MD before rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_4: Rescan with removeInstances:"auto" and interactive:true.
cluster.rescan({removeInstances: "auto", interactive: true});

//@<> WL10644 - TSF3_4: Number of instances in the MD after rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_4: Validate that the instances were removed.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "N/A"]]);

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
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "MISSING"],
                                   [__mysql_sandbox_port3, "MISSING"]]);

//@<> WL10644 - TSF3_5: Number of instances in the MD before rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_5: Rescan with removeInstances:"auto" and interactive:false.
cluster.rescan({removeInstances: "AUTO", interactive: false});

//@<> WL10644 - TSF3_5: Number of instances in the MD after rescan().
count_in_metadata_schema();

//@<> WL10644 - TSF3_5: Validate that the instances were removed.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "N/A"]]);

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

//@ WL10644 - TSF4_5: Set auto_increment settings to unused values.
set_auto_increment_to_unused_values(__sandbox_uri1);
set_auto_increment_to_unused_values(__sandbox_uri2);
set_auto_increment_to_unused_values(__sandbox_uri3);

//@ WL10644 - TSF4_2: status() error because topology mode changed.
// NOTE: dba.getCluster() needed for new metadata info to be loaded by cluster object.
cluster = dba.getCluster();
cluster.status();

//BUG#29330769: UPDATETOPOLOGYMODE SHOULD DEFAULT TO TRUE

//@<> BUG#29330769: Change the topology mode in the MD to the wrong value again
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = 'mm'");

//@<> BUG#29330769: Topology mode in MD before rescan().
get_metadata_topology_mode();

//@<> BUG#29330769: Rescan without using updateTopologyMode and change needed.
cluster.rescan();

//@<> BUG#29330769: Check topology mode in MD after rescan().
get_metadata_topology_mode();

//@<> BUG#29330769: Verify deprecation message added about updateTopologyMode
cluster.rescan({updateTopologyMode: true});

//@<> WL10644 - TSF4_2: status() succeeds after rescan() updates topology mode.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

//@<> WL10644 - TSF4_5: Check auto_increment settings after change to single-primary.
check_auto_increment_settings(__sandbox_uri1);
check_auto_increment_settings(__sandbox_uri2);
check_auto_increment_settings(__sandbox_uri3);

//@<> Create multi-primary cluster.
// NOTE: Cluster re-created for test to work with both 5.7 and 8.0 servers.
cluster.dissolve();
var cluster = dba.createCluster("c", {multiPrimary: true, clearReadOnly: true, force: true, gtidSetIsComplete: true, manualStartOnBoot: true});
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

//@ WL10644 - TSF4_3: Change the topology mode in the MD to the wrong value.
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = 'pm'");

//@<> WL10644 - TSF4_3: Topology mode in MD before rescan().
get_metadata_topology_mode();

//@ WL10644 - TSF4_6: Set auto_increment settings to unused values.
set_auto_increment_to_unused_values(__sandbox_uri1);
set_auto_increment_to_unused_values(__sandbox_uri2);
set_auto_increment_to_unused_values(__sandbox_uri3);

//@<> WL10644 - TSF4_3: Rescan with interactive:true and change needed.
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

// Tests for cluster.rescan() handling of group_replication_view_change_uuid:
//
//  - When the option 'updateViewChangeUuid' is not used:
//    - Check if a Warning is printed when cluster is running a version >=
//      8.0.27 and group_replication_view_change_uuid=AUTOMATIC
//    - Check if a prompt is presented to the user when in interactive mode
//    - Check that a note is printed to use 'updateViewChangeUuid' when not in //      interactive mode
//  - When the option 'updateViewChangeUuid' is used:
//    - Check that a new value is generated and set in the whole cluster and
//      information is printed about it and the required reboot of the cluster
//  - For both cases, if a new value is generated, confirm the metadata is
//    updated

//@<> Cluster.rescan() - view_change_uuid handling {VER(>=8.0.27)}

// Reduce the cluster to 2 members to speed-up and switch to single-primary mode
cluster.removeInstance(__sandbox_uri3);
cluster.switchToSinglePrimaryMode();

// Reset the value of view_change_uuid to the default since createCluster will generate a value for it if the target is >= 8.0.27
reboot_with_view_change_uuid_default(cluster);

// Get a fresh cluster handle
cluster = dba.getCluster();

// Disable interactive mode
shell.options["useWizards"] = false;

// Validate output with interactive disabled
EXPECT_NO_THROWS(function() { cluster.rescan(); });

EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster is not configured to use 'group_replication_view_change_uuid', which is required for InnoDB ClusterSet. Configuring it requires a full Cluster reboot.");
EXPECT_OUTPUT_CONTAINS("Use the 'updateViewChangeUuid' option to generate and configure a value for the Cluster.");
WIPE_STDOUT();

// Use the option and validate it's successful
EXPECT_NO_THROWS(function() { cluster.rescan({updateViewChangeUuid: true}); });

EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is not set");
EXPECT_OUTPUT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
EXPECT_OUTPUT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");
WIPE_STDOUT();

// Do not restart yet and confirm that cluster.rescan() detects a restart is needed for the change to be effective
EXPECT_NO_THROWS(function() { cluster.rescan({updateViewChangeUuid: true}); });
EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is set but not yet effective");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
WIPE_STDOUT();

// Restart the instances and reboot it from complete outage after rescan() changed group_replication_view_change_uuid
testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage("c", {rejoinInstances: [__endpoint2]});

var view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
EXPECT_NE(view_change_uuid, "AUTOMATIC");

var view_change_uuid_md = session.runSql("select (attributes->>'$.group_replication_view_change_uuid') from mysql_innodb_cluster_metadata.clusters where cluster_name='c'").fetchOne()[0];
EXPECT_EQ(view_change_uuid, view_change_uuid_md);

// Shutdown the cluster and reboot it from complete outage after rescan() changed group_replication_view_change_uuid
// BUG#33850528: shutting down the cluster can be done by just stopping group replication and not by restarting the instances. Since the persisted settings are only applied when a restart of the server happens, rebootClusterFromCompleteOutage() must consider that and query performance_schema.persisted_variables to obtain the right value of view_change_uuid to be used

// Reboot the cluster to use back AUTOMATIC
reboot_with_view_change_uuid_default(cluster);

shell.connect(__sandbox_uri1);
cluster = dba.getCluster()
EXPECT_NO_THROWS(function() { cluster.rescan({updateViewChangeUuid: true}); });
WIPE_STDOUT();

shell.connect(__sandbox_uri2);
session.runSql("stop group_replication");

shell.connect(__sandbox_uri1);
session.runSql("stop group_replication");

cluster = dba.rebootClusterFromCompleteOutage("c", {rejoinInstances: [__endpoint2]});

var view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
EXPECT_NE(view_change_uuid, "AUTOMATIC");

var view_change_uuid_md = session.runSql("select (attributes->>'$.group_replication_view_change_uuid') from mysql_innodb_cluster_metadata.clusters where cluster_name='c'").fetchOne()[0];
EXPECT_EQ(view_change_uuid, view_change_uuid_md);

// cluster.rescan() must not warn or act if the target cluster has group_replication_view_change_uuid set
EXPECT_NO_THROWS(function() { cluster.rescan(); });
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is not set");
EXPECT_OUTPUT_NOT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
EXPECT_OUTPUT_NOT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");
WIPE_STDOUT();

// Reboot the cluster to use back AUTOMATIC
reboot_with_view_change_uuid_default(cluster);

shell.connect(__sandbox_uri1);
cluster = dba.getCluster()

// Validate the prompt with interactive enabled and accept the change
shell.options["useWizards"] = true;

testutil.expectPrompt("Would you like 'group_replication_view_change_uuid' to be configured automatically?", "y");

EXPECT_NO_THROWS(function() { cluster.rescan(); });

EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster is not configured to use 'group_replication_view_change_uuid', which is required for InnoDB ClusterSet. Configuring it requires a full Cluster reboot.");

EXPECT_OUTPUT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_CONTAINS("WARNING: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
EXPECT_OUTPUT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");
WIPE_STDOUT();

// Restart the instances and reboot it from complete outage after rescan() changed group_replication_view_change_uuid
testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage("c", {rejoinInstances: [__endpoint2]});

var view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
EXPECT_NE(view_change_uuid, "AUTOMATIC");

var view_change_uuid_md = session.runSql("select (attributes->>'$.group_replication_view_change_uuid') from mysql_innodb_cluster_metadata.clusters where cluster_name='c'").fetchOne()[0];
EXPECT_EQ(view_change_uuid, view_change_uuid_md);

// Confirm cluster.rescan() does not warn or act if the target cluster has group_replication_view_change_uuid set
EXPECT_NO_THROWS(function() { cluster.rescan(); });
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is not set");
EXPECT_OUTPUT_NOT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
EXPECT_OUTPUT_NOT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");


//@<> check metadata repair when report_host is changed  {VER(>=8.0.27)}
shell.connect(__sandbox_uri1);
reset_instance(session);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {server_uuid: instance3_uuid, server_id: instance3_id, report_host: hostname_ip});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

c = dba.createCluster("cluster" , {gtidSetIsComplete:1});
c.addInstance(__sandbox_uri3);
dba.configureLocalInstance(__sandbox_uri3);
c.status();

shell.connect(__sandbox_uri3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "report_host", hostname);
testutil.restartSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);
c.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

s = c.status();

EXPECT_EQ(`ERROR: Metadata for this instance does not match hostname reported by instance (metadata=${hostname_ip}:${__mysql_sandbox_port3}, actual=${hostname}:${__mysql_sandbox_port3}). Use rescan() to update the metadata.`, s["defaultReplicaSet"]["topology"][hostname_ip+":"+__mysql_sandbox_port3]["instanceErrors"][0])

EXPECT_EQ(hostname_ip+":"+__mysql_sandbox_port3, s["defaultReplicaSet"]["topology"][hostname_ip+":"+__mysql_sandbox_port3]["address"]);

row = session.runSql("select * from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=?", [instance3_uuid]).fetchOne();
EXPECT_EQ(hostname_ip+":"+__mysql_sandbox_port3, row.address);
EXPECT_EQ(hostname_ip+":"+__mysql_sandbox_port3, row.instance_name);
EXPECT_EQ( {"mysqlX": hostname_ip+":"+__mysql_sandbox_port3+"0", "grLocal": hostname_ip+":"+__mysql_sandbox_port3+"1", "mysqlClassic": hostname_ip+":"+__mysql_sandbox_port3}, JSON.parse(row.addresses));

//@<> setPrimary is expected to throw b/c metadata doesn't match {VER(>=8.0.27)}
EXPECT_THROWS(function(){c.setPrimaryInstance(__sandbox_uri3);}, `The instance 'localhost:${__mysql_sandbox_port3}' does not belong to the cluster: 'cluster'.`);

//@<> rescan to repair metadata according to new report_host {VER(>=8.0.27)}

// sb3 address should be fixed now
c.rescan();
row = session.runSql("select * from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=?", [instance3_uuid]).fetchOne();
EXPECT_EQ(hostname+":"+__mysql_sandbox_port3, row.address);
EXPECT_EQ(hostname+":"+__mysql_sandbox_port3, row.instance_name);
EXPECT_EQ( {"mysqlX": hostname+":"+__mysql_sandbox_port3+"0", "grLocal": hostname_ip+":"+__mysql_sandbox_port3+"1", "mysqlClassic": hostname+":"+__mysql_sandbox_port3}, JSON.parse(row.addresses));

s = c.status();

EXPECT_EQ(hostname+":"+__mysql_sandbox_port3, s["defaultReplicaSet"]["topology"][hostname+":"+__mysql_sandbox_port3]["address"]);

//@<> ensure setPrimary works now {VER(>=8.0.27)}
c.setPrimaryInstance(__sandbox_uri3);
c.setPrimaryInstance(__sandbox_uri1);

//@<> check that label is not changed if it's not the default  {VER(>=8.0.27)}
c.setInstanceOption(hostname+":"+__mysql_sandbox_port3, "label", "hooray");

shell.connect(__sandbox_uri3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "report_host", hostname_ip);
testutil.restartSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);
c.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

c.rescan();
s = c.status();
EXPECT_EQ(undefined, s["defaultReplicaSet"]["topology"][hostname_ip+":"+__mysql_sandbox_port3]);
EXPECT_EQ(hostname_ip+":"+__mysql_sandbox_port3, s["defaultReplicaSet"]["topology"]["hooray"]["address"]);


//@ Finalize.
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
