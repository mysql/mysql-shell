// Assumptions: smart deployment rountines available

//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.options.useWizards=false;

shell.connect(__sandbox_uri1);

//@<> Dba: createCluster multiPrimary, ok
var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster('devCluster', {multiPrimary: true, force: true, clearReadOnly: true}); });

cluster.disconnect();

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
var Cluster = dba.getCluster('devCluster');

//@<> Dissolve cluster
EXPECT_NO_THROWS(function() { Cluster.dissolve({force: true}); });

Cluster.disconnect();

//@<> Dba: createCluster multiMaster with interaction, regression for BUG#25926603
EXPECT_NO_THROWS(function() { Cluster = dba.createCluster('devCluster', {multiMaster: true, force: true, clearReadOnly: true, gtidSetIsComplete: true}); });

EXPECT_OUTPUT_CONTAINS(`WARNING: The multiMaster option is deprecated. Please use the multiPrimary option instead.`);

Cluster.disconnect();

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
Cluster = dba.getCluster('devCluster');

//@<> Cluster: addInstance 2
EXPECT_NO_THROWS(function() { Cluster.addInstance(__sandbox_uri2); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Cluster: addInstance 3
EXPECT_NO_THROWS(function() {  Cluster.addInstance(__sandbox_uri3); });

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Cluster: describe cluster with instance
Cluster.describe();

//@<> Cluster: status cluster with instance
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@<> Cluster: removeInstance 2
EXPECT_NO_THROWS(function() { Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2}); });

//@<OUT> Cluster: describe removed member
Cluster.describe();

//@<> Cluster: status removed member
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@<> Cluster: removeInstance 3
EXPECT_NO_THROWS(function() { Cluster.removeInstance(__sandbox_uri3); });

//@<> Cluster: Error cannot remove last instance
EXPECT_THROWS(function() { Cluster.removeInstance(__sandbox_uri1); }, "The instance 'localhost:<<<__mysql_sandbox_port1>>>' is the last member of the cluster");

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance 'localhost:<<<__mysql_sandbox_port1>>>' cannot be removed because it is the only member of the Cluster. Please use <Cluster>.dissolve() instead to remove the last instance and dissolve the Cluster.`);


//@<> Dissolve cluster with success
EXPECT_NO_THROWS(function() { Cluster.dissolve({force: true}); });

Cluster.disconnect();

//@<> Dba: createCluster multiPrimary 2, ok
EXPECT_NO_THROWS(function() { Cluster = dba.createCluster('devCluster', {multiPrimary: true, force: true, memberSslMode: 'REQUIRED', clearReadOnly: true, gtidSetIsComplete: true}); });

Cluster.disconnect();

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
var Cluster = dba.getCluster('devCluster');

//@<> Cluster: addInstance 2 again
EXPECT_NO_THROWS(function() { Cluster.addInstance(__sandbox_uri2); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Cluster: addInstance 3 again
EXPECT_NO_THROWS(function() { Cluster.addInstance(__sandbox_uri3); });

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Cluster: status: success
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

// Rejoin tests

//@<> Dba: stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
testutil.stopSandbox(__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

// start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

//@<> Cluster: rejoinInstance errors
EXPECT_THROWS_TYPE(function() { Cluster.rejoinInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0", "ArgumentError");
EXPECT_THROWS_TYPE(function() { Cluster.rejoinInstance(1,2,3); }, "Invalid number of arguments, expected 1 to 2 but got 3", "ArgumentError");
EXPECT_THROWS_TYPE(function() { Cluster.rejoinInstance(1); }, "Invalid connection options, expected either a URI or a Connection Options Dictionary", "TypeError");
EXPECT_THROWS_TYPE(function() { Cluster.rejoinInstance({host: "localhost", schema: 'abs', "authMethod":56}); }, "Invalid values in connection options: authMethod", "ArgumentError");
EXPECT_THROWS_TYPE(function() { Cluster.rejoinInstance({host: "localhost"}); }, "Could not open connection to 'localhost'", "MySQL Error");
EXPECT_THROWS_TYPE(function() { Cluster.rejoinInstance("localhost:3306"); }, "Could not open connection to 'localhost:3306'", "MySQL Error");

//@<> Dba: rejoin instance 3 ok
EXPECT_NO_THROWS(function() { Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, password:"root"}, {memberSslMode: 'REQUIRED'}); });

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Verify if the cluster is OK

//@<> Cluster: status for rejoin: success
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

Cluster.dissolve({'force': true})

//@<> Cluster: disconnect should work, tho
EXPECT_NO_THROWS(function() { Cluster.disconnect(); });

// Close session
session.close();

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
