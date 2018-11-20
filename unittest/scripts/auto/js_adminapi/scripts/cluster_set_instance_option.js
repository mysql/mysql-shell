function print_metadata_instance_label(session, address) {
  var res = session.runSql("select instance_name from mysql_innodb_cluster_metadata.instances where addresses->'$.mysqlClassic' = '" + address + "'");
  var row = res.fetchOne();
  print(row[0] + "\n");
}

function print_member_weight_variable(session) {
  var res = session.runSql('SHOW VARIABLES like "group_replication_member_weight"');
  var row = res.fetchOne();
  print(row[1] + "\n");
}

function print_exit_state_action_variable(session) {
  var res = session.runSql('SHOW VARIABLES like "group_replication_exit_state_action"');
  var row = res.fetchOne();
  print(row[1] + "\n");
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
//   - If the 'instance' parameter is empty.
//   - If the 'instance' definition is invalid.
//   - If the 'instance' definition is a connection dictionary but empty.
//   - If the 'option' parameter is empty.
//   - If the 'value' parameter is empty.
//   - If the 'option' parameter is invalid.
//
//   - If 'instance' does not refer to a cluster member
//   - If the cluster has no visible quorum.
//   - If 'instance' is not ONLINE.
//   - If 'instance' does not support the configuration option passed in 'option'.
//   - If the value passed in 'option' is not valid for Group Replication.


//@ WL#11465: ArgumentErrors of setInstanceOption
cluster.setInstanceOption();
cluster.setInstanceOption("", "memberWeight", 1);
cluster.setInstanceOption(1234, "memberWeight", 1);
cluster.setInstanceOption({}, "memberWeight", 1);
cluster.setInstanceOption(__sandbox_uri2);
cluster.setInstanceOption(__sandbox_uri2, "memberWeight");
cluster.setInstanceOption(__sandbox_uri2, "foobar", 1);
cluster.setInstanceOption(null, "label", "newLabel");

// F2.2.1.2 - The function shall not be allowed if the target 'instance' does not belong to the cluster.
//@<ERR> WL#11465: Error when executing setInstanceOption for a target instance that does not belong to the cluster
cluster.setInstanceOption("localhost:3350", "memberWeight", 25);

// F2.2.1.1 - The function shall not be allowed if the cluster does not have quorum or the target 'instance' is not ONLINE.
//@ WL#11465: Error when executing setInstanceOption when the target instance is not ONLINE
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.setInstanceOption(__sandbox_uri3, "memberWeight", 25);

//@<ERR> WL#11465: Error when executing setInstanceOption on a cluster with no visible quorum {VER(>=8.0.14)}
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25);

//@<ERR> WL#11465: Error when executing setInstanceOption on a cluster with no visible quorum 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
testutil.startSandbox(__mysql_sandbox_port3);
cluster.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25);

//@ WL#11465: Re-create the cluster
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster

// F1.5.1.2 - The accepted values are:
//
// - label
// - exitStateAction
// - memberWeight

//@<ERR> WL#11465: setInstanceOption label with invalid value for label 1
cluster.setInstanceOption(__sandbox_uri2, "label", "-0_a");

//@<ERR> WL#11465: setInstanceOption label with invalid value for label 2
var __invalid_label = localhost + ":" + __mysql_sandbox_port1;

cluster.setInstanceOption(__sandbox_uri2, "label", __invalid_label);

//@<OUT> WL#11465: setInstanceOption label
cluster.setInstanceOption(__sandbox_uri2, "label", "newLabel");

session.close();
shell.connect(__sandbox_uri2);
//@<OUT> WL#11465: Verify label changed correctly
var __address2 = localhost + ":" + __mysql_sandbox_port2;
print_metadata_instance_label(session, __address2);

//@<OUT> WL#11465: setInstanceOption memberWeight
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25);

session.close();
shell.connect(__sandbox_uri2);
//@<OUT> WL#11465: memberWeight label changed correctly
print_member_weight_variable(session);

//@<ERR> WL#11465: setInstanceOption exitStateAction with invalid value
cluster.setInstanceOption(__sandbox_uri2, "exitStateAction", "ABORT");

//@<OUT> WL#11465: setInstanceOption exitStateAction
cluster.setInstanceOption(__sandbox_uri2, "exitStateAction", "ABORT_SERVER");

session.close();
shell.connect(__sandbox_uri2);
//@<OUT> WL#11465: exitStateAction label changed correctly
print_exit_state_action_variable(session);
session.close();

//@ WL#11465: Finalization
scene.destroy();
