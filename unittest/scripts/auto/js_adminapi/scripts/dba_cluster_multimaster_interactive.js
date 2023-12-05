//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.options.useWizards=true;

shell.connect(__sandbox_uri1);

//@<OUT> Dba: createCluster multiPrimary with interaction, cancel
testutil.expectPrompt("Confirm [y/N]", "n");
dba.createCluster('devCluster', {multiPrimary: true});

//@<OUT> Dba: createCluster multiPrimary with interaction, ok
testutil.expectPrompt("Confirm [y/N]", "y");
dba.createCluster('devCluster', {multiPrimary: true});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
var Cluster = dba.getCluster('devCluster');

//@ Dissolve cluster
testutil.expectPrompt("Are you sure you want to dissolve the cluster? [y/N]:", "y");
Cluster.dissolve({force: true});
Cluster.disconnect();

//@<OUT> Dba: createCluster multiPrimary with interaction, regression for BUG#25926603
testutil.expectPrompt("Confirm [y/N]", "y");
dba.createCluster('devCluster', {multiPrimary: true, gtidSetIsComplete: true});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
Cluster = dba.getCluster('devCluster');

//@ Cluster: addInstance with interaction, error
add_instance_options['port'] = __mysql_sandbox_port1;
add_instance_options['user'] = 'root';
Cluster.addInstance(add_instance_options, {});

//@<OUT> Cluster: addInstance with interaction, ok
Cluster.addInstance(__sandbox_uri2);

//@<OUT> Cluster: addInstance 3 with interaction, ok
Cluster.addInstance(__sandbox_uri3);

//@<OUT> Cluster: describe1
Cluster.describe();

//@<> Cluster: status1
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])


//@ Cluster: removeInstance
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2});

//@<OUT> Cluster: describe2
Cluster.describe();

//@<> Cluster: status removed member
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@ Cluster: remove_instance 3
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port3});

//@ Cluster: Error cannot remove last instance
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port1});

//@ Dissolve cluster with success
testutil.expectPrompt("Are you sure you want to dissolve the cluster? [y/N]:", "y");
Cluster.dissolve({force: true});

//@<OUT> Dba: createCluster multiPrimary with interaction 2, ok
testutil.expectPrompt("Confirm [y/N]", "y");
dba.createCluster('devCluster', {multiPrimary: true, gtidSetIsComplete: true});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
Cluster = dba.getCluster('devCluster');

//@<OUT> Cluster: addInstance with interaction, ok 2
Cluster.addInstance(__sandbox_uri2);

//@<OUT> Cluster: addInstance with interaction, ok 3
Cluster.addInstance(__sandbox_uri3);

//@<> Cluster: status: success
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

// Rejoin tests

session.close();
shell.connect(__sandbox_uri3);

//@ Disable group_replication_start_on_boot if version >= 8.0.11 {VER(>=8.0.11)}
session.runSql("SET PERSIST group_replication_start_on_boot=OFF");

//@# Dba: stop instance 3
// Use stop sandbox instance to make sure the instance is gone before restarting it
session.close();
shell.connect(__sandbox_uri1);
testutil.stopSandbox(__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

// start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

//@: Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost", schema: "abs", "authMethod":56});
Cluster.rejoinInstance("localhost:3306");

//@<OUT> Cluster: rejoinInstance with interaction, ok
var session3 = mysql.getSession(__sandbox_uri3);
var server_id = session3.runSql("select @@server_id").fetchOne()[0];
var repl_user = "mysql_innodb_cluster_"+server_id;

Cluster.rejoinInstance({user: "root", host: "localhost", port: __mysql_sandbox_port3});

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

testutil.expectPrompt("Are you sure you want to dissolve the cluster? [y/N]:", "y");
Cluster.dissolve({force: true})

// Disable super-read-only (BUG#26422638)
shell.connect(__sandbox_uri1);
session.runSql("SET GLOBAL SUPER_READ_ONLY = 0;");
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
