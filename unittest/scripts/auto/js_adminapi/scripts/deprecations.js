//@{!__dbug_off}

//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1]);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
var cluster = scene.cluster;
shell.connect(__sandbox_uri1);

//@<> A deprecation warning must be emitted when using the command dba.configureLocalInstance()
dba.configureLocalInstance();

EXPECT_OUTPUT_CONTAINS(`This function is deprecated and will be removed in a future release of MySQL Shell, use dba.configureInstance() instead.`);
WIPE_OUTPUT()

//@<> A deprecation warning must be emitted when using the command Cluster.checkInstanceState().
cluster.checkInstanceState(__sandbox_uri2)

EXPECT_OUTPUT_CONTAINS(`This function is deprecated and will be removed in a future release of MySQL Shell.`);
WIPE_OUTPUT()

//@<> A deprecation warning must be emitted when using the `interactive` password in any AdminAPI command {!__dbug_off}
testutil.dbugSet("+d,dba_deprecated_option_fail");

EXPECT_THROWS_TYPE(function() { dba.checkInstanceConfiguration(__sandbox_uri1, {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { dba.configureInstance(__sandbox_uri1, {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { dba.configureLocalInstance(__sandbox_uri1, {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { dba.configureReplicaSetInstance(__sandbox_uri1, {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

shell.connect(__sandbox_uri2);

EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { dba.createReplicaSet("test", {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { dba.upgradeMetadata({interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.addInstance(__endpoint2, {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.dissolve({interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__endpoint2, {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__endpoint2, {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.rescan({interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.resetRecoveryAccountsPassword({interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.setupAdminAccount("foo", {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.setupRouterAccount("foo", {interactive: true}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

testutil.dbugSet("");

EXPECT_NO_THROWS(function() { cluster.dissolve(); })

if (__version_num > 80017) {
  shell.connect(__sandbox_uri1);
  EXPECT_NO_THROWS(function() { replicaset = dba.createReplicaSet("rs"); })

  testutil.dbugSet("+d,dba_deprecated_option_fail");

  EXPECT_THROWS_TYPE(function() { replicaset.addInstance(__endpoint2, {interactive: true}); }, "debug", "ArgumentError");
  EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
  WIPE_OUTPUT()

  EXPECT_THROWS_TYPE(function() { replicaset.rejoinInstance(__endpoint2, {interactive: true}); }, "debug", "ArgumentError");
  EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
  WIPE_OUTPUT()

  EXPECT_THROWS_TYPE(function() { replicaset.setupAdminAccount("foo", {interactive: true}); }, "debug", "ArgumentError");
  EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
  WIPE_OUTPUT()

  EXPECT_THROWS_TYPE(function() { replicaset.setupRouterAccount("foo", {interactive: true}); }, "debug", "ArgumentError");
  EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
  WIPE_OUTPUT()
} else {
  testutil.dbugSet("+d,dba_deprecated_option_fail");
}

shell.connect(__sandbox_uri2);
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("c"); })

if (__version_num > 80027) {
  EXPECT_NO_THROWS(function() { clusterset = cluster.createClusterSet("cs"); })

  EXPECT_THROWS_TYPE(function() { clusterset.createReplicaCluster(__endpoint2, "bar", {interactive: true}); }, "debug", "ArgumentError");
  EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
  WIPE_OUTPUT()

  EXPECT_THROWS_TYPE(function() { clusterset.setupAdminAccount("foo", {interactive: true}); }, "debug", "ArgumentError");
  EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
  WIPE_OUTPUT()

  EXPECT_THROWS_TYPE(function() { clusterset.setupRouterAccount("foo", {interactive: true}); }, "debug", "ArgumentError");
  EXPECT_OUTPUT_CONTAINS(`WARNING: The interactive option is deprecated and will be removed in a future release.`);
  WIPE_OUTPUT()
}

//@<> A deprecation warning must be emitted when the options `user` and/or `password` are used in any AdminAPI command that supports it

EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri1, {password: "foo"}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The password option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.removeInstance(__sandbox_uri1, {password: "foo"}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The password option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.rejoinInstance(__sandbox_uri1, {password: "foo"}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The password option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.forceQuorumUsingPartitionOf(__sandbox_uri1, "foo"); }, "debug", "LogicError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The password option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

session.runSql("stop group_replication");

EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("c", {password: "foo"}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The 'password' option is no longer used (it's deprecated): the connection data is taken from the active shell session.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { dba.checkInstanceConfiguration(__sandbox_uri1, {password: "foo"}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The password option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { dba.configureInstance(__sandbox_uri1, {password: "foo"}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The password option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { dba.configureLocalInstance(__sandbox_uri1, {password: "foo"}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The password option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { dba.configureReplicaSetInstance(__sandbox_uri1, {password: "foo"}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The password option is deprecated and will be removed in a future release.`);
WIPE_OUTPUT()

//@<> A deprecation warning must be emitted when using the `waitRecovery` option in any AdminAPI command, also suggesting the usage of `recoveryProgress`
EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri1, {waitRecovery: 0}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The waitRecovery option is deprecated. Please use the recoveryProgress option instead.`);
WIPE_OUTPUT()

EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri1, {waitRecovery: 0, recoveryProgress: 0}); }, "Cannot use the waitRecovery and recoveryProgress options simultaneously. The waitRecovery option is deprecated, please use the recoveryProgress option instead.", "ArgumentError");

if (__version_num > 80017) {
  shell.connect(__sandbox_uri1);
  EXPECT_NO_THROWS(function() { replicaset = dba.getReplicaSet(); })

  EXPECT_THROWS_TYPE(function() { replicaset.addInstance(__sandbox_uri2, {waitRecovery: 0}); }, "debug", "ArgumentError");
  EXPECT_OUTPUT_CONTAINS(`WARNING: The waitRecovery option is deprecated. Please use the recoveryProgress option instead.`);
  WIPE_OUTPUT()

  EXPECT_THROWS_TYPE(function() { replicaset.addInstance(__sandbox_uri2, {waitRecovery: 0, recoveryProgress: 0}); }, "Cannot use the waitRecovery and recoveryProgress options simultaneously. The waitRecovery option is deprecated, please use the recoveryProgress option instead.", "ArgumentError");
}

//@<> A deprecation warning must be emitted when the option `clearReadOnly` is used in `dba.configureInstance()`, `dba.configureLocalInstance`, and `dba.dropMetadataSchema()`.

// The command must automatically clear super_read_only if needed.

EXPECT_THROWS_TYPE(function() { dba.dropMetadataSchema({clearReadOnly: 1}); }, "debug", "ArgumentError");
EXPECT_OUTPUT_CONTAINS(`WARNING: The clearReadOnly option is deprecated and will be removed in a future release.`);

testutil.dbugSet("");

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);
