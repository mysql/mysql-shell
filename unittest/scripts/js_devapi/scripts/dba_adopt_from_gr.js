// Assumptions: smart deployment rountines available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
// Create account with all privileges but remove one of the necessary privileges
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'%' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'%' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
// Create account with all privileges but remove one of the necessary privileges
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'%' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'%' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");


shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

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

// Close session
session.close();
cluster.disconnect();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
