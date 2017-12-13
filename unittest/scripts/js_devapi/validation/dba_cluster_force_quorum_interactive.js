//@ Initialization
||

//@<OUT> create cluster
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Add instance 2
||

//@ Add instance 3
||

//@<OUT> Cluster status
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' and cannot process write transactions. 2 members are not active",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Cluster.forceQuorumUsingPartitionOf errors
||Invalid number of arguments in Cluster.forceQuorumUsingPartitionOf, expected 1 to 2 but got 0
||Cluster.forceQuorumUsingPartitionOf: Invalid connection options, expected either a URI or a Dictionary
||Cluster.forceQuorumUsingPartitionOf: Invalid URI: empty.
||Cluster.forceQuorumUsingPartitionOf: Invalid connection options, expected either a URI or a Dictionary

//@ Cluster.forceQuorumUsingPartitionOf error interactive
||Cluster.forceQuorumUsingPartitionOf: The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' cannot be used to restore the cluster as it is not an active member of replication group.

//@<OUT> Cluster.forceQuorumUsingPartitionOf success
Restoring replicaset 'default' from loss of quorum, by using the partition composed of [<<<localhost>>>:<<<__mysql_sandbox_port1>>>]

Please provide the password for 'root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>': Restoring the InnoDB cluster ...

The InnoDB cluster was successfully restored using the partition from the instance 'root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>'.

WARNING: To avoid a split-brain scenario, ensure that all other members of the replicaset are removed or joined back to the group that was restored.

//@<OUT> Cluster status after force quorum
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 2 members are not active",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
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
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port3>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Finalization
||
