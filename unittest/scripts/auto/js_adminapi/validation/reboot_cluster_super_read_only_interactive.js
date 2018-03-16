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
    },
    "groupInformationSourceMember": "mysql://root@localhost:<<<__mysql_sandbox_port1>>>"
}

//@ status after stop GR - error
||Cluster.status: This function is not available through a session to a standalone instance

//@ getCluster() - error
||Dba.getCluster: This function is not available through a session to a standalone instance (metadata exists, but GR is not active)

//@<OUT> No flag, yes on prompt
Reconfiguring the cluster 'dev' from complete outage...

The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only
system variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

Note: there are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y/N]:

//@<OUT> No flag, yes on prompt {VER(>=8.0.5)}
The cluster was successfully rebooted.

//@<OUT> No flag, yes on prompt {VER(<8.0.5)}
WARNING: On instance 'localhost:<<<__mysql_sandbox_port1>>>' membership change cannot be persisted since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.5 required). Please use the <Dba>.configureLocalInstance command locally to persist the changes.

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

Do you want to disable super_read_only and continue? [y/N]:
Cancelled

//@ Invalid flag value
||Dba.rebootClusterFromCompleteOutage: Argument 'clearReadOnly' is expected to be a bool

//@<OUT> Flag false
Reconfiguring the cluster 'dev' from complete outage...

ERROR: The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system variable set to protect it from inadvertent updates from applications. You must first unset it to be able to perform any changes to this instance. For more information see: https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

//@<ERR> Flag false
Dba.rebootClusterFromCompleteOutage: Server in SUPER_READ_ONLY mode (RuntimeError)

//@ Flag true
|Reconfiguring the cluster 'dev' from complete outage...|
|The cluster was successfully rebooted.|
