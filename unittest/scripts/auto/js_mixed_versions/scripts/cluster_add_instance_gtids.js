//@ {DEF(MYSQLD_SECONDARY_SERVER_A) && VER(>=8.3.0) && testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, "<", "8.3.0")}

// Deploy sandboxes with secondary version (< 8.3)
testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: localhost }, { mysqldPath: MYSQLD_SECONDARY_SERVER_A.path });

// Deploy sandbox with current version (>= 8.3)
testutil.deploySandbox(__mysql_sandbox_port2, "root", { report_host: localhost });

//@<> Check if the primary instance detects tagged GTIDs from a joining instance (Cluster)
shell.connect(__sandbox_uri2);
session.runSql("SET gtid_next = 'AUTOMATIC:foo'");
session.runSql("CREATE DATABASE test");

shell.connect(__sandbox_uri1);

var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

shell.options.useWizards=0;
EXPECT_THROWS(function() {
    cluster.addInstance(__sandbox_uri2);
}, "Instance provisioning required");

EXPECT_OUTPUT_CONTAINS(`WARNING: A GTID set check found GTIDs with tags, which the donor instance '${localhost}:${__mysql_sandbox_port1}' doesn't support. In order to use GTID tags, all members of the Cluster must support them.`);
EXPECT_OUTPUT_CONTAINS(`Discarding these extra GTID events can either be done manually or by completely overwriting the state of '${localhost}:${__mysql_sandbox_port2}' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);

//@<> Check if the primary cluster detects tagged GTIDs from a joining replica cluster (ClusterSet) {testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, ">=", "8.0.27")}
var cset = cluster.createClusterSet("cset");

shell.options.useWizards=0;
EXPECT_THROWS(function() {
    cset.createReplicaCluster(__sandbox_uri2, "c2");
}, "Instance provisioning required");

EXPECT_OUTPUT_CONTAINS(`WARNING: A GTID set check found GTIDs with tags, which the donor instance '${localhost}:${__mysql_sandbox_port1}' doesn't support. In order to use GTID tags, all members of the ClusterSet must support them.`);
EXPECT_OUTPUT_CONTAINS(`Discarding these extra GTID events can either be done manually or by completely overwriting the state of '${localhost}:${__mysql_sandbox_port2}' with a physical snapshot from an existing clusterset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);

//@<> Check if the primary instance detects tagged GTIDs from a joining instance (ReplicaSet) {testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, ">=", "8.0.11")}
reset_instance(session, true);

var rset = dba.createReplicaSet("cluster", {gtidSetIsComplete: true});

shell.options.useWizards=0;
EXPECT_THROWS(function() {
    rset.addInstance(__sandbox_uri2);
}, "Instance provisioning required");

EXPECT_OUTPUT_CONTAINS(`WARNING: A GTID set check found GTIDs with tags, which the donor instance '${localhost}:${__mysql_sandbox_port1}' doesn't support. In order to use GTID tags, all members of the ReplicaSet must support them.`);
EXPECT_OUTPUT_CONTAINS(`Discarding these extra GTID events can either be done manually or by completely overwriting the state of '${localhost}:${__mysql_sandbox_port2}' with a physical snapshot from an existing replicaset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);

//@<> Check if the primary instance detects tagged GTIDs from a re-joining instance (Cluster)
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2, true);

shell.connect(__sandbox_uri1);
reset_instance(session, true);

cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
cluster.addInstance(__sandbox_uri2);

session2.runSql("STOP GROUP_REPLICATION");
session2.runSql("SET GLOBAL super_read_only = 0");
session2.runSql("SET gtid_next = 'AUTOMATIC:foo'");
session2.runSql("CREATE DATABASE test");

EXPECT_THROWS(function() {
    cluster.rejoinInstance(__sandbox_uri2);
}, "Instance provisioning required");

EXPECT_OUTPUT_CONTAINS(`WARNING: A GTID set check found GTIDs with tags, which the donor instance '${localhost}:${__mysql_sandbox_port1}' doesn't support. In order to use GTID tags, all members of the Cluster must support them.`);
EXPECT_OUTPUT_CONTAINS(`Discarding these extra GTID events can either be done manually or by completely overwriting the state of '${localhost}:${__mysql_sandbox_port2}' with a physical snapshot from an existing cluster member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);

//@<> Check if reboot (Cluster)

session2.runSql("STOP GROUP_REPLICATION");
session.runSql("STOP GROUP_REPLICATION");

EXPECT_THROWS(function() {
    dba.rebootClusterFromCompleteOutage();
}, `Instance '${localhost}:${__mysql_sandbox_port1}' doesn't support GTID tags.`);

EXPECT_OUTPUT_CONTAINS("A GTID set with tags was detected while verifying GTID compatibility between Cluster instances. To use GTID tags, all members of the Cluster must support them.");

//@<> Check if the primary instance detects tagged GTIDs from a re-joining instance (ReplicaSet) {testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, ">=", "8.0.11")}
reset_instance(session2, true);
reset_instance(session, true);

rset = dba.createReplicaSet("cluster", {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);

session2.runSql("STOP REPLICA");
session2.runSql("RESET REPLICA ALL");
session2.runSql("SET GLOBAL super_read_only = 0");
session2.runSql("SET gtid_next = 'AUTOMATIC:foo'");
session2.runSql("CREATE DATABASE test");

EXPECT_THROWS(function() {
    rset.rejoinInstance(__sandbox_uri2);
}, "Instance provisioning required");

EXPECT_OUTPUT_CONTAINS(`WARNING: A GTID set check found GTIDs with tags, which the donor instance '${localhost}:${__mysql_sandbox_port1}' doesn't support. In order to use GTID tags, all members of the ReplicaSet must support them.`);
EXPECT_OUTPUT_CONTAINS(`Discarding these extra GTID events can either be done manually or by completely overwriting the state of '${localhost}:${__mysql_sandbox_port2}' with a physical snapshot from an existing replicaset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.`);

session2.close();

//@<> Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
