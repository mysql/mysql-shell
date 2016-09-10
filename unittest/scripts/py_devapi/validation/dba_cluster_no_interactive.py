#@ Cluster: validating members
|Cluster Members: 10|
|name: OK|
|get_name: OK|
|admin_type: OK|
|get_admin_type: OK|
|add_instance: OK|
|remove_instance: OK|
|rejoin_instance: OK|
|describe: OK|
|status: OK|
|help: OK|

#@# Cluster: add_instance errors
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 4
||Cluster.add_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.add_instance: Missing instance options: host
||Cluster.add_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.add_instance: Unexpected instance options: authMethod, schema
||Cluster.add_instance: Missing instance options: host, password
||Cluster.add_instance: The instance 'localhost:<<<__mysql_sandbox_port1>>>' already belongs to the ReplicaSet: 'default'

#@ Cluster: add_instance
||

#@<OUT> Cluster: describe1
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

#@<OUT> Cluster: status1
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



#@ Cluster: remove_instance errors
||ArgumentError: Invalid number of arguments in Cluster.remove_instance, expected 1 but got 0
||ArgumentError: Invalid number of arguments in Cluster.remove_instance, expected 1 but got 2
||ArgumentError: Cluster.remove_instance: Invalid connection options, expected either a URI or a Dictionary
||ArgumentError: Cluster.remove_instance: Unexpected instance options: authMethod, schema, user
||ArgumentError: Cluster.remove_instance: The instance 'somehost:3306' does not belong to the ReplicaSet: 'default'

#@ Cluster: remove_instance
||

#@<OUT> Cluster: describe2
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

#@<OUT> Cluster: status2
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

#@ Cluster: remove_instance last
||

#@<OUT> Cluster: describe3
{
    "clusterName": "devCluster",
    "adminType": "local",
    "defaultReplicaSet": {
        "name": "default",
        "instances": []
    }
}

#@<OUT> Cluster: status3
{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "status": "Cluster is NOT tolerant to any failures.",
        "topology": {}
    }
}
