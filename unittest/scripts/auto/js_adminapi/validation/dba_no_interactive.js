//@# Dba: createCluster errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Argument #1 is expected to be a string
||The Cluster name cannot be empty
||Argument #2 is expected to be a map
||Argument #2 is expected to be a map
||Invalid options: another, invalid
||Invalid value for memberSslMode option. Supported values: DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY,AUTO.
||Invalid value for memberSslMode option. Supported values: DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY,AUTO.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.

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
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB Cluster usage.|

//@ Dba: checkInstanceConfiguration ok2
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB Cluster usage.|

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.3)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
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

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.26) && VER(<8.3.0)}
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

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.4.0)}
+-------------------------------+---------------+----------------+------------------------+
| Variable                      | Current Value | Required Value | Note                   |
+-------------------------------+---------------+----------------+------------------------+
| binlog_format                 | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency      | <not set>     | ON             | Update the config file |
| gtid_mode                     | OFF           | ON             | Update the config file |
| replica_preserve_commit_order | <not set>     | ON             | Update the config file |
| report_port                   | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                     | <not set>     | <unique ID>    | Update the config file |
+-------------------------------+---------------+----------------+------------------------+


//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.3)}
NOTE: Please use the dba.configureInstance() command to repair these issues.

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(<8.0.3)}
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file mybad.cnf will also be checked.

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                           |
+----------------------------------+---------------+----------------+------------------------------------------------+
| binlog_checksum                  | <not set>     | NONE           | Update the config file                         |
| binlog_format                    | <not set>     | ROW            | Update the config file                         |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file                         |
| gtid_mode                        | OFF           | ON             | Update the config file                         |
| log_bin                          | <not present> | ON             | Update the config file and restart the server  |
| log_slave_updates                | <not set>     | ON             | Update the server variable and the config file |
| master_info_repository           | <not set>     | TABLE          | Update the server variable and the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the server variable and the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the server variable and the config file |
| server_id                        | <not set>     | <unique ID>    | Update the server variable and the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the server variable and the config file |
+----------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
NOTE: Please use the dba.configureInstance() command to repair these issues.

//@ createCluster() should fail if user does not have global GRANT OPTION {VER(>=8.0.18)}
|Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB Cluster:|
|GRANT CLONE_ADMIN, CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, REPLICATION_APPLIER, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SELECT, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB Cluster. (RuntimeError)

//@# Dba: getCluster errors
||Argument #1 is expected to be a string
||Invalid number of arguments, expected 0 to 1 but got 3
||Invalid number of arguments, expected 0 to 1 but got 2
||The cluster with the name '' does not exist.
||The cluster with the name '#' does not exist.
||The cluster with the name 'over40chars_12345678901234567890123456789' does not exist.

//@ Dba: getCluster
|<Cluster:devCluster>|
