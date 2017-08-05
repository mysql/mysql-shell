#@ Initialization
||

#@<OUT> create cluster
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
    }
}

#@ Add instance 2
||

#@ Add instance 3
||

#@<OUT> Cluster status
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
    }
}

#@ Cluster.force_quorum_using_partition_of errors
||Invalid number of arguments in Cluster.force_quorum_using_partition_of, expected 1 to 2 but got 0
||Cluster.force_quorum_using_partition_of: Invalid connection options, expected either a URI or a Dictionary
||Cluster.force_quorum_using_partition_of: Invalid URI: empty.
||Cluster.force_quorum_using_partition_of: Invalid connection options, expected either a URI or a Dictionary
||Cluster.force_quorum_using_partition_of: The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' cannot be used to restore the cluster as it is not an active member of replication group.

#@ Cluster.force_quorum_using_partition_of success
||

#@<OUT> Cluster status after force quorum
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
    }
}

#@ Rejoin instance 2
||

#@ Rejoin instance 3
||

#@<OUT> Cluster status after rejoins
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
    }
}

#@ Finalization
||
