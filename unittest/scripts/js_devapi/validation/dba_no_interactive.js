//@ Session: validating members
|Session Members: 13|
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
||Cannot use multiMaster option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
||Cannot use multiMaster option if adoptFromGR is set to true. Using adoptFromGR mode will adopt the primary mode in use by the Cluster.
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
||Dba.checkInstanceConfiguration: Missing password for 'root@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.checkInstanceConfiguration: Missing password for 'sample@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.checkInstanceConfiguration: The instance 'localhost:<<<__mysql_sandbox_port1>>>' is already part of an InnoDB Cluster

//@ Dba: checkInstanceConfiguration ok1
|ok|

//@ Dba: checkInstanceConfiguration ok2
|ok|

//@ Dba: checkInstanceConfiguration ok3
|ok|

//@<OUT> Dba: checkInstanceConfiguration report with errors
{
    "config_errors": [
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "binlog_checksum",
            "required": "NONE"
        },
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "binlog_format",
            "required": "ROW"
        },
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "disabled_storage_engines",
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        },
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "enforce_gtid_consistency",
            "required": "ON"
        },
        {
            "action": "config_update",
            "current": "OFF",
            "option": "gtid_mode",
            "required": "ON"
        },
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "log_bin",
            "required": "<no value>"
        },
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "log_slave_updates",
            "required": "ON"
        },
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "master_info_repository",
            "required": "TABLE"
        },
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "relay_log_info_repository",
            "required": "TABLE"
        },
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "report_port",
            "required": "<<<__mysql_sandbox_port2>>>"
        },
        {
            "action": "config_update",
            "current": "<not set>",
            "option": "transaction_write_set_extraction",
            "required": "XXHASH64"
        }
    ],
    "errors": [],
    "restart_required": false,
    "status": "error"
}


//@# Dba: configureLocalInstance errors
||Dba.configureLocalInstance: This function only works with local instances
||Dba.configureLocalInstance: Missing password for 'root@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.configureLocalInstance: Missing password for 'sample@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.configureLocalInstance: The path to the MySQL Configuration is required to verify and fix the InnoDB Cluster settings

//@<OUT> Dba: configureLocalInstance updating config file
{
    "status": "ok"
}

//@ Dba: configureLocalInstance report fixed 1
|ok|

//@ Dba: configureLocalInstance report fixed 2
|ok|

//@ Dba: configureLocalInstance report fixed 3
|ok|

//@ Dba: Create user without all necessary privileges
|Number of accounts: 1|

//@ Dba: configureLocalInstance not enough privileges
||Dba.configureLocalInstance: Session account 'missingprivileges'@'localhost' does not have all the required privileges to execute this operation. For more information, see the online documentation.

//@ Dba: Show list of users to make sure the user missingprivileges@% was not created
|Number of accounts: 0|

//@ Dba: Delete created user and reconnect to previous sandbox
|Number of accounts: 0|

//@ Dba: create an admin user with all needed privileges
|Number of 'mydba'@'localhost' accounts: 1|

//@<OUT> Dba: configureLocalInstance create different admin user
{
    "status": "ok"
}

//@<OUT> Dba: configureLocalInstance create existing valid admin user
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
||Invalid cluster name: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.getCluster, expected 0 to 2 but got 3
||Invalid typecast: Map expected, but value is Integer
||Dba.getCluster: The cluster with the name '' does not exist.
||Dba.getCluster: The cluster with the name '#' does not exist.
||Dba.getCluster: The cluster with the name 'over40chars_12345678901234567890123456789' does not exist.


//@ Dba: getCluster
|<Cluster:devCluster>|
