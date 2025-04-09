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
        } else if ((expected == "RECOVERY_UNUSED") || (expected == "RECOVERY_UNUSED_SINGLE")) {
            EXPECT_EQ(instance_status["status"], "ONLINE");
            EXPECT_TRUE("instanceErrors" in instance_status);
            if (expected == "RECOVERY_UNUSED")
                EXPECT_CONTAINS("WARNING: Detected unused recovery accounts: ", instance_status["instanceErrors"][0]);
            else
                EXPECT_CONTAINS("WARNING: Detected an unused recovery account: ", instance_status["instanceErrors"][0]);
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

    cluster = dba.rebootClusterFromCompleteOutage(cluster_name);
}

function check_status_instance_error_msg(status, instance_port, msg) {

    inst = status["defaultReplicaSet"]["topology"][`${hostname}:${instance_port}`];
    if (!inst.hasOwnProperty('instanceErrors'))
        return false;

    inst_errors = inst["instanceErrors"];
    for (let i = 0; i < inst_errors.length; i++) {

        error_msg = inst_errors[i];
        if (error_msg === msg)
            return true;
    }

    return false;
}

function test_with_dtrap(dtrap, func) {
    testutil.dbugSet(dtrap);
    func();
    testutil.dbugSet("");
    func();
}

//@<> Initialize.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {server_uuid: "cd93e780-b558-11ea-b3de-0242ac130004", report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {server_uuid: "cd93e9a6-b558-11ea-b3de-0242ac130004", report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
var mycnf_path2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {server_uuid: "cd93eab4-b558-11ea-b3de-0242ac130004", report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
var mycnf_path3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);

disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

//@<> Get needed Server_Ids and UUIDs.
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

//@<> Configure sandboxes.
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
                                   [__mysql_sandbox_port3, "OK"]]);

//@<> Rescan must change the recovery username if it doesn't have the expected format

session2 = mysql.getSession(__sandbox_uri2);

session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name = ?", [`${hostname}:${__mysql_sandbox_port2}`]);

session2.runSql("STOP group_replication");
session2.runSql("SET sql_log_bin=0");
session2.runSql("SET global super_read_only=0");
session2.runSql("CREATE USER IF NOT EXISTS foo@'%' IDENTIFIED BY 'password'");
session2.runSql("GRANT REPLICATION SLAVE ON *.* TO foo@'%'");
session2.runSql("GRANT CONNECTION_ADMIN ON *.* TO foo@'%'");
session2.runSql("GRANT BACKUP_ADMIN ON *.* TO foo@'%'");
session2.runSql("GRANT GROUP_REPLICATION_STREAM ON *.* TO foo@'%'");
session2.runSql("FLUSH PRIVILEGES");
session2.runSql("SET sql_log_bin=1");
session2.runSql("CHANGE " + get_replication_source_keyword() + " TO " + get_replication_source_keyword(__version_num, "SOURCE_USER") + "='foo', " + get_replication_source_keyword(__version_num, "SOURCE_PASSWORD") + "='password' FOR CHANNEL 'group_replication_recovery'");

session.runSql("CREATE USER IF NOT EXISTS foo@'%' IDENTIFIED BY 'password'");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO foo@'%'");
session.runSql("GRANT CONNECTION_ADMIN ON *.* TO foo@'%'");
session.runSql("GRANT BACKUP_ADMIN ON *.* TO foo@'%'");
session.runSql("GRANT GROUP_REPLICATION_STREAM ON *.* TO foo@'%'");
session.runSql("FLUSH PRIVILEGES");

session2.runSql("START group_replication");
session2.close();

EXPECT_NO_THROWS(function(){ cluster.rescan({addUnmanaged: true}); });
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' was successfully added to the cluster metadata.`);
EXPECT_OUTPUT_CONTAINS(`Fixing incorrect recovery account 'foo' in instance '${hostname}:${__mysql_sandbox_port2}'`);

validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

