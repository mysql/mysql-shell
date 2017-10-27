# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes();

shell.connect({'scheme': 'mysql', 'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

# Change dynamic variables manually
session.run_sql('SET GLOBAL binlog_checksum=CRC32');
session.run_sql('SET GLOBAL binlog_format=MIXED');

#@ Dba.createCluster
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode':'REQUIRED'})
else:
  cluster = dba.create_cluster('dev', {'memberSslMode':'DISABLED'})

#@ Dissolve cluster (to re-create again)
# Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
cluster.dissolve({"force": True})

# Disable super-read-only (BUG#26422638)
session.run_sql("SET GLOBAL SUPER_READ_ONLY = 0;")

# Create test schema and tables PK, PKE, and UK
# Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
session.run_sql('SET sql_log_bin=0')
session.run_sql('CREATE SCHEMA pke_test')
# Create test table t1 with Primary Key (PK)
session.run_sql('CREATE TABLE pke_test.t1 (id int unsigned NOT NULL AUTO_INCREMENT, PRIMARY KEY `id` (id)) ENGINE=InnoDB')
session.run_sql('INSERT INTO pke_test.t1 VALUES (1);')
# Create test table t2 with Non Null Unique Key, Primary Key Equivalent (PKE)
session.run_sql('CREATE TABLE pke_test.t2 (id int unsigned NOT NULL AUTO_INCREMENT, UNIQUE KEY `id` (id)) ENGINE=InnoDB')
session.run_sql('INSERT INTO pke_test.t2 VALUES (1);')
# Create test table t3 with Nullable Unique Key (UK)
session.run_sql('CREATE TABLE pke_test.t3 (id int unsigned NULL, UNIQUE KEY `id` (id)) ENGINE=InnoDB')
session.run_sql('INSERT INTO pke_test.t3 VALUES (1);')
session.run_sql('INSERT INTO pke_test.t3 VALUES (NULL);')
session.run_sql('INSERT INTO pke_test.t3 VALUES (NULL);')
session.run_sql('SET sql_log_bin=1')

#@ Create cluster fails (one table is not compatible)
# Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
cluster = dba.create_cluster('dev')

#@ Enable verbose
# Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES NOT EXIST
dba.verbose = 1

#@ Create cluster fails (one table is not compatible) - verbose mode
# Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES NOT EXIST
cluster = dba.create_cluster('dev')

#@ Disable verbose
# Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES NOT EXIST
dba.verbose = 0

# Clean-up test schema with PK, PKE, and UK
# Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
session.run_sql('SET sql_log_bin=0')
session.run_sql('DROP SCHEMA pke_test')
session.run_sql('SET sql_log_bin=1')

# @ Create cluster succeeds (no incompatible table)
# Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
cluster = dba.create_cluster('dev')

#@ Dissolve cluster at the end (clean-up)
# Regression for BUG#25974689 : CHECKS ARE MORE STRICT THAN GROUP REPLICATION
cluster.dissolve({"force": True})

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
