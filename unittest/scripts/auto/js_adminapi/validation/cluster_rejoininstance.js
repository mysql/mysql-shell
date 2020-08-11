//@ Initialization
||

//@<OUT> rejoinInstance async replication error {VER(<8.0.22)}
ERROR: Cannot rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (source-replica) replication configured and running. Please stop the replication threads by executing the query: 'STOP SLAVE;'

//@<OUT> rejoinInstance async replication error {VER(>=8.0.22)}
ERROR: Cannot rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (source-replica) replication configured and running. Please stop the replication threads by executing the query: 'STOP REPLICA;'

//@<ERR> rejoinInstance async replication error
Cluster.rejoinInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is running asynchronous replication. (RuntimeError)

//@ Finalization
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
<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port3+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.":"">>>

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

// If the target instance is >= 8.0.22, when ipWhitelist is used a deprecation warning must be printed
//@<> IPv6 addresses are supported on rejoinInstance ipWhitelist WL#12758 {VER(>=8.0.22)}
|WARNING: The ipWhitelist option is deprecated in favor of ipAllowlist. ipAllowlist will be set instead.|

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port2>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Cluster.rejoinInstance: Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)

//@ IPv6 on ipWhitelist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
||Cluster.rejoinInstance: Invalid value for ipWhitelist '::1': IPv6 not supported (version >= 8.0.14 required for IPv6 support). (ArgumentError)

//@ Rejoin instance fails if the target instance contains errant transactions (BUG#29953812) {VER(>=8.0.17)}
|ERROR: A GTID set check of the MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' determined that it contains transactions that do not originate from the cluster, which must be discarded before it can join the cluster.|
||
|<<<hostname>>>:<<<__mysql_sandbox_port2>>> has the following errant GTIDs that do not exist in the cluster:|
|00025721-1111-1111-1111-111111111111:1|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
||
|Discarding these extra GTID events can either be done manually or by completely|
||Cluster.rejoinInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' contains errant transactions that did not originate from the cluster. (RuntimeError)

//@ Rejoin instance fails if the target instance contains errant transactions 5.7 (BUG#29953812) {VER(<8.0.17)}
|ERROR: A GTID set check of the MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'|
||
|<<<hostname>>>:<<<__mysql_sandbox_port2>>> has the following errant GTIDs that do not exist in the cluster:|
|00025721-1111-1111-1111-111111111111:1|
||
|Having extra GTID events is not expected, and it is recommended to investigate|
||Cluster.rejoinInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' contains errant transactions that did not originate from the cluster. (RuntimeError)

//@ Rejoin instance fails if the target instance has an empty gtid-set (BUG#29953812)
|ERROR: The target instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has an empty GTID set so it cannot|
||Cluster.rejoinInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has an empty GTID set. (RuntimeError)

//@ Rejoin instance fails if the transactions were purged from the cluster (BUG#29953812)
|ERROR: A GTID set check of the MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' determined that it is missing transactions that were purged from all cluster members.|
||Cluster.rejoinInstance: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is missing transactions that were purged from all cluster members. (RuntimeError)