//@ No-op - Still missing server_id attributes are added
var initial_atts = session.runSql(`SELECT attributes->'$.server_id' AS server_id FROM mysql_innodb_cluster_metadata.instances WHERE address = '${hostname}:${__mysql_sandbox_port3}'`).fetchOneObject();
session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances set attributes=JSON_REMOVE(attributes, '$.server_id') WHERE address= '${hostname}:${__mysql_sandbox_port3}'`);
cluster.rescan();
var final_atts = session.runSql(`SELECT attributes->'$.server_id' AS server_id FROM mysql_innodb_cluster_metadata.instances WHERE address = '${hostname}:${__mysql_sandbox_port3}'`).fetchOneObject();
EXPECT_EQ(initial_atts.server_id, final_atts.server_id);

//@<> invalid addUnmanaged and removeObsolete value type throws TypeError
EXPECT_THROWS_TYPE(function() {
    cluster.rescan({addUnmanaged: "yes"})
}, "Argument #1: Option 'addUnmanaged' Bool expected, but value is String", "TypeError");

EXPECT_THROWS_TYPE(function() {
    cluster.rescan({removeObsolete: "yes"})
}, "Argument #1: Option 'removeObsolete' Bool expected, but value is String", "TypeError");

//@<> ensure removeObsolete also accounts for missing server id.
EXPECT_NO_THROWS(function(){ cluster.rescan({removeObsolete: true}); });

//@<> Remove instance on port 2 and 3 from MD but keep it in the group.
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [hostname+":"+__mysql_sandbox_port2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [hostname+":"+__mysql_sandbox_port3]);
validate_status(cluster.status(), [[__mysql_sandbox_port1, "RECOVERY_UNUSED"],
                                   [__mysql_sandbox_port2, "UNMANAGED"],
                                   [__mysql_sandbox_port3, "UNMANAGED"]]);

//@<> addUnmanaged must ensure all metadata is updated
cluster.rescan({addUnmanaged: true});

//@<> WL10644 - TSF2_1: Validate that the instances were added.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                [__mysql_sandbox_port2, "OK"],
                                [__mysql_sandbox_port3, "OK"]]);

//@<> Rescan must change the recovery username if it doesn't have the correct format (even if the prefix is the same)
session.runSql("CREATE USER IF NOT EXISTS mysql_innodb_cluster_11foo22@'%' IDENTIFIED BY 'password'");
session.runSql("GRANT ALL ON *.* TO mysql_innodb_cluster_11foo22@'%'");
session.runSql("FLUSH PRIVILEGES");

session2 = mysql.getSession(__sandbox_uri2);
session2.runSql(`CHANGE ${get_replication_source_keyword()} TO ${get_replication_option_keyword()}_USER='mysql_innodb_cluster_11foo22', ${get_replication_option_keyword()}_PASSWORD='password' FOR CHANNEL 'group_replication_recovery'`);
session2.close();

var recovery_account = session.runSql("SELECT (attributes->>'$.recoveryAccountUser') FROM mysql_innodb_cluster_metadata.instances WHERE address = ?", [`${hostname}:${__mysql_sandbox_port2}`]).fetchOne()[0];
session.runSql("DROP USER ?", [recovery_account]);

WIPE_SHELL_LOG();

EXPECT_NO_THROWS(function(){ cluster.rescan(); });

EXPECT_OUTPUT_CONTAINS(`Fixing incorrect recovery account 'mysql_innodb_cluster_11foo22' in instance '${hostname}:${__mysql_sandbox_port2}'`);
EXPECT_OUTPUT_CONTAINS(`A new recovery user for instance '${hostname}:${__mysql_sandbox_port2}' was created, which leaves the previous user, 'mysql_innodb_cluster_11foo22', unused, so it should be removed.`);
EXPECT_SHELL_LOG_CONTAINS(`Recovery account '${recovery_account}' does not exist in the cluster.`);
EXPECT_SHELL_LOG_NOT_CONTAINS(`Updating recovery account '${recovery_account}' in metadata.`);

validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

//@<> Make sure that "addUnmanaged: true" works.
var member_address2 = hostname + ":" + __mysql_sandbox_port2;
var member_address3 = hostname + ":" + __mysql_sandbox_port3;

session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address2]);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [member_address3]);

validate_status(cluster.status(), [[__mysql_sandbox_port1, "RECOVERY_UNUSED"],
                                   [__mysql_sandbox_port2, "UNMANAGED"],
                                   [__mysql_sandbox_port3, "UNMANAGED"]]);

cluster.rescan({addUnmanaged: true});

validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

shell.options.useWizards=0;

//@<> WL10644 - TSF3_1: Disable GR in persisted settings {VER(>=8.0.11)}.
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
EXPECT_EQ(3, count_in_metadata_schema());

//@<> Rescan with removeObsolete: true.
cluster.rescan({removeObsolete: true});

//@<> WL10644 - TSF3_1: Number of instances in the MD after rescan().
EXPECT_EQ(1, count_in_metadata_schema());

//@<> WL10644 - TSF3_1: Validate that the instances were removed.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "N/A"]]);

//@<> WL10644 - TSF3_2: Start instances and add them back to the cluster.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
cluster.addInstance(__hostname_uri2);
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL10644 - TSF3_2: Disable GR in persisted settings {VER(>=8.0.11)}.
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
EXPECT_EQ(3, count_in_metadata_schema());

//@<> Rescan with interactive:true.
shell.options.useWizards=1;

testutil.expectPrompt("Would you like to remove it from the cluster metadata? [Y/n]: ", "y");
testutil.expectPrompt("Would you like to remove it from the cluster metadata? [Y/n]: ", "y");

cluster.rescan();

shell.options.useWizards=0;

//@<> WL10644 - TSF3_2: Number of instances in the MD after rescan().
EXPECT_EQ(1, count_in_metadata_schema());

//@<> WL10644 - TSF3_2: Validate that the instances were removed.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "N/A"]]);

//@<> WL10644 - TSF3_3: Start instances and add them back to the cluster.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
cluster.addInstance(__hostname_uri2);
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL10644 - TSF3_3: Disable GR in persisted settings {VER(>=8.0.11)}.
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
EXPECT_EQ(3, count_in_metadata_schema());

//@<> Rescan with removeObsolete: true and interactive:true.
var member_fqdn_address2 = hostname + ":" + __mysql_sandbox_port2;
var member_fqdn_address3 = hostname + ":" + __mysql_sandbox_port3;

shell.options.useWizards=1;
cluster.rescan({removeObsolete: true});
shell.options.useWizards=0;

//@<> WL10644 - TSF3_4: Number of instances in the MD after rescan().
EXPECT_EQ(1, count_in_metadata_schema());

//@<> WL10644 - TSF3_4: Validate that the instances were removed.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "N/A"]]);

//@<> Make sure that "removeObsolete: true" behaves as removeInstances "auto".
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
cluster.addInstance(__hostname_uri2);
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

EXPECT_EQ(3, count_in_metadata_schema());
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

testutil.stopSandbox(__mysql_sandbox_port2);
testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "MISSING"],
                                   [__mysql_sandbox_port3, "MISSING"]]);

shell.options.useWizards=1;
cluster.rescan({removeObsolete: true});
shell.options.useWizards=0;

EXPECT_EQ(1, count_in_metadata_schema());
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "N/A"]]);

//@<> WL10644 - TSF3_5: Start instances and add them back to the cluster.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);

shell.options["dba.connectivityChecks"] = false;
cluster.addInstance(__hostname_uri2);
cluster.addInstance(__hostname_uri3);
shell.options["dba.connectivityChecks"] = true;

//@<> WL10644 - TSF3_5: Disable GR in persisted settings {VER(>=8.0.11)}.
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
EXPECT_EQ(3, count_in_metadata_schema());

//@<> Rescan with removeObsolete: true and interactive:false.
cluster.rescan({removeObsolete: true});

//@<> WL10644 - TSF3_5: Number of instances in the MD after rescan().
EXPECT_EQ(1, count_in_metadata_schema());

//@<> WL10644 - TSF3_5: Validate that the instances were removed.
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "N/A"],
                                   [__mysql_sandbox_port3, "N/A"]]);

//@<> WL10644: Start instances and add them back to the cluster again.
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
cluster.addInstance(__hostname_uri2);
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> WL10644 - TSF4_1: Change the topology mode in the MD to the wrong value.
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = 'mm'");

//@<> WL10644 - TSF4_1: Topology mode in MD before rescan().
EXPECT_EQ("mm", get_metadata_topology_mode());

//@<> WL10644 - TSF4_5: Set auto_increment settings to unused values.
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

//@<> BUG#29330769: Check topology mode in MD after rescan().
cluster.rescan();
get_metadata_topology_mode();

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
var cluster = dba.createCluster("c", {multiPrimary: true, force: true, gtidSetIsComplete: true, manualStartOnBoot: true});
cluster.addInstance(__hostname_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
cluster.addInstance(__hostname_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
validate_status(cluster.status(), [[__mysql_sandbox_port1, "OK"],
                                   [__mysql_sandbox_port2, "OK"],
                                   [__mysql_sandbox_port3, "OK"]]);

//@<> WL10644 - TSF4_3: Change the topology mode in the MD to the wrong value.
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = 'pm'");

//@<> WL10644 - TSF4_3: Topology mode in MD before rescan().
EXPECT_EQ("pm", get_metadata_topology_mode());

//@<> WL10644 - TSF4_6: Set auto_increment settings to unused values.
set_auto_increment_to_unused_values(__sandbox_uri1);
set_auto_increment_to_unused_values(__sandbox_uri2);
set_auto_increment_to_unused_values(__sandbox_uri3);

//@<> WL10644 - TSF4_3: Rescan with interactive:true and change needed.
shell.options.useWizards=1;
cluster.rescan();
shell.options.useWizards=0;

//@<> WL10644 - TSF4_3: Check topology mode in MD after rescan().
EXPECT_EQ("mm", get_metadata_topology_mode());

//@<> WL10644 - TSF4_6: Check auto_increment settings after change to multi-primary.
offset1 = 1 + instance1_id % 7;
offset2 = 1 + instance2_id % 7;
offset3 = 1 + instance3_id % 7;
check_auto_increment_settings(__sandbox_uri1);
check_auto_increment_settings(__sandbox_uri2);
check_auto_increment_settings(__sandbox_uri3);

//@<> WL10644 - TSF4_4: Change the topology mode in the MD to the wrong value.
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET primary_mode = 'pm'");

//@<> WL10644 - TSF4_4: Topology mode in MD before rescan().
EXPECT_EQ("pm", get_metadata_topology_mode());

//@<> WL10644 - TSF4_4: Rescan with interactive:false and change needed.
cluster.rescan();

//@<> WL10644 - TSF4_4: Check topology mode in MD after rescan().
EXPECT_EQ("mm", get_metadata_topology_mode());

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

//@<> Cluster.rescan() - view_change_uuid handling {VER(>=8.0.27) && VER(<8.3.0)}

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

EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster is not configured to use 'group_replication_view_change_uuid', which is required for InnoDB ClusterSet. Configuring it requires a full Cluster reboot.");
EXPECT_OUTPUT_CONTAINS("Use the 'updateViewChangeUuid' option to generate and configure a value for the Cluster.");
WIPE_STDOUT();

// Use the option and validate it's successful
EXPECT_NO_THROWS(function() { cluster.rescan({updateViewChangeUuid: true}); });

EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is not set.");
EXPECT_OUTPUT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
EXPECT_OUTPUT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");
WIPE_STDOUT();

// Do not restart yet and confirm that cluster.rescan() detects a restart is needed for the change to be effective
EXPECT_NO_THROWS(function() { cluster.rescan({updateViewChangeUuid: true}); });
EXPECT_OUTPUT_CONTAINS("WARNING: The current Cluster group_replication_view_change_uuid setting does not allow ClusterSet to be implemented, because it's set but not yet effective.");
EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
WIPE_STDOUT();

// Restart the instances and reboot it from complete outage after rescan() changed group_replication_view_change_uuid
testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage("c");

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

testutil.stopGroup([__mysql_sandbox_port2,__mysql_sandbox_port1]);

shell.connect(__sandbox_uri1);

cluster = dba.rebootClusterFromCompleteOutage("c");

var view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
EXPECT_NE(view_change_uuid, "AUTOMATIC");

var view_change_uuid_md = session.runSql("select (attributes->>'$.group_replication_view_change_uuid') from mysql_innodb_cluster_metadata.clusters where cluster_name='c'").fetchOne()[0];
EXPECT_EQ(view_change_uuid, view_change_uuid_md);

// cluster.rescan() must not warn or act if the target cluster has group_replication_view_change_uuid set
EXPECT_NO_THROWS(function() { cluster.rescan(); });
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is not set.");
EXPECT_OUTPUT_NOT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
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

EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster is not configured to use 'group_replication_view_change_uuid', which is required for InnoDB ClusterSet. Configuring it requires a full Cluster reboot.");

EXPECT_OUTPUT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_CONTAINS("NOTE: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
EXPECT_OUTPUT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");
WIPE_STDOUT();

// Restart the instances and reboot it from complete outage after rescan() changed group_replication_view_change_uuid
testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage("c");

var view_change_uuid = session.runSql("SELECT @@group_replication_view_change_uuid").fetchOne()[0];
EXPECT_NE(view_change_uuid, "AUTOMATIC");

var view_change_uuid_md = session.runSql("select (attributes->>'$.group_replication_view_change_uuid') from mysql_innodb_cluster_metadata.clusters where cluster_name='c'").fetchOne()[0];
EXPECT_EQ(view_change_uuid, view_change_uuid_md);

// Confirm cluster.rescan() does not warn or act if the target cluster has group_replication_view_change_uuid set
EXPECT_NO_THROWS(function() { cluster.rescan(); });
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is not set.");
EXPECT_OUTPUT_NOT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");
EXPECT_OUTPUT_NOT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");

//@<> Bug #33235502 Check for incorrect recovery accounts
shell.options["useWizards"] = false;

cluster.dissolve({force:true});

var cluster = dba.createCluster("c");

if (testutil.versionCheck(__version, ">=", "8.0.11")) {

    cluster.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});

    status = cluster.status();
    EXPECT_FALSE(check_status_instance_error_msg(status, __mysql_sandbox_port1, `WARNING: Incorrect recovery account (mysql_innodb_cluster_${instance2_id}) being used. Use Cluster.rescan() to repair.`));
}

//@<> Bug #33235502 Check for multiple unused recovery accounts and fix them

if (testutil.versionCheck(__version, "<", "8.0.11")) {
    cluster.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"});
}

// delete the instance using the recovery account to create the cenario where the account isn't being used
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name=?", [`${hostname}:${__mysql_sandbox_port2}`]);

status = cluster.status();
EXPECT_TRUE(check_status_instance_error_msg(status, __mysql_sandbox_port1, `WARNING: Detected an unused recovery account: mysql_innodb_cluster_${instance2_id}. Use Cluster.rescan() to clean up.`));

EXPECT_NO_THROWS(function () { cluster.rescan() });
EXPECT_OUTPUT_CONTAINS(`Dropping unused recovery account: 'mysql_innodb_cluster_${instance2_id}'@'%'`);

status = cluster.status();
EXPECT_FALSE(check_status_instance_error_msg(status, __mysql_sandbox_port1, `WARNING: Detected an unused recovery account: mysql_innodb_cluster_${instance2_id}. Use Cluster.rescan() to clean up.`));

EXPECT_NO_THROWS(function () { cluster.rescan({addUnmanaged: true}) });
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' was successfully added to the cluster metadata.`);

