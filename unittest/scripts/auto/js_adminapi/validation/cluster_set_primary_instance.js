//@<OUT> WL#12052: Create single-primary cluster
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@<ERR> WL#12052: Error when executing setPrimaryInstance on a cluster with members < 8.0.13 {VER(<8.0.13)}
Cluster.setPrimaryInstance: Operation not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ WL#12052: ArgumentErrors of setPrimaryInstance {VER(>=8.0.13)}
||Argument #1: Invalid URI: empty. (ArgumentError)
||Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
||Argument #1: Invalid connection options, no options provided. (ArgumentError)
||The instance 'localhost:3355' does not belong to the cluster: 'cluster'. (RuntimeError)

//@ WL#12052: Error when executing setPrimaryInstance on a cluster with 1 or more members not ONLINE
||This operation requires all the cluster members to be ONLINE (RuntimeError)

//@<ERR> WL#12052: Error when executing setPrimaryInstance on a cluster with no visible quorum < 8.0.13 {VER(>=8.0.13)}
Cluster.setPrimaryInstance: There is no quorum to perform the operation (MYSQLSH 51011)

//@ WL#12052: Re-create the cluster but in multi-primary mode {VER(>=8.0.13)}
||

//@<ERR> WL#12052: Error when executing setPrimaryInstance on a multi-primary cluster {VER(>=8.0.13)}
Cluster.setPrimaryInstance: Operation not allowed: The cluster is in Multi-Primary mode. (RuntimeError)

//@ Re-create the cluster {VER(>=8.0.13)}
||

//@<OUT> WL#12052: Set new primary {VER(>=8.0.13)}
Setting instance 'localhost:<<<__mysql_sandbox_port2>>>' as the primary instance of cluster 'cluster'...

Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was switched from PRIMARY to SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was switched from SECONDARY to PRIMARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' remains SECONDARY.

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully elected as primary.

//@<OUT> WL#12052: Set new primary 2 {VER(>=8.0.13)}
Setting instance 'localhost:<<<__mysql_sandbox_port3>>>' as the primary instance of cluster 'cluster'...

Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' remains SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was switched from PRIMARY to SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was switched from SECONDARY to PRIMARY.

The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully elected as primary.

//@<OUT> WL#12052: Cluster status after setting a new primary {VER(>=8.0.13)}
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>"
}

//@ Finalization
||
