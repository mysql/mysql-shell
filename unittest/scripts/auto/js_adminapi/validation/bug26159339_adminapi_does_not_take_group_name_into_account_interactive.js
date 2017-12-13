//@<OUT> Cluster state
{
    "clusterName": "clus",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
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
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@localhost:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Should have 2 members ONLINE and one missing
{
    "clusterName": "clus",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures. 1 member is not active",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
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
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "(MISSING)"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@localhost:<<<__mysql_sandbox_port1>>>"
}

//@ Rescan
|Rescanning the cluster...|
|The instance 'localhost:<<<__mysql_sandbox_port3>>>' is no longer part of the HA setup. It is either offline or left the HA group.|

//@# Try to rejoin it (error)
||Cluster.rejoinInstance: The instance 'localhost:<<<__mysql_sandbox_port3>>>' may belong to a different ReplicaSet as the one registered in the Metadata since the value of 'group_replication_group_name' does not match the one registered in the ReplicaSet's Metadata: possible split-brain scenario. Please remove the instance from the cluster.

//@ getCluster() where the member we're connected to has a mismatched group_name vs metadata
||

//@# check error
||Dba.getCluster: Unable to get a InnoDB cluster handle. The instance 'localhost:<<<__mysql_sandbox_port1>>>' may belong to a different ReplicaSet as the one registered in the Metadata since the value of 'group_replication_group_name' does not match the one registered in the ReplicaSet's Metadata: possible split-brain scenario. Please connect to another member of the ReplicaSet to get the Cluster.

//@ Cleanup
||
