#@ Initialization
||

#@<OUT> create cluster
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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

#@ Dba.rebootClusterFromCompleteOutage errors
||Dba.reboot_cluster_from_complete_outage: The cluster name cannot be empty.
||Dba.reboot_cluster_from_complete_outage: The cluster with the name 'dev2' does not exist.
||Dba.reboot_cluster_from_complete_outage: The cluster's instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' belongs to an InnoDB Cluster and is reachable. Please use force_quorum_using_partition_of() to restore the quorum loss

#@ Dba.rebootClusterFromCompleteOutage success
||

#@<OUT> cluster status after reboot
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
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
    }
}

#@ Finalization
||

