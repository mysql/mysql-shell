// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

// Create a new administrative account on instance 1
shell.connect({scheme:'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', password: 'root'});
session.runSql('SET sql_log_bin=0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('CREATE USER \'foo\'@\'%\' IDENTIFIED BY \'bar\'');
session.runSql('GRANT ALL PRIVILEGES ON *.* TO \'foo\'@\'%\' WITH GRANT OPTION');
session.runSql('SET sql_log_bin=1');
session.close();

// Create a new administrative account on instance 2
shell.connect({scheme:'mysql', 'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', password: 'root'});
session.runSql('SET sql_log_bin=0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('CREATE USER \'foo\'@\'%\' IDENTIFIED BY \'bar\'');
session.runSql('GRANT ALL PRIVILEGES ON *.* TO \'foo\'@\'%\' WITH GRANT OPTION');
session.runSql('SET sql_log_bin=1');
session.close();

// Create a new administrative account on instance 3
shell.connect({scheme:'mysql', 'host': localhost, 'port': __mysql_sandbox_port3, 'user': 'root', password: 'root'});
session.runSql('SET sql_log_bin=0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('CREATE USER \'foo\'@\'%\' IDENTIFIED BY \'bar\'');
session.runSql('GRANT ALL PRIVILEGES ON *.* TO \'foo\'@\'%\' WITH GRANT OPTION');
session.runSql('SET sql_log_bin=1');
session.close();

//@ Connect to instance 1
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'foo', password: 'bar'});

//@ create cluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode: 'REQUIRED'});
else
  var cluster = dba.createCluster('dev', {memberSslMode: 'DISABLED'});

//@ Adding instance 2 using the root account
cluster.addInstance({dbUser: 'root', host: 'localhost', port:__mysql_sandbox_port2}, {password: 'root'});

// Waiting for the instance 2 to become online
wait_slave_state(cluster, uri2, "ONLINE");

//@ Adding instance 3
cluster.addInstance({dbUser: 'foo', host: 'localhost', port:__mysql_sandbox_port3}, {password: 'bar'});

// Waiting for the instance 3 to become online
wait_slave_state(cluster, uri3, "ONLINE");

// stop instance 2
// Use stop sandbox instance to make sure the instance is gone before restarting it
testutil.stopSandbox(__mysql_sandbox_port2, 'root');

// Waiting for instance 2 to become missing
wait_slave_state(cluster, uri2, "(MISSING)");

// Start instance 2
testutil.startSandbox(__mysql_sandbox_port2);

//@<OUT> Cluster status
cluster.status()

//@ Rejoin instance 2
if (__have_ssl)
  cluster.rejoinInstance({DBUser: 'foo', Host: 'localhost', PORT:__mysql_sandbox_port2}, {memberSslMode: 'REQUIRED', password: 'bar'});
else
  cluster.rejoinInstance({DBUser: 'foo', Host: 'localhost', PORT:__mysql_sandbox_port2}, {memberSslMode: 'DISABLED', password: 'bar'});

// Waiting for instance 2 to become back online
wait_slave_state(cluster, uri2, "ONLINE");

//@<OUT> Cluster status after rejoin
cluster.status();

//@<ERR> Cannot rejoin an instance that is already in the group (not missing) Bug#26870329
cluster.rejoinInstance({dbUser: 'foo', host: 'localhost', port:__mysql_sandbox_port2}, {password: 'bar'});

//@ Dissolve cluster
cluster.dissolve({force: true})

session.close();

// Delete the account on instance 1
shell.connect({scheme:'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', password: 'root'});
session.runSql('SET GLOBAL super_read_only = 0');
session.runSql('SET sql_log_bin=0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('SET sql_log_bin=1');
session.close();

// Delete the account on instance 2
shell.connect({scheme:'mysql', 'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', password: 'root'});
session.runSql('SET GLOBAL super_read_only = 0');
session.runSql('SET sql_log_bin=0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('SET sql_log_bin=1');
session.close();

// Delete the account on instance 3
shell.connect({scheme:'mysql', 'host': localhost, 'port': __mysql_sandbox_port3, 'user': 'root', password: 'root'});
session.runSql('SET GLOBAL super_read_only = 0');
session.runSql('SET sql_log_bin=0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('SET sql_log_bin=1');
session.close();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
