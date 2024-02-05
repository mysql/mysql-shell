//@# Dba: createCluster errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Invalid number of arguments, expected 1 to 2 but got 4
||Argument #1 is expected to be a string
||The Cluster name cannot be empty
||Invalid options: another, invalid
||Invalid value for memberSslMode option. Supported values: DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY,AUTO.
||Invalid value for memberSslMode option. Supported values: DISABLED,REQUIRED,VERIFY_CA,VERIFY_IDENTITY,AUTO.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (#) (ArgumentError)
||_1234567890::_1234567890123456789012345678901234567890123456789012345678901234: The Cluster name can not be greater than 63 characters. (ArgumentError)
||Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (::) (ArgumentError)

//@ Dba: createCluster with ANSI_QUOTES success
|Current sql_mode is: ANSI_QUOTES|
|Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.|
|<Cluster:devCluster>|

//@ Dba: dissolve cluster created with ansi_quotes and restore original sql_mode
|The cluster was successfully dissolved.|
|Original SQL_MODE has been restored: true|

//@ Dba: create cluster using a non existing user that authenticates as another user (BUG#26979375)
|<Cluster:devCluster>|

//@ Dba: dissolve cluster created using a non existing user that authenticates as another user (BUG#26979375)
||

//@<OUT> Dba: createCluster with interaction {VER(>=8.0.11)}
A new InnoDB Cluster will be created on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

* Checking connectivity and SSL configuration...

Creating InnoDB Cluster 'devCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@<OUT> Dba: createCluster with interaction {VER(<8.0.11)}
A new InnoDB Cluster will be created on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

NOTE: The 'localAddress' "<<<hostname>>>" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.
See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.
NOTE: When adding more instances to the Cluster, be aware that the subnet masks dictate whether the instance's address is automatically added to the allowlist or not. Please specify the 'ipAllowlist' accordingly if needed.
* Checking connectivity and SSL configuration...

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally to persist the changes.
Creating InnoDB Cluster 'devCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ Dba: checkInstanceConfiguration in a cluster member
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid for InnoDB Cluster usage.|

//@<OUT> Dba: checkInstanceConfiguration ok 1
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB Cluster usage.

//@<OUT> Dba: checkInstanceConfiguration ok 2
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB Cluster usage.


//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.3)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
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

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.23) && VER(<8.0.25)}
+----------------------------------------+---------------+----------------+------------------------+
| Variable                               | Current Value | Required Value | Note                   |
+----------------------------------------+---------------+----------------+------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file |
| binlog_transaction_dependency_tracking | <not set>     | WRITESET       | Update the config file |
| enforce_gtid_consistency               | <not set>     | ON             | Update the config file |
| gtid_mode                              | OFF           | ON             | Update the config file |
| master_info_repository                 | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository              | <not set>     | TABLE          | Update the config file |
| report_port                            | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                              | <not set>     | <unique ID>    | Update the config file |
| slave_parallel_type                    | <not set>     | LOGICAL_CLOCK  | Update the config file |
| slave_preserve_commit_order            | <not set>     | ON             | Update the config file |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file |
+----------------------------------------+---------------+----------------+------------------------+

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(==8.0.25)}
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
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB Cluster...
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
|A new InnoDB Cluster will be created on instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'.|
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

//@<OUT> Dba: getCluster with interaction
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - tabbed
tabbed
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - table
table
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - vertical
vertical
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - json
json
<Cluster:devCluster>

//@<OUT> Dba: getCluser validate object serialization output - json/raw
json/raw
<Cluster:devCluster>

//@<OUT> Dba: getCluster with interaction (default)
<Cluster:devCluster>

//@<OUT> Dba: getCluster with interaction (default null)
<Cluster:devCluster>
