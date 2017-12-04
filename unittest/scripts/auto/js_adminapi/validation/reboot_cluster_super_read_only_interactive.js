//@ Make a cluster with a single node then stop GR to simulate a dead cluster
|<Cluster:dev>|

//@<OUT> status before stop GR
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "localhost:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}

//@# status after stop GR - error
||Cluster.status: This function is not available through a session to a standalone instance

//@# getCluster() - error
||Dba.getCluster: This function is not available through a session to a standalone instance

//@<OUT> No flag, yes on prompt
Reconfiguring the cluster 'dev' from complete outage...

The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:

The cluster was successfully rebooted.

//@<OUT> No flag, no on prompt
Reconfiguring the cluster 'dev' from complete outage...

The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y|N]:
Cancelled

//@# Invalid flag value
||Dba.rebootClusterFromCompleteOutage: Argument 'clearReadOnly' is expected to be a bool

//@# Flag false
||Dba.rebootClusterFromCompleteOutage: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system variable set to protect it from inadvertent updates from applications. You must first unset it to be able to perform any changes to this instance. For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only. If you unset super_read_only you should consider closing the following: 1 open session(s) of 'root@localhost'.  (RuntimeError)

//@<OUT> Flag true
Reconfiguring the cluster 'dev' from complete outage...


The cluster was successfully rebooted.
