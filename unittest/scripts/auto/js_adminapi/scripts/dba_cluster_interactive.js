//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

// Create a test dataset so that RECOVERY takes a while and we ensure right monitoring messages for addInstance
session.runSql("create schema test");
session.runSql("create table test.data (a int primary key auto_increment, data longtext)");
for (i = 0; i < 20; i++) {
    session.runSql("insert into test.data values (default, repeat('x', 4*1024*1024))");
}

var cluster = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED', gtidSetIsComplete: true});

var Cluster = dba.getCluster('devCluster');

// Sets the correct local host
var desc = Cluster.describe();
var localhost = desc.defaultReplicaSet.topology[0].label.split(':')[0];


//@<> Cluster: validating members
validateMembers(Cluster, [
  'name',
  'getClusterSet',
  'getName',
  'addInstance',
  'removeInstance',
  'rejoinInstance',
  'describe',
  'status',
  'help',
  'dissolve',
  'disconnect',
  'rescan',
  'listRouters',
  'removeRouterMetadata',
  'resetRecoveryAccountsPassword',
  'forceQuorumUsingPartitionOf',
  'switchToSinglePrimaryMode',
  'switchToMultiPrimaryMode',
  'setPrimaryInstance',
  'options',
  'setOption',
  'setInstanceOption',
  'setupAdminAccount',
  'setupRouterAccount',
  'createClusterSet',
  'fenceAllTraffic',
  'fenceWrites',
  'unfenceWrites',
  'addReplicaInstance',
  'routingOptions',
  'setRoutingOption',
  'routerOptions'
])

//@<> Cluster: addInstance errors
EXPECT_THROWS(function(){
    Cluster.addInstance();
}, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function(){
    Cluster.addInstance(5,6,7,1);
}, "Invalid number of arguments, expected 1 to 2 but got 4");
EXPECT_THROWS(function(){
    Cluster.addInstance(5, 5);
}, "Argument #2 is expected to be a map");
EXPECT_THROWS(function(){
    Cluster.addInstance('', 5);
}, "Argument #2 is expected to be a map");
EXPECT_THROWS(function(){
    Cluster.addInstance(5);
}, "Invalid connection options, expected either a URI or a Connection Options Dictionary");
EXPECT_THROWS(function(){
    Cluster.addInstance('');
}, "Invalid URI: empty.");
EXPECT_THROWS(function(){
    Cluster.addInstance({host: "localhost", schema: 'abs', user:"sample", "auth-method":56, memberSslMode: "foo", ipWhitelist: " "});
}, "Invalid values in connection options: ipWhitelist, memberSslMode");
EXPECT_THROWS(function(){
    Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, "root");
}, "Argument #2 is expected to be a map");
EXPECT_THROWS(function(){
    Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: ""});
}, "The label can not be empty.");
EXPECT_THROWS(function(){
    Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "#invalid"});
}, "The label can only start with an alphanumeric or the '_' character.");
EXPECT_THROWS(function(){
    Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "invalid#char"});
}, "The label can only contain alphanumerics or the '_', '.', '-', ':' characters. Invalid character '#' found.");
EXPECT_THROWS(function(){
    Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "over256chars_1234567890123456789012345678990123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123"});
}, "The label can not be greater than 256 characters.");

//@ Cluster: addInstance with interaction, error
add_instance_options['port'] = __mysql_sandbox_port1;
add_instance_options['user'] = 'root';
Cluster.addInstance(add_instance_options);

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
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])


//@<> Cluster: removeInstance errors
testutil.expectPrompt("*", "no");
testutil.expectPrompt("*", "yes");

EXPECT_THROWS(function(){
    Cluster.removeInstance();
}, "Cluster.removeInstance: Invalid number of arguments, expected 1 to 2 but got 0", "ArgumentError");
EXPECT_THROWS(function(){
    Cluster.removeInstance(1,2,3);
}, "Cluster.removeInstance: Invalid number of arguments, expected 1 to 2 but got 3", "ArgumentError");
EXPECT_THROWS(function(){
    Cluster.removeInstance(1);
}, "Cluster.removeInstance: Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary", "TypeError");
EXPECT_THROWS(function(){
    Cluster.removeInstance({host: "localhost", port:33060, schema: 'abs', user:"sample", "auth-method":56});
}, "Cluster.removeInstance: Argument #1: Argument auth-method is expected to be a string", "TypeError");

// try to remove instance that is not in the cluster using the classic port
EXPECT_THROWS(function(){
    Cluster.removeInstance({user: __user, host: __host, port: __mysql_port, password: shell.parseUri(__uripwd).password});
}, `Cluster.removeInstance: Metadata for instance '${__host}:${__mysql_port}' not found`);

//@<> Cluster: removeInstance
EXPECT_NO_THROWS(function(){ Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2}); });

//@<OUT> Cluster: describe2
Cluster.describe();