//@<> Bug #33235502 Check if recreated recovery account has the correct host

var row = session.runSql("SELECT cluster_id, attributes->>'$.recoveryAccountUser' FROM mysql_innodb_cluster_metadata.instances WHERE (instance_name = ?)", [`${hostname}:${__mysql_sandbox_port2}`]).fetchOne();
var cluster_id = row[0];
var recover_user = row[1];
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_set(COALESCE(attributes, '{}'), \"$.recoveryAccountUser\", ?) WHERE (instance_name = ?)", [recover_user + "00", `${hostname}:${__mysql_sandbox_port2}`]);
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = json_set(COALESCE(attributes, '{}'), \"$.opt_replicationAllowedHost\", ?) WHERE (cluster_id = ?)", ["fake_host", cluster_id]);

cluster.rescan();

host = session.runSql("SELECT attributes->>'$.recoveryAccountHost' FROM mysql_innodb_cluster_metadata.instances WHERE (instance_name = ?)", [`${hostname}:${__mysql_sandbox_port2}`]).fetchOne()[0];
EXPECT_EQ(host, "fake_host");

//@<> Bug #33235502 Check if status uses the correct server_id value

session2 = mysql.getSession(__sandbox_uri2);
var server_id2 = session2.runSql("SELECT @@server_id").fetchOne()[0];
session2.close();

