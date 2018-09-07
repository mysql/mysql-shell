//@<OUT> Init
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@<ERR> addInstance
Cluster.addInstance: There is no quorum to perform the operation (RuntimeError)

//@<ERR> removeInstance
Cluster.removeInstance: There is no quorum to perform the operation (RuntimeError)

//@<ERR> rejoinInstance
Cluster.rejoinInstance: There is no quorum to perform the operation (RuntimeError)

//@<ERR> dissolve
Cluster.dissolve: There is no quorum to perform the operation (RuntimeError)

//@<ERR> checkInstanceState
Cluster.checkInstanceState: There is no quorum to perform the operation (RuntimeError)

//@<ERR> rescan
Cluster.rescan: There is no quorum to perform the operation (RuntimeError)

//@<OUT> status (OK)
{
    "clusterName": "cluster",
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

//@<OUT> describe (OK)
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "label": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "localhost:<<<__mysql_sandbox_port2>>>",
                "label": "localhost:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ]
    }
}

//@ Fini
||
