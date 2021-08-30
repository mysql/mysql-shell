// This module contains simple tests for the InnoDB Cluster APIs
var common = require("_common")

exports.test_dba_check_instance_configuration = function(raw_instance_port) {
    dba.checkInstanceConfiguration(common.sandbox_uri(raw_instance_port));
    EXPECT_OUTPUT_CONTAINS("NOTE: Some configuration options need to be fixed:");
    WIPE_OUTPUT();
}

exports.test_dba_configure_instance = function(port) {
    var cnfpath = testutil.getSandboxConfPath(port);

    if (shell.options.useWizards) {
        testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]: ", "y")
        testutil.expectPrompt("Do you want to restart the instance after configuring it? [y/n]: ", "n")
    }
    dba.configureInstance(common.sandbox_uri(port), {mycnfPath: cnfpath});
    EXPECT_OUTPUT_CONTAINS(`The instance '${common.end_point(port)}' was configured to be used in an InnoDB cluster.`);
    testutil.restartSandbox(port);
}

exports.test_cluster_add_instance = function(cluster, port, recovery_method) {
    cluster.addInstance(common.sandbox_uri(port), {recoveryMethod:recovery_method})
    EXPECT_OUTPUT_CONTAINS(`The instance '${common.end_point(port)}' was successfully added to the cluster.`);
}

exports.test_cluster_describe = function(cluster, ports) {
    var desc = cluster.describe()
    print(desc)
    for(index in ports) {
        EXPECT_EQ(common.end_point(ports[index]),
            desc.defaultReplicaSet.topology[index].address,
            "Cluster.describe: mismatched endpoints");
    }
}

exports.test_cluster_status = function(cluster, ports, roles, states) {
    var status = cluster.status({extended:3})
    EXPECT_EQ(ports.length, Object.keys(status.defaultReplicaSet.topology).length,
        "Cluster.status: unexpected number of nodes in topology");
    for(index in ports) {
        EXPECT_EQ(states[index],
            status.defaultReplicaSet.topology[common.end_point(ports[index])].status,
            `Cluster.status: unexpected state for instance ${common.end_point(ports[index])}`);

        EXPECT_EQ(roles[index],
            status.defaultReplicaSet.topology[common.end_point(ports[index])].memberRole,
            `Cluster.status: unexpected memberRole for instance ${common.end_point(ports[index])}`);
    }
}

exports.test_cluster_remove_instance = function(cluster, port) {
    cluster.removeInstance(common.sandbox_uri(port));
    EXPECT_OUTPUT_CONTAINS(`The instance '${common.sandbox(port)}' was successfully removed from the cluster.`);
}

exports.test_cluster_set_primary_instance = function(cluster, port) {
    cluster.setPrimaryInstance(common.sandbox_uri(port));
    EXPECT_OUTPUT_CONTAINS(`The instance '${common.sandbox(port)}' was successfully elected as primary.`);
}

exports.test_cluster_rejoin_instance = function(cluster, port) {
    var asession = shell.openSession(common.sandbox_uri(port));
    asession.runSql("STOP group_replication");
    asession.close()
    cluster.rejoinInstance(common.sandbox_uri(port))
    testutil.waitMemberState(port, "ONLINE");
    EXPECT_OUTPUT_CONTAINS(`The instance '${common.end_point(port)}' was successfully rejoined to the cluster.`);
}

exports.test_cluster_force_quorum_using_partition_of = function(cluster, live_port, dead_port) {
    testutil.killSandbox(dead_port);
    testutil.waitMemberState(dead_port, "(MISSING),UNREACHABLE");
    cluster.forceQuorumUsingPartitionOf(common.sandbox_uri(live_port));
    testutil.startSandbox(dead_port);
    cluster.rejoinInstance(common.sandbox_uri(dead_port));
    testutil.waitMemberState(dead_port, "ONLINE");
    EXPECT_OUTPUT_CONTAINS(`The instance '${common.end_point(dead_port)}' was successfully rejoined to the cluster.`);
}

exports.test_cluster_reboot_cluster_using_from_complete_outage = function(ports) {
    var last_port = 0;
    for(index in ports) {
        last_port = ports[index]
        var asession = shell.openSession(common.sandbox_uri(last_port));
        asession.runSql("STOP group_replication");
        asession.close();
    }

    if (shell.options.useWizards) {
        testutil.expectPrompt("Would you like to rejoin it to the cluster? [y/N]: ", "y");
    }
    cluster = dba.rebootClusterFromCompleteOutage();

    if (!shell.options.useWizards) {
        cluster.rejoinInstance(common.sandbox_uri(last_port));
    }
    testutil.waitMemberState(last_port, "ONLINE");

    exports.test_cluster_describe(cluster, ports)
}

