//@ Initialization
||

//@# Dba: createCluster errors
||Dba.createCluster: Invalid number of arguments, expected 1 to 2 but got 0
||Dba.createCluster: Invalid number of arguments, expected 1 to 2 but got 4
||Dba.createCluster: Argument #1 is expected to be a string
||Dba.createCluster: The Cluster name cannot be empty
||Dba.createCluster: Invalid options: another, invalid
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
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
||Dba.createCluster: Cluster name may only contain alphanumeric characters or '_', and may not start with a number (#) (ArgumentError)
||Dba.createCluster: _1234567890::_1234567890123456789012345678901: The Cluster name can not be greater than 40 characters. (ArgumentError)
||Dba.createCluster: Cluster name may only contain alphanumeric characters or '_', and may not start with a number (::) (ArgumentError)

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
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

Creating InnoDB cluster 'devCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
NOTE: Metadata schema found in target instance
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@<OUT> Dba: createCluster with interaction {VER(<8.0.11)}
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
Creating InnoDB cluster 'devCluster' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
NOTE: Metadata schema found in target instance
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ Dba: checkInstanceConfiguration in a cluster member
|Please provide the password for 'root@localhost:<<<__mysql_sandbox_port1>>>':|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.|

//@<OUT> Dba: checkInstanceConfiguration ok 1
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.

//@<OUT> Dba: checkInstanceConfiguration ok 2
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Instance configuration is compatible with InnoDB cluster

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.


//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(>=8.0.3)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
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
+----------------------------------+---------------+----------------+------------------------+<<<(__version_num<80021) ?  "\n| binlog_checksum                  | <not set>     | NONE           | Update the config file |\n":"">>>
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| log_slave_updates                | <not set>     | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

NOTE: Please use the dba.configureInstance() command to repair these issues.

//@<OUT> Dba: checkInstanceConfiguration report with errors {VER(<8.0.3)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
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

//@<OUT> Dba: configureLocalInstance error 3 {VER(<8.0.11)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port1>>>': The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.
Sandbox MySQL configuration file at: <<<__output_sandbox_dir>>><<<__mysql_sandbox_port1>>><<<__path_splitter>>>my.cnf
Persisting the cluster settings...
WARNING: The 'group_replication_group_seeds' is not defined on instance 'localhost:<<<__mysql_sandbox_port1>>>'. This option is mandatory to allow the server to automatically rejoin the cluster after reboot. Please manually update its value on the option file.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured for use in an InnoDB cluster.

The instance cluster settings were successfully persisted.

//@ Dba: configureLocalInstance error 3 bad call {VER(>=8.0.11)}
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.|
|Calling this function on a cluster member is only required for MySQL versions 8.0.4 or earlier.|

//@ Dba: Create user without all necessary privileges
|Number of accounts: 1|

//@# Dba: configureLocalInstance not enough privileges 1 {VER(>=8.0.0)}
|ERROR: Unable to check privileges for user 'missingprivileges'@'localhost'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Dba.configureLocalInstance: Unable to get roles information. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 1 {VER(<8.0.0)}
|ERROR: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster:|
|GRANT FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SELECT, SHUTDOWN ON *.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 2 {VER(>=8.0.0)}
|ERROR: Unable to check privileges for user 'missingprivileges'@'localhost'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Dba.configureLocalInstance: Unable to get roles information. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 2 {VER(<8.0.0)}
|ERROR: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster:|
|GRANT FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SELECT, SHUTDOWN ON *.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 3 {VER(>=8.0.0)}
|ERROR: Unable to check privileges for user 'missingprivileges'@'localhost'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Dba.configureLocalInstance: Unable to get roles information. (RuntimeError)

//@# Dba: configureLocalInstance not enough privileges 3 {VER(<8.0.0)}
|ERROR: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster:|
|GRANT FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SELECT, SHUTDOWN ON *.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'missingprivileges'@'localhost' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ Dba: Show list of users to make sure the user missingprivileges@% was not created
|Number of accounts: 0|

//@ Dba: Delete created user and reconnect to previous sandbox
|Number of accounts: 0|

//@<OUT> Dba: configureLocalInstance updating config file {VER(>=8.0.3)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------+
| Variable                         | Current Value | Required Value | Note                   |
+----------------------------------+---------------+----------------+------------------------+<<<(__version_num<80021) ?  "\n| binlog_checksum                  | <not set>     | NONE           | Update the config file |\n":"">>>
| binlog_format                    | <not set>     | ROW            | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON             | Update the config file |
| gtid_mode                        | OFF           | ON             | Update the config file |
| log_slave_updates                | <not set>     | ON             | Update the config file |
| master_info_repository           | <not set>     | TABLE          | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>           | Update the config file |
| server_id                        | <not set>     | <unique ID>    | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

Do you want to perform the required configuration changes? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was configured to be used in an InnoDB cluster.


//@<OUT> Dba: configureLocalInstance updating config file {VER(<8.0.3)}
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
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

Do you want to perform the required configuration changes? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was configured to be used in an InnoDB cluster.


//@ Dba: create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 1|

//@<OUT> Dba: configureLocalInstance create different admin user
Please provide the password for 'mydba@localhost:<<<__mysql_sandbox_port2>>>': Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

ERROR: User 'mydba' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'mydba' with same grants and password
2) Create a new admin account for InnoDB cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]: Please provide an account name (e.g: icroot@%) to have it created with the necessary
privileges or leave empty and press Enter to cancel.
Account Name: Password for new account: Confirm password:
The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.

