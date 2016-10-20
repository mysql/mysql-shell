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
|checkInstanceConfig: OK|
|stopSandboxInstance: OK|
|dropMetadataSchema: OK|
|configLocalInstance: OK|
|verbose: OK|

//@# Dba: createCluster errors
||ArgumentError: Invalid number of arguments in Dba.createCluster, expected 1 to 2 but got 0
||Argument #1 is expected to be a string
||Dba.createCluster: The Cluster name cannot be empty
||Argument #2 is expected to be a map
||Invalid values in the options: another, invalid

//@# Dba: createCluster succeed
|<Cluster:devCluster>|

//@# Dba: createCluster already exist
||Dba.createCluster: Cluster is already initialized. Use Dba.getCluster() to access it.

//@# Dba: checkInstanceConfig errors
||Dba.checkInstanceConfig: Missing password for 'root@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.checkInstanceConfig: Missing password for 'sample@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.checkInstanceConfig: The instance 'root@localhost:<<<__mysql_sandbox_port1>>>' is already part of an InnoDB Cluster

//@ Dba: checkInstanceConfig ok1
|ok|

//@ Dba: checkInstanceConfig ok2
|ok|

//@ Dba: checkInstanceConfig ok3
|ok|

//@<OUT> Dba: checkInstanceConfig report with errors
{
    "config_errors": [
        {
            "action": "config_update", 
            "current": "<no value>", 
            "option": "binlog_checksum", 
            "required": "NONE"
        },
        {
            "action": "config_update", 
            "current": "<no value>", 
            "option": "binlog_format", 
            "required": "ROW"
        },
        {
            "action": "config_update", 
            "current": "<no value>", 
            "option": "disabled_storage_engines", 
            "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"
        },
        {
            "action": "config_update", 
            "current": "<no value>", 
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
            "current": "<no value>", 
            "option": "log_slave_updates", 
            "required": "ON"
        },
        {
            "action": "config_update", 
            "current": "<no value>", 
            "option": "master_info_repository", 
            "required": "TABLE"
        },
        {
            "action": "config_update", 
            "current": "<no value>", 
            "option": "relay_log_info_repository", 
            "required": "TABLE"
        },
        {
            "action": "config_update", 
            "current": "<no value>", 
            "option": "report_port", 
            "required": "<<<__mysql_sandbox_port2>>>"
        },
        {
            "action": "config_update", 
            "current": "<no value>", 
            "option": "transaction_write_set_extraction", 
            "required": "XXHASH64"
        }
    ], 
    "errors": [
    ], 
    "restart_required": false, 
    "status": "error"
}

//@# Dba: configLocalInstance errors
||Dba.configLocalInstance: This function only works with local instances
||Dba.configLocalInstance: Missing password for 'root@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.configLocalInstance: Missing password for 'sample@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.configLocalInstance: The path to the MySQL Configuration is required to verify and fix the InnoDB Cluster settings
||Dba.configLocalInstance: The instance 'root@localhost:<<<__mysql_sandbox_port1>>>' is already part of an InnoDB Cluster

//@<OUT> Dba: configLocalInstance updating config file
{
    "status": "ok"
}

//@<OUT> Dba: configLocalInstance report fixed 1
{
    "status": "ok"
}

//@<OUT> Dba: configLocalInstance report fixed 2
{
    "status": "ok"
}
//@<OUT> Dba: configLocalInstance report fixed 3
{
    "status": "ok"
}


//@# Dba: getCluster errors
||Invalid cluster name: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.getCluster, expected 0 to 1 but got 2
||Dba.getCluster: The Cluster name cannot be empty

//@ Dba: getCluster
|<Cluster:devCluster>|
