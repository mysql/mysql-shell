// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var singleSession = session;

if (__have_ssl)
  var devCluster = dba.createCluster('devCluster', {memberSslMode:'REQUIRED'});
else
  var devCluster = dba.createCluster('devCluster', {memberSslMode:'DISABLED'});

devCluster.disconnect();

//@ Cluster: validating members
var Cluster = dba.getCluster('devCluster');

session.close();
// session is stored on the cluster object so changing the global session should not affect cluster operations
shell.connect({scheme:'mysql', host: "localhost", port: __mysql_sandbox_port2, user: 'root', password: 'root'});
session.close();

// Sets the correct local host
var desc = Cluster.describe();
var localhost = desc.defaultReplicaSet.instances[0].label.split(':')[0];
var hostname = localhost;

var members = dir(Cluster);

print("Cluster Members:", members.length);

validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');
validateMember(members, 'rejoinInstance');
validateMember(members, 'checkInstanceState');
validateMember(members, 'describe');
validateMember(members, 'status');
validateMember(members, 'help');
validateMember(members, 'dissolve');
validateMember(members, 'disconnect');
validateMember(members, 'rescan');
validateMember(members, 'forceQuorumUsingPartitionOf');

//@ Cluster: addInstance errors
Cluster.addInstance();
Cluster.addInstance(5,6,7,1);
Cluster.addInstance(5, 5);
Cluster.addInstance('', 5);
Cluster.addInstance({user:"sample", weird:1},{});
Cluster.addInstance({user:"sample"});
Cluster.addInstance({host: "localhost", schema: 'abs', user:"sample", "auth-method":56, memberSslMode: "foo", ipWhitelist: " "});
Cluster.addInstance({port: __mysql_sandbox_port1});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {memberSslMode: "foo", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {memberSslMode: "", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {ipWhitelist: " ", password: "root"});
add_instance_options['port'] = __mysql_sandbox_port1;
Cluster.addInstance(add_instance_options, add_instance_extra_opts);
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "#invalid", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "invalid#char", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "over256chars_1234567890123456789012345678990123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123", password: "root"});

//@ Cluster: addInstance 2
add_instance_to_cluster(Cluster, __mysql_sandbox_port2, 'second');

// Third instance will be added while the second is still on RECOVERY
//@ Cluster: addInstance 3
add_instance_to_cluster(Cluster, __mysql_sandbox_port3);

wait_slave_state(Cluster, 'second', "ONLINE");
wait_slave_state(Cluster, uri3, "ONLINE");

//@<OUT> Cluster: describe cluster with instance
Cluster.describe();

//@<OUT> Cluster: status cluster with instance
Cluster.status();

//@ Cluster: removeInstance errors
Cluster.removeInstance();
Cluster.removeInstance(1,2,3);
Cluster.removeInstance(1);
Cluster.removeInstance({host: "localhost", schema: 'abs', user:"sample", fakeOption:56});
Cluster.removeInstance("localhost:3306");
Cluster.removeInstance("localhost");
Cluster.removeInstance({host: "localhost"});

//@ Cluster: removeInstance read only
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2});

//@<OUT> Cluster: describe removed read only member
Cluster.describe();

//@<OUT> Cluster: status removed read only member
Cluster.status();

//@ Cluster: addInstance read only back
add_instance_to_cluster(Cluster, __mysql_sandbox_port2);

wait_slave_state(Cluster, uri2, "ONLINE");

//@<OUT> Cluster: describe after adding read only instance back
Cluster.describe();

//@<OUT> Cluster: status after adding read only instance back
Cluster.status();

// Make sure uri2 is selected as the new master
Cluster.removeInstance(uri3);

//@ Cluster: remove_instance master
Cluster.removeInstance(uri1);

//@ Cluster: no operations can be done on a disconnected cluster
Cluster.disconnect();
Cluster.addInstance();
Cluster.checkInstanceState();
Cluster.describe();
Cluster.dissolve();
Cluster.forceQuorumUsingPartitionOf();
Cluster.rejoinInstance();
Cluster.removeInstance();
Cluster.rescan();
Cluster.status();

//@ Connecting to new master
shell.connect({scheme:'mysql', host:localhost, port:__mysql_sandbox_port2, user:'root', password: 'root'});
var Cluster = dba.getCluster();

// Add back uri3
add_instance_to_cluster(Cluster, __mysql_sandbox_port3, 'third_sandbox');

wait_slave_state(Cluster, 'third_sandbox', "ONLINE");

//@<OUT> Cluster: describe on new master
Cluster.describe();

//@<OUT> Cluster: status on new master
Cluster.status();

//@ Cluster: addInstance adding old master as read only
add_instance_to_cluster(Cluster, __mysql_sandbox_port1, 'first_sandbox');

wait_slave_state(Cluster, 'first_sandbox', "ONLINE");

//@<OUT> Cluster: describe on new master with slave
Cluster.describe();

//@<OUT> Cluster: status on new master with slave
Cluster.status();

// Rejoin tests

//@# Dba: kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

// Since the cluster has quorum, the instance will be kicked off the
// Cluster going OFFLINE->UNREACHABLE->(MISSING)
wait_slave_state(Cluster, 'third_sandbox', "(MISSING)");

//@# Dba: start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

//@ Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost", schema: 'abs', "auth-method":56, memberSslMode: "foo", ipWhitelist: " "});
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance("localhost:3306");
Cluster.rejoinInstance("somehost:3306", "root");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, {memberSslMode: "foo", password: "root"});
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, {memberSslMode: "", password: "root"});
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, {ipWhitelist: " ", password: "root"});

//@#: Dba: rejoin instance 3 ok {VER(<8.0.4)}
if (__have_ssl)
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, {memberSslMode: "AUTO", "password": "root"});
else
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, "password":"root"});

wait_slave_state(Cluster, 'third_sandbox', "ONLINE");

//@#: Dba: Wait instance 3 ONLINE {VER(>=8.0.4)}
wait_slave_state(Cluster, 'third_sandbox', "ONLINE");

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status();

//@ Cluster: dissolve errors
Cluster.dissolve();
Cluster.dissolve(1);
Cluster.dissolve(1,2);
Cluster.dissolve("");
Cluster.dissolve({foobar: true});
Cluster.dissolve({force: "whatever"});

//@ Cluster: final dissolve
Cluster.dissolve({force: true});

//@ Cluster: no operations can be done on a dissolved cluster
Cluster.name;
Cluster.addInstance();
Cluster.checkInstanceState();
Cluster.describe();
Cluster.dissolve();
Cluster.forceQuorumUsingPartitionOf();
Cluster.getName();
Cluster.rejoinInstance();
Cluster.removeInstance();
Cluster.rescan();
Cluster.status();

//@ Cluster: disconnect should work, tho
Cluster.disconnect();

session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
