// Assumptions: smart deployment routines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect(__sandbox_uri2);

// Create root@<hostname> account with all privileges, required to create a
// cluster.
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'"+real_hostname+"' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'"+real_hostname+"' WITH GRANT OPTION");
session.runSql("CREATE USER 'root'@'"+hostname_ip+"' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'"+hostname_ip+"' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

shell.connect(__sandbox_uri1);

// Create root@<hostname> account with all privileges, required to create a
// cluster.
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'root'@'"+real_hostname+"' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'"+real_hostname+"' WITH GRANT OPTION");
session.runSql("CREATE USER 'root'@'"+hostname_ip+"' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'root'@'"+hostname_ip+"' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");

//@ it's not possible to adopt from GR without existing group replication
dba.createCluster('testCluster', {adoptFromGR: true});

//@ Create cluster
var cluster = dba.createCluster('testCluster', {memberSslMode: 'DISABLED'});

//@ Adding instance to cluster
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

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
shell.connect({scheme:'mysql', host: real_hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

cluster.disconnect();

//@<OUT> Create cluster adopting from GR - answer 'yes' to prompt
var cluster = dba.createCluster('testCluster');
//@<OUT> Check cluster status - success
cluster.status();

// To simulate an existing unmanaged replication group we simply drop the
// metadata schema
dba.dropMetadataSchema()

// Close session
session.close();
cluster.disconnect();

// Establish a session using the hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: real_hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Create cluster adopting from GR - answer 'no' to prompt
var cluster = dba.createCluster('testCluster');
//@ Check cluster status - failure
cluster.status();

// Close session
session.close();
cluster.disconnect();

// Establish a session using the hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: real_hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> Create cluster adopting from GR - use 'adoptFromGR' option
var cluster = dba.createCluster('testCluster', {adoptFromGR: true});
//@<OUT> Check cluster status - success - 'adoptFromGR'
cluster.status();

// Dismantle the cluster
cluster.dissolve({force: true})

// Close session
session.close();
cluster.disconnect();

// create cluster in multi-master mode
shell.connect(__sandbox_uri1);

var cluster = dba.createCluster('testCluster', {multiMaster: true, memberSslMode: __ssl_mode, clearReadOnly: true, force: true});

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// To simulate an existing unmanaged replication group we simply drop the
// metadata schema
dba.dropMetadataSchema()

// Close session
session.close();
cluster.disconnect();

// Establish a session using the hostname
// because when adopting from GR, the information in the
// performance_schema.replication_group_members will have the hostname
// and not 'localhost'
shell.connect({scheme:'mysql', host: real_hostname, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> Create cluster adopting from multi-master GR - use 'adoptFromGR' option
var cluster = dba.createCluster('testCluster', {adoptFromGR: true, force: true});
//@<OUT> Check cluster status - success - adopt from multi-master
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
