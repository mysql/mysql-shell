// Assumptions: smart deployment rountines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

//@ create first cluster
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

if (__have_ssl)
  var single = dba.createCluster('single', {memberSslMode:'REQUIRED'});
else
  var single = dba.createCluster('single');

//@ Success adding instance
add_instance_to_cluster(single, __mysql_sandbox_port2);

// Waiting for the second added instance to become online
wait_slave_state(single, uri2, "ONLINE");

session.close();

//@ create second cluster
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});

if (__have_ssl)
  var multi = dba.createCluster('multi', {memberSslMode:'REQUIRED', multiMaster:true, force:true});
else
  var multi = dba.createCluster('multi', {multiMaster:true, force:true});

session.close();

//@ Failure adding instance from multi cluster into single
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster();
add_instance_options['port'] = __mysql_sandbox_port3;
cluster.addInstance(add_instance_options);
session.close();

// Drops the metadata on the multi cluster letting a non managed replication group
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
dba.dropMetadataSchema({force:true});
session.close()

//@ Failure adding instance from an unmanaged replication group into single
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var cluster = dba.getCluster();
add_instance_options['port'] = __mysql_sandbox_port3;
cluster.addInstance(add_instance_options);

//@ Failre adding instance already in the InnoDB cluster
add_instance_options['port'] = __mysql_sandbox_port2;
cluster.addInstance(add_instance_options);

session.close();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here)
  cleanup_sandboxes(true);