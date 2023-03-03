//@<> INCLUDE gr_utils.inc

//@<> Check options
EXPECT_THROWS(function (){ dba.rebootClusterFromCompleteOutage("", {user:""}); }, "Dba.rebootClusterFromCompleteOutage: The shell must be connected to a member of the InnoDB cluster being managed");
EXPECT_OUTPUT_CONTAINS("WARNING: The 'user' option is no longer used (it's deprecated): the connection data is taken from the active shell session.");

testutil.wipeAllOutput();
EXPECT_THROWS(function (){ dba.rebootClusterFromCompleteOutage("", {password:""}); }, "Dba.rebootClusterFromCompleteOutage: The shell must be connected to a member of the InnoDB cluster being managed");
EXPECT_OUTPUT_CONTAINS("WARNING: The 'password' option is no longer used (it's deprecated): the connection data is taken from the active shell session.");

testutil.wipeAllOutput();
EXPECT_THROWS(function (){ dba.rebootClusterFromCompleteOutage("", {clearReadOnly:false}); }, "Dba.rebootClusterFromCompleteOutage: The shell must be connected to a member of the InnoDB cluster being managed");
EXPECT_OUTPUT_CONTAINS("WARNING: The 'clearReadOnly' option is no longer used (it's deprecated): super_read_only is automatically cleared.");

testutil.wipeAllOutput();
EXPECT_THROWS(function (){ dba.rebootClusterFromCompleteOutage("", {primary:""}); }, "Invalid value '' for 'primary' option: Invalid URI: empty.");
EXPECT_THROWS(function (){ dba.rebootClusterFromCompleteOutage("", {primary:"host"}); }, "Invalid value 'host' for 'primary' option: port is missing");
EXPECT_THROWS(function (){ dba.rebootClusterFromCompleteOutage("", {primary:":12"}); }, "Invalid value ':12' for 'primary' option: host cannot be empty.");
EXPECT_THROWS(function (){ dba.rebootClusterFromCompleteOutage("", {primary:":port"}); }, "Invalid value ':port' for 'primary' option: Invalid URI: Illegal character [p] found at position 1");

testutil.wipeAllOutput();
EXPECT_THROWS(function (){ dba.rebootClusterFromCompleteOutage("", {timeout:""}); }, "Option 'timeout' UInteger expected, but value is String");
EXPECT_THROWS(function (){ dba.rebootClusterFromCompleteOutage("", {timeout:-1}); }, "Option 'timeout' UInteger expected, but Integer value is out of range");

//@<> Initialization
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);

scene.session.close();

//@<> FR1 reboot an ONLINE cluster
shell.connect(__sandbox_uri1);

EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage();
}, `The Cluster is ONLINE`);

EXPECT_OUTPUT_CONTAINS(`Cluster instances: '${hostname}:${__mysql_sandbox_port1}' (ONLINE), '${hostname}:${__mysql_sandbox_port2}' (ONLINE), '${hostname}:${__mysql_sandbox_port3}' (ONLINE)`);

//@<> FR1 reboot should work even if metadata is dropped

testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2,__mysql_sandbox_port3]);

shell.connect(__sandbox_uri3);
dba.dropMetadataSchema({force: true, clearReadOnly: true});
session.close();

shell.connect(__sandbox_uri1);

EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage();
}, `The instance '${hostname}:${__mysql_sandbox_port3}' doesn't belong to the Cluster. Use option 'force' to ignore this check.`);

// to test if reboot doesn't persist the variable (BUG#34778797)
session.runSql("SET GLOBAL offline_mode = 1");
var session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("SET GLOBAL offline_mode = 1");

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("cluster", {force: true}); });
EXPECT_OUTPUT_CONTAINS(`WARNING: The instance '${hostname}:${__mysql_sandbox_port3}' doesn't belong to the Cluster and will be ignored.`);

//make sure that "offline_mode" is disabled (BUG#34778797)
EXPECT_OUTPUT_CONTAINS(`Disabling 'offline_mode' on '${hostname}:${__mysql_sandbox_port2}'`);

