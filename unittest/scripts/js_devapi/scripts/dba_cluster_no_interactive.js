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

//@ Cluster: addInstance errors
Cluster.addInstance()
Cluster.addInstance(5,6,7,1)
Cluster.addInstance(5, 5)
Cluster.addInstance('', 5)
Cluster.addInstance({user:"sample", weird:1},5)
Cluster.addInstance({user:"sample"})
Cluster.addInstance({host: "localhost", schema: 'abs', user:"sample", authMethod:56});
Cluster.addInstance({port: __mysql_sandbox_port1});
if (__have_ssl)
  Cluster.addInstance({host: "localhost", port:__mysql_sandbox_port1}, "root");
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port1, ssl: false}, "root");

var uri1 = localhost + ":" + __mysql_sandbox_port1;
var uri2 = localhost + ":" + __mysql_sandbox_port2;
var uri3 = localhost + ":" + __mysql_sandbox_port3;

//@ Cluster: addInstance 2
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, "root");
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, ssl: false}, "root");

wait_slave_state(Cluster, uri1, uri2, "ONLINE");

//@ Cluster: addInstance 3
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, "root");
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, ssl: false}, "root");

wait_slave_state(Cluster, uri1, uri3, "ONLINE");

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
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2})

//@<OUT> Cluster: describe removed read only member
Cluster.describe()

//@<OUT> Cluster: status removed read only member
Cluster.status()

//@ Cluster: addInstance read only back
var uri = "root@localhost:" + __mysql_sandbox_port2;
if (__have_ssl)
  Cluster.addInstance(uri, "root");
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, ssl: false}, "root");

wait_slave_state(Cluster, uri1, uri2, "ONLINE");

//@<OUT> Cluster: describe after adding read only instance back
Cluster.describe()

//@<OUT> Cluster: status after adding read only instance back
Cluster.status()

// Make sure uri2 is selected as the new master
Cluster.removeInstance(uri3)

//@ Cluster: remove_instance master
Cluster.removeInstance(uri1)

//@ Connecting to new master
var mysql = require('mysql');
var customSession = mysql.getClassicSession({host:localhost, port:__mysql_sandbox_port2, user:'root', password: 'root'});
dba.resetSession(customSession);
var Cluster = dba.getCluster();

// Add back uri3
var uri = "root@localhost:" + __mysql_sandbox_port3;
if (__have_ssl)
  Cluster.addInstance(uri, "root");
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, ssl: false}, "root");

wait_slave_state(Cluster, uri2, uri3, "ONLINE");

//@<OUT> Cluster: describe on new master
Cluster.describe()

//@<OUT> Cluster: status on new master
Cluster.status()

//@ Cluster: addInstance adding old master as read only
var uri = "root@localhost:" + __mysql_sandbox_port1;
if (__have_ssl)
  Cluster.addInstance(uri, "root");
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port1, ssl: false}, "root");

wait_slave_state(Cluster, uri2, uri1, "ONLINE");

//@<OUT> Cluster: describe on new master with slave
Cluster.describe()

//@<OUT> Cluster: status on new master with slave
Cluster.status()

// Rejoin tests

//@# Dba: kill instance 3
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir})
else
  dba.killSandboxInstance(__mysql_sandbox_port3)

  wait_slave_state(Cluster, uri2, uri3, ["UNREACHABLE", "OFFLINE"]);

//@# Dba: start instance 3
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir})
else
  dba.startSandboxInstance(__mysql_sandbox_port3)

//@ Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance({host: "localhost", schema: 'abs', authMethod:56});
Cluster.rejoinInstance("somehost:3306");

//@#: Dba: rejoin instance 3 ok
if (__have_ssl)
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, "root");
else
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, ssl: false}, "root");

wait_slave_state(Cluster, uri2, uri3, "ONLINE");

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status()

//@ Cluster: dissolve errors
Cluster.dissolve()
Cluster.dissolve(1)
Cluster.dissolve(1,2)
Cluster.dissolve("")
Cluster.dissolve({foobar: true})
Cluster.dissolve({force: "whatever"})

//Cluster.dissolve({force: true})

customSession.close();

reset_or_deploy_sandboxes();