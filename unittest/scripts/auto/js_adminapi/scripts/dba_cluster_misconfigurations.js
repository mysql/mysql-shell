// Assumptions: smart deployment rountines available
//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

// create cluster admin as a root
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
EXPECT_STDERR_EMPTY();

//@<> create cluster admin
dba.configureLocalInstance("root:root@localhost:" + __mysql_sandbox_port1, {clusterAdmin: "ca", clusterAdminPassword: "ca", mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});
EXPECT_STDOUT_CONTAINS("Cluster admin user 'ca'@'%' created.")

session.close();

testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_format", "MIXED");

testutil.restartSandbox(__mysql_sandbox_port1);

// connect as cluster admin
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'ca', password: 'ca'});

//@<> Dba.createCluster (fail because of bad configuration)
EXPECT_THROWS(function() { dba.createCluster('dev', {memberSslMode:'REQUIRED', gtidSetIsComplete: true}); }, "Instance check failed", "RuntimeError")

//@<> Dba.createCluster (fail because of bad configuration of parallel-appliers) {VER(>=8.0.23)}
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_transaction_dependency_tracking", "COMMIT_ORDER");
testutil.changeSandboxConf(__mysql_sandbox_port1, "slave_parallel_type", "DATABASE");
testutil.changeSandboxConf(__mysql_sandbox_port1, "slave_preserve_commit_order", "OFF");
testutil.changeSandboxConf(__mysql_sandbox_port1, "transaction_write_set_extraction", "OFF");

testutil.restartSandbox(__mysql_sandbox_port1);

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'ca', password: 'ca'});
EXPECT_THROWS(function() { dba.createCluster('dev', {memberSslMode:'REQUIRED', gtidSetIsComplete: true}); }, "Instance check failed", "RuntimeError")

//@<> Dba.createCluster (succeeds with right configuration of parallel-appliers) {VER(>=8.0.23)}
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_transaction_dependency_tracking", "WRITESET");
testutil.changeSandboxConf(__mysql_sandbox_port1, "slave_parallel_type", "LOGICAL_CLOCK");
testutil.changeSandboxConf(__mysql_sandbox_port1, "slave_preserve_commit_order", "ON");
testutil.changeSandboxConf(__mysql_sandbox_port1, "transaction_write_set_extraction", "XXHASH64");
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_format", "ROW");

testutil.restartSandbox(__mysql_sandbox_port1);

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'ca', password: 'ca'});
var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED', gtidSetIsComplete: true}); });


//@<> Cluster.addInstance (fail because of bad configuration of parallel-appliers) {VER(>=8.0.23)}
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

// Create cluster admin account
dba.configureLocalInstance("root:root@localhost:" + __mysql_sandbox_port2, {clusterAdmin: "ca", clusterAdminPassword: "ca", mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port2)});
EXPECT_STDOUT_CONTAINS("Cluster admin user 'ca'@'%' created.")

testutil.changeSandboxConf(__mysql_sandbox_port2, "binlog_transaction_dependency_tracking", "COMMIT_ORDER");
testutil.changeSandboxConf(__mysql_sandbox_port2, "slave_parallel_type", "DATABASE");
testutil.changeSandboxConf(__mysql_sandbox_port2, "slave_preserve_commit_order", "OFF");
testutil.changeSandboxConf(__mysql_sandbox_port2, "transaction_write_set_extraction", "OFF");
testutil.restartSandbox(__mysql_sandbox_port2);

EXPECT_THROWS(function() { cluster.addInstance(__sandbox_uri2); }, "Instance check failed")

//@<> Cluster.addInstance (succeeds with right configuration of parallel-appliers) {VER(>=8.0.23)}
testutil.changeSandboxConf(__mysql_sandbox_port2, "binlog_transaction_dependency_tracking", "WRITESET");
testutil.changeSandboxConf(__mysql_sandbox_port2, "slave_parallel_type", "LOGICAL_CLOCK");
testutil.changeSandboxConf(__mysql_sandbox_port2, "slave_preserve_commit_order", "ON");
testutil.changeSandboxConf(__mysql_sandbox_port2, "transaction_write_set_extraction", "XXHASH64");
testutil.restartSandbox(__mysql_sandbox_port2);

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); })

