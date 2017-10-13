// Assumptions: smart deployment routines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

// Create root@<hostname> account with all privileges, required to create a
// cluster.
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'" + hostname + "' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'" + hostname +
    "'WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

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

//@<OUT> Drop Metadata
dba.dropMetadataSchema()

//@ Check cluster status after drop metadata schema
cluster.status();

session.close();

// Establish a session using the hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the hostname
// and not 'localhost'
shell.connect({host: hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> Create cluster adopting from GR
var cluster = dba.createCluster('testCluster');

//@<OUT> Check cluster status
cluster.status();

// Close session
session.close();

//@ Finalization
if (deployed_here)
  cleanup_sandboxes(true);
