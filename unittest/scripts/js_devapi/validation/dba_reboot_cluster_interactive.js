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

//@ Dba.rebootClusterFromCompleteOutage errors
||Dba.rebootClusterFromCompleteOutage: The MySQL instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' belongs to an InnoDB Cluster and is reachable. Please use <Cluster>.forceQuorumUsingPartitionOf() to restore from the quorum loss
||Dba.rebootClusterFromCompleteOutage: Invalid values in the options: invalidOpt

//@ Dba.rebootClusterFromCompleteOutage error unreachable server cannot be on the rejoinInstances list
||Dba.rebootClusterFromCompleteOutage: The following instances: '<<<localhost>>>:<<<__mysql_sandbox_port3>>>' were specified in the rejoinInstances list but are not reachable.

//@ Dba.rebootClusterFromCompleteOutage error cannot use same server on both rejoinInstances and removeInstances list
||Dba.rebootClusterFromCompleteOutage: The following instances: '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' belong to both 'rejoinInstances' and 'removeInstances' lists.

//@ Dba.rebootClusterFromCompleteOutage success
|The cluster was successfully rebooted.|

//@<OUT> cluster status after reboot
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
            },
            "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}
//@ Rescan cluster to add instance 3 back to metadata {VER(>=8.0.5)}
||

//@ Add instance 3 back to the cluster {VER(<=8.0.5)}
||

//@ Dba.rebootClusterFromCompleteOutage regression test for BUG#25516390
|The cluster was successfully rebooted.|

//@ Finalization
||