//@<> Cluster.addInstance (succeeds with right configuration of parallel-appliers): finalization {VER(>=8.0.23)}
cluster.dissolve({force: true});

testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_format", "MIXED");
testutil.restartSandbox(__mysql_sandbox_port1);

//@<> Setup next test
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
// Create test schema and tables PK, PKE, and UK
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
session.runSql('SET GLOBAL super_read_only=0');
session.runSql('SET sql_log_bin=0');
session.runSql('CREATE SCHEMA pke_test');
// Create test table t1 with Primary Key (PK)
session.runSql('CREATE TABLE pke_test.t1 (id int unsigned NOT NULL AUTO_INCREMENT, PRIMARY KEY `id` (id)) ENGINE=InnoDB');
session.runSql('INSERT INTO pke_test.t1 VALUES (1);');
// Create test table t2 with Non Null Unique Key, Primary Key Equivalent (PKE)
session.runSql('CREATE TABLE pke_test.t2 (id int unsigned NOT NULL AUTO_INCREMENT, UNIQUE KEY `id` (id)) ENGINE=InnoDB');
session.runSql('INSERT INTO pke_test.t2 VALUES (1);');
// Create test table t3 with Nullable Unique Key (UK)
session.runSql('CREATE TABLE pke_test.t3 (id int unsigned NULL, UNIQUE KEY `id` (id)) ENGINE=InnoDB');
session.runSql('INSERT INTO pke_test.t3 VALUES (1);');
session.runSql('INSERT INTO pke_test.t3 VALUES (NULL);');
session.runSql('INSERT INTO pke_test.t3 VALUES (NULL);');
session.runSql('SET sql_log_bin=1');

session.close();

EXPECT_STDERR_EMPTY();

// switch back to cluster admin
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'ca', password: 'ca'});

//@<> Create cluster fails (one table is not compatible)
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
WIPE_STDOUT()
EXPECT_THROWS(function() { dba.createCluster('dev'); }, "Instance check failed", "RuntimeError");
EXPECT_STDOUT_CONTAINS("ERROR: The following tables do not have a Primary Key or equivalent column:")

//@<> Enable verbose
// Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES NOT EXIST
dba.verbose = 1;
EXPECT_STDERR_EMPTY();

//@<> Create cluster fails (one table is not compatible) - verbose mode
// Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES NOT EXIST
WIPE_STDOUT()
EXPECT_THROWS(function() { dba.createCluster('dev'); }, "Instance check failed", "RuntimeError");
EXPECT_STDOUT_CONTAINS("ERROR: The following tables do not have a Primary Key or equivalent column:")

//@<> Disable verbose
// Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES NOT EXIST
dba.verbose = 0;
EXPECT_STDERR_EMPTY();

session.close();

// delete database as root
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

// Clean-up test schema with PK, PKE, and UK
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
session.runSql('SET sql_log_bin=0');
session.runSql('DROP SCHEMA pke_test');
session.runSql('SET sql_log_bin=1');

session.close();

// switch back to cluster admin
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'ca', password: 'ca'});

testutil.removeFromSandboxConf(__mysql_sandbox_port1, "binlog_format");
session.runSql("SET GLOBAL binlog_format='ROW'");

//@<> Create cluster succeeds (no incompatible table)
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster('dev', {gtidSetIsComplete: true}); })
EXPECT_STDOUT_NOT_CONTAINS("the --allow-non-compatible-tables option");

//@<> Cluster.addInstance (fail because of bad configuration of group_replication_tls_source) {VER(>=8.0.21)}
testutil.changeSandboxConf(__mysql_sandbox_port2, "group_replication_tls_source", "MYSQL_ADMIN");
testutil.restartSandbox(__mysql_sandbox_port2);
EXPECT_THROWS(function() { cluster.addInstance(__sandbox_uri2); }, "Instance check failed")

