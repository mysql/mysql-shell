#@ Session: validating members
|Session Members: 14|
|create_cluster: OK|
|delete_sandbox_instance: OK|
|deploy_sandbox_instance: OK|
|get_cluster: OK|
|help: OK|
|kill_sandbox_instance: OK|
|reset_session: OK|
|start_sandbox_instance: OK|
|check_instance_configuration: OK|
|stop_sandbox_instance: OK|
|drop_metadata_schema: OK|
|configure_local_instance: OK|
|verbose: OK|
|reboot_cluster_from_complete_outage: OK|

#@# Dba: create_cluster errors
||ArgumentError: Invalid number of arguments in Dba.create_cluster, expected 1 to 2 but got 0
||Argument #1 is expected to be a string
||Dba.create_cluster: The Cluster name cannot be empty
||Argument #2 is expected to be a map
||Invalid values in the options: another, invalid
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Invalid value for memberSslMode option. Supported values: AUTO,DISABLED,REQUIRED.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Cannot use memberSslMode option if adoptFromGR is set to true.
||Invalid value for ipWhitelist, string value cannot be empty.

#@# Dba: create_cluster succeed
|<Cluster:devCluster>|

#@# Dba: create_cluster already exist
||Dba.create_cluster: Unable to create cluster. The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' already belongs to an InnoDB cluster. Use <Dba>.get_cluster() to access it.

#@# Dba: check_instance_configuration errors
||Dba.check_instance_configuration: Missing password for 'root@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.check_instance_configuration: Missing password for 'sample@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.check_instance_configuration: The instance 'root@localhost:<<<__mysql_sandbox_port1>>>' is already part of an InnoDB Cluster

#@ Dba: check_instance_configuration ok1
|ok|

#@ Dba: check_instance_configuration ok2
|ok|

#@ Dba: check_instance_configuration ok3
|ok|

#@<OUT> Dba: check_instance_configuration report with errors
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


#@# Dba: configure_local_instance errors
||Dba.configure_local_instance: This function only works with local instances
||Dba.configure_local_instance: Missing password for 'root@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.configure_local_instance: Missing password for 'sample@localhost:<<<__mysql_sandbox_port1>>>'
||Dba.configure_local_instance: The path to the MySQL Configuration is required to verify and fix the InnoDB Cluster settings

#@<OUT> Dba: configure_local_instance updating config file
{
    "status": "ok"
}

#@<OUT> Dba: configure_local_instance report fixed 1
{
    "status": "ok"
}

#@<OUT> Dba: configure_local_instance report fixed 2
{
    "status": "ok"
}

#@<OUT> Dba: configure_local_instance report fixed 3
{
    "status": "osk"
}

#@# Dba: get_cluster errors
||Invalid cluster name: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.get_cluster, expected 0 to 1 but got 2
||Dba.get_cluster: The Cluster name cannot be empty

#@ Dba: get_cluster
|<Cluster:devCluster>|
