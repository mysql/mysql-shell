//@ {VER(>=5.7.17)}

// Assumptions: smart deployment rountines available

// Due to the fixes for BUG #26159339: SHELL: ADMINAPI DOES NOT TAKE
// GROUP_NAME INTO ACCOUNT
// We must reset the instances to reset 'group_replication_group_name' since on
// the previous test Shell_js_dba_tests.configure_local_instance the instances
// configurations are persisted on my.cnf so upon the restart of any instance
// it can bootstrap a new group, changing the value of
// 'group_replication_group_name' and leading to a failure on forceQuorum
// cleanup_sandboxes(true);

//@<> Initialization
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {memberSslMode: 'REQUIRED', gtidSetIsComplete: true});
var cluster = scene.cluster;

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
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@<> Disconnect and reconnect to instance
// Regression for BUG#27148943: getCluster() should warn when connected to member with no quorum
cluster.disconnect();
session.close();
shell.connect(__sandbox_uri1);

//@<OUT> Get cluster operation must show a warning because there is no quorum
// Regression for BUG#27148943: getCluster() should warn when connected to member with no quorum
var cluster = dba.getCluster();

//@ Cluster.forceQuorumUsingPartitionOf errors
cluster.forceQuorumUsingPartitionOf();
cluster.forceQuorumUsingPartitionOf(1);
cluster.forceQuorumUsingPartitionOf("");
cluster.forceQuorumUsingPartitionOf(1, "");
cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port2, password:'root', user:'root'});

//@<> enable interactive mode
shell.options.useWizards=true;

//@<OUT> Cluster.forceQuorumUsingPartitionOf success intractive
testutil.expectPassword(`Please provide the password for 'root@localhost:${__mysql_sandbox_port1}': `, "root");
cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1, user: 'root'});

//@<> disable interactive mode
shell.options.useWizards=false;

//@<> Cluster.forceQuorumUsingPartitionOf success noninteractive
cluster.rejoinInstance(__sandbox_uri2);
cluster.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
scene.make_no_quorum([__mysql_sandbox_port1]);
testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitForDelayedGRStart(__mysql_sandbox_port2, "root");
testutil.waitForDelayedGRStart(__mysql_sandbox_port3, "root");
cluster.forceQuorumUsingPartitionOf({host:localhost, port: __mysql_sandbox_port1, user: 'root', password:'root'});

//@<> Cluster status after force quorum
var status = cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"], "Status for sandbox 1 is invalid")
EXPECT_EQ("(MISSING)", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"], "Status for sandbox 1 is invalid")
EXPECT_EQ("(MISSING)", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"], "Status for sandbox 1 is invalid")
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"], "Mode for sandbox 1 is invalid")
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"], "Mode for sandbox 2 is invalid")
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"], "Mode for sandbox 3 is invalid")

//@<> Rejoin instance 2
cluster.rejoinInstance({host:localhost, port: __mysql_sandbox_port2, password:'root', user:'root'}, {memberSslMode: 'REQUIRED'});

// Waiting for the second rejoined instance to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Rejoin instance 3
cluster.rejoinInstance({host:localhost, port: __mysql_sandbox_port3, password:'root', user:'root'}, {memberSslMode: 'REQUIRED'});

// Waiting for the third rejoined instance to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Cluster status after rejoins
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

//@<OUT> BUG#30739252: Confirm instance 3 is never included as OFFLINE (no undefined behaviour) {VER(>=8.0.16)}
shell.dumpRows(session.runSql("SELECT * FROM performance_schema.replication_group_members"), "tabbed");

// --- END --- BUG#30739252 : race condition in forcequorum

//@<> Finalization
//  Will close opened sessions and delete the sandboxes ONLY if this test was executed standalone
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
