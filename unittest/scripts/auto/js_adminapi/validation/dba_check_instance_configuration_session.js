//@ Check Instance Configuration must work without a session
||Can't connect to MySQL server on '<<<libmysql_host_description('localhost', __mysql_sandbox_port1)>>>'

//@ Check Instance Configuration should fail if there's no session nor parameters provided
||An open session is required to perform this operation.

//@ Check Instance Configuration ok with a session
||

//@ Error: user has no privileges to run the checkInstanceConfiguration command (BUG#26609909)
|ERROR: Unable to check privileges for user 'test_user'@'%'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Unable to get roles information. (RuntimeError)

//@ Check instance configuration using a non existing user that authenticates as another user that does not have enough privileges (BUG#26979375)
|ERROR: Unable to check privileges for user 'test_user'@'%'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Unable to get roles information. (RuntimeError)

//@ Check instance configuration using a non existing user that authenticates as another user that has all privileges (BUG#26979375)
||

//@ Check if all missing privileges are reported for user with no privileges
|ERROR: Unable to check privileges for user 'no_privileges'@'%'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Unable to get roles information. (RuntimeError)

//@ Create a user with the root role
||

//@ Check instance using user with a root role as parameter
||

//@ Connect using admin user
||

//@ Check instance using session user with a root role
||

//@ Create role with missing privileges and grant to admin user
||

//@ Check instance using user with admin role as parameter, missing privileges {VER(>=8.0.18)}
|ERROR: The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster:|
|GRANT CLONE_ADMIN, CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION_APPLIER, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SELECT, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'admin_test'@'%' WITH GRANT OPTION;|
||The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster. (RuntimeError)

//@ Check instance using user with admin role as parameter, missing privileges {VER(>=8.0.17) && VER(<8.0.18)}
|ERROR: The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster:|
|GRANT CLONE_ADMIN, CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SELECT, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'admin_test'@'%' WITH GRANT OPTION;|
||The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster. (RuntimeError)

//@ Check instance using user with admin role as parameter, missing privileges {VER(<8.0.17)}
|ERROR: The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster:|
|GRANT CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SELECT, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'admin_test'@'%' WITH GRANT OPTION;|
||The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster. (RuntimeError)

//@ Connect using admin user again
||

//@ Check instance using session user with admin role, missing privileges {VER(>=8.0.18)}
|ERROR: The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster:|
|GRANT CLONE_ADMIN, CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION_APPLIER, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SELECT, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'admin_test'@'%' WITH GRANT OPTION;|
||The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster. (RuntimeError)

//@ Check instance using session user with admin role, missing privileges {VER(>=8.0.17) && VER(<8.0.18)}
|ERROR: The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster:|
|GRANT CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SELECT, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'admin_test'@'%' WITH GRANT OPTION;|
||The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster. (RuntimeError)

//@ Check instance using session user with admin role, missing privileges {VER(<8.0.17)}
|ERROR: The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster:|
|GRANT CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SELECT, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'admin_test'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'admin_test'@'%' WITH GRANT OPTION;|
||The account 'admin_test'@'%' is missing privileges required to manage an InnoDB Cluster. (RuntimeError)

//@ Drop user with role
||

//@<OUT> Check instance must fail if report_host is defined but empty.
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.
ERROR: Invalid 'report_host' value for instance 'localhost:<<<__mysql_sandbox_port2>>>'. The value cannot be empty if defined.

//@<ERR> Check instance must fail if report_host is defined but empty.
The value for variable 'report_host' cannot be empty. (RuntimeError)

//@<OUT> Configure instance must fail if report_host is defined but empty.
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.
ERROR: Invalid 'report_host' value for instance 'localhost:<<<__mysql_sandbox_port2>>>'. The value cannot be empty if defined.

//@<ERR> Configure instance must fail if report_host is defined but empty.
The value for variable 'report_host' cannot be empty. (RuntimeError)

//@<OUT> Create cluster must fail if report_host is defined but empty.
A new InnoDB Cluster will be created on instance ':<<<__mysql_sandbox_port2>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.
ERROR: Invalid 'report_host' value for instance 'localhost:<<<__mysql_sandbox_port2>>>'. The value cannot be empty if defined.

//@<ERR> Create cluster must fail if report_host is defined but empty.
The value for variable 'report_host' cannot be empty. (RuntimeError)

//@ Remove the sandboxes
||

//@ Deploy instances (setting performance_schema value).
||

//@ checkInstanceConfiguration error with performance_schema=off
|ERROR: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' has the performance_schema disabled (performance_schema=OFF). Instances must have the performance_schema enabled to for InnoDB Cluster usage.|performance_schema disabled on target instance. (RuntimeError)

// checkInstanceConfiguration no error with performance_schema=on
||

//@ Remove the sandboxes (final)
||

//@ BUG29018457 - Deploy instance.
||

//@ BUG29018457 - Create a admin_local user only for localhost.
||

//@<OUT> BUG29018457 - check instance fails because only a localhost account is available.
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

ERROR: New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

//@<ERR> BUG29018457 - check instance fails because only a localhost account is available.
User 'admin_local' can only connect from 'localhost'. (RuntimeError)

//@ BUG29018457 - Create a admin_host user only for report_host.
||

//@<OUT> BUG29018457 - check instance fails because only report_host account is available.
Validating local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB Cluster...

ERROR: New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

//@<ERR> BUG29018457 - check instance fails because only report_host account is available.
User 'admin_host' can only connect from '<<<hostname_ip>>>'. (RuntimeError)

//@ BUG29018457 - clean-up (destroy sandbox).
||
