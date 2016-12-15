// Assumptions: smart deployment rountines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

//@<OUT> Standalone Instance : check instance config
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
dba.checkInstanceConfig({host: localhost, port: __mysql_sandbox_port1, password:'root'});

//@<OUT> Standalone Instance : config local instance
dba.configLocalInstance({host: localhost, port: __mysql_sandbox_port1, password:'root'}, {mycnfPath:'mybad.cnf'});

//@<OUT> Standalone Instance: create cluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSsl: true});
else
  var cluster = dba.createCluster('dev');

cluster.status();
session.close();

//@ Standalone Instance: Failed preconditions
shell.connect({host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});

dba.getCluster();
dba.dropMetadataSchema({force: true});
cluster.addInstance({host: localhost, port: __mysql_sandbox_port2});
cluster.rejoinInstance({host: localhost, port: __mysql_sandbox_port2});
cluster.removeInstance({host: localhost, port: __mysql_sandbox_port2});
cluster.describe();
cluster.status();
cluster.dissolve();
cluster.checkInstanceState({host: localhost, port: __mysql_sandbox_port2, password: 'root'});
cluster.rescan();
session.close();

//@ Read Only Instance : get cluster
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

add_instance_to_cluster(cluster, __mysql_sandbox_port2);

// Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

session.close();

shell.connect({host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
var cluster = dba.getCluster()

//@<OUT> Read Only Instance : check instance config
dba.checkInstanceConfig({host: localhost, port: __mysql_sandbox_port3, password:'root'});

//@<OUT> Read Only Instance : config local instance
dba.configLocalInstance({host: localhost, port: __mysql_sandbox_port3, password:'root'}, {mycnfPath:'mybad.cnf'});

//@<OUT> Read Only Instance : check instance state
cluster.checkInstanceState({host: localhost, port: __mysql_sandbox_port3, password: 'root'});

//@ Read Only Instance : rejoin instance
cluster.rejoinInstance({host: localhost, port: __mysql_sandbox_port3, password:'root'});

//@<OUT> Read Only Instance : describe
cluster.describe();

//@<OUT> Read Only Instance : status
cluster.status();

//@ Read Only: Failed preconditions
dba.createCluster('sample');
dba.dropMetadataSchema({force: true});
cluster.addInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.removeInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.dissolve();
cluster.rescan();
session.close();

//@ Preparation for quorumless cluster tests
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster();

if (__sandbox_dir)
  dba.killSandboxInstance(__mysql_sandbox_port2, {sandboxDir:__sandbox_dir});
else
  dba.killSandboxInstance(__mysql_sandbox_port2);

// Waiting for the second instance to become offline
wait_slave_state(cluster, uri2, ["UNREACHABLE", "OFFLINE"]);

//@ Quorumless Cluster: Failed preconditions
dba.createCluster('failed');
dba.dropMetadataSchema({force: true});
cluster.addInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.rejoinInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.removeInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.dissolve();
cluster.checkInstanceState({host: localhost, port: __mysql_sandbox_port3, password: 'root'});
cluster.rescan();

//@ Quorumless Cluster: get cluster
var cluster = dba.getCluster();

//@<OUT> Quorumless Cluster : check instance config
dba.checkInstanceConfig({host: localhost, port: __mysql_sandbox_port3, password:'root'});

//@<OUT> Quorumless Cluster : config local instance
dba.configLocalInstance({host: localhost, port: __mysql_sandbox_port3, password:'root'}, {mycnfPath:'mybad.cnf'});

//@<OUT> Quorumless Cluster : describe
cluster.describe();

//@<OUT> Quorumless Cluster : status
cluster.status();
session.close();

//@ Preparation for unmanaged instance tests
reset_or_deploy_sandbox(__mysql_sandbox_port1);
reset_or_deploy_sandbox(__mysql_sandbox_port2);

shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

if (__have_ssl)
  var cluster = dba.createCluster('temporal', {memberSsl: true});
else
  var cluster = dba.createCluster('temporal');

dba.dropMetadataSchema({force:true});

//@ Unmanaged Instance: Failed preconditions
dba.createCluster('failed');
dba.getCluster();
dba.dropMetadataSchema({force: true});
cluster.addInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.rejoinInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.removeInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.describe();
cluster.status();
cluster.dissolve();
cluster.checkInstanceState({host: localhost, port: __mysql_sandbox_port3, password: 'root'});
cluster.rescan();

//@<OUT> Unmanaged Instance: create cluster
// TODO: Uncomment this test case once the hostname resolution is fixed for adoptFromGR

//var cluster = dba.createCluster('fromGR', {adoptFromGR:true});
//cluster.status();
session.close();


//@ XSession: Failed preconditions
shell.connect({host: localhost, port: __mysql_sandbox_port1*10, user: 'root', password: 'root'});
dba.createCluster('failed');
dba.getCluster();
dba.dropMetadataSchema({force: true});
cluster.addInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.rejoinInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.removeInstance({host: localhost, port: __mysql_sandbox_port3});
cluster.describe();
cluster.status();
cluster.dissolve();
cluster.checkInstanceState({host: localhost, port: __mysql_sandbox_port3, password: 'root'});
cluster.rescan();
session.close();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here)
  cleanup_sandboxes(true);
else
  reset_or_deploy_sandboxes();
