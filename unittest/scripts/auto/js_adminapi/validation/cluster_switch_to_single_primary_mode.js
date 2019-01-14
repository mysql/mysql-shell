//@<OUT> WL#12052: Create multi-primary cluster
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@<ERR> WL#12052: Error when executing switchToSinglePrimaryMode on a cluster with members < 8.0.13 {VER(<8.0.13)}
Cluster.switchToSinglePrimaryMode: Operation not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ WL#12052: ArgumentErrors of switchToSinglePrimaryMode {VER(>=8.0.13)}
||Invalid URI: empty. (ArgumentError)
||Argument #1 is expected to be a string (ArgumentError)
||Invalid connection options, no options provided. (ArgumentError)

//@ WL#12052: Error when executing switchToSinglePrimaryMode on a cluster with 1 or more members not ONLINE < 8.0.13 {VER(>=8.0.13)}
|ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' has the status: '(MISSING)'. All members must be ONLINE.|One or more instances of the cluster are not ONLINE. (RuntimeError)

//@<ERR> WL#12052: Error when executing switchToSinglePrimaryMode on a cluster with no visible quorum < 8.0.13 {VER(>=8.0.13)}
Cluster.switchToSinglePrimaryMode: There is no quorum to perform the operation (RuntimeError)

//@ WL#12052: Re-create the cluster {VER(>=8.0.13)}
||

//@<OUT> WL#12052: Switch to single-primary mode {VER(>=8.0.13)}
Switching cluster 'cluster' to Single-Primary mode...

Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' remains PRIMARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was switched from PRIMARY to SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was switched from PRIMARY to SECONDARY.

WARNING: Existing connections that expected a R/W connection must be disconnected, i.e. instances that became SECONDARY.

The cluster successfully switched to Single-Primary mode.

//@<OUT> WL#12052: Check cluster status after changing to single-primary {VER(>=8.0.13)}
{
    "clusterName": "cluster",
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
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL#12052: Verify the values of auto_increment_% in the seed instance {VER(>=8.0.13)}
auto_increment_increment = 1
auto_increment_offset = 2

//@<OUT> WL#12052: Verify the values of auto_increment_% in the member2 {VER(>=8.0.13)}
auto_increment_increment = 1
auto_increment_offset = 2

//@<OUT> WL#12052: Verify the values of auto_increment_% in the member3 {VER(>=8.0.13)}
auto_increment_increment = 1
auto_increment_offset = 2

//@<OUT> WL#12052: Verify the value of replicasets.topology_type {VER(>=8.0.13)}
pm

//@<OUT> WL#12052: Switch a single-primary cluster to single-primary is a no-op {VER(>=8.0.13)}
Switching cluster 'cluster' to Single-Primary mode...

Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' remains PRIMARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' remains SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' remains SECONDARY.

WARNING: Existing connections that expected a R/W connection must be disconnected, i.e. instances that became SECONDARY.

The cluster successfully switched to Single-Primary mode.

//@<OUT> WL#12052: Check cluster status after changing to single-primary no-op {VER(>=8.0.13)}
{
    "clusterName": "cluster",
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
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL#12052: Re-create the multi-primary cluster {VER(>=8.0.13)}
||

//@<OUT> WL#12052: Switch to single-primary mode defining which shall be the primary {VER(>=8.0.13)}
Switching cluster 'cluster' to Single-Primary mode...

Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was switched from PRIMARY to SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' remains PRIMARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was switched from PRIMARY to SECONDARY.

WARNING: The cluster internal session is not the primary member anymore. For cluster management operations please obtain a fresh cluster handle using <Dba>.getCluster().

WARNING: Existing connections that expected a R/W connection must be disconnected, i.e. instances that became SECONDARY.

The cluster successfully switched to Single-Primary mode.

//@<OUT> WL#12052: Check cluster status after switching to single-primary and defining the primary {VER(>=8.0.13)}
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ WL#12052: Finalization
||
