// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script

//@ Cluster: validating members
var Cluster = dba.getCluster('devCluster');

// Sets the correct local host
var desc = Cluster.describe();
var localhost = desc.defaultReplicaSet.instances[0].name.split(':')[0];
var hostname = localhost;

var members = dir(Cluster);

print("Cluster Members:", members.length);

validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'adminType');
validateMember(members, 'getAdminType');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');
validateMember(members, 'rejoinInstance');
validateMember(members, 'checkInstanceState');
validateMember(members, 'describe');
validateMember(members, 'status');
validateMember(members, 'help');
validateMember(members, 'dissolve');
validateMember(members, 'rescan');

//@# Cluster: addInstance errors
Cluster.addInstance()
Cluster.addInstance(5,6,7,1)
Cluster.addInstance(5, 5)
Cluster.addInstance('', 5)
Cluster.addInstance({user:"sample", weird:1},5)
Cluster.addInstance({user:"sample"})
Cluster.addInstance({host: "localhost", schema: 'abs', user:"sample", authMethod:56});
Cluster.addInstance({port: __mysql_sandbox_port1});
Cluster.addInstance({host: "localhost", port:__mysql_sandbox_port1}, "root");

//@ Cluster: addInstance
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, "root");

//@<OUT> Cluster: describe cluster with instance
Cluster.describe()

//@<OUT> Cluster: status cluster with instance
Cluster.status()

//@ Cluster: removeInstance errors
Cluster.removeInstance();
Cluster.removeInstance(1,2);
Cluster.removeInstance(1);
Cluster.removeInstance({host: "localhost"});
Cluster.removeInstance({host: "localhost", schema: 'abs', user:"sample", authMethod:56});
Cluster.removeInstance("somehost:3306");

//@ Cluster: removeInstance read only
Cluster.removeInstance({host:localhost, port:__mysql_sandbox_port2})

//@<OUT> Cluster: describe removed read only member
Cluster.describe()

//@<OUT> Cluster: status removed read only member
Cluster.status()

//@ Cluster: addInstance read only back
var uri = "root@localhost:" + __mysql_sandbox_port2;
Cluster.addInstance(uri, "root");

//@<OUT> Cluster: describe after adding read only instance back
Cluster.describe()

//@<OUT> Cluster: status after adding read only instance back
Cluster.status()

//@ Cluster: remove_instance master
var uri1 = localhost + ":" + __mysql_sandbox_port1;
var uri2 = localhost + ":" + __mysql_sandbox_port2;
check_slave_online(Cluster, uri1, uri2);

Cluster.removeInstance(uri1)

//@ Connecting to new master
var mysql = require('mysql');
var customSession = mysql.getClassicSession({host:localhost, port:__mysql_sandbox_port2, user:'root', password: 'root'});
dba.resetSession(customSession);
var Cluster = dba.getCluster();

//@<OUT> Cluster: describe on new master
Cluster.describe()

//@<OUT> Cluster: status on new master
Cluster.status()

//@ Cluster: addInstance adding old master as read only
var uri = "root@localhost:" + __mysql_sandbox_port1;
Cluster.addInstance(uri, "root");


//@<OUT> Cluster: describe on new master with slave
Cluster.describe()

//@<OUT> Cluster: status on new master with slave
Cluster.status()

//@ Cluster: dissolve errors
Cluster.dissolve()
Cluster.dissolve(1)
Cluster.dissolve(1,2)
Cluster.dissolve("")
Cluster.dissolve({foobar: true})
Cluster.dissolve({force: "whatever"})

//@ Cluster: dissolve
check_slave_online(Cluster, uri2, uri1);
Cluster.dissolve({force: true})

//@ Cluster: describe: dissolved cluster
Cluster.describe()

//@ Cluster: status: dissolved cluster
Cluster.status()
customSession.close();