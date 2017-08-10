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

#@ Dba.rebootClusterFromCompleteOutage errors
||Dba.reboot_cluster_from_complete_outage: The cluster's instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' belongs to an InnoDB Cluster and is reachable. Please use <Cluster>.force_quorum_using_partition_of() to restore the quorum loss.
||Dba.reboot_cluster_from_complete_outage: Invalid values in the options: invalidOpt

#@ Dba.rebootClusterFromCompleteOutage error unreachable server cannot be on the rejoinInstances list
||Dba.reboot_cluster_from_complete_outage: The following instances: '<<<localhost>>>:<<<__mysql_sandbox_port3>>>' were specified in the rejoinInstances list but are not reachable.

#@ Dba.rebootClusterFromCompleteOutage error cannot use same server on both rejoinInstances and removeInstances list
||Dba.reboot_cluster_from_complete_outage: The following instances: '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' belong to both 'rejoinInstances' and 'removeInstances' lists.

#@<OUT> Dba.rebootClusterFromCompleteOutage: super-read-only error (BUG#26422638)
Reconfiguring the cluster 'dev' from complete outage...

The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' was part of the cluster configuration.
Would you like to rejoin it to the cluster? [y|N]:
Could not open a connection to '<<<localhost>>>:<<<__mysql_sandbox_port3>>>': 'Can't connect to MySQL server on 'localhost' (111)'
Would you like to remove it from the cluster's metadata? [y|N]:
The MySQL instance at '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:
Cancelled

#@<OUT> Dba.rebootClusterFromCompleteOutage success
Reconfiguring the cluster 'dev' from complete outage...

The instance '<<<localhost>>>:<<<__mysql_sandbox_port2>>>' was part of the cluster configuration.
Would you like to rejoin it to the cluster? [y|N]:
Could not open a connection to '<<<localhost>>>:<<<__mysql_sandbox_port3>>>': 'Can't connect to MySQL server on 'localhost' (111)'
Would you like to remove it from the cluster's metadata? [y|N]:
The MySQL instance at '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:

The cluster was successfully rebooted.

#@<OUT> cluster status after reboot
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
    }
}

#@ Add instance 3 back to the cluster
||

#@ Dba.rebootClusterFromCompleteOutage regression test for BUG#25516390
|The cluster was successfully rebooted.|

#@ Finalization
||

