function print_member_weight_variable(session) {
  var res = session.runSql('SHOW VARIABLES like "group_replication_member_weight"')
  var row = res.fetchOne();
  print(row[1] + "\n");
}

function print_exit_state_action_variable(session) {
  var res = session.runSql('SHOW VARIABLES like "group_replication_exit_state_action"')
  var row = res.fetchOne();
  print(row[1] + "\n");
}

function print_consistency_variable(session) {
  var res = session.runSql('SHOW VARIABLES like "group_replication_consistency"')
  var row = res.fetchOne();
  print(row[1] + "\n");
}

function print_persisted_variables_like(session, pattern) {
    var res = session.runSql("SELECT * from performance_schema.persisted_variables WHERE Variable_name like '%" + pattern + "%'").fetchAll();
    for (var i = 0; i < res.length; i++) {
        print(res[i][0] + " = " + res[i][1] + "\n");
    }
    print("\n");
}

function print_expel_timeout_variable(session) {
  var res = session.runSql('SHOW VARIABLES like "group_replication_member_expel_timeout"')
  var row = res.fetchOne();
  print(row[1] + "\n");
}

function print_auto_rejoin_tries_variable(session) {
    var res = session.runSql('SHOW VARIABLES like "group_replication_autorejoin_tries"')
    var row = res.fetchOne();
    print(row[1] + "\n");
}

function print_metadata_clusters_cluster_name(session) {
    var res = session.runSql("select cluster_name from mysql_innodb_cluster_metadata.clusters")
    var row = res.fetchOne();
    print(row[0] + "\n");
}

// WL#11465 AdminAPI: AdminAPI: change cluster member options
//
// Currently, it's not possible to change a previously configuration option
// of a running Cluster or Cluster active member in the AdminAPI.
// Such settings can be set during the calls to:
//
//     - dba.createCluster()
//     - <Cluster.>addInstance()
//     - <Cluster.>rejoinInstance()
//
// In order to support changing configuration options of cluster members
// individually or globally, the AdminAPI was extended with two new commands:
//
//     - <Cluster.>setOption(option, value)
//     - <Cluster.>setInstanceOption(instance, option, value)
//
// Each command has a defined set of accepted options supported by the AdminAPI.
//
// On top of that, in order to verify which are the cluster configuration options
// in place, a new command was added to the AdminAPI:
//
//     - <Cluster.>options([options])

//@<OUT> WL#11465: Create single-primary cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster
var session = scene.session

// Exceptions in setOption():
//  - If the 'option' parameter is empty.
//  - If the 'value' parameter is empty.
//  - If the 'option' parameter is invalid.
//
//  - If any of the cluster members do not support the configuration option passed in 'option'.
//  - If the value passed in 'option' is not valid for Group Replication.
//  - If the cluster has no visible quorum.
//  - If any of the cluster members is not ONLINE.

//@ WL#11465: ArgumentErrors of setOption
cluster.setOption();
cluster.setOption("memberWeight");
cluster.setOption("foobar", 1);

// F1.4 - The function shall not be allowed if the cluster does not have quorum or not all cluster members are ONLINE.
//@ WL#11465: Error when executing setOption on a cluster with 1 or more members not ONLINE
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.setOption("memberWeight", 25);

//@<ERR> WL#11465: Error when executing setOption on a cluster with no visible quorum {VER(>=8.0.14)}
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.setOption("memberWeight", 25);

//@<ERR> WL#11465: Error when executing setOption on a cluster with no visible quorum 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
testutil.startSandbox(__mysql_sandbox_port3);
cluster.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.setOption("memberWeight", 25);

//@ WL#11465: Re-create the cluster
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster

// F1.5.1.2 - The accepted values are:
//
// - exitStateAction
// - memberWeight
// - consistency
// - expelTimeout
// - clusterName

//@ WL#11465: setOption clusterName with invalid value for cluster-name
cluster.setOption("clusterName", "0_a");

//@<OUT> WL#11465: setOption clusterName
cluster.setOption("clusterName", "newName");

//@<OUT> WL#11465: Verify clusterName changed correctly
print_metadata_clusters_cluster_name(session);

//@<OUT> WL#11465: setOption memberWeight {VER(>=8.0.0)}
cluster.setOption("memberWeight", 25);

//@<OUT> WL#11465: setOption memberWeight 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
cluster.setOption("memberWeight", 25);