//@<> Cluster: status2
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@<OUT> Cluster: dissolve error: not empty
// WL11889 FR2_01: prompt to confirm dissolve in interactive mode.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
Cluster.dissolve();

//@<> Cluster: dissolve errors
EXPECT_THROWS(function(){
    Cluster.dissolve(1);
}, "Argument #1 is expected to be a map");
EXPECT_THROWS(function(){
    Cluster.dissolve(1,2);
}, "Invalid number of arguments, expected 0 to 1 but got 2");
EXPECT_THROWS(function(){
    Cluster.dissolve("");
}, "Argument #1 is expected to be a map");
EXPECT_THROWS(function(){
    Cluster.dissolve({enforce: true});
}, "Invalid options: enforce");
EXPECT_THROWS(function(){
    Cluster.dissolve({force: 'sample'});
}, "Option 'force' Bool expected, but value is String");

//@<> Cluster: remove_instance 3
EXPECT_NO_THROWS(function(){ Cluster.removeInstance({host:localhost, port:__mysql_sandbox_port3}); });

//@<OUT> Cluster: addInstance with interaction, ok 3
Cluster.addInstance(__sandbox_uri2, {'label': 'z2nd_sandbox'});

//@<OUT> Cluster: addInstance with interaction, ok 4
Cluster.addInstance(__sandbox_uri3, {'label': 'z3rd_sandbox'});

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Cluster: status: success
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"]["z2nd_sandbox"]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"]["z3rd_sandbox"]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"]["z2nd_sandbox"]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"]["z3rd_sandbox"]["mode"])

// Rejoin tests

//@# Dba: kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@# Dba: start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

//@<> Cluster: rejoinInstance errors
EXPECT_THROWS(function(){
    Cluster.rejoinInstance();
}, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function(){
    Cluster.rejoinInstance(1,2,3);
}, "Invalid number of arguments, expected 1 to 2 but got 3");
EXPECT_THROWS(function(){
    Cluster.rejoinInstance(1);
}, "Invalid connection options, expected either a URI or a Connection Options Dictionary");
EXPECT_THROWS(function(){
    Cluster.rejoinInstance({host: "localhost", schema: "abs", "auth-method":56, memberSslMode: "foo", ipWhitelist: " "});
}, "Invalid values in connection options: ipWhitelist, memberSslMode");
EXPECT_THROWS(function(){
    Cluster.rejoinInstance("somehost:3306", "root");
}, "Argument #2 is expected to be a map");
EXPECT_THROWS(function(){
    Cluster.rejoinInstance({host: "localhost"});
}, "Could not open connection to 'localhost'");
EXPECT_THROWS(function(){
    Cluster.rejoinInstance("localhost:3306");
}, "Could not open connection to 'localhost:3306'");

//@<OUT> Cluster: rejoinInstance with interaction, ok
var session3 = mysql.getSession(__sandbox_uri3);
var server_id = session3.runSql("select @@server_id").fetchOne()[0];
var repl_user = "mysql_innodb_cluster_"+server_id;
Cluster.rejoinInstance({user: "root", host: "localhost", port: __mysql_sandbox_port3, password: 'root'});

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Verify if the cluster is OK

//@<> Cluster: status for rejoin: success
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"]["z2nd_sandbox"]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"]["z3rd_sandbox"]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"]["z2nd_sandbox"]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"]["z3rd_sandbox"]["mode"])

//@<OUT> Cluster: final dissolve
// WL11889 FR2_01: prompt to confirm dissolve in interactive mode.
// WL11889 FR2_02: force option no longer required.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
Cluster.dissolve();

//@<> Cluster: no operations can be done on an offline cluster
EXPECT_EQ("devCluster", Cluster.name);
EXPECT_EQ("devCluster", Cluster.getName());

EXPECT_THROWS(function(){ Cluster.addInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function(){ Cluster.describe(); }, "Can't call function 'describe' on an offline Cluster");
EXPECT_THROWS(function(){ Cluster.dissolve(); }, "Can't call function 'dissolve' on an offline Cluster");
EXPECT_THROWS(function(){ Cluster.forceQuorumUsingPartitionOf(); }, "Invalid number of arguments, expected 1 but got 0");
EXPECT_THROWS(function(){ Cluster.rejoinInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function(){ Cluster.removeInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function(){ Cluster.rescan(); }, "Can't call function 'rescan' on an offline Cluster");
EXPECT_THROWS(function(){ Cluster.status(); }, "Can't call function 'status' on an offline Cluster");
EXPECT_THROWS(function(){ Cluster.listRouters(); }, "Can't call function 'listRouters' on an offline Cluster");
EXPECT_THROWS(function(){ Cluster.removeRouterInstance(); }, "The cluster object is disconnected. Please use dba.getCluster() to obtain a fresh cluster handle");

//@<> Cluster: disconnect() is ok on an offline cluster
EXPECT_NO_THROWS(function(){ Cluster.disconnect(); });

//<> Close session
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
