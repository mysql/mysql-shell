//@ {VER(>=5.7.17)}

// Assumptions: smart deployment rountines available

// Due to the fixes for BUG #26159339: SHELL: ADMINAPI DOES NOT TAKE
// GROUP_NAME INTO ACCOUNT
// We must reset the instances to reset 'group_replication_group_name' since on
// the previous test Shell_js_dba_tests.configure_local_instance the instances
// configurations are persisted on my.cnf so upon the restart of any instance
// it can bootstrap a new group, changing the value of
// 'group_replication_group_name' and leading to a failure on forceQuorum

//@<> INCLUDE read_replicas_utils.inc

//@<> Initialization
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {memberSslMode: 'REQUIRED', gtidSetIsComplete: true});
var cluster = scene.cluster;

testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

//@<> forceQuorumUsingPartitionOf() must not be allowed on cluster with quorum
// Regression for BUG#27508698: forceQuorumUsingPartitionOf() on cluster with quorum should be forbidden
EXPECT_THROWS(function() {cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1, password:'root', user:'root'});}, `Cluster.forceQuorumUsingPartitionOf: The cluster has quorum according to instance 'localhost:${__mysql_sandbox_port1}'`)
EXPECT_STDOUT_CONTAINS("ERROR: Cannot perform operation on an healthy cluster because it can only be used to restore a cluster from quorum loss.");

//@<> Disable group_replication_start_on_boot on second instance {VER(>=8.0.11)}
// If we don't set the start_on_boot variable to OFF, it is possible that instance 2 will
// be still trying to join the cluster from the moment it was started again until
// the cluster is unlocked after the forceQuorumUsingPartitionOf command
var s2 = mysql.getSession({host:localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
s2.runSql("SET PERSIST group_replication_start_on_boot = 0");
s2.close();

//@<> Disable group_replication_start_on_boot on second instance {VER(<8.0.11)}
var s2 = mysql.getSession({host:localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
s2.runSql("SET GLOBAL group_replication_start_on_boot = 0");
s2.close();

//@<> Disable group_replication_start_on_boot on third instance {VER(>=8.0.11)}
// If we don't set the start_on_boot variable to OFF, it is possible that instance 3 will
// be still trying to join the cluster from the moment it was started again until
// the cluster is unlocked after the forceQuorumUsingPartitionOf command
var s3 = mysql.getSession({host:localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
s3.runSql("SET PERSIST group_replication_start_on_boot = 0");
s3.close();

//@<> Disable group_replication_start_on_boot on third instance {VER(<8.0.11)}
var s3 = mysql.getSession({host:localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
s3.runSql("SET GLOBAL group_replication_start_on_boot = 0");
s3.close();

//@<> Kill instance 2
testutil.killSandbox(__mysql_sandbox_port2);

// Since the cluster has quorum, the instance will be kicked off the
// Cluster going OFFLINE->UNREACHABLE->(MISSING)
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@<> Kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

// Waiting for the third added instance to become unreachable
// Will remain unreachable since there's no quorum to kick it off
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

//@<> Start instance 2
testutil.startSandbox(__mysql_sandbox_port2);

//@<> Start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

//@<> Cluster status
var status = cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("(MISSING)", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("UNREACHABLE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("n/a", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("n/a", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@<> Disconnect and reconnect to instance
// Regression for BUG#27148943: getCluster() should warn when connected to member with no quorum
cluster.disconnect();
session.close();
shell.connect(__sandbox_uri1);

//@<> Get cluster operation must show a warning because there is no quorum
// Regression for BUG#27148943: getCluster() should warn when connected to member with no quorum
var cluster = dba.getCluster();
EXPECT_OUTPUT_CONTAINS("WARNING: Cluster has no quorum and cannot process write transactions: Group has no quorum");

//@<> Cluster.forceQuorumUsingPartitionOf errors
EXPECT_THROWS(function(){ cluster.forceQuorumUsingPartitionOf(); }, "Invalid number of arguments, expected 1 but got 0");
EXPECT_THROWS(function(){ cluster.forceQuorumUsingPartitionOf(1); }, "Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary");
EXPECT_THROWS(function(){ cluster.forceQuorumUsingPartitionOf(""); }, "Argument #1: Invalid URI: empty.");
EXPECT_THROWS(function(){ cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port2, password:'root', user:'root'}); }, `The instance '${localhost}:${__mysql_sandbox_port2}' cannot be used to restore the cluster as it is not an active member of replication group.`);

//@<> enable interactive mode
shell.options.useWizards=true;

//@<> Cluster.forceQuorumUsingPartitionOf success (no password)
EXPECT_NO_THROWS(function(){ cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1, user: 'root'}); });

EXPECT_OUTPUT_CONTAINS("Restoring cluster 'cluster' from loss of quorum, by using the partition composed of [" + __endpoint1 + "]");
EXPECT_OUTPUT_CONTAINS("Restoring the InnoDB cluster ...");
EXPECT_OUTPUT_CONTAINS("The InnoDB cluster was successfully restored using the partition from the instance 'root@localhost:" + __mysql_sandbox_port1 + "'.");
EXPECT_OUTPUT_CONTAINS("WARNING: To avoid a split-brain scenario, ensure that all other members of the cluster are removed or joined back to the group that was restored.");

//@<> disable interactive mode
shell.options.useWizards=false;

//@<> Cluster.forceQuorumUsingPartitionOf success noninteractive
cluster.rejoinInstance(__sandbox_uri2);
cluster.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
scene.make_no_quorum([__mysql_sandbox_port1]);
//NOTE: Due to BUG#26394418 (GR) when the remaining cluster members are
//killed and restarted, restoring the quorum can result in a timeout.
//Since this bug was only fixed in 8.0 and not ported to 5.7, we must
//not restart the killed instances.
EXPECT_NO_THROWS(function(){ cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1, user: 'root', password:'root'}); });

//@<> Cluster.forceQuorumUsingPartitionOf check paswword mismatch

testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
scene.make_no_quorum([__mysql_sandbox_port1]);

WIPE_OUTPUT();

EXPECT_THROWS(function(){
    cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1, user: 'root', password:'foo'});
}, "Invalid target instance specification");
EXPECT_OUTPUT_CONTAINS(`Password for user root at ${localhost}:${__mysql_sandbox_port1} must be the same as in the rest of the cluster.`);

WIPE_OUTPUT();

//@<> Cluster.forceQuorumUsingPartitionOf check user mismatch

EXPECT_THROWS(function(){
    cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1, user: 'foo'});
}, `Could not open connection to '${localhost}:${__mysql_sandbox_port1}': Access denied for user 'foo'@'localhost'`);
EXPECT_OUTPUT_CONTAINS(`Unable to connect to the target instance '${localhost}:${__mysql_sandbox_port1}'. Please verify the connection settings, make sure the instance is available and try again.`);