//@<OUT> WL#11465: Verify memberWeight changed correctly in instance 1
print_member_weight_variable(session);
session.close();

shell.connect(__sandbox_uri2);
//@<OUT> WL#11465: Verify memberWeight changed correctly in instance 2
print_member_weight_variable(session);
session.close();

shell.connect(__sandbox_uri3);
//@<OUT> WL#11465: Verify memberWeight changed correctly in instance 3
print_member_weight_variable(session);
session.close();

//@<ERR> WL#11465: setOption exitStateAction with invalid value
cluster.setOption("exitStateAction", "ABORT");

//@<OUT> WL#11465: setOption exitStateAction {VER(>=8.0.0)}
cluster.setOption("exitStateAction", "ABORT_SERVER");

//@<OUT> WL#11465: setOption exitStateAction 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
cluster.setOption("exitStateAction", "ABORT_SERVER");

//@<OUT> WL#11465: Verify exitStateAction changed correctly in instance 1
shell.connect(__sandbox_uri1);
print_exit_state_action_variable(session);
session.close();

shell.connect(__sandbox_uri2);
//@<OUT> WL#11465: Verify exitStateAction changed correctly in instance 2
print_exit_state_action_variable(session);
session.close();

shell.connect(__sandbox_uri3);
//@<OUT> WL#11465: Verify exitStateAction changed correctly in instance 3
print_exit_state_action_variable(session);
session.close();

//@<OUT> WL#11465: setOption consistency {VER(>=8.0.14)}
cluster.setOption("consistency", "BEFORE_ON_PRIMARY_FAILOVER");

shell.connect(__sandbox_uri1);
//@<OUT> WL#11465: Verify consistency changed correctly in instance 1 {VER(>=8.0.14)}
print_consistency_variable(session);
session.close();

shell.connect(__sandbox_uri2);
//@<OUT> WL#11465: Verify consistency changed correctly in instance 2 {VER(>=8.0.14)}
print_consistency_variable(session);
session.close();

shell.connect(__sandbox_uri3);
//@<OUT> WL#11465: Verify consistency changed correctly in instance 3 {VER(>=8.0.14)}
print_consistency_variable(session);
session.close();

//@<OUT> WL#11465: setOption expelTimeout {VER(>=8.0.14)}
cluster.setOption("expelTimeout", 3500);

shell.connect(__sandbox_uri1);
//@<OUT> WL#11465: Verify expelTimeout changed correctly in instance 1 {VER(>=8.0.14)}
print_expel_timeout_variable(session);
session.close();

shell.connect(__sandbox_uri2);
//@<OUT> WL#11465: Verify expelTimeout changed correctly in instance 2 {VER(>=8.0.14)}
print_expel_timeout_variable(session);
session.close();

shell.connect(__sandbox_uri3);
//@<OUT> WL#11465: Verify expelTimeout changed correctly in instance 3 {VER(>=8.0.14)}
print_expel_timeout_variable(session);
session.close();

//@<OUT> WL#12066: TSF6_1 setOption autoRejoinTries {VER(>=8.0.16)}
cluster.setOption("autoRejoinTries", 2016);

//@ WL#12066: TSF2_4 setOption autoRejoinTries doesn't accept negative values {VER(>=8.0.16)}
cluster.setOption("autoRejoinTries", -1);

//@ WL#12066: TSF2_5 setOption autoRejoinTries doesn't accept values out of range {VER(>=8.0.16)}
cluster.setOption("autoRejoinTries", 2017);

shell.connect(__sandbox_uri1);
//@ WL#12066: TSF2_3 Verify autoRejoinTries changed correctly in instance 1 {VER(>=8.0.16)}
print_auto_rejoin_tries_variable(session);
print_persisted_variables_like(session, "group_replication_autorejoin_tries");
session.close();

shell.connect(__sandbox_uri2);
//@ WL#12066: TSF2_3 Verify autoRejoinTries changed correctly in instance 2 {VER(>=8.0.16)}
print_auto_rejoin_tries_variable(session);
print_persisted_variables_like(session, "group_replication_autorejoin_tries");
session.close();

shell.connect(__sandbox_uri3);
//@ WL#12066: TSF2_3 Verify autoRejoinTries changed correctly in instance 3 {VER(>=8.0.16)}
print_auto_rejoin_tries_variable(session);
print_persisted_variables_like(session, "group_replication_autorejoin_tries");
session.close();

//@ WL#11465: Finalization
scene.destroy();
