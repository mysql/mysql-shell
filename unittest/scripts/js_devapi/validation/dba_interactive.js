//@ Initialization
||

//@ Session: validating members
|Session Members: 14|
|createCluster: OK|
|deleteSandboxInstance: OK|
|deploySandboxInstance: OK|
|dropMetadataSchema: OK|
|getCluster: OK|
|help: OK|
|killSandboxInstance: OK|
|resetSession: OK|
|startSandboxInstance: OK|
|checkInstanceConfiguration: OK|
|stopSandboxInstance: OK|
|configureLocalInstance: OK|
|verbose: OK|
|rebootClusterFromCompleteOutage: OK|

//@# Dba: createCluster errors
||Invalid number of arguments in Dba.createCluster, expected 1 to 2 but got 0
||Invalid number of arguments in Dba.createCluster, expected 1 to 2 but got 4
||Dba.createCluster: Argument #1 is expected to be a string
||Dba.createCluster: The Cluster name cannot be empty
||Dba.createCluster: Invalid values in the options: another, invalid
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use multiMaster option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiMaster option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Invalid value for ipWhitelist, string value cannot be empty.
||Dba.createCluster: The Cluster name can only start with an alphabetic or the '_' character.

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

//@<OUT> Dba: createCluster with interaction
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ Dba: checkInstanceConfiguration error
|Please provide the password for 'root@localhost:<<<__mysql_sandbox_port1>>>':|Dba.checkInstanceConfiguration: The instance 'root@localhost:<<<__mysql_sandbox_port1>>>' is already part of an InnoDB Cluster

//@<OUT> Dba: checkInstanceConfiguration ok 1
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for Cluster usage
{
    "status": "ok"
}

//@<OUT> Dba: checkInstanceConfiguration ok 2
Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for Cluster usage
{
    "status": "ok"
}


//@<OUT> Dba: checkInstanceConfiguration report with errors
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is not valid for Cluster usage.

The following issues were encountered:

 - Some configuration options need to be fixed.

+----------------------------------+---------------+----------------------------------------+------------------------+
| Variable                         | Current Value | Required Value                         | Note                   |
+----------------------------------+---------------+----------------------------------------+------------------------+
| binlog_checksum                  | <not set>     | NONE                                   | Update the config file |
| binlog_format                    | <not set>     | ROW                                    | Update the config file |
| disabled_storage_engines         | <not set>     | MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE | Update the config file |
| enforce_gtid_consistency         | <not set>     | ON                                     | Update the config file |
| gtid_mode                        | OFF           | ON                                     | Update the config file |
| log_bin                          | <not set>     | <no value>                             | Update the config file |
| log_slave_updates                | <not set>     | ON                                     | Update the config file |
| master_info_repository           | <not set>     | TABLE                                  | Update the config file |
| relay_log_info_repository        | <not set>     | TABLE                                  | Update the config file |
| report_port                      | <not set>     | <<<__mysql_sandbox_port2>>>                                   | Update the config file |
| transaction_write_set_extraction | <not set>     | XXHASH64                               | Update the config file |
+----------------------------------+---------------+----------------------------------------+------------------------+


Please fix these issues and try again.

//@ Dba: configureLocalInstance error 1
||Dba.configureLocalInstance: This function only works with local instances

// TODO(rennox): This test case is not reliable since requires
// that no my.cnf exist on the default paths
//--@<OUT> Dba: configureLocalInstance error 2
// Please provide the password for 'root@localhost:<<<__mysql_port>>>':
// Detecting the configuration file...
// Default file not found at the standard locations.
// Please specify the path to the MySQL configuration file:
// The path to the MySQL Configuration is required to verify and fix the InnoDB Cluster settings

//@<OUT> Dba: configureLocalInstance error 3
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port1>>>':
Detected as sandbox instance.

Validating MySQL configuration file at: <<<__output_sandbox_dir>>><<<__mysql_sandbox_port1>>><<<__path_splitter>>>my.cnf
Validating instance...

//@ Dba: Create user without all necessary privileges
|Number of accounts: 1|

//@ Dba: configureLocalInstance not enough privileges 1
||Dba.configureLocalInstance: Session account 'missingprivileges'@'localhost' does not have all the required privileges to execute this operation. For more information, see the online documentation.

//@ Dba: configureLocalInstance not enough privileges 2
||Dba.configureLocalInstance: Session account 'missingprivileges'@'localhost' does not have all the required privileges to execute this operation. For more information, see the online documentation.

//@ Dba: configureLocalInstance not enough privileges 3
||Dba.configureLocalInstance: Session account 'missingprivileges'@'localhost' does not have all the required privileges to execute this operation. For more information, see the online documentation.

//@ Dba: Show list of users to make sure the user missingprivileges@% was not created
|Number of accounts: 0|

//@ Dba: Delete created user and reconnect to previous sandbox
|Number of accounts: 0|

//@<OUT> Dba: configureLocalInstance updating config file
Please provide the password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for Cluster usage
You can now use it in an InnoDB Cluster.

//@ Dba: create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 1|

//@<OUT> Dba: configureLocalInstance create different admin user
Please provide the password for 'mydba@localhost:<<<__mysql_sandbox_port2>>>':
Detected as sandbox instance.

Validating MySQL configuration file at: <<<__output_sandbox_dir>>><<<__mysql_sandbox_port2>>><<<__path_splitter>>>my.cnf
MySQL user 'mydba' cannot be verified to have access to other hosts in the network.

1) Create mydba@% with necessary grants
2) Create account with different name
3) Continue without creating account
4) Cancel
Please select an option [1]: Please provide an account name (e.g: icroot@%) to have it created with the necessary
privileges or leave empty and press Enter to cancel.
Account Name: Password for new account: Confirm password: Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for Cluster usage
You can now use it in an InnoDB Cluster.

{
    "status": "ok"
}

//@<OUT> Dba: configureLocalInstance create existing valid admin user
Please provide the password for 'mydba@localhost:<<<__mysql_sandbox_port2>>>':
Detected as sandbox instance.

Validating MySQL configuration file at: <<<__output_sandbox_dir>>><<<__mysql_sandbox_port2>>><<<__path_splitter>>>my.cnf
MySQL user 'mydba' cannot be verified to have access to other hosts in the network.

1) Create mydba@% with necessary grants
2) Create account with different name
3) Continue without creating account
4) Cancel
Please select an option [1]: Please provide an account name (e.g: icroot@%) to have it created with the necessary
privileges or leave empty and press Enter to cancel.
Account Name: Password for new account: Confirm password: Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for Cluster usage
You can now use it in an InnoDB Cluster.

{
    "status": "ok"
}

//@ Dba: remove needed privilege (REPLICATION SLAVE) from created admin user
||

//@ Dba: configureLocalInstance create existing invalid admin user
||Dba.configureLocalInstance: Cluster Admin account 'dba_test'@'%' does not have all the required privileges to execute this operation. For more information, see the online documentation.

//@ Dba: Delete previously create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 0|

//@# Dba: getCluster errors
||Dba.getCluster: Invalid cluster name: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.getCluster, expected 0 to 1 but got 2
||Dba.getCluster: The Cluster name cannot be empty.
||Dba.getCluster: The Cluster name can only start with an alphabetic or the '_' character.
||Dba.getCluster: The Cluster name can not be greater than 40 characters.

//@<OUT> Dba: getCluster with interaction
<Cluster:devCluster>

//@<OUT> Dba: getCluster with interaction (default)
<Cluster:devCluster>

//@ Finalization
||
