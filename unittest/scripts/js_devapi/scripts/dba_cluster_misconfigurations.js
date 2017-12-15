// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_checksum", "CRC32");
// TODO(.) - changing the binlog_format will cause the createCluster to fail because of bug #27112727
// testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_format", "MIXED");

testutil.restartSandbox(__mysql_sandbox_port1, "root");

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Dba.createCluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});
else
  var cluster = dba.createCluster('dev', {memberSslMode:'DISABLED'});

//@ Dissolve cluster (to re-create again)
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
cluster.dissolve({force: true});
cluster.disconnect();

// Disable super-read-only (BUG#26422638)
session.runSql("SET GLOBAL SUPER_READ_ONLY = 0;");

// Create test schema and tables PK, PKE, and UK
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
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

//@ Create cluster fails (one table is not compatible)
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
var cluster = dba.createCluster('dev');

//@ Enable verbose
// Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES NOT EXIST
dba.verbose = 1;

//@ Create cluster fails (one table is not compatible) - verbose mode
// Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES NOT EXIST
var cluster = dba.createCluster('dev');

//@ Disable verbose
// Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES NOT EXIST
dba.verbose = 0;

// Clean-up test schema with PK, PKE, and UK
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
session.runSql('SET sql_log_bin=0');
session.runSql('DROP SCHEMA pke_test');
session.runSql('SET sql_log_bin=1');

//@ Create cluster succeeds (no incompatible table)
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
var cluster = dba.createCluster('dev');

//@ Dissolve cluster at the end (clean-up)
// Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
cluster.dissolve({force: true});
cluster.disconnect();
session.close();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
testutil.destroySandbox(__mysql_sandbox_port1);
