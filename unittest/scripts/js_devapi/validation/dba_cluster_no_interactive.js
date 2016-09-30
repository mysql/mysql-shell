//@ Cluster: validating members
|Cluster Members: 11|
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
|dissolve: OK|


//@# Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 4
||Cluster.addInstance: Invalid connection options, expected either a URI, a Dictionary or an Instance object
||Cluster.addInstance: Missing instance options: host
||Cluster.addInstance: Invalid connection options, expected either a URI, a Dictionary or an Instance object
||Cluster.addInstance: Unexpected instance options: authMethod, schema
||Cluster.addInstance: Missing instance options: host, password
||Cluster.addInstance: The instance '<<<__hostname>>>:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'

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
                "name": "<<<__hostname>>>:<<<__mysql_sandbox_port1>>>",
                "host": "<<<__hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "name": "<<<__hostname>>>:<<<__mysql_sandbox_port2>>>",
                "host": "<<<__hostname>>>:<<<__mysql_sandbox_port2>>>",
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
            "<<<__hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<__hostname>>>:<<<__mysql_sandbox_port1>>>",
                "status": "ONLINE",
                "role": "HA",
                "mode": "R/W",
                "leaves": {
                    "<<<__hostname>>>:<<<__mysql_sandbox_port2>>>": {
                        "address": "<<<__hostname>>>:<<<__mysql_sandbox_port2>>>",
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
||Invalid number of arguments in Cluster.removeInstance, expected 1 but got 0
||Invalid number of arguments in Cluster.removeInstance, expected 1 but got 2
||Cluster.removeInstance: Invalid connection options, expected either a URI, a Dictionary or an Instance object
||Cluster.removeInstance: Unexpected instance options: authMethod, schema, user
||Cluster.removeInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

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
                "name": "<<<__hostname>>>:<<<__mysql_sandbox_port1>>>",
                "host": "<<<__hostname>>>:<<<__mysql_sandbox_port1>>>",
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
            "<<<__hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<__hostname>>>:<<<__mysql_sandbox_port1>>>",
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

//@ Cluster: dissolve errors
||Cluster.dissolve: Cannot drop cluster: The cluster is not empty
||Cluster.dissolve: Argument #1 is expected to be a map
||Invalid number of arguments in Cluster.dissolve, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: The options contain the following invalid attributes: foobar
||Cluster.dissolve: Invalid data type for 'force' field, should be a boolean

//@ Cluster: dissolve
||

//@ Cluster: describe: dissolved cluster
||The cluster 'devCluster' no longer exists.

//@ Cluster: status: dissolved cluster
||The cluster 'devCluster' no longer exists.
