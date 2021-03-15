// Assumptions: smart deployment rountines available
//@ Initialization
begin_dba_log_sql(2);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

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
var cluster = dba.createCluster('dev', {memberSslMode: 'REQUIRED', gtidSetIsComplete: true});

//@ Adding instance 2 using the root account
cluster.addInstance({dbUser: 'root', host: 'localhost', port:__mysql_sandbox_port2}, {password: 'root'});

// Waiting for the instance 2 to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Adding instance 3
cluster.addInstance({dbUser: 'foo', host: 'localhost', port:__mysql_sandbox_port3}, {password: 'bar'});

// Waiting for the instance 3 to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// stop instance 2
// Use stop sandbox instance to make sure the instance is gone before restarting it
testutil.stopSandbox(__mysql_sandbox_port2);

// Waiting for instance 2 to become missing
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

// Start instance 2
testutil.startSandbox(__mysql_sandbox_port2);

//@<> Cluster status
var topology = cluster.status()["defaultReplicaSet"]["topology"];
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port2}`]["status"], "(MISSING)");
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port3}`]["status"], "ONLINE");


//@ ipWhitelist deprecation error {VER(>=8.0.22)}
cluster.rejoinInstance(__sandbox_uri2, {ipWhitelist: "AUTOMATIC", ipAllowlist: "127.0.0.1"});

//@<OUT> Rejoin instance 2
// Regression for BUG#270621122: Deprecate memberSslMode
cluster.rejoinInstance({DBUser: 'foo', Host: 'localhost', PORT:__mysql_sandbox_port2}, {memberSslMode: 'REQUIRED', password: 'bar'});

// Waiting for instance 2 to become back online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Cluster status after rejoin
var topology = cluster.status()["defaultReplicaSet"]["topology"];
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port2}`]["status"], "ONLINE");
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port3}`]["status"], "ONLINE");

//@<OUT> Cannot rejoin an instance that is already in the group (not missing) Bug#26870329
// operation should succeed (no error), but not do anything
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

//@<> Finalization
var logs = end_dba_log_sql();
EXPECT_NO_SQL(__sandbox1, logs);
EXPECT_NO_SQL(__sandbox2, logs);
EXPECT_NO_SQL(__sandbox3, logs);
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