//@<> make sure that "offline_mode" isn't persisted (BUG#34778797) {VER(<8.0.11)}
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});
EXPECT_THROWS(function() { testutil.getSandboxConf(__mysql_sandbox_port1, "offline_mode"); }, "Option 'offline_mode' does not exist in group 'mysqld'");
EXPECT_THROWS(function() { testutil.getSandboxConf(__mysql_sandbox_port2, "offline_mode"); }, "Option 'offline_mode' does not exist in group 'mysqld'");
session2.close();

//@<> make sure that "offline_mode" isn't persisted (BUG#34778797) {VER(>=8.0.11)}
EXPECT_EQ(0, session.runSql("SELECT count(*) FROM performance_schema.persisted_variables WHERE (variable_name = 'offline_mode')").fetchOne()[0], `Variable 'offline_mode' is persisted in '${__sandbox_uri1}'`);
EXPECT_EQ(0, session2.runSql("SELECT count(*) FROM performance_schema.persisted_variables WHERE (variable_name = 'offline_mode')").fetchOne()[0], `Variable 'offline_mode' is persisted in '${__sandbox_uri2}'`);
session2.close();

//@<> reset cluster

cluster.removeInstance(__sandbox_uri3, {force: true});

shell.connect(__sandbox_uri3);
reset_instance(session);

cluster.addInstance(__sandbox_uri3);

shell.connect(__sandbox_uri1);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> FR1 reboot should work even if instances enter recovery automatically
dba.configureLocalInstance(__sandbox_uri1);
dba.configureLocalInstance(__sandbox_uri2);
dba.configureLocalInstance(__sandbox_uri3);

testutil.killSandbox(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port3);
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() {
    cluster = dba.rebootClusterFromCompleteOutage(null);
});
EXPECT_OUTPUT_CONTAINS(`NOTE: Cancelling active GR auto-initialization at ${hostname}:${__mysql_sandbox_port1}`);
EXPECT_OUTPUT_CONTAINS_ONE_OF([
    `Rejoining instance '${hostname}:${__mysql_sandbox_port2}' to cluster 'cluster'...`,
    `NOTE: The instance '${hostname}:${__mysql_sandbox_port2}' is running auto-rejoin process, which will be cancelled.`,
    `NOTE: ${hostname}:${__mysql_sandbox_port2} already rejoined the cluster 'cluster'.`
]);
EXPECT_OUTPUT_CONTAINS_ONE_OF([
    `Rejoining instance '${hostname}:${__mysql_sandbox_port3}' to cluster 'cluster'...`,
    `NOTE: The instance '${hostname}:${__mysql_sandbox_port3}' is running auto-rejoin process, which will be cancelled.`,
    `NOTE: ${hostname}:${__mysql_sandbox_port3} already rejoined the cluster 'cluster'.`,
]);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> FR1 fail if we have a reachable member
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage();
}, "The Cluster is ONLINE");
EXPECT_OUTPUT_CONTAINS(`Cluster instances: '${hostname}:${__mysql_sandbox_port1}' (ONLINE), '${hostname}:${__mysql_sandbox_port2}' (UNREACHABLE), '${hostname}:${__mysql_sandbox_port3}' (ONLINE)`);

// lose quorum
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage();
}, `The MySQL instance '${hostname}:${__mysql_sandbox_port1}' belongs to an InnoDB Cluster and is reachable. Please use <Cluster>.forceQuorumUsingPartitionOf() to restore from the quorum loss.`);

testutil.killSandbox(__mysql_sandbox_port1);

//@<> FR1 must fail if we have an unreachable or !OFFLINE member
testutil.startSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage();
}, "Could not determine if Cluster is completely OFFLINE");
EXPECT_OUTPUT_CONTAINS(`WARNING: One or more instances of the Cluster could not be reached and cannot be rejoined nor ensured to be OFFLINE: '${hostname}:${__mysql_sandbox_port2}', '${hostname}:${__mysql_sandbox_port3}'. Cluster may diverge and become inconsistent unless all instances are either reachable or certain to be OFFLINE and not accepting new transactions. You may use the 'force' option to bypass this check and proceed anyway.`);

