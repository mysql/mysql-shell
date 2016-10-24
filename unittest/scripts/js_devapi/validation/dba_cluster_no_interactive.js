//@ Cluster: validating members
|Cluster Members: 12|
|name: OK|
|getName: OK|
|adminType: OK|
|getAdminType: OK|
|addInstance: OK|
|removeInstance: OK|
|rejoinInstance: OK|
|checkInstanceState: OK|
|describe: OK|
|status: OK|
|help: OK|
|dissolve: OK|

//@# Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 4
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Connection definition is empty
||Cluster.addInstance: Invalid and missing values in instance definition (invalid: weird), (missing: host)
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Invalid values in instance definition: authMethod, schema
||Cluster.addInstance: Missing values in instance definition: host
||Cluster.addInstance: The instance '<<<localhost>>>:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'

//@ Cluster: addInstance
||

//@<OUT> Cluster: describe1
{
    "adminType": "local",
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

//@<OUT> Cluster: status1
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "leaves": {
                    "<<<localhost>>>:<<<__mysql_sandbox_port2>>>": {
                        "address": "<<<localhost>>>:<<<__mysql_sandbox_port2>>>",
                        "leaves": {

                        },
                        "mode": "R/O",
                        "role": "HA",
                        "status": "RECOVERING"
                    }
                },
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}



//@ Cluster: removeInstance errors
||Invalid number of arguments in Cluster.removeInstance, expected 1 but got 0
||Invalid number of arguments in Cluster.removeInstance, expected 1 but got 2
||Cluster.removeInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.removeInstance: Invalid values in instance definition: authMethod, schema, user
||Cluster.removeInstance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

//@ Cluster: removeInstance
||

//@<OUT> Cluster: describe2
{
    "adminType": "local",
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
            {
                "host": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "name": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            }
        ],
        "name": "default"
    }
}

//@<OUT> Cluster: status2
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "leaves": {

                },
                "mode": "R/W",
                "role": "HA",
                "status": "ONLINE"
            }
        }
    }
}


//@ Cluster: remove_instance last
||

//@<OUT> Cluster: describe3
{
    "adminType": "local",
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "instances": [
        ],
        "name": "default"
    }
}

//@<OUT> Cluster: status3
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {

        }
    }
}

//@ Cluster: dissolve errors
||Cluster.dissolve: Cannot drop cluster: The cluster is not empty
||Cluster.dissolve: Argument #1 is expected to be a map
||Invalid number of arguments in Cluster.dissolve, expected 0 to 1 but got 2
||Cluster.dissolve: Argument #1 is expected to be a map
||Cluster.dissolve: Invalid values in dissolve options: foobar
||Cluster.dissolve: Argument 'force' is expected to be a bool

//@ Cluster: dissolve
||

//@ Cluster: describe: dissolved cluster
||The cluster 'devCluster' no longer exists.

//@ Cluster: status: dissolved cluster
||The cluster 'devCluster' no longer exists.