//@<> Cluster.forceQuorumUsingPartitionOf success (user omitted, must fetch from the cluster)
EXPECT_NO_THROWS(function(){ cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1}); });

//@<> Cluster status after force quorum and rejoins

testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

var status = cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@<> STOP group_replication on instance where forceQuorumUsingPartitionOf() was executed.
// Regression for BUG#28064621: group_replication_force_members should be unset after forcing quorum
session.runSql('STOP group_replication');

//@<> Start group_replication on instance the same instance succeeds because group_replication_force_members is empty.
// Regression for BUG#28064621: group_replication_force_members should be unset after forcing quorum
session.runSql('START group_replication');

//@<> dissolve cluster
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.status();
cluster = dba.getCluster();
cluster.dissolve();
cluster.disconnect();

// --- BEGIN --- BUG#30739252 : race condition in forcequorum

//@<> BUG#30739252: create cluster, enabling auto-rejoin. {VER(>=8.0.16)}
shell.connect(__sandbox_uri2);
var c = dba.createCluster("c", {autoRejoinTries: 100});
c.addInstance(__sandbox_uri3, {autoRejoinTries: 100});

//@<> BUG#30739252: kill instance 3 and restart it {VER(>=8.0.16)}
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");
testutil.startSandbox(__mysql_sandbox_port3);

//@<> BUG#30739252: force quorum {VER(>=8.0.16)}
c.forceQuorumUsingPartitionOf(__sandbox_uri2);

//@<> BUG#30739252: Confirm instance 3 is never included as OFFLINE (no undefined behaviour) {VER(>=8.0.16) && VER(<8.0.27)}
shell.dumpRows(session.runSql("SELECT * FROM performance_schema.replication_group_members"), "json");
EXPECT_STDOUT_CONTAINS_MULTILINE(`{
    "CHANNEL_NAME": "group_replication_applier",
    "MEMBER_ID": "[[*]]",
    "MEMBER_HOST": "${hostname}",
    "MEMBER_PORT": ${__mysql_sandbox_port2},
    "MEMBER_STATE": "ONLINE",
    "MEMBER_ROLE": "PRIMARY",
    "MEMBER_VERSION": "${__version}"
}`);

//@<> BUG#30739252: Confirm instance 3 is never included as OFFLINE (no undefined behaviour) {VER(>=8.0.27)}
shell.dumpRows(session.runSql("SELECT * FROM performance_schema.replication_group_members"), "json");
EXPECT_STDOUT_CONTAINS_MULTILINE(`{
    "CHANNEL_NAME": "group_replication_applier",
    "MEMBER_ID": "[[*]]",
    "MEMBER_HOST": "${hostname}",
    "MEMBER_PORT": ${__mysql_sandbox_port2},
    "MEMBER_STATE": "ONLINE",
    "MEMBER_ROLE": "PRIMARY",
    "MEMBER_VERSION": "${__version}",
    "MEMBER_COMMUNICATION_STACK": "MySQL"
}`);

// --- END --- BUG#30739252 : race condition in forcequorum

//@<> forceQuorumUsingPartitionOf() must first stop replication on all reachable ONLINE Read-Replicas of the Cluster and when quorum is restored, restart it. {VER(>=8.0.23)}
EXPECT_NO_THROWS(function() { c.rejoinInstance(__sandbox_uri3); });

// Add a Read-Replica with defaults
EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri4, {recoveryMethod: "clone"}); });

testutil.waitReplicationChannelState(__mysql_sandbox_port4, "read_replica_replication", "ON");

// Add a Read-Replica with replicationSources: __endpoint2, __endpoint1
EXPECT_NO_THROWS(function() { c.addReplicaInstance(__sandbox_uri5, {replicationSources: [__endpoint2, __endpoint3], recoveryMethod: "clone"}); });

testutil.waitReplicationChannelState(__mysql_sandbox_port5, "read_replica_replication", "ON");

// Force quorum loss
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");
testutil.startSandbox(__mysql_sandbox_port3);

EXPECT_NO_THROWS(function() { c.forceQuorumUsingPartitionOf(__sandbox_uri2); });

CHECK_READ_REPLICA(__sandbox_uri4, c, "primary", __endpoint2);
CHECK_READ_REPLICA(__sandbox_uri5, c, [__endpoint2, __endpoint3], [__endpoint2]);

//@<> Finalization
//  Will close opened sessions and delete the sandboxes ONLY if this test was executed standalone
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
