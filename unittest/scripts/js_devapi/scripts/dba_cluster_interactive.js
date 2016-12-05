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
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: false, memberSslCa: "ca"}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: false, memberSslCert: "cert"}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: false, memberSslKey: "key"}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: true, memberSslCa: ""}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: true, memberSslCert: ""}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: true, memberSslKey: ""}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: true, memberSslCa: " "}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: true, memberSslCert: " "}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: true, memberSslKey: " "}, "root");

var uri1 = localhost + ":" + __mysql_sandbox_port1;
var uri2 = localhost + ":" + __mysql_sandbox_port2;
var uri3 = localhost + ":" + __mysql_sandbox_port3;

//@ Cluster: addInstance with interaction, error
Cluster.addInstance({host: "localhost", port:__mysql_sandbox_port1});

//@<OUT> Cluster: addInstance with interaction, ok
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2});
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: false});

wait_slave_state(Cluster, uri2, "ONLINE");

//@<OUT> Cluster: addInstance 3 with interaction, ok
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3});
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: false});

wait_slave_state(Cluster, uri3, "ONLINE");

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
Cluster.dissolve({enforce: true})
Cluster.dissolve({force: 'sample'})

//@ Cluster: remove_instance 3
Cluster.removeInstance({host:localhost, port:__mysql_sandbox_port3})

//@<OUT> Cluster: addInstance with interaction, ok 3
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2});
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2, memberSsl: false});

wait_slave_state(Cluster, uri2, "ONLINE");

//@<OUT> Cluster: addInstance with interaction, ok 4
if (__have_ssl)
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3});
else
  Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: false});

wait_slave_state(Cluster, uri3, "ONLINE");

//@<OUT> Cluster: status: success
Cluster.status()

// Rejoin tests

//@# Dba: kill instance 3
if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir});
else
  dba.killSandboxInstance(__mysql_sandbox_port3);

wait_slave_state(Cluster, uri3, ["UNREACHABLE", "OFFLINE"]);

//@# Dba: start instance 3
if (__sandbox_dir)
  dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir})
else
  dba.startSandboxInstance(__mysql_sandbox_port3)

wait_slave_state(Cluster, uri3, "OFFLINE");

//@: Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance({host: "localhost", schema: "abs", authMethod:56});
Cluster.rejoinInstance("somehost:3306");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: false, memberSslCa: "ca"}, "root");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: false, memberSslCert: "cert"}, "root");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: false, memberSslKey: "key"}, "root");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: true, memberSslCa: ""}, "root");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: true, memberSslCert: ""}, "root");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: true, memberSslKey: ""}, "root");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: true, memberSslCa: " "}, "root");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: true, memberSslCert: " "}, "root");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3, memberSsl: true, memberSslKey: " "}, "root");

//@<OUT> Cluster: rejoinInstance with interaction, ok
if (__have_ssl)
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3});
else
  Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3, memberSsl: false});

wait_slave_state(Cluster, uri3, "ONLINE");

// Verify if the cluster is OK

//@<OUT> Cluster: status for rejoin: success
Cluster.status()

//Cluster.dissolve({force: true})

reset_or_deploy_sandboxes();
