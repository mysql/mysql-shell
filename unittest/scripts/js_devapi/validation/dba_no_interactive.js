
//@# Dba: createCluster errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Argument #1 is expected to be a string
||The Cluster name cannot be empty
||Argument #2 is expected to be a map
||Argument #2 is expected to be a map
||Invalid options: another, invalid
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use the multiMaster and multiPrimary options simultaneously. The multiMaster option is deprecated, please use the multiPrimary option instead.
||Cannot use the multiMaster and multiPrimary options simultaneously. The multiMaster option is deprecated, please use the multiPrimary option instead.
||Invalid value for ipWhitelist: string value cannot be empty.

//@# Dba: createCluster ipWhitelist deprecation {VER(>=8.0.22)}
||Cannot use the ipWhitelist and ipAllowlist options simultaneously. The ipWhitelist option is deprecated, please use the ipAllowlist option instead. (ArgumentError)

//@ Dba: createCluster with ANSI_QUOTES success
|Current sql_mode is: ANSI_QUOTES|
|<Cluster:devCluster>|

//@ Dba: dissolve cluster created with ansi_quotes and restore original sql_mode
|Original SQL_MODE has been restored: true|

//@ Dba: create cluster with memberSslMode AUTO succeed
|<Cluster:devCluster>|

//@ Dba: dissolve cluster created with memberSslMode AUTO
||

//@ Dba: createCluster success
|<Cluster:devCluster>|

//@# Dba: createCluster already exist
||Unable to create cluster. The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' already belongs to an InnoDB cluster. Use dba.getCluster() to access it.

//@# Dba: checkInstanceConfiguration errors
||Access denied for user 'root'@'localhost' (using password: NO) (MySQL Error 1045)
||Access denied for user 'sample'@'localhost' (using password: NO) (MySQL Error 1045)

//@ Dba: checkInstanceConfiguration ok1
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.|

//@ Dba: checkInstanceConfiguration ok2
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.|

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.3)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file mybad.cnf will also be checked.

NOTE: Some configuration options need to be fixed:
//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.3) && VER(<8.0.23)}
+----------------------------------+---------------+----------------+------------------------+
| Variable                         | Current Value | Required Value | Note                   |
+----------------------------------+---------------+----------------+------------------------+<<<(__version_num<80021) ?  "\n| binlog_checksum                  | <not set>     | NONE           | Update the config file |\n":"">>>
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.23) && VER(<8.0.26)}
+----------------------------------------+---------------+----------------+------------------------+
| Variable                               | Current Value | Required Value | Note                   |
+----------------------------------------+---------------+----------------+------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file |
| binlog_transaction_dependency_tracking | <not set>     | WRITESET       | Update the config file |
| enforce_gtid_consistency               | <not set>     | ON             | Update the config file |
| gtid_mode                              | OFF           | ON             | Update the config file |
| report_port                            | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                              | <not set>     | <unique ID>    | Update the config file |
| slave_parallel_type                    | <not set>     | LOGICAL_CLOCK  | Update the config file |
| slave_preserve_commit_order            | <not set>     | ON             | Update the config file |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file |
+----------------------------------------+---------------+----------------+------------------------+

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.26)}
+----------------------------------------+---------------+----------------+------------------------+
| Variable                               | Current Value | Required Value | Note                   |
+----------------------------------------+---------------+----------------+------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file |
| binlog_transaction_dependency_tracking | <not set>     | WRITESET       | Update the config file |
| enforce_gtid_consistency               | <not set>     | ON             | Update the config file |
| gtid_mode                              | OFF           | ON             | Update the config file |
| replica_parallel_type                  | <not set>     | LOGICAL_CLOCK  | Update the config file |
| replica_preserve_commit_order          | <not set>     | ON             | Update the config file |
| report_port                            | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                              | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file |
+----------------------------------------+---------------+----------------+------------------------+

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.3)}
NOTE: Please use the dba.configureInstance() command to repair these issues.

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(<8.0.3)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file mybad.cnf will also be checked.

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------+
| Variable                         | Current Value | Required Value | Note                   |
+----------------------------------+---------------+----------------+------------------------+
| binlog_checksum                  | <not set>     | NONE           | Update the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| log_bin                          | <not set>     | <no value>     | Update the config file |
| log_slave_updates                | <not set>     | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

NOTE: Please use the dba.configureInstance() command to repair these issues.

//@# Dba: configureLocalInstance errors
//||This function only works with local instances
||Access denied for user 'root'@'localhost' (using password: NO) (MySQL Error 1045)
||Access denied for user 'sample'@'localhost' (using password: NO) (MySQL Error 1045)

//@# Dba: configureLocalInstance errors 5.7 {VER(<8.0.11)}
|ERROR: The path to the MySQL configuration file is required to verify and fix InnoDB cluster related options.|
||Unable to update MySQL configuration file.

//@# Dba: configureLocalInstance errors 8.0 {VER(>=8.0.11)}
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.|
|Calling this function on a cluster member is only required for MySQL versions 8.0.4 or earlier.|

//@<OUT> Dba: configureLocalInstance updating config file {VER(>=8.0.3)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
//@<OUT> Dba: configureLocalInstance updating config file {VER(>=8.0.3) && VER(<8.0.23)}
+----------------------------------+---------------+----------------+------------------------+
| Variable                         | Current Value | Required Value | Note                   |
+----------------------------------+---------------+----------------+------------------------+<<<(__version_num<80021) ?  "\n| binlog_checksum                  | <not set>     | NONE           | Update the config file |\n":"">>>
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