//@<> Cluster.addInstance (fail because of bad configuration of group_replication_tls_source / PERSISTED) {VER(>=8.0.21)}
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "group_replication_tls_source");
testutil.restartSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri2);
session.runSql("SET PERSIST group_replication_tls_source='mysql_admin'");
EXPECT_THROWS(function() { cluster.addInstance(__sandbox_uri2); }, "Instance check failed")

//@<> dba.configureInstance() to fix the bad configuration of group_replication_tls_source {VER(>=8.0.21)}
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri2, {clearReadOnly: 1}); });

//@<> Cluster.addInstance() successful {VER(>=8.0.21)}
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

//@<> Cluster.rejoinInstance (fail because of bad configuration of group_replication_tls_source / PERSISTED) {VER(>=8.0.21)}
session.runSql("stop group_replication");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "group_replication_tls_source");
disable_auto_rejoin(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri2);
session.runSql("SET PERSIST group_replication_tls_source='mysql_admin'");
EXPECT_THROWS(function() { cluster.rejoinInstance(__endpoint2); }, "Instance check failed")

//@<> dba.configureInstance() to fix the bad configuration of group_replication_tls_source in rejoinInstance {VER(>=8.0.21)}
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri2, {clearReadOnly: 1}); });

//@<> Cluster.rejoinInstance() successful {VER(>=8.0.21)}
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint2); });

//@<> Tests for rebootCluster(), simulate an outage
if (__version_num >= 80021) disable_auto_rejoin(__mysql_sandbox_port2);
disable_auto_rejoin(__mysql_sandbox_port1);
if (__version_num >= 80021) testutil.killSandbox(__mysql_sandbox_port2);
testutil.killSandbox(__mysql_sandbox_port1);
if (__version_num >= 80021) testutil.startSandbox(__mysql_sandbox_port2);
testutil.startSandbox(__mysql_sandbox_port1);

//@<> dba.rebootClusterFromCompleteOutage (fail because of bad configuration of group_replication_tls_source / PERSISTED) {VER(>=8.0.21)}
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "group_replication_tls_source");

shell.connect(__sandbox_uri1);
session.runSql("SET PERSIST group_replication_tls_source='mysql_admin'");

EXPECT_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage(); }, "Instance check failed")
EXPECT_OUTPUT_CONTAINS("ERROR: Instance must be configured and validated with dba.checkInstanceConfiguration() and dba.configureInstance() before it can be used in an InnoDB cluster.");

//@<> dba.configureInstance() to fix the bad configuration of group_replication_tls_source in rebootClusterFromCompleteOutage() {VER(>=8.0.21)}
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri1, {clearReadOnly: 1}); });

//@<> dba.rebootClusterFromCompleteOutage() successful {VER(>=8.0.21)}
EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage(); });

//@<> dba.rebootClusterFromCompleteOutage (fail because of bad configuration of binlog_format) {VER(<8.0.21)}
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_format", "MIXED");
disable_auto_rejoin(__mysql_sandbox_port1);
testutil.killSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
EXPECT_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage(); }, "Instance check failed")
EXPECT_OUTPUT_CONTAINS("ERROR: Instance must be configured and validated with dba.checkInstanceConfiguration() and dba.configureInstance() before it can be used in an InnoDB cluster.");

//@<> dba.configureInstance() to fix the bad configuration of group_replication_tls_source in rebootClusterFromCompleteOutage() {VER(<8.0.21)}
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri1, {clearReadOnly: 1}); });

//@<> dba.rebootClusterFromCompleteOutage() successful {VER(<8.0.21)}
EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage(); });

//@<> Dissolve cluster at the end (clean-up)
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
cluster.dissolve({force: true});
cluster.disconnect();
session.close();

//@<> Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