exports.test_cluster_set_instance_option = function(cluster, port, option, value) {
    cluster.setInstanceOption(common.sandbox_uri(port), option, value);
    var options = cluster.options()
    var ioptions = options.defaultReplicaSet.topology[common.end_point(port)]

    for(index in ioptions) {
        var ioption = ioptions[index]
        if (ioption["option"] == option) {
            EXPECT_EQ(value.toString(), ioption["value"],
            "Cluster.setInstanceOption: unexpected value");
            break;
        }
    }
}

exports.test_cluster_switch_to_multi_primary_mode = function(cluster) {
    cluster.switchToMultiPrimaryMode();
    EXPECT_OUTPUT_CONTAINS("The cluster successfully switched to Multi-Primary mode.");
}

exports.test_cluster_switch_to_single_primary_mode = function(cluster) {
    cluster.switchToSinglePrimaryMode();
    EXPECT_OUTPUT_CONTAINS("The cluster successfully switched to Single-Primary mode.");
}


exports.test_cluster_rescan = function(primary_port, secondary_port, session) {
    var cluster = dba.getCluster()
    session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+secondary_port.toString()]);
    var options = {};
    if (shell.options.useWizards) {
        testutil.expectPrompt("Would you like to add it to the cluster metadata? [Y/n]: ", "y");
    } else {
        options.addInstances="auto";
    }
    cluster.rescan(options);

    exports.test_cluster_describe(cluster, [primary_port, secondary_port])
}


exports.test_router_functions = function(cluster, port) {
    var asession = shell.openSession(common.sandbox_uri(port))

    var cluster_id = asession.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

    asession.runSql("create user if not exists mysql_router_12345@'%'");
    asession.runSql("create user if not exists mysql_router1_12345@'%'");

    var old_num_routers = asession.runSql("select count(*) from mysql_innodb_cluster_metadata.routers").fetchOne()[0];

    var attrs1 = '{"ROEndpoint": "6447", "RWEndpoint": "6446", "ROXEndpoint": "64470", "RWXEndpoint": "64460", "MetadataUser": "mysql_router_12345", "bootstrapTargetType": "cluster"}';
    var attrs2 = '{"ROEndpoint": "6447", "RWEndpoint": "6446", "ROXEndpoint": "6448", "RWXEndpoint": "6449", "MetadataUser": "mysql_router1_12345", "bootstrapTargetType": "cluster"}';

    if (metadata_201) {
        asession.runSql(`INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', '${attrs1}', ?, NULL, NULL)`, [cluster_id]);
        asession.runSql(`INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', '${attrs2}', ?, NULL, NULL)`, [cluster_id]);
        asession.close();
    } else {
        asession.runSql(`INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', '${attrs1}', ?, NULL)`, [cluster_id]);
        asession.runSql(`INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', '${attrs2}', ?, NULL)`, [cluster_id]);
        asession.close();
    }

    var routers = cluster.listRouters();
    EXPECT_EQ(2+old_num_routers, Object.keys(routers.routers).length, "Cluster.listRouters: unexpected router count");

    cluster.removeRouterMetadata(Object.keys(routers.routers)[old_num_routers]);
    var new_routers = cluster.listRouters();
    EXPECT_EQ(1+old_num_routers, Object.keys(new_routers.routers).length, "Cluster.listRouters: unexpected router count after deletion");
}

exports.test_setup_accounts = function(cluster, port) {
    cluster.setupRouterAccount("router@'%'", {password:"boo"});
    EXPECT_OUTPUT_CONTAINS("Account router@% was successfully created.");

    cluster.setupAdminAccount("cadmin@'%'", {password:"boo"});
    EXPECT_OUTPUT_CONTAINS("Account cadmin@% was successfully created.");

    var asession = shell.openSession(common.sandbox_uri(port));
    asession.runSql("DROP USER router@'%'");
    asession.runSql("DROP USER cadmin@'%'");
}

exports.test_dba_create_cluster = function(port, session) {
    session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
    dba.createCluster("sample", {adoptFromGR:true});
    EXPECT_OUTPUT_CONTAINS("Cluster successfully created based on existing replication group.");
}

exports.test_cluster_dissolve = function(cluster) {
    if (shell.options.useWizards) {
        testutil.expectPrompt("Are you sure you want to dissolve the cluster? [y/N]: ", "y");
    }

    cluster.dissolve({force:true})
    EXPECT_OUTPUT_CONTAINS("Replication was disabled but user data was left intact.");
}