testutil.startSandbox(__mysql_sandbox_port3);

testutil.wipeAllOutput();

EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage();
}, "Could not determine if Cluster is completely OFFLINE");
EXPECT_OUTPUT_CONTAINS(`WARNING: One or more instances of the Cluster could not be reached and cannot be rejoined nor ensured to be OFFLINE: '${hostname}:${__mysql_sandbox_port2}'. Cluster may diverge and become inconsistent unless all instances are either reachable or certain to be OFFLINE and not accepting new transactions. You may use the 'force' option to bypass this check and proceed anyway.`);

testutil.startSandbox(__mysql_sandbox_port2);

testutil.wipeAllOutput();

EXPECT_NO_THROWS(function() {
    cluster = dba.rebootClusterFromCompleteOutage();
});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> FR1 must work if member is OFFLINE
testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2,__mysql_sandbox_port3]);

shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function () { cluster = dba.rebootClusterFromCompleteOutage("cluster", {dryRun: true}); });
EXPECT_OUTPUT_CONTAINS("NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.");
EXPECT_OUTPUT_CONTAINS("dryRun finished.");
EXPECT_OUTPUT_NOT_CONTAINS("Restoring the Cluster 'cluster' from complete outage...");

testutil.wipeAllOutput();

EXPECT_NO_THROWS(function () { cluster = dba.rebootClusterFromCompleteOutage(); });
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.");
EXPECT_OUTPUT_CONTAINS("Restoring the Cluster 'cluster' from complete outage...");

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

status = cluster.status();
EXPECT_EQ(3, Object.keys(status["defaultReplicaSet"]["topology"]).length)
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])

//@<> BUG#34647972 make sure SRO isn't changed in multi-primary topology {VER(>=8.0.13)}

EXPECT_NO_THROWS(function () { cluster.switchToMultiPrimaryMode(); });

testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2,__mysql_sandbox_port3]);

shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function () { cluster = dba.rebootClusterFromCompleteOutage(); });

status = cluster.status();
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

EXPECT_NO_THROWS(function () { cluster.switchToSinglePrimaryMode(); });

//@<> FR1 if a member is in ERROR and the rest is OFFLINE or in ERROR, the command must succeed stopping GR where necessary
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port3);

shell.connect(__sandbox_uri3);
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("SET GLOBAL read_only = 0");
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE DATABASE error_trx_db");
session.runSql("SET sql_log_bin = 1");
session.runSql("SET GLOBAL super_read_only = 1");

shell.connect(__sandbox_uri2);
session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("SET GLOBAL read_only = 0");
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE DATABASE error_trx_db");
session.runSql("SET sql_log_bin = 1");
session.runSql("SET GLOBAL super_read_only = 1");

shell.connect(__sandbox_uri1);
session.runSql("CREATE DATABASE error_trx_db");

shell.connect(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ERROR");
shell.connect(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ERROR");

testutil.stopGroup([__mysql_sandbox_port1]);

shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage(); });
EXPECT_OUTPUT_CONTAINS(`Cluster instances: '${hostname}:${__mysql_sandbox_port1}' (OFFLINE), '${hostname}:${__mysql_sandbox_port2}' (ERROR), '${hostname}:${__mysql_sandbox_port3}' (ERROR)`);
EXPECT_OUTPUT_CONTAINS(`Stopping Group Replication on '${hostname}:${__mysql_sandbox_port2}'...`);
EXPECT_OUTPUT_CONTAINS(`Stopping Group Replication on '${hostname}:${__mysql_sandbox_port3}'...`);
EXPECT_OUTPUT_CONTAINS(`NOTE: Unable to rejoin instance '${hostname}:${__mysql_sandbox_port2}' to the Cluster but the dba.rebootClusterFromCompleteOutage() operation will continue.`);
EXPECT_OUTPUT_CONTAINS(`NOTE: Unable to rejoin instance '${hostname}:${__mysql_sandbox_port3}' to the Cluster but the dba.rebootClusterFromCompleteOutage() operation will continue.`);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
