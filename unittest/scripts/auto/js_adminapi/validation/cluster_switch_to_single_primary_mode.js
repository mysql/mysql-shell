//@<OUT> WL#12052: Create multi-primary cluster
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@<ERR> WL#12052: Error when executing switchToSinglePrimaryMode on a cluster with members < 8.0.13 {VER(<8.0.13)}
Operation not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ WL#12052: ArgumentErrors of switchToSinglePrimaryMode {VER(>=8.0.13)}
||Argument #1: Invalid URI: empty. (ArgumentError)
||Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
||Argument #1: Invalid connection options, no options provided. (ArgumentError)

//@ WL#12052: Error when executing switchToSinglePrimaryMode on a cluster with 1 or more members not ONLINE
||This operation requires all the cluster members to be ONLINE (RuntimeError)

//@<ERR> WL#12052: Error when executing switchToSinglePrimaryMode on a cluster with no visible quorum < 8.0.13 {VER(>=8.0.13)}
There is no quorum to perform the operation (MYSQLSH 51011)

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
                "memberRole": "PRIMARY",
                "mode": "R/W",
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
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

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
                "memberRole": "PRIMARY",
                "mode": "R/W",
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
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
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

Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' remains PRIMARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was switched from PRIMARY to SECONDARY.
Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' was switched from PRIMARY to SECONDARY.

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
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"
}

//@ WL#12052: Finalization
||
