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

//@ Cluster: addInstance errors
Cluster.addInstance()
Cluster.addInstance(5,6,7,1)
Cluster.addInstance(5, 5)
Cluster.addInstance('', 5)
Cluster.addInstance( 5)
Cluster.addInstance({host: "localhost", schema: 'abs', user:"sample", authMethod:56});
Cluster.addInstance({port: __mysql_sandbox_port1});

//@ Cluster: addInstance with interaction, error
Cluster.addInstance({host: "localhost", port:__mysql_sandbox_port1});

//@<OUT> Cluster: addInstance with interaction, ok
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2});

//@<OUT> Cluster: describe1
Cluster.describe()

//@<OUT> Cluster: status1
Cluster.status()

//@ Cluster: removeInstance errors
Cluster.removeInstance();
Cluster.removeInstance(1,2);
Cluster.removeInstance(1);
Cluster.removeInstance({host: "localhost"});
Cluster.removeInstance({host: "localhost", schema: 'abs', user:"sample", authMethod:56});
Cluster.removeInstance("somehost:3306");

//@ Cluster: removeInstance
Cluster.removeInstance({host:'localhost', port:__mysql_sandbox_port2})

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

//@ Cluster: remove_instance last
Cluster.removeInstance({host:'localhost', port:__mysql_sandbox_port1})

//@<OUT> Cluster: describe3
Cluster.describe()

//@<OUT> Cluster: status3
Cluster.status()

//@<OUT> Cluster: addInstance with interaction, ok 2
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port1});

//@<OUT> Cluster: addInstance with interaction, ok 3
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2});

//@<OUT> Cluster: dissolve
Cluster.dissolve({force: true})

//@ Cluster: describe: dissolved cluster
Cluster.describe()

//@ Cluster: status: dissolved cluster
Cluster.status()
