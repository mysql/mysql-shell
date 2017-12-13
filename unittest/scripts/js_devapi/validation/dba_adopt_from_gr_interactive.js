//@ Initialization
||

//@ Create cluster
||

//@ Adding instance to cluster
||

//@<OUT> Drop Metadata
Are you sure you want to remove the Metadata? [y|N]:  [y|N]: Metadata Schema successfully removed.

//@ Check cluster status after drop metadata schema
||Cluster.status: This function is not available through a session to an instance belonging to an unmanaged replication group (RuntimeError)

//@<OUT> Create cluster adopting from GR
You are connected to an instance that belongs to an unmanaged replication group.
Do you want to setup an InnoDB cluster based on this replication group? [Y|n]: A new InnoDB cluster will be created on instance 'root@<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Creating InnoDB cluster 'testCluster' on 'root@<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@<OUT> Check cluster status
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "DISABLED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
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
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ Finalization
||
