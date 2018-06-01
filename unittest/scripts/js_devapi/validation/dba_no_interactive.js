//@ Session: validating members
|Session Members: 14|
|createCluster: OK|
|deleteSandboxInstance: OK|
|deploySandboxInstance: OK|
|dropMetadataSchema: OK|
|getCluster: OK|
|help: OK|
|killSandboxInstance: OK|
|startSandboxInstance: OK|
|checkInstanceConfiguration: OK|
|stopSandboxInstance: OK|
|configureInstance: OK|
|configureLocalInstance: OK|
|verbose: OK|
|rebootClusterFromCompleteOutage: OK|

//@# Dba: createCluster errors
||Invalid number of arguments in Dba.createCluster, expected 1 to 2 but got 0
||Argument #1 is expected to be a string
||Dba.createCluster: The Cluster name cannot be empty
||Argument #2 is expected to be a map
||Invalid values in the options: another, invalid
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use multiPrimary option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiPrimary option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiMaster option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiMaster option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use the multiMaster and multiPrimary options simultaneously. The multiMaster option is deprecated, please use the multiPrimary option instead.
||Cannot use the multiMaster and multiPrimary options simultaneously. The multiMaster option is deprecated, please use the multiPrimary option instead.
||Invalid value for ipWhitelist, string value cannot be empty.

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
||Dba.createCluster: Unable to create cluster. The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' already belongs to an InnoDB cluster. Use <Dba>.getCluster() to access it.

//@# Dba: checkInstanceConfiguration errors
||Access denied for user 'root'@'<<<localhost>>>' (using password: NO) (MySQL Error 1045)
||Access denied for user 'sample'@'<<<localhost>>>' (using password: NO) (MySQL Error 1045)
||Dba.checkInstanceConfiguration: This function is not available through a session to an instance already in an InnoDB cluster (RuntimeError)

//@ Dba: checkInstanceConfiguration ok1
|The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.|

//@ Dba: checkInstanceConfiguration ok2
|The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.|

//@<OUT> Dba: checkInstanceConfiguration report with errors
Validating local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<real_hostname>>>
Clients and other cluster members will communicate with it through this address by default. If this is not correct, the report_host MySQL system variable should be changed.

Checking whether existing tables comply with Group Replication requirements...
No incompatible tables detected

Checking instance configuration...
Configuration file mybad.cnf will also be checked.

Some configuration options need to be fixed:
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
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

The following variable needs to be changed, but cannot be done dynamically: 'log_bin'
Please use the dba.configureInstance() command to repair these issues.

//@# Dba: configureLocalInstance errors
//||Dba.configureLocalInstance: This function only works with local instances
||Access denied for user 'root'@'<<<localhost>>>' (using password: NO) (MySQL Error 1045)
||Access denied for user 'sample'@'<<<localhost>>>' (using password: NO) (MySQL Error 1045)

//@# Dba: configureLocalInstance errors 5.7 {VER(<8.0.11)}
|ERROR: The path to the MySQL configuration file is required to verify and fix InnoDB cluster related options.|
||Dba.configureLocalInstance: Unable to update MySQL configuration file.

//@# Dba: configureLocalInstance errors 8.0 {VER(>=8.0.11)}
|The instance 'localhost:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB cluster.|
|Calling this function on a cluster member is only required for MySQL versions 8.0.4 or earlier.|

//@<OUT> Dba: configureLocalInstance updating config file
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<real_hostname>>>
Clients and other cluster members will communicate with it through this address by default. If this is not correct, the report_host MySQL system variable should be changed.

WARNING: User 'root' can only connect from localhost.
If you need to manage this instance while connected from other hosts, new account(s) with the proper source address specification must be created.

Some configuration options need to be fixed:
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
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file |
+----------------------------------+---------------+----------------+------------------------+

The following variable needs to be changed, but cannot be done dynamically: 'log_bin'
Configuring instance...
The instance 'localhost:<<<__mysql_sandbox_port2>>>' was configured for use in an InnoDB cluster.

//@ Dba: configureLocalInstance report fixed 1
|The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.|

//@ Dba: configureLocalInstance report fixed 2
|The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.|

//@ Dba: Create user without all necessary privileges
|Number of accounts: 1|

//@# Dba: configureLocalInstance not enough privileges
|ERROR: The account 'missingprivileges'@'<<<localhost>>>' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: FILE, GRANT OPTION, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SHUTDOWN.|
|Missing privileges on schema 'mysql': DELETE, INSERT, SELECT, UPDATE.|
|Missing privileges on schema 'mysql_innodb_cluster_metadata': ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SELECT, SHOW VIEW, TRIGGER, UPDATE.|
|Missing privileges on schema 'sys': SELECT.|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'missingprivileges'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ Dba: Show list of users to make sure the user missingprivileges@% was not created
|Number of accounts: 0|

//@ Dba: Delete created user and reconnect to previous sandbox
|Number of accounts: 0|

//@ Dba: create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 1|

//@<OUT> Dba: configureLocalInstance create different admin user
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<real_hostname>>>
Clients and other cluster members will communicate with it through this address by default. If this is not correct, the report_host MySQL system variable should be changed.
Assuming full account name 'dba_test'@'%' for dba_test

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.
Cluster admin user 'dba_test'@'%' created.

//@<OUT> Dba: configureLocalInstance create existing valid admin user
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...
Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<real_hostname>>>
Clients and other cluster members will communicate with it through this address by default. If this is not correct, the report_host MySQL system variable should be changed.
Assuming full account name 'dba_test'@'%' for dba_test
User 'dba_test'@'%' already exists and will not be created.

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for InnoDB cluster usage.

//@ Dba: remove needed privilege (REPLICATION SLAVE) from created admin user
||

//@# Dba: configureLocalInstance create existing invalid admin user
|WARNING: User 'dba_test'@'%' already exists and will not be created. However, it is missing privileges.|
|The account 'dba_test'@'%' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: REPLICATION SLAVE.|
||Dba.configureLocalInstance: The account 'mydba'@'localhost' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ Dba: Delete previously create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 0|

//@ configureLocalInstance() should fail if user does not have global GRANT OPTION
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: GRANT OPTION.|
|For more information, see the online documentation.|
||Dba.configureLocalInstance: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@ createCluster() should fail if user does not have global GRANT OPTION
|Validating instance at localhost:<<<__mysql_sandbox_port2>>>...|
|ERROR: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster:|
|Missing global privileges: GRANT OPTION.|
|For more information, see the online documentation.|
||Dba.createCluster: The account 'no_global_grant'@'%' is missing privileges required to manage an InnoDB cluster. (RuntimeError)

//@# Dba: getCluster errors
||Invalid cluster name: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.getCluster, expected 0 to 2 but got 3
||Invalid typecast: Map expected, but value is Integer
||Dba.getCluster: The cluster with the name '' does not exist.
||Dba.getCluster: The cluster with the name '#' does not exist.
||Dba.getCluster: The cluster with the name 'over40chars_12345678901234567890123456789' does not exist.


//@ Dba: getCluster
|<Cluster:devCluster>|
