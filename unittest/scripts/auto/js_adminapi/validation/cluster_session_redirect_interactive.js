//@ Init
||

//@ Connect to secondary, add instance, remove instance (should work)
|Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port2>>>'|

//@ Connect to secondary without redirect, add instance, remove instance (fail)
|Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port2>>>'|

//@<ERR> addInstance should fail
Cluster.addInstance: This function is not available through a session to a read only instance (RuntimeError)

//@<ERR> removeInstance should fail
Cluster.removeInstance: This function is not available through a session to a read only instance (RuntimeError)

//@<OUT> status should work
{
    "clusterName": "clus",
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
            },
            "localhost:<<<__mysql_sandbox_port2>>>": {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<real_hostname>>>:<<<__mysql_sandbox_port2>>>"
}

//@ Fini
||
