//@<OUT> Cluster state
{
    "clusterName": "clus",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:3316",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "localhost:3316": {
                "address": "localhost:3316",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:3326": {
                "address": "localhost:3326",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:3336": {
                "address": "localhost:3336",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

//@<OUT> Should have 2 members ONLINE and one missing
{
    "clusterName": "clus",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:3316",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 1 member is not active",
        "topology": {
            "localhost:3316": {
                "address": "localhost:3316",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:3326": {
                "address": "localhost:3326",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            },
            "localhost:3336": {
                "address": "localhost:3336",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            }
        }
    }
}

//@ Rescan
|Rescanning the cluster...|
|The instance 'localhost:3336' is no longer part of the HA setup. It is either offline or left the HA group.|

//@# Try to rejoin it (error)
||Cluster.rejoinInstance: The instance 'localhost:3336' may belong to a different ReplicaSet as the one registered in the Metadata since the value of 'group_replication_group_name' does not match the one registered in the ReplicaSet's Metadata: possible split-brain scenario. Please remove the instance from the cluster.
