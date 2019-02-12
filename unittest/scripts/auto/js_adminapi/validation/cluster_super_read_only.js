//@ Init
|Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'|
|Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port2>>>'|

//@<ERR> addInstance
Cluster.addInstance: This function is not available through a session to a read only instance (RuntimeError)

//@<ERR> removeInstance
Cluster.removeInstance: This function is not available through a session to a read only instance (RuntimeError)

//@<ERR> rejoinInstance (OK - should fail, but not because of R/O)
Cluster.rejoinInstance: Cannot rejoin instance 'localhost:<<<__mysql_sandbox_port1>>>' to the ReplicaSet 'default' since it is an active (ONLINE) member of the ReplicaSet. (RuntimeError)

//@<ERR> dissolve
Cluster.dissolve: This function is not available through a session to a read only instance (RuntimeError)

//@<ERR> rescan
Cluster.rescan: This function is not available through a session to a read only instance (RuntimeError)

//@<OUT> status (OK)
{
    "clusterName": "cluster",
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

//@<OUT> describe (OK)
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"<<<(__version_num>=80011)?",\n[[*]]\"version\": \"" + __version + "\"":"">>>
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@ disconnect (OK)
||

//@ Fini
||
