// Assumptions: smart deployment rountines available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

// by default, root account can connect only via localhost, create 'root'@'%'
// so it's possible to connect via hostname
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});

session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'%' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'%' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'%' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'%' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

//@ it's not possible to adopt from GR without existing group replication
dba.createCluster('testCluster', {adoptFromGR: true});

// create cluster with two instances and drop metadata schema

//@ Create cluster
if (__have_ssl)
  var cluster = dba.createCluster('testCluster', {memberSslMode: 'REQUIRED'});
else
  var cluster = dba.createCluster('testCluster', {memberSslMode: 'DISABLED'});

//@ Adding instance to cluster
add_instance_to_cluster(cluster, __mysql_sandbox_port2);
wait_slave_state(cluster, uri2, "ONLINE");

// To simulate an existing unmanaged replication group we simply drop the
// metadata schema

//@ Drop Metadata
dba.dropMetadataSchema({force: true})

//@ Check cluster status after drop metadata schema
cluster.status();

session.close();
cluster.disconnect();

// Establish a session using the real hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the real hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: real_hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Create cluster adopting from GR
var cluster = dba.createCluster('testCluster', {adoptFromGR: true});

//@<OUT> Check cluster status
cluster.status();

//@ dissolve the cluster
cluster.dissolve({force: true});

//@ it's not possible to adopt from GR when cluster was dissolved
dba.createCluster('testCluster', {adoptFromGR: true});

// Close session
session.close();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
