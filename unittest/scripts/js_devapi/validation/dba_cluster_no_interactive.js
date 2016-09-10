//@ Cluster: validating members
|Cluster Members: 10|
|name: OK|
|getName: OK|
|adminType: OK|
|getAdminType: OK|
|addInstance: OK|
|removeInstance: OK|
|rejoinInstance: OK|
|describe: OK|
|status: OK|
|help: OK|


//@# Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 4
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Missing instance options: host
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Unexpected instance options: authMethod, schema
||Cluster.addInstance: Missing instance options: host, password
||Cluster.addInstance: The instance 'localhost:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'

//@ Cluster: addInstance
||

//@<OUT> Cluster: describe1
{
    "clusterName": "devCluster",
    "adminType": "local",
    "defaultReplicaSet": {
        "name": "default",
        "instances": [
            {
                "name": "localhost:<<<__mysql_sandbox_port1>>>",
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "name": "localhost:<<<__mysql_sandbox_port2>>>",
                "host": "localhost:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ]
    }
}

//@<OUT> Cluster: status1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "status": "ONLINE",
                "role": "HA",
                "mode": "R/W",
                "leaves": {
                    "localhost:<<<__mysql_sandbox_port2>>>": {
                        "address": "localhost:<<<__mysql_sandbox_port2>>>",
                        "status": "RECOVERING",
                        "role": "HA",
                        "mode": "R/O",
                        "leaves": {}
                    }
                }
            }
        }
    }
}



//@ Cluster: removeInstance errors
||ArgumentError: Invalid number of arguments in Cluster.removeInstance, expected 1 but got 0
||ArgumentError: Invalid number of arguments in Cluster.removeInstance, expected 1 but got 2
||ArgumentError: Cluster.removeInstance: Invalid connection options, expected either a URI or a Dictionary
||ArgumentError: Cluster.removeInstance: Unexpected instance options: authMethod, schema, user
||ArgumentError: Cluster.removeInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

//@ Cluster: removeInstance
||

//@<OUT> Cluster: describe2
{
    "clusterName": "devCluster",
    "adminType": "local",
    "defaultReplicaSet": {
        "name": "default",
        "instances": [
            {
                "name": "localhost:<<<__mysql_sandbox_port1>>>",
                "host": "localhost:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            }
        ]
    }
}

//@<OUT> Cluster: status2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "localhost:<<<__mysql_sandbox_port1>>>": {
                "address": "localhost:<<<__mysql_sandbox_port1>>>",
                "status": "ONLINE",
                "role": "HA",
                "mode": "R/W",
                "leaves": {}
            }
        }
    }
}


//@ Cluster: remove_instance last
||

//@<OUT> Cluster: describe3
{
    "clusterName": "devCluster",
    "adminType": "local",
    "defaultReplicaSet": {
        "name": "default",
        "instances": []
    }
}

//@<OUT> Cluster: status3
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {}
    }
}
