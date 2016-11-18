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
Cluster.addInstance({host: "localhost", port:__mysql_sandbox_port1}, "root");

var uri1 = localhost + ":" + __mysql_sandbox_port1;
var uri2 = localhost + ":" + __mysql_sandbox_port2;
var uri3 = localhost + ":" + __mysql_sandbox_port3;

//@ Cluster: addInstance 2
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, "root");

check_slave_online(Cluster, uri1, uri2);

//@ Cluster: addInstance 3
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, "root");

check_slave_online(Cluster, uri1, uri3);

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
Cluster.addInstance(uri, "root");
check_slave_online(Cluster, uri1, uri2);

//@<OUT> Cluster: describe after adding read only instance back
Cluster.describe()

//@<OUT> Cluster: status after adding read only instance back
Cluster.status()

//@ Cluster: remove_instance master

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

// Rejoin tests

//@# Dba: kill instance 3
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir})
else
  dba.killSandboxInstance(__mysql_sandbox_port3)

// XCOM needs time to kick out the member of the group. The GR team has a patch to fix this
// But won't be available for the GA release. So we need a sleep here
os.sleep(2)

//@# Dba: start instance 3
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir})
else
  dba.startSandboxInstance(__mysql_sandbox_port3)

check_slave_offline(Cluster, uri2, uri3);

//@ Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance({host: "localhost", schema: 'abs', authMethod:56});
Cluster.rejoinInstance("somehost:3306");

//@#: Dba: rejoin instance 3 ok
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, "root");

check_slave_online(Cluster, uri2, uri3);

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status()


// test the lost of quorum

//@# Dba: kill instance 1
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port1, {sandboxDir:__sandbox_dir})
else
  dba.killSandboxInstance(__mysql_sandbox_port1)

//@# Dba: kill instance 3
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir})
else
  dba.killSandboxInstance(__mysql_sandbox_port3)

// GR needs to detect the loss of quorum
os.sleep(5)

//@# Dba: start instance 1
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port1, {sandboxDir: __sandbox_dir})
else
  dba.startSandboxInstance(__mysql_sandbox_port1)

//@# Dba: start instance 3
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir})
else
  dba.startSandboxInstance(__mysql_sandbox_port3)

// Verify the cluster status after loosing 2 members
//@<OUT> Cluster: status for rejoin quorum lost
Cluster.status()

//@#: Dba: rejoin instance 3
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, "root");

//@ Cluster: dissolve: error quorum
Cluster.dissolve()

// In order to be able to run dissolve, we must force the reconfiguration of the group using
// group_replication_force_members
customSession.runSql("SET GLOBAL group_replication_force_members = 'localhost:1" + __mysql_sandbox_port2 + "'");
os.sleep(5)

// Add back the instances to make the dissolve possible

//@ Cluster: rejoinInstance 1
var uri = "root@localhost:" + __mysql_sandbox_port1;
Cluster.rejoinInstance(uri, "root");
check_slave_online(Cluster, uri2, uri1)

//@ Cluster: rejoinInstance 3
var uri = "root@localhost:" + __mysql_sandbox_port3;
Cluster.rejoinInstance(uri, "root");
check_slave_online(Cluster, uri2, uri3)

// Verify the cluster status after rejoining the 2 members
//@<OUT> Cluster: status for rejoin successful
Cluster.status()

//@ Cluster: dissolve errors
Cluster.dissolve()
Cluster.dissolve(1)
Cluster.dissolve(1,2)
Cluster.dissolve("")
Cluster.dissolve({foobar: true})
Cluster.dissolve({force: "whatever"})

// We cannot test the output of dissolve because it will crash the rejoined instance, hitting the bug:
// BUG#24818604: MYSQLD CRASHES WHILE STARTING GROUP REPLICATION FOR A NODE IN RECOVERY PROCESS
// As soon as the bug is fixed, dissolve will work fine and we can remove the above workaround to do a clean-up

//Cluster.dissolve({force: true})
//Cluster.describe()
//Cluster.status()

customSession.close();