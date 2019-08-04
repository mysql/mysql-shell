//@ BUG#29305551: Initialization
||

//@<OUT> rejoinInstance async replication error
ERROR: Cannot rejoin instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (master-slave) replication configured and running. Please stop the slave threads by executing the query: 'STOP SLAVE;'

//@<ERR> rejoinInstance async replication error
Cluster.rejoinInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' is running asynchronous (master-slave) replication. (RuntimeError)

//@ BUG#29305551: Finalization
||

//@ BUG#29754915: deploy sandboxes.
||

//@ BUG#29754915: create cluster.
||

//@ BUG#29754915: keep instance 2 in RECOVERING state by setting a wrong recovery user.
||

//@ BUG#29754915: stop Group Replication on instance 3.
||

//@ BUG#29754915: get cluster to try to rejoin instance 3.
||

//@<OUT> BUG#29754915: rejoin instance 3 successfully.
<<<(__version_num==80016)?"NOTE: Unable to determine the Group Replication protocol version, while verifying if a protocol upgrade would be possible: Can't initialize function 'group_replication_get_communication_protocol'; A member is joining the group, wait for it to be ONLINE.":"">>>
<<<(__version_num<80011)?"WARNING: Instance 'localhost:"+__mysql_sandbox_port3+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":"">>>ONLINE

//@<OUT> BUG#29754915: confirm cluster status.
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "recovery": {
                    "receiverError": "error connecting to master 'not_exist@<<<hostname>>>:[[*]]",
                    "receiverErrorNumber": 1045,
                    "state": "CONNECTION_ERROR"
                },
                "recoveryStatusText": "Distributed recovery in progress",
                "role": "HA",
                "status": "RECOVERING"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}


//@ BUG#29754915: clean-up.
||

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance 'localhost:<<<__mysql_sandbox_port2>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Cluster.rejoinInstance: Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)

//@ IPv6 on ipWhitelist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
||Cluster.rejoinInstance: Invalid value for ipWhitelist '::1': IPv6 not supported (version >= 8.0.14 required for IPv6 support). (ArgumentError)