session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_remove(attributes, '$.server_id') WHERE (instance_name = ?)", [`${hostname}:${__mysql_sandbox_port2}`]);

test_with_dtrap("+d,assume_no_server_id_in_metadata", function () {
    status = cluster.status();
    EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
    EXPECT_TRUE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
    EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0], "NOTE: instance server_id is not registered in the metadata. Use cluster.rescan() to update the metadata.");
});

EXPECT_NO_THROWS(function () { cluster.rescan() });

test_with_dtrap("+d,assume_no_server_id_in_metadata", function () {
    status = cluster.status();
    EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
    EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
    EXPECT_EQ(session.runSql("SELECT attributes->>'$.server_id' FROM mysql_innodb_cluster_metadata.instances WHERE (instance_name = ?)", [`${hostname}:${__mysql_sandbox_port2}`]).fetchOne()[0], String(server_id2));
});

cluster.disconnect();

//@<> check metadata repair when report_host is changed  {VER(>=8.0.27)}
shell.connect(__sandbox_uri1);
reset_instance(session);

testutil.destroySandbox(__mysql_sandbox_port3);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {server_uuid: instance3_uuid, server_id: instance3_id, report_host: hostname_ip});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

c = dba.createCluster("cluster" , {gtidSetIsComplete:1});
c.addInstance(__sandbox_uri3);
dba.configureInstance(__sandbox_uri3);
c.status();

