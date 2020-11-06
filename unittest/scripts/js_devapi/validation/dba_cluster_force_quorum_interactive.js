//@ Initialization
||

//@<OUT> create cluster
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
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

//@ Add instance 2
||

//@ Add instance 3
||

//@ Disable group_replication_start_on_boot on second instance {VER(>=8.0.11)}
||

//@ Disable group_replication_start_on_boot on second instance {VER(<8.0.11)}
||

//@ Disable group_replication_start_on_boot on third instance {VER(>=8.0.11)}
||

//@ Disable group_replication_start_on_boot on third instance {VER(<8.0.11)}
||

//@ Kill instance 2
||

//@ Kill instance 3
||

//@ Start instance 2
||

//@ Start instance 3
||

//@<OUT> Cluster status
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' and cannot process write transactions. 2 members are not active.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "instanceErrors": [
                    "NOTE: group_replication is stopped."
                ], 
                "memberState": "OFFLINE", 
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "instanceErrors": [
                    "NOTE: group_replication is stopped."
                ], 
                "memberState": "OFFLINE", 
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Cluster.forceQuorumUsingPartitionOf errors
||Cluster.forceQuorumUsingPartitionOf: Invalid number of arguments, expected 1 to 2 but got 0
||Cluster.forceQuorumUsingPartitionOf: Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary
||Cluster.forceQuorumUsingPartitionOf: Argument #1: Invalid URI: empty.
||Cluster.forceQuorumUsingPartitionOf: Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary

//@ Cluster.forceQuorumUsingPartitionOf error interactive
||Cluster.forceQuorumUsingPartitionOf: The instance 'localhost:<<<__mysql_sandbox_port2>>>' cannot be used to restore the cluster as it is not an active member of replication group.

//@<OUT> Cluster.forceQuorumUsingPartitionOf success
Restoring cluster 'dev' from loss of quorum, by using the partition composed of [<<<hostname>>>:<<<__mysql_sandbox_port1>>>]

Restoring the InnoDB cluster ...

Please provide the password for 'root@localhost:<<<__mysql_sandbox_port1>>>': The InnoDB cluster was successfully restored using the partition from the instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

WARNING: To avoid a split-brain scenario, ensure that all other members of the cluster are removed or joined back to the group that was restored.

//@<OUT> Cluster status after force quorum
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "instanceErrors": [
                    "NOTE: group_replication is stopped."
                ], 
                "memberState": "OFFLINE",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "instanceErrors": [
                    "NOTE: group_replication is stopped."
                ], 
                "memberState": "OFFLINE", 
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Rejoin instance 2
||

//@ Rejoin instance 3
||

//@<OUT> Cluster status after rejoins
{
    "clusterName": "dev",
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
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
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

//@ Finalization
||
