//@ Deploy 2 instances, 1 for base cluster and another for intruder
||

//@# 1- Rejoin on a active member from different group
||Cluster.rejoinInstance: The instance 'localhost:<<<__mysql_sandbox_port3>>>' does not belong to the ReplicaSet: 'default'.

//@# 1- Add on active member from a different group
||Cluster.addInstance: The instance 'localhost:<<<__mysql_sandbox_port3>>>' is already part of another InnoDB cluster

//@ Stop GR
|Query OK|

//@ Clear sro
|Query OK|

//@# 3- Rejoin on inactive member from different group
||Cluster.rejoinInstance: The instance 'localhost:<<<__mysql_sandbox_port3>>>' does not belong to the ReplicaSet: 'default'.

////@# 3- Add on inactive member from a different group

//@ Drop MD schema
|Query OK|

//@# 4- Rejoin on non-cluster inactive member from different group
||Cluster.rejoinInstance: The instance 'localhost:<<<__mysql_sandbox_port3>>>' does not belong to the ReplicaSet: 'default'.

////@# 4- Add on non-cluster inactive member from a different group

//@ Start back GR
|Query OK|

//@# 2- Rejoin on non-cluster active member from different group
||Cluster.rejoinInstance: The instance 'localhost:<<<__mysql_sandbox_port3>>>' does not belong to the ReplicaSet: 'default'.

//@# 2- Add on non-cluster active member from a different group
||Cluster.addInstance: The instance 'localhost:<<<__mysql_sandbox_port3>>>' is already part of another Replication Group


//----

//@ Preparation
||

//@ Remove the persist group_replication_group_name {VER(>=8.0.11)}
||

//@ Kill instance 2
||

//@<OUT> status() on no-quorum
{
    "clusterName": "clus",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "NO_QUORUM",
        "statusText": "Cluster has no quorum as visible from 'localhost:<<<__mysql_sandbox_port1>>>' and cannot process write transactions. 1 member is not active",
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
                "mode": "n/a",
                "readReplicas": {},
                "role": "HA",
                "status": "UNREACHABLE"
            }
        }
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Change the group_name of instance 2 and start it back
[
    [
        "ffd94a44-cce1-11e7-987e-4cfc0b4022e7"
    ]
]

//@# forceQuorum
||Cluster.forceQuorumUsingPartitionOf: The instance 'localhost:<<<__mysql_sandbox_port2>>>' cannot be used to restore the cluster as it may belong to a different ReplicaSet as the one registered in the Metadata since the value of 'group_replication_group_name' does not match the one registered in the ReplicaSet's Metadata: possible split-brain scenario.

//@ Cleanup
||