shell.connect(__sandbox_uri3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "report_host", hostname);
testutil.restartSandbox(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

s = c.status();

EXPECT_EQ(`ERROR: Metadata for this instance does not match hostname reported by instance (metadata=${hostname_ip}:${__mysql_sandbox_port3}, actual=${hostname}:${__mysql_sandbox_port3}). Use rescan() to update the metadata.`, s["defaultReplicaSet"]["topology"][hostname_ip+":"+__mysql_sandbox_port3]["instanceErrors"][0])

EXPECT_EQ(hostname_ip+":"+__mysql_sandbox_port3, s["defaultReplicaSet"]["topology"][hostname_ip+":"+__mysql_sandbox_port3]["address"]);

row = session.runSql("select * from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=?", [instance3_uuid]).fetchOne();
EXPECT_EQ(hostname_ip+":"+__mysql_sandbox_port3, row.address);
EXPECT_EQ(hostname_ip+":"+__mysql_sandbox_port3, row.instance_name);
EXPECT_EQ( {"mysqlX": hostname_ip+":"+__mysql_sandbox_port3+"0", "grLocal": hostname_ip+":"+__mysql_sandbox_gr_port3, "mysqlClassic": hostname_ip+":"+__mysql_sandbox_port3}, JSON.parse(row.addresses));

//@<> rescan to repair metadata according to new report_host {VER(>=8.0.27)}

// sb3 address should be fixed now
c.rescan();
row = session.runSql("select * from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=?", [instance3_uuid]).fetchOne();
EXPECT_EQ(hostname+":"+__mysql_sandbox_port3, row.address);
EXPECT_EQ(hostname+":"+__mysql_sandbox_port3, row.instance_name);
EXPECT_EQ( {"mysqlX": hostname+":"+__mysql_sandbox_x_port3, "grLocal": hostname_ip+":"+__mysql_sandbox_gr_port3, "mysqlClassic": hostname+":"+__mysql_sandbox_port3}, JSON.parse(row.addresses));

s = c.status();

EXPECT_EQ(hostname+":"+__mysql_sandbox_port3, s["defaultReplicaSet"]["topology"][hostname+":"+__mysql_sandbox_port3]["address"]);

//@<> ensure setPrimary works now {VER(>=8.0.27)}
EXPECT_NO_THROWS(function(){ c.setPrimaryInstance(__sandbox_uri3); });
EXPECT_NO_THROWS(function(){ c.setPrimaryInstance(__sandbox_uri1); });

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

//@<> Test communication protocol upgrade {VER(>=8.0)}

// Downgrade the protocol version to 5.7.14
session.runSql("SELECT group_replication_set_communication_protocol('5.7.14')");

s = c.status();

if (__version_num >= 80027) {
    EXPECT_EQ(["Group communication protocol in use is version 5.7.14 but it is possible to upgrade to 8.0.27. Message fragmentation for large transactions and Single Consensus Leader can only be enabled after upgrade. Use Cluster.rescan({upgradeCommProtocol:true}) to upgrade."], s["defaultReplicaSet"]["clusterErrors"]);
} else if (__version_num >= 80016 && __version_num < 80027) {
    EXPECT_EQ(["Group communication protocol in use is version 5.7.14 but it is possible to upgrade to 8.0.16. Message fragmentation for large transactions can only be enabled after upgrade. Use Cluster.rescan({upgradeCommProtocol:true}) to upgrade."], s["defaultReplicaSet"]["clusterErrors"]);
}

EXPECT_NO_THROWS(function () {c.rescan({upgradeCommProtocol:1});});

s = c.status();
EXPECT_EQ(undefined, s["defaultReplicaSet"]["clusterErrors"]);

// Downgrade the protocol version to 8.0.16
if (__version_num >= 80027) {
    session.runSql("SELECT group_replication_set_communication_protocol('8.0.16')");

    s = c.status();
    EXPECT_EQ(["Group communication protocol in use is version 8.0.16 but it is possible to upgrade to 8.0.27. Single Consensus Leader can only be enabled after upgrade. Use Cluster.rescan({upgradeCommProtocol:true}) to upgrade."], s["defaultReplicaSet"]["clusterErrors"]);

    EXPECT_NO_THROWS(function () {c.rescan({upgradeCommProtocol:1});});

    s = c.status();
    EXPECT_EQ(undefined, s["defaultReplicaSet"]["clusterErrors"]);
}

// BUG#35000998 results in failures during switchovers/failovers in ClusterSet.
// The bug is caused by a mismatch of the value in use by the Replica Cluster
// for `group_replication_view_change_uuid` and the value stored in the
// Metadata.
// Shell must detect it in .status() and allow users to fix it using .rescan().

//@<> BUG#35000998 ensure group_replication_view_change_uuid stored in the metadata matches the current cluster's value {VER(>=8.0.27) && VER(<8.3.0)}

// Change the runtime value of view_change_uuid
session3 = mysql.getSession(__sandbox_uri3);
session.runSql("set persist_only group_replication_view_change_uuid='aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee';")
session3.runSql("set persist_only group_replication_view_change_uuid='aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee';")

session3.runSql("STOP group_replication");
session.runSql("STOP group_replication");

// Reboot the cluster so the new value gets used
EXPECT_NO_THROWS(function() { c = dba.rebootClusterFromCompleteOutage(); });

// Verify the warning is added to .status()
s = c.status();
EXPECT_EQ(["WARNING: The Cluster's group_replication_view_change_uuid value in use does not match the value stored in the Metadata. Please use <Cluster>.rescan() to update the metadata."], s["defaultReplicaSet"]["clusterErrors"]);

// Call .rescan() to updated the Metadata
EXPECT_NO_THROWS(function() { c.rescan(); });

EXPECT_OUTPUT_CONTAINS("Updating group_replication_view_change_uuid in the Cluster's metadata...");

// Validate the warning is gone from status
s = c.status();
EXPECT_EQ(undefined, s["defaultReplicaSet"]["clusterErrors"]);

// Validate the metadada update
s_ext = c.status({extended:1});
EXPECT_EQ("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee", s_ext["defaultReplicaSet"]["groupViewChangeUuid"]);

//@<> check metadata repair when mysqlx_port is changed {VER(>=8.0.27)}
shell.connect(__sandbox_uri3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "loose_mysqlx_port", __mysql_sandbox_x_port4.toString());
testutil.restartSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

c = dba.getCluster()
s = c.status();

EXPECT_EQ(`ERROR: Metadata for this instance does not match X plugin port reported by instance (metadata=${hostname_ip}:${__mysql_sandbox_x_port3}, actual=${hostname_ip}:${__mysql_sandbox_x_port4}). Use rescan() to update the metadata.`, s["defaultReplicaSet"]["topology"]["hooray"]["instanceErrors"][0])

row = session.runSql("select * from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=?", [instance3_uuid]).fetchOne();
EXPECT_EQ( {"mysqlX": hostname_ip+":"+__mysql_sandbox_port3+"0", "grLocal": hostname_ip+":"+__mysql_sandbox_gr_port3, "mysqlClassic": hostname_ip+":"+__mysql_sandbox_port3}, JSON.parse(row.addresses));

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { c.rescan(); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
Result of the rescanning operation for the 'cluster' cluster:
{
    "metadataConsistent": true,
    "name": "cluster",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": [
        {
            "host": "${hostname_ip}:${__mysql_sandbox_port3}",
            "id": 2,
            "label": "hooray",
            "member_id": "${instance3_uuid}",
            "old_xport_endpoint": "${hostname_ip}:${__mysql_sandbox_x_port3}",
            "xport_endpoint": "${hostname_ip}:${__mysql_sandbox_x_port4}"
        }
    ]
}

The instance '${hostname_ip}:${__mysql_sandbox_port3}' is part of the Cluster but the reported X plugin port has changed. Old xport: ${hostname_ip}:${__mysql_sandbox_x_port3}. Current xport: ${hostname_ip}:${__mysql_sandbox_x_port4}.
Updating instance metadata...
The instance metadata for '${hostname_ip}:${__mysql_sandbox_port3}' was successfully updated.
`);

// Confirm the address was updated
row = session.runSql("select * from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=?", [instance3_uuid]).fetchOne();
EXPECT_EQ( {"mysqlX": hostname_ip+":"+__mysql_sandbox_port4+"0", "grLocal": hostname_ip+":"+__mysql_sandbox_gr_port3, "mysqlClassic": hostname_ip+":"+__mysql_sandbox_port3}, JSON.parse(row.addresses));

// Re-check status
s = c.status();

EXPECT_EQ(undefined, s["defaultReplicaSet"]["topology"]["hooray"]["instanceErrors"])

//@<> check metadata repair when mysqlx_port is disabled {VER(>=8.0.27)}
testutil.changeSandboxConf(__mysql_sandbox_port3, "mysqlx", "off");
testutil.restartSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

c = dba.getCluster()
s = c.status();

EXPECT_EQ(`ERROR: Metadata for this instance does not match X plugin port reported by instance (metadata=${hostname_ip}:${__mysql_sandbox_x_port4}, actual=<disabled>). Use rescan() to update the metadata.`, s["defaultReplicaSet"]["topology"]["hooray"]["instanceErrors"][0])

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { c.rescan(); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
Result of the rescanning operation for the 'cluster' cluster:
{
    "metadataConsistent": true,
    "name": "cluster",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": [
        {
            "host": "${hostname_ip}:${__mysql_sandbox_port3}",
            "id": 2,
            "label": "hooray",
            "member_id": "${instance3_uuid}",
            "old_xport_endpoint": "${hostname_ip}:${__mysql_sandbox_x_port4}",
            "xport_endpoint": "<disabled>"
        }
    ]
}

The instance '${hostname_ip}:${__mysql_sandbox_port3}' is part of the Cluster but the reported X plugin port has changed. Old xport: ${hostname_ip}:${__mysql_sandbox_x_port4}. Current xport: <disabled>.
Updating instance metadata...
The instance metadata for '${hostname_ip}:${__mysql_sandbox_port3}' was successfully updated.
`);

