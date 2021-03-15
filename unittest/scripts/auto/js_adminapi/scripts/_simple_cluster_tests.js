// This SCRIPT is meant to be included by any test willing to do full
// API testing of a given InnoDb Cluster
//
// Requires the following variables:
// - session: the shell global session
// - cluster: An InnoDB Cluster instance with 1 primary node
// - primary_port: the port of the primary instance in the cluster
// - secondary_port: the port of an instance to be used as secondary
//
// Additional requirements
// - 1 I Raw sandbox to be used as secondary instance
//
// Usage:
// A test script should:
// - Setup the cluster to be tested
// - Include this file as: //@<> INCLUDE _simple_cluster_tests.js
// - Cleanup

//@<> Initialization
var idc = require("_innodb_cluster_testing_api")

//@<> Check Instance State
idc.test_dba_check_instance_configuration(secondary_port)

//@<> Configure Instance
idc.test_dba_configure_instance(secondary_port)

//@<> Cluster Add Instance
idc.test_cluster_add_instance(cluster, secondary_port, "clone")

//@<> Describe
idc.test_cluster_describe(cluster, [primary_port, secondary_port])

//@<> Status
idc.test_cluster_status(cluster, [primary_port, secondary_port], ["PRIMARY", "SECONDARY"], ["ONLINE", "ONLINE"]);

//@<> Remove Instance
idc.test_cluster_remove_instance(cluster, secondary_port)
idc.test_cluster_status(cluster, [primary_port], ["PRIMARY"], ["ONLINE"]);

//@<> Add instance back
idc.test_cluster_add_instance(cluster, secondary_port, "incremental")
idc.test_cluster_status(cluster, [primary_port, secondary_port], ["PRIMARY", "SECONDARY"], ["ONLINE", "ONLINE"]);

//@<> Set Primary Instance
idc.test_cluster_set_primary_instance(cluster, secondary_port)
idc.test_cluster_status(cluster, [primary_port, secondary_port], ["SECONDARY", "PRIMARY"], ["ONLINE", "ONLINE"]);

//@<> Rejoin Instance
idc.test_cluster_rejoin_instance(cluster, secondary_port)
idc.test_cluster_status(cluster, [primary_port, secondary_port], ["PRIMARY", "SECONDARY"], ["ONLINE", "ONLINE"]);

//@<> Force Quorum
idc.test_cluster_force_quorum_using_partition_of(cluster, primary_port, secondary_port)
idc.test_cluster_status(cluster, [primary_port, secondary_port], ["PRIMARY", "SECONDARY"], ["ONLINE", "ONLINE"]);

//@<> Reboot
idc.test_cluster_reboot_cluster_using_from_complete_outage([primary_port, secondary_port])
idc.test_cluster_status(cluster, [primary_port, secondary_port], ["PRIMARY", "SECONDARY"], ["ONLINE", "ONLINE"]);

//@<> Set Instance Option
idc.test_cluster_set_instance_option(cluster, secondary_port, "memberWeight", 20)

//@<> switchToMultiPrimaryMode {VER(>=8.0.0)}
idc.test_cluster_switch_to_multi_primary_mode(cluster);

//@<> switchToSinglePrimaryMode {VER(>=8.0.0)}
idc.test_cluster_switch_to_single_primary_mode(cluster);

//@<> rescan
idc.test_cluster_rescan(primary_port, secondary_port, session)
idc.test_cluster_status(cluster, [primary_port, secondary_port], ["PRIMARY", "SECONDARY"], ["ONLINE", "ONLINE"]);

//@<> listRouters
idc.test_router_functions(cluster, primary_port)

//@<> setupRouterAccount
idc.test_setup_accounts(cluster, primary_port);

//@<> createCluster(adopt)
idc.test_dba_create_cluster(primary_port, session)

//@<> dissolve
idc.test_cluster_dissolve(cluster);