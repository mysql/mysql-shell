// Assumptions: smart deployment rountines available
//@<> Initialization
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

//@<> Connect to instance 1
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'foo', password: 'bar'});

//@<> create cluster
var cluster;
EXPECT_NO_THROWS(function(){ cluster = dba.createCluster('dev', {memberSslMode: 'REQUIRED', gtidSetIsComplete: true}); });

//@<> Adding instance 2 using the root account
EXPECT_NO_THROWS(function(){ cluster.addInstance({user: 'root', password: 'root', host: 'localhost', port:__mysql_sandbox_port2}); });

// Waiting for the instance 2 to become online
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Adding instance 3
EXPECT_NO_THROWS(function(){ cluster.addInstance({user: 'foo', password: 'bar', host: 'localhost', port:__mysql_sandbox_port3}); });

// Waiting for the instance 3 to become online
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// stop instance 2
// Use stop sandbox instance to make sure the instance is gone before restarting it
disable_auto_rejoin(__mysql_sandbox_port2);
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

//@<> Rejoin instance 2
cluster.rejoinInstance({user: 'foo', Host: 'localhost', PORT:__mysql_sandbox_port2, password: 'bar'});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Cluster status after rejoin
var topology = cluster.status()["defaultReplicaSet"]["topology"];
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port2}`]["status"], "ONLINE");
EXPECT_EQ(topology[`${hostname}:${__mysql_sandbox_port3}`]["status"], "ONLINE");

//@<> Cannot rejoin an instance that is already in the group (not missing) Bug#26870329
EXPECT_NO_THROWS(function(){ cluster.rejoinInstance({user: 'foo', password: 'bar', host: 'localhost', port:__mysql_sandbox_port2}); });
EXPECT_OUTPUT_CONTAINS(`NOTE: ${hostname}:${__mysql_sandbox_port2} is already an active (ONLINE) member of cluster 'dev'.`)

//@<> Dissolve cluster
EXPECT_NO_THROWS(function(){ cluster.dissolve({force: true}); });

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
