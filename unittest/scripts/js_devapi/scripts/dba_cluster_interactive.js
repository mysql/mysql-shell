// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script

var Cluster = dba.getCluster('devCluster');

// Sets the correct local host
var desc = Cluster.describe();
var localhost = desc.defaultReplicaSet.instances[0].name.split(':')[0];


var members = dir(Cluster);

//@ Cluster: validating members
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
Cluster.addInstance( 5)
Cluster.addInstance({host: "localhost", schema: 'abs', user:"sample", authMethod:56});
Cluster.addInstance({port: __mysql_sandbox_port1});

var uri1 = localhost + ":" + __mysql_sandbox_port1;
var uri2 = localhost + ":" + __mysql_sandbox_port2;
var uri3 = localhost + ":" + __mysql_sandbox_port3;

//@ Cluster: addInstance with interaction, error
Cluster.addInstance({host: "localhost", port:__mysql_sandbox_port1});

//@<OUT> Cluster: addInstance with interaction, ok
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2});
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, ssl: false});

check_slave_online(Cluster, uri1, uri2);

//@<OUT> Cluster: addInstance 3 with interaction, ok
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3});
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, ssl: false});

check_slave_online(Cluster, uri1, uri3);

//@<OUT> Cluster: describe1
Cluster.describe()

//@<OUT> Cluster: status1
Cluster.status()

//@ Cluster: removeInstance errors
Cluster.removeInstance();
Cluster.removeInstance(1,2);
Cluster.removeInstance(1);
Cluster.removeInstance({host: "localhost", port:33060});
Cluster.removeInstance({host: "localhost", port:33060, schema: 'abs', user:"sample", authMethod:56});
Cluster.removeInstance("somehost:3306");

//@ Cluster: removeInstance
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2})

//@<OUT> Cluster: describe2
Cluster.describe()

//@<OUT> Cluster: status2
Cluster.status()

//@<OUT> Cluster: dissolve error: not empty
Cluster.dissolve()

//@ Cluster: dissolve errors
Cluster.dissolve(1)
Cluster.dissolve(1,2)
Cluster.dissolve("")
Cluster.dissolve({foobar: true})
Cluster.dissolve({force: 'sample'})

//@ Cluster: remove_instance 3
Cluster.removeInstance({host:localhost, port:__mysql_sandbox_port3})

//@ Cluster: remove_instance last
Cluster.removeInstance({host:localhost, port:__mysql_sandbox_port1})

//@<OUT> Cluster: describe3
Cluster.describe()

//@<OUT> Cluster: status3
Cluster.status()

//@<OUT> Cluster: addInstance with interaction, ok 2
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port1});
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port1, ssl: false});

//@<OUT> Cluster: addInstance with interaction, ok 3
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2});
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, ssl: false});

check_slave_online(Cluster, uri1, uri2);

//@<OUT> Cluster: addInstance with interaction, ok 4
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3});
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, ssl: false});

check_slave_online(Cluster, uri1, uri3);

//@<OUT> Cluster: status: success
Cluster.status()

// Rejoin tests

//@# Dba: kill instance 3
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir})
else
  dba.killSandboxInstance(__mysql_sandbox_port3)

// XCOM needs time to kick out the member of the group. The GR team has a patch to fix this
// But won't be available for the GA release. So we need a sleep here
os.sleep(10)

//@# Dba: start instance 3
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir})
else
  dba.startSandboxInstance(__mysql_sandbox_port3)

check_slave_offline(Cluster, uri1, uri3);

//@: Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance({host: "localhost", schema: "abs", authMethod:56});
Cluster.rejoinInstance("somehost:3306");

//@<OUT> Cluster: rejoinInstance with interaction, ok
if (__have_ssl)
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3});
else
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3, ssl: false});

check_slave_online(Cluster, uri1, uri3);

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status()


// We cannot test the output of dissolve because it will crash the rejoined instance, hitting the bug:
// BUG#24818604: MYSQLD CRASHES WHILE STARTING GROUP REPLICATION FOR A NODE IN RECOVERY PROCESS
// As soon as the bug is fixed, dissolve will work fine and we can remove the above workaround to do a clean-up

//Cluster.dissolve({force: true})

//Cluster.describe()

//Cluster.status()
