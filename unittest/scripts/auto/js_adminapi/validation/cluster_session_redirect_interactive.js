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
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"
}

//@ Fini
||
