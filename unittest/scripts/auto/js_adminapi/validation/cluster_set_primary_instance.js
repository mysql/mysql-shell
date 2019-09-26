//@<OUT> WL#12052: Create single-primary cluster
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@<ERR> WL#12052: Error when executing setPrimaryInstance on a cluster with members < 8.0.13 {VER(<8.0.13)}
Cluster.setPrimaryInstance: Operation not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ WL#12052: ArgumentErrors of setPrimaryInstance {VER(>=8.0.13)}
||Invalid URI: empty. (ArgumentError)
||Argument #1 is expected to be a string (ArgumentError)
||Invalid connection options, no options provided. (ArgumentError)
||The instance 'localhost:3355' does not belong to the ReplicaSet: 'default'. (RuntimeError)

//@ WL#12052: Error when executing setPrimaryInstance on a cluster with 1 or more members not ONLINE
||Cluster.setPrimaryInstance: This operation requires all the cluster members to be ONLINE (RuntimeError)

//@<ERR> WL#12052: Error when executing setPrimaryInstance on a cluster with no visible quorum < 8.0.13 {VER(>=8.0.13)}
Cluster.setPrimaryInstance: There is no quorum to perform the operation (RuntimeError)

//@ WL#12052: Re-create the cluster but in multi-primary mode {VER(>=8.0.13)}
||

//@<ERR> WL#12052: Error when executing setPrimaryInstance on a multi-primary cluster {VER(>=8.0.13)}
Cluster.setPrimaryInstance: Operation not allowed: The cluster is in Multi-Primary mode. (RuntimeError)

//@ WL#12052: Re-create the cluster {VER(>=8.0.13)}
||

//@<OUT> WL#12052: Set new primary {VER(>=8.0.13)}
Setting instance 'localhost:<<<__mysql_sandbox_port2>>>' as the primary instance of cluster 'cluster'...

Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was switched from PRIMARY to SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was switched from SECONDARY to PRIMARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' remains SECONDARY.

WARNING: The cluster internal session is not the primary member anymore. For cluster management operations please obtain a fresh cluster handle using <Dba>.getCluster().

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully elected as primary.

//@<OUT> WL#12052: Set new primary 2 {VER(>=8.0.13)}
Setting instance 'localhost:<<<__mysql_sandbox_port3>>>' as the primary instance of cluster 'cluster'...

Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' remains SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was switched from PRIMARY to SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was switched from SECONDARY to PRIMARY.

WARNING: The cluster internal session is not the primary member anymore. For cluster management operations please obtain a fresh cluster handle using <Dba>.getCluster().

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
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL#12052: Finalization
||