//@<OUT> Dba: configureLocalInstance updating config file {VER(>=8.0.23) && VER(<8.0.26)}
+----------------------------------------+---------------+----------------+------------------------+
| Variable                               | Current Value | Required Value | Note                   |
+----------------------------------------+---------------+----------------+------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file |
| binlog_transaction_dependency_tracking | <not set>     | WRITESET       | Update the config file |
| enforce_gtid_consistency               | <not set>     | ON             | Update the config file |
| gtid_mode                              | OFF           | ON             | Update the config file |
| report_port                            | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                              | <not set>     | <unique ID>    | Update the config file |
| slave_parallel_type                    | <not set>     | LOGICAL_CLOCK  | Update the config file |
| slave_preserve_commit_order            | <not set>     | ON             | Update the config file |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file |
+----------------------------------------+---------------+----------------+------------------------+

//@<OUT> Dba: configureLocalInstance updating config file {VER(>=8.0.26)}
+----------------------------------------+---------------+----------------+------------------------+
| Variable                               | Current Value | Required Value | Note                   |
+----------------------------------------+---------------+----------------+------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file |
| binlog_transaction_dependency_tracking | <not set>     | WRITESET       | Update the config file |
| enforce_gtid_consistency               | <not set>     | ON             | Update the config file |
| gtid_mode                              | OFF           | ON             | Update the config file |
| replica_parallel_type                  | <not set>     | LOGICAL_CLOCK  | Update the config file |
| replica_preserve_commit_order          | <not set>     | ON             | Update the config file |
| report_port                            | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                              | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file |
+----------------------------------------+---------------+----------------+------------------------+

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was configured to be used in an InnoDB cluster.

//@<OUT> Dba: configureLocalInstance updating config file {VER(<8.0.3)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------+
| Variable                         | Current Value | Required Value | Note                   |
+----------------------------------+---------------+----------------+------------------------+
| binlog_checksum                  | <not set>     | NONE           | Update the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| log_bin                          | <not set>     | <no value>     | Update the config file |
| log_slave_updates                | <not set>     | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was configured to be used in an InnoDB cluster.


//@ Dba: configureLocalInstance report fixed 1
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.|

//@ Dba: configureLocalInstance report fixed 2
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.|

//@ Dba: Create user without all necessary privileges
|Number of accounts: 1|

//@# Dba: configureLocalInstance not enough privileges {VER(>=8.0.0)}
|ERROR: Unable to check privileges for user 'missingprivileges'@'localhost'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Unable to get roles information. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges {VER(<8.0.0)}
|ERROR: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster:|
|GRANT FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SELECT, SHUTDOWN ON *.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ Dba: Show list of users to make sure the user missingprivileges@% was not created
|Number of accounts: 0|

//@ Dba: Delete created user and reconnect to previous sandbox
|Number of accounts: 0|

//@ Dba: create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 1|

//@<OUT> Dba: configureLocalInstance create different admin user
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>
Assuming full account name 'dba_test'@'%' for dba_test

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.
Cluster admin user 'dba_test'@'%' created.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is already ready to be used in an InnoDB cluster.

?{VER(>=8.0.23)}
Successfully enabled parallel appliers.
?{}

//@<OUT> Dba: configureLocalInstance create existing valid admin user
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>
Assuming full account name 'dba_test'@'%' for dba_test
User 'dba_test'@'%' already exists and will not be created.

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.

//@ Dba: remove needed privilege (REPLICATION SLAVE) from created admin user
||

//@# Dba: configureLocalInstance create existing invalid admin user
|WARNING: User 'dba_test'@'%' already exists and will not be created. However, it is missing privileges.|
|The account 'dba_test'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT REPLICATION SLAVE ON *.* TO 'dba_test'@'%' WITH GRANT OPTION;|
||The account 'mydba'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ Dba: Delete previously create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 0|

//@ configureLocalInstance() should fail if user does not have global GRANT OPTION {VER(>=8.0.18)}
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT CLONE_ADMIN, CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, REPLICATION_APPLIER, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SELECT, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ configureLocalInstance() should fail if user does not have global GRANT OPTION {VER(>=5.7.0) && VER(<8.0.0)}
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT CREATE USER, FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SELECT, SHUTDOWN, SUPER ON *.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ createCluster() should fail if user does not have global GRANT OPTION {VER(>=8.0.18)}
|Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT CLONE_ADMIN, CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, REPLICATION_APPLIER, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SELECT, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ createCluster() should fail if user does not have global GRANT OPTION {VER(>=5.7.0) && VER(<8.0.0)}
|Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT CREATE USER, FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SELECT, SHUTDOWN, SUPER ON *.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@# Dba: getCluster errors
||Argument #1 is expected to be a string
||Invalid number of arguments, expected 0 to 2 but got 3
||Argument #2 is expected to be a map
||The cluster with the name '' does not exist.
||The cluster with the name '#' does not exist.
||The cluster with the name 'over40chars_12345678901234567890123456789' does not exist.


//@ Dba: getCluster
|<Cluster:devCluster>|
