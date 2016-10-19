//@ Session: validating members
|Session Members: 13|
|createCluster: OK|
|deleteSandboxInstance: OK|
|deploySandboxInstance: OK|
|getCluster: OK|
|help: OK|
|killSandboxInstance: OK|
|resetSession: OK|
|startSandboxInstance: OK|
|validateInstance: OK|
|stopSandboxInstance: OK|
|dropMetadataSchema: OK|
|configureLocalInstance: OK|
|verbose: OK|

//@# Dba: createCluster errors
||Invalid number of arguments in Dba.createCluster, expected 1 to 2 but got 0
||Invalid number of arguments in Dba.createCluster, expected 1 to 2 but got 4
||Dba.createCluster: Argument #1 is expected to be a string
||Dba.createCluster: The Cluster name cannot be empty
||Dba.createCluster: Invalid values in the options: another, invalid

//@<OUT> Dba: createCluster with interaction
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ Dba: validateInstance error
|Please provide a password for 'root@localhost:<<<__mysql_sandbox_port1>>>':|Dba.validateInstance: The instance 'root@localhost:<<<__mysql_sandbox_port1>>>' is already part of an InnoDB Cluster

//@<OUT> Dba: validateInstance ok 1
Please provide a password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for Cluster usage
{
    "status": "ok"
}

//@<OUT> Dba: validateInstance ok 2
Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for Cluster usage
{
    "status": "ok"
}


//@<OUT> Dba: validateInstance report with errors
Please provide a password for 'root@localhost:<<<__mysql_sandbox_port1>>>': Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port1>>>' is not valid for Cluster usage.

The following issues were encountered:

 - Some configuration options need to be fixed.

+----------------------------------+---------------+----------------------------------------+------------------------+
| Variable                         | Current Value | Required Value                         | Note                   |
+----------------------------------+---------------+----------------------------------------+------------------------+
| binlog_checksum                  | <no value>    | NONE                                   | Update the config file |
| binlog_format                    | <no value>    | ROW                                    | Update the config file |
| disabled_storage_engines         | <no value>    | MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE | Update the config file |
| enforce_gtid_consistency         | <no value>    | ON                                     | Update the config file |
| gtid_mode                        | OFF           | ON                                     | Update the config file |
| log_slave_updates                | <no value>    | ON                                     | Update the config file |
| master_info_repository           | <no value>    | TABLE                                  | Update the config file |
| relay_log_info_repository        | <no value>    | TABLE                                  | Update the config file |
| report_port                      | <no value>    | <<<__mysql_sandbox_port1>>>                                   | Update the config file |
| transaction_write_set_extraction | <no value>    | XXHASH64                               | Update the config file |
+----------------------------------+---------------+----------------------------------------+------------------------+


Please fix these issues and try again.

//@ Dba: configureLocalInstance error 1
||Dba.configureLocalInstance: This function only works with local instances

//@<OUT> Dba: configureLocalInstance error 2
Please provide a password for 'root@localhost:<<<__mysql_port>>>': Please specify the path to the MySQL configuration file: 
The path to the MySQL Configuration is required to verify and fix the InnoDB Cluster settings

//@<OUT> Dba: configureLocalInstance error 3
Please provide a password for 'root@localhost:<<<__mysql_sandbox_port1>>>': 
Detected as sandbox instance.

Validating MySQL configuration file at: /home/rennox/mysql-sandboxes/<<<__mysql_sandbox_port1>>>/my.cnf
Validating instance...

//@<ERR> Dba: configureLocalInstance error 3
RuntimeError: Dba.configureLocalInstance: The instance 'root@localhost:<<<__mysql_sandbox_port1>>>' is already part of an InnoDB Cluster

//@<OUT> Dba: configureLocalInstance updating config file
Please provide a password for 'root@localhost:<<<__mysql_sandbox_port2>>>': Validating instance...

The instance 'localhost:<<<__mysql_sandbox_port2>>>' is valid for Cluster usage
You can now add it to an InnoDB Cluster with the <Cluster>.addInstance() function.

//@# Dba: getCluster errors
||ArgumentError: Dba.getCluster: Invalid cluster name: Argument #1 is expected to be a string
||Dba.getCluster: The Cluster name cannot be empty

//@<OUT> Dba: getCluster with interaction
<Cluster:devCluster>

//@<OUT> Dba: getCluster with interaction (default)
<Cluster:devCluster>