Cluster admin user 'dba_test'@'%' created.

//@<OUT> Dba: configureLocalInstance create existing valid admin user
Please provide the password for 'mydba@localhost:<<<__mysql_sandbox_port2>>>': Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>

ERROR: User 'mydba' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'mydba' with same grants and password
2) Create a new admin account for InnoDB cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]: Please provide an account name (e.g: icroot@%) to have it created with the necessary
privileges or leave empty and press Enter to cancel.
Account Name: User 'dba_test'@'%' already exists and will not be created.

The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.

//@ Dba: remove needed privilege (REPLICATION SLAVE) from created admin user
||

//@ Dba: configureLocalInstance create existing invalid admin user
||Dba.configureLocalInstance: The account 'mydba'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ Dba: Delete previously create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 0|

//@# Check if all missing privileges are reported for user with no privileges {VER(>=8.0.0)}
|ERROR: Unable to check privileges for user 'no_privileges'@'%'. User requires SELECT privilege on mysql.* to obtain information about all roles.|
||Dba.configureLocalInstance: Unable to get roles information. (RuntimeError)

//@# Check if all missing privileges are reported for user with no privileges {VER(<8.0.0)}
|ERROR: The account 'no_privileges'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT CREATE USER, FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SELECT, SHUTDOWN, SUPER ON *.* TO 'no_privileges'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_privileges'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_privileges'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_privileges'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_privileges'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'no_privileges'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)


//@ configureLocalInstance() should fail if user does not have global GRANT OPTION {VER(>=8.0.18)}
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT BACKUP_ADMIN, CLONE_ADMIN, CREATE USER, EXECUTE, FILE, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, REPLICATION_APPLIER, SELECT, SHUTDOWN, SUPER, SYSTEM_VARIABLES_ADMIN ON *.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ configureLocalInstance() should fail if user does not have global GRANT OPTION {VER(<8.0.0)}
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT CREATE USER, FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SELECT, SHUTDOWN, SUPER ON *.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ createCluster() should fail if user does not have global GRANT OPTION {VER(>=8.0.18)}
|A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port2>>>'.|
|Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT BACKUP_ADMIN, CLONE_ADMIN, CREATE USER, EXECUTE, FILE, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, REPLICATION_APPLIER, SELECT, SHUTDOWN, SUPER, SYSTEM_VARIABLES_ADMIN ON *.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||Dba.createCluster: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ createCluster() should fail if user does not have global GRANT OPTION {VER(<8.0.0)}
|A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port2>>>'.|
|Validating instance configuration at localhost:<<<__mysql_sandbox_port2>>>...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|GRANT CREATE USER, FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SELECT, SHUTDOWN, SUPER ON *.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'no_global_grant'@'%' WITH GRANT OPTION;|
|For more information, see the online documentation.|
||Dba.createCluster: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)


//@# Dba: getCluster errors
||Dba.getCluster: Argument #1 is expected to be a string
||Dba.getCluster: Invalid number of arguments, expected 0 to 2 but got 3
||Dba.getCluster: Argument #2 is expected to be a map
||Dba.getCluster: The cluster with the name '' does not exist.
||Dba.getCluster: The cluster with the name '#' does not exist.
||Dba.getCluster: The cluster with the name 'over40chars_12345678901234567890123456789' does not exist.

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

//@ Finalization
||