row = session.runSql("select * from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=?", [instance3_uuid]).fetchOne();
EXPECT_EQ( {"grLocal": hostname_ip+":"+__mysql_sandbox_gr_port3, "mysqlClassic": hostname_ip+":"+__mysql_sandbox_port3}, JSON.parse(row.addresses));

s = c.status();

EXPECT_EQ(undefined, s["defaultReplicaSet"]["topology"]["hooray"]["instanceErrors"])

// Confirm rescan does not detect anything else
WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { c.rescan(); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
Rescanning the cluster...

Result of the rescanning operation for the 'cluster' cluster:
{
    "metadataConsistent": true,
    "name": "cluster",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": []
}
`);

//@<> check metadata repair when mysqlx_port is enabled back {VER(>=8.0.27)}
testutil.changeSandboxConf(__mysql_sandbox_port3, "mysqlx", "on");
testutil.restartSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

c = dba.getCluster()
s = c.status();

EXPECT_EQ(`ERROR: Metadata for this instance does not match X plugin port reported by instance (metadata=, actual=${hostname_ip}:${__mysql_sandbox_x_port4}). Use rescan() to update the metadata.`, s["defaultReplicaSet"]["topology"]["hooray"]["instanceErrors"][0])

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { c.rescan(); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
Result of the rescanning operation for the 'cluster' cluster:
{
    "metadataConsistent": true,
    "name": "cluster",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": [
        {
            "host": "${hostname_ip}:${__mysql_sandbox_port3}",
            "id": 2,
            "label": "hooray",
            "member_id": "${instance3_uuid}",
            "old_xport_endpoint": "",
            "xport_endpoint": "${hostname_ip}:${__mysql_sandbox_x_port4}"
        }
    ]
}

The instance '${hostname_ip}:${__mysql_sandbox_port3}' is part of the Cluster but the reported X plugin port has changed. Old xport: . Current xport: ${hostname_ip}:${__mysql_sandbox_x_port4}.
Updating instance metadata...
The instance metadata for '${hostname_ip}:${__mysql_sandbox_port3}' was successfully updated.
`);

