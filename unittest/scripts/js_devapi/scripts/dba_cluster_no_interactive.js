// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
var ClusterPassword = 'testing';

//@ Cluster: validating members
var Cluster = dba.getCluster('devCluster', {masterKey:ClusterPassword});

var members = dir(Cluster);

print("Cluster Members:", members.length);

validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'adminType');
validateMember(members, 'getAdminType');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');
validateMember(members, 'rejoinInstance');
validateMember(members, 'describe');
validateMember(members, 'status');
validateMember(members, 'help');

//@# Cluster: addInstance errors
Cluster.addInstance()
Cluster.addInstance(5,6,7,1)
Cluster.addInstance(5, 5)
Cluster.addInstance('', 5)
Cluster.addInstance( 5)
Cluster.addInstance({host: "localhost", schema: 'abs', user:"sample", authMethod:56});
Cluster.addInstance({port: __mysql_sandbox_port1});
Cluster.addInstance({host: "localhost", port:__mysql_sandbox_port1}, "root");

//@ Cluster: addInstance
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, "root");

//@<OUT> Cluster: describe1
Cluster.describe()

//@<OUT> Cluster: status1
Cluster.status()

//@ Cluster: removeInstance errors
Cluster.removeInstance();
Cluster.removeInstance(1,2);
Cluster.removeInstance(1);
Cluster.removeInstance({host: "localhost");
Cluster.removeInstance({host: "localhost", schema: 'abs', user:"sample", authMethod:56});
Cluster.removeInstance("somehost:3306");


//@ Cluster: removeInstance
Cluster.removeInstance({host:'localhost', port:__mysql_sandbox_port2})

//@<OUT> Cluster: describe2
Cluster.describe()

//@<OUT> Cluster: status2
Cluster.status()

//@ Cluster: remove_instance last
Cluster.removeInstance({host:'localhost', port:__mysql_sandbox_port1})

//@<OUT> Cluster: describe3
Cluster.describe()

//@<OUT> Cluster: status3
Cluster.status()
