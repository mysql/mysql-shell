// Assumptions: smart deployment rountines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

// Create a new administrative account on instance 1
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', password: 'root'});
session.runSql('SET sql_log_bin=0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('CREATE USER \'foo\'@\'%\' IDENTIFIED BY \'bar\'');
session.runSql('GRANT ALL PRIVILEGES ON *.* TO \'foo\'@\'%\' WITH GRANT OPTION');
session.runSql('SET sql_log_bin=1');

// Create a new administrative account on instance 2
shell.connect({'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', password: 'root'});
session.runSql('SET sql_log_bin=0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('CREATE USER \'foo\'@\'%\' IDENTIFIED BY \'bar\'');
session.runSql('GRANT ALL PRIVILEGES ON *.* TO \'foo\'@\'%\' WITH GRANT OPTION');
session.runSql('SET sql_log_bin=1');

// Create a new administrative account on instance 3
shell.connect({'host': localhost, 'port': __mysql_sandbox_port3, 'user': 'root', password: 'root'});
session.runSql('SET sql_log_bin=0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('CREATE USER \'foo\'@\'%\' IDENTIFIED BY \'bar\'');
session.runSql('GRANT ALL PRIVILEGES ON *.* TO \'foo\'@\'%\' WITH GRANT OPTION');
session.runSql('SET sql_log_bin=1');

//@ Connect to instance 1
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'foo', password: 'bar'});

//@ create cluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode: 'REQUIRED'});
else
  var cluster = dba.createCluster('dev');

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
if (__sandbox_dir)
    dba.stopSandboxInstance(__mysql_sandbox_port2, {sandboxDir:__sandbox_dir, password: 'root'});
else
    dba.stopSandboxInstance(__mysql_sandbox_port2, {password: 'root'});

// Waiting for instance 2 to become missing
wait_slave_state(cluster, uri2, "(MISSING)");

// Start instance 2
try_restart_sandbox(__mysql_sandbox_port2);

//@<OUT> Cluster status
cluster.status()

//@ Rejoin instance 2
if (__have_ssl)
  cluster.rejoinInstance({dbUser: 'foo', host: 'localhost', port:__mysql_sandbox_port2}, {memberSslMode: 'AUTO', password: 'bar'});
else
  cluster.rejoinInstance({dbUser: 'foo', host: 'localhost', port:__mysql_sandbox_port2}, {password: 'bar'});

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
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', password: 'root'});
session.runSql('SET sql_log_bin=0');
session.runSql('SET @sro = @@super_read_only');
session.runSql('SET @ro = @@read_only');
session.runSql('SET GLOBAL super_read_only = 0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('SET GLOBAL super_read_only = @sro');
session.runSql('SET GLOBAL read_only = @ro');
session.runSql('SET sql_log_bin=1');

// Delete the account on instance 2
shell.connect({'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', password: 'root'});
session.runSql('SET sql_log_bin=0');
session.runSql('SET @sro = @@super_read_only');
session.runSql('SET @ro = @@read_only');
session.runSql('SET GLOBAL super_read_only = 0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('SET GLOBAL super_read_only = @sro');
session.runSql('SET GLOBAL read_only = @ro');
session.runSql('SET sql_log_bin=1');

// Delete the account on instance 3
shell.connect({'host': localhost, 'port': __mysql_sandbox_port3, 'user': 'root', password: 'root'});
session.runSql('SET sql_log_bin=0');
session.runSql('SET @sro = @@super_read_only');
session.runSql('SET @ro = @@read_only');
session.runSql('SET GLOBAL super_read_only = 0');
session.runSql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.runSql('SET GLOBAL super_read_only = @sro');
session.runSql('SET GLOBAL read_only = @ro');
session.runSql('SET sql_log_bin=1');

session.close();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here)
  cleanup_sandboxes(true);