row = session.runSql("select * from mysql_innodb_cluster_metadata.instances where mysql_server_uuid=?", [instance3_uuid]).fetchOne();
EXPECT_EQ( {"mysqlX": hostname_ip+":"+__mysql_sandbox_port4+"0", "grLocal": hostname_ip+":"+__mysql_sandbox_gr_port3, "mysqlClassic": hostname_ip+":"+__mysql_sandbox_port3}, JSON.parse(row.addresses));

s = c.status();

EXPECT_EQ(undefined, s["defaultReplicaSet"]["topology"]["hooray"]["instanceErrors"])

// Confirm rescan does not detect anything else
WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { c.rescan(); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
Rescanning the cluster...

Result of the rescanning operation for the 'cluster' cluster:
{
    "metadataConsistent": true,
    "name": "cluster",
    "newTopologyMode": null,
    "newlyDiscoveredInstances": [],
    "unavailableInstances": [],
    "updatedInstances": []
}
`);

//@<> Cluster.rescan() - view_change_uuid handling in mixed version scenario {VER(>=8.0.27) && !__dbug_off}

// When the Primary Cluster is running a version >= 8.3.0, view_change_uuid is not required so it won't be set. If any Replica Cluster is running a version < 8.3.0 then view_change_uuid is required for itself and will be set.
// When a user runs .rescan() on that Cluster the command must do the view_change_uuid validations on the Cluster itself and not on the Primary Cluster.

testutil.dbugSet("+d,dba_view_change_uuid_needed");

session2 = mysql.getSession(__sandbox_uri2)
reset_instance(session2)

cs = c.createClusterSet("cs")
replica_cluster = cs.createReplicaCluster(__sandbox_uri2, "replica_cluster")

// Simulate the Replica Cluster having a different value for view_change_uuid to ensure that .rescan() does the check on itself and not on the primary cluster
session2.runSql("set persist_only group_replication_view_change_uuid='aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee'");
session2.runSql("STOP group_replication");

shell.connect(__sandbox_uri2);

c = dba.rebootClusterFromCompleteOutage()

EXPECT_NO_THROWS(function() { c.rescan(); });
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: The Cluster's group_replication_view_change_uuid is not set.");
EXPECT_OUTPUT_NOT_CONTAINS("Generating and setting a value for group_replication_view_change_uuid...");
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: The Cluster must be completely taken OFFLINE and restarted (dba.rebootClusterFromCompleteOutage()) for the settings to be effective");

testutil.dbugSet("");

//@<> Finalize.
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
