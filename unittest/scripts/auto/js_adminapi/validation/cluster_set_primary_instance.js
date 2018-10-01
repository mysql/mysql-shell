//@<OUT> WL#12052: Create single-primary cluster
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@<ERR> WL#12052: Error when executing setPrimaryInstance on a cluster with members < 8.0.13 {VER(<8.0.13)}
Cluster.setPrimaryInstance: Operation not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ WL#12052: ArgumentErrors of setPrimaryInstance {VER(>=8.0.13)}
||Invalid URI: empty. (ArgumentError)
||Argument #1 is expected to be a string (ArgumentError)
||Invalid connection options, no options provided. (ArgumentError)
||The instance 'localhost:3355' does not belong to the ReplicaSet: 'default'. (RuntimeError)

//@ WL#12052: Error when executing setPrimaryInstance on a cluster with 1 or more members not ONLINE < 8.0.13 {VER(>=8.0.13)}
|ERROR: The instance 'localhost:<<<__mysql_sandbox_port3>>>' has the status: '(MISSING)'. All members must be ONLINE.|One or more instances of the cluster are not ONLINE. (RuntimeError)

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

Instance 'localhost:<<<__mysql_sandbox_port1>>>' was switched from PRIMARY to SECONDARY.
Instance 'localhost:<<<__mysql_sandbox_port2>>>' was switched from SECONDARY to PRIMARY.
Instance 'localhost:<<<__mysql_sandbox_port3>>>' remains SECONDARY.

WARNING: The cluster internal session is not the primary member anymore. For cluster management operations please obtain a fresh cluster handle using <Dba>.getCluster().

The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully elected as primary.

//@<OUT> WL#12052: Set new primary 2 {VER(>=8.0.13)}
Setting instance 'localhost:<<<__mysql_sandbox_port3>>>' as the primary instance of cluster 'cluster'...

Instance 'localhost:<<<__mysql_sandbox_port1>>>' remains SECONDARY.
Instance 'localhost:<<<__mysql_sandbox_port2>>>' was switched from PRIMARY to SECONDARY.
Instance 'localhost:<<<__mysql_sandbox_port3>>>' was switched from SECONDARY to PRIMARY.

WARNING: The cluster internal session is not the primary member anymore. For cluster management operations please obtain a fresh cluster handle using <Dba>.getCluster().

The instance 'localhost:<<<__mysql_sandbox_port3>>>' was successfully elected as primary.

//@<OUT> WL#12052: Cluster status after setting a new primary {VER(>=8.0.13)}
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port3>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:<<<__mysql_sandbox_port3>>>": {
                "address": "localhost:<<<__mysql_sandbox_port3>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL#12052: Finalization
||
